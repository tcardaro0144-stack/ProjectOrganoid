// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidAdminSectorController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidAdminInitialized, AProjectOrganoidAdminSectorController*, Controller);

/**
 *  Admin local brain (Section 14). Queries existing security/power subsystems.
 *  Does not implement a second lockdown or power grid.
 */
UCLASS(Blueprintable)
class AProjectOrganoidAdminSectorController : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidAdminSectorController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|State")
	FName FacilityState = TEXT("Normal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|State")
	FName CurrentRoom = TEXT("None");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|State")
	EProjectOrganoidSecurityTier AdminAccessLevel = EProjectOrganoidSecurityTier::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Admin|State")
	bool bAdminInitialized = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Flags")
	bool bSecurityScanned = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Flags")
	bool bDirectorOfficeVisited = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Flags")
	bool bRecordsAccessed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Flags")
	bool bOperationsActivated = false;

	UPROPERTY(BlueprintAssignable, Category = "Admin|State")
	FOnProjectOrganoidAdminInitialized OnAdminInitialized;

	UFUNCTION(BlueprintCallable, Category = "Admin|State")
	void InitializeAdmin();

	UFUNCTION(BlueprintCallable, Category = "Admin|State")
	void SetCurrentRoom(FName NewRoom);

protected:

	UFUNCTION()
	void HandleFacilityLockdownChanged(bool bIsLockdownActive, FName LockdownId);

	void ApplyFacilityStateFromSubsystems();
	void BindSecuritySubsystem();
	void UnbindSecuritySubsystem();
};
