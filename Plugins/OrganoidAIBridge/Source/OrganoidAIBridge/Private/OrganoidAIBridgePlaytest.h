#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace OrganoidAIBridgePlaytest
{
	bool IsPlaytestCommand(const FString& NormalizedCommand);

	TSharedRef<FJsonObject> Dispatch(
		const FString& NormalizedCommand,
		const TSharedPtr<FJsonObject>& Args);
}
