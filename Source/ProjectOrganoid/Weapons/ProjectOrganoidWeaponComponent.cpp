// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidDefaultWeapon.h"
#include "ProjectOrganoidWeaponData.h"
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
		EquippedWeapon->CancelReload();
		StoreEquippedMagazineState();
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
	RestoreMagazineStateFor(EquippedWeapon);

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

bool UProjectOrganoidWeaponComponent::UnlockWeaponRoster(FName WeaponId)
{
	UProjectOrganoidWeaponData* Data = UProjectOrganoidWeaponData::ResolveById(WeaponId);
	if (!Data)
	{
		return false;
	}
	const FSoftObjectPath Path(Data);
	if (!UnlockedWeaponRoster.Contains(Path))
	{
		UnlockedWeaponRoster.Add(Path);
	}
	return true;
}

bool UProjectOrganoidWeaponComponent::IsWeaponRosterUnlocked(FName WeaponId) const
{
	UProjectOrganoidWeaponData* Data = UProjectOrganoidWeaponData::ResolveById(WeaponId);
	return Data && UnlockedWeaponRoster.Contains(FSoftObjectPath(Data));
}

bool UProjectOrganoidWeaponComponent::EquipWeaponRoster(FName WeaponId)
{
	if (!IsWeaponRosterUnlocked(WeaponId))
	{
		return false;
	}
	UProjectOrganoidWeaponData* Data = UProjectOrganoidWeaponData::ResolveById(WeaponId);
	AProjectOrganoidWeapon* Weapon = EquipWeaponClass(AProjectOrganoidDefaultWeapon::StaticClass());
	if (!Weapon || !Data)
	{
		return false;
	}
	Weapon->ApplyRosterData(Data);
	EquippedWeaponRoster = FSoftObjectPath(Data);
	return true;
}

void UProjectOrganoidWeaponComponent::ApplySavedWeaponRoster(const TArray<FSoftObjectPath>& Unlocked, const FSoftObjectPath& Equipped)
{
	UnlockedWeaponRoster = Unlocked;
	EquippedWeaponRoster = Equipped;
	if (Equipped.IsNull())
	{
		return;
	}
	if (UProjectOrganoidWeaponData* Data = Cast<UProjectOrganoidWeaponData>(Equipped.TryLoad()))
	{
		if (AProjectOrganoidWeapon* Weapon = EquipWeaponClass(AProjectOrganoidDefaultWeapon::StaticClass()))
		{
			Weapon->ApplyRosterData(Data);
		}
	}
}

bool UProjectOrganoidWeaponComponent::FireEquippedWeapon()
{
	return EquippedWeapon ? EquippedWeapon->Fire() : false;
}

bool UProjectOrganoidWeaponComponent::ReloadEquippedWeapon()
{
	return EquippedWeapon ? EquippedWeapon->RequestReload() : false;
}

void UProjectOrganoidWeaponComponent::CancelReload()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->CancelReload();
	}
}

void UProjectOrganoidWeaponComponent::StoreEquippedMagazineState()
{
	if (EquippedWeapon)
	{
		UpsertMagazineState(EquippedWeapon->CaptureMagazineState());
	}
}

void UProjectOrganoidWeaponComponent::RestoreMagazineStateFor(AProjectOrganoidWeapon* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	const FSoftClassPath ClassPath(Weapon->GetClass());
	for (const FProjectOrganoidWeaponMagazineState& State : HolsteredMagazineStates)
	{
		if (State.WeaponClass == ClassPath)
		{
			Weapon->ApplyMagazineState(State);
			return;
		}
	}
}

void UProjectOrganoidWeaponComponent::UpsertMagazineState(const FProjectOrganoidWeaponMagazineState& State)
{
	if (!State.IsValid())
	{
		return;
	}

	for (FProjectOrganoidWeaponMagazineState& Existing : HolsteredMagazineStates)
	{
		if (Existing.WeaponClass == State.WeaponClass)
		{
			Existing = State;
			return;
		}
	}

	HolsteredMagazineStates.Add(State);
}

TArray<FProjectOrganoidWeaponMagazineState> UProjectOrganoidWeaponComponent::CaptureMagazineStates() const
{
	TArray<FProjectOrganoidWeaponMagazineState> States = HolsteredMagazineStates;
	if (EquippedWeapon)
	{
		const FProjectOrganoidWeaponMagazineState Live = EquippedWeapon->CaptureMagazineState();
		bool bReplaced = false;
		for (FProjectOrganoidWeaponMagazineState& Existing : States)
		{
			if (Existing.WeaponClass == Live.WeaponClass)
			{
				Existing = Live;
				bReplaced = true;
				break;
			}
		}
		if (!bReplaced)
		{
			States.Add(Live);
		}
	}
	return States;
}

void UProjectOrganoidWeaponComponent::ApplyMagazineStates(const TArray<FProjectOrganoidWeaponMagazineState>& States)
{
	HolsteredMagazineStates.Reset();
	for (const FProjectOrganoidWeaponMagazineState& State : States)
	{
		UpsertMagazineState(State);
	}

	if (EquippedWeapon)
	{
		RestoreMagazineStateFor(EquippedWeapon);
	}
}

int32 UProjectOrganoidWeaponComponent::GetHolsteredMagazineCount(TSubclassOf<AProjectOrganoidWeapon> WeaponClass) const
{
	if (!WeaponClass)
	{
		return INDEX_NONE;
	}

	const FSoftClassPath ClassPath(WeaponClass.Get());
	for (const FProjectOrganoidWeaponMagazineState& State : HolsteredMagazineStates)
	{
		if (State.WeaponClass == ClassPath)
		{
			return State.LoadedMagazineCount;
		}
	}
	return INDEX_NONE;
}

bool UProjectOrganoidWeaponComponent::FireOverchargedPulse()
{
	return EquippedWeapon ? EquippedWeapon->FireOverchargedPulse() : false;
}
