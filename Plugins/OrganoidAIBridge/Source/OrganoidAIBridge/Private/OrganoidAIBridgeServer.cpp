#include "OrganoidAIBridgeServer.h"
#include "OrganoidAIBridgeCommands.h"
#include "OrganoidAIBridgeJson.h"
#include "OrganoidAIBridgeLogSink.h"

#include "Async/Async.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "HttpPath.h"
#include "HttpRequestHandler.h"
#include "HttpRouteHandle.h"
#include "HttpServerConstants.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "IPAddress.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FOrganoidAIBridgeServer::FOrganoidAIBridgeServer() = default;

FOrganoidAIBridgeServer::~FOrganoidAIBridgeServer()
{
	Stop();
}

bool FOrganoidAIBridgeServer::Start()
{
	int32 PortInt = 8732;
	int32 MaxLogLines = 2000;
	const FString ConfigPath = FPaths::Combine(
		FPaths::ProjectPluginsDir(),
		TEXT("OrganoidAIBridge/Config/DefaultOrganoidAIBridge.ini"));
	GConfig->LoadFile(ConfigPath);
	GConfig->GetInt(TEXT("OrganoidAIBridge"), TEXT("ListenPort"), PortInt, ConfigPath);
	GConfig->GetInt(TEXT("OrganoidAIBridge"), TEXT("MaxLogLines"), MaxLogLines, ConfigPath);
	GConfig->GetBool(TEXT("OrganoidAIBridge"), TEXT("ReadOnly"), bReadOnly, ConfigPath);

	FString EnvPort = FPlatformMisc::GetEnvironmentVariable(TEXT("ORGANOID_BRIDGE_PORT"));
	if (!EnvPort.IsEmpty())
	{
		PortInt = FCString::Atoi(*EnvPort);
	}
	AuthToken = FPlatformMisc::GetEnvironmentVariable(TEXT("ORGANOID_BRIDGE_TOKEN"));
	Port = static_cast<uint32>(FMath::Clamp(PortInt, 1024, 65535));

	LogSink = MakeUnique<FOrganoidAIBridgeLogSink>(MaxLogLines);
	if (GLog)
	{
		GLog->AddOutputDevice(LogSink.Get());
	}

	Router = FHttpServerModule::Get().GetHttpRouter(Port, true);
	if (!Router.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[OrganoidAIBridge] Failed to bind 127.0.0.1:%u"), Port);
		return false;
	}

	HealthRoute = Router->BindRoute(
		FHttpPath(TEXT("/health")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateSP(this, &FOrganoidAIBridgeServer::HandleHealth));
	CommandRoute = Router->BindRoute(
		FHttpPath(TEXT("/v1/command")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpRequestHandler::CreateSP(this, &FOrganoidAIBridgeServer::HandleCommand));

	FHttpServerModule::Get().StartAllListeners();
	UE_LOG(LogTemp, Log, TEXT("[OrganoidAIBridge] Listening on 127.0.0.1:%u (read-only=%s)"),
		Port, bReadOnly ? TEXT("true") : TEXT("false"));
	return HealthRoute.IsValid() && CommandRoute.IsValid();
}

void FOrganoidAIBridgeServer::Stop()
{
	if (Router.IsValid())
	{
		if (HealthRoute.IsValid())
		{
			Router->UnbindRoute(HealthRoute);
			HealthRoute.Reset();
		}
		if (CommandRoute.IsValid())
		{
			Router->UnbindRoute(CommandRoute);
			CommandRoute.Reset();
		}
	}
	Router.Reset();
	if (LogSink.IsValid() && GLog)
	{
		GLog->RemoveOutputDevice(LogSink.Get());
	}
	LogSink.Reset();
}

bool FOrganoidAIBridgeServer::IsLoopbackPeer(const FHttpServerRequest& Request) const
{
	if (!Request.PeerAddress.IsValid())
	{
		return false;
	}
	const FString Peer = Request.PeerAddress->ToString(false);
	return Peer.StartsWith(TEXT("127.0.0.1"))
		|| Peer.StartsWith(TEXT("::1"))
		|| Peer.Contains(TEXT("localhost"));
}

void FOrganoidAIBridgeServer::ReplyJson(const FHttpResultCallback& OnComplete, int32 HttpCode, const FString& Json)
{
	TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(Json, TEXT("application/json"));
	Response->Code = static_cast<EHttpServerResponseCodes>(HttpCode);
	OnComplete(MoveTemp(Response));
}

bool FOrganoidAIBridgeServer::HandleHealth(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	if (!IsLoopbackPeer(Request))
	{
		ReplyJson(OnComplete, 403, OrganoidAIBridgeJson::ToString(OrganoidAIBridgeJson::Fail(TEXT("forbidden"), TEXT("localhost only"))));
		return true;
	}
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetBoolField(TEXT("ok"), true);
	Data->SetStringField(TEXT("service"), TEXT("OrganoidAIBridge"));
	Data->SetStringField(TEXT("version"), OrganoidAIBridgeJson::BridgeVersion());
	Data->SetNumberField(TEXT("port"), Port);
	Data->SetBoolField(TEXT("read_only"), bReadOnly);
	Data->SetBoolField(TEXT("game_thread_marshal"), true);
	ReplyJson(OnComplete, 200, OrganoidAIBridgeJson::ToString(Data));
	return true;
}

bool FOrganoidAIBridgeServer::HandleCommand(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	if (!IsLoopbackPeer(Request))
	{
		ReplyJson(OnComplete, 403, OrganoidAIBridgeJson::ToString(OrganoidAIBridgeJson::Fail(TEXT("forbidden"), TEXT("localhost only"))));
		return true;
	}

	if (!AuthToken.IsEmpty())
	{
		const TArray<FString>* TokenHeader = Request.Headers.Find(TEXT("X-Organoid-Bridge-Token"));
		const FString Provided = (TokenHeader && TokenHeader->Num() > 0) ? (*TokenHeader)[0] : TEXT("");
		if (Provided != AuthToken)
		{
			ReplyJson(OnComplete, 401, OrganoidAIBridgeJson::ToString(OrganoidAIBridgeJson::Fail(TEXT("unauthorized"), TEXT("Invalid bridge token."))));
			return true;
		}
	}

	TArray<uint8> Copy = Request.Body;
	Copy.Add(0);
	const FString Body = UTF8_TO_TCHAR(reinterpret_cast<const char*>(Copy.GetData()));
	TSharedPtr<FJsonObject> Root = OrganoidAIBridgeJson::ParseObject(Body);
	if (!Root.IsValid())
	{
		ReplyJson(OnComplete, 400, OrganoidAIBridgeJson::ToString(OrganoidAIBridgeJson::Fail(TEXT("bad_json"), TEXT("Request body must be JSON."))));
		return true;
	}

	const FString Command = OrganoidAIBridgeJson::GetString(Root, TEXT("command"));
	const TSharedPtr<FJsonObject>* ArgsPtr = nullptr;
	TSharedPtr<FJsonObject> Args;
	if (Root->TryGetObjectField(TEXT("args"), ArgsPtr) && ArgsPtr)
	{
		Args = *ArgsPtr;
	}
	const TSharedPtr<FJsonObject>* SessionPtr = nullptr;
	TSharedPtr<FJsonObject> Session;
	if (Root->TryGetObjectField(TEXT("session"), SessionPtr) && SessionPtr)
	{
		Session = *SessionPtr;
	}
	else
	{
		Session = MakeShared<FJsonObject>();
		Session->SetBoolField(TEXT("read_only"), bReadOnly);
	}

	const TSharedRef<FJsonObject> Result = [&]()
	{
		auto DispatchNow = [&]()
		{
			return FOrganoidAIBridgeCommands::Dispatch(
				Command,
				Args,
				Session,
				LogSink.Get());
		};

		if (IsInGameThread())
		{
			return DispatchNow();
		}

		TSharedPtr<FJsonObject> GameThreadResult;
		FEvent* Done = FPlatformProcess::GetSynchEventFromPool(true);
		AsyncTask(ENamedThreads::GameThread, [Command, Args, Session, LogSinkRaw = LogSink.Get(), &GameThreadResult, Done]()
		{
			GameThreadResult = FOrganoidAIBridgeCommands::Dispatch(Command, Args, Session, LogSinkRaw);
			Done->Trigger();
		});
		Done->Wait();
		FPlatformProcess::ReturnSynchEventToPool(Done);
		if (!GameThreadResult.IsValid())
		{
			return OrganoidAIBridgeJson::Fail(
				TEXT("game_thread_failed"),
				TEXT("Game-thread dispatch returned no result."));
		}
		return GameThreadResult.ToSharedRef();
	}();
	const bool bOk = Result->GetBoolField(TEXT("ok"));
	ReplyJson(OnComplete, bOk ? 200 : 400, OrganoidAIBridgeJson::ToString(Result));
	return true;
}
