#include "OrganoidAIBridgePlaytest.h"
#include "OrganoidAIBridgePlaytestHost.h"
#include "OrganoidAIBridgeJson.h"

using OrganoidAIBridgeJson::Fail;
using OrganoidAIBridgeJson::GetString;

namespace
{
	IOrganoidPlaytestHost* GPlaytestHost = nullptr;
}

namespace OrganoidAIBridgePlaytest
{
	void RegisterHost(IOrganoidPlaytestHost* Host)
	{
		GPlaytestHost = Host;
	}

	IOrganoidPlaytestHost* GetHost()
	{
		return GPlaytestHost;
	}

	bool IsPlaytestCommand(const FString& NormalizedCommand)
	{
		return NormalizedCommand == TEXT("list_playtests")
			|| NormalizedCommand == TEXT("run_playtest")
			|| NormalizedCommand == TEXT("get_playtest_status")
			|| NormalizedCommand == TEXT("get_playtest_result")
			|| NormalizedCommand == TEXT("stop_playtest");
	}

	TSharedRef<FJsonObject> Dispatch(
		const FString& NormalizedCommand,
		const TSharedPtr<FJsonObject>& Args)
	{
		IOrganoidPlaytestHost* Host = GetHost();
		if (!Host)
		{
			return Fail(
				TEXT("playtest_unavailable"),
				TEXT("ProjectOrganoidPlaytest is not registered. Rebuild the Editor target and restart Unreal. Playtest commands do not mutate the project."));
		}

		if (NormalizedCommand == TEXT("list_playtests"))
		{
			return Host->ListPlaytests();
		}
		if (NormalizedCommand == TEXT("run_playtest"))
		{
			const FString TestId = GetString(Args, TEXT("test_id"), GetString(Args, TEXT("id")));
			return Host->RunPlaytest(TestId);
		}
		if (NormalizedCommand == TEXT("get_playtest_status"))
		{
			return Host->GetStatus(GetString(Args, TEXT("run_id")));
		}
		if (NormalizedCommand == TEXT("get_playtest_result"))
		{
			return Host->GetResult(GetString(Args, TEXT("run_id")));
		}
		if (NormalizedCommand == TEXT("stop_playtest"))
		{
			return Host->Stop(GetString(Args, TEXT("run_id")));
		}

		return Fail(TEXT("unknown_command"), TEXT("Not a playtest command."));
	}
}
