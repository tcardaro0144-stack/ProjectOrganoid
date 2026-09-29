// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSaveCleanup.h"

#include "ProjectOrganoidSaveSubsystem.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidSaveCleanup::AProjectOrganoidSaveCleanup()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

const TArray<FString>& AProjectOrganoidSaveCleanup::GetCleanupSlotNames()
{
	static const TArray<FString> Slots = {
		TEXT("OrganoidAutosave"),
		TEXT("OrganoidOpeningFoundationTest"),
		TEXT("OrganoidOpeningInvestigationTest"),
	};
	return Slots;
}

bool AProjectOrganoidSaveCleanup::ApplyCleanup(EProjectOrganoidNodeZeroFate Fate)
{
	if (Fate != EProjectOrganoidNodeZeroFate::Destroy && Fate != EProjectOrganoidNodeZeroFate::Extract)
	{
		return false;
	}
	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UProjectOrganoidSaveSubsystem* Saves = GameInstance ? GameInstance->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
	if (!Saves)
	{
		return false;
	}

	DeletedSlotCount = 0;
	for (const FString& Slot : GetCleanupSlotNames())
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
		{
			UGameplayStatics::DeleteGameInSlot(Slot, 0);
		}
		// Count as cleaned whether or not the slot existed — contract is wipe-attempted.
		++DeletedSlotCount;
		if (Saves->DoesSaveExist(Slot))
		{
			Saves->DeleteSave(Slot);
		}
	}

	LastCleanupFate = Fate;
	if (Fate == EProjectOrganoidNodeZeroFate::Extract)
	{
		Saves->GrantNewGamePlus();
		bKeptNewGamePlus = Saves->HasNewGamePlus();
	}
	else
	{
		Saves->ClearNewGamePlus();
		bKeptNewGamePlus = false;
	}
	return DeletedSlotCount == GetCleanupSlotNames().Num();
}
