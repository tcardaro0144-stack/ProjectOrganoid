#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAdminLightingLibrary.h"
#include "String/BytesToHex.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S20_AdminLighting_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 20 Admin Lighting Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR ReactorPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	constexpr TCHAR SeamBandLabel[] = TEXT("StreamBand_Admin_NeuroGenetics");
	constexpr TCHAR RoomTriggerBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger");
	constexpr TCHAR LightControllerBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController");
	constexpr TCHAR LightControllerLabel[] = TEXT("Admin_LightController");
	constexpr TCHAR SectorControllerLabel[] = TEXT("Admin_SectorController");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr TCHAR ZoneLightPrefix[] = TEXT("Admin_ZoneLight_");
	constexpr TCHAR ZoneLightTag[] = TEXT("Admin_ZoneLight");
	constexpr float ActiveIntensity = 5000.0f;
	constexpr float InactiveIntensity = 1250.0f;
	constexpr float InactiveMultiplier = 0.25f;
	constexpr float IntensityTolerance = 1.0f;
	constexpr float ColorTolerance = 0.02f;
	constexpr float AdminSectorIntensity = 4500.0f;
	constexpr float SectorEmergencyDimScale = 0.22f;
	constexpr float NeuroEmergencySectorIntensity = AdminSectorIntensity * SectorEmergencyDimScale;
	constexpr float NeuroEmergencyFixtureIntensity = 1400.0f;
	constexpr float OverlapWaitSeconds = 0.45f;
	constexpr int32 RoomCount = 10;
	constexpr int32 TransitRoomIndex = 8;

	const TCHAR* TerminalLabels[] = {
		TEXT("Admin_Terminal_Reception"),
		TEXT("Admin_Terminal_Security"),
		TEXT("Admin_Terminal_Records"),
		TEXT("Admin_Terminal_Executive"),
		TEXT("Admin_Terminal_Operations"),
		TEXT("Admin_Terminal_Transit"),
	};

	const TCHAR* StreamPackages[] = {
		TEXT("/Game/Maps/Epitope/SL_Epitope_Admin"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Compute"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor"),
	};

	const TCHAR* DisplayLabels[] = {
		TEXT("Admin_Operations_CentralDisplay"),
		TEXT("Admin_Transit_Display_Admin"),
		TEXT("Admin_Transit_Display_NeuroGenetics"),
		TEXT("Admin_Transit_Display_Cryo"),
		TEXT("Admin_Transit_Display_Compute"),
		TEXT("Admin_Transit_Display_Reactor"),
	};

	struct FRoomSpec
	{
		const TCHAR* RoomId;
		const TCHAR* TriggerLabel;
		const TCHAR* LightLabel;
	};

	const FRoomSpec Rooms[RoomCount] = {
		{TEXT("Vestibule"), TEXT("Admin_RoomTrigger_Vestibule"), TEXT("Admin_ZoneLight_Vestibule")},
		{TEXT("Reception"), TEXT("Admin_RoomTrigger_Reception"), TEXT("Admin_ZoneLight_Reception")},
		{TEXT("Hub"), TEXT("Admin_RoomTrigger_Hub"), TEXT("Admin_ZoneLight_Hub")},
		{TEXT("Security"), TEXT("Admin_RoomTrigger_Security"), TEXT("Admin_ZoneLight_Security")},
		{TEXT("Records"), TEXT("Admin_RoomTrigger_Records"), TEXT("Admin_ZoneLight_Records")},
		{TEXT("Conference"), TEXT("Admin_RoomTrigger_Conference"), TEXT("Admin_ZoneLight_Conference")},
		{TEXT("DirectorSuite"), TEXT("Admin_RoomTrigger_DirectorSuite"), TEXT("Admin_ZoneLight_DirectorSuite")},
		{TEXT("Operations"), TEXT("Admin_RoomTrigger_Operations"), TEXT("Admin_ZoneLight_Operations")},
		{TEXT("Transit"), TEXT("Admin_RoomTrigger_Transit"), TEXT("Admin_ZoneLight_Transit")},
		{TEXT("ServiceCorridor"), TEXT("Admin_RoomTrigger_ServiceCorridor"), TEXT("Admin_ZoneLight_ServiceCorridor")},
	};

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString HashFileSha1(const FString& Path)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path))
		{
			return FString();
		}
		uint8 Digest[FSHA1::DigestSize];
		FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
		TStringBuilder<48> Builder;
		UE::String::BytesToHexLower(MakeArrayView(Digest, FSHA1::DigestSize), Builder);
		return Builder.ToString();
	}

	FString ContentFile(const TCHAR* Relative)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / Relative);
	}

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	UActorComponent* FindComp(AActor* Actor, const TCHAR* Name)
	{
		if (!Actor || !Name)
		{
			return nullptr;
		}
		if (UActorComponent* Direct = OrganoidPlaytestActions::FindNamedComponent(Actor, Name))
		{
			return Direct;
		}
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		const FString Prefix(Name);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetName().StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				return Component;
			}
		}
		return nullptr;
	}

	UBoxComponent* FindTriggerBox(AActor* Trigger)
	{
		if (!Trigger)
		{
			return nullptr;
		}
		if (UBoxComponent* Root = Cast<UBoxComponent>(Trigger->GetRootComponent()))
		{
			return Root;
		}
		return Trigger->FindComponentByClass<UBoxComponent>();
	}

	float LightIntensity(AActor* Actor)
	{
		if (!Actor)
		{
			return -1.0f;
		}
		if (APointLight* Point = Cast<APointLight>(Actor))
		{
			if (ULightComponent* Light = Point->GetLightComponent())
			{
				return Light->Intensity;
			}
		}
		TArray<ULightComponent*> Lights;
		Actor->GetComponents<ULightComponent>(Lights);
		return Lights.Num() > 0 ? Lights[0]->Intensity : -1.0f;
	}

	bool IntensityEquals(float Actual, float Expected)
	{
		return FMath::IsNearlyEqual(Actual, Expected, IntensityTolerance);
	}

	FString IntensityText(float Value)
	{
		return FString::Printf(TEXT("%.1f"), Value);
	}

	FLinearColor ZoneLightColor(AActor* Actor)
	{
		if (APointLight* Point = Cast<APointLight>(Actor))
		{
			if (ULightComponent* Light = Point->GetLightComponent())
			{
				return Light->GetLightColor();
			}
		}
		return FLinearColor::Transparent;
	}

	bool ColorEquals(const FLinearColor& Actual, const FLinearColor& Expected)
	{
		return Actual.Equals(Expected, ColorTolerance);
	}

	FString ColorText(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("R=%.4f G=%.4f B=%.4f"), Color.R, Color.G, Color.B);
	}

	bool WriteNameProperty(UObject* Object, const TCHAR* Name, FName Value)
	{
		if (!Object)
		{
			return false;
		}
		FNameProperty* Property = FindFProperty<FNameProperty>(Object->GetClass(), FName(Name));
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool CallApplyAdminZoneLighting(UObject* WorldContext, FName Zone, float Authored, float Inactive)
	{
		UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminLightingLibrary"));
		if (!Class)
		{
			Class = LoadObject<UClass>(nullptr, TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminLightingLibrary"));
		}
		if (!Class)
		{
			return false;
		}
		UFunction* Function = Class->FindFunctionByName(TEXT("ApplyAdminZoneLighting"));
		UObject* CDO = Class->GetDefaultObject();
		if (!Function || !CDO)
		{
			return false;
		}
		TArray<uint8> Parms;
		Parms.AddZeroed(Function->ParmsSize);
		if (FObjectProperty* Ctx = FindFProperty<FObjectProperty>(Function, TEXT("WorldContextObject")))
		{
			Ctx->SetObjectPropertyValue(Ctx->ContainerPtrToValuePtr<void>(Parms.GetData()), WorldContext);
		}
		if (FNameProperty* ZoneProp = FindFProperty<FNameProperty>(Function, TEXT("CurrentZone")))
		{
			ZoneProp->SetPropertyValue(ZoneProp->ContainerPtrToValuePtr<void>(Parms.GetData()), Zone);
		}
		if (FFloatProperty* Auth = FindFProperty<FFloatProperty>(Function, TEXT("AuthoredIntensity")))
		{
			Auth->SetPropertyValue(Auth->ContainerPtrToValuePtr<void>(Parms.GetData()), Authored);
		}
		if (FFloatProperty* Mult = FindFProperty<FFloatProperty>(Function, TEXT("InactiveMultiplier")))
		{
			Mult->SetPropertyValue(Mult->ContainerPtrToValuePtr<void>(Parms.GetData()), Inactive);
		}
		CDO->ProcessEvent(Function, Parms.GetData());
		return true;
	}

	bool IsS20ZoneLight(AActor* Actor)
	{
		APointLight* Point = Cast<APointLight>(Actor);
		if (!Point || !IsValid(Point))
		{
			return false;
		}
		if (!OrganoidPlaytestActions::ActorPackage(Point).Equals(AdminPackage, ESearchCase::IgnoreCase))
		{
			return false;
		}
		if (!Point->ActorHasTag(FName(ZoneLightTag)))
		{
			return false;
		}
		return OrganoidPlaytestActions::ActorLabel(Point).StartsWith(ZoneLightPrefix, ESearchCase::CaseSensitive);
	}

	bool IsPawnInsideNamedBox(APawn* Pawn, AActor* Actor, const TCHAR* ComponentName)
	{
		if (!Pawn || !Actor || !ComponentName)
		{
			return false;
		}
		UBoxComponent* Box = Cast<UBoxComponent>(OrganoidPlaytestActions::FindNamedComponent(Actor, ComponentName));
		if (!Box)
		{
			return false;
		}
		const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
		const FVector Extent = Box->GetScaledBoxExtent();
		return FMath::Abs(Local.X) <= Extent.X
			&& FMath::Abs(Local.Y) <= Extent.Y
			&& FMath::Abs(Local.Z) <= Extent.Z;
	}

	bool IsPawnInsideTrigger(APawn* Pawn, AActor* Trigger)
	{
		UBoxComponent* Box = FindTriggerBox(Trigger);
		if (!Pawn || !Box)
		{
			return false;
		}
		const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
		const FVector Extent = Box->GetScaledBoxExtent();
		return FMath::Abs(Local.X) <= Extent.X
			&& FMath::Abs(Local.Y) <= Extent.Y
			&& FMath::Abs(Local.Z) <= Extent.Z;
	}

	int32 CountOverlappingRoomTriggers(APawn* Pawn, UWorld* World, AActor*& OutMatch, const TCHAR* ExpectedLabel)
	{
		OutMatch = nullptr;
		int32 Count = 0;
		if (!Pawn || !World)
		{
			return 0;
		}
		TArray<AActor*> Overlapping;
		Pawn->GetOverlappingActors(Overlapping);
		for (AActor* Actor : Overlapping)
		{
			if (!Actor)
			{
				continue;
			}
			const FString Label = OrganoidPlaytestActions::ActorLabel(Actor);
			if (!Label.StartsWith(TEXT("Admin_RoomTrigger_"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			++Count;
			if (Label.Equals(ExpectedLabel, ESearchCase::IgnoreCase))
			{
				OutMatch = Actor;
			}
		}
		if (!OutMatch)
		{
			if (AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, ExpectedLabel))
			{
				if (IsPawnInsideTrigger(Pawn, Trigger))
				{
					OutMatch = Trigger;
				}
			}
		}
		return Count;
	}

	struct FStreamLook
	{
		bool bShouldBeLoaded = false;
		bool bShouldBeVisible = false;
		bool bIsLoaded = false;
		bool bFound = false;
	};

	FString StreamFlagText(const FStreamLook& Look)
	{
		if (!Look.bFound)
		{
			return TEXT("<missing>");
		}
		return FString::Printf(
			TEXT("shouldBeLoaded=%s shouldBeVisible=%s isLoaded=%s"),
			*BoolText(Look.bShouldBeLoaded),
			*BoolText(Look.bShouldBeVisible),
			*BoolText(Look.bIsLoaded));
	}

	FString ProtectedLightKey(AActor* Actor, ULightComponent* Light)
	{
		if (!Actor || !Light)
		{
			return FString();
		}
		return OrganoidPlaytestActions::ActorPackage(Actor)
			+ TEXT("::")
			+ OrganoidPlaytestActions::ActorLabel(Actor)
			+ TEXT(".")
			+ Light->GetName();
	}

	bool PackageEquals(const FString& Package, const TCHAR* Expected)
	{
		return OrganoidPlaytestActions::NormalizePackage(Package).Equals(Expected, ESearchCase::IgnoreCase);
	}

	bool IsFacilityLightActor(const AActor* Actor)
	{
		return Actor && Actor->GetClass() && Actor->GetClass()->GetName().Contains(TEXT("FacilityLight"));
	}

	bool IsEmergencyFacilityRole(AActor* Actor)
	{
		const FOrganoidPlaytestPropValue Role = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("LightRole"));
		return Role.EnumInternal.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase)
			|| Role.Text.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase);
	}

	TArray<FName> ReadNameArray(UObject* Object, const TCHAR* PropertyName)
	{
		TArray<FName> Result;
		if (!Object || !PropertyName || !Object->GetClass())
		{
			return Result;
		}
		const FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		if (!ArrayProp)
		{
			return Result;
		}
		const FNameProperty* Inner = CastField<FNameProperty>(ArrayProp->Inner);
		if (!Inner)
		{
			return Result;
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Object));
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			Result.Add(Inner->GetPropertyValue(Helper.GetRawPtr(Index)));
		}
		return Result;
	}

	UObject* FindPowerSubsystem(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/ProjectOrganoid.ProjectOrganoidPowerSubsystem"));
		if (!Class)
		{
			return nullptr;
		}
		return World->GetSubsystemBase(Class);
	}

	FString EnumPropertyToText(FProperty* Property, const void* ValuePtr)
	{
		if (!Property || !ValuePtr)
		{
			return FString();
		}
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			if (UEnum* Enum = EnumProp->GetEnum())
			{
				return Enum->GetNameStringByValue(Value);
			}
			return FString::Printf(TEXT("%lld"), Value);
		}
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			const uint8 Value = ByteProp->GetPropertyValue(ValuePtr);
			if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
			{
				return Enum->GetNameStringByValue(Value);
			}
			return FString::FromInt(Value);
		}
		return FString();
	}

	FString CallPowerEnumFunction(UObject* Subsystem, const TCHAR* FunctionName, const TCHAR* SectorParam, uint8 SectorValue)
	{
		if (!Subsystem || !FunctionName)
		{
			return FString();
		}
		UFunction* Function = Subsystem->FindFunction(FName(FunctionName));
		if (!Function)
		{
			return FString();
		}
		TArray<uint8> Parms;
		Parms.AddZeroed(Function->ParmsSize);
		if (SectorParam)
		{
			if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, SectorParam))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
					static_cast<int64>(SectorValue));
			}
			else if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(Function, SectorParam))
			{
				ByteProp->SetPropertyValue(ByteProp->ContainerPtrToValuePtr<void>(Parms.GetData()), SectorValue);
			}
		}
		Subsystem->ProcessEvent(Function, Parms.GetData());
		if (FProperty* ReturnProp = Function->GetReturnProperty())
		{
			return EnumPropertyToText(ReturnProp, ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData()));
		}
		if (FProperty* Named = FindFProperty<FProperty>(Function, TEXT("ReturnValue")))
		{
			return EnumPropertyToText(Named, Named->ContainerPtrToValuePtr<void>(Parms.GetData()));
		}
		return FString();
	}

	FString ReadSectorPowerState(UWorld* World, uint8 SectorValue)
	{
		return CallPowerEnumFunction(FindPowerSubsystem(World), TEXT("GetSectorPowerState"), TEXT("Sector"), SectorValue);
	}

	bool NameArrayContains(const TArray<FName>& Names, const TCHAR* Token)
	{
		const FString Wanted(Token);
		for (const FName& Name : Names)
		{
			const FString Text = OrganoidPlaytestActions::NormalizePackage(Name.ToString());
			if (Text.Equals(Wanted, ESearchCase::IgnoreCase) || Name.ToString().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	struct FHologramLook
	{
		bool bAdminOnline = false;
		float LightAdmin = -1.0f;
		bool bNeuro = false;
		bool bCryo = false;
		bool bCompute = false;
		bool bReactor = false;
		float LightNeuro = -1.0f;
		float LightCryo = -1.0f;
		float LightCompute = -1.0f;
		float LightReactor = -1.0f;
	};

	struct FIsolationLook
	{
		FStreamLook Streams[UE_ARRAY_COUNT(StreamPackages)];
		bool bTerminalActivated[UE_ARRAY_COUNT(TerminalLabels)] = {};
		bool bTerminalPowered[UE_ARRAY_COUNT(TerminalLabels)] = {};
		FVector DoorLocation = FVector::ZeroVector;
		FVector DoorTriggerExtent = FVector::ZeroVector;
		bool bDoorOpen = false;
		bool bDoorLocked = false;
		bool bDoorAutomatic = false;
		FString FacilityState;
		FString AdminPowerState;
		FString NeuroPowerState;
		FString FacilityWidePowerState;
		FString DoorStatusLightKey;
		bool bAdminInitialized = false;
		bool bSecurityScanned = false;
		bool bDirectorOfficeVisited = false;
		bool bRecordsAccessed = false;
		bool bOperationsActivated = false;
		FHologramLook Hologram;
		TMap<FString, float> ProtectedLights;
		TMap<FString, FVector> Displays;
	};

	FStreamLook ReadStream(UWorld* World, const TCHAR* PackageName)
	{
		FStreamLook Look;
		if (!World)
		{
			return Look;
		}
		for (ULevelStreaming* Level : World->GetStreamingLevels())
		{
			if (!Level)
			{
				continue;
			}
			const FString Name = OrganoidPlaytestActions::NormalizePackage(Level->GetWorldAssetPackageFName().ToString());
			if (!Name.Equals(PackageName, ESearchCase::IgnoreCase))
			{
				continue;
			}
			Look.bFound = true;
			Look.bShouldBeLoaded = Level->ShouldBeLoaded();
			Look.bShouldBeVisible = Level->ShouldBeVisible();
			Look.bIsLoaded = Level->IsLevelLoaded();
			return Look;
		}
		return Look;
	}

	FHologramLook ReadHologram(UWorld* World)
	{
		FHologramLook Look;
		AActor* Actor = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel);
		if (!Actor)
		{
			return Look;
		}
		Look.bAdminOnline = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("bAdminOnline")).bBool;
		Look.bNeuro = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("bNeuroGeneticsOnline")).bBool;
		Look.bCryo = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("bCryoOnline")).bBool;
		Look.bCompute = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("bComputeOnline")).bBool;
		Look.bReactor = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("bReactorOnline")).bBool;
		if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, TEXT("Light_Admin"))))
		{
			Look.LightAdmin = Light->Intensity;
		}
		if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, TEXT("Light_NeuroGenetics"))))
		{
			Look.LightNeuro = Light->Intensity;
		}
		if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, TEXT("Light_Cryo"))))
		{
			Look.LightCryo = Light->Intensity;
		}
		if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, TEXT("Light_Compute"))))
		{
			Look.LightCompute = Light->Intensity;
		}
		if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, TEXT("Light_Reactor"))))
		{
			Look.LightReactor = Light->Intensity;
		}
		return Look;
	}

	void CaptureProtectedLights(UWorld* World, TMap<FString, float>& Out, FString& OutDoorStatusKey)
	{
		Out.Reset();
		OutDoorStatusKey.Reset();
		if (!World)
		{
			return;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || IsS20ZoneLight(Actor))
			{
				continue;
			}
			const bool bFacility = IsFacilityLightActor(Actor);
			const bool bSky = Actor->IsA(ASkyLight::StaticClass());
			const bool bDir = Actor->IsA(ADirectionalLight::StaticClass());
			const FString Label = OrganoidPlaytestActions::ActorLabel(Actor);
			const bool bHologram = Label.Equals(HologramLabel, ESearchCase::IgnoreCase);
			const bool bDoor = Label.Contains(TEXT("AccessDoor"));
			const bool bTerminal = Label.StartsWith(TEXT("Admin_Terminal_"), ESearchCase::IgnoreCase);
			if (!bFacility && !bSky && !bDir && !bHologram && !bDoor && !bTerminal)
			{
				continue;
			}
			if (bFacility)
			{
				continue;
			}
			TArray<ULightComponent*> Lights;
			Actor->GetComponents<ULightComponent>(Lights);
			for (ULightComponent* Light : Lights)
			{
				if (!Light)
				{
					continue;
				}
				const FString Key = ProtectedLightKey(Actor, Light);
				Out.Add(Key, Light->Intensity);
				if (bDoor && Light->GetName().Equals(TEXT("StatusLight"), ESearchCase::IgnoreCase))
				{
					OutDoorStatusKey = Key;
				}
			}
		}
	}

	FIsolationLook CaptureIsolation(UWorld* World)
	{
		FIsolationLook Look;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
		{
			Look.Streams[Index] = ReadStream(World, StreamPackages[Index]);
		}
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
		{
			if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
			{
				const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
				const FOrganoidPlaytestPropValue Powered = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bIsPowered"));
				Look.bTerminalActivated[Index] = Activated.bHasBool && Activated.bBool;
				Look.bTerminalPowered[Index] = Powered.bHasBool && Powered.bBool;
			}
		}
		if (AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World))
		{
			Look.DoorLocation = Door->GetActorLocation();
			Look.DoorTriggerExtent = OrganoidPlaytestActions::ReadBoxExtent(Door, TEXT("AccessTrigger"));
			const FOrganoidPlaytestPropValue Open = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bIsOpen"));
			const FOrganoidPlaytestPropValue Locked = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bLocked"));
			const FOrganoidPlaytestPropValue Automatic = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bAutomatic"));
			Look.bDoorOpen = Open.bHasBool && Open.bBool;
			Look.bDoorLocked = Locked.bHasBool && Locked.bBool;
			Look.bDoorAutomatic = Automatic.bHasBool && Automatic.bBool;
		}
		if (AActor* Controller = OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel))
		{
			Look.FacilityState = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("FacilityState")).Text;
			Look.bAdminInitialized = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bAdminInitialized")).bBool;
			Look.bSecurityScanned = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bSecurityScanned")).bBool;
			Look.bDirectorOfficeVisited = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bDirectorOfficeVisited")).bBool;
			Look.bRecordsAccessed = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bRecordsAccessed")).bBool;
			Look.bOperationsActivated = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bOperationsActivated")).bBool;
		}
		Look.Hologram = ReadHologram(World);
		CaptureProtectedLights(World, Look.ProtectedLights, Look.DoorStatusLightKey);
		Look.FacilityWidePowerState = ReadSectorPowerState(World, 0);
		Look.AdminPowerState = ReadSectorPowerState(World, 1);
		Look.NeuroPowerState = ReadSectorPowerState(World, 2);
		for (const TCHAR* Label : DisplayLabels)
		{
			if (AActor* Display = OrganoidPlaytestActions::FindUniqueByLabel(World, Label))
			{
				Look.Displays.Add(Label, Display->GetActorLocation());
			}
		}
		return Look;
	}

	void CollectDirtyPackageNames(TArray<FString>& Out)
	{
		Out.Reset();
		TArray<UPackage*> WorldDirty;
		TArray<UPackage*> ContentDirty;
		FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
		FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
		auto Add = [&Out](const TArray<UPackage*>& Packages)
		{
			for (UPackage* Package : Packages)
			{
				if (Package)
				{
					Out.AddUnique(Package->GetName());
				}
			}
		};
		Add(WorldDirty);
		Add(ContentDirty);
		Out.Sort();
	}

	class FS20AdminLightingFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			RoomIndex = 0;
			bAnyAssertFailed = false;
			bConfigFailed = false;
			bRestored = false;
			bClearTeleported = false;
			bAccessDoorAutoOpened = false;
			FailingRoom.Reset();
			FailingStage.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Owner.SetStage(TEXT("Abort"));
		}

		virtual void Tick(UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) override
		{
			FOrganoidPlaytestRecord* Record = Owner.GetActiveRecord();
			if (!Record)
			{
				return;
			}

			switch (Stage)
			{
			case EStage::Preflight:
				TickPreflight(Owner, *Record);
				break;
			case EStage::StartPie:
				TickStartPie(Owner, *Record);
				break;
			case EStage::WaitPieReady:
				TickWaitPieReady(Owner, *Record, DeltaTime);
				break;
			case EStage::ClearSpawn:
				TickClearSpawn(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertBaseline:
				TickAssertBaseline(Owner, *Record);
				break;
			case EStage::EnterRoom:
				TickEnterRoom(Owner, *Record);
				break;
			case EStage::WaitOverlap:
				TickWaitOverlap(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertRoom:
				TickAssertRoom(Owner, *Record);
				break;
			case EStage::Restore:
				TickRestore(Owner, *Record);
				break;
			case EStage::EndPie:
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitPieStopped;
				Owner.SetStage(TEXT("WaitPieStopped"));
				break;
			case EStage::WaitPieStopped:
				TickWaitPieStopped(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertDurable:
				TickAssertDurable(Owner, *Record);
				break;
			case EStage::Finalize:
				Finalize(Owner, *Record);
				break;
			}
		}

	private:
		enum class EStage : uint8
		{
			Preflight,
			StartPie,
			WaitPieReady,
			ClearSpawn,
			AssertBaseline,
			EnterRoom,
			WaitOverlap,
			AssertRoom,
			Restore,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		int32 RoomIndex = 0;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		bool bRestored = false;
		bool bClearTeleported = false;
		bool bAccessDoorAutoOpened = false;
		FString FailingRoom;
		FString FailingStage;
		FString AdminHashBefore;
		FString RoomTriggerHashBefore;
		FString LightControllerHashBefore;
		TArray<FString> DirtyBefore;
		FIsolationLook BaselineIsolation;
		float PreviousIntensities[RoomCount] = {};
		TWeakObjectPtr<AActor> LightController;
		TWeakObjectPtr<AActor> SectorController;

		void FailArchitectural(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			const FString& RoomId,
			const FString& ChainStage,
			const FString& Reason)
		{
			FailingRoom = RoomId;
			FailingStage = ChainStage;
			const FString Full = FString::Printf(
				TEXT("room=%s stage=%s %s"),
				RoomId.IsEmpty() ? TEXT("<none>") : *RoomId,
				*ChainStage,
				*Reason);
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Full;
			}
			bAnyAssertFailed = true;
			TryRestoreRuntime();
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		bool AssertTrue(
			FOrganoidPlaytestRecord& Record,
			const FString& Id,
			bool bPassed,
			const FString& Expected,
			const FString& Actual,
			const FString& ActorId,
			bool bDurable)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, bDurable);
			if (!bPassed)
			{
				bAnyAssertFailed = true;
				if (bDurable)
				{
					bConfigFailed = true;
				}
			}
			return bPassed;
		}

		void TryRestoreRuntime()
		{
			if (bRestored)
			{
				return;
			}
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Lights = LightController.Get();
			AActor* Sector = SectorController.Get();
			if (!World)
			{
				return;
			}
			if (Lights)
			{
				WriteNameProperty(Lights, TEXT("CurrentLightingZone"), NAME_None);
			}
			if (Sector)
			{
				WriteNameProperty(Sector, TEXT("CurrentRoom"), NAME_None);
			}
			if (UProjectOrganoidAdminFacilityStateSubsystem* State = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
			{
				State->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
			}
			CallApplyAdminZoneLighting(World, NAME_None, ActiveIntensity, InactiveMultiplier);
			bRestored = true;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Preflight"));
			if (!GEditor)
			{
				Record.State = EOrganoidPlaytestState::Blocked;
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Editor is not available."));
				return;
			}
			if (GEditor->IsPlaySessionInProgress())
			{
				Record.RecommendedNextAction = TEXT("Stop the existing PIE session, then rerun. The bot will not steal an in-progress Play session.");
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage)
				|| PackageIsDirty(AdminPackage)
				|| PackageIsDirty(RoomTriggerBpPackage)
				|| PackageIsDirty(LightControllerBpPackage))
			{
				Record.RecommendedNextAction = TEXT("Leave unsaved map/Blueprint work as-is. Save or discard it yourself, then rerun. The bot will not save or discard.");
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, BP_AdminRoomTrigger, or BP_AdminLightController is dirty. Refusing to start."));
				return;
			}

			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			RoomTriggerHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger.uasset")));
			LightControllerHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController.uasset")));
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("admin_hash_before"), AdminHashBefore);
			Record.AddActor(TEXT("room_trigger_hash_before"), RoomTriggerHashBefore);
			Record.AddActor(TEXT("light_controller_hash_before"), LightControllerHashBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("StartPie"));
			if (!Owner.RequestStartPie(MapPackage))
			{
				if (GEditor && GEditor->IsPlaySessionInProgress())
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
					return;
				}
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			(void)Record;
			WaitSeconds = 0.0f;
			Stage = EStage::WaitPieReady;
			Owner.SetStage(TEXT("WaitPieReady"));
		}

		void TickWaitPieReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			ACharacter* Character = OrganoidPlaytestActions::GetPlayerCharacter(World);
			TArray<AActor*> Controllers = World ? OrganoidPlaytestActions::FindActorsByLabel(World, LightControllerLabel) : TArray<AActor*>();
			const FStreamLook AdminStream = ReadStream(World, AdminPackage);
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}
			if (World && Character && Controllers.Num() == 1 && AdminStream.bIsLoaded && (bWalking || WaitSeconds > 8.0f))
			{
				bClearTeleported = false;
				WaitSeconds = 0.0f;
				Stage = EStage::ClearSpawn;
				Owner.SetStage(TEXT("ClearSpawn"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Record.AddActor(TEXT("player"), Character ? Character->GetName() : TEXT(""));
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE Admin lighting (world=%s pawn=%s controllers=%d admin_loaded=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						Controllers.Num(),
						AdminStream.bIsLoaded ? TEXT("true") : TEXT("false")));
			}
		}

		bool TeleportPawnTo(APawn* Pawn, const FVector& XY, float CapsuleZ)
		{
			if (!Pawn)
			{
				return false;
			}
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->GravityScale = 1.0f;
					Move->SetMovementMode(MOVE_Walking);
					Move->StopMovementImmediately();
				}
				Character->SetActorEnableCollision(true);
			}
			const bool bMoved = OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
			return bMoved;
		}

		float CapsuleZFor(APawn* Pawn) const
		{
			float CapsuleZ = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleZ = Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
				}
			}
			return CapsuleZ;
		}

		FVector PickClearLocation(UWorld* World, float CapsuleZ)
		{
			const FVector Candidates[] = {
				FVector(950.0f, 0.0f, CapsuleZ),
				FVector(1900.0f, 0.0f, CapsuleZ),
				FVector(2800.0f, 0.0f, CapsuleZ),
				FVector(400.0f, 250.0f, CapsuleZ),
			};
			for (const FVector& Candidate : Candidates)
			{
				bool bInside = false;
				for (const FRoomSpec& Spec : Rooms)
				{
					if (AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, Spec.TriggerLabel))
					{
						UBoxComponent* Box = FindTriggerBox(Trigger);
						if (!Box)
						{
							continue;
						}
						const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Candidate);
						const FVector Extent = Box->GetScaledBoxExtent();
						if (FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z)
						{
							bInside = true;
							break;
						}
					}
				}
				if (!bInside)
				{
					return Candidate;
				}
			}
			return Candidates[0];
		}

		void TickClearSpawn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"), TEXT("Lost PIE world or pawn before spawn clear."));
				return;
			}
			if (!bClearTeleported)
			{
				const float CapsuleZ = CapsuleZFor(Pawn);
				const FVector Clear = PickClearLocation(World, CapsuleZ);
				TeleportPawnTo(Pawn, Clear, CapsuleZ);
				bClearTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 0.25f)
			{
				return;
			}

			AActor* Lights = OrganoidPlaytestActions::FindUniqueByLabel(World, LightControllerLabel);
			AActor* Sector = OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel);
			LightController = Lights;
			SectorController = Sector;
			if (Lights)
			{
				WriteNameProperty(Lights, TEXT("CurrentLightingZone"), NAME_None);
			}
			if (Sector)
			{
				WriteNameProperty(Sector, TEXT("CurrentRoom"), NAME_None);
			}
			if (UProjectOrganoidAdminFacilityStateSubsystem* State = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
			{
				State->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
			}
			else
			{
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"),
					TEXT("UProjectOrganoidAdminFacilityStateSubsystem missing in PIE. S20 requires Normal Admin facility state."));
				return;
			}
			CallApplyAdminZoneLighting(World, NAME_None, ActiveIntensity, InactiveMultiplier);
			bRestored = false;
			Stage = EStage::AssertBaseline;
			Owner.SetStage(TEXT("AssertBaseline"));
		}

		AActor* FindZoneLight(UWorld* World, const TCHAR* Label)
		{
			return OrganoidPlaytestActions::FindUniqueByLabel(World, Label);
		}

		int32 CountS20Lights(UWorld* World, TArray<AActor*>& OutLights)
		{
			OutLights.Reset();
			if (!World)
			{
				return 0;
			}
			for (TActorIterator<APointLight> It(World); It; ++It)
			{
				APointLight* Light = *It;
				if (IsS20ZoneLight(Light))
				{
					OutLights.Add(Light);
				}
			}
			return OutLights.Num();
		}

		void TickAssertBaseline(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Lights = LightController.Get();
			AActor* Sector = SectorController.Get();
			Record.AddActor(TEXT("player"), Pawn ? OrganoidPlaytestActions::ActorLabel(Pawn) : TEXT(""));
			Record.AddActor(TEXT("player_class"), Pawn && Pawn->GetClass() ? Pawn->GetClass()->GetName() : TEXT(""));

			const bool bPawnClass = Pawn && Pawn->GetClass() && Pawn->GetClass()->GetName().Contains(TEXT("ProjectOrganoidCharacter"));
			if (!AssertTrue(Record, TEXT("baseline.player_is_project_organoid_character"), bPawnClass,
				TEXT("ProjectOrganoidCharacter"), Record.Actors.FindRef(TEXT("player_class")), TEXT("player"), true))
			{
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"), TEXT("PIE pawn is not AProjectOrganoidCharacter; overlap chain cannot fire."));
				return;
			}

			TArray<AActor*> Controllers = OrganoidPlaytestActions::FindActorsByLabel(World, LightControllerLabel);
			if (!AssertTrue(Record, TEXT("baseline.exactly_one_light_controller"), Controllers.Num() == 1,
				TEXT("1"), FString::FromInt(Controllers.Num()), LightControllerLabel, true))
			{
				Record.MarkNeedsApproval(
					TEXT("Admin_LightController count is not exactly one."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"), TEXT("exactly one Admin_LightController required."));
				return;
			}
			Lights = Controllers[0];
			LightController = Lights;
			const FString LightPkg = OrganoidPlaytestActions::ActorPackage(Lights);
			AssertTrue(Record, TEXT("baseline.light_controller_owner"), LightPkg.Equals(AdminPackage),
				AdminPackage, LightPkg, LightControllerLabel, true);

			TArray<AActor*> Sectors = OrganoidPlaytestActions::FindActorsByLabel(World, SectorControllerLabel);
			AssertTrue(Record, TEXT("baseline.exactly_one_sector_controller"), Sectors.Num() == 1,
				TEXT("1"), FString::FromInt(Sectors.Num()), SectorControllerLabel, true);
			if (Sectors.Num() == 1)
			{
				Sector = Sectors[0];
				SectorController = Sector;
			}

			TArray<AActor*> ZoneLights;
			CountS20Lights(World, ZoneLights);
			AssertTrue(Record, TEXT("baseline.transit_room_index"),
				FCString::Strcmp(Rooms[TransitRoomIndex].RoomId, TEXT("Transit")) == 0,
				TEXT("Transit"), Rooms[TransitRoomIndex].RoomId, TEXT(""), true);
			if (!AssertTrue(Record, TEXT("baseline.exactly_ten_s20_lights"), ZoneLights.Num() == RoomCount,
				TEXT("10"), FString::FromInt(ZoneLights.Num()), TEXT("Admin_ZoneLight_*"), true))
			{
				Record.MarkNeedsApproval(
					TEXT("S20 zone light count is not exactly ten."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"), TEXT("exactly ten Admin_ZoneLight_* required."));
				return;
			}

			for (const FRoomSpec& Spec : Rooms)
			{
				AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, Spec.TriggerLabel);
				AActor* Light = FindZoneLight(World, Spec.LightLabel);
				AssertTrue(Record, FString::Printf(TEXT("baseline.trigger_%s"), Spec.RoomId), Trigger != nullptr,
					TEXT("present"), Trigger ? TEXT("present") : TEXT("missing"), Spec.TriggerLabel, true);
				AssertTrue(Record, FString::Printf(TEXT("baseline.light_%s"), Spec.RoomId), Light != nullptr,
					TEXT("present"), Light ? TEXT("present") : TEXT("missing"), Spec.LightLabel, true);
				if (Trigger)
				{
					const FString Pkg = OrganoidPlaytestActions::ActorPackage(Trigger);
					AssertTrue(Record, FString::Printf(TEXT("baseline.trigger_owner_%s"), Spec.RoomId),
						Pkg.Equals(AdminPackage), AdminPackage, Pkg, Spec.TriggerLabel, true);
					const FOrganoidPlaytestPropValue RoomId = OrganoidPlaytestActions::ReadProperty(Trigger, TEXT("RoomID"));
					AssertTrue(Record, FString::Printf(TEXT("baseline.trigger_roomid_%s"), Spec.RoomId),
						RoomId.Text.Equals(Spec.RoomId, ESearchCase::CaseSensitive),
						Spec.RoomId, RoomId.bFound ? RoomId.Text : TEXT("<missing>"), Spec.TriggerLabel, true);
				}
				if (Light)
				{
					const FString Pkg = OrganoidPlaytestActions::ActorPackage(Light);
					AssertTrue(Record, FString::Printf(TEXT("baseline.light_owner_%s"), Spec.RoomId),
						Pkg.Equals(AdminPackage), AdminPackage, Pkg, Spec.LightLabel, true);
					const float Intensity = LightIntensity(Light);
					AssertTrue(Record, FString::Printf(TEXT("baseline.light_intensity_%s"), Spec.RoomId),
						IntensityEquals(Intensity, ActiveIntensity),
						IntensityText(ActiveIntensity), IntensityText(Intensity), Spec.LightLabel, false);
					const FLinearColor ExpectedWhite = UProjectOrganoidAdminLightingLibrary::GetAdminFacilityStateLightColor(
						EProjectOrganoidAdminFacilityState::Normal);
					const FLinearColor ActualColor = ZoneLightColor(Light);
					AssertTrue(Record, FString::Printf(TEXT("baseline.light_color_%s"), Spec.RoomId),
						ColorEquals(ActualColor, ExpectedWhite),
						ColorText(ExpectedWhite), ColorText(ActualColor), Spec.LightLabel, false);
				}
			}

			if (Lights)
			{
				const FOrganoidPlaytestPropValue Zone = OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone"));
				const bool bNone = !Zone.bFound || Zone.Text.IsEmpty() || Zone.Text.Equals(TEXT("None"), ESearchCase::IgnoreCase);
				AssertTrue(Record, TEXT("baseline.CurrentLightingZone"), bNone, TEXT("None"),
					Zone.bFound ? Zone.Text : TEXT("<missing>"), LightControllerLabel, false);
				const FOrganoidPlaytestPropValue Mult = OrganoidPlaytestActions::ReadProperty(Lights, TEXT("InactiveMultiplier"));
				AssertTrue(Record, TEXT("baseline.InactiveMultiplier"),
					Mult.bHasNumber && FMath::IsNearlyEqual(static_cast<float>(Mult.Number), InactiveMultiplier, 0.001f),
					TEXT("0.25"), Mult.bFound ? Mult.Text : TEXT("<missing>"), LightControllerLabel, true);
				const FOrganoidPlaytestPropValue Authored = OrganoidPlaytestActions::ReadProperty(Lights, TEXT("AuthoredIntensity"));
				AssertTrue(Record, TEXT("baseline.AuthoredIntensity"),
					Authored.bHasNumber && FMath::IsNearlyEqual(static_cast<float>(Authored.Number), ActiveIntensity, IntensityTolerance),
					TEXT("5000"), Authored.bFound ? Authored.Text : TEXT("<missing>"), LightControllerLabel, true);
			}

			BaselineIsolation = CaptureIsolation(World);
			AssertTrue(Record, TEXT("baseline.admin_streamed"),
				BaselineIsolation.Streams[0].bFound && BaselineIsolation.Streams[0].bIsLoaded,
				TEXT("Admin loaded"),
				BaselineIsolation.Streams[0].bIsLoaded ? TEXT("loaded") : TEXT("not loaded"),
				AdminPackage, false);
			AssertTrue(Record, TEXT("baseline.neuro_not_handoff"),
				BaselineIsolation.Streams[1].bFound && !BaselineIsolation.Streams[1].bIsLoaded
					&& !BaselineIsolation.Streams[1].bShouldBeLoaded,
				TEXT("NeuroGenetics not loaded before Transit"),
				StreamFlagText(BaselineIsolation.Streams[1]),
				NeuroPackage, false);
			AssertTrue(Record, TEXT("baseline.power.Admin"),
				BaselineIsolation.AdminPowerState.Equals(TEXT("Online"), ESearchCase::IgnoreCase),
				TEXT("Online"),
				BaselineIsolation.AdminPowerState.IsEmpty() ? TEXT("<missing>") : BaselineIsolation.AdminPowerState,
				TEXT("PowerSubsystem"), false);
			AssertTrue(Record, TEXT("baseline.power.NeuroGenetics"),
				BaselineIsolation.NeuroPowerState.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase),
				TEXT("Emergency"),
				BaselineIsolation.NeuroPowerState.IsEmpty() ? TEXT("<missing>") : BaselineIsolation.NeuroPowerState,
				TEXT("PowerSubsystem"), false);

			{
				TArray<AActor*> Seams = OrganoidPlaytestActions::FindActorsByLabel(World, SeamBandLabel);
				AssertTrue(Record, TEXT("baseline.seam_band_unique"), Seams.Num() == 1,
					TEXT("1"), FString::FromInt(Seams.Num()), SeamBandLabel, true);
				if (Seams.Num() == 1)
				{
					const TArray<FName> Requested = ReadNameArray(Seams[0], TEXT("RequestedStreamingLevels"));
					FString RequestedText;
					for (const FName& Name : Requested)
					{
						if (!RequestedText.IsEmpty())
						{
							RequestedText += TEXT(",");
						}
						RequestedText += Name.ToString();
					}
					AssertTrue(Record, TEXT("baseline.seam_requests_Admin"),
						NameArrayContains(Requested, TEXT("SL_Epitope_Admin")),
						TEXT("SL_Epitope_Admin"),
						RequestedText,
						SeamBandLabel, true);
					AssertTrue(Record, TEXT("baseline.seam_requests_NeuroGenetics"),
						NameArrayContains(Requested, TEXT("SL_Epitope_NeuroGenetics")),
						TEXT("SL_Epitope_NeuroGenetics"),
						RequestedText,
						SeamBandLabel, true);
					const FOrganoidPlaytestPropValue Region = OrganoidPlaytestActions::ReadProperty(Seams[0], TEXT("RegionContextTag"));
					const bool bNoRegion = !Region.bFound
						|| Region.EnumInternal.Equals(TEXT("None"), ESearchCase::IgnoreCase)
						|| Region.Text.Equals(TEXT("None"), ESearchCase::IgnoreCase)
						|| Region.Text.IsEmpty();
					AssertTrue(Record, TEXT("baseline.seam_region_none"),
						bNoRegion,
						TEXT("None (seam band, not player-region volume)"),
						Region.bFound ? (Region.EnumInternal.IsEmpty() ? Region.Text : Region.EnumInternal) : TEXT("<missing>"),
						SeamBandLabel, false);
				}
			}

			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				AssertTrue(Record, FString::Printf(TEXT("baseline.terminal_powered_%s"), TerminalLabels[Index]),
					BaselineIsolation.bTerminalPowered[Index], TEXT("true"),
					BoolText(BaselineIsolation.bTerminalPowered[Index]), TerminalLabels[Index], false);
				AssertTrue(Record, FString::Printf(TEXT("baseline.terminal_unactivated_%s"), TerminalLabels[Index]),
					!BaselineIsolation.bTerminalActivated[Index], TEXT("false"),
					BoolText(BaselineIsolation.bTerminalActivated[Index]), TerminalLabels[Index], false);
			}

			const bool bAlert = BaselineIsolation.FacilityState.Contains(TEXT("Alert"), ESearchCase::IgnoreCase)
				|| BaselineIsolation.FacilityState.Contains(TEXT("Lockdown"), ESearchCase::IgnoreCase);
			AssertTrue(Record, TEXT("baseline.no_alert_lockdown"), !bAlert, TEXT("not Alert/Lockdown"),
				BaselineIsolation.FacilityState, SectorControllerLabel, false);

			if (bAnyAssertFailed)
			{
				if (bConfigFailed)
				{
					Record.MarkNeedsApproval(
						TEXT("Durable Section 20 baseline does not match the expected lighting architecture."),
						TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				}
				FailArchitectural(Owner, Record, TEXT(""), TEXT("baseline"), Record.FailureReason);
				return;
			}

			for (int32 Index = 0; Index < RoomCount; ++Index)
			{
				PreviousIntensities[Index] = ActiveIntensity;
			}
			RoomIndex = 0;
			Stage = EStage::EnterRoom;
			Owner.SetStage(TEXT("EnterRoom_Vestibule"));
		}

		void TickEnterRoom(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			const FRoomSpec& Spec = Rooms[RoomIndex];
			AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, Spec.TriggerLabel);
			if (!Pawn || !Trigger)
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("player enters room trigger"),
					TEXT("Lost player pawn or room trigger before teleport."));
				return;
			}

			UWorld* UnusedWorld = World;
			(void)UnusedWorld;
			const float CapsuleZ = CapsuleZFor(Pawn);
			const FVector Dest = Trigger->GetActorLocation();
			if (!TeleportPawnTo(Pawn, Dest, CapsuleZ))
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("player enters room trigger"),
					TEXT("Teleport into room trigger failed."));
				return;
			}
			if (UBoxComponent* Box = FindTriggerBox(Trigger))
			{
				Box->UpdateOverlaps();
			}
			WaitSeconds = 0.0f;
			Stage = EStage::WaitOverlap;
			Owner.SetStage(FString::Printf(TEXT("WaitOverlap_%s"), Spec.RoomId));
		}

		void TickWaitOverlap(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OverlapWaitSeconds)
			{
				return;
			}
			(void)Owner;
			(void)Record;
			Stage = EStage::AssertRoom;
			Owner.SetStage(FString::Printf(TEXT("AssertRoom_%s"), Rooms[RoomIndex].RoomId));
		}

		bool AssertIsolation(FOrganoidPlaytestRecord& Record, UWorld* World, APawn* Pawn, const FString& Prefix, int32 LastVisitedInclusive)
		{
			const FIsolationLook Live = CaptureIsolation(World);
			bool bAll = true;
			const bool bNeuroHandoff = LastVisitedInclusive >= TransitRoomIndex;
			const bool bInTransitRoom = LastVisitedInclusive == TransitRoomIndex;

			auto AssertStream = [&](int32 Index, bool bPass, const FString& Expected, const FString& Actual)
			{
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.stream_%s"), *Prefix, StreamPackages[Index]),
					bPass,
					Expected,
					Actual,
					StreamPackages[Index],
					false);
			};

			{
				const FStreamLook& Admin = Live.Streams[0];
				AssertStream(
					0,
					Admin.bFound && Admin.bShouldBeLoaded && Admin.bShouldBeVisible && Admin.bIsLoaded,
					TEXT("Admin shouldBeLoaded=true shouldBeVisible=true isLoaded=true while pawn is in Admin"),
					StreamFlagText(Admin));
			}
			{
				const FStreamLook& Neuro = Live.Streams[1];
				if (!bNeuroHandoff)
				{
					const FStreamLook& Expected = BaselineIsolation.Streams[1];
					const bool bPass = Expected.bFound && Neuro.bFound
						&& Expected.bShouldBeLoaded == Neuro.bShouldBeLoaded
						&& Expected.bShouldBeVisible == Neuro.bShouldBeVisible
						&& Expected.bIsLoaded == Neuro.bIsLoaded;
					AssertStream(
						1,
						bPass,
						FString::Printf(TEXT("pre-Transit baseline %s"), *StreamFlagText(Expected)),
						StreamFlagText(Neuro));
				}
				else
				{
					AssertStream(
						1,
						Neuro.bFound && Neuro.bShouldBeLoaded && Neuro.bShouldBeVisible && Neuro.bIsLoaded,
						TEXT("Transit seam StreamBand_Admin_NeuroGenetics requests SL_Epitope_NeuroGenetics: shouldBeLoaded=true shouldBeVisible=true isLoaded=true"),
						StreamFlagText(Neuro));
				}
			}
			for (int32 Index = 2; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
			{
				const FStreamLook& Expected = BaselineIsolation.Streams[Index];
				const FStreamLook& Actual = Live.Streams[Index];
				const bool bPass = Expected.bFound && Actual.bFound
					&& Expected.bShouldBeLoaded == Actual.bShouldBeLoaded
					&& Expected.bShouldBeVisible == Actual.bShouldBeVisible
					&& Expected.bIsLoaded == Actual.bIsLoaded;
				AssertStream(
					Index,
					bPass,
					FString::Printf(TEXT("unrelated partition unchanged %s"), *StreamFlagText(Expected)),
					StreamFlagText(Actual));
			}

			if (bInTransitRoom)
			{
				AActor* Seam = OrganoidPlaytestActions::FindUniqueByLabel(World, SeamBandLabel);
				const bool bInsideSeam = Seam && IsPawnInsideTrigger(Pawn, Seam);
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".stream_seam_StreamBand_Admin_NeuroGenetics"),
					bInsideSeam,
					TEXT("pawn overlapping StreamBand_Admin_NeuroGenetics"),
					bInsideSeam ? TEXT("overlapping") : TEXT("not overlapping"),
					SeamBandLabel,
					false);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.terminal_%s"), *Prefix, TerminalLabels[Index]),
					Live.bTerminalActivated[Index] == BaselineIsolation.bTerminalActivated[Index]
						&& Live.bTerminalPowered[Index] == BaselineIsolation.bTerminalPowered[Index],
					TEXT("powered unactivated"),
					FString::Printf(TEXT("activated=%s powered=%s"),
						*BoolText(Live.bTerminalActivated[Index]),
						*BoolText(Live.bTerminalPowered[Index])),
					TerminalLabels[Index],
					false);
			}
			if (AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World))
			{
				const FVector Extent = OrganoidPlaytestActions::ReadBoxExtent(Door, TEXT("AccessTrigger"));
				const bool bInAccessTrigger = IsPawnInsideNamedBox(Pawn, Door, TEXT("AccessTrigger"));
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.actor_transform"),
					Door->GetActorLocation().Equals(BaselineIsolation.DoorLocation, 1.0f),
					TEXT("unchanged"),
					FString::Printf(TEXT("dist=%.1f"), FVector::Dist(Door->GetActorLocation(), BaselineIsolation.DoorLocation)),
					DoorLabel,
					false);
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.AccessTrigger_extent"),
					Extent.Equals(BaselineIsolation.DoorTriggerExtent, 0.5f),
					TEXT("unchanged"),
					Extent.ToCompactString(),
					DoorLabel,
					false);
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.bLocked"),
					Live.bDoorLocked == BaselineIsolation.bDoorLocked,
					BoolText(BaselineIsolation.bDoorLocked),
					BoolText(Live.bDoorLocked),
					DoorLabel,
					false);
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.bAutomatic"),
					Live.bDoorAutomatic == BaselineIsolation.bDoorAutomatic,
					BoolText(BaselineIsolation.bDoorAutomatic),
					BoolText(Live.bDoorAutomatic),
					DoorLabel,
					false);

				const float* BaselineStatus = !Live.DoorStatusLightKey.IsEmpty()
					? BaselineIsolation.ProtectedLights.Find(Live.DoorStatusLightKey)
					: (!BaselineIsolation.DoorStatusLightKey.IsEmpty()
						? BaselineIsolation.ProtectedLights.Find(BaselineIsolation.DoorStatusLightKey)
						: nullptr);
				const float* LiveStatus = !Live.DoorStatusLightKey.IsEmpty()
					? Live.ProtectedLights.Find(Live.DoorStatusLightKey)
					: nullptr;
				const bool bStatusOk = BaselineStatus && LiveStatus && IntensityEquals(*LiveStatus, *BaselineStatus);
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.StatusLight"),
					bStatusOk,
					BaselineStatus ? IntensityText(*BaselineStatus) : TEXT("<missing>"),
					LiveStatus ? IntensityText(*LiveStatus) : TEXT("<missing>"),
					DoorLabel,
					false);

				if (bInAccessTrigger
					&& Live.bDoorAutomatic
					&& !Live.bDoorLocked
					&& !BaselineIsolation.bDoorLocked)
				{
					bAccessDoorAutoOpened = true;
				}

				bool bOpenOk = true;
				FString OpenExpected = TEXT("unchanged, or S16 AccessTrigger auto-open / sticky after authorized overlap");
				FString OpenActual = FString::Printf(
					TEXT("open=%s baseline=%s in_trigger=%s auto=%s locked=%s authorized_auto_open=%s"),
					*BoolText(Live.bDoorOpen),
					*BoolText(BaselineIsolation.bDoorOpen),
					*BoolText(bInAccessTrigger),
					*BoolText(Live.bDoorAutomatic),
					*BoolText(Live.bDoorLocked),
					*BoolText(bAccessDoorAutoOpened));
				if (Live.bDoorOpen == BaselineIsolation.bDoorOpen)
				{
					bOpenOk = true;
				}
				else if (Live.bDoorOpen && !BaselineIsolation.bDoorOpen)
				{
					bOpenOk = bAccessDoorAutoOpened;
					OpenExpected = TEXT("false->true only after authorized S16 AccessTrigger overlap (bAutomatic, unlocked)");
				}
				else
				{
					bOpenOk = false;
					OpenExpected = TEXT("bIsOpen must not close during S20; S16 has no auto-close");
				}
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".access_door.bIsOpen_s16_auto_path"),
					bOpenOk,
					OpenExpected,
					OpenActual,
					DoorLabel,
					false);
			}
			const bool bAlert = Live.FacilityState.Contains(TEXT("Alert"), ESearchCase::IgnoreCase)
				|| Live.FacilityState.Contains(TEXT("Lockdown"), ESearchCase::IgnoreCase);
			bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.no_alert_lockdown"), *Prefix),
				!bAlert && Live.FacilityState.Equals(BaselineIsolation.FacilityState),
				BaselineIsolation.FacilityState, Live.FacilityState, SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Admin"),
				Live.AdminPowerState.Equals(TEXT("Online"), ESearchCase::IgnoreCase),
				TEXT("Online"),
				Live.AdminPowerState.IsEmpty() ? TEXT("<missing>") : Live.AdminPowerState,
				TEXT("PowerSubsystem"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.NeuroGenetics"),
				Live.NeuroPowerState.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase),
				TEXT("Emergency"),
				Live.NeuroPowerState.IsEmpty() ? TEXT("<missing>") : Live.NeuroPowerState,
				TEXT("PowerSubsystem"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.FacilityWide"),
				Live.FacilityWidePowerState.Equals(BaselineIsolation.FacilityWidePowerState, ESearchCase::IgnoreCase)
					&& !Live.FacilityWidePowerState.Equals(TEXT("Blackout"), ESearchCase::IgnoreCase)
					&& !Live.FacilityWidePowerState.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase),
				BaselineIsolation.FacilityWidePowerState.IsEmpty() ? TEXT("Online") : BaselineIsolation.FacilityWidePowerState,
				Live.FacilityWidePowerState.IsEmpty() ? TEXT("<missing>") : Live.FacilityWidePowerState,
				TEXT("PowerSubsystem"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Admin_not_emergency_or_blackout"),
				!Live.AdminPowerState.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase)
					&& !Live.AdminPowerState.Equals(TEXT("Blackout"), ESearchCase::IgnoreCase),
				TEXT("Admin remains Online"),
				Live.AdminPowerState.IsEmpty() ? TEXT("<missing>") : Live.AdminPowerState,
				TEXT("PowerSubsystem"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bAdminInitialized"),
				Live.bAdminInitialized == BaselineIsolation.bAdminInitialized,
				BoolText(BaselineIsolation.bAdminInitialized),
				BoolText(Live.bAdminInitialized),
				SectorControllerLabel, false);

			bool bExpectSecurity = BaselineIsolation.bSecurityScanned;
			bool bExpectRecords = BaselineIsolation.bRecordsAccessed;
			bool bExpectDirector = BaselineIsolation.bDirectorOfficeVisited;
			bool bExpectOperations = BaselineIsolation.bOperationsActivated;
			for (int32 Visited = 0; Visited <= LastVisitedInclusive && Visited < RoomCount; ++Visited)
			{
				const FString VisitedId(Rooms[Visited].RoomId);
				if (VisitedId.Equals(TEXT("Security")))
				{
					bExpectSecurity = true;
				}
				else if (VisitedId.Equals(TEXT("Records")))
				{
					bExpectRecords = true;
				}
				else if (VisitedId.Equals(TEXT("DirectorSuite")))
				{
					bExpectDirector = true;
				}
				else if (VisitedId.Equals(TEXT("Operations")))
				{
					bExpectOperations = true;
				}
			}

			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bSecurityScanned"),
				Live.bSecurityScanned == bExpectSecurity,
				bExpectSecurity ? TEXT("true after Security SetCurrentRoom") : TEXT("false until Security overlap"),
				BoolText(Live.bSecurityScanned),
				SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bRecordsAccessed"),
				Live.bRecordsAccessed == bExpectRecords,
				bExpectRecords ? TEXT("true after Records SetCurrentRoom") : TEXT("false until Records overlap"),
				BoolText(Live.bRecordsAccessed),
				SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bDirectorOfficeVisited"),
				Live.bDirectorOfficeVisited == bExpectDirector,
				bExpectDirector ? TEXT("true after DirectorSuite SetCurrentRoom") : TEXT("false until DirectorSuite overlap"),
				BoolText(Live.bDirectorOfficeVisited),
				SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bOperationsActivated"),
				Live.bOperationsActivated == bExpectOperations,
				bExpectOperations ? TEXT("true after Operations SetCurrentRoom") : TEXT("false until Operations overlap"),
				BoolText(Live.bOperationsActivated),
				SectorControllerLabel, false);

			const FHologramLook& H0 = BaselineIsolation.Hologram;
			const FHologramLook& H1 = Live.Hologram;
			bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.hologram_baseline"), *Prefix),
				H0.bAdminOnline == H1.bAdminOnline && H0.bNeuro == H1.bNeuro && H0.bCryo == H1.bCryo
					&& H0.bCompute == H1.bCompute && H0.bReactor == H1.bReactor
					&& IntensityEquals(H0.LightAdmin, H1.LightAdmin),
				TEXT("S19 baseline"),
				FString::Printf(TEXT("admin_online=%s light=%.1f"), *BoolText(H1.bAdminOnline), H1.LightAdmin),
				HologramLabel, false);

			for (const auto& Pair : BaselineIsolation.ProtectedLights)
			{
				const float* LiveValue = Live.ProtectedLights.Find(Pair.Key);
				const bool bPass = LiveValue && IntensityEquals(*LiveValue, Pair.Value);
				bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.protected_light_%s"), *Prefix, *Pair.Key),
					bPass, IntensityText(Pair.Value), LiveValue ? IntensityText(*LiveValue) : TEXT("<missing>"),
					Pair.Key, false);
			}

			int32 AdminSectorCount = 0;
			int32 AdminEmergencyCount = 0;
			int32 NeuroSectorCount = 0;
			int32 NeuroEmergencyCount = 0;
			if (World)
			{
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Actor = *It;
					if (!Actor || !IsFacilityLightActor(Actor) || IsS20ZoneLight(Actor))
					{
						continue;
					}
					const FString Package = OrganoidPlaytestActions::ActorPackage(Actor);
					const bool bEmergency = IsEmergencyFacilityRole(Actor);
					const bool bAdminOwned = PackageEquals(Package, AdminPackage);
					const bool bNeuroOwned = PackageEquals(Package, NeuroPackage);
					TArray<ULightComponent*> Lights;
					Actor->GetComponents<ULightComponent>(Lights);
					for (ULightComponent* Light : Lights)
					{
						if (!Light)
						{
							continue;
						}
						const FString Key = ProtectedLightKey(Actor, Light);
						const float Intensity = Light->Intensity;
						const bool bS20Value = IntensityEquals(Intensity, ActiveIntensity)
							|| IntensityEquals(Intensity, InactiveIntensity);
						bAll &= AssertTrue(
							Record,
							Prefix + TEXT(".facilitylight_not_s20_zone_") + Key,
							!bS20Value,
							TEXT("must not be 5000 or 1250"),
							IntensityText(Intensity),
							Key,
							false);

						if (bAdminOwned)
						{
							if (bEmergency)
							{
								++AdminEmergencyCount;
								bAll &= AssertTrue(
									Record,
									Prefix + TEXT(".facilitylight_admin_emergency_") + Key,
									IntensityEquals(Intensity, 0.0f),
									TEXT("0 (Admin Online)"),
									IntensityText(Intensity),
									Key,
									false);
							}
							else
							{
								++AdminSectorCount;
								bAll &= AssertTrue(
									Record,
									Prefix + TEXT(".facilitylight_admin_sector_") + Key,
									IntensityEquals(Intensity, AdminSectorIntensity)
										&& !IntensityEquals(Intensity, NeuroEmergencySectorIntensity)
										&& !IntensityEquals(Intensity, NeuroEmergencyFixtureIntensity),
									IntensityText(AdminSectorIntensity),
									IntensityText(Intensity),
									Key,
									false);
							}
						}
						else if (bNeuroOwned)
						{
							if (!bNeuroHandoff)
							{
								bAll &= AssertTrue(
									Record,
									Prefix + TEXT(".facilitylight_neuro_unexpected_") + Key,
									false,
									TEXT("NeuroGenetics FacilityLight absent before Transit seam"),
									IntensityText(Intensity),
									Key,
									false);
								continue;
							}
							if (bEmergency)
							{
								++NeuroEmergencyCount;
								bAll &= AssertTrue(
									Record,
									Prefix + TEXT(".facilitylight_neuro_emergency_") + Key,
									IntensityEquals(Intensity, NeuroEmergencyFixtureIntensity),
									IntensityText(NeuroEmergencyFixtureIntensity),
									IntensityText(Intensity),
									Key,
									false);
							}
							else
							{
								++NeuroSectorCount;
								bAll &= AssertTrue(
									Record,
									Prefix + TEXT(".facilitylight_neuro_sector_") + Key,
									IntensityEquals(Intensity, NeuroEmergencySectorIntensity),
									TEXT("990 (4500 x 0.22 NeuroGenetics Emergency)"),
									IntensityText(Intensity),
									Key,
									false);
							}
						}
						else if (PackageEquals(Package, CryoPackage)
							|| PackageEquals(Package, ComputePackage)
							|| PackageEquals(Package, ReactorPackage))
						{
							bAll &= AssertTrue(
								Record,
								Prefix + TEXT(".facilitylight_wrong_sector_") + Key,
								false,
								TEXT("unrelated partition FacilityLight must not appear"),
								Package,
								Key,
								false);
						}
					}
				}
			}
			bAll &= AssertTrue(
				Record,
				Prefix + TEXT(".facilitylight_admin_present"),
				AdminSectorCount > 0 && AdminEmergencyCount > 0,
				TEXT("Admin sector + emergency FacilityLights present"),
				FString::Printf(TEXT("sector=%d emergency=%d"), AdminSectorCount, AdminEmergencyCount),
				AdminPackage,
				false);
			if (bNeuroHandoff)
			{
				bAll &= AssertTrue(
					Record,
					Prefix + TEXT(".facilitylight_neuro_present"),
					NeuroSectorCount > 0 && NeuroEmergencyCount > 0,
					TEXT("NeuroGenetics sector + emergency FacilityLights present after Transit seam"),
					FString::Printf(TEXT("sector=%d emergency=%d"), NeuroSectorCount, NeuroEmergencyCount),
					NeuroPackage,
					false);
			}
			for (const auto& Pair : BaselineIsolation.Displays)
			{
				const FVector* LiveLoc = Live.Displays.Find(Pair.Key);
				const bool bPass = LiveLoc && LiveLoc->Equals(Pair.Value, 1.0f);
				bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.display_%s"), *Prefix, *Pair.Key),
					bPass, TEXT("unchanged"), LiveLoc ? TEXT("unchanged") : TEXT("<missing>"), Pair.Key, false);
			}
			return bAll;
		}

		void TickAssertRoom(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Lights = LightController.Get();
			AActor* Sector = SectorController.Get();
			const FRoomSpec& Spec = Rooms[RoomIndex];
			const FString Prefix = FString::Printf(TEXT("room.%s"), Spec.RoomId);

			AActor* Overlapped = nullptr;
			const int32 OverlapCount = CountOverlappingRoomTriggers(Pawn, World, Overlapped, Spec.TriggerLabel);
			AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, Spec.TriggerLabel);
			const bool bInside = IsPawnInsideTrigger(Pawn, Trigger);
			const bool bEntered = bInside || Overlapped == Trigger;
			if (!AssertTrue(Record, Prefix + TEXT(".entered_trigger"), bEntered,
				Spec.TriggerLabel,
				bEntered ? Spec.TriggerLabel : TEXT("<not overlapping expected trigger>"),
				Spec.TriggerLabel, false))
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("player enters room trigger"),
					TEXT("Pawn is not inside the expected room trigger after teleport."));
				return;
			}
			AssertTrue(Record, Prefix + TEXT(".unique_trigger_overlap"),
				OverlapCount <= 1 || Overlapped == Trigger,
				TEXT("exactly the expected trigger"),
				FString::FromInt(OverlapCount), Spec.TriggerLabel, false);

			const FOrganoidPlaytestPropValue CurrentRoom = Sector
				? OrganoidPlaytestActions::ReadProperty(Sector, TEXT("CurrentRoom"))
				: FOrganoidPlaytestPropValue();
			if (!AssertTrue(Record, Prefix + TEXT(".SectorController.CurrentRoom"),
				CurrentRoom.Text.Equals(Spec.RoomId, ESearchCase::CaseSensitive),
				Spec.RoomId, CurrentRoom.bFound ? CurrentRoom.Text : TEXT("<missing>"),
				SectorControllerLabel, false))
			{
				FailArchitectural(Owner, Record, Spec.RoomId,
					TEXT("native C++ room identity -> Admin_SectorController.SetCurrentRoom"),
					FString::Printf(TEXT("expected=%s actual=%s"), Spec.RoomId, *CurrentRoom.Text));
				return;
			}

			const FOrganoidPlaytestPropValue Zone = Lights
				? OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone"))
				: FOrganoidPlaytestPropValue();
			if (!AssertTrue(Record, Prefix + TEXT(".LightController.CurrentLightingZone"),
				Zone.Text.Equals(Spec.RoomId, ESearchCase::CaseSensitive),
				Spec.RoomId, Zone.bFound ? Zone.Text : TEXT("<missing>"),
				LightControllerLabel, false))
			{
				FailArchitectural(Owner, Record, Spec.RoomId,
					TEXT("shared BP_AdminRoomTrigger ActorBeginOverlap -> RequestAdminLightingZone -> unique Admin_LightController.SetLightingZone"),
					FString::Printf(TEXT("expected=%s actual=%s"), Spec.RoomId, *Zone.Text));
				return;
			}

			int32 ActiveCount = 0;
			for (int32 Index = 0; Index < RoomCount; ++Index)
			{
				AActor* Light = FindZoneLight(World, Rooms[Index].LightLabel);
				const float Intensity = LightIntensity(Light);
				const bool bActive = Index == RoomIndex;
				const float Expected = bActive ? ActiveIntensity : InactiveIntensity;
				if (!AssertTrue(Record, FString::Printf(TEXT("%s.light_%s"), *Prefix, Rooms[Index].RoomId),
					IntensityEquals(Intensity, Expected),
					IntensityText(Expected), IntensityText(Intensity), Rooms[Index].LightLabel, false))
				{
					FailArchitectural(Owner, Record, Spec.RoomId, TEXT("S20 fixture intensities"),
						FString::Printf(TEXT("%s expected=%.1f actual=%.1f"), Rooms[Index].LightLabel, Expected, Intensity));
					return;
				}
				if (IntensityEquals(Intensity, ActiveIntensity))
				{
					++ActiveCount;
				}

				if (RoomIndex > 0)
				{
					if (Index == RoomIndex)
					{
						AssertTrue(Record, FString::Printf(TEXT("transition.%s_to_%s.new_rises"), Rooms[RoomIndex - 1].RoomId, Spec.RoomId),
							IntensityEquals(PreviousIntensities[Index], InactiveIntensity) && IntensityEquals(Intensity, ActiveIntensity),
							TEXT("1250 -> 5000"),
							FString::Printf(TEXT("%.1f -> %.1f"), PreviousIntensities[Index], Intensity),
							Spec.LightLabel, false);
					}
					else if (Index == RoomIndex - 1)
					{
						AssertTrue(Record, FString::Printf(TEXT("transition.%s_to_%s.previous_drops"), Rooms[RoomIndex - 1].RoomId, Spec.RoomId),
							IntensityEquals(PreviousIntensities[Index], ActiveIntensity) && IntensityEquals(Intensity, InactiveIntensity),
							TEXT("5000 -> 1250"),
							FString::Printf(TEXT("%.1f -> %.1f"), PreviousIntensities[Index], Intensity),
							Rooms[Index].LightLabel, false);
					}
				}
				else if (!bActive)
				{
					AssertTrue(Record, FString::Printf(TEXT("transition.None_to_Vestibule.%s_drops"), Rooms[Index].RoomId),
						IntensityEquals(PreviousIntensities[Index], ActiveIntensity) && IntensityEquals(Intensity, InactiveIntensity),
						TEXT("5000 -> 1250"),
						FString::Printf(TEXT("%.1f -> %.1f"), PreviousIntensities[Index], Intensity),
						Rooms[Index].LightLabel, false);
				}
			}

			if (!AssertTrue(Record, Prefix + TEXT(".exactly_one_active_s20_light"), ActiveCount == 1,
				TEXT("1"), FString::FromInt(ActiveCount), Spec.LightLabel, false))
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("S20 fixture intensities"),
					TEXT("exactly one S20 light must be at 5000."));
				return;
			}

			if (!AssertIsolation(Record, World, Pawn, Prefix + TEXT(".isolation"), RoomIndex))
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("isolation"),
					TEXT("Protected system or streaming isolation failed."));
				return;
			}

			if (bAnyAssertFailed)
			{
				FailArchitectural(Owner, Record, Spec.RoomId, TEXT("room assertions"), Record.FailureReason);
				return;
			}

			for (int32 Index = 0; Index < RoomCount; ++Index)
			{
				PreviousIntensities[Index] = LightIntensity(FindZoneLight(World, Rooms[Index].LightLabel));
			}

			++RoomIndex;
			if (RoomIndex < RoomCount)
			{
				Stage = EStage::EnterRoom;
				Owner.SetStage(FString::Printf(TEXT("EnterRoom_%s"), Rooms[RoomIndex].RoomId));
				return;
			}

			Stage = EStage::Restore;
			Owner.SetStage(TEXT("Restore"));
		}

		void TickRestore(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!World)
			{
				FailArchitectural(Owner, Record, TEXT(""), TEXT("restoration"), TEXT("Lost PIE world before restore."));
				return;
			}
			TryRestoreRuntime();
			AActor* Lights = LightController.Get();
			const FOrganoidPlaytestPropValue Zone = Lights
				? OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone"))
				: FOrganoidPlaytestPropValue();
			const bool bNone = !Zone.bFound || Zone.Text.IsEmpty() || Zone.Text.Equals(TEXT("None"), ESearchCase::IgnoreCase);
			AssertTrue(Record, TEXT("restore.CurrentLightingZone"), bNone, TEXT("None"),
				Zone.bFound ? Zone.Text : TEXT("<missing>"), LightControllerLabel, false);
			for (const FRoomSpec& Spec : Rooms)
			{
				const float Intensity = LightIntensity(FindZoneLight(World, Spec.LightLabel));
				AssertTrue(Record, FString::Printf(TEXT("restore.light_%s"), Spec.RoomId),
					IntensityEquals(Intensity, ActiveIntensity),
					IntensityText(ActiveIntensity), IntensityText(Intensity), Spec.LightLabel, false);
			}
			AssertIsolation(Record, World, OrganoidPlaytestActions::GetPlayerPawn(World), TEXT("restore.isolation"), RoomCount - 1);
			if (bAnyAssertFailed)
			{
				FailArchitectural(Owner, Record, TEXT(""), TEXT("restoration"), Record.FailureReason);
				return;
			}
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void TickWaitPieStopped(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			const bool bPie = GEditor && GEditor->IsPlaySessionInProgress();
			if (!bPie && !OrganoidPlaytestActions::GetPieWorld())
			{
				Stage = EStage::AssertDurable;
				Owner.SetStage(TEXT("AssertDurable"));
				return;
			}
			if (WaitSeconds > 30.0f)
			{
				Record.FailureReason = TEXT("Timed out waiting for PIE to stop.");
				bAnyAssertFailed = true;
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record.FailureReason);
			}
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("AssertDurable"));
			const bool bTrackedDirty = PackageIsDirty(MapPackage)
				|| PackageIsDirty(AdminPackage)
				|| PackageIsDirty(RoomTriggerBpPackage)
				|| PackageIsDirty(LightControllerBpPackage);
			AssertTrue(Record, TEXT("durable.tracked_packages_clean"), !bTrackedDirty,
				TEXT("not dirty"), bTrackedDirty ? TEXT("dirty") : TEXT("clean"), TEXT(""), true);

			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			TArray<FString> NewDirty;
			for (const FString& Name : DirtyAfter)
			{
				if (!DirtyBefore.Contains(Name))
				{
					NewDirty.Add(Name);
				}
			}
			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"), NewDirty.Num() == 0,
				TEXT("0"), FString::FromInt(NewDirty.Num()),
				NewDirty.Num() > 0 ? FString::Join(NewDirty, TEXT(",")) : TEXT(""), true);
			AssertTrue(Record, TEXT("durable.dirty_package_count_unchanged"),
				DirtyAfter.Num() == DirtyBefore.Num(),
				FString::FromInt(DirtyBefore.Num()), FString::FromInt(DirtyAfter.Num()), TEXT(""), true);

			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString RoomTriggerHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger.uasset")));
			const FString LightControllerHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController.uasset")));
			Record.AddActor(TEXT("admin_hash_after"), AdminHashAfter);
			Record.AddActor(TEXT("room_trigger_hash_after"), RoomTriggerHashAfter);
			Record.AddActor(TEXT("light_controller_hash_after"), LightControllerHashAfter);
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"), AdminHashAfter.Equals(AdminHashBefore),
				AdminHashBefore, AdminHashAfter, TEXT("SL_Epitope_Admin"), true);
			AssertTrue(Record, TEXT("durable.room_trigger_hash_unchanged"), RoomTriggerHashAfter.Equals(RoomTriggerHashBefore),
				RoomTriggerHashBefore, RoomTriggerHashAfter, TEXT("BP_AdminRoomTrigger"), true);
			AssertTrue(Record, TEXT("durable.light_controller_hash_unchanged"), LightControllerHashAfter.Equals(LightControllerHashBefore),
				LightControllerHashBefore, LightControllerHashAfter, TEXT("BP_AdminLightController"), true);

			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable Section 20 state changed. The playtest bot will not repair it."),
					TEXT("Inspect Admin lighting actors. Use OrganoidAIBridge if a mutation is required. Do not let the bot write."));
			}
			Stage = EStage::Finalize;
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			if (!FailingRoom.IsEmpty())
			{
				Record.AddActor(TEXT("failing_room"), FailingRoom);
				Record.AddActor(TEXT("failing_stage"), FailingStage);
			}
			const EOrganoidPlaytestState State = bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass;
			Owner.CompleteActive(State, Record.FailureReason);
		}
	};

	struct FS20AutoRegister
	{
		FS20AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS20AdminLightingFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS20AutoRegister GRegisterS20;
}
