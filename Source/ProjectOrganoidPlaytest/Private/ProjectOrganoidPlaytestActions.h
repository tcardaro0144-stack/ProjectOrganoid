#pragma once

#include "CoreMinimal.h"

class AActor;
class ACharacter;
class APawn;
class APlayerController;
class UActorComponent;
class UWorld;

struct FOrganoidPlaytestPropValue
{
	bool bFound = false;
	FString Type;
	FString Text;
	bool bBool = false;
	bool bHasBool = false;
	double Number = 0.0;
	bool bHasNumber = false;
	FString EnumInternal;
	FString EnumDisplay;
};

namespace OrganoidPlaytestActions
{
	FString ActorLabel(AActor* Actor);
	FString ActorPackage(AActor* Actor);
	FString NormalizePackage(const FString& PackageName);
	UWorld* GetPieWorld();
	APlayerController* GetPlayerController(UWorld* World);
	APawn* GetPlayerPawn(UWorld* World);
	ACharacter* GetPlayerCharacter(UWorld* World);
	TArray<AActor*> FindActorsByLabel(UWorld* World, const FString& Label);
	AActor* FindUniqueByLabel(UWorld* World, const FString& Label);
	UActorComponent* FindNamedComponent(AActor* Actor, const FString& ComponentName);
	UActorComponent* FindInteractionComponent(APawn* Pawn);
	AActor* GetFocusedInteractable(UActorComponent* InteractionComponent);
	bool TryInteract(UActorComponent* InteractionComponent);
	FOrganoidPlaytestPropValue ReadProperty(UObject* Object, const FString& PropertyName);
	bool TeleportNear(APawn* Pawn, const FVector& Target, float DistanceUu, float CapsuleZ);
	bool TeleportNear(APawn* Pawn, const FVector& Target, float DistanceUu, float CapsuleZ, const FVector& PlanarDirection);
	bool FaceActor(APawn* Pawn, AActor* Target);
	float DistanceTo(AActor* A, AActor* B);
	float AimDotTo(APawn* Pawn, AActor* Target);
	int32 CountVisibleHackingWidgets(UWorld* World);
	AActor* FindAccessDoor(UWorld* World);
	FVector ReadBoxExtent(AActor* Actor, const FString& ComponentName);
}
