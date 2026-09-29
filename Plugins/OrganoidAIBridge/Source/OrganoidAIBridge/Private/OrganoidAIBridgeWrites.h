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

	TSharedRef<FJsonObject> InspectAdminBlock4NavMesh(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectAlarmPulse(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectEpitopeNavMesh(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectNathanGrantMesh(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectAdminWorldColor(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectNeuroWorldColor(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectCryoWorldColor(const TSharedPtr<FJsonObject>& Args);
	TSharedRef<FJsonObject> InspectComputeWorldColor(const TSharedPtr<FJsonObject>& Args);

	TSharedRef<FJsonObject> InspectReactorWorldColor(const TSharedPtr<FJsonObject>& Args);
}
