// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidInteractionComponent.generated.h"

class AProjectOrganoidCharacter;
class AProjectOrganoidInteractable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidFocusChanged, AProjectOrganoidInteractable*, NewFocus);

/**
 *  Scans for nearby interactables and routes Avery's Interact input.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidInteractionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Trace / scan radius around Avery */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "50.0"))
	float ScanRadius = 250.0f;

	/** Prefer aim-facing interactables within this cone (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float AimConeDegrees = 55.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AProjectOrganoidInteractable> FocusedInteractable;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnProjectOrganoidFocusChanged OnFocusChanged;

	/** Attempt interaction with the current focus */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AProjectOrganoidInteractable* GetFocusedInteractable() const { return FocusedInteractable; }

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetFocusedPrompt() const;

protected:

	UPROPERTY()
	TObjectPtr<AProjectOrganoidCharacter> OwnerCharacter;

	void RefreshFocus();
	void SetFocusedInteractable(AProjectOrganoidInteractable* NewFocus);
	AProjectOrganoidInteractable* FindBestInteractable() const;
};
