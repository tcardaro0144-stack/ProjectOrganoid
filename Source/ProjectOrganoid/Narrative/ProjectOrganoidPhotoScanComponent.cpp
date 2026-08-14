// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPhotoScanComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidScannable.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"

UProjectOrganoidPhotoScanComponent::UProjectOrganoidPhotoScanComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UProjectOrganoidPhotoScanComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveCamera();
}

void UProjectOrganoidPhotoScanComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bPhotoModeActive)
	{
		SetPhotoModeActive(false);
	}
	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidPhotoScanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPhotoModeActive)
	{
		return;
	}

	UpdateScanFocus();
	UpdateAutoFocusDoF(DeltaTime);
}

AProjectOrganoidCharacter* UProjectOrganoidPhotoScanComponent::GetOwnerCharacter() const
{
	return Cast<AProjectOrganoidCharacter>(GetOwner());
}

UCameraComponent* UProjectOrganoidPhotoScanComponent::ResolveCamera()
{
	if (CachedCamera.IsValid())
	{
		return CachedCamera.Get();
	}

	if (AProjectOrganoidCharacter* Character = GetOwnerCharacter())
	{
		CachedCamera = Character->GetFollowCamera();
	}

	return CachedCamera.Get();
}

void UProjectOrganoidPhotoScanComponent::SetPhotoModeActive(bool bActive)
{
	if (bPhotoModeActive == bActive)
	{
		return;
	}

	bPhotoModeActive = bActive;
	SetComponentTickEnabled(bActive);

	if (bActive)
	{
		CacheAndApplyPhotoPostProcess();
		UpdateScanFocus();
	}
	else
	{
		FocusedScanTarget.Reset();
		OnScanFocusChanged.Broadcast(nullptr, FText::GetEmpty());
		RestoreCameraPostProcess();
	}

	OnPhotoModeChanged.Broadcast(bPhotoModeActive);
}

void UProjectOrganoidPhotoScanComponent::TogglePhotoMode()
{
	SetPhotoModeActive(!bPhotoModeActive);
}

void UProjectOrganoidPhotoScanComponent::CacheAndApplyPhotoPostProcess()
{
	UCameraComponent* Camera = ResolveCamera();
	if (!Camera)
	{
		return;
	}

	if (!bCachedPostProcess)
	{
		CachedPostProcessSettings = Camera->PostProcessSettings;
		CachedPostProcessBlendWeight = Camera->PostProcessBlendWeight;
		bCachedPostProcess = true;
	}

	ApplyDepthOfFieldSettings(PhotoFocalDistance, PhotoFstop);
}

void UProjectOrganoidPhotoScanComponent::RestoreCameraPostProcess()
{
	UCameraComponent* Camera = ResolveCamera();
	if (!Camera || !bCachedPostProcess)
	{
		return;
	}

	Camera->PostProcessSettings = CachedPostProcessSettings;
	Camera->PostProcessBlendWeight = CachedPostProcessBlendWeight;
	bCachedPostProcess = false;
}

void UProjectOrganoidPhotoScanComponent::ApplyDepthOfFieldSettings(float FocalDistance, float Fstop)
{
	UCameraComponent* Camera = ResolveCamera();
	if (!Camera)
	{
		return;
	}

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	Camera->PostProcessBlendWeight = 1.0f;

	PP.bOverride_DepthOfFieldFstop = true;
	PP.DepthOfFieldFstop = FMath::Max(0.5f, Fstop);

	PP.bOverride_DepthOfFieldFocalDistance = true;
	PP.DepthOfFieldFocalDistance = FMath::Max(0.0f, FocalDistance);

	PP.bOverride_DepthOfFieldDepthBlurAmount = true;
	PP.DepthOfFieldDepthBlurAmount = PhotoDepthBlurAmount;

	PP.bOverride_DepthOfFieldMinFstop = true;
	PP.DepthOfFieldMinFstop = 1.0f;
}

void UProjectOrganoidPhotoScanComponent::ClearDepthOfFieldOverrides()
{
	RestoreCameraPostProcess();
}

