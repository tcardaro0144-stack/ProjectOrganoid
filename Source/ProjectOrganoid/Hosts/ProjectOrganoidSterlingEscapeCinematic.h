// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidSterlingEscapeCinematic.generated.h"

class UBoxComponent;

/**
 * Diegetic escape beat after Node Zero. Destroy = core overload / escape pod.
 * Extract = carrier ending + NG+ already granted by the core.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidSterlingEscapeCinematic : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidSterlingEscapeCinematic();

	UFUNCTION(BlueprintCallable, Category = "Sterling|Escape")
	bool PlayEscapeSequence(EProjectOrganoidNodeZeroFate Fate);

	UFUNCTION(BlueprintPure, Category = "Sterling|Escape")
	EProjectOrganoidNodeZeroFate GetLastPlayedFate() const { return LastPlayedFate; }

	UFUNCTION(BlueprintPure, Category = "Sterling|Escape")
	int32 GetPlayCount() const { return PlayCount; }

	bool UsesPursuerAI() const { return false; }

private:
	void PresentLine(const FString& Speaker, const FString& Line) const;

	EProjectOrganoidNodeZeroFate LastPlayedFate = EProjectOrganoidNodeZeroFate::None;
	int32 PlayCount = 0;

	static constexpr float LineSeconds = 7.f;
};

/** Overlap near Terminal_SterlingFinal. Plays the escape cinematic once after a fate is chosen. */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidSterlingEscapeTrigger : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidSterlingEscapeTrigger();

	void NotifyPlayerOverlap(AActor* OtherActor);

	bool WasConsumed() const { return bConsumed; }
	bool UsesPursuerAI() const { return false; }

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Sterling|Escape")
	TObjectPtr<UBoxComponent> Trigger;

	bool bConsumed = false;
};
