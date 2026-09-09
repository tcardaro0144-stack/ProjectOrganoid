#include "ProjectOrganoidPlaytestActions.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"

FString OrganoidPlaytestActions::ActorLabel(AActor* Actor)
{
	return Actor ? Actor->GetActorNameOrLabel() : FString();
}

FString OrganoidPlaytestActions::NormalizePackage(const FString& PackageName)
{
	return UWorld::RemovePIEPrefix(PackageName);
}

FString OrganoidPlaytestActions::ActorPackage(AActor* Actor)
{
	if (!Actor)
	{
		return FString();
	}
	if (ULevel* Level = Actor->GetLevel())
	{
		if (UPackage* Package = Level->GetOutermost())
		{
			return NormalizePackage(Package->GetName());
		}
	}
	if (UPackage* Package = Actor->GetOutermost())
	{
		return NormalizePackage(Package->GetName());
	}
	return FString();
}

UWorld* OrganoidPlaytestActions::GetPieWorld()
{
	if (!GEditor)
	{
		return nullptr;
	}
	if (UWorld* PlayWorld = GEditor->PlayWorld)
	{
		return PlayWorld;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			return Context.World();
		}
	}
	return nullptr;
}

APlayerController* OrganoidPlaytestActions::GetPlayerController(UWorld* World)
{
	return World ? World->GetFirstPlayerController() : nullptr;
}

APawn* OrganoidPlaytestActions::GetPlayerPawn(UWorld* World)
{
	APlayerController* PC = GetPlayerController(World);
	return PC ? PC->GetPawn() : nullptr;
}

ACharacter* OrganoidPlaytestActions::GetPlayerCharacter(UWorld* World)
{
	return Cast<ACharacter>(GetPlayerPawn(World));
}

TArray<AActor*> OrganoidPlaytestActions::FindActorsByLabel(UWorld* World, const FString& Label)
{
	TArray<AActor*> Matches;
	if (!World || Label.IsEmpty())
	{
		return Matches;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && ActorLabel(Actor).Equals(Label, ESearchCase::IgnoreCase))
		{
			Matches.Add(Actor);
		}
	}
	return Matches;
}

AActor* OrganoidPlaytestActions::FindUniqueByLabel(UWorld* World, const FString& Label)
{
	TArray<AActor*> Matches = FindActorsByLabel(World, Label);
	return Matches.Num() == 1 ? Matches[0] : nullptr;
}

UActorComponent* OrganoidPlaytestActions::FindNamedComponent(AActor* Actor, const FString& ComponentName)
{
	if (!Actor)
	{
		return nullptr;
	}
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetName().Equals(ComponentName, ESearchCase::IgnoreCase))
		{
			return Component;
		}
	}
	return nullptr;
}

UActorComponent* OrganoidPlaytestActions::FindInteractionComponent(APawn* Pawn)
{
	if (!Pawn)
	{
		return nullptr;
	}
	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetClass() && Component->GetClass()->GetName().Contains(TEXT("ProjectOrganoidInteractionComponent")))
		{
			return Component;
		}
	}
	return nullptr;
}

AActor* OrganoidPlaytestActions::GetFocusedInteractable(UActorComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return nullptr;
	}
	FObjectProperty* FocusProp = FindFProperty<FObjectProperty>(InteractionComponent->GetClass(), TEXT("FocusedInteractable"));
	if (!FocusProp)
	{
		return nullptr;
	}
	return Cast<AActor>(FocusProp->GetObjectPropertyValue_InContainer(InteractionComponent));
}

bool OrganoidPlaytestActions::TryInteract(UActorComponent* InteractionComponent)
{
	if (!InteractionComponent)
	{
		return false;
	}
	UFunction* Function = InteractionComponent->FindFunction(TEXT("TryInteract"));
	if (!Function)
	{
		return false;
	}
	struct FTryInteractParams
	{
		bool ReturnValue = false;
	};
	FTryInteractParams Params;
	InteractionComponent->ProcessEvent(Function, &Params);
	return Params.ReturnValue;
}

FOrganoidPlaytestPropValue OrganoidPlaytestActions::ReadProperty(UObject* Object, const FString& PropertyName)
{
	FOrganoidPlaytestPropValue Result;
	if (!Object || PropertyName.IsEmpty() || !Object->GetClass())
	{
		return Result;
	}
	FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), FName(*PropertyName));
	if (!Property)
	{
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			if ((*It) && (*It)->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
			{
				Property = *It;
				break;
			}
		}
	}
	if (!Property)
	{
		return Result;
	}
	Result.bFound = true;
	Result.Type = Property->GetClass()->GetName();

	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		Result.bHasBool = true;
		Result.bBool = BoolProp->GetPropertyValue_InContainer(Object);
		Result.Text = Result.bBool ? TEXT("true") : TEXT("false");
	}
	else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		const void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Object);
		const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
		Result.bHasNumber = true;
		Result.Number = static_cast<double>(Value);
		if (UEnum* Enum = EnumProp->GetEnum())
		{
			Result.EnumInternal = Enum->GetNameStringByValue(Value);
			Result.EnumDisplay = Enum->GetDisplayNameTextByValue(Value).ToString();
		}
		Result.Text = Result.EnumDisplay.IsEmpty() ? Result.EnumInternal : Result.EnumDisplay;
	}
	else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		const uint8 Value = ByteProp->GetPropertyValue_InContainer(Object);
		Result.bHasNumber = true;
		Result.Number = static_cast<double>(Value);
		if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
		{
			Result.EnumInternal = Enum->GetNameStringByValue(Value);
			Result.EnumDisplay = Enum->GetDisplayNameTextByValue(Value).ToString();
			Result.Text = Result.EnumDisplay.IsEmpty() ? Result.EnumInternal : Result.EnumDisplay;
		}
		else
		{
			Result.Text = FString::FromInt(Value);
		}
	}
	else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		Result.Text = NameProp->GetPropertyValue_InContainer(Object).ToString();
	}
	else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		Result.Text = StrProp->GetPropertyValue_InContainer(Object);
	}
	else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		Result.Text = TextProp->GetPropertyValue_InContainer(Object).ToString();
	}
	else if (const FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
	{
		Result.bHasNumber = true;
		if (NumProp->IsFloatingPoint())
		{
			Result.Number = NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object));
			Result.Text = FString::SanitizeFloat(Result.Number);
		}
		else
		{
			const int64 Value = NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object));
			Result.Number = static_cast<double>(Value);
			Result.Text = FString::Printf(TEXT("%lld"), Value);
		}
	}
	return Result;
}

