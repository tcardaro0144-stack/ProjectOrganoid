#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FOrganoidAIBridgeLogSink;

class FOrganoidAIBridgeCommands
{
public:
	static TSharedRef<FJsonObject> Dispatch(
		const FString& Command,
		const TSharedPtr<FJsonObject>& Args,
		const TSharedPtr<FJsonObject>& Session,
		FOrganoidAIBridgeLogSink* LogSink);
};
