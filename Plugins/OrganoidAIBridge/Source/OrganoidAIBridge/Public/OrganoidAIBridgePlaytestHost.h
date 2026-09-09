#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class IOrganoidPlaytestHost
{
public:
	virtual ~IOrganoidPlaytestHost() = default;
	virtual TSharedRef<FJsonObject> ListPlaytests() = 0;
	virtual TSharedRef<FJsonObject> RunPlaytest(const FString& TestId) = 0;
	virtual TSharedRef<FJsonObject> GetStatus(const FString& RunId) = 0;
	virtual TSharedRef<FJsonObject> GetResult(const FString& RunId) = 0;
	virtual TSharedRef<FJsonObject> Stop(const FString& RunId) = 0;
};

namespace OrganoidAIBridgePlaytest
{
	ORGANOIDAIBRIDGE_API void RegisterHost(IOrganoidPlaytestHost* Host);
	ORGANOIDAIBRIDGE_API IOrganoidPlaytestHost* GetHost();
}
