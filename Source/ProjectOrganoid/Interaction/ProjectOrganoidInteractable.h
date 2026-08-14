// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidInteractable.generated.h"

class AProjectOrganoidCharacter;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidInteracted, AProjectOrganoidInteractable*, Interactable, AProjectOrganoidCharacter*, Interactor);

/**
 *  Base world interactable for Avery (doors, terminals, pickups, locks).
 */
UCLASS(Abstract, Blueprintable)
class AProjectOrganoidInteractable : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidInteractable();

	/** Interaction focus / overlap radius */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** Prompt shown by UMG when focused */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText InteractionPrompt = FText::FromString(TEXT("Interact"));

	/** Max distance Avery can interact from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "50.0"))
	float InteractionRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsInteractable = true;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnProjectOrganoidInteracted OnInteracted;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AProjectOrganoidCharacter* Interactor) const;
	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool Interact(AProjectOrganoidCharacter* Interactor);
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FText GetInteractionPrompt() const { return InteractionPrompt; }
};
