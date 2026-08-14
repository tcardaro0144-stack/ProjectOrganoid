// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Scene.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPhotoScanComponent.generated.h"

class AActor;
class UCameraComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidPhotoModeChanged, bool, bActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidScanCompleted, AActor*, ScannedActor, const FProjectOrganoidLogEntry&, LoreEntry, bool, bNewLore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidScanFocusChanged, AActor*, FocusedActor, FText, DisplayName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidPhotoCaptured, const FString&, ScreenshotPath);

/**
 *  Photography / scanning mode — DoF framing, lore extraction from IProjectOrganoidScannable,
 *  and high-resolution screenshot capture.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidPhotoScanComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidPhotoScanComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category = "Photo|Events")
	FOnProjectOrganoidPhotoModeChanged OnPhotoModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Photo|Events")
	FOnProjectOrganoidScanCompleted OnScanCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Photo|Events")
	FOnProjectOrganoidScanFocusChanged OnScanFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Photo|Events")
	FOnProjectOrganoidPhotoCaptured OnPhotoCaptured;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|Scan", meta = (ClampMin = "100.0"))
	float ScanTraceDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|Scan", meta = (ClampMin = "0.0"))
	float ScanSphereRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|DoF", meta = (ClampMin = "0.5", ClampMax = "32.0"))
	float PhotoFstop = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|DoF", meta = (ClampMin = "0.0"))
	float PhotoFocalDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|DoF", meta = (ClampMin = "0.0"))
	float PhotoDepthBlurAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|DoF", meta = (ClampMin = "0.0"))
	float AutoFocusInterpSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|Capture")
	FString ScreenshotSubFolder = TEXT("ProjectOrganoid");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|Capture", meta = (ClampMin = "1"))
	int32 HighResShotMultiplier = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo|Capture")
	bool bUseHighResShot = true;

	UFUNCTION(BlueprintCallable, Category = "Photo")
	void SetPhotoModeActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Photo")
	void TogglePhotoMode();

	UFUNCTION(BlueprintPure, Category = "Photo")
	bool IsPhotoModeActive() const { return bPhotoModeActive; }

	UFUNCTION(BlueprintCallable, Category = "Photo|Scan")
	bool TryScanFocusedTarget();

	UFUNCTION(BlueprintCallable, Category = "Photo|Capture")
	bool CaptureHighResScreenshot();

	UFUNCTION(BlueprintPure, Category = "Photo|Scan")
	AActor* GetFocusedScanTarget() const { return FocusedScanTarget.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Photo|DoF")
	void ApplyDepthOfFieldSettings(float FocalDistance, float Fstop);

	UFUNCTION(BlueprintCallable, Category = "Photo|DoF")
	void ClearDepthOfFieldOverrides();

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Photo")
	bool bPhotoModeActive = false;

	UPROPERTY()
	TWeakObjectPtr<AActor> FocusedScanTarget;

	UPROPERTY()
	TWeakObjectPtr<UCameraComponent> CachedCamera;

	bool bCachedPostProcess = false;
	FPostProcessSettings CachedPostProcessSettings;
	float CachedPostProcessBlendWeight = 1.0f;

	AProjectOrganoidCharacter* GetOwnerCharacter() const;
	UCameraComponent* ResolveCamera();
	void UpdateScanFocus();
	void UpdateAutoFocusDoF(float DeltaTime);
	void CacheAndApplyPhotoPostProcess();
	void RestoreCameraPostProcess();
	FString BuildScreenshotFileName() const;
};
