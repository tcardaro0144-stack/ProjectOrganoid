// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidInteractionComponent.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetSystemLibrary.h"

UProjectOrganoidInteractionComponent::UProjectOrganoidInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UProjectOrganoidInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<AProjectOrganoidCharacter>(GetOwner());
}

void UProjectOrganoidInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshFocus();
}

void UProjectOrganoidInteractionComponent::RefreshFocus()
{
	SetFocusedInteractable(FindBestInteractable());
}

void UProjectOrganoidInteractionComponent::SetFocusedInteractable(AProjectOrganoidInteractable* NewFocus)
{
	if (FocusedInteractable == NewFocus)
	{
		return;
	}

	FocusedInteractable = NewFocus;
	OnFocusChanged.Broadcast(FocusedInteractable);
}

AProjectOrganoidInteractable* UProjectOrganoidInteractionComponent::FindBestInteractable() const
{
	if (!OwnerCharacter || !GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> Overlapping;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerCharacter->GetActorLocation(),
		ScanRadius,
		ObjectTypes,
		AProjectOrganoidInteractable::StaticClass(),
		ActorsToIgnore,
		Overlapping);

	FVector ViewLoc = OwnerCharacter->GetActorLocation();
	FVector ViewDir = OwnerCharacter->GetActorForwardVector();
	if (AController* Controller = OwnerCharacter->GetController())
	{
		FRotator ViewRot;
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		ViewDir = ViewRot.Vector();
	}

	AProjectOrganoidInteractable* Best = nullptr;
	float BestScore = -BIG_NUMBER;
	const float ConeCos = FMath::Cos(FMath::DegreesToRadians(AimConeDegrees));

	for (AActor* Actor : Overlapping)
	{
		AProjectOrganoidInteractable* Interactable = Cast<AProjectOrganoidInteractable>(Actor);
		if (!Interactable || !Interactable->CanInteract(OwnerCharacter))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerCharacter->GetActorLocation(), Interactable->GetActorLocation());
		const float Range = Interactable->InteractionRange;
		if (DistSq > FMath::Square(Range))
		{
			continue;
		}

		const FVector ToTarget = (Interactable->GetActorLocation() - ViewLoc).GetSafeNormal();
		const float Dot = FVector::DotProduct(ViewDir, ToTarget);
		if (Dot < ConeCos)
		{
			continue;
		}

		// Prefer closer + more centered in aim
		const float Score = Dot * 2.0f - FMath::Sqrt(DistSq) * 0.001f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Interactable;
		}
	}

	return Best;
}

bool UProjectOrganoidInteractionComponent::TryInteract()
{
	if (!OwnerCharacter || !FocusedInteractable)
	{
		return false;
	}

	return FocusedInteractable->Interact(OwnerCharacter);
}

FText UProjectOrganoidInteractionComponent::GetFocusedPrompt() const
{
	return FocusedInteractable ? FocusedInteractable->GetInteractionPrompt() : FText::GetEmpty();
}
