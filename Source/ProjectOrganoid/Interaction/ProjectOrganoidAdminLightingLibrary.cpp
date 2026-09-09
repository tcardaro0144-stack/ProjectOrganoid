// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminLightingLibrary.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/LightComponent.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* AdminPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	const TCHAR* ZoneLightPrefix = TEXT("Admin_ZoneLight_");
	const TCHAR* ZoneLightGroupTag = TEXT("Admin_ZoneLight");

	const TCHAR* ApprovedZones[] = {
		TEXT("Vestibule"),
		TEXT("Reception"),
		TEXT("Hub"),
		TEXT("Security"),
		TEXT("Records"),
		TEXT("Conference"),
		TEXT("DirectorSuite"),
		TEXT("Operations"),
		TEXT("Transit"),
		TEXT("ServiceCorridor"),
	};

	bool ZoneNameIsNone(FName ZoneName)
	{
		return ZoneName.IsNone() || ZoneName.ToString().Equals(TEXT("None"), ESearchCase::IgnoreCase);
	}

	FString ActorDisplayLabel(const AActor* Actor)
	{
		if (!Actor)
		{
			return FString();
		}
#if WITH_EDITOR
		return Actor->GetActorLabel();
#else
		return Actor->GetActorNameOrLabel();
#endif
	}

	FString ActorOwningPackage(const AActor* Actor)
	{
		if (!Actor)
		{
			return FString();
		}
		FString PackageName;
		if (ULevel* Level = Actor->GetLevel())
		{
			if (UPackage* Package = Level->GetOutermost())
			{
				PackageName = Package->GetName();
			}
		}
		if (PackageName.IsEmpty())
		{
			if (UPackage* Package = Actor->GetOutermost())
			{
				PackageName = Package->GetName();
			}
		}
		if (PackageName.IsEmpty())
		{
			return FString();
		}
		// PIE instances are named UEDPIE_<n>_<Asset>. Compare against the
		// canonical Admin package only, after engine PIE-prefix removal.
		return UWorld::RemovePIEPrefix(PackageName);
	}

	bool IsCanonicalAdminPackage(const FString& PackageName)
	{
		return PackageName.Equals(AdminPackage, ESearchCase::IgnoreCase);
	}

	bool IsS20AdminZoneLight(const AActor* Actor)
	{
		const APointLight* PointLight = Cast<APointLight>(Actor);
		if (!PointLight || !IsValid(PointLight))
		{
			return false;
		}
		if (!IsCanonicalAdminPackage(ActorOwningPackage(PointLight)))
		{
			return false;
		}
		if (!PointLight->ActorHasTag(FName(ZoneLightGroupTag)))
		{
			return false;
		}
		const FString Label = ActorDisplayLabel(PointLight);
		return Label.StartsWith(ZoneLightPrefix, ESearchCase::CaseSensitive);
	}
}

bool UProjectOrganoidAdminLightingLibrary::IsApprovedAdminLightingZone(FName ZoneName)
{
	if (ZoneNameIsNone(ZoneName))
	{
		return false;
	}
	const FString Token = ZoneName.ToString();
	for (const TCHAR* Zone : ApprovedZones)
	{
		if (Token.Equals(Zone, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}
	return false;
}

FLinearColor UProjectOrganoidAdminLightingLibrary::GetAdminFacilityStateLightColor(EProjectOrganoidAdminFacilityState State)
{
	switch (State)
	{
	case EProjectOrganoidAdminFacilityState::Alert:
		return FLinearColor::FromSRGBColor(FColor(0xFF, 0x9A, 0x3C));
	case EProjectOrganoidAdminFacilityState::Lockdown:
		return FLinearColor::FromSRGBColor(FColor(0xFF, 0x30, 0x28));
	case EProjectOrganoidAdminFacilityState::Normal:
	default:
		return FLinearColor::FromSRGBColor(FColor::White);
	}
}

int32 UProjectOrganoidAdminLightingLibrary::ApplyAdminZoneLighting(
	UObject* WorldContextObject,
	FName CurrentZone,
	float AuthoredIntensity,
	float InactiveMultiplier)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return 0;
	}
	if (AuthoredIntensity < 0.0f)
	{
		return 0;
	}
	const float ClampedMultiplier = FMath::Clamp(InactiveMultiplier, 0.0f, 1.0f);
	const bool bAllFull = ZoneNameIsNone(CurrentZone);
	const bool bValidActive = !bAllFull && IsApprovedAdminLightingZone(CurrentZone);
	if (!bAllFull && !bValidActive)
	{
		return 0;
	}

	const FString ActiveLabel = bValidActive
		? FString(ZoneLightPrefix) + CurrentZone.ToString()
		: FString();

	EProjectOrganoidAdminFacilityState Presentation = EProjectOrganoidAdminFacilityState::Normal;
	if (UProjectOrganoidAdminFacilityStateSubsystem* FacilityState = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
	{
		Presentation = FacilityState->GetAdminFacilityState();
	}
	const FLinearColor PresentationColor = GetAdminFacilityStateLightColor(Presentation);

	int32 Affected = 0;
	for (TActorIterator<APointLight> It(World); It; ++It)
	{
		APointLight* LightActor = *It;
		if (!IsS20AdminZoneLight(LightActor))
		{
			continue;
		}
		ULightComponent* Light = LightActor->GetLightComponent();
		if (!Light)
		{
			continue;
		}
		float Intensity = AuthoredIntensity;
		if (bValidActive)
		{
			const bool bActive = ActorDisplayLabel(LightActor).Equals(ActiveLabel, ESearchCase::CaseSensitive)
				|| LightActor->ActorHasTag(FName(*ActiveLabel));
			Intensity = bActive ? AuthoredIntensity : AuthoredIntensity * ClampedMultiplier;
		}
		Light->SetIntensity(Intensity);
		Light->SetLightColor(PresentationColor, true);
		++Affected;
	}
	return Affected;
}

void UProjectOrganoidAdminLightingLibrary::RequestAdminLightingZone(
	UObject* WorldContextObject,
	AActor* OtherActor,
	FName ZoneName)
{
	if (!Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		return;
	}
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	AActor* Found = nullptr;
	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		if (!ActorDisplayLabel(Candidate).Equals(TEXT("Admin_LightController"), ESearchCase::CaseSensitive))
		{
			continue;
		}
		if (!IsCanonicalAdminPackage(ActorOwningPackage(Candidate)))
		{
			continue;
		}
		++Count;
		Found = Candidate;
	}
	if (Count != 1 || !Found)
	{
		return;
	}

	UFunction* SetZone = Found->FindFunction(FName(TEXT("SetLightingZone")));
	if (!SetZone)
	{
		return;
	}
	TArray<uint8> Parms;
	Parms.AddZeroed(SetZone->ParmsSize);
	if (FNameProperty* ZoneProp = FindFProperty<FNameProperty>(SetZone, TEXT("ZoneName")))
	{
		ZoneProp->SetPropertyValue(ZoneProp->ContainerPtrToValuePtr<void>(Parms.GetData()), ZoneName);
	}
	else
	{
		return;
	}
	Found->ProcessEvent(SetZone, Parms.GetData());
}
