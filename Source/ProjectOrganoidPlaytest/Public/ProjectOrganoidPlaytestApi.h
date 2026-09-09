#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/** Bridge-facing playtest API. Returns full {ok,data} / {ok:false,...} JSON objects. */
namespace OrganoidPlaytestApi
{
	PROJECTORGANOIDPLAYTEST_API TSharedRef<FJsonObject> ListPlaytests();
	PROJECTORGANOIDPLAYTEST_API TSharedRef<FJsonObject> RunPlaytest(const FString& TestId);
	PROJECTORGANOIDPLAYTEST_API TSharedRef<FJsonObject> GetPlaytestStatus(const FString& RunId);
	PROJECTORGANOIDPLAYTEST_API TSharedRef<FJsonObject> GetPlaytestResult(const FString& RunId);
	PROJECTORGANOIDPLAYTEST_API TSharedRef<FJsonObject> StopPlaytest(const FString& RunId);
}
