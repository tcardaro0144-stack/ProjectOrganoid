// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDiagnosticsUploader.h"
#include "ProjectOrganoidTelemetrySubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformProperties.h"
#include "Engine/GameInstance.h"
#include "ProjectOrganoid.h"

void UProjectOrganoidDiagnosticsUploader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UProjectOrganoidTelemetrySubsystem* Telemetry = GetGameInstance()->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
	{
		Telemetry->OnCrashReportWritten.AddDynamic(this, &UProjectOrganoidDiagnosticsUploader::HandleCrashReportWritten);
	}
}

void UProjectOrganoidDiagnosticsUploader::Deinitialize()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidTelemetrySubsystem* Telemetry = GI->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
		{
			Telemetry->OnCrashReportWritten.RemoveDynamic(this, &UProjectOrganoidDiagnosticsUploader::HandleCrashReportWritten);
		}
	}
	Super::Deinitialize();
}

FString UProjectOrganoidDiagnosticsUploader::GetDiagnosticsRoot() const
{
	return FPaths::ProjectSavedDir() / TEXT("ProjectOrganoid") / TEXT("Diagnostics");
}

void UProjectOrganoidDiagnosticsUploader::HandleCrashReportWritten(const FString& FilePath)
{
	UE_LOG(LogProjectOrganoid, Warning, TEXT("Diagnostics uploader received crash report: %s"), *FilePath);
	if (bAutoUploadOnCrash)
	{
		PackageAndUpload(TEXT("CrashAuto"));
	}
	else
	{
		PackageDiagnosticsArchive(TEXT("CrashLocal"));
	}
}

void UProjectOrganoidDiagnosticsUploader::WriteManifest(const FString& PackageDir, const FString& Reason) const
{
	FString SafeReason = Reason;
	SafeReason.ReplaceInline(TEXT("\""), TEXT("'"));
	SafeReason.ReplaceInline(TEXT("\\"), TEXT("/"));

	const FString Manifest = FString::Printf(
		TEXT("{\n")
		TEXT("  \"project\": \"ProjectOrganoid\",\n")
		TEXT("  \"reason\": \"%s\",\n")
		TEXT("  \"timestamp\": \"%s\",\n")
		TEXT("  \"platform\": \"%s\",\n")
		TEXT("  \"engine\": \"%s\"\n")
		TEXT("}\n"),
		*SafeReason,
		*FDateTime::UtcNow().ToIso8601(),
		ANSI_TO_TCHAR(FPlatformProperties::PlatformName()),
		*FEngineVersion::Current().ToString());

	FFileHelper::SaveStringToFile(Manifest, *(PackageDir / TEXT("manifest.json")));
}

bool UProjectOrganoidDiagnosticsUploader::CopyTelemetryFiles(const FString& PackageDir) const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString TelemetryDir = FPaths::ProjectSavedDir() / TEXT("ProjectOrganoid") / TEXT("Telemetry");
	const FString Dest = PackageDir / TEXT("Telemetry");
	PlatformFile.CreateDirectoryTree(*Dest);

	if (!PlatformFile.DirectoryExists(*TelemetryDir))
	{
		return false;
	}

	TArray<FString> Files;
	PlatformFile.FindFiles(Files, *TelemetryDir, TEXT(".log"));
	PlatformFile.FindFiles(Files, *TelemetryDir, TEXT(".txt"));

	int32 Copied = 0;
	for (const FString& File : Files)
	{
		const FString Target = Dest / FPaths::GetCleanFilename(File);
		if (PlatformFile.CopyFile(*Target, *File))
		{
			++Copied;
		}
	}
	return Copied > 0;
}