void UProjectOrganoidPhotoScanComponent::UpdateScanFocus()
{
	AProjectOrganoidCharacter* Character = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	FVector TraceStart;
	FVector TraceDir;

	if (PC)
	{
		FRotator ViewRot;
		PC->GetPlayerViewPoint(TraceStart, ViewRot);
		TraceDir = ViewRot.Vector();
	}
	else if (UCameraComponent* Camera = ResolveCamera())
	{
		TraceStart = Camera->GetComponentLocation();
		TraceDir = Camera->GetForwardVector();
	}
	else
	{
		return;
	}

	const FVector TraceEnd = TraceStart + TraceDir * ScanTraceDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PhotoScanFocus), false, Character);
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	bool bHit = false;
	if (ScanSphereRadius > KINDA_SMALL_NUMBER)
	{
		bHit = World->SweepSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(ScanSphereRadius),
			Params);
	}
	else
	{
		bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	}

	AActor* NewFocus = nullptr;
	FText DisplayName = FText::GetEmpty();

	if (bHit && Hit.GetActor()
		&& Hit.GetActor()->GetClass()->ImplementsInterface(UProjectOrganoidScannable::StaticClass()))
	{
		if (IProjectOrganoidScannable::Execute_CanBeScanned(Hit.GetActor(), Character))
		{
			NewFocus = Hit.GetActor();
			DisplayName = IProjectOrganoidScannable::Execute_GetScanDisplayName(NewFocus);
			PhotoFocalDistance = FMath::Max(50.0f, Hit.Distance);
		}
	}

	if (NewFocus != FocusedScanTarget.Get())
	{
		FocusedScanTarget = NewFocus;
		OnScanFocusChanged.Broadcast(NewFocus, DisplayName);
	}
}

void UProjectOrganoidPhotoScanComponent::UpdateAutoFocusDoF(float DeltaTime)
{
	if (!bPhotoModeActive)
	{
		return;
	}

	UCameraComponent* Camera = ResolveCamera();
	if (!Camera)
	{
		return;
	}

	const float Current = Camera->PostProcessSettings.DepthOfFieldFocalDistance;
	const float Target = PhotoFocalDistance;
	const float NewDistance = FMath::FInterpTo(Current, Target, DeltaTime, AutoFocusInterpSpeed);
	ApplyDepthOfFieldSettings(NewDistance, PhotoFstop);
}

bool UProjectOrganoidPhotoScanComponent::TryScanFocusedTarget()
{
	AProjectOrganoidCharacter* Character = GetOwnerCharacter();
	AActor* Target = FocusedScanTarget.Get();
	if (!Character || !Target
		|| !Target->GetClass()->ImplementsInterface(UProjectOrganoidScannable::StaticClass()))
	{
		return false;
	}

	if (!IProjectOrganoidScannable::Execute_CanBeScanned(Target, Character))
	{
		return false;
	}

	const FProjectOrganoidLogEntry Lore = IProjectOrganoidScannable::Execute_GetScanLoreEntry(Target);
	bool bNewLore = false;

	if (!Lore.EntryId.IsNone())
	{
		if (UProjectOrganoidLogComponent* Log = Character->GetLogComponent())
		{
			bNewLore = !Log->HasEntry(Lore.EntryId);
			Log->CollectLogEntry(Lore);
		}
	}

	IProjectOrganoidScannable::Execute_NotifyScanned(Target, Character);

	const FName EventId = IProjectOrganoidScannable::Execute_GetScanObjectiveEventId(Target);
	if (!EventId.IsNone())
	{
		if (UGameInstance* GI = Character->GetGameInstance())
		{
			if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
			{
				Objectives->TriggerEvent(EventId);
			}
		}
	}

	OnScanCompleted.Broadcast(Target, Lore, bNewLore);
	return true;
}

FString UProjectOrganoidPhotoScanComponent::BuildScreenshotFileName() const
{
	const FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	return FPaths::Combine(ScreenshotSubFolder, FString::Printf(TEXT("Scan_%s"), *TimeStamp));
}

bool UProjectOrganoidPhotoScanComponent::CaptureHighResScreenshot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FString RelativePath = BuildScreenshotFileName();
	const FString AbsolutePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / RelativePath + TEXT(".png");

	if (bUseHighResShot && GEngine)
	{
		const FString Cmd = FString::Printf(TEXT("HighResShot %d"), FMath::Max(1, HighResShotMultiplier));
		GEngine->Exec(World, *Cmd);
	}
	else
	{
		FScreenshotRequest::RequestScreenshot(AbsolutePath, false, false);
	}

	OnPhotoCaptured.Broadcast(AbsolutePath);
	return true;
}
