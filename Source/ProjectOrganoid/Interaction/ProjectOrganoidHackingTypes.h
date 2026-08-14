// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidHackingTypes.generated.h"

/** Facility terminal mini-game flavor */
UENUM(BlueprintType)
enum class EProjectOrganoidHackingMiniGame : uint8
{
	NodeMatch UMETA(DisplayName = "Node Matching"),
	PasswordDecrypt UMETA(DisplayName = "Password Decryption")
};

/** Hacking UI finite-state machine */
UENUM(BlueprintType)
enum class EProjectOrganoidHackingUIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Booting UMETA(DisplayName = "Booting"),
	Playing UMETA(DisplayName = "Playing"),
	Success UMETA(DisplayName = "Success"),
	Failed UMETA(DisplayName = "Failed"),
	Closing UMETA(DisplayName = "Closing")
};

/** Designer-tunable puzzle parameters for a terminal session */
USTRUCT(BlueprintType)
struct FProjectOrganoidHackingSessionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking")
	EProjectOrganoidHackingMiniGame MiniGame = EProjectOrganoidHackingMiniGame::NodeMatch;

	/** Node-match: length of the required sequence */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking|NodeMatch", meta = (ClampMin = "2", ClampMax = "12"))
	int32 NodeSequenceLength = 4;

	/** Node-match: how many selectable nodes are shown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking|NodeMatch", meta = (ClampMin = "3", ClampMax = "16"))
	int32 NodePoolSize = 6;

	/** Password decrypt: plaintext target (scrambled for UI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking|Password")
	FString TargetPassword = TEXT("EPITOPE");

	/** Password decrypt: correct guesses needed to finish */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking|Password", meta = (ClampMin = "1", ClampMax = "20"))
	int32 DecryptStepsRequired = 4;

	/** Shared attempt budget before Forced Fail */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking", meta = (ClampMin = "1", ClampMax = "20"))
	int32 MaxAttempts = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking", meta = (ClampMin = "0.0"))
	float BootDurationSeconds = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hacking", meta = (ClampMin = "0.0"))
	float ResultHoldSeconds = 1.0f;
};
