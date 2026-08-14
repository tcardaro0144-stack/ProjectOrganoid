// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidAmbienceZone.generated.h"

class UBoxComponent;
class UReverbEffect;
class USoundBase;
class AProjectOrganoidCharacter;

/**
 *  Facility room / corridor volume that pushes environment reverb + optional room bed
 *  into UProjectOrganoidAudioAmbienceSubsystem when Avery enters.
 */
UCLASS(Blueprintable)
class AProjectOrganoidAmbienceZone : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidAmbienceZone();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ZoneVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone")
	FName ZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone")
	FText DisplayName;

	/** Higher priority wins when overlapping multiple zones */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone", meta = (ClampMin = "0"))
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Reverb")
	TSoftObjectPtr<UReverbEffect> ZoneReverb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReverbVolume = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Reverb", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float ReverbFadeTime = 0.6f;

	/** Optional spatial-ish room tone (played non-spatial on the ambience stack) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Ambience")
	TSoftObjectPtr<USoundBase> RoomToneSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Ambience", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoomToneVolume = 0.35f;

	/** Geometry occlusion multiplier applied while Avery is inside (1 = full muffling scale) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Zone|Occlusion", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float OcclusionStrengthBias = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Audio|Zone")
	bool IsLocalPlayerInside() const { return bLocalPlayerInside; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Audio|Zone")
	bool bLocalPlayerInside = false;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void NotifyEnter(AProjectOrganoidCharacter* Character);
	void NotifyExit(AProjectOrganoidCharacter* Character);
};
