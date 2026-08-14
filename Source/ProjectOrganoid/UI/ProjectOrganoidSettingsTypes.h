// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidSettingsTypes.generated.h"

/** Overall graphics scalability bucket (maps to UGameUserSettings 0–4). */
UENUM(BlueprintType)
enum class EProjectOrganoidGraphicsQuality : uint8
{
	Low = 0,
	Medium = 1,
	High = 2,
	Epic = 3,
	Cinematic = 4
};

/** Lightweight save-slot summary for main-menu load UI. */
USTRUCT(BlueprintType)
struct FProjectOrganoidSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bExists = false;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Vitals")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Vitals")
	float PEEnergy = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Upgrades")
	int32 SuitHealthUpgradeLevel = 0;
};
