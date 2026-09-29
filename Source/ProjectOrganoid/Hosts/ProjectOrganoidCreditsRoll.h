// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidCreditsRoll.generated.h"

/**
 * End-of-run credits. Destroy and Extract both roll the core slate;
 * Extract adds Sterling's carrier line once more.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidCreditsRoll : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidCreditsRoll();

	UFUNCTION(BlueprintCallable, Category = "Conclusion|Credits")
	bool PlayCredits(EProjectOrganoidNodeZeroFate Fate);

	UFUNCTION(BlueprintPure, Category = "Conclusion|Credits")
	int32 GetPlayCount() const { return PlayCount; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Credits")
	EProjectOrganoidNodeZeroFate GetLastPlayedFate() const { return LastPlayedFate; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Credits")
	FString GetLastCreditsText() const { return LastCreditsText; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Credits")
	bool DidShowCarrierLine() const { return bShowedCarrierLine; }

	bool UsesPursuerAI() const { return false; }

private:
	void PresentLine(const FString& Speaker, const FString& Line) const;
	FString BuildCreditsBody() const;

	EProjectOrganoidNodeZeroFate LastPlayedFate = EProjectOrganoidNodeZeroFate::None;
	int32 PlayCount = 0;
	FString LastCreditsText;
	bool bShowedCarrierLine = false;

	static constexpr float LineSeconds = 7.f;
};