bool UProjectOrganoidDiagnosticsUploader::CopyEngineLogs(const FString& PackageDir) const
{
	if (!bIncludeEngineLogs)
	{
		return false;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString LogsDir = FPaths::ProjectLogDir();
	const FString Dest = PackageDir / TEXT("EngineLogs");
	PlatformFile.CreateDirectoryTree(*Dest);

	TArray<FString> Files;
	PlatformFile.FindFiles(Files, *LogsDir, TEXT(".log"));

	int32 Copied = 0;
	for (const FString& File : Files)
	{
		const FString Name = FPaths::GetCleanFilename(File);
		if (Name.Contains(TEXT("ProjectOrganoid")) || Name.Contains(TEXT("Crash")) || Name.EndsWith(TEXT(".log")))
		{
			if (PlatformFile.CopyFile(*(Dest / Name), *File))
			{
				++Copied;
			}
		}
		if (Copied >= 8)
		{
			break;
		}
	}
	return Copied > 0;
}

bool UProjectOrganoidDiagnosticsUploader::TryCompressDirectory(const FString& PackageDir, FString& OutZipPath) const
{
	OutZipPath = PackageDir + TEXT(".zip");
#if PLATFORM_WINDOWS
	const FString Cmd = FString::Printf(
		TEXT("Compress-Archive -Path \"%s\\*\" -DestinationPath \"%s\" -Force"),
		*PackageDir.Replace(TEXT("/"), TEXT("\\")),
		*OutZipPath.Replace(TEXT("/"), TEXT("\\")));

	FString Args = FString::Printf(TEXT("-NoProfile -NonInteractive -Command \"%s\""), *Cmd.Replace(TEXT("\""), TEXT("\\\"")));
	void* ReadPipe = nullptr;
	void* WritePipe = nullptr;
	FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
	FProcHandle Proc = FPlatformProcess::CreateProc(
		TEXT("powershell.exe"),
		*Args,
		true, true, true,
		nullptr, 0,
		nullptr,
		WritePipe, ReadPipe);

	if (Proc.IsValid())
	{
		FPlatformProcess::WaitForProc(Proc);
		int32 Code = 0;
		FPlatformProcess::GetProcReturnCode(Proc, &Code);
		FPlatformProcess::CloseProc(Proc);
		FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		return Code == 0 && FPaths::FileExists(OutZipPath);
	}
	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
#endif
	OutZipPath = PackageDir;
	return false;
}

FString UProjectOrganoidDiagnosticsUploader::PackageDiagnosticsArchive(const FString& Reason)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString Root = GetDiagnosticsRoot();
	PlatformFile.CreateDirectoryTree(*Root);

	const FString Stamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString PackageDir = Root / FString::Printf(TEXT("diag_%s"), *Stamp);
	PlatformFile.CreateDirectoryTree(*PackageDir);

	WriteManifest(PackageDir, Reason);
	CopyTelemetryFiles(PackageDir);
	CopyEngineLogs(PackageDir);

	// Include any crash dumps under Saved/Crashes if present
	const FString CrashesDir = FPaths::ProjectSavedDir() / TEXT("Crashes");
	if (PlatformFile.DirectoryExists(*CrashesDir))
	{
		const FString DestCrashes = PackageDir / TEXT("Crashes");
		PlatformFile.CreateDirectoryTree(*DestCrashes);
		TArray<FString> CrashFiles;
		PlatformFile.FindFilesRecursively(CrashFiles, *CrashesDir, TEXT(""));
		int32 Count = 0;
		for (const FString& File : CrashFiles)
		{
			const FString Rel = File.RightChop(CrashesDir.Len() + 1);
			const FString Target = DestCrashes / Rel;
			PlatformFile.CreateDirectoryTree(*FPaths::GetPath(Target));
			PlatformFile.CopyFile(*Target, *File);
			if (++Count >= 32)
			{
				break;
			}
		}
	}

	FString ZipPath;
	const bool bZipped = TryCompressDirectory(PackageDir, ZipPath);
	const FString ResultPath = bZipped ? ZipPath : PackageDir;
	OnDiagnosticsArchiveReady.Broadcast(ResultPath, true);
	UE_LOG(LogProjectOrganoid, Log, TEXT("Diagnostics archive ready: %s"), *ResultPath);
	return ResultPath;
}

bool UProjectOrganoidDiagnosticsUploader::UploadDiagnosticsArchive(const FString& ArchiveDirectoryOrZip)
{
	if (!bConsentToUpload)
	{
		UE_LOG(LogProjectOrganoid, Warning, TEXT("Diagnostics upload skipped — bConsentToUpload is false"));
		OnDiagnosticsUploadFinished.Broadcast(false, 0, TEXT("ConsentRequired"));
		return false;
	}

	if (UploadEndpointUrl.IsEmpty())
	{
		OnDiagnosticsUploadFinished.Broadcast(false, 0, TEXT("NoEndpoint"));
		return false;
	}

	FString PayloadPath = ArchiveDirectoryOrZip;
	if (FPaths::DirectoryExists(ArchiveDirectoryOrZip))
	{
		FString ZipPath;
		if (TryCompressDirectory(ArchiveDirectoryOrZip, ZipPath))
		{
			PayloadPath = ZipPath;
		}
		else
		{
			// Upload manifest as fallback payload
			PayloadPath = ArchiveDirectoryOrZip / TEXT("manifest.json");
		}
	}

	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *PayloadPath))
	{
		OnDiagnosticsUploadFinished.Broadcast(false, 0, TEXT("ReadFailed"));
		return false;
	}

	if (FileBytes.Num() > MaxArchiveBytes)
	{
		OnDiagnosticsUploadFinished.Broadcast(false, 0, TEXT("ArchiveTooLarge"));
		return false;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(UploadEndpointUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
	Request->SetHeader(TEXT("X-ProjectOrganoid-Diagnostics"), TEXT("1"));
	Request->SetHeader(TEXT("X-Archive-Name"), FPaths::GetCleanFilename(PayloadPath));
	Request->SetContent(MoveTemp(FileBytes));
	Request->OnProcessRequestComplete().BindUObject(this, &UProjectOrganoidDiagnosticsUploader::OnHttpRequestComplete);
	return Request->ProcessRequest();
}

bool UProjectOrganoidDiagnosticsUploader::PackageAndUpload(const FString& Reason)
{
	const FString Archive = PackageDiagnosticsArchive(Reason);
	return UploadDiagnosticsArchive(Archive);
}

void UProjectOrganoidDiagnosticsUploader::OnHttpRequestComplete(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bConnectedSuccessfully)
{
	const int32 Status = Response.IsValid() ? Response->GetResponseCode() : 0;
	const FString Body = Response.IsValid() ? Response->GetContentAsString() : TEXT("NoResponse");
	const bool bOk = bConnectedSuccessfully && Status >= 200 && Status < 300;
	OnDiagnosticsUploadFinished.Broadcast(bOk, Status, Body);
	UE_LOG(LogProjectOrganoid, Log, TEXT("Diagnostics upload finished ok=%d status=%d body=%s"), bOk ? 1 : 0, Status, *Body);
}