bool OrganoidPlaytestActions::TeleportNear(APawn* Pawn, const FVector& Target, float DistanceUu, float CapsuleZ)
{
	return TeleportNear(Pawn, Target, DistanceUu, CapsuleZ, FVector(-1.0f, 0.0f, 0.0f));
}

bool OrganoidPlaytestActions::TeleportNear(APawn* Pawn, const FVector& Target, float DistanceUu, float CapsuleZ, const FVector& PlanarDirection)
{
	if (!Pawn)
	{
		return false;
	}
	FVector Dir = PlanarDirection;
	Dir.Z = 0.0f;
	if (!Dir.Normalize())
	{
		Dir = FVector(-1.0f, 0.0f, 0.0f);
	}
	FVector Dest = Target + Dir * DistanceUu;
	Dest.Z = CapsuleZ;
	const bool bMoved = Pawn->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
			if (Move->MovementMode == MOVE_None)
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
	}
	return bMoved || FVector::Dist(Pawn->GetActorLocation(), Dest) < 5.0f;
}

bool OrganoidPlaytestActions::FaceActor(APawn* Pawn, AActor* Target)
{
	if (!Pawn || !Target)
	{
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	FVector ViewLoc = Pawn->GetActorLocation();
	FRotator ViewRot = Pawn->GetActorRotation();
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLoc, ViewRot);
	}
	const FRotator LookAt = (Target->GetActorLocation() - ViewLoc).Rotation();
	Pawn->SetActorRotation(FRotator(0.0f, LookAt.Yaw, 0.0f));
	if (PC)
	{
		PC->SetControlRotation(LookAt);
	}
	return true;
}

float OrganoidPlaytestActions::DistanceTo(AActor* A, AActor* B)
{
	if (!A || !B)
	{
		return TNumericLimits<float>::Max();
	}
	return FVector::Dist(A->GetActorLocation(), B->GetActorLocation());
}

float OrganoidPlaytestActions::AimDotTo(APawn* Pawn, AActor* Target)
{
	if (!Pawn || !Target)
	{
		return -1.0f;
	}
	FVector ViewLoc = Pawn->GetActorLocation();
	FRotator ViewRot = Pawn->GetActorRotation();
	if (AController* Controller = Pawn->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
	}
	const FVector ToTarget = (Target->GetActorLocation() - ViewLoc).GetSafeNormal();
	return FVector::DotProduct(ViewRot.Vector(), ToTarget);
}

int32 OrganoidPlaytestActions::CountVisibleHackingWidgets(UWorld* World)
{
	int32 Count = 0;
	if (!World)
	{
		return Count;
	}
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!Widget || Widget->GetWorld() != World)
		{
			continue;
		}
		const FString ClassName = Widget->GetClass() ? Widget->GetClass()->GetName() : FString();
		if (!ClassName.Contains(TEXT("Hacking")))
		{
			continue;
		}
		if (Widget->IsInViewport() || Widget->IsVisible())
		{
			++Count;
		}
	}
	return Count;
}

AActor* OrganoidPlaytestActions::FindAccessDoor(UWorld* World)
{
	TArray<AActor*> Exact = FindActorsByLabel(World, TEXT("BP_AdminAccessDoor"));
	if (Exact.Num() == 1)
	{
		return Exact[0];
	}
	if (!World)
	{
		return nullptr;
	}
	AActor* Fallback = nullptr;
	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}
		const FString Label = ActorLabel(Actor);
		const FString ClassName = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();
		if (Label.Contains(TEXT("AdminAccessDoor")) || ClassName.Contains(TEXT("BP_AdminAccessDoor")))
		{
			Fallback = Actor;
			++Count;
		}
	}
	return Count == 1 ? Fallback : nullptr;
}

FVector OrganoidPlaytestActions::ReadBoxExtent(AActor* Actor, const FString& ComponentName)
{
	UActorComponent* Component = FindNamedComponent(Actor, ComponentName);
	if (UBoxComponent* Box = Cast<UBoxComponent>(Component))
	{
		return Box->GetUnscaledBoxExtent();
	}
	return FVector::ZeroVector;
}
