// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidConclusionChoiceTrigger.generated.h"

class UBoxComponent;

/**
 * Overlap near the escape path. Once a Node Zero fate is chosen, rolls credits
 * and runs save cleanup reflecting Destroy vs Extract.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidConclusionChoiceTrigger : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidConclusionChoiceTrigger();

	void NotifyPlayerOverlap(AActor* OtherActor);

	UFUNCTION(BlueprintCallable, Category = "Conclusion|Choice")
	bool ResolveConclusionChoice(EProjectOrganoidNodeZeroFate Fate);

	UFUNCTION(BlueprintPure, Category = "Conclusion|Choice")
	bool WasConsumed() const { return bConsumed; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Choice")
	EProjectOrganoidNodeZeroFate GetResolvedFate() const { return ResolvedFate; }

	bool UsesPursuerAI() const { return false; }

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Conclusion|Choice")
	TObjectPtr<UBoxComponent> Trigger;

	bool bConsumed = false;
	EProjectOrganoidNodeZeroFate ResolvedFate = EProjectOrganoidNodeZeroFate::None;
};
