#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FOrganoidAIBridgeLogSink;

namespace OrganoidAIBridgeWrites
{
	bool IsLifecycleCommand(const FString& NormalizedCommand);

	TSharedRef<FJsonObject> Dispatch(
		const FString& NormalizedCommand,
		const TSharedPtr<FJsonObject>& Args,
		const TSharedPtr<FJsonObject>& Session,
		FOrganoidAIBridgeLogSink* LogSink);
}
