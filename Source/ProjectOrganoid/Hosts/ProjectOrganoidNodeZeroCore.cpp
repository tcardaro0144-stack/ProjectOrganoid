// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidNodeZeroCore.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidSaveSubsystem.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"

AProjectOrganoidNodeZeroCore::AProjectOrganoidNodeZeroCore()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionPrompt = FText::FromString(TEXT("Approach Node Zero"));
	if (InteractionSphere)
	{
		InteractionSphere->SetCanEverAffectNavigation(false);
	}
}

bool AProjectOrganoidNodeZeroCore::HasBothSyringeAdaptations(const AProjectOrganoidCharacter* Interactor) const
{
	const UProjectOrganoidBiologicalAdaptationComponent* Adaptations = Interactor ? Interactor->GetBiologicalAdaptationComponent() : nullptr;
	if (!Adaptations)
	{
		return false;
	}
	const UProjectOrganoidBiologicalAdaptationData* Locomotor = UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve();
	const UProjectOrganoidBiologicalAdaptationData* Optical = UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve();
	return Adaptations->IsAdaptationUnlocked(Locomotor) && Adaptations->IsAdaptationUnlocked(Optical);
}

bool AProjectOrganoidNodeZeroCore::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract(Interactor) || ChosenFate != EProjectOrganoidNodeZeroFate::None)
	{
		return false;
	}
	if (ShutdownStage >= ShutdownStageCount)
	{
		return false;
	}
	++ShutdownStage;
	if (SterlingPresentationCount == 0)
	{
		ShowSterlingLine();
	}
	return true;
}

bool AProjectOrganoidNodeZeroCore::CraftVaccine(AProjectOrganoidCharacter* Interactor)
{
	if (bVaccineCrafted || ChosenFate != EProjectOrganoidNodeZeroFate::None || !HasBothSyringeAdaptations(Interactor))
	{
		return bVaccineCrafted && HasBothSyringeAdaptations(Interactor);
	}
	bVaccineCrafted = true;
	return true;
}

bool AProjectOrganoidNodeZeroCore::ChooseFate(AProjectOrganoidCharacter* Interactor, EProjectOrganoidNodeZeroFate Fate)
{
	if (!Interactor || ChosenFate != EProjectOrganoidNodeZeroFate::None || !bVaccineCrafted || ShutdownStage < ShutdownStageCount)
	{
		return false;
	}
	if (Fate != EProjectOrganoidNodeZeroFate::Destroy && Fate != EProjectOrganoidNodeZeroFate::Extract)
	{
		return false;
	}
	ChosenFate = Fate;
	bIsInteractable = false;
	CompleteObjectives();
	return bObjectivesCompleted && bGrantedNewGamePlus;
}

void AProjectOrganoidNodeZeroCore::ShowSterlingLine()
{
	++SterlingPresentationCount;
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD)
	{
		HUD->ShowTransientNotification(
			FText::FromString(TEXT("Sterling")),
			FText::FromString(TEXT("I built it to heal. It learned to keep.")),
			LineSeconds);
	}
}

void AProjectOrganoidNodeZeroCore::CompleteObjectives()
{
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UProjectOrganoidSaveSubsystem* Saves = GameInstance ? GameInstance->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
	UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance ? GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
	if (Saves)
	{
		Saves->GrantNewGamePlus();
		bGrantedNewGamePlus = Saves->HasNewGamePlus();
	}
	if (Objectives)
	{
		Objectives->TriggerEvent(TEXT("Event_NodeZeroReached"));
		Objectives->TriggerEvent(TEXT("Event_FateChosen"));
		bObjectivesCompleted = true;
	}
}
