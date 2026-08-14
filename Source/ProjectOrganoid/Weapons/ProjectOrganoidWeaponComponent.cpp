// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidDefaultWeapon.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

UProjectOrganoidWeaponComponent::UProjectOrganoidWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DefaultWeaponClass = AProjectOrganoidDefaultWeapon::StaticClass();
}

void UProjectOrganoidWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	SpawnDefaultWeapon();
}

void UProjectOrganoidWeaponComponent::SpawnDefaultWeapon()
{
	if (!EquippedWeapon && DefaultWeaponClass)
	{
		EquipWeaponClass(DefaultWeaponClass);
	}
}

AProjectOrganoidWeapon* UProjectOrganoidWeaponComponent::EquipWeaponClass(TSubclassOf<AProjectOrganoidWeapon> WeaponClass)
{
	if (!WeaponClass || !GetWorld())
	{
		return nullptr;
	}

	AProjectOrganoidCharacter* CharacterOwner = Cast<AProjectOrganoidCharacter>(GetOwner());
	if (!CharacterOwner)
	{
		CharacterOwner = Cast<AProjectOrganoidCharacter>(GetAttachmentRootActor());
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = CharacterOwner ? static_cast<AActor*>(CharacterOwner) : GetOwner();
	SpawnParams.Instigator = CharacterOwner;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectOrganoidWeapon* NewWeapon = GetWorld()->SpawnActor<AProjectOrganoidWeapon>(
		WeaponClass,
		GetComponentTransform(),
		SpawnParams);

	if (!NewWeapon)
	{
		return nullptr;
	}

	EquippedWeapon = NewWeapon;
	EquippedWeapon->SetWeaponOwnerCharacter(CharacterOwner);

	USkeletalMeshComponent* CharacterMesh = CharacterOwner ? CharacterOwner->GetMesh() : nullptr;
	if (CharacterMesh && CharacterMesh->DoesSocketExist(WeaponAttachSocketName))
	{
		EquippedWeapon->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			WeaponAttachSocketName);
	}
	else
	{
		EquippedWeapon->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	return EquippedWeapon;
}

bool UProjectOrganoidWeaponComponent::FireEquippedWeapon()
{
	return EquippedWeapon ? EquippedWeapon->Fire() : false;
}
