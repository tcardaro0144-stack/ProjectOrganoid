// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidNodeZeroCore.generated.h"

class AProjectOrganoidCharacter;

UENUM(BlueprintType)
enum class EProjectOrganoidNodeZeroFate : uint8
{
	None,
	Destroy,
	Extract
};

/**
 * Reactor heart. Three shutdown stages, a vaccine that needs both syringe-kit adaptations,
 * then one Destroy or Extract choice. Sterling speaks once. The choice completes both
 * Node Zero objectives and grants the NG+ flag.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidNodeZeroCore : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:
	AProjectOrganoidNodeZeroCore();

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	bool CraftVaccine(AProjectOrganoidCharacter* Interactor);
	bool ChooseFate(AProjectOrganoidCharacter* Interactor, EProjectOrganoidNodeZeroFate Fate);

	int32 GetShutdownStage() const { return ShutdownStage; }
	int32 GetShutdownStageCount() const { return ShutdownStageCount; }
	bool IsVaccineCrafted() const { return bVaccineCrafted; }
	bool WasSterlingLineShown() const { return SterlingPresentationCount > 0; }
	int32 GetSterlingPresentationCount() const { return SterlingPresentationCount; }
	EProjectOrganoidNodeZeroFate GetChosenFate() const { return ChosenFate; }
	bool HasGrantedNewGamePlus() const { return bGrantedNewGamePlus; }
	bool UsesPursuerAI() const { return false; }

private:
	bool HasBothSyringeAdaptations(const AProjectOrganoidCharacter* Interactor) const;
	void ShowSterlingLine();
	void CompleteObjectives();

	int32 ShutdownStage = 0;
	int32 SterlingPresentationCount = 0;
	EProjectOrganoidNodeZeroFate ChosenFate = EProjectOrganoidNodeZeroFate::None;
	bool bVaccineCrafted = false;
	bool bGrantedNewGamePlus = false;
	bool bObjectivesCompleted = false;

	static constexpr int32 ShutdownStageCount = 3;
	static constexpr float LineSeconds = 7.f;
};
