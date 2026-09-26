#include "OrganoidAIBridgeWrites.h"
#include "OrganoidAIBridgeJson.h"
#include "OrganoidAIBridgeLogSink.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceConstant.h"

#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/CollisionProfile.h"
#include "Editor.h"
#include "EditorLevelUtils.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/OverlapResult.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Misc/App.h"
#include "GameFramework/Actor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Engine/UserDefinedEnum.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/Script.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"

using namespace OrganoidAIBridgeJson;

namespace
{
	const TCHAR* AdminPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	const TCHAR* EpitopePackage = TEXT("/Game/Maps/Lvl_Epitope");
	const TCHAR* NeuroPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	const TCHAR* CryoPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	const TCHAR* ComputePackage = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	const TCHAR* ReactorPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	const TCHAR* MainMenuPackage = TEXT("/Game/Maps/Lvl_MainMenu");
	const TCHAR* AccessDoorBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	const TCHAR* AccessDoorBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	const TCHAR* AdminTerminalBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal");
	const TCHAR* AdminTerminalBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal");
	const TCHAR* ReceptionLabel = TEXT("Admin_Terminal_Reception");
	const TCHAR* SecurityTerminalLabel = TEXT("Admin_Terminal_Security");
	const TCHAR* LegacySecurityLabel = TEXT("Terminal_AdminSecurity");
	const TCHAR* SecurityTriggerLabel = TEXT("Admin_RoomTrigger_Security");
	const TCHAR* AccessDoorLabel = TEXT("BP_AdminAccessDoor");
	const FVector ReceptionLocation(1450.f, -150.f, 110.f);
	const FRotator ReceptionRotation(0.f, 0.f, 0.f);
	const FVector ReceptionScale(1.f, 1.f, 1.f);
	const FVector SecurityTerminalLocation(2680.f, -400.f, 110.f);
	const FRotator SecurityTerminalRotation(0.f, 0.f, 0.f);
	const FVector SecurityTerminalScale(1.f, 1.f, 1.f);
	const FVector SecurityTriggerLocation(2680.f, -400.f, 150.f);
	const FVector AccessDoorLocation(800.f, 0.f, 0.f);
	const FVector LegacySecurityLocation(-1950.f, -1400.f, 100.f);
	const float LocationEps = 0.51f;
	const float RotationEps = 0.51f;
	const float ScaleEps = 0.02f;

	const TSet<FString> AllowlistedActions = {
		TEXT("delete_actor"),
		TEXT("set_component_property"),
		TEXT("set_collision"),
		TEXT("set_visibility"),
		TEXT("set_transform"),
		TEXT("compile_blueprint"),
		TEXT("save_asset"),
		TEXT("save_maps"),
		TEXT("move_actor_to_level"),
		TEXT("set_actor_property"),
		TEXT("rerun_construction"),
		TEXT("add_blueprint_variable"),
		TEXT("add_scs_component"),
		TEXT("author_hologram_apply_state"),
		TEXT("author_light_controller_set_zone"),
		TEXT("author_admin_room_trigger_lighting_hook"),
		TEXT("author_access_door_facility_state_listener"),
		TEXT("author_light_controller_facility_state_listener"),
		TEXT("connect_blueprint_pins"),
		TEXT("spawn_blueprint_actor"),
		TEXT("spawn_s20_zone_light"),
		TEXT("set_s20_light_intensity"),
		TEXT("invoke_s20_set_lighting_zone"),
		TEXT("delete_s22_legacy_admin_ambience"),
		TEXT("spawn_s22_admin_audio_zones"),
		TEXT("spawn_neuro_navmesh_bounds"),
		TEXT("spawn_neuro_research_station"),
		TEXT("spawn_admin_research_wing_connector"),
		TEXT("spawn_admin_research_wing_keycard"),
		TEXT("trim_spine_landing_admin"),
		TEXT("set_admin_doorlock_interactable"),
		TEXT("spawn_admin_block2_dressing"),
		TEXT("spawn_admin_block3_resources"),
		TEXT("spawn_admin_block4_security_officer"),
		TEXT("spawn_admin_block4_navmesh_bounds"),
		TEXT("save_admin_block4_navmesh_prerequisite"),
		TEXT("spawn_neuro_arrival_lab_dressing"),
		TEXT("spawn_neuro_ch3_containment_evidence"),
		TEXT("spawn_neuro_ch4_transformed_personnel"),
		TEXT("configure_neuro_power_failure_discovery"),
		TEXT("spawn_neuro_power_diagnostic"),
		TEXT("create_neurogenetics_mission"),
		TEXT("expand_neurogenetics_mission_beat3"),
		TEXT("expand_neurogenetics_mission_beat4"),
		TEXT("expand_neurogenetics_mission_beat5"),
		TEXT("expand_neurogenetics_mission_beat6"),
		TEXT("create_neuro_power_restore_mission"),
		TEXT("set_neurogenetics_next_mission_power_restore"),
		TEXT("configure_neuro_backup_power_restore"),
		TEXT("create_neuro_targeting_why_mission"),
		TEXT("set_neuro_power_restore_next_targeting_why"),
		TEXT("configure_neuro_researcher_targeting_why"),
		TEXT("create_neuro_research_station_intro_mission"),
		TEXT("set_neuro_targeting_why_next_research_station"),
		TEXT("configure_neuro_research_station_intro"),
		TEXT("create_neural_slow_adaptation_asset"),
		TEXT("create_neuro_neural_slow_use_mission"),
		TEXT("set_neuro_research_station_intro_next_neural_slow"),
		TEXT("create_neuro_adaptation_connection_mission"),
		TEXT("set_neuro_neural_slow_use_next_adaptation_connection"),
		TEXT("configure_neuro_live_adaptation_connection"),
		TEXT("create_neuro_revelation_mission"),
		TEXT("set_neuro_adaptation_connection_next_revelation"),
		TEXT("configure_neuro_revelation_observation"),
		TEXT("create_cryo_access_mission"),
		TEXT("set_neuro_revelation_next_cryo_access"),
		TEXT("configure_cryo_backup_power_panel"),
		TEXT("create_cryo_entry_mission"),
		TEXT("set_cryo_access_next_cryo_entry"),
		TEXT("configure_cryo_entry_checkpoint"),
		TEXT("create_cryo_evidence_mission"),
		TEXT("set_cryo_entry_next_cryo_evidence"),
		TEXT("configure_cryo_evidence_datapads"),
		TEXT("create_compute_entry_mission"),
		TEXT("set_cryo_evidence_next_compute_entry"),
		TEXT("configure_compute_entry_checkpoint"),
		TEXT("create_compute_handover_mission"),
		TEXT("set_compute_entry_next_compute_handover"),
		TEXT("configure_compute_handover_terminals_and_datapad"),
		TEXT("create_the_conclusion_mission"),
		TEXT("set_compute_handover_next_the_conclusion"),
		TEXT("configure_reactor_control_spine"),
		TEXT("create_research_station_mission"),
		TEXT("set_the_conclusion_next_research_station"),
		TEXT("configure_research_station"),
		TEXT("create_locomotor_disrupt_adaptation"),
		TEXT("create_optical_disrupt_adaptation"),
		TEXT("create_syringe_kit_mission"),
		TEXT("set_research_station_next_syringe_kit"),
		TEXT("configure_research_station_syringe_kit"),
		TEXT("create_nathan_grant_look"),
		TEXT("spawn_neuro_adaptation_subject"),
		TEXT("spawn_neuro_neural_mapping_array"),
		TEXT("spawn_neuro_research_load_cutoff"),
		TEXT("spawn_neuro_neural_mapping_terminal"),
		TEXT("spawn_neuro_neural_signature_observation_node"),
		TEXT("spawn_neuro_neural_change_evidence_instrument"),
	};

	const TSet<FString> HighRiskActions = {
		TEXT("delete_actor"),
		TEXT("compile_blueprint"),
		TEXT("save_asset"),
		TEXT("save_maps"),
		TEXT("move_actor_to_level"),
		TEXT("connect_blueprint_pins"),
		TEXT("spawn_blueprint_actor"),
		TEXT("add_scs_component"),
		TEXT("author_hologram_apply_state"),
		TEXT("author_light_controller_set_zone"),
		TEXT("author_admin_room_trigger_lighting_hook"),
		TEXT("author_access_door_facility_state_listener"),
		TEXT("author_light_controller_facility_state_listener"),
		TEXT("spawn_s20_zone_light"),
		TEXT("invoke_s20_set_lighting_zone"),
		TEXT("delete_s22_legacy_admin_ambience"),
		TEXT("spawn_s22_admin_audio_zones"),
		TEXT("spawn_neuro_navmesh_bounds"),
		TEXT("spawn_neuro_research_station"),
		TEXT("spawn_admin_research_wing_connector"),
		TEXT("spawn_admin_research_wing_keycard"),
		TEXT("trim_spine_landing_admin"),
		TEXT("set_admin_doorlock_interactable"),
		TEXT("spawn_admin_block2_dressing"),
		TEXT("spawn_admin_block3_resources"),
		TEXT("spawn_admin_block4_security_officer"),
		TEXT("spawn_admin_block4_navmesh_bounds"),
		TEXT("save_admin_block4_navmesh_prerequisite"),
		TEXT("spawn_neuro_arrival_lab_dressing"),
		TEXT("spawn_neuro_ch3_containment_evidence"),
		TEXT("spawn_neuro_ch4_transformed_personnel"),
		TEXT("configure_neuro_power_failure_discovery"),
		TEXT("spawn_neuro_power_diagnostic"),
		TEXT("create_neurogenetics_mission"),
		TEXT("expand_neurogenetics_mission_beat3"),
		TEXT("expand_neurogenetics_mission_beat4"),
		TEXT("expand_neurogenetics_mission_beat5"),
		TEXT("expand_neurogenetics_mission_beat6"),
		TEXT("create_neuro_power_restore_mission"),
		TEXT("set_neurogenetics_next_mission_power_restore"),
		TEXT("configure_neuro_backup_power_restore"),
		TEXT("create_neuro_targeting_why_mission"),
		TEXT("set_neuro_power_restore_next_targeting_why"),
		TEXT("configure_neuro_researcher_targeting_why"),
		TEXT("create_neuro_research_station_intro_mission"),
		TEXT("set_neuro_targeting_why_next_research_station"),
		TEXT("configure_neuro_research_station_intro"),
		TEXT("create_neural_slow_adaptation_asset"),
		TEXT("create_neuro_neural_slow_use_mission"),
		TEXT("set_neuro_research_station_intro_next_neural_slow"),
		TEXT("create_neuro_adaptation_connection_mission"),
		TEXT("set_neuro_neural_slow_use_next_adaptation_connection"),
		TEXT("configure_neuro_live_adaptation_connection"),
		TEXT("create_neuro_revelation_mission"),
		TEXT("set_neuro_adaptation_connection_next_revelation"),
		TEXT("configure_neuro_revelation_observation"),
		TEXT("create_cryo_access_mission"),
		TEXT("set_neuro_revelation_next_cryo_access"),
		TEXT("configure_cryo_backup_power_panel"),
		TEXT("create_cryo_entry_mission"),
		TEXT("set_cryo_access_next_cryo_entry"),
		TEXT("configure_cryo_entry_checkpoint"),
		TEXT("create_cryo_evidence_mission"),
		TEXT("set_cryo_entry_next_cryo_evidence"),
		TEXT("configure_cryo_evidence_datapads"),
		TEXT("create_compute_entry_mission"),
		TEXT("set_cryo_evidence_next_compute_entry"),
		TEXT("configure_compute_entry_checkpoint"),
		TEXT("create_compute_handover_mission"),
		TEXT("set_compute_entry_next_compute_handover"),
		TEXT("configure_compute_handover_terminals_and_datapad"),
		TEXT("create_the_conclusion_mission"),
		TEXT("set_compute_handover_next_the_conclusion"),
		TEXT("configure_reactor_control_spine"),
		TEXT("create_research_station_mission"),
		TEXT("set_the_conclusion_next_research_station"),
		TEXT("configure_research_station"),
		TEXT("create_locomotor_disrupt_adaptation"),
		TEXT("create_optical_disrupt_adaptation"),
		TEXT("create_syringe_kit_mission"),
		TEXT("set_research_station_next_syringe_kit"),
		TEXT("configure_research_station_syringe_kit"),
		TEXT("create_nathan_grant_look"),
		TEXT("spawn_neuro_adaptation_subject"),
		TEXT("spawn_neuro_neural_mapping_array"),
		TEXT("spawn_neuro_research_load_cutoff"),
		TEXT("spawn_neuro_neural_mapping_terminal"),
		TEXT("spawn_neuro_neural_signature_observation_node"),
		TEXT("spawn_neuro_neural_change_evidence_instrument"),
	};

	const TSet<FString> SpawnPropertyAllowlist = {
		TEXT("TerminalID"),
		TEXT("Title"),
		TEXT("TerminalType"),
		TEXT("bInitiallyPowered"),
		TEXT("bOneShot"),
		TEXT("InteractionPrompt"),
		TEXT("InteractionRange"),
	};

	const TSet<FString> AllowlistedProperties = {
		TEXT("CollisionProfileName"),
		TEXT("CollisionEnabled"),
		TEXT("bHiddenInGame"),
		TEXT("bVisible"),
		TEXT("RelativeLocation"),
		TEXT("RelativeRotation"),
		TEXT("RelativeScale3D"),
		TEXT("BoxExtent"),
	};

	FCriticalSection GLedgerMutex;

	AActor* FindUniqueLabel(UWorld* World, const FString& Label);
	UPrimitiveComponent* FindPrimitive(AActor* Actor, const FString& ComponentName);
	UPackage* FindPackageByName(const FString& PackageName);
	ULevel* FindLoadedLevelByPackage(UWorld* World, const FString& PackageName);
	FProperty* FindInstanceProperty(UObject* Object, const FString& PropertyName);
	FString ReadNameOrTextValue(AActor* Actor, FProperty* Property);

	struct FBridgeApproval
	{
		FString Identity;
		FString Timestamp;
		bool bApproved = false;
	};

	struct FBridgeChange
	{
		FString ChangeId;
		FString Action;
		FString Description;
		FString Package;
		FString Risk;
		FString Status;
		FString CreatedAt;
		FString ExecutedAt;
		bool bExecuted = false;
		bool bSavePerformed = false;
		TSharedPtr<FJsonObject> Targets;
		TSharedPtr<FJsonObject> Before;
		TSharedPtr<FJsonObject> Proposed;
		TSharedPtr<FJsonObject> After;
		TSharedPtr<FJsonObject> Args;
		FBridgeApproval User;
		FBridgeApproval SecondReview;
	};

	TMap<FString, TSharedRef<FBridgeChange>> GChanges;

	FString NowIso()
	{
		return FDateTime::UtcNow().ToIso8601();
	}

	FString NewChangeId()
	{
		return FString::Printf(TEXT("chg_%s"), *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower());
	}

	TSharedPtr<FJsonObject> CloneJson(const TSharedPtr<FJsonObject>& Object)
	{
		if (!Object.IsValid())
		{
			return MakeShared<FJsonObject>();
		}
		return ParseObject(ToString(Object.ToSharedRef()));
	}

	FString RiskFor(const FString& Action)
	{
		if (HighRiskActions.Contains(Action))
		{
			return TEXT("high");
		}
		if (Action == TEXT("set_visibility"))
		{
			return TEXT("low");
		}
		return TEXT("medium");
	}

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	UWorld* GetPieWorld()
	{
		if (!GEditor)
		{
			return nullptr;
		}
		if (FWorldContext* Context = GEditor->GetPIEWorldContext())
		{
			return Context->World();
		}
		return nullptr;
	}

	FString WorldPackageName(UWorld* World)
	{
		if (!World)
		{
			return TEXT("");
		}
		if (UPackage* Package = World->GetOutermost())
		{
			return Package->GetName();
		}
		return World->GetMapName();
	}

	FString ActorLabel(AActor* Actor)
	{
		return Actor ? Actor->GetActorNameOrLabel() : TEXT("");
	}

	FString ClassName(const UObject* Object)
	{
		return Object && Object->GetClass() ? Object->GetClass()->GetName() : TEXT("");
	}

	FString ActorOwningPackage(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("");
		}
		if (ULevel* Level = Actor->GetLevel())
		{
			if (UPackage* Package = Level->GetOutermost())
			{
				return Package->GetName();
			}
		}
		if (UPackage* Package = Actor->GetOutermost())
		{
			return Package->GetName();
		}
		return TEXT("");
	}

	FString NormalizePackage(const FString& Path)
	{
		FString Clean = Path;
		Clean.ReplaceInline(TEXT("\\"), TEXT("/"));
		int32 DotIndex = INDEX_NONE;
		if (Clean.FindChar(TEXT('.'), DotIndex))
		{
			Clean.LeftInline(DotIndex, EAllowShrinking::No);
		}
		if (Clean.EndsWith(TEXT("/SL_Epitope_Admin")) || Clean == TEXT("SL_Epitope_Admin"))
		{
			return AdminPackage;
		}
		return Clean;
	}

	bool PackagesEqual(const FString& A, const FString& B)
	{
		return NormalizePackage(A) == NormalizePackage(B);
	}

	bool LocationMatches(const FVector& Actual, const FVector& Expected)
	{
		return FMath::Abs(Actual.X - Expected.X) <= LocationEps
			&& FMath::Abs(Actual.Y - Expected.Y) <= LocationEps
			&& FMath::Abs(Actual.Z - Expected.Z) <= LocationEps;
	}

	bool VecMatches(const FVector& Actual, const FVector& Expected, float Eps)
	{
		return FMath::Abs(Actual.X - Expected.X) <= Eps
			&& FMath::Abs(Actual.Y - Expected.Y) <= Eps
			&& FMath::Abs(Actual.Z - Expected.Z) <= Eps;
	}

	bool RotationMatches(const FRotator& Actual, const FRotator& Expected)
	{
		return FMath::Abs(Actual.Pitch - Expected.Pitch) <= RotationEps
			&& FMath::Abs(Actual.Yaw - Expected.Yaw) <= RotationEps
			&& FMath::Abs(Actual.Roll - Expected.Roll) <= RotationEps;
	}

	bool ScaleMatches(const FVector& Actual, const FVector& Expected)
	{
		return VecMatches(Actual, Expected, ScaleEps);
	}

	bool TransformMatches(AActor* Actor, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
		if (!Actor)
		{
			return false;
		}
		return LocationMatches(Actor->GetActorLocation(), Location)
			&& RotationMatches(Actor->GetActorRotation(), Rotation)
			&& ScaleMatches(Actor->GetActorScale3D(), Scale);
	}

	FString TransformMismatch(AActor* Actor, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
		if (!Actor)
		{
			return TEXT("Actor is null.");
		}
		const FVector Loc = Actor->GetActorLocation();
		const FRotator Rotator = Actor->GetActorRotation();
		const FVector Scl = Actor->GetActorScale3D();
		if (!LocationMatches(Loc, Location))
		{
			return FString::Printf(
				TEXT("location (%.2f, %.2f, %.2f) does not match (%.2f, %.2f, %.2f)"),
				Loc.X, Loc.Y, Loc.Z, Location.X, Location.Y, Location.Z);
		}
		if (!RotationMatches(Rotator, Rotation))
		{
			return FString::Printf(
				TEXT("rotation (%.2f, %.2f, %.2f) does not match (%.2f, %.2f, %.2f)"),
				Rotator.Pitch, Rotator.Yaw, Rotator.Roll, Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
		}
		if (!ScaleMatches(Scl, Scale))
		{
			return FString::Printf(
				TEXT("scale (%.2f, %.2f, %.2f) does not match (%.2f, %.2f, %.2f)"),
				Scl.X, Scl.Y, Scl.Z, Scale.X, Scale.Y, Scale.Z);
		}
		return TEXT("");
	}

	bool IsMapPackageName(const FString& PackageName)
	{
		const FString Normalized = NormalizePackage(PackageName);
		return PackagesEqual(Normalized, AdminPackage)
			|| PackagesEqual(Normalized, EpitopePackage)
			|| Normalized.Contains(TEXT("/Maps/"));
	}

	bool IsEpitopeWorldPackage(const FString& PackageName)
	{
		return PackagesEqual(PackageName, EpitopePackage);
	}

	ULevelStreaming* FindStreamingLevel(UWorld* World, const FString& PackageName)
	{
		if (!World)
		{
			return nullptr;
		}
		for (ULevelStreaming* Streaming : World->GetStreamingLevels())
		{
			if (Streaming && PackagesEqual(Streaming->GetWorldAssetPackageFName().ToString(), PackageName))
			{
				return Streaming;
			}
		}
		return nullptr;
	}

	FString MeshPath(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("");
		}
		if (UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>())
		{
			if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
			{
				return Mesh->GetPathName();
			}
		}
		return TEXT("");
	}

	bool IsEngineCubeMesh(const FString& Path)
	{
		return Path.Contains(TEXT("/Engine/BasicShapes/Cube"));
	}

	FString AttachParentLabel(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("");
		}
		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root || !Root->GetAttachParent())
		{
			return TEXT("");
		}
		AActor* Owner = Root->GetAttachParent()->GetOwner();
		if (!Owner || Owner == Actor)
		{
			return TEXT("");
		}
		return ActorLabel(Owner);
	}

	TArray<FString> ChildLabels(AActor* Actor)
	{
		TArray<FString> Out;
		if (!Actor)
		{
			return Out;
		}
		TArray<AActor*> Attached;
		Actor->GetAttachedActors(Attached);
		for (AActor* Child : Attached)
		{
			if (Child)
			{
				Out.Add(ActorLabel(Child));
			}
		}
		return Out;
	}

	TArray<AActor*> FindByExactLabel(UWorld* World, const FString& Label)
	{
		TArray<AActor*> Matches;
		if (!World || Label.IsEmpty())
		{
			return Matches;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && ActorLabel(Actor).Equals(Label, ESearchCase::CaseSensitive))
			{
				Matches.Add(Actor);
			}
		}
		return Matches;
	}

	TSharedRef<FJsonObject> ActorSnapshot(AActor* Actor)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Actor)
		{
			return Out;
		}
		Out->SetStringField(TEXT("label"), ActorLabel(Actor));
		Out->SetStringField(TEXT("name"), Actor->GetName());
		Out->SetStringField(TEXT("class"), ClassName(Actor));
		Out->SetStringField(TEXT("path"), Actor->GetPathName());
		Out->SetStringField(TEXT("owning_package"), ActorOwningPackage(Actor));
		Out->SetArrayField(TEXT("location"), Vec(Actor->GetActorLocation()));
		Out->SetArrayField(TEXT("rotation"), Rot(Actor->GetActorRotation()));
		Out->SetArrayField(TEXT("scale"), Vec(Actor->GetActorScale3D()));
		Out->SetStringField(TEXT("mesh"), MeshPath(Actor));
		const FString Parent = AttachParentLabel(Actor);
		if (Parent.IsEmpty())
		{
			Out->SetField(TEXT("attach_parent"), MakeShared<FJsonValueNull>());
		}
		else
		{
			Out->SetStringField(TEXT("attach_parent"), Parent);
		}
		TArray<TSharedPtr<FJsonValue>> Children;
		for (const FString& Child : ChildLabels(Actor))
		{
			Children.Add(MakeShared<FJsonValueString>(Child));
		}
		Out->SetArrayField(TEXT("children"), Children);
		return Out;
	}

	TSharedRef<FJsonObject> AuditBase(const FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetStringField(TEXT("change_id"), Change.ChangeId);
		Out->SetStringField(TEXT("action"), Change.Action);
		Out->SetStringField(TEXT("description"), Change.Description);
		Out->SetStringField(TEXT("package"), Change.Package);
		Out->SetStringField(TEXT("risk"), Change.Risk);
		Out->SetStringField(TEXT("status"), Change.Status);
		Out->SetStringField(TEXT("timestamp"), NowIso());
		Out->SetBoolField(TEXT("save_performed"), Change.bSavePerformed);
		if (Change.Targets.IsValid())
		{
			Out->SetObjectField(TEXT("target"), Change.Targets.ToSharedRef());
		}
		if (Change.Before.IsValid())
		{
			Out->SetObjectField(TEXT("before_state"), Change.Before.ToSharedRef());
		}
		if (Change.Proposed.IsValid())
		{
			Out->SetObjectField(TEXT("proposed_new_state"), Change.Proposed.ToSharedRef());
		}
		if (Change.After.IsValid())
		{
			Out->SetObjectField(TEXT("after_state"), Change.After.ToSharedRef());
		}
		else
		{
			Out->SetField(TEXT("after_state"), MakeShared<FJsonValueNull>());
		}
		TSharedRef<FJsonObject> Approvals = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> User = MakeShared<FJsonObject>();
		User->SetBoolField(TEXT("approved"), Change.User.bApproved);
		User->SetStringField(TEXT("identity"), Change.User.Identity);
		User->SetStringField(TEXT("timestamp"), Change.User.Timestamp);
		TSharedRef<FJsonObject> Review = MakeShared<FJsonObject>();
		Review->SetBoolField(TEXT("approved"), Change.SecondReview.bApproved);
		Review->SetStringField(TEXT("identity"), Change.SecondReview.Identity);
		Review->SetStringField(TEXT("timestamp"), Change.SecondReview.Timestamp);
		Approvals->SetObjectField(TEXT("user"), User);
		Approvals->SetObjectField(TEXT("second_review"), Review);
		Approvals->SetBoolField(TEXT("dual_approved"), Change.User.bApproved && Change.SecondReview.bApproved);
		Out->SetObjectField(TEXT("approvals"), Approvals);
		return Out;
	}

	void LogAudit(const FString& Event, const FBridgeChange& Change)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[OrganoidAIBridge][%s] change_id=%s action=%s package=%s status=%s user=%s/%s review=%s/%s save=%s"),
			*Event,
			*Change.ChangeId,
			*Change.Action,
			*Change.Package,
			*Change.Status,
			Change.User.bApproved ? TEXT("approved") : TEXT("pending"),
			*Change.User.Identity,
			Change.SecondReview.bApproved ? TEXT("approved") : TEXT("pending"),
			*Change.SecondReview.Identity,
			Change.bSavePerformed ? TEXT("yes") : TEXT("no"));
	}

	TSharedRef<FJsonObject> FailAudit(const FString& Code, const FString& Message, const TSharedPtr<FBridgeChange>& Change)
	{
		TSharedRef<FJsonObject> Root = Fail(Code, Message);
		if (Change.IsValid())
		{
			Root->SetObjectField(TEXT("audit"), AuditBase(*Change));
		}
		return Root;
	}

	bool GetTargetLocation(const TSharedPtr<FJsonObject>& Target, FVector& Out)
	{
		return GetVector(Target, TEXT("location"), Out);
	}

	FString PreflightDeleteActors(
		const TSharedPtr<FJsonObject>& Args,
		TArray<AActor*>& OutActors,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		OutActors.Reset();
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}

		const FString RequiredPackage = NormalizePackage(GetString(Args, TEXT("required_package"), AdminPackage));
		const TArray<TSharedPtr<FJsonValue>>* TargetArr = nullptr;
		if (!Args.IsValid() || !Args->TryGetArrayField(TEXT("targets"), TargetArr) || !TargetArr)
		{
			return TEXT("targets array is required.");
		}
		const int32 ExactCount = GetInt(Args, TEXT("require_exact_count"), TargetArr->Num());
		if (TargetArr->Num() != ExactCount)
		{
			return FString::Printf(TEXT("target count=%d expected=%d"), TargetArr->Num(), ExactCount);
		}

		TArray<TSharedPtr<FJsonValue>> BeforeActors;
		TArray<TSharedPtr<FJsonValue>> ProposedActors;
		TSet<FString> SeenLabels;
		for (const TSharedPtr<FJsonValue>& Entry : *TargetArr)
		{
			const TSharedPtr<FJsonObject> Target = Entry.IsValid() ? Entry->AsObject() : nullptr;
			if (!Target.IsValid())
			{
				return TEXT("Each target must be an object with label and location.");
			}
			const FString Label = GetString(Target, TEXT("label"));
			FVector Expected = FVector::ZeroVector;
			if (Label.IsEmpty() || !GetTargetLocation(Target, Expected))
			{
				return TEXT("Each target requires exact label and location [x,y,z].");
			}
			if (SeenLabels.Contains(Label))
			{
				return FString::Printf(TEXT("Duplicate target label %s"), *Label);
			}
			SeenLabels.Add(Label);

			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1"), *Label, Matches.Num());
			}
			AActor* Actor = Matches[0];
			const FVector Loc = Actor->GetActorLocation();
			if (!LocationMatches(Loc, Expected))
			{
				return FString::Printf(
					TEXT("%s location (%.2f, %.2f, %.2f) does not match (%.2f, %.2f, %.2f)"),
					*Label, Loc.X, Loc.Y, Loc.Z, Expected.X, Expected.Y, Expected.Z);
			}
			const FString Owner = NormalizePackage(ActorOwningPackage(Actor));
			if (!PackagesEqual(Owner, RequiredPackage))
			{
				return FString::Printf(TEXT("%s owning package '%s' does not match '%s'"), *Label, *Owner, *RequiredPackage);
			}
			if (GetBool(Args, TEXT("require_engine_cube_mesh"), false) && !IsEngineCubeMesh(MeshPath(Actor)))
			{
				return FString::Printf(TEXT("%s mesh '%s' is not Engine BasicShapes Cube"), *Label, *MeshPath(Actor));
			}
			if (GetBool(Args, TEXT("require_no_attach"), true))
			{
				const FString Parent = AttachParentLabel(Actor);
				if (!Parent.IsEmpty())
				{
					return FString::Printf(TEXT("%s is attached to %s"), *Label, *Parent);
				}
				const TArray<FString> Kids = ChildLabels(Actor);
				if (Kids.Num() > 0)
				{
					return FString::Printf(TEXT("%s has attached children"), *Label);
				}
			}
			const FString RequiredClass = GetString(Args, TEXT("require_class"));
			if (!RequiredClass.IsEmpty() && ClassName(Actor) != RequiredClass)
			{
				return FString::Printf(TEXT("%s class=%s expected=%s"), *Label, *ClassName(Actor), *RequiredClass);
			}

			OutActors.Add(Actor);
			BeforeActors.Add(MakeShared<FJsonValueObject>(ActorSnapshot(Actor)));
			TSharedRef<FJsonObject> Gone = MakeShared<FJsonObject>();
			Gone->SetStringField(TEXT("label"), Label);
			Gone->SetStringField(TEXT("state"), TEXT("deleted"));
			ProposedActors.Add(MakeShared<FJsonValueObject>(Gone));
		}

		Before->SetArrayField(TEXT("actors"), BeforeActors);
		Before->SetStringField(TEXT("owning_package"), RequiredPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetArrayField(TEXT("actors"), ProposedActors);
		Proposed->SetStringField(TEXT("owning_package"), RequiredPackage);
		Proposed->SetStringField(TEXT("result"), TEXT("actors removed; package not saved by this change"));
		return TEXT("");
	}

	FString CollisionEnabledLabel(ECollisionEnabled::Type Value)
	{
		switch (Value)
		{
		case ECollisionEnabled::NoCollision: return TEXT("NoCollision");
		case ECollisionEnabled::QueryOnly: return TEXT("QueryOnly");
		case ECollisionEnabled::PhysicsOnly: return TEXT("PhysicsOnly");
		case ECollisionEnabled::QueryAndPhysics: return TEXT("QueryAndPhysics");
		default: return TEXT("Unknown");
		}
	}

	TSharedRef<FJsonObject> BoxComponentSnapshot(UBoxComponent* Box)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Box)
		{
			return Out;
		}
		Out->SetStringField(TEXT("component"), Box->GetName());
		Out->SetStringField(TEXT("class"), ClassName(Box));
		Out->SetArrayField(TEXT("relative_location"), Vec(Box->GetRelativeLocation()));
		Out->SetArrayField(TEXT("box_extent"), Vec(Box->GetUnscaledBoxExtent()));
		Out->SetArrayField(TEXT("component_scale"), Vec(Box->GetRelativeScale3D()));
		Out->SetStringField(TEXT("collision_enabled"), CollisionEnabledLabel(Box->GetCollisionEnabled()));
		Out->SetStringField(TEXT("collision_profile"), Box->GetCollisionProfileName().ToString());
		const FBoxSphereBounds Bounds = Box->Bounds;
		Out->SetArrayField(TEXT("bounds_origin"), Vec(Bounds.Origin));
		Out->SetArrayField(TEXT("bounds_extent"), Vec(Bounds.BoxExtent));
		return Out;
	}

	TSharedRef<FJsonObject> SnapshotDoorComponents(AActor* Actor)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		if (UBoxComponent* Box = Cast<UBoxComponent>(FindPrimitive(Actor, TEXT("AccessTrigger"))))
		{
			Out->SetObjectField(TEXT("AccessTrigger"), BoxComponentSnapshot(Box));
		}
		const TCHAR* Names[] = { TEXT("Door_Left"), TEXT("Door_Right"), TEXT("DoorFrame") };
		for (const TCHAR* Name : Names)
		{
			UPrimitiveComponent* Primitive = FindPrimitive(Actor, Name);
			if (!Primitive)
			{
				continue;
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("component"), Primitive->GetName());
			Row->SetArrayField(TEXT("relative_location"), Vec(Primitive->GetRelativeLocation()));
			Row->SetArrayField(TEXT("relative_rotation"), Rot(Primitive->GetRelativeRotation()));
			Row->SetArrayField(TEXT("relative_scale"), Vec(Primitive->GetRelativeScale3D()));
			Row->SetStringField(TEXT("collision_profile"), Primitive->GetCollisionProfileName().ToString());
			Row->SetStringField(TEXT("attach_parent"), Primitive->GetAttachParent() ? Primitive->GetAttachParent()->GetName() : TEXT(""));
			Out->SetObjectField(Name, Row);
		}
		return Out;
	}

	FString PreflightRerunConstruction(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. This change does not save.");
		}
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		const FString ActorQuery = GetString(Args, TEXT("actor"), GetString(Args, TEXT("name")));
		AActor* Actor = FindUniqueLabel(World, ActorQuery);
		if (!Actor)
		{
			return TEXT("Actor not found or not unique.");
		}
		if (!Actor->GetName().Equals(TEXT("BP_AdminAccessDoor_C_0"), ESearchCase::IgnoreCase))
		{
			return TEXT("Rerun is limited to placed instance BP_AdminAccessDoor_C_0.");
		}
		if (!ClassName(Actor).Equals(TEXT("BP_AdminAccessDoor_C"), ESearchCase::IgnoreCase))
		{
			return TEXT("Actor class is not BP_AdminAccessDoor_C.");
		}
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), AdminPackage));
		if (!PackagesEqual(ActorOwningPackage(Actor), RequiredPackage))
		{
			return TEXT("Actor owning package does not match required_package.");
		}
		FVector ExpectedLoc;
		if (GetVector(Args, TEXT("location"), ExpectedLoc) && !LocationMatches(Actor->GetActorLocation(), ExpectedLoc))
		{
			return TEXT("Actor location does not match the required placed door.");
		}

		Before->SetStringField(TEXT("owning_package"), ActorOwningPackage(Actor));
		Before->SetStringField(TEXT("scope"), TEXT("placed_instance_construction"));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("instance"), SnapshotDoorComponents(Actor));

		Proposed->SetStringField(TEXT("owning_package"), ActorOwningPackage(Actor));
		Proposed->SetStringField(TEXT("actor"), ActorLabel(Actor));
		Proposed->SetStringField(TEXT("instance_name"), Actor->GetName());
		Proposed->SetStringField(TEXT("action"), TEXT("RerunConstructionScripts"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Rerun Construction Script on BP_AdminAccessDoor_C_0 only. No geometry override. Package not saved."));
		return TEXT("");
	}

	FString PreflightAccessTriggerGeometry(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. This change does not save.");
		}

		const FString ComponentName = GetString(Args, TEXT("component"));
		if (!ComponentName.Equals(TEXT("AccessTrigger"), ESearchCase::IgnoreCase))
		{
			return TEXT("Box geometry writes are limited to BP_AdminAccessDoor.AccessTrigger.");
		}
		if (ComponentName.Equals(TEXT("Door_Left"), ESearchCase::IgnoreCase)
			|| ComponentName.Equals(TEXT("Door_Right"), ESearchCase::IgnoreCase)
			|| ComponentName.Equals(TEXT("DoorFrame"), ESearchCase::IgnoreCase))
		{
			return TEXT("Door_Left / Door_Right / DoorFrame are forbidden targets.");
		}

		FVector NewLocation;
		FVector NewExtent;
		if (!GetVector(Args, TEXT("relative_location"), NewLocation))
		{
			return TEXT("relative_location [x,y,z] is required.");
		}
		if (!GetVector(Args, TEXT("box_extent"), NewExtent))
		{
			return TEXT("box_extent [x,y,z] is required.");
		}
		if (NewExtent.X < 1.0f || NewExtent.Y < 1.0f || NewExtent.Z < 1.0f)
		{
			return TEXT("box_extent must be positive.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		const FString ActorQuery = GetString(Args, TEXT("actor"), GetString(Args, TEXT("name")));
		AActor* Actor = FindUniqueLabel(World, ActorQuery);
		if (!Actor)
		{
			return TEXT("Actor not found or not unique.");
		}
		const FString RequiredPackage = NormalizePackage(GetString(Args, TEXT("required_package"), AdminPackage));
		if (!PackagesEqual(ActorOwningPackage(Actor), RequiredPackage))
		{
			return FString::Printf(
				TEXT("Actor owning package '%s' does not match '%s'"),
				*ActorOwningPackage(Actor),
				*RequiredPackage);
		}
		const FString RequiredClass = GetString(Args, TEXT("require_class"));
		if (!RequiredClass.IsEmpty() && ClassName(Actor) != RequiredClass)
		{
			return FString::Printf(TEXT("class=%s expected=%s"), *ClassName(Actor), *RequiredClass);
		}
		if (!ClassName(Actor).Contains(TEXT("BP_AdminAccessDoor")))
		{
			return TEXT("Actor is not BP_AdminAccessDoor.");
		}

		UPrimitiveComponent* Primitive = FindPrimitive(Actor, ComponentName);
		UBoxComponent* Box = Cast<UBoxComponent>(Primitive);
		if (!Box)
		{
			return TEXT("AccessTrigger box component not found.");
		}
		if (Box->GetCollisionEnabled() != ECollisionEnabled::QueryOnly)
		{
			return TEXT("AccessTrigger collision is not QueryOnly. Refusing to resize a blocking volume.");
		}

		Before->SetStringField(TEXT("owning_package"), RequiredPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		Before->SetObjectField(TEXT("component"), BoxComponentSnapshot(Box));
		Before->SetStringField(TEXT("collision_note"), TEXT("QueryOnly / OverlapAllDynamic / Pawn Overlap must be preserved. Not mutated."));

		Proposed->SetStringField(TEXT("owning_package"), RequiredPackage);
		Proposed->SetStringField(TEXT("actor"), ActorLabel(Actor));
		Proposed->SetStringField(TEXT("component"), TEXT("AccessTrigger"));
		Proposed->SetArrayField(TEXT("relative_location"), Vec(NewLocation));
		Proposed->SetArrayField(TEXT("box_extent"), Vec(NewExtent));
		Proposed->SetStringField(TEXT("collision_enabled"), TEXT("QueryOnly (unchanged)"));
		Proposed->SetStringField(TEXT("collision_profile"), TEXT("OverlapAllDynamic (unchanged)"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("AccessTrigger geometry only; package not saved by this change"));
		return TEXT("");
	}

	UBlueprint* LoadAccessDoorBlueprint()
	{
		return LoadObject<UBlueprint>(nullptr, AccessDoorBpPath);
	}

	UBoxComponent* FindAccessTriggerTemplate(UBlueprint* Blueprint)
	{
		if (!Blueprint || !Blueprint->SimpleConstructionScript)
		{
			return nullptr;
		}
		UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (!Node || !Node->GetVariableName().ToString().Equals(TEXT("AccessTrigger"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (UBoxComponent* FromGenerated = Cast<UBoxComponent>(Node->GetActualComponentTemplate(BPGC)))
			{
				return FromGenerated;
			}
			if (UBoxComponent* FromNode = Cast<UBoxComponent>(Node->ComponentTemplate))
			{
				return FromNode;
			}
		}
		return nullptr;
	}

	FString PreflightAccessTriggerBlueprintTemplate(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. This change does not save.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Compile only with the editor Compile button after execute if required.");
		}
		if (!GetString(Args, TEXT("scope")).Equals(TEXT("blueprint_template"), ESearchCase::IgnoreCase))
		{
			return TEXT("scope must be blueprint_template.");
		}
		const FString ComponentName = GetString(Args, TEXT("component"));
		if (!ComponentName.Equals(TEXT("AccessTrigger"), ESearchCase::IgnoreCase))
		{
			return TEXT("Blueprint template writes are limited to AccessTrigger.");
		}
		if (ComponentName.Equals(TEXT("Door_Left"), ESearchCase::IgnoreCase)
			|| ComponentName.Equals(TEXT("Door_Right"), ESearchCase::IgnoreCase)
			|| ComponentName.Equals(TEXT("DoorFrame"), ESearchCase::IgnoreCase))
		{
			return TEXT("Door_Left / Door_Right / DoorFrame / Timeline are forbidden targets.");
		}

		FVector NewLocation;
		FVector NewExtent;
		if (!GetVector(Args, TEXT("relative_location"), NewLocation))
		{
			return TEXT("relative_location [x,y,z] is required.");
		}
		if (!GetVector(Args, TEXT("box_extent"), NewExtent))
		{
			return TEXT("box_extent [x,y,z] is required.");
		}
		if (NewExtent.X < 1.0f || NewExtent.Y < 1.0f || NewExtent.Z < 1.0f)
		{
			return TEXT("box_extent must be positive.");
		}

		UBlueprint* Blueprint = LoadAccessDoorBlueprint();
		if (!Blueprint)
		{
			return TEXT("BP_AdminAccessDoor not found.");
		}
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), AccessDoorBpPackage));
		if (!PackagesEqual(Blueprint->GetOutermost()->GetName(), RequiredPackage)
			&& !Blueprint->GetPathName().StartsWith(RequiredPackage))
		{
			return TEXT("Blueprint package does not match required_package.");
		}
		UBoxComponent* Box = FindAccessTriggerTemplate(Blueprint);
		if (!Box)
		{
			return TEXT("AccessTrigger SCS template not found.");
		}
		if (Box->GetCollisionEnabled() != ECollisionEnabled::QueryOnly)
		{
			return TEXT("AccessTrigger template collision is not QueryOnly. Refusing.");
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("scope"), TEXT("blueprint_template"));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("component"), BoxComponentSnapshot(Box));
		Before->SetStringField(TEXT("collision_note"), TEXT("QueryOnly / OverlapAllDynamic / Pawn Overlap must be preserved. Not mutated."));

		Proposed->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Proposed->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Proposed->SetStringField(TEXT("scope"), TEXT("blueprint_template"));
		Proposed->SetStringField(TEXT("component"), TEXT("AccessTrigger"));
		Proposed->SetStringField(TEXT("template"), TEXT("AccessTrigger_GEN_VARIABLE"));
		Proposed->SetArrayField(TEXT("relative_location"), Vec(NewLocation));
		Proposed->SetArrayField(TEXT("box_extent"), Vec(NewExtent));
		Proposed->SetStringField(TEXT("collision_enabled"), TEXT("QueryOnly (unchanged)"));
		Proposed->SetStringField(TEXT("collision_profile"), TEXT("OverlapAllDynamic (unchanged)"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("SCS AccessTrigger template only; no instance override; package not saved; compile not performed"));
		return TEXT("");
	}

	UBlueprint* LoadBlueprintAsset(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return nullptr;
		}
		if (UBlueprint* Direct = LoadObject<UBlueprint>(nullptr, *Path))
		{
			return Direct;
		}
		return LoadObject<UBlueprint>(nullptr, *NormalizePackage(Path));
	}

	UUserDefinedEnum* LoadEnumAsset(const FString& Path)
	{
		if (Path.IsEmpty())
		{
			return nullptr;
		}
		if (UUserDefinedEnum* Direct = LoadObject<UUserDefinedEnum>(nullptr, *Path))
		{
			return Direct;
		}
		return LoadObject<UUserDefinedEnum>(nullptr, *NormalizePackage(Path));
	}

	TSharedRef<FJsonObject> VariableListSnapshot(UBlueprint* Blueprint)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Names;
		if (Blueprint)
		{
			for (const FBPVariableDescription& Var : Blueprint->NewVariables)
			{
				TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("name"), Var.VarName.ToString());
				Entry->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
				if (UObject* Sub = Var.VarType.PinSubCategoryObject.Get())
				{
					Entry->SetStringField(TEXT("sub_type"), Sub->GetPathName());
				}
				Entry->SetBoolField(TEXT("instance_editable"), !(Var.PropertyFlags & CPF_DisableEditOnInstance));
				Entry->SetStringField(TEXT("category"), Var.Category.ToString());
				Entry->SetStringField(TEXT("default_value"), Var.DefaultValue);
				Names.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		Out->SetArrayField(TEXT("variables"), Names);
		return Out;
	}

	FString PreflightAddHologramBoolVariables(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAddHologramBoolVariables(FBridgeChange& Change);
	FString PreflightAddScsComponent(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAddScsComponent(FBridgeChange& Change);
	FString PreflightAuthorHologramApplyState(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAuthorHologramApplyState(FBridgeChange& Change);
	FString PreflightSpawnHologramActor(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	bool IsHologramSpawnArgs(const TSharedPtr<FJsonObject>& Args);
	FString PreflightAddLightControllerVariables(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAddLightControllerVariables(FBridgeChange& Change);
	FString PreflightAuthorLightControllerSetZone(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAuthorLightControllerSetZone(FBridgeChange& Change);
	FString PreflightSpawnLightControllerActor(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	bool IsLightControllerSpawnArgs(const TSharedPtr<FJsonObject>& Args);
	FString PreflightSpawnS20ZoneLight(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteSpawnS20ZoneLight(FBridgeChange& Change);
	FString PreflightSetS20LightIntensity(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteSetS20LightIntensity(FBridgeChange& Change);
	FString PreflightAuthorAdminRoomTriggerLightingHook(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteAuthorAdminRoomTriggerLightingHook(FBridgeChange& Change);
	FString PreflightInvokeS20SetLightingZone(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteInvokeS20SetLightingZone(FBridgeChange& Change);
	FString PreflightDeleteS22LegacyAdminAmbience(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteDeleteS22LegacyAdminAmbience(FBridgeChange& Change);
	FString PreflightSpawnS22AdminAudioZones(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed);
	TSharedRef<FJsonObject> ExecuteSpawnS22AdminAudioZones(FBridgeChange& Change);

	FString PreflightAddBlueprintVariable(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. This change does not save.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Full compile is not part of add_blueprint_variable.");
		}

		const FString Path = GetString(Args, TEXT("path"));
		const FString VarName = GetString(Args, TEXT("variable_name"));
		const FString EnumPath = GetString(Args, TEXT("enum_path"), GetString(Args, TEXT("type")));
		const FString Category = GetString(Args, TEXT("category"));
		const FString PinCategory = GetString(Args, TEXT("pin_category"), TEXT("byte")).ToLower();
		const FString Spec = GetString(Args, TEXT("spec"));
		if (PinCategory == TEXT("bool") || PinCategory == TEXT("boolean") || Spec == TEXT("s19_facility_hologram_bools_v1"))
		{
			return PreflightAddHologramBoolVariables(Args, Before, Proposed);
		}
		if (Spec == TEXT("s20_admin_lighting_v1"))
		{
			return PreflightAddLightControllerVariables(Args, Before, Proposed);
		}
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"))));

		if (Path.IsEmpty())
		{
			return TEXT("path is required.");
		}
		if (VarName.IsEmpty())
		{
			return TEXT("variable_name is required.");
		}
		if (EnumPath.IsEmpty())
		{
			return TEXT("enum_path is required.");
		}
		if (Category.IsEmpty())
		{
			return TEXT("category is required.");
		}
		if (RequiredPackage.IsEmpty())
		{
			return TEXT("required_package is required.");
		}
		if (PinCategory != TEXT("byte") && PinCategory != TEXT("enum"))
		{
			return TEXT("add_blueprint_variable currently supports enum-typed variables only (pin_category=byte or enum).");
		}
		if (NormalizePackage(Path).Equals(NormalizePackage(AccessDoorBpPath), ESearchCase::IgnoreCase)
			|| Path.Contains(TEXT("BP_AdminAccessDoor")))
		{
			return TEXT("add_blueprint_variable cannot target BP_AdminAccessDoor.");
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(Path);
		if (!Blueprint)
		{
			return TEXT("Blueprint not found.");
		}
		if (!PackagesEqual(Blueprint->GetOutermost()->GetName(), RequiredPackage)
			&& !Blueprint->GetPathName().StartsWith(RequiredPackage))
		{
			return TEXT("Blueprint package does not match required_package.");
		}

		UUserDefinedEnum* EnumAsset = LoadEnumAsset(EnumPath);
		if (!EnumAsset)
		{
			return TEXT("UserDefinedEnum not found.");
		}
		if (!UEdGraphSchema_K2::IsAllowableBlueprintVariableType(EnumAsset))
		{
			return TEXT("Enum is not an allowable Blueprint variable type.");
		}

		const FName VarFName(*VarName);
		if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarFName) != INDEX_NONE)
		{
			return FString::Printf(TEXT("Variable '%s' already exists."), *VarName);
		}
		TSet<FName> CurrentVars;
		FBlueprintEditorUtils::GetClassVariableList(Blueprint, CurrentVars, true);
		if (CurrentVars.Contains(VarFName))
		{
			return FString::Printf(TEXT("Variable '%s' already exists on this Blueprint or a parent class."), *VarName);
		}

		const bool bInstanceEditable = GetBool(Args, TEXT("instance_editable"), true);

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));

		Proposed->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Proposed->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Proposed->SetStringField(TEXT("variable_name"), VarName);
		Proposed->SetStringField(TEXT("pin_category"), TEXT("byte"));
		Proposed->SetStringField(TEXT("enum_path"), EnumAsset->GetPathName());
		Proposed->SetBoolField(TEXT("instance_editable"), bInstanceEditable);
		Proposed->SetStringField(TEXT("category"), Category);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Add enum member variable only; skeleton regen may occur; package not saved; full compile not performed"));
		return TEXT("");
	}

	FString DoorDelta(AActor* Door, const TSharedPtr<FJsonObject>& BeforeDoor)
	{
		if (!Door)
		{
			return TEXT("Access Door not found.");
		}
		if (!BeforeDoor.IsValid())
		{
			return TEXT("Access Door before-snapshot missing.");
		}
		const TSharedPtr<FJsonObject>* ActorObj = nullptr;
		TSharedPtr<FJsonObject> BeforeActor = BeforeDoor;
		if (BeforeDoor->TryGetObjectField(TEXT("actor"), ActorObj) && ActorObj)
		{
			BeforeActor = *ActorObj;
		}
		FVector BeforeLoc = FVector::ZeroVector;
		FVector BeforeRot = FVector::ZeroVector;
		FVector BeforeScale = FVector::OneVector;
		if (!GetVector(BeforeActor, TEXT("location"), BeforeLoc)
			|| !GetVector(BeforeActor, TEXT("rotation"), BeforeRot)
			|| !GetVector(BeforeActor, TEXT("scale"), BeforeScale))
		{
			return TEXT("Access Door before-snapshot is incomplete.");
		}
		const FString BeforePackage = NormalizePackage(GetString(BeforeActor, TEXT("owning_package")));
		const FString AfterPackage = NormalizePackage(ActorOwningPackage(Door));
		if (!PackagesEqual(BeforePackage, AfterPackage))
		{
			return FString::Printf(
				TEXT("Access Door owning package changed from '%s' to '%s'"),
				*BeforePackage, *AfterPackage);
		}
		const FString Mismatch = TransformMismatch(
			Door,
			BeforeLoc,
			FRotator(BeforeRot.X, BeforeRot.Y, BeforeRot.Z),
			BeforeScale);
		if (!Mismatch.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door %s"), *Mismatch);
		}
		return TEXT("");
	}

	FString RequireEpitopeEditorWorld(UWorld*& OutWorld)
	{
		OutWorld = GetEditorWorld();
		if (!OutWorld)
		{
			return TEXT("No editor world.");
		}
		if (!IsEpitopeWorldPackage(WorldPackageName(OutWorld)))
		{
			return FString::Printf(
				TEXT("Persistent map must be %s (current package '%s', map '%s'). Open Lvl_Epitope. Do not use Lvl_MainMenu."),
				EpitopePackage,
				*NormalizePackage(WorldPackageName(OutWorld)),
				*OutWorld->GetMapName());
		}
		return TEXT("");
	}

	FString PreflightMoveActorToLevel(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. move_actor_to_level does not save maps.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Blueprints are not compiled.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}

		const FString Label = GetString(Args, TEXT("actor"), GetString(Args, TEXT("label"), GetString(Args, TEXT("name"))));
		const FString ActorPath = GetString(Args, TEXT("actor_path"));
		const FString SourcePackage = NormalizePackage(GetString(Args, TEXT("source_package"), GetString(Args, TEXT("expected_source_package"))));
		const FString DestPackage = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("dest_package"), GetString(Args, TEXT("destination_level_package")))));
		const FString RequiredPersistent = NormalizePackage(
			GetString(Args, TEXT("require_persistent_map"), GetString(Args, TEXT("required_world_package"), EpitopePackage)));
		const FString DoorQuery = GetString(Args, TEXT("door_label"), AccessDoorLabel);

		FVector ExpectedLocation = FVector::ZeroVector;
		FVector ExpectedRotationVec = FVector::ZeroVector;
		FVector ExpectedScale = FVector::OneVector;
		const bool bHasLocation = GetVector(Args, TEXT("expected_location"), ExpectedLocation) || GetVector(Args, TEXT("location"), ExpectedLocation);
		const bool bHasRotation = GetVector(Args, TEXT("expected_rotation"), ExpectedRotationVec) || GetVector(Args, TEXT("rotation"), ExpectedRotationVec);
		const bool bHasScale = GetVector(Args, TEXT("expected_scale"), ExpectedScale) || GetVector(Args, TEXT("scale"), ExpectedScale);

		if (Label.IsEmpty())
		{
			return TEXT("explicit actor label is required.");
		}
		if (SourcePackage.IsEmpty())
		{
			return TEXT("explicit source_package is required.");
		}
		if (DestPackage.IsEmpty())
		{
			return TEXT("explicit destination_package is required.");
		}
		if (!bHasLocation || !bHasRotation || !bHasScale)
		{
			return TEXT("explicit expected_location, expected_rotation, and expected_scale are required.");
		}

		if (!Label.Equals(ReceptionLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("move_actor_to_level is allowlisted only for '%s' (got '%s')."), ReceptionLabel, *Label);
		}
		if (!PackagesEqual(SourcePackage, EpitopePackage))
		{
			return TEXT("source_package must be /Game/Maps/Lvl_Epitope.");
		}
		if (!PackagesEqual(DestPackage, AdminPackage))
		{
			return TEXT("destination_package must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}
		if (!PackagesEqual(RequiredPersistent, EpitopePackage))
		{
			return TEXT("require_persistent_map must be /Game/Maps/Lvl_Epitope.");
		}
		if (!LocationMatches(ExpectedLocation, ReceptionLocation)
			|| !RotationMatches(FRotator(ExpectedRotationVec.X, ExpectedRotationVec.Y, ExpectedRotationVec.Z), ReceptionRotation)
			|| !ScaleMatches(ExpectedScale, ReceptionScale))
		{
			return TEXT("expected transform must be location (1450,-150,110), rotation (0,0,0), scale (1,1,1).");
		}

		UWorld* World = nullptr;
		const FString WorldError = RequireEpitopeEditorWorld(World);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}

		ULevelStreaming* DestStreaming = FindStreamingLevel(World, DestPackage);
		if (!DestStreaming)
		{
			return TEXT("Destination streaming level SL_Epitope_Admin is not present on Lvl_Epitope.");
		}
		if (!DestStreaming->IsLevelLoaded() || !DestStreaming->GetLoadedLevel())
		{
			return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_Admin is not loaded.");
		}

		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1"), *Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (!ActorPath.IsEmpty() && !Actor->GetPathName().Contains(ActorPath))
		{
			return FString::Printf(TEXT("Actor path '%s' does not contain expected path '%s'."), *Actor->GetPathName(), *ActorPath);
		}
		const FString Owner = NormalizePackage(ActorOwningPackage(Actor));
		if (!PackagesEqual(Owner, SourcePackage))
		{
			return FString::Printf(TEXT("%s owning package '%s' does not match source '%s'"), *Label, *Owner, *SourcePackage);
		}
		const FString XformError = TransformMismatch(Actor, ReceptionLocation, ReceptionRotation, ReceptionScale);
		if (!XformError.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s"), *Label, *XformError);
		}

		AActor* Door = FindUniqueLabel(World, DoorQuery);
		if (!Door)
		{
			return TEXT("Access Door not found or not unique. ZERO writes.");
		}

		Before->SetStringField(TEXT("owning_package"), Owner);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		Before->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		Before->SetStringField(TEXT("destination_package"), DestPackage);
		Before->SetBoolField(TEXT("destination_loaded"), true);

		Proposed->SetStringField(TEXT("owning_package"), DestPackage);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetArrayField(TEXT("location"), Vec(ReceptionLocation));
		Proposed->SetArrayField(TEXT("rotation"), Rot(ReceptionRotation));
		Proposed->SetArrayField(TEXT("scale"), Vec(ReceptionScale));
		Proposed->SetStringField(TEXT("result"), TEXT("Actor cut/pasted into Admin via UEditorLevelUtils::MoveActorsToLevel on the game thread. Maps not saved."));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	bool ParseSaveMapsPackages(const TSharedPtr<FJsonObject>& Args, TArray<FString>& OutPackages, bool& bAdminOnly, bool& bNeuroOnly, bool& bEpitopeOnly, bool& bCryoOnly, bool& bComputeOnly, bool& bReactorOnly, FString& OutError)
	{
		OutPackages.Reset();
		bAdminOnly = false;
		bNeuroOnly = false;
		bEpitopeOnly = false;
		bCryoOnly = false;
		bComputeOnly = false;
		bReactorOnly = false;
		const TArray<TSharedPtr<FJsonValue>>* PackagesArr = nullptr;
		if (!Args.IsValid() || !Args->TryGetArrayField(TEXT("packages"), PackagesArr) || !PackagesArr)
		{
			OutError = TEXT("packages array is required.");
			return false;
		}
		if (PackagesArr->Num() == 1)
		{
			const FString Only = NormalizePackage((*PackagesArr)[0].IsValid() ? (*PackagesArr)[0]->AsString() : FString());
			if (PackagesEqual(Only, AdminPackage))
			{
				OutPackages.Add(FString(AdminPackage));
				bAdminOnly = true;
				return true;
			}
			if (PackagesEqual(Only, NeuroPackage))
			{
				OutPackages.Add(FString(NeuroPackage));
				bNeuroOnly = true;
				return true;
			}
			if (PackagesEqual(Only, EpitopePackage))
			{
				OutPackages.Add(FString(EpitopePackage));
				bEpitopeOnly = true;
				return true;
			}
			if (PackagesEqual(Only, CryoPackage))
			{
				OutPackages.Add(FString(CryoPackage));
				bCryoOnly = true;
				return true;
			}
			if (PackagesEqual(Only, ComputePackage))
			{
				OutPackages.Add(FString(ComputePackage));
				bComputeOnly = true;
				return true;
			}
			if (PackagesEqual(Only, ReactorPackage))
			{
				OutPackages.Add(FString(ReactorPackage));
				bReactorOnly = true;
				return true;
			}
			OutError = TEXT("single-package save_maps must be Admin, NeuroGenetics, Cryo, Compute, Reactor, or Lvl_Epitope.");
			return false;
		}
		if (PackagesArr->Num() == 2)
		{
			const FString First = NormalizePackage((*PackagesArr)[0].IsValid() ? (*PackagesArr)[0]->AsString() : FString());
			const FString Second = NormalizePackage((*PackagesArr)[1].IsValid() ? (*PackagesArr)[1]->AsString() : FString());
			if (!PackagesEqual(First, AdminPackage) || !PackagesEqual(Second, EpitopePackage))
			{
				OutError = TEXT("two-package save_maps must be exactly [/Game/Maps/Epitope/SL_Epitope_Admin, /Game/Maps/Lvl_Epitope] in that order.");
				return false;
			}
			OutPackages.Add(FString(AdminPackage));
			OutPackages.Add(FString(EpitopePackage));
			bAdminOnly = false;
			return true;
		}
		OutError = FString::Printf(
			TEXT("packages count=%d expected=1 (Admin or Neuro only) or 2 (Admin then Lvl_Epitope)."),
			PackagesArr->Num());
		return false;
	}

	FString CheckBoolProperty(AActor* Actor, const TCHAR* Name, bool bExpected)
	{
		FProperty* Property = FindInstanceProperty(Actor, Name);
		const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
		if (!BoolProp)
		{
			return FString::Printf(TEXT("Property '%s' is not a bool on instance."), Name);
		}
		const bool bLive = BoolProp->GetPropertyValue_InContainer(Actor);
		if (bLive != bExpected)
		{
			return FString::Printf(
				TEXT("%s live %s expected %s"),
				Name,
				bLive ? TEXT("true") : TEXT("false"),
				bExpected ? TEXT("true") : TEXT("false"));
		}
		return TEXT("");
	}

	FString ReceptionConfigMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Reception actor is null.");
		}

		auto CheckNameOrText = [&](const TCHAR* Name, const TCHAR* Expected) -> FString
		{
			FProperty* Property = FindInstanceProperty(Actor, Name);
			if (!Property)
			{
				return FString::Printf(TEXT("Property '%s' not found on instance."), Name);
			}
			const FString Live = ReadNameOrTextValue(Actor, Property);
			if (!Live.Equals(Expected, ESearchCase::CaseSensitive))
			{
				return FString::Printf(TEXT("%s live '%s' expected '%s'"), Name, *Live, Expected);
			}
			return TEXT("");
		};

		FString Error = CheckNameOrText(TEXT("TerminalID"), TEXT("Terminal_AdminReception"));
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckNameOrText(TEXT("Title"), TEXT("Reception Terminal"));
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckNameOrText(TEXT("InteractionPrompt"), TEXT("Use Terminal"));
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckBoolProperty(Actor, TEXT("bInitiallyPowered"), true);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckBoolProperty(Actor, TEXT("bOneShot"), false);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckBoolProperty(Actor, TEXT("bIsPowered"), false);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckBoolProperty(Actor, TEXT("bHasActivated"), false);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = CheckBoolProperty(Actor, TEXT("bIsInteractable"), true);
		if (!Error.IsEmpty())
		{
			return Error;
		}

		FProperty* TypeProp = FindInstanceProperty(Actor, TEXT("TerminalType"));
		int64 TypeValue = INDEX_NONE;
		FString Internal;
		FString Display;
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(TypeProp))
		{
			TypeValue = ByteProp->GetPropertyValue_InContainer(Actor);
			if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
			{
				Internal = Enum->GetNameStringByValue(TypeValue);
				Display = Enum->GetDisplayNameTextByValue(TypeValue).ToString();
			}
		}
		else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(TypeProp))
		{
			TypeValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
				EnumProp->ContainerPtrToValuePtr<void>(Actor));
			if (UEnum* Enum = EnumProp->GetEnum())
			{
				Internal = Enum->GetNameStringByValue(TypeValue);
				Display = Enum->GetDisplayNameTextByValue(TypeValue).ToString();
			}
		}
		else
		{
			return TEXT("TerminalType is not a byte/enum property.");
		}
		if (TypeValue != 0)
		{
			return FString::Printf(TEXT("TerminalType numeric %lld expected 0"), TypeValue);
		}
		if (!Internal.Contains(TEXT("NewEnumerator0")) && !Display.Equals(TEXT("Reception")))
		{
			return FString::Printf(
				TEXT("TerminalType internal '%s' display '%s' expected NewEnumerator0 / Reception"),
				*Internal, *Display);
		}

		FProperty* RangeProp = FindInstanceProperty(Actor, TEXT("InteractionRange"));
		const FNumericProperty* NumProp = CastField<FNumericProperty>(RangeProp);
		if (!NumProp || !NumProp->IsFloatingPoint())
		{
			return TEXT("InteractionRange is not a float.");
		}
		const double Range = NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Actor));
		if (!FMath::IsNearlyEqual(Range, 150.0, 0.01))
		{
			return FString::Printf(TEXT("InteractionRange live %f expected 150"), Range);
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> SnapshotReceptionConfig(AActor* Actor)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Actor)
		{
			return Out;
		}
		auto PutNameOrText = [&](const TCHAR* Name)
		{
			FProperty* Property = FindInstanceProperty(Actor, Name);
			Out->SetStringField(Name, ReadNameOrTextValue(Actor, Property));
		};
		auto PutBool = [&](const TCHAR* Name)
		{
			FProperty* Property = FindInstanceProperty(Actor, Name);
			if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
			{
				Out->SetBoolField(Name, BoolProp->GetPropertyValue_InContainer(Actor));
			}
		};
		PutNameOrText(TEXT("TerminalID"));
		PutNameOrText(TEXT("Title"));
		PutNameOrText(TEXT("InteractionPrompt"));
		PutBool(TEXT("bInitiallyPowered"));
		PutBool(TEXT("bOneShot"));
		PutBool(TEXT("bIsPowered"));
		PutBool(TEXT("bHasActivated"));
		PutBool(TEXT("bIsInteractable"));
		FProperty* TypeProp = FindInstanceProperty(Actor, TEXT("TerminalType"));
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(TypeProp))
		{
			const uint8 Value = ByteProp->GetPropertyValue_InContainer(Actor);
			Out->SetNumberField(TEXT("TerminalType"), static_cast<double>(Value));
			if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
			{
				Out->SetStringField(TEXT("TerminalType_internal"), Enum->GetNameStringByValue(Value));
				Out->SetStringField(TEXT("TerminalType_display"), Enum->GetDisplayNameTextByValue(Value).ToString());
			}
		}
		FProperty* RangeProp = FindInstanceProperty(Actor, TEXT("InteractionRange"));
		if (const FNumericProperty* NumProp = CastField<FNumericProperty>(RangeProp))
		{
			Out->SetNumberField(
				TEXT("InteractionRange"),
				NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Actor)));
		}
		return Out;
	}

	FString PreflightSaveMaps(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("save_dirty"), false))
		{
			return TEXT("Save All / save_dirty is forbidden. save_maps targets explicit packages only.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Blueprints are not compiled.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}

		TArray<FString> Packages;
		bool bAdminOnly = false;
		bool bNeuroOnly = false;
		bool bEpitopeOnly = false;
		bool bCryoOnly = false;
		bool bComputeOnly = false;
		bool bReactorOnly = false;
		FString ParseError;
		if (!ParseSaveMapsPackages(Args, Packages, bAdminOnly, bNeuroOnly, bEpitopeOnly, bCryoOnly, bComputeOnly, bReactorOnly, ParseError))
		{
			return ParseError;
		}
		for (const FString& PackageName : Packages)
		{
			if (PackagesEqual(PackageName, MainMenuPackage))
			{
				return TEXT("Lvl_MainMenu must not be included in save_maps.");
			}
			if (bAdminOnly && PackagesEqual(PackageName, EpitopePackage))
			{
				return TEXT("Admin-only save_maps must not include /Game/Maps/Lvl_Epitope.");
			}
			if (bNeuroOnly && (PackagesEqual(PackageName, AdminPackage) || PackagesEqual(PackageName, EpitopePackage)))
			{
				return TEXT("Neuro-only save_maps must not include Admin or Lvl_Epitope.");
			}
			if (bCryoOnly && (PackagesEqual(PackageName, AdminPackage) || PackagesEqual(PackageName, EpitopePackage) || PackagesEqual(PackageName, NeuroPackage) || PackagesEqual(PackageName, ComputePackage) || PackagesEqual(PackageName, ReactorPackage)))
			{
				return TEXT("Cryo-only save_maps must not include Admin, NeuroGenetics, Compute, Reactor, or Lvl_Epitope.");
			}
			if (bComputeOnly && (PackagesEqual(PackageName, AdminPackage) || PackagesEqual(PackageName, EpitopePackage) || PackagesEqual(PackageName, NeuroPackage) || PackagesEqual(PackageName, CryoPackage) || PackagesEqual(PackageName, ReactorPackage)))
			{
				return TEXT("Compute-only save_maps must not include Admin, NeuroGenetics, Cryo, Reactor, or Lvl_Epitope.");
			}
			if (bReactorOnly && (PackagesEqual(PackageName, AdminPackage) || PackagesEqual(PackageName, EpitopePackage) || PackagesEqual(PackageName, NeuroPackage) || PackagesEqual(PackageName, CryoPackage) || PackagesEqual(PackageName, ComputePackage)))
			{
				return TEXT("Reactor-only save_maps must not include Admin, NeuroGenetics, Cryo, Compute, or Lvl_Epitope.");
			}
			if (bEpitopeOnly && (PackagesEqual(PackageName, AdminPackage) || PackagesEqual(PackageName, NeuroPackage)))
			{
				return TEXT("Lvl_Epitope-only save_maps must not include Admin or NeuroGenetics.");
			}
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (bCryoOnly)
		{
			ULevel* CryoLevel = FindLoadedLevelByPackage(World, CryoPackage);
			if (!CryoLevel)
			{
				return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_Cryo is not loaded.");
			}
			if (!FindPackageByName(CryoPackage))
			{
				return TEXT("Cryo map package is not loaded in memory.");
			}
			if (GetPieWorld())
			{
				return TEXT("PIE is running. Stop Play before a Cryo-only save.");
			}
			TArray<AActor*> PanelMatches = FindByExactLabel(World, TEXT("PowerPanel_CryoBackup"));
			if (PanelMatches.Num() != 1 || !PackagesEqual(ActorOwningPackage(PanelMatches[0]), CryoPackage))
			{
				return FString::Printf(TEXT("PowerPanel_CryoBackup count=%d must be the unique Cryo panel before Cryo save."), PanelMatches.Num());
			}
			TArray<AActor*> CheckpointMatches = FindByExactLabel(World, TEXT("Checkpoint_FreightAirlock"));
			if (CheckpointMatches.Num() != 1 || !LocationMatches(CheckpointMatches[0]->GetActorLocation(), FVector(1950.f, 0.f, -2340.f)))
			{
				return TEXT("Checkpoint_FreightAirlock must stay unique at (1950, 0, -2340) before Cryo save.");
			}
			Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
			Before->SetBoolField(TEXT("pie_running"), false);
			Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Before->SetBoolField(TEXT("cryo_only"), true);
			Before->SetBoolField(TEXT("neuro_only"), false);
			Before->SetBoolField(TEXT("admin_only"), false);
			TArray<TSharedPtr<FJsonValue>> CryoPkgs;
			CryoPkgs.Add(MakeShared<FJsonValueString>(FString(CryoPackage)));
			Before->SetArrayField(TEXT("packages"), CryoPkgs);
			Proposed->SetArrayField(TEXT("packages"), CryoPkgs);
			Proposed->SetBoolField(TEXT("cryo_only"), true);
			Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Proposed->SetBoolField(TEXT("dialog"), false);
			Proposed->SetBoolField(TEXT("save_all"), false);
			Proposed->SetBoolField(TEXT("compile"), false);
			Proposed->SetStringField(TEXT("result"), TEXT("Save SL_Epitope_Cryo only. Does not save Admin, NeuroGenetics, or Lvl_Epitope."));
			return TEXT("");
		}
		if (bComputeOnly)
		{
			ULevel* ComputeLevel = FindLoadedLevelByPackage(World, ComputePackage);
			if (!ComputeLevel)
			{
				return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_Compute is not loaded.");
			}
			if (!FindPackageByName(ComputePackage))
			{
				return TEXT("Compute map package is not loaded in memory.");
			}
			if (GetPieWorld())
			{
				return TEXT("PIE is running. Stop Play before a Compute-only save.");
			}
			TArray<AActor*> CheckpointMatches = FindByExactLabel(World, TEXT("Checkpoint_InterfaceChamber"));
			if (CheckpointMatches.Num() != 1 || !PackagesEqual(ActorOwningPackage(CheckpointMatches[0]), ComputePackage) || !LocationMatches(CheckpointMatches[0]->GetActorLocation(), FVector(-2425.f, -1650.f, -3540.f)))
			{
				return TEXT("Checkpoint_InterfaceChamber must stay unique at (-2425, -1650, -3540) on SL_Epitope_Compute before Compute save.");
			}
			Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
			Before->SetBoolField(TEXT("pie_running"), false);
			Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Before->SetBoolField(TEXT("compute_only"), true);
			Before->SetBoolField(TEXT("cryo_only"), false);
			Before->SetBoolField(TEXT("neuro_only"), false);
			Before->SetBoolField(TEXT("admin_only"), false);
			TArray<TSharedPtr<FJsonValue>> ComputePkgs;
			ComputePkgs.Add(MakeShared<FJsonValueString>(FString(ComputePackage)));
			Before->SetArrayField(TEXT("packages"), ComputePkgs);
			Proposed->SetArrayField(TEXT("packages"), ComputePkgs);
			Proposed->SetBoolField(TEXT("compute_only"), true);
			Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Proposed->SetBoolField(TEXT("dialog"), false);
			Proposed->SetBoolField(TEXT("save_all"), false);
			Proposed->SetBoolField(TEXT("compile"), false);
			Proposed->SetStringField(TEXT("result"), TEXT("Save SL_Epitope_Compute only. Does not save Admin, NeuroGenetics, Cryo, or Lvl_Epitope."));
			return TEXT("");
		}
		if (bReactorOnly)
		{
			ULevel* ReactorLevel = FindLoadedLevelByPackage(World, ReactorPackage);
			if (!ReactorLevel)
			{
				return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_Reactor is not loaded.");
			}
			if (!FindPackageByName(ReactorPackage))
			{
				return TEXT("Reactor map package is not loaded in memory.");
			}
			if (GetPieWorld())
			{
				return TEXT("PIE is running. Stop Play before a Reactor-only save.");
			}
			TArray<AActor*> CheckpointMatches = FindByExactLabel(World, TEXT("Checkpoint_BasinRim"));
			if (CheckpointMatches.Num() != 1 || !PackagesEqual(ActorOwningPackage(CheckpointMatches[0]), ReactorPackage) || !LocationMatches(CheckpointMatches[0]->GetActorLocation(), FVector(25.f, 0.f, -4740.f)))
			{
				return TEXT("Checkpoint_BasinRim must stay unique at (25, 0, -4740) on SL_Epitope_Reactor before Reactor save.");
			}
			Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
			Before->SetBoolField(TEXT("pie_running"), false);
			Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Before->SetBoolField(TEXT("reactor_only"), true);
			Before->SetBoolField(TEXT("compute_only"), false);
			Before->SetBoolField(TEXT("cryo_only"), false);
			Before->SetBoolField(TEXT("neuro_only"), false);
			Before->SetBoolField(TEXT("admin_only"), false);
			TArray<TSharedPtr<FJsonValue>> ReactorPkgs;
			ReactorPkgs.Add(MakeShared<FJsonValueString>(FString(ReactorPackage)));
			Before->SetArrayField(TEXT("packages"), ReactorPkgs);
			Proposed->SetArrayField(TEXT("packages"), ReactorPkgs);
			Proposed->SetBoolField(TEXT("reactor_only"), true);
			Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Proposed->SetBoolField(TEXT("dialog"), false);
			Proposed->SetBoolField(TEXT("save_all"), false);
			Proposed->SetBoolField(TEXT("compile"), false);
			Proposed->SetStringField(TEXT("result"), TEXT("Save SL_Epitope_Reactor only. Does not save Admin, NeuroGenetics, Cryo, Compute, or Lvl_Epitope."));
			return TEXT("");
		}
		if (bNeuroOnly)
		{
			ULevel* NeuroLevel = FindLoadedLevelByPackage(World, NeuroPackage);
			if (!NeuroLevel)
			{
				return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_NeuroGenetics is not loaded.");
			}
			if (!FindPackageByName(NeuroPackage))
			{
				return TEXT("NeuroGenetics map package is not loaded in memory.");
			}
			TArray<AActor*> NavMatches = FindByExactLabel(World, TEXT("NavMeshBounds_NeuroGenetics"));
			if (NavMatches.Num() != 1)
			{
				return FString::Printf(TEXT("NavMeshBounds_NeuroGenetics count=%d expected=1 before Neuro save."), NavMatches.Num());
			}
			const FString NavOwner = NormalizePackage(ActorOwningPackage(NavMatches[0]));
			if (!PackagesEqual(NavOwner, NeuroPackage))
			{
				return FString::Printf(TEXT("NavMeshBounds_NeuroGenetics owning package '%s' is not NeuroGenetics."), *NavOwner);
			}

			Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
			Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
			Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Before->SetBoolField(TEXT("admin_only"), false);
			Before->SetBoolField(TEXT("neuro_only"), true);
			Before->SetObjectField(TEXT("nav_bounds"), ActorSnapshot(NavMatches[0]));
			TArray<TSharedPtr<FJsonValue>> NeuroPkgs;
			NeuroPkgs.Add(MakeShared<FJsonValueString>(FString(NeuroPackage)));
			Before->SetArrayField(TEXT("packages"), NeuroPkgs);
			Proposed->SetArrayField(TEXT("packages"), NeuroPkgs);
			Proposed->SetBoolField(TEXT("neuro_only"), true);
			Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Proposed->SetBoolField(TEXT("dialog"), false);
			Proposed->SetBoolField(TEXT("save_all"), false);
			Proposed->SetBoolField(TEXT("compile"), false);
			Proposed->SetStringField(
				TEXT("result"),
				TEXT("Save SL_Epitope_NeuroGenetics only on the game thread. Persistent may be Lvl_MainMenu. Admin, Lvl_Epitope, and Lvl_MainMenu not saved."));
			return TEXT("");
		}
		if (bEpitopeOnly)
		{
			const FString WorldError = RequireEpitopeEditorWorld(World);
			if (!WorldError.IsEmpty())
			{
				return WorldError;
			}
			if (!FindPackageByName(EpitopePackage))
			{
				return TEXT("Lvl_Epitope map package is not loaded in memory.");
			}
			TArray<AActor*> LandingMatches = FindByExactLabel(World, TEXT("Spine_Landing_Admin"));
			if (LandingMatches.Num() != 1)
			{
				return FString::Printf(TEXT("Spine_Landing_Admin count=%d expected=1 before Lvl_Epitope save."), LandingMatches.Num());
			}
			const FString LandingOwner = NormalizePackage(ActorOwningPackage(LandingMatches[0]));
			if (!PackagesEqual(LandingOwner, EpitopePackage))
			{
				return FString::Printf(TEXT("Spine_Landing_Admin owning package '%s' is not Lvl_Epitope."), *LandingOwner);
			}
			TArray<AActor*> GateMatches = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
			if (GateMatches.Num() != 1)
			{
				return FString::Printf(TEXT("Gate_ResearchWing count=%d expected=1 before Lvl_Epitope save."), GateMatches.Num());
			}
			const FString GateOwner = NormalizePackage(ActorOwningPackage(GateMatches[0]));
			if (!PackagesEqual(GateOwner, EpitopePackage))
			{
				return FString::Printf(TEXT("Gate_ResearchWing owning package '%s' is not Lvl_Epitope."), *GateOwner);
			}

			Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
			Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
			Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Before->SetBoolField(TEXT("admin_only"), false);
			Before->SetBoolField(TEXT("neuro_only"), false);
			Before->SetBoolField(TEXT("epitope_only"), true);
			Before->SetObjectField(TEXT("spine_landing_admin"), ActorSnapshot(LandingMatches[0]));
			Before->SetObjectField(TEXT("gate_research_wing"), ActorSnapshot(GateMatches[0]));
			TArray<TSharedPtr<FJsonValue>> EpitopePkgs;
			EpitopePkgs.Add(MakeShared<FJsonValueString>(FString(EpitopePackage)));
			Before->SetArrayField(TEXT("packages"), EpitopePkgs);
			Proposed->SetArrayField(TEXT("packages"), EpitopePkgs);
			Proposed->SetBoolField(TEXT("epitope_only"), true);
			Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Proposed->SetBoolField(TEXT("dialog"), false);
			Proposed->SetBoolField(TEXT("save_all"), false);
			Proposed->SetBoolField(TEXT("compile"), false);
			Proposed->SetStringField(
				TEXT("result"),
				TEXT("Save Lvl_Epitope only on the game thread. Persistent must be Lvl_Epitope. Admin, Neuro, and Lvl_MainMenu not saved."));
			return TEXT("");
		}
		if (!bAdminOnly)
		{
			const FString WorldError = RequireEpitopeEditorWorld(World);
			if (!WorldError.IsEmpty())
			{
				return WorldError;
			}
		}

		ULevel* AdminLevel = FindLoadedLevelByPackage(World, AdminPackage);
		if (!AdminLevel)
		{
			return TEXT("Destination /Game/Maps/Epitope/SL_Epitope_Admin is not loaded.");
		}

		TArray<AActor*> SecurityMatches = FindByExactLabel(World, SecurityTerminalLabel);
		if (SecurityMatches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1 before save"), SecurityTerminalLabel, SecurityMatches.Num());
		}
		AActor* Security = SecurityMatches[0];
		const FString SecurityOwner = NormalizePackage(ActorOwningPackage(Security));
		if (!PackagesEqual(SecurityOwner, AdminPackage))
		{
			return FString::Printf(
				TEXT("%s owning package '%s' is not Admin."),
				SecurityTerminalLabel, *SecurityOwner);
		}
		const FString SecurityXformError = TransformMismatch(
			Security, SecurityTerminalLocation, SecurityTerminalRotation, SecurityTerminalScale);
		if (!SecurityXformError.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s"), SecurityTerminalLabel, *SecurityXformError);
		}

		TArray<AActor*> Matches = FindByExactLabel(World, ReceptionLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1 before save"), ReceptionLabel, Matches.Num());
		}
		AActor* Actor = Matches[0];
		const FString Owner = NormalizePackage(ActorOwningPackage(Actor));
		if (!PackagesEqual(Owner, AdminPackage))
		{
			return FString::Printf(
				TEXT("%s owning package '%s' is not Admin. Move must succeed before save_maps."),
				ReceptionLabel, *Owner);
		}
		const FString XformError = TransformMismatch(Actor, ReceptionLocation, ReceptionRotation, ReceptionScale);
		if (!XformError.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s"), ReceptionLabel, *XformError);
		}
		const FString ConfigError = ReceptionConfigMismatch(Actor);
		if (!ConfigError.IsEmpty())
		{
			return ConfigError;
		}

		AActor* Door = FindUniqueLabel(World, AccessDoorLabel);
		if (!Door)
		{
			return TEXT("Access Door not found or not unique. ZERO writes.");
		}

		if (!FindPackageByName(AdminPackage))
		{
			return TEXT("Admin map package is not loaded in memory.");
		}
		if (!bAdminOnly && !FindPackageByName(EpitopePackage))
		{
			return TEXT("Lvl_Epitope map package is not loaded in memory.");
		}

		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Before->SetBoolField(TEXT("admin_only"), bAdminOnly);
		Before->SetObjectField(TEXT("security"), ActorSnapshot(Security));
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		Before->SetObjectField(TEXT("reception_properties"), SnapshotReceptionConfig(Actor));
		Before->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		TArray<TSharedPtr<FJsonValue>> Pkgs;
		for (const FString& PackageName : Packages)
		{
			Pkgs.Add(MakeShared<FJsonValueString>(PackageName));
		}
		Before->SetArrayField(TEXT("packages"), Pkgs);

		Proposed->SetArrayField(TEXT("packages"), Pkgs);
		Proposed->SetBoolField(TEXT("admin_only"), bAdminOnly);
		Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
		Proposed->SetBoolField(TEXT("dialog"), false);
		Proposed->SetBoolField(TEXT("save_all"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(
			TEXT("result"),
			bAdminOnly
				? TEXT("Save SL_Epitope_Admin only on the game thread. Persistent may be Lvl_MainMenu. Lvl_Epitope and Lvl_MainMenu not saved. No checkout UI. No Save All.")
				: TEXT("Save Admin then Lvl_Epitope on the game thread. Persistent must be Lvl_Epitope. No checkout UI. No Save All."));
		return TEXT("");
	}

	FProperty* FindInstanceProperty(UObject* Object, const FString& PropertyName)
	{
		if (!Object || PropertyName.IsEmpty() || !Object->GetClass())
		{
			return nullptr;
		}
		return FindFProperty<FProperty>(Object->GetClass(), FName(*PropertyName));
	}

	FString ReadNameOrTextValue(AActor* Actor, FProperty* Property)
	{
		if (!Actor || !Property)
		{
			return TEXT("");
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			return NameProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			return TextProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		return TEXT("");
	}

	bool SetNameOrTextValue(AActor* Actor, FProperty* Property, const FString& NewValue, FString& OutError)
	{
		if (!Actor || !Property)
		{
			OutError = TEXT("Actor or property is null.");
			return false;
		}
		if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			NameProp->SetPropertyValue_InContainer(Actor, FName(*NewValue));
			return true;
		}
		if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			TextProp->SetPropertyValue_InContainer(Actor, FText::FromString(NewValue));
			return true;
		}
		OutError = FString::Printf(TEXT("Property '%s' type '%s' is not FNameProperty or FTextProperty."),
			*Property->GetName(), *Property->GetClass()->GetName());
		return false;
	}

	struct FActorPropertyMutation
	{
		FString PropertyName;
		FString Expected;
		FString NewValue;
	};

	bool HasExplicitStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* A, const TCHAR* B)
	{
		return Object.IsValid() && (Object->HasField(A) || Object->HasField(B));
	}

	bool ParseMutations(const TSharedPtr<FJsonObject>& Args, TArray<FActorPropertyMutation>& OutMutations, FString& OutError)
	{
		OutMutations.Reset();
		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (Args.IsValid() && Args->TryGetArrayField(TEXT("mutations"), Arr) && Arr && Arr->Num() > 0)
		{
			for (const TSharedPtr<FJsonValue>& Entry : *Arr)
			{
				const TSharedPtr<FJsonObject> Row = Entry.IsValid() ? Entry->AsObject() : nullptr;
				if (!Row.IsValid())
				{
					OutError = TEXT("Each mutation must be an object with property, expected, and value.");
					return false;
				}
				if (!HasExplicitStringField(Row, TEXT("expected"), TEXT("expected_old")))
				{
					OutError = TEXT("Each mutation requires an explicit expected old value.");
					return false;
				}
				if (!HasExplicitStringField(Row, TEXT("value"), TEXT("new")))
				{
					OutError = TEXT("Each mutation requires an explicit new value.");
					return false;
				}
				FActorPropertyMutation Mutation;
				Mutation.PropertyName = GetString(Row, TEXT("property"), GetString(Row, TEXT("property_name")));
				Mutation.Expected = GetString(Row, TEXT("expected"), GetString(Row, TEXT("expected_old")));
				Mutation.NewValue = GetString(Row, TEXT("value"), GetString(Row, TEXT("new")));
				if (Mutation.PropertyName.IsEmpty())
				{
					OutError = TEXT("Each mutation requires an explicit property name.");
					return false;
				}
				OutMutations.Add(Mutation);
			}
			return true;
		}

		if (!HasExplicitStringField(Args, TEXT("expected"), TEXT("expected_old")))
		{
			OutError = TEXT("expected old value is required.");
			return false;
		}
		if (!HasExplicitStringField(Args, TEXT("value"), TEXT("new")))
		{
			OutError = TEXT("new value is required.");
			return false;
		}
		FActorPropertyMutation Mutation;
		Mutation.PropertyName = GetString(Args, TEXT("property"), GetString(Args, TEXT("property_name")));
		Mutation.Expected = GetString(Args, TEXT("expected"), GetString(Args, TEXT("expected_old")));
		Mutation.NewValue = GetString(Args, TEXT("value"), GetString(Args, TEXT("new")));
		if (Mutation.PropertyName.IsEmpty())
		{
			OutError = TEXT("property or mutations array is required.");
			return false;
		}
		OutMutations.Add(Mutation);
		return true;
	}

	FString PreflightSetActorProperty(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. set_actor_property does not save maps.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Blueprints are not compiled.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}

		TArray<FActorPropertyMutation> Mutations;
		FString ParseError;
		if (!ParseMutations(Args, Mutations, ParseError))
		{
			return ParseError;
		}
		if (Mutations.Num() < 1)
		{
			return TEXT("At least one mutation is required.");
		}

		const FString Label = GetString(Args, TEXT("actor"), GetString(Args, TEXT("label"), GetString(Args, TEXT("name"))));
		if (Label.IsEmpty())
		{
			return TEXT("explicit actor label/path is required.");
		}
		if (Label.Contains(TEXT("AdminAccessDoor")) || Label.Contains(TEXT("BP_AdminAccessDoor")))
		{
			return TEXT("set_actor_property cannot target BP_AdminAccessDoor.");
		}

		UWorld* World = nullptr;
		const FString WorldError = RequireEpitopeEditorWorld(World);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1"), *Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		const FString Owner = NormalizePackage(ActorOwningPackage(Actor));
		const FString RequiredOwner = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("require_owning_package"), AdminPackage)));
		if (!PackagesEqual(Owner, RequiredOwner))
		{
			return FString::Printf(TEXT("%s owning package '%s' does not match '%s'"), *Label, *Owner, *RequiredOwner);
		}

		FVector ExpectedLocation = ReceptionLocation;
		FVector ExpectedRotationVec = FVector::ZeroVector;
		FVector ExpectedScale = ReceptionScale;
		if (!GetVector(Args, TEXT("expected_location"), ExpectedLocation) && !GetVector(Args, TEXT("location"), ExpectedLocation))
		{
			return TEXT("expected_location is required.");
		}
		GetVector(Args, TEXT("expected_rotation"), ExpectedRotationVec) || GetVector(Args, TEXT("rotation"), ExpectedRotationVec);
		GetVector(Args, TEXT("expected_scale"), ExpectedScale) || GetVector(Args, TEXT("scale"), ExpectedScale);
		{
			const FString XformError = TransformMismatch(
				Actor,
				ExpectedLocation,
				FRotator(ExpectedRotationVec.X, ExpectedRotationVec.Y, ExpectedRotationVec.Z),
				ExpectedScale);
			if (!XformError.IsEmpty())
			{
				return FString::Printf(TEXT("%s %s"), *Label, *XformError);
			}
		}

		AActor* Door = FindUniqueLabel(World, GetString(Args, TEXT("door_label"), AccessDoorLabel));
		if (!Door)
		{
			return TEXT("Access Door not found or not unique. ZERO writes.");
		}
		FVector ExpectedDoorLocation;
		if (GetVector(Args, TEXT("expected_door_location"), ExpectedDoorLocation)
			&& !LocationMatches(Door->GetActorLocation(), ExpectedDoorLocation))
		{
			const FVector DoorLoc = Door->GetActorLocation();
			return FString::Printf(
				TEXT("Access Door location (%.2f, %.2f, %.2f) does not match expected (%.2f, %.2f, %.2f). ZERO writes."),
				DoorLoc.X, DoorLoc.Y, DoorLoc.Z,
				ExpectedDoorLocation.X, ExpectedDoorLocation.Y, ExpectedDoorLocation.Z);
		}

		TArray<TSharedPtr<FJsonValue>> BeforeMutations;
		TArray<TSharedPtr<FJsonValue>> ProposedMutations;
		TSet<FString> SeenProps;
		for (const FActorPropertyMutation& Mutation : Mutations)
		{
			if (SeenProps.Contains(Mutation.PropertyName))
			{
				return FString::Printf(TEXT("Duplicate mutation for '%s'."), *Mutation.PropertyName);
			}
			SeenProps.Add(Mutation.PropertyName);
			FProperty* Property = FindInstanceProperty(Actor, Mutation.PropertyName);
			if (!Property)
			{
				return FString::Printf(TEXT("Property '%s' not found on instance."), *Mutation.PropertyName);
			}
			if (!CastField<FNameProperty>(Property) && !CastField<FTextProperty>(Property))
			{
				return FString::Printf(
					TEXT("Property '%s' type '%s' is not supported. set_actor_property currently allows FNameProperty and FTextProperty only."),
					*Mutation.PropertyName, *Property->GetClass()->GetName());
			}
			const FString Live = ReadNameOrTextValue(Actor, Property);
			if (!Live.Equals(Mutation.Expected, ESearchCase::CaseSensitive))
			{
				return FString::Printf(
					TEXT("Property '%s' live '%s' does not match expected '%s'. ZERO writes."),
					*Mutation.PropertyName, *Live, *Mutation.Expected);
			}

			TSharedRef<FJsonObject> BeforeRow = MakeShared<FJsonObject>();
			BeforeRow->SetStringField(TEXT("property"), Mutation.PropertyName);
			BeforeRow->SetStringField(TEXT("property_class"), Property->GetClass()->GetName());
			BeforeRow->SetStringField(TEXT("value"), Live);
			BeforeMutations.Add(MakeShared<FJsonValueObject>(BeforeRow));

			TSharedRef<FJsonObject> ProposedRow = MakeShared<FJsonObject>();
			ProposedRow->SetStringField(TEXT("property"), Mutation.PropertyName);
			ProposedRow->SetStringField(TEXT("expected"), Mutation.Expected);
			ProposedRow->SetStringField(TEXT("value"), Mutation.NewValue);
			ProposedMutations.Add(MakeShared<FJsonValueObject>(ProposedRow));
		}

		Before->SetStringField(TEXT("owning_package"), Owner);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		Before->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		Before->SetArrayField(TEXT("mutations"), BeforeMutations);
		Proposed->SetArrayField(TEXT("mutations"), ProposedMutations);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Set instance FName/FText properties on the game thread. Maps not saved. Blueprint not compiled."));
		return TEXT("");
	}

	UEdGraph* FindFunctionGraph(UBlueprint* Blueprint, const FString& GraphName)
	{
		if (!Blueprint || GraphName.IsEmpty())
		{
			return nullptr;
		}
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}
		return nullptr;
	}

	bool PinNamesMatch(const UEdGraphPin* Pin, const FString& Wanted)
	{
		if (!Pin || Wanted.IsEmpty())
		{
			return false;
		}
		const FString Name = Pin->PinName.ToString();
		const FString Friendly = Pin->PinFriendlyName.ToString();
		return Name.Equals(Wanted, ESearchCase::IgnoreCase)
			|| Friendly.Equals(Wanted, ESearchCase::IgnoreCase);
	}

	UEdGraphPin* FindNodePin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
	{
		if (!Node)
		{
			return nullptr;
		}
		UEdGraphPin* Fallback = nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != Direction)
			{
				continue;
			}
			if (PinNamesMatch(Pin, PinName))
			{
				return Pin;
			}
			if (PinName.Equals(TEXT("then"), ESearchCase::IgnoreCase)
				&& Pin->PinName == UEdGraphSchema_K2::PN_Then)
			{
				return Pin;
			}
			if (PinName.Equals(TEXT("execute"), ESearchCase::IgnoreCase)
				&& Pin->PinName == UEdGraphSchema_K2::PN_Execute)
			{
				return Pin;
			}
			if (PinName.Equals(TEXT("ReturnValue"), ESearchCase::IgnoreCase)
				&& Pin->PinName == UEdGraphSchema_K2::PN_ReturnValue)
			{
				Fallback = Pin;
			}
		}
		return Fallback;
	}

	bool PinsAlreadyLinked(const UEdGraphPin* A, const UEdGraphPin* B)
	{
		if (!A || !B)
		{
			return false;
		}
		for (const UEdGraphPin* Linked : A->LinkedTo)
		{
			if (Linked == B)
			{
				return true;
			}
		}
		return false;
	}

	UEdGraphNode* ResolveGraphNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Selector, FString& OutError)
	{
		if (!Graph || !Selector.IsValid())
		{
			OutError = TEXT("Node selector is required.");
			return nullptr;
		}
		const FString Name = GetString(Selector, TEXT("name"), GetString(Selector, TEXT("node")));
		const FString ClassName = GetString(Selector, TEXT("class"));
		const FString FunctionName = GetString(Selector, TEXT("function"));
		const FString TitleContains = GetString(Selector, TEXT("title"));
		TArray<UEdGraphNode*> Matches;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}
			if (!Name.IsEmpty() && !Node->GetName().Equals(Name, ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FString NodeClass = Node->GetClass() ? Node->GetClass()->GetName() : FString();
			if (!ClassName.IsEmpty() && !NodeClass.Contains(ClassName))
			{
				continue;
			}
			if (!FunctionName.IsEmpty())
			{
				UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
				if (!Call || !Call->GetFunctionName().ToString().Equals(FunctionName, ESearchCase::IgnoreCase))
				{
					continue;
				}
			}
			if (!TitleContains.IsEmpty() && !Node->GetNodeTitle(ENodeTitleType::ListView).ToString().Contains(TitleContains))
			{
				continue;
			}
			Matches.Add(Node);
		}
		if (Matches.Num() == 0)
		{
			OutError = TEXT("No node matched the selector.");
			return nullptr;
		}
		if (Matches.Num() != 1)
		{
			OutError = FString::Printf(TEXT("Node selector matched %d nodes; need exactly 1."), Matches.Num());
			return nullptr;
		}
		return Matches[0];
	}

	TSharedRef<FJsonObject> PinLinkSnapshot(UEdGraphPin* Pin)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetStringField(TEXT("node"), Pin && Pin->GetOwningNode() ? Pin->GetOwningNode()->GetName() : TEXT(""));
		Out->SetStringField(TEXT("pin"), Pin ? Pin->PinName.ToString() : TEXT(""));
		TArray<TSharedPtr<FJsonValue>> Links;
		if (Pin)
		{
			for (UEdGraphPin* Linked : Pin->LinkedTo)
			{
				if (!Linked || !Linked->GetOwningNode())
				{
					continue;
				}
				TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("node"), Linked->GetOwningNode()->GetName());
				Row->SetStringField(TEXT("pin"), Linked->PinName.ToString());
				Links.Add(MakeShared<FJsonValueObject>(Row));
			}
		}
		Out->SetArrayField(TEXT("linked_to"), Links);
		return Out;
	}

	FString PreflightConnectBlueprintPins(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. connect_blueprint_pins does not save.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. Compile with a separate compile_blueprint change.");
		}

		const FString Path = GetString(Args, TEXT("path"));
		const FString GraphName = GetString(Args, TEXT("graph"));
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"))));
		if (Path.IsEmpty())
		{
			return TEXT("path is required.");
		}
		if (GraphName.IsEmpty())
		{
			return TEXT("graph is required (function graph name).");
		}
		if (GraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase)
			|| GraphName.Contains(TEXT("Ubergraph")))
		{
			return TEXT("connect_blueprint_pins cannot target EventGraph.");
		}
		if (RequiredPackage.IsEmpty())
		{
			return TEXT("required_package is required.");
		}
		if (Path.Contains(TEXT("BP_AdminAccessDoor"))
			|| PackagesEqual(NormalizePackage(Path), AccessDoorBpPackage))
		{
			return TEXT("connect_blueprint_pins cannot target BP_AdminAccessDoor.");
		}

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
		if (!Blueprint)
		{
			return TEXT("Blueprint not found.");
		}
		const FString BpPackage = NormalizePackage(Blueprint->GetOutermost()->GetName());
		if (!PackagesEqual(BpPackage, RequiredPackage))
		{
			return TEXT("Blueprint package does not match required_package.");
		}
		UEdGraph* Graph = FindFunctionGraph(Blueprint, GraphName);
		if (!Graph)
		{
			return TEXT("Function graph not found. EventGraph is not eligible.");
		}

		const TArray<TSharedPtr<FJsonValue>>* Connections = nullptr;
		if (!Args.IsValid() || !Args->TryGetArrayField(TEXT("connections"), Connections) || !Connections || Connections->Num() == 0)
		{
			return TEXT("connections array is required.");
		}

		TArray<TSharedPtr<FJsonValue>> BeforeLinks;
		TArray<TSharedPtr<FJsonValue>> ProposedLinks;
		for (const TSharedPtr<FJsonValue>& Value : *Connections)
		{
			const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
			if (!Row.IsValid())
			{
				return TEXT("Each connection must be an object.");
			}
			TSharedPtr<FJsonObject> FromSel = Row;
			TSharedPtr<FJsonObject> ToSel = Row;
			const TSharedPtr<FJsonObject>* FromObj = nullptr;
			const TSharedPtr<FJsonObject>* ToObj = nullptr;
			if (Row->TryGetObjectField(TEXT("from"), FromObj) && FromObj)
			{
				FromSel = *FromObj;
			}
			if (Row->TryGetObjectField(TEXT("to"), ToObj) && ToObj)
			{
				ToSel = *ToObj;
			}
			if (!Row->HasField(TEXT("from")))
			{
				FromSel = MakeShared<FJsonObject>();
				FromSel->SetStringField(TEXT("name"), GetString(Row, TEXT("from_node")));
				FromSel->SetStringField(TEXT("class"), GetString(Row, TEXT("from_class")));
				FromSel->SetStringField(TEXT("function"), GetString(Row, TEXT("from_function")));
			}
			if (!Row->HasField(TEXT("to")))
			{
				ToSel = MakeShared<FJsonObject>();
				ToSel->SetStringField(TEXT("name"), GetString(Row, TEXT("to_node")));
				ToSel->SetStringField(TEXT("class"), GetString(Row, TEXT("to_class")));
				ToSel->SetStringField(TEXT("function"), GetString(Row, TEXT("to_function")));
			}
			const FString FromPinName = GetString(FromSel, TEXT("pin"), GetString(Row, TEXT("from_pin")));
			const FString ToPinName = GetString(ToSel, TEXT("pin"), GetString(Row, TEXT("to_pin")));
			if (FromPinName.IsEmpty() || ToPinName.IsEmpty())
			{
				return TEXT("from_pin and to_pin are required.");
			}

			FString FromErr;
			FString ToErr;
			UEdGraphNode* FromNode = ResolveGraphNode(Graph, FromSel, FromErr);
			UEdGraphNode* ToNode = ResolveGraphNode(Graph, ToSel, ToErr);
			if (!FromNode)
			{
				return FString::Printf(TEXT("from node: %s"), *FromErr);
			}
			if (!ToNode)
			{
				return FString::Printf(TEXT("to node: %s"), *ToErr);
			}
			UEdGraphPin* FromPin = FindNodePin(FromNode, FromPinName, EGPD_Output);
			UEdGraphPin* ToPin = FindNodePin(ToNode, ToPinName, EGPD_Input);
			if (!FromPin)
			{
				return FString::Printf(TEXT("Output pin '%s' not found on %s."), *FromPinName, *FromNode->GetName());
			}
			if (!ToPin)
			{
				return FString::Printf(TEXT("Input pin '%s' not found on %s."), *ToPinName, *ToNode->GetName());
			}
			if (ToPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec
				&& ToPin->LinkedTo.Num() > 0
				&& !PinsAlreadyLinked(FromPin, ToPin))
			{
				return FString::Printf(
					TEXT("Dest exec pin %s.%s is already connected. Refusing to break existing links."),
					*ToNode->GetName(), *ToPin->PinName.ToString());
			}
			if (ToPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec
				&& ToPin->LinkedTo.Num() > 0
				&& !PinsAlreadyLinked(FromPin, ToPin))
			{
				return FString::Printf(
					TEXT("Dest data pin %s.%s is already connected. Refusing to break existing links."),
					*ToNode->GetName(), *ToPin->PinName.ToString());
			}

			BeforeLinks.Add(MakeShared<FJsonValueObject>(PinLinkSnapshot(FromPin)));
			BeforeLinks.Add(MakeShared<FJsonValueObject>(PinLinkSnapshot(ToPin)));
			TSharedRef<FJsonObject> ProposedRow = MakeShared<FJsonObject>();
			ProposedRow->SetStringField(TEXT("from_node"), FromNode->GetName());
			ProposedRow->SetStringField(TEXT("from_pin"), FromPin->PinName.ToString());
			ProposedRow->SetStringField(TEXT("to_node"), ToNode->GetName());
			ProposedRow->SetStringField(TEXT("to_pin"), ToPin->PinName.ToString());
			ProposedRow->SetBoolField(TEXT("already_linked"), PinsAlreadyLinked(FromPin, ToPin));
			ProposedLinks.Add(MakeShared<FJsonValueObject>(ProposedRow));
		}

		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("graph"), Graph->GetName());
		Before->SetArrayField(TEXT("pins"), BeforeLinks);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetArrayField(TEXT("connections"), ProposedLinks);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("spawns_nodes"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Create the listed pin links on one function graph. Does not compile or save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConnectBlueprintPins(FBridgeChange& Change)
	{
		if (GetPieWorld())
		{
			return FailAudit(TEXT("pie_running"), TEXT("PIE is running. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const FString Path = GetString(Change.Args, TEXT("path"));
		const FString GraphName = GetString(Change.Args, TEXT("graph"));
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
		if (!Blueprint)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(Blueprint->GetOutermost()->GetName(), Change.Package))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Blueprint package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		UEdGraph* Graph = FindFunctionGraph(Blueprint, GraphName);
		if (!Graph)
		{
			return FailAudit(TEXT("graph_not_found"), TEXT("Function graph not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> ReBefore = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> ReProposed = MakeShared<FJsonObject>();
		const FString Replay = PreflightConnectBlueprintPins(Change.Args, ReBefore, ReProposed);
		if (!Replay.IsEmpty())
		{
			return FailAudit(TEXT("preflight_replay_failed"), Replay, MakeShared<FBridgeChange>(Change));
		}

		const UEdGraphSchema* Schema = Graph->GetSchema();
		if (!Schema)
		{
			return FailAudit(TEXT("no_schema"), TEXT("Graph schema missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const TArray<TSharedPtr<FJsonValue>>* ProposedConnections = nullptr;
		ReProposed->TryGetArrayField(TEXT("connections"), ProposedConnections);
		TArray<TSharedPtr<FJsonValue>> AfterLinks;
		bool bAnyNew = false;
		if (ProposedConnections)
		{
			for (const TSharedPtr<FJsonValue>& Value : *ProposedConnections)
			{
				const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
				if (!Row.IsValid())
				{
					continue;
				}
				const FString FromNodeName = GetString(Row, TEXT("from_node"));
				const FString FromPinName = GetString(Row, TEXT("from_pin"));
				const FString ToNodeName = GetString(Row, TEXT("to_node"));
				const FString ToPinName = GetString(Row, TEXT("to_pin"));
				UEdGraphNode* FromNode = nullptr;
				UEdGraphNode* ToNode = nullptr;
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (!Node)
					{
						continue;
					}
					if (Node->GetName().Equals(FromNodeName, ESearchCase::IgnoreCase))
					{
						FromNode = Node;
					}
					if (Node->GetName().Equals(ToNodeName, ESearchCase::IgnoreCase))
					{
						ToNode = Node;
					}
				}
				UEdGraphPin* FromPin = FindNodePin(FromNode, FromPinName, EGPD_Output);
				UEdGraphPin* ToPin = FindNodePin(ToNode, ToPinName, EGPD_Input);
				if (!FromPin || !ToPin)
				{
					return FailAudit(TEXT("pin_missing"), TEXT("Resolved pin vanished before connect. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
				}
				if (!PinsAlreadyLinked(FromPin, ToPin))
				{
					if (!Schema->TryCreateConnection(FromPin, ToPin))
					{
						return FailAudit(
							TEXT("connect_failed"),
							FString::Printf(TEXT("TryCreateConnection failed for %s.%s -> %s.%s. Partial links may exist."),
								*FromNodeName, *FromPinName, *ToNodeName, *ToPinName),
							MakeShared<FBridgeChange>(Change));
					}
					bAnyNew = true;
				}
				AfterLinks.Add(MakeShared<FJsonValueObject>(PinLinkSnapshot(FromPin)));
				AfterLinks.Add(MakeShared<FJsonValueObject>(PinLinkSnapshot(ToPin)));
			}
		}

		if (bAnyNew)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			Graph->NotifyGraphChanged();
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Change.After->SetStringField(TEXT("graph"), Graph->GetName());
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetBoolField(TEXT("made_new_links"), bAnyNew);
		Change.After->SetArrayField(TEXT("pins"), AfterLinks);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString NormalizeBlueprintPath(const FString& Path)
	{
		FString Clean = Path;
		Clean.ReplaceInline(TEXT("\\"), TEXT("/"));
		int32 DotIndex = INDEX_NONE;
		if (Clean.FindChar(TEXT('.'), DotIndex))
		{
			Clean.LeftInline(DotIndex, EAllowShrinking::No);
		}
		return Clean;
	}

	ULevel* FindLoadedLevelByPackage(UWorld* World, const FString& PackageName)
	{
		if (!World)
		{
			return nullptr;
		}
		if (PackagesEqual(WorldPackageName(World), PackageName) && World->PersistentLevel)
		{
			return World->PersistentLevel;
		}
		if (ULevelStreaming* Streaming = FindStreamingLevel(World, PackageName))
		{
			if (Streaming->IsLevelLoaded())
			{
				return Streaming->GetLoadedLevel();
			}
		}
		for (ULevel* Level : World->GetLevels())
		{
			if (Level && Level->GetOutermost() && PackagesEqual(Level->GetOutermost()->GetName(), PackageName))
			{
				return Level;
			}
		}
		return nullptr;
	}

	FString JsonValueDebug(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid() || Value->IsNull())
		{
			return TEXT("null");
		}
		switch (Value->Type)
		{
		case EJson::Boolean: return Value->AsBool() ? TEXT("true") : TEXT("false");
		case EJson::Number: return FString::SanitizeFloat(static_cast<float>(Value->AsNumber()));
		case EJson::String: return Value->AsString();
		default: return TEXT("<unsupported>");
		}
	}

	bool ResolveEnumValue(UEnum* Enum, const TSharedPtr<FJsonValue>& Value, int64& OutValue, FString& OutError)
	{
		if (!Enum)
		{
			OutError = TEXT("Enum asset is null.");
			return false;
		}
		if (!Value.IsValid() || Value->IsNull())
		{
			OutError = TEXT("Enum value is required.");
			return false;
		}
		if (Value->Type == EJson::Number)
		{
			OutValue = static_cast<int64>(Value->AsNumber());
			if (Enum->IsValidEnumValue(OutValue))
			{
				return true;
			}
			OutError = FString::Printf(TEXT("Enum numeric %lld is not valid."), OutValue);
			return false;
		}
		if (Value->Type != EJson::String)
		{
			OutError = TEXT("Enum value must be a string or number.");
			return false;
		}
		const FString Token = Value->AsString();
		if (Token.IsNumeric())
		{
			OutValue = FCString::Atoi64(*Token);
			if (Enum->IsValidEnumValue(OutValue))
			{
				return true;
			}
		}
		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			if (Enum->HasMetaData(TEXT("Hidden"), Index))
			{
				continue;
			}
			if (Enum->GetNameStringByIndex(Index).Equals(Token, ESearchCase::IgnoreCase)
				|| Enum->GetDisplayNameTextByIndex(Index).ToString().Equals(Token, ESearchCase::IgnoreCase))
			{
				OutValue = Enum->GetValueByIndex(Index);
				return true;
			}
		}
		OutError = FString::Printf(TEXT("Enum token '%s' did not match a visible entry."), *Token);
		return false;
	}

	UEnum* EnumFromProperty(FProperty* Property)
	{
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			return ByteProp->GetIntPropertyEnum();
		}
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			return EnumProp->GetEnum();
		}
		return nullptr;
	}

	TSharedRef<FJsonObject> ReadPropertySnapshot(UObject* Object, FProperty* Property)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Object || !Property)
		{
			Out->SetField(TEXT("value"), MakeShared<FJsonValueNull>());
			return Out;
		}
		Out->SetStringField(TEXT("property"), Property->GetName());
		Out->SetStringField(TEXT("property_class"), Property->GetClass()->GetName());
		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			Out->SetBoolField(TEXT("value"), BoolProp->GetPropertyValue_InContainer(Object));
		}
		else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			Out->SetStringField(TEXT("value"), NameProp->GetPropertyValue_InContainer(Object).ToString());
		}
		else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			Out->SetStringField(TEXT("value"), TextProp->GetPropertyValue_InContainer(Object).ToString());
		}
		else if (const FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			if (NumProp->IsFloatingPoint())
			{
				Out->SetNumberField(TEXT("value"), NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object)));
			}
			else
			{
				Out->SetNumberField(TEXT("value"), static_cast<double>(NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object))));
			}
		}
		if (UEnum* Enum = EnumFromProperty(Property))
		{
			int64 EnumValue = INDEX_NONE;
			if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
			{
				EnumValue = ByteProp->GetPropertyValue_InContainer(Object);
			}
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
			{
				EnumValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Object));
			}
			Out->SetNumberField(TEXT("numeric_value"), static_cast<double>(EnumValue));
			Out->SetStringField(TEXT("internal_name"), Enum->GetNameStringByValue(EnumValue));
			Out->SetStringField(TEXT("display_name"), Enum->GetDisplayNameTextByValue(EnumValue).ToString());
		}
		return Out;
	}

	bool SetPropertyFromJson(UObject* Object, FProperty* Property, const TSharedPtr<FJsonValue>& Value, FString& OutError)
	{
		if (!Object || !Property || !Value.IsValid() || Value->IsNull())
		{
			OutError = TEXT("Object, property, or value is missing.");
			return false;
		}
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool bDesired = false;
			if (Value->Type == EJson::Boolean)
			{
				bDesired = Value->AsBool();
			}
			else if (Value->Type == EJson::String)
			{
				bDesired = Value->AsString().Equals(TEXT("true"), ESearchCase::IgnoreCase)
					|| Value->AsString() == TEXT("1");
			}
			else if (Value->Type == EJson::Number)
			{
				bDesired = Value->AsNumber() != 0.0;
			}
			else
			{
				OutError = FString::Printf(TEXT("Property '%s' requires a bool."), *Property->GetName());
				return false;
			}
			BoolProp->SetPropertyValue_InContainer(Object, bDesired);
			return true;
		}
		if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			if (Value->Type != EJson::String)
			{
				OutError = FString::Printf(TEXT("Property '%s' requires a string."), *Property->GetName());
				return false;
			}
			NameProp->SetPropertyValue_InContainer(Object, FName(*Value->AsString()));
			return true;
		}
		if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			if (Value->Type != EJson::String)
			{
				OutError = FString::Printf(TEXT("Property '%s' requires a string."), *Property->GetName());
				return false;
			}
			TextProp->SetPropertyValue_InContainer(Object, FText::FromString(Value->AsString()));
			return true;
		}
		if (UEnum* Enum = EnumFromProperty(Property))
		{
			int64 EnumValue = INDEX_NONE;
			if (!ResolveEnumValue(Enum, Value, EnumValue, OutError))
			{
				return false;
			}
			if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
			{
				ByteProp->SetPropertyValue_InContainer(Object, static_cast<uint8>(EnumValue));
				return true;
			}
			if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Object), EnumValue);
				return true;
			}
		}
		if (FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			if (Value->Type != EJson::Number && !(Value->Type == EJson::String && Value->AsString().IsNumeric()))
			{
				OutError = FString::Printf(TEXT("Property '%s' requires a number."), *Property->GetName());
				return false;
			}
			const double Number = Value->Type == EJson::Number ? Value->AsNumber() : FCString::Atod(*Value->AsString());
			if (NumProp->IsFloatingPoint())
			{
				NumProp->SetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object), Number);
			}
			else
			{
				NumProp->SetIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object), static_cast<int64>(Number));
			}
			return true;
		}
		OutError = FString::Printf(
			TEXT("Property '%s' type '%s' is not supported for spawn config."),
			*Property->GetName(),
			*Property->GetClass()->GetName());
		return false;
	}

	bool PropertyMatchesJson(UObject* Object, FProperty* Property, const TSharedPtr<FJsonValue>& Expected, FString& OutError)
	{
		if (!Object || !Property)
		{
			OutError = TEXT("Object or property is missing during verify.");
			return false;
		}
		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool bExpected = false;
			if (Expected.IsValid() && Expected->Type == EJson::Boolean)
			{
				bExpected = Expected->AsBool();
			}
			else if (Expected.IsValid() && Expected->Type == EJson::String)
			{
				bExpected = Expected->AsString().Equals(TEXT("true"), ESearchCase::IgnoreCase);
			}
			const bool bLive = BoolProp->GetPropertyValue_InContainer(Object);
			if (bLive != bExpected)
			{
				OutError = FString::Printf(TEXT("%s live %s expected %s"),
					*Property->GetName(), bLive ? TEXT("true") : TEXT("false"), bExpected ? TEXT("true") : TEXT("false"));
				return false;
			}
			return true;
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			const FString Live = NameProp->GetPropertyValue_InContainer(Object).ToString();
			const FString Wanted = Expected.IsValid() ? Expected->AsString() : FString();
			if (!Live.Equals(Wanted, ESearchCase::CaseSensitive))
			{
				OutError = FString::Printf(TEXT("%s live '%s' expected '%s'"), *Property->GetName(), *Live, *Wanted);
				return false;
			}
			return true;
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			const FString Live = TextProp->GetPropertyValue_InContainer(Object).ToString();
			const FString Wanted = Expected.IsValid() ? Expected->AsString() : FString();
			if (!Live.Equals(Wanted, ESearchCase::CaseSensitive))
			{
				OutError = FString::Printf(TEXT("%s live '%s' expected '%s'"), *Property->GetName(), *Live, *Wanted);
				return false;
			}
			return true;
		}
		if (UEnum* Enum = EnumFromProperty(Property))
		{
			int64 ExpectedValue = INDEX_NONE;
			if (!ResolveEnumValue(Enum, Expected, ExpectedValue, OutError))
			{
				return false;
			}
			int64 LiveValue = INDEX_NONE;
			if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
			{
				LiveValue = ByteProp->GetPropertyValue_InContainer(Object);
			}
			else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
			{
				LiveValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Object));
			}
			if (LiveValue != ExpectedValue)
			{
				OutError = FString::Printf(TEXT("%s live %lld expected %lld"), *Property->GetName(), LiveValue, ExpectedValue);
				return false;
			}
			return true;
		}
		if (const FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			const double Wanted = (Expected.IsValid() && Expected->Type == EJson::Number)
				? Expected->AsNumber()
				: (Expected.IsValid() && Expected->Type == EJson::String ? FCString::Atod(*Expected->AsString()) : 0.0);
			const double Live = NumProp->IsFloatingPoint()
				? NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object))
				: static_cast<double>(NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Object)));
			if (!FMath::IsNearlyEqual(Live, Wanted, 0.01))
			{
				OutError = FString::Printf(TEXT("%s live %f expected %f"), *Property->GetName(), Live, Wanted);
				return false;
			}
			return true;
		}
		OutError = FString::Printf(TEXT("Cannot verify property '%s'."), *Property->GetName());
		return false;
	}

	FString GuardExistingActor(
		UWorld* World,
		const TCHAR* Label,
		const FVector& ExpectedLocation,
		const TCHAR* ExpectedPackage)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1"), Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (ExpectedPackage && !PackagesEqual(ActorOwningPackage(Actor), ExpectedPackage))
		{
			return FString::Printf(TEXT("%s owner '%s' expected '%s'"), Label, *ActorOwningPackage(Actor), ExpectedPackage);
		}
		if (!LocationMatches(Actor->GetActorLocation(), ExpectedLocation))
		{
			const FVector Loc = Actor->GetActorLocation();
			return FString::Printf(
				TEXT("%s location (%.2f, %.2f, %.2f) expected (%.2f, %.2f, %.2f)"),
				Label, Loc.X, Loc.Y, Loc.Z, ExpectedLocation.X, ExpectedLocation.Y, ExpectedLocation.Z);
		}
		return TEXT("");
	}

#include "OrganoidAIBridgeHologram.inl"
#include "OrganoidAIBridgeLighting.inl"
#include "OrganoidAIBridgeAccessDoor.inl"
#include "OrganoidAIBridgeAudio.inl"
#include "OrganoidAIBridgeNavMesh.inl"
#include "OrganoidAIBridgeResearchStation.inl"
#include "OrganoidAIBridgeResearchWingKeycard.inl"
#include "OrganoidAIBridgeTraversal.inl"
#include "OrganoidAIBridgeOpeningBlock2.inl"
#include "OrganoidAIBridgeOpeningBlock3.inl"
#include "OrganoidAIBridgeOpeningBlock4.inl"
#include "OrganoidAIBridgeOpeningBlock4NavMesh.inl"
#include "OrganoidAIBridgeNeuroArrivalLab.inl"
#include "OrganoidAIBridgeNeuroCh3Evidence.inl"
#include "OrganoidAIBridgeNeuroCh4Personnel.inl"
#include "OrganoidAIBridgeNeuroPowerFailureDiscovery.inl"
#include "OrganoidAIBridgeNeuroPowerDiagnosis.inl"
#include "OrganoidAIBridgeNeuroGeneticsMission.inl"
#include "OrganoidAIBridgeNeuroGeneticsMissionBeat3.inl"
#include "OrganoidAIBridgeNeuroGeneticsMissionBeat4.inl"
#include "OrganoidAIBridgeNeuroGeneticsMissionBeat5.inl"
#include "OrganoidAIBridgeNeuroGeneticsMissionBeat6.inl"
#include "OrganoidAIBridgeNeuroPowerRestoreMission.inl"
#include "OrganoidAIBridgeNeuroGeneticsNextPowerRestore.inl"
#include "OrganoidAIBridgeNeuroBackupPowerRestore.inl"
#include "OrganoidAIBridgeNeuroTargetingWhyMission.inl"
#include "OrganoidAIBridgeNeuroPowerRestoreNextTargetingWhy.inl"
#include "OrganoidAIBridgeNeuroResearcherTargetingWhy.inl"
#include "OrganoidAIBridgeNeuroResearchStationIntroMission.inl"
#include "OrganoidAIBridgeNeuroTargetingWhyNextResearchStation.inl"
#include "OrganoidAIBridgeNeuroResearchStationIntro.inl"
#include "OrganoidAIBridgeNeuralSlowAdaptationAsset.inl"
#include "OrganoidAIBridgeNeuroNeuralSlowUseMission.inl"
#include "OrganoidAIBridgeNeuroResearchStationIntroNextNeuralSlow.inl"
#include "OrganoidAIBridgeNeuroAdaptationConnectionMission.inl"
#include "OrganoidAIBridgeNeuroRevelationMission.inl"
#include "OrganoidAIBridgeNeuroAdaptationConnectionNextRevelation.inl"
#include "OrganoidAIBridgeNeuroNeuralSlowUseNextAdaptationConnection.inl"
#include "OrganoidAIBridgeNeuroAdaptationSubject.inl"
#include "OrganoidAIBridgeNeuroNeuralMappingArray.inl"
#include "OrganoidAIBridgeNeuroResearchLoadCutoff.inl"
#include "OrganoidAIBridgeNeuroNeuralMappingTerminal.inl"
#include "OrganoidAIBridgeNeuroNeuralSignatureObservationNode.inl"
#include "OrganoidAIBridgeNeuroRevelationObservation.inl"
#include "OrganoidAIBridgeCryoAccessMission.inl"
#include "OrganoidAIBridgeNeuroRevelationNextCryoAccess.inl"
#include "OrganoidAIBridgeCryoBackupPowerPanel.inl"
#include "OrganoidAIBridgeCryoEntryMission.inl"
#include "OrganoidAIBridgeCryoAccessNextCryoEntry.inl"
#include "OrganoidAIBridgeCryoEntryCheckpoint.inl"
#include "OrganoidAIBridgeCryoEvidenceMission.inl"
#include "OrganoidAIBridgeCryoEntryNextCryoEvidence.inl"
#include "OrganoidAIBridgeCryoEvidenceDatapads.inl"
#include "OrganoidAIBridgeComputeEntryMission.inl"
#include "OrganoidAIBridgeCryoEvidenceNextComputeEntry.inl"
#include "OrganoidAIBridgeComputeEntryCheckpoint.inl"
#include "OrganoidAIBridgeComputeHandoverMission.inl"
#include "OrganoidAIBridgeComputeEntryNextComputeHandover.inl"
#include "OrganoidAIBridgeComputeHandoverTerminals.inl"
#include "OrganoidAIBridgeTheConclusionMission.inl"
#include "OrganoidAIBridgeComputeHandoverNextTheConclusion.inl"
#include "OrganoidAIBridgeTheConclusionTerminal.inl"
#include "OrganoidAIBridgeResearchStationMission.inl"
#include "OrganoidAIBridgeTheConclusionNextResearchStation.inl"
#include "OrganoidAIBridgeResearchStationConfigure.inl"
#include "OrganoidAIBridgeSyringeKitAdaptations.inl"
#include "OrganoidAIBridgeSyringeKitMission.inl"
#include "OrganoidAIBridgeResearchStationNextSyringeKit.inl"
#include "OrganoidAIBridgeResearchStationSyringeKit.inl"
#include "OrganoidAIBridgeNathanGrantLook.inl"
#include "OrganoidAIBridgeNeuroNeuralChangeEvidenceInstrument.inl"
#include "OrganoidAIBridgeNeuroLiveAdaptationConnection.inl"

	FString PreflightSpawnBlueprintActor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_blueprint_actor does not save. No Save All.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false. spawn_blueprint_actor does not compile.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_blueprint_actor preflight must run on the game thread.");
		}

		const FString BlueprintPath = NormalizeBlueprintPath(
			GetString(Args, TEXT("blueprint"), GetString(Args, TEXT("class_path"), GetString(Args, TEXT("path")))));
		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("level_package"), GetString(Args, TEXT("required_package")))));
		const FString Label = GetString(Args, TEXT("label"), GetString(Args, TEXT("actor")));
		FVector Location = FVector::ZeroVector;
		FVector RotationVec = FVector::ZeroVector;
		FVector Scale = FVector::OneVector;
		if (!GetVector(Args, TEXT("location"), Location))
		{
			return TEXT("explicit location [x,y,z] is required.");
		}
		GetVector(Args, TEXT("rotation"), RotationVec);
		if (!GetVector(Args, TEXT("scale"), Scale))
		{
			Scale = FVector::OneVector;
		}

		if (BlueprintPath.IsEmpty())
		{
			return TEXT("explicit Blueprint class path is required.");
		}
		if (Destination.IsEmpty())
		{
			return TEXT("explicit destination_package / required_package is required.");
		}
		if (Label.IsEmpty())
		{
			return TEXT("explicit actor label is required.");
		}
		if (IsHologramSpawnArgs(Args))
		{
			return PreflightSpawnHologramActor(Args, Before, Proposed);
		}
		if (IsLightControllerSpawnArgs(Args))
		{
			return PreflightSpawnLightControllerActor(Args, Before, Proposed);
		}
		if (!PackagesEqual(BlueprintPath, AdminTerminalBpPath) && !PackagesEqual(BlueprintPath, AdminTerminalBpPackage))
		{
			return TEXT("spawn_blueprint_actor allows only BP_AdminTerminal, BP_AdminFacilityHologram, or BP_AdminLightController.");
		}
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("First spawn_blueprint_actor implementation only targets /Game/Maps/Epitope/SL_Epitope_Admin.");
		}
		if (BlueprintPath.Contains(TEXT("BP_AdminAccessDoor")) || Label.Contains(TEXT("AdminAccessDoor")))
		{
			return TEXT("spawn_blueprint_actor refuses BP_AdminAccessDoor.");
		}
		if (Label.Equals(ReceptionLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("spawn_blueprint_actor will not touch Reception.");
		}
		if (Label.Equals(LegacySecurityLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("Refusing label Terminal_AdminSecurity. That is the legacy C++ hacking terminal.");
		}
		if (Label.Equals(SecurityTerminalLabel, ESearchCase::CaseSensitive))
		{
			if (!LocationMatches(Location, SecurityTerminalLocation)
				|| !RotationMatches(FRotator(RotationVec.X, RotationVec.Y, RotationVec.Z), SecurityTerminalRotation)
				|| !ScaleMatches(Scale, SecurityTerminalScale))
			{
				return TEXT("Admin_Terminal_Security must use location (2680,-400,110), rotation (0,0,0), scale (1,1,1).");
			}
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		ULevel* TargetLevel = FindLoadedLevelByPackage(World, Destination);
		if (!TargetLevel)
		{
			return TEXT("Target level is not loaded. ZERO writes.");
		}

		TArray<AActor*> Existing = FindByExactLabel(World, Label);
		if (Existing.Num() != 0)
		{
			return FString::Printf(TEXT("Label '%s' already exists (count=%d). Fail closed."), *Label, Existing.Num());
		}

		FString GuardError = GuardExistingActor(World, ReceptionLabel, ReceptionLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception guard failed: %s"), *GuardError);
		}
		AActor* Reception = FindByExactLabel(World, ReceptionLabel)[0];
		GuardError = ReceptionConfigMismatch(Reception);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception config changed; refusing spawn. %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, SecurityTriggerLabel, SecurityTriggerLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Security trigger guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, LegacySecurityLabel, LegacySecurityLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Legacy Terminal_AdminSecurity guard failed: %s"), *GuardError);
		}

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, *FString::Printf(TEXT("%s.%s"), *BlueprintPath, *FPaths::GetBaseFilename(BlueprintPath)));
		}
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return TEXT("BP_AdminTerminal Blueprint or generated class not found.");
		}
		if (!Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
		{
			return TEXT("Blueprint generated class is not an Actor.");
		}
		if (Blueprint->GeneratedClass->GetName().Contains(TEXT("AdminAccessDoor")))
		{
			return TEXT("spawn_blueprint_actor refuses BP_AdminAccessDoor.");
		}

		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		const TSharedPtr<FJsonObject>* PropertiesObj = nullptr;
		if (!Args.IsValid() || !Args->TryGetObjectField(TEXT("properties"), PropertiesObj) || !PropertiesObj || !(*PropertiesObj).IsValid())
		{
			return TEXT("properties object is required so spawn can configure the instance atomically.");
		}
		TArray<FString> MissingRequired;
		for (const FString& Required : SpawnPropertyAllowlist)
		{
			if (!(*PropertiesObj)->HasField(Required))
			{
				MissingRequired.Add(Required);
			}
		}
		if (MissingRequired.Num() > 0)
		{
			return FString::Printf(TEXT("Missing required spawn properties: %s"), *FString::Join(MissingRequired, TEXT(", ")));
		}
		TArray<TSharedPtr<FJsonValue>> ProposedProps;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PropertiesObj)->Values)
		{
			if (!SpawnPropertyAllowlist.Contains(Pair.Key))
			{
				return FString::Printf(TEXT("Property '%s' is not allowlisted for spawn config."), *Pair.Key);
			}
			FProperty* Property = FindInstanceProperty(CDO, Pair.Key);
			if (!Property)
			{
				return FString::Printf(TEXT("Property '%s' not found on BP_AdminTerminal."), *Pair.Key);
			}
			FString ApplyError;
			if (UEnum* Enum = EnumFromProperty(Property))
			{
				int64 EnumValue = INDEX_NONE;
				if (!ResolveEnumValue(Enum, Pair.Value, EnumValue, ApplyError))
				{
					return FString::Printf(TEXT("%s: %s"), *Pair.Key, *ApplyError);
				}
			}
			else if (!(CastField<FBoolProperty>(Property)
				|| CastField<FNameProperty>(Property)
				|| CastField<FTextProperty>(Property)
				|| CastField<FNumericProperty>(Property)))
			{
				return FString::Printf(TEXT("Property '%s' type is not supported for spawn config."), *Pair.Key);
			}
			if (CastField<FNameProperty>(Property) && Pair.Value.IsValid() && Pair.Value->Type != EJson::String)
			{
				return FString::Printf(TEXT("%s requires a string."), *Pair.Key);
			}
			if (CastField<FTextProperty>(Property) && Pair.Value.IsValid() && Pair.Value->Type != EJson::String)
			{
				return FString::Printf(TEXT("%s requires a string."), *Pair.Key);
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("property"), Pair.Key);
			Row->SetStringField(TEXT("value"), JsonValueDebug(Pair.Value));
			ProposedProps.Add(MakeShared<FJsonValueObject>(Row));
		}

		if (Label.Equals(SecurityTerminalLabel, ESearchCase::CaseSensitive))
		{
			const FString TerminalId = GetString(*PropertiesObj, TEXT("TerminalID"));
			const FString Title = GetString(*PropertiesObj, TEXT("Title"));
			const FString Prompt = GetString(*PropertiesObj, TEXT("InteractionPrompt"));
			if (!TerminalId.Equals(TEXT("Terminal_AdminSecurity")))
			{
				return TEXT("Admin_Terminal_Security TerminalID must be Terminal_AdminSecurity.");
			}
			if (!Title.Equals(TEXT("Security Terminal")))
			{
				return TEXT("Admin_Terminal_Security Title must be Security Terminal.");
			}
			if (!Prompt.Equals(TEXT("Use Terminal")))
			{
				return TEXT("Admin_Terminal_Security InteractionPrompt must be Use Terminal.");
			}
			FProperty* TypeProp = FindInstanceProperty(CDO, TEXT("TerminalType"));
			int64 TypeValue = INDEX_NONE;
			FString TypeErr;
			if (!ResolveEnumValue(EnumFromProperty(TypeProp), (*PropertiesObj)->TryGetField(TEXT("TerminalType")), TypeValue, TypeErr)
				|| TypeValue != 1)
			{
				return FString::Printf(TEXT("Admin_Terminal_Security TerminalType must resolve to Security / NewEnumerator1 / 1 (%s)."), *TypeErr);
			}
			const TSharedPtr<FJsonValue> Powered = (*PropertiesObj)->TryGetField(TEXT("bInitiallyPowered"));
			const TSharedPtr<FJsonValue> OneShot = (*PropertiesObj)->TryGetField(TEXT("bOneShot"));
			if (!Powered.IsValid() || Powered->Type != EJson::Boolean || !Powered->AsBool())
			{
				return TEXT("Admin_Terminal_Security bInitiallyPowered must be true.");
			}
			if (!OneShot.IsValid() || OneShot->Type != EJson::Boolean || OneShot->AsBool())
			{
				return TEXT("Admin_Terminal_Security bOneShot must be false.");
			}
			const double Range = GetNumber(*PropertiesObj, TEXT("InteractionRange"), -1.0);
			if (!FMath::IsNearlyEqual(Range, 150.0, 0.01))
			{
				return TEXT("Admin_Terminal_Security InteractionRange must be 150.");
			}
		}

		Before->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetStringField(TEXT("editor_world"), WorldPackageName(World));
		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("destination_loaded"), true);
		Before->SetNumberField(TEXT("existing_label_count"), 0);
		Before->SetObjectField(TEXT("reception"), ActorSnapshot(Reception));
		Before->SetObjectField(TEXT("reception_properties"), SnapshotReceptionConfig(Reception));
		Before->SetObjectField(TEXT("security_trigger"), ActorSnapshot(FindByExactLabel(World, SecurityTriggerLabel)[0]));
		Before->SetObjectField(TEXT("access_door"), ActorSnapshot(FindByExactLabel(World, AccessDoorLabel)[0]));
		Before->SetObjectField(TEXT("legacy_security_terminal"), ActorSnapshot(FindByExactLabel(World, LegacySecurityLabel)[0]));
		Before->SetBoolField(TEXT("save"), false);
		Before->SetBoolField(TEXT("compile"), false);

		Proposed->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Proposed->SetStringField(TEXT("generated_class"), Blueprint->GeneratedClass->GetPathName());
		Proposed->SetStringField(TEXT("destination_package"), Destination);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetArrayField(TEXT("location"), Vec(Location));
		Proposed->SetArrayField(TEXT("rotation"), Rot(FRotator(RotationVec.X, RotationVec.Y, RotationVec.Z)));
		Proposed->SetArrayField(TEXT("scale"), Vec(Scale));
		Proposed->SetArrayField(TEXT("properties"), ProposedProps);
		Proposed->SetBoolField(TEXT("atomic_instance_config"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn BP_AdminTerminal into loaded SL_Epitope_Admin with instance properties. Does not save or compile. Does not touch Reception, Access Door, or legacy Terminal_AdminSecurity."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnBlueprintActor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_blueprint_actor must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (GetPieWorld())
		{
			return FailAudit(TEXT("pie_running"), TEXT("PIE is running. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> ReBefore = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> ReProposed = MakeShared<FJsonObject>();
		const FString Replay = PreflightSpawnBlueprintActor(Change.Args, ReBefore, ReProposed);
		if (!Replay.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_replay_failed"), FString::Printf(TEXT("ZERO writes. %s"), *Replay), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		const FString BlueprintPath = NormalizeBlueprintPath(
			GetString(Change.Args, TEXT("blueprint"), GetString(Change.Args, TEXT("class_path"), GetString(Change.Args, TEXT("path")))));
		const FString Destination = NormalizePackage(
			GetString(Change.Args, TEXT("destination_package"), GetString(Change.Args, TEXT("level_package"), GetString(Change.Args, TEXT("required_package")))));
		const FString Label = GetString(Change.Args, TEXT("label"), GetString(Change.Args, TEXT("actor")));
		FVector Location = SecurityTerminalLocation;
		FVector RotationVec = FVector::ZeroVector;
		FVector Scale = FVector::OneVector;
		GetVector(Change.Args, TEXT("location"), Location);
		GetVector(Change.Args, TEXT("rotation"), RotationVec);
		GetVector(Change.Args, TEXT("scale"), Scale);
		ULevel* TargetLevel = FindLoadedLevelByPackage(World, Destination);
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, *FString::Printf(TEXT("%s.%s"), *BlueprintPath, *FPaths::GetBaseFilename(BlueprintPath)));
		}
		if (!World || !TargetLevel || !Blueprint || !Blueprint->GeneratedClass)
		{
			return FailAudit(TEXT("not_found"), TEXT("World, target level, or Blueprint vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const TSharedPtr<FJsonObject>* PropertiesObj = nullptr;
		Change.Args->TryGetObjectField(TEXT("properties"), PropertiesObj);

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnBlueprintActor", "Spawn Blueprint Actor"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		Params.bNoFail = false;

		const FRotator Rotation(RotationVec.X, RotationVec.Y, RotationVec.Z);
		AActor* Spawned = World->SpawnActor(Blueprint->GeneratedClass, &Location, &Rotation, Params);
		if (!Spawned)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		auto DestroySpawned = [&]()
		{
			if (!Spawned)
			{
				return;
			}
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				ActorSub->DestroyActor(Spawned);
			}
			else
			{
				World->DestroyActor(Spawned);
			}
			Spawned = nullptr;
		};

		Spawned->SetActorScale3D(Scale);
		Spawned->SetActorLabel(Label, true);
		if (IsLightControllerSpawnArgs(Change.Args))
		{
			Spawned->SetActorEnableCollision(false);
			Spawned->PrimaryActorTick.bCanEverTick = false;
			Spawned->SetActorTickEnabled(false);
		}
		Spawned->Modify();

		if (PropertiesObj && (*PropertiesObj).IsValid())
		{
			FString SetError;
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PropertiesObj)->Values)
			{
				FProperty* Property = FindInstanceProperty(Spawned, Pair.Key);
				if (!SetPropertyFromJson(Spawned, Property, Pair.Value, SetError)
					|| !PropertyMatchesJson(Spawned, Property, Pair.Value, SetError))
				{
					DestroySpawned();
					Change.Status = TEXT("execute_failed");
					return FailAudit(
						TEXT("config_failed"),
						FString::Printf(TEXT("%s Spawned actor destroyed. Maps not saved. Reception untouched."), *SetError),
						MakeShared<FBridgeChange>(Change));
				}
			}
		}

		if (!PackagesEqual(ActorOwningPackage(Spawned), Destination))
		{
			const FString Owner = ActorOwningPackage(Spawned);
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("wrong_package"),
				FString::Printf(TEXT("Spawned owner '%s' is not '%s'. Actor destroyed. Maps not saved."), *Owner, *Destination),
				MakeShared<FBridgeChange>(Change));
		}

		FString GuardError = GuardExistingActor(World, ReceptionLabel, ReceptionLocation, AdminPackage);
		if (GuardError.IsEmpty())
		{
			GuardError = ReceptionConfigMismatch(FindByExactLabel(World, ReceptionLabel)[0]);
		}
		if (GuardError.IsEmpty())
		{
			GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		}
		if (GuardError.IsEmpty())
		{
			GuardError = GuardExistingActor(World, LegacySecurityLabel, LegacySecurityLocation, nullptr);
		}
		if (!GuardError.IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("neighbor_changed"),
				FString::Printf(TEXT("%s Spawned actor destroyed. Maps not saved."), *GuardError),
				MakeShared<FBridgeChange>(Change));
		}

		TArray<AActor*> AfterMatches = FindByExactLabel(World, Label);
		if (AfterMatches.Num() != 1)
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("label_not_unique"),
				FString::Printf(TEXT("After spawn, label '%s' count=%d. Actor destroyed. Maps not saved."), *Label, AfterMatches.Num()),
				MakeShared<FBridgeChange>(Change));
		}

		TArray<TSharedPtr<FJsonValue>> AfterProps;
		if (PropertiesObj && (*PropertiesObj).IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PropertiesObj)->Values)
			{
				AfterProps.Add(MakeShared<FJsonValueObject>(ReadPropertySnapshot(Spawned, FindInstanceProperty(Spawned, Pair.Key))));
			}
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("on_game_thread"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Spawned));
		Change.After->SetBoolField(TEXT("actor_enable_collision"), Spawned->GetActorEnableCollision());
		Change.After->SetBoolField(TEXT("can_ever_tick"), Spawned->PrimaryActorTick.bCanEverTick);
		Change.After->SetBoolField(TEXT("tick_enabled"), Spawned->IsActorTickEnabled());
		Change.After->SetArrayField(TEXT("properties"), AfterProps);
		Change.After->SetObjectField(TEXT("reception"), ActorSnapshot(FindByExactLabel(World, ReceptionLabel)[0]));
		Change.After->SetObjectField(TEXT("access_door"), ActorSnapshot(FindByExactLabel(World, AccessDoorLabel)[0]));
		Change.After->SetObjectField(TEXT("legacy_security_terminal"), ActorSnapshot(FindByExactLabel(World, LegacySecurityLabel)[0]));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> CmdPrepareWrite(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Action = GetString(Args, TEXT("action")).ToLower();
		if (!AllowlistedActions.Contains(Action))
		{
			return Fail(
				TEXT("action_not_allowlisted"),
				FString::Printf(TEXT("Action '%s' is not allowlisted. No eval. No shell."), *Action));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		TArray<AActor*> DeleteActors;
		FString PreflightError;

		if (Action == TEXT("delete_actor"))
		{
			PreflightError = PreflightDeleteActors(Args, DeleteActors, Before, Proposed);
		}
		else if (Action == TEXT("set_component_property")
			&& Args.IsValid()
			&& GetString(Args, TEXT("scope")).Equals(TEXT("blueprint_template"), ESearchCase::IgnoreCase))
		{
			PreflightError = PreflightAccessTriggerBlueprintTemplate(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_component_property")
			&& Args.IsValid()
			&& (Args->HasField(TEXT("box_extent")) || Args->HasField(TEXT("relative_location"))))
		{
			PreflightError = TEXT("AccessTrigger geometry writes must use scope=blueprint_template. Instance overrides are not durable.");
		}
		else if (Action == TEXT("rerun_construction"))
		{
			PreflightError = PreflightRerunConstruction(Args, Before, Proposed);
		}
		else if (Action == TEXT("add_blueprint_variable"))
		{
			PreflightError = PreflightAddBlueprintVariable(Args, Before, Proposed);
		}
		else if (Action == TEXT("add_scs_component"))
		{
			PreflightError = PreflightAddScsComponent(Args, Before, Proposed);
		}
		else if (Action == TEXT("author_hologram_apply_state"))
		{
			PreflightError = PreflightAuthorHologramApplyState(Args, Before, Proposed);
		}
		else if (Action == TEXT("author_light_controller_set_zone"))
		{
			PreflightError = PreflightAuthorLightControllerSetZone(Args, Before, Proposed);
		}
		else if (Action == TEXT("author_admin_room_trigger_lighting_hook"))
		{
			PreflightError = PreflightAuthorAdminRoomTriggerLightingHook(Args, Before, Proposed);
		}
		else if (Action == TEXT("author_access_door_facility_state_listener"))
		{
			PreflightError = PreflightAuthorAccessDoorFacilityStateListener(Args, Before, Proposed);
		}
		else if (Action == TEXT("author_light_controller_facility_state_listener"))
		{
			PreflightError = PreflightAuthorLightControllerFacilityStateListener(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_s20_zone_light"))
		{
			PreflightError = PreflightSpawnS20ZoneLight(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_s20_light_intensity"))
		{
			PreflightError = PreflightSetS20LightIntensity(Args, Before, Proposed);
		}
		else if (Action == TEXT("invoke_s20_set_lighting_zone"))
		{
			PreflightError = PreflightInvokeS20SetLightingZone(Args, Before, Proposed);
		}
		else if (Action == TEXT("delete_s22_legacy_admin_ambience"))
		{
			PreflightError = PreflightDeleteS22LegacyAdminAmbience(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_s22_admin_audio_zones"))
		{
			PreflightError = PreflightSpawnS22AdminAudioZones(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_navmesh_bounds"))
		{
			PreflightError = PreflightSpawnNeuroNavMeshBounds(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_research_station"))
		{
			PreflightError = PreflightSpawnNeuroResearchStation(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_research_wing_connector"))
		{
			PreflightError = PreflightSpawnAdminResearchWingConnector(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_research_wing_keycard"))
		{
			PreflightError = PreflightSpawnAdminResearchWingKeycard(Args, Before, Proposed);
		}
		else if (Action == TEXT("trim_spine_landing_admin"))
		{
			PreflightError = PreflightTrimSpineLandingAdmin(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_admin_doorlock_interactable"))
		{
			PreflightError = PreflightSetAdminDoorlockInteractable(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_block2_dressing"))
		{
			PreflightError = PreflightSpawnAdminBlock2Dressing(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_block3_resources"))
		{
			PreflightError = PreflightSpawnAdminBlock3Resources(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_block4_security_officer"))
		{
			PreflightError = PreflightSpawnAdminBlock4SecurityOfficer(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_admin_block4_navmesh_bounds"))
		{
			PreflightError = PreflightSpawnAdminBlock4NavMeshBounds(Args, Before, Proposed);
		}
		else if (Action == TEXT("save_admin_block4_navmesh_prerequisite"))
		{
			PreflightError = PreflightSaveAdminBlock4NavMeshPrerequisite(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_arrival_lab_dressing"))
		{
			PreflightError = PreflightSpawnNeuroArrivalLabDressing(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_ch3_containment_evidence"))
		{
			PreflightError = PreflightSpawnNeuroCh3ContainmentEvidence(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_ch4_transformed_personnel"))
		{
			PreflightError = PreflightSpawnNeuroCh4TransformedPersonnel(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_power_failure_discovery"))
		{
			PreflightError = PreflightConfigureNeuroPowerFailureDiscovery(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_power_diagnostic"))
		{
			PreflightError = PreflightSpawnNeuroPowerDiagnostic(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neurogenetics_mission"))
		{
			PreflightError = PreflightCreateNeuroGeneticsMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("expand_neurogenetics_mission_beat3"))
		{
			PreflightError = PreflightExpandNeuroGeneticsMissionBeat3(Args, Before, Proposed);
		}
		else if (Action == TEXT("expand_neurogenetics_mission_beat4"))
		{
			PreflightError = PreflightExpandNeuroGeneticsMissionBeat4(Args, Before, Proposed);
		}
		else if (Action == TEXT("expand_neurogenetics_mission_beat5"))
		{
			PreflightError = PreflightExpandNeuroGeneticsMissionBeat5(Args, Before, Proposed);
		}
		else if (Action == TEXT("expand_neurogenetics_mission_beat6"))
		{
			PreflightError = PreflightExpandNeuroGeneticsMissionBeat6(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_power_restore_mission"))
		{
			PreflightError = PreflightCreateNeuroPowerRestoreMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neurogenetics_next_mission_power_restore"))
		{
			PreflightError = PreflightSetNeuroGeneticsNextMissionPowerRestore(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_backup_power_restore"))
		{
			PreflightError = PreflightConfigureNeuroBackupPowerRestore(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_targeting_why_mission"))
		{
			PreflightError = PreflightCreateNeuroTargetingWhyMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_power_restore_next_targeting_why"))
		{
			PreflightError = PreflightSetNeuroPowerRestoreNextTargetingWhy(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_researcher_targeting_why"))
		{
			PreflightError = PreflightConfigureNeuroResearcherTargetingWhy(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_research_station_intro_mission"))
		{
			PreflightError = PreflightCreateNeuroResearchStationIntroMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_targeting_why_next_research_station"))
		{
			PreflightError = PreflightSetNeuroTargetingWhyNextResearchStation(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_research_station_intro"))
		{
			PreflightError = PreflightConfigureNeuroResearchStationIntro(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neural_slow_adaptation_asset"))
		{
			PreflightError = PreflightCreateNeuralSlowAdaptationAsset(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_neural_slow_use_mission"))
		{
			PreflightError = PreflightCreateNeuroNeuralSlowUseMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_research_station_intro_next_neural_slow"))
		{
			PreflightError = PreflightSetNeuroResearchStationIntroNextNeuralSlow(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_adaptation_connection_mission"))
		{
			PreflightError = PreflightCreateNeuroAdaptationConnectionMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_neural_slow_use_next_adaptation_connection"))
		{
			PreflightError = PreflightSetNeuroNeuralSlowUseNextAdaptationConnection(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_live_adaptation_connection"))
		{
			PreflightError = PreflightConfigureNeuroLiveAdaptationConnection(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_neuro_revelation_mission"))
		{
			PreflightError = PreflightCreateNeuroRevelationMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_adaptation_connection_next_revelation"))
		{
			PreflightError = PreflightSetNeuroAdaptationConnectionNextRevelation(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_neuro_revelation_observation"))
		{
			PreflightError = PreflightConfigureNeuroRevelationObservation(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_cryo_access_mission"))
		{
			PreflightError = PreflightCreateCryoAccessMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_neuro_revelation_next_cryo_access"))
		{
			PreflightError = PreflightSetNeuroRevelationNextCryoAccess(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_cryo_backup_power_panel"))
		{
			PreflightError = PreflightConfigureCryoBackupPowerPanel(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_cryo_entry_mission"))
		{
			PreflightError = PreflightCreateCryoEntryMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_cryo_access_next_cryo_entry"))
		{
			PreflightError = PreflightSetCryoAccessNextCryoEntry(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_cryo_entry_checkpoint"))
		{
			PreflightError = PreflightConfigureCryoEntryCheckpoint(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_cryo_evidence_mission"))
		{
			PreflightError = PreflightCreateCryoEvidenceMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_cryo_entry_next_cryo_evidence"))
		{
			PreflightError = PreflightSetCryoEntryNextCryoEvidence(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_cryo_evidence_datapads"))
		{
			PreflightError = PreflightConfigureCryoEvidenceDatapads(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_compute_entry_mission"))
		{
			PreflightError = PreflightCreateComputeEntryMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_cryo_evidence_next_compute_entry"))
		{
			PreflightError = PreflightSetCryoEvidenceNextComputeEntry(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_compute_entry_checkpoint"))
		{
			PreflightError = PreflightConfigureComputeEntryCheckpoint(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_compute_handover_mission"))
		{
			PreflightError = PreflightCreateComputeHandoverMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_compute_entry_next_compute_handover"))
		{
			PreflightError = PreflightSetComputeEntryNextComputeHandover(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_compute_handover_terminals_and_datapad"))
		{
			PreflightError = PreflightConfigureComputeHandoverActors(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_the_conclusion_mission"))
		{
			PreflightError = PreflightCreateTheConclusionMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_compute_handover_next_the_conclusion"))
		{
			PreflightError = PreflightSetComputeHandoverNextTheConclusion(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_reactor_control_spine"))
		{
			PreflightError = PreflightConfigureReactorControlSpine(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_research_station_mission"))
		{
			PreflightError = PreflightCreateRespecBeatMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_the_conclusion_next_research_station"))
		{
			PreflightError = PreflightSetTheConclusionNextResearchStation(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_research_station"))
		{
			PreflightError = PreflightConfigureResearchStation(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_locomotor_disrupt_adaptation"))
		{
			PreflightError = PreflightCreateLocomotorDisruptAdaptation(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_optical_disrupt_adaptation"))
		{
			PreflightError = PreflightCreateOpticalDisruptAdaptation(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_syringe_kit_mission"))
		{
			PreflightError = PreflightCreateSyringeKitMission(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_research_station_next_syringe_kit"))
		{
			PreflightError = PreflightSetResearchStationNextSyringeKit(Args, Before, Proposed);
		}
		else if (Action == TEXT("configure_research_station_syringe_kit"))
		{
			PreflightError = PreflightConfigureResearchStationSyringeKit(Args, Before, Proposed);
		}
		else if (Action == TEXT("create_nathan_grant_look"))
		{
			PreflightError = PreflightCreateNathanGrantLook(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_adaptation_subject"))
		{
			PreflightError = PreflightSpawnNeuroAdaptationSubject(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_neural_mapping_array"))
		{
			PreflightError = PreflightSpawnNeuroNeuralMappingArray(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_research_load_cutoff"))
		{
			PreflightError = PreflightSpawnNeuroResearchLoadCutoff(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_neural_mapping_terminal"))
		{
			PreflightError = PreflightSpawnNeuroNeuralMappingTerminal(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_neural_signature_observation_node"))
		{
			PreflightError = PreflightSpawnNeuroNeuralSignatureObservationNode(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_neuro_neural_change_evidence_instrument"))
		{
			PreflightError = PreflightSpawnNeuroNeuralChangeEvidenceInstrument(Args, Before, Proposed);
		}
		else if (Action == TEXT("move_actor_to_level"))
		{
			PreflightError = PreflightMoveActorToLevel(Args, Before, Proposed);
		}
		else if (Action == TEXT("save_maps"))
		{
			PreflightError = PreflightSaveMaps(Args, Before, Proposed);
		}
		else if (Action == TEXT("set_actor_property"))
		{
			PreflightError = PreflightSetActorProperty(Args, Before, Proposed);
		}
		else if (Action == TEXT("connect_blueprint_pins"))
		{
			PreflightError = PreflightConnectBlueprintPins(Args, Before, Proposed);
		}
		else if (Action == TEXT("spawn_blueprint_actor"))
		{
			PreflightError = PreflightSpawnBlueprintActor(Args, Before, Proposed);
		}
		else
		{
			const FString Package = NormalizePackage(GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"))));
			if (Package.IsEmpty())
			{
				PreflightError = TEXT("required_package is required.");
			}
			else
			{
				Before->SetStringField(TEXT("note"), TEXT("Snapshot captured at prepare; execute re-validates."));
				Before->SetStringField(TEXT("package"), Package);
				Proposed = CloneJson(Args).ToSharedRef();
			}
		}

		if (!PreflightError.IsEmpty())
		{
			return Fail(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError));
		}

		FString Package = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"))));
		if (Action == TEXT("spawn_neuro_navmesh_bounds")
			|| Action == TEXT("spawn_neuro_research_station")
			|| Action == TEXT("spawn_neuro_arrival_lab_dressing")
			|| Action == TEXT("spawn_neuro_ch3_containment_evidence")
			|| Action == TEXT("spawn_neuro_ch4_transformed_personnel")
			|| Action == TEXT("configure_neuro_power_failure_discovery")
			|| Action == TEXT("spawn_neuro_power_diagnostic")
			|| Action == TEXT("create_neurogenetics_mission")
			|| Action == TEXT("expand_neurogenetics_mission_beat3")
			|| Action == TEXT("expand_neurogenetics_mission_beat4")
			|| Action == TEXT("expand_neurogenetics_mission_beat5")
			|| Action == TEXT("expand_neurogenetics_mission_beat6")
			|| Action == TEXT("create_neuro_power_restore_mission")
			|| Action == TEXT("set_neurogenetics_next_mission_power_restore")
			|| Action == TEXT("configure_neuro_backup_power_restore")
			|| Action == TEXT("create_neuro_targeting_why_mission")
			|| Action == TEXT("set_neuro_power_restore_next_targeting_why")
			|| Action == TEXT("configure_neuro_researcher_targeting_why")
			|| Action == TEXT("create_neuro_research_station_intro_mission")
			|| Action == TEXT("set_neuro_targeting_why_next_research_station")
			|| Action == TEXT("configure_neuro_research_station_intro")
			|| Action == TEXT("create_neuro_neural_slow_use_mission")
			|| Action == TEXT("set_neuro_research_station_intro_next_neural_slow")
			|| Action == TEXT("create_neuro_adaptation_connection_mission")
			|| Action == TEXT("set_neuro_neural_slow_use_next_adaptation_connection")
			|| Action == TEXT("configure_neuro_live_adaptation_connection")
			|| Action == TEXT("create_neuro_revelation_mission")
			|| Action == TEXT("set_neuro_adaptation_connection_next_revelation")
			|| Action == TEXT("configure_neuro_revelation_observation")
			|| Action == TEXT("create_cryo_access_mission")
			|| Action == TEXT("set_neuro_revelation_next_cryo_access")
			|| Action == TEXT("create_cryo_entry_mission")
			|| Action == TEXT("set_cryo_access_next_cryo_entry")
			|| Action == TEXT("create_cryo_evidence_mission")
			|| Action == TEXT("set_cryo_entry_next_cryo_evidence")
			|| Action == TEXT("create_compute_entry_mission")
			|| Action == TEXT("set_cryo_evidence_next_compute_entry")
			|| Action == TEXT("create_compute_handover_mission")
			|| Action == TEXT("set_compute_entry_next_compute_handover")
			|| Action == TEXT("create_the_conclusion_mission")
			|| Action == TEXT("set_compute_handover_next_the_conclusion")
			|| Action == TEXT("create_research_station_mission")
			|| Action == TEXT("set_the_conclusion_next_research_station")
			|| Action == TEXT("configure_research_station")
			|| Action == TEXT("create_syringe_kit_mission")
			|| Action == TEXT("set_research_station_next_syringe_kit")
			|| Action == TEXT("configure_research_station_syringe_kit")
			|| Action == TEXT("spawn_neuro_adaptation_subject")
			|| Action == TEXT("spawn_neuro_neural_mapping_array")
			|| Action == TEXT("spawn_neuro_research_load_cutoff")
			|| Action == TEXT("spawn_neuro_neural_mapping_terminal")
			|| Action == TEXT("spawn_neuro_neural_signature_observation_node")
			|| Action == TEXT("spawn_neuro_neural_change_evidence_instrument"))
		{
			Package = NeuroPackage;
		}
		else if (Action == TEXT("configure_cryo_backup_power_panel")
			|| Action == TEXT("configure_cryo_entry_checkpoint")
			|| Action == TEXT("configure_cryo_evidence_datapads"))
		{
			Package = CryoPackage;
		}
		else if (Action == TEXT("configure_compute_entry_checkpoint")
			|| Action == TEXT("configure_compute_handover_terminals_and_datapad"))
		{
			Package = ComputePackage;
		}
		else if (Action == TEXT("configure_reactor_control_spine"))
		{
			Package = ReactorPackage;
		}
		else if (Action == TEXT("create_nathan_grant_look"))
		{
			Package = EpitopePackage;
		}
		else if (Action == TEXT("spawn_admin_research_wing_connector")
			|| Action == TEXT("spawn_admin_research_wing_keycard")
			|| Action == TEXT("set_admin_doorlock_interactable")
			|| Action == TEXT("spawn_admin_block2_dressing")
			|| Action == TEXT("spawn_admin_block3_resources")
			|| Action == TEXT("spawn_admin_block4_security_officer")
			|| Action == TEXT("spawn_admin_block4_navmesh_bounds")
			|| Action == TEXT("save_admin_block4_navmesh_prerequisite"))
		{
			Package = AdminPackage;
		}
		else if (Action == TEXT("create_neural_slow_adaptation_asset"))
		{
			Package = NeuralSlowAdaptationPackage;
		}
		else if (Action == TEXT("create_locomotor_disrupt_adaptation"))
		{
			Package = LocomotorDisruptPackage;
		}
		else if (Action == TEXT("create_optical_disrupt_adaptation"))
		{
			Package = OpticalDisruptPackage;
		}
		else if (Action == TEXT("trim_spine_landing_admin"))
		{
			Package = EpitopePackage;
		}
		else if (Action == TEXT("save_maps"))
		{
			TArray<FString> SavePackages;
			bool bSaveAdminOnly = false;
			bool bSaveNeuroOnly = false;
			bool bSaveEpitopeOnly = false;
			bool bSaveCryoOnly = false;
			bool bSaveComputeOnly = false;
			bool bSaveReactorOnly = false;
			FString SaveParseError;
			if (ParseSaveMapsPackages(Args, SavePackages, bSaveAdminOnly, bSaveNeuroOnly, bSaveEpitopeOnly, bSaveCryoOnly, bSaveComputeOnly, bSaveReactorOnly, SaveParseError) && bSaveNeuroOnly)
			{
				Package = NeuroPackage;
			}
			else if (bSaveCryoOnly)
			{
				Package = CryoPackage;
			}
			else if (bSaveComputeOnly)
			{
				Package = ComputePackage;
			}
			else if (bSaveReactorOnly)
			{
				Package = ReactorPackage;
			}
			else if (bSaveEpitopeOnly)
			{
				Package = EpitopePackage;
			}
			else
			{
				Package = AdminPackage;
			}
		}
		else if (Action == TEXT("move_actor_to_level") || Action == TEXT("set_actor_property")
			|| Action == TEXT("spawn_blueprint_actor") || Action == TEXT("spawn_s20_zone_light")
			|| Action == TEXT("set_s20_light_intensity") || Action == TEXT("invoke_s20_set_lighting_zone")
			|| Action == TEXT("delete_s22_legacy_admin_ambience") || Action == TEXT("spawn_s22_admin_audio_zones"))
		{
			Package = AdminPackage;
		}
		else if (Action == TEXT("author_admin_room_trigger_lighting_hook"))
		{
			Package = RoomTriggerBpPackage;
		}
		else if (Action == TEXT("author_access_door_facility_state_listener"))
		{
			Package = AccessDoorBpPackage;
		}
		else if (Action == TEXT("author_light_controller_facility_state_listener"))
		{
			Package = LightControllerBpPackage;
		}
		else if (Package.IsEmpty())
		{
			Package = GetString(Args, TEXT("scope")).Equals(TEXT("blueprint_template"), ESearchCase::IgnoreCase)
				? FString(AccessDoorBpPackage)
				: FString(AdminPackage);
		}

		TSharedRef<FBridgeChange> Change = MakeShared<FBridgeChange>();
		Change->ChangeId = NewChangeId();
		Change->Action = Action;
		Change->Description = GetString(Args, TEXT("description"), Action);
		Change->Package = Package;
		Change->Risk = RiskFor(Action);
		Change->Status = TEXT("prepared_awaiting_dual_approval");
		Change->CreatedAt = NowIso();
		Change->Targets = MakeShared<FJsonObject>();
		const TArray<TSharedPtr<FJsonValue>>* TargetArr = nullptr;
		if (Args.IsValid() && Args->TryGetArrayField(TEXT("targets"), TargetArr) && TargetArr)
		{
			Change->Targets->SetArrayField(TEXT("actors"), *TargetArr);
		}
		else
		{
			Change->Targets->SetStringField(TEXT("path"), GetString(Args, TEXT("path"), GetString(Args, TEXT("actor"))));
		}
		Change->Before = Before;
		Change->Proposed = Proposed;
		Change->Args = CloneJson(Args);
		Change->Args->SetStringField(TEXT("action"), Action);
		Change->Args->SetStringField(TEXT("required_package"), Package);

		{
			FScopeLock Lock(&GLedgerMutex);
			GChanges.Add(Change->ChangeId, Change);
		}

		LogAudit(TEXT("prepare"), *Change);
		TSharedRef<FJsonObject> Data = AuditBase(*Change);
		Data->SetStringField(TEXT("created_at"), Change->CreatedAt);
		Data->SetBoolField(TEXT("executed"), false);
		Data->SetStringField(TEXT("next"), TEXT("approve_write as user, then approve_write as second_review, then execute_write"));
		return Ok(Data);
	}

	TSharedPtr<FBridgeChange> FindChange(const FString& ChangeId)
	{
		FScopeLock Lock(&GLedgerMutex);
		if (TSharedRef<FBridgeChange>* Found = GChanges.Find(ChangeId))
		{
			return *Found;
		}
		return nullptr;
	}

	TSharedRef<FJsonObject> CmdGetChange(const TSharedPtr<FJsonObject>& Args)
	{
		const FString ChangeId = GetString(Args, TEXT("change_id"));
		TSharedPtr<FBridgeChange> Change = FindChange(ChangeId);
		if (!Change.IsValid())
		{
			return Fail(TEXT("not_found"), TEXT("Unknown change_id."));
		}
		return Ok(AuditBase(*Change));
	}

	TSharedRef<FJsonObject> CmdListChanges()
	{
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Items;
		FScopeLock Lock(&GLedgerMutex);
		for (const TPair<FString, TSharedRef<FBridgeChange>>& Pair : GChanges)
		{
			Items.Add(MakeShared<FJsonValueObject>(AuditBase(*Pair.Value)));
		}
		Data->SetArrayField(TEXT("changes"), Items);
		Data->SetNumberField(TEXT("count"), Items.Num());
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdApproveWrite(const TSharedPtr<FJsonObject>& Args)
	{
		const FString ChangeId = GetString(Args, TEXT("change_id"));
		const FString Role = GetString(Args, TEXT("role")).ToLower();
		const FString Identity = GetString(Args, TEXT("identity"));
		if (Identity.IsEmpty())
		{
			return Fail(TEXT("identity_required"), TEXT("Approval requires a non-empty identity."));
		}
		if (Role != TEXT("user") && Role != TEXT("second_review"))
		{
			return Fail(TEXT("bad_role"), TEXT("role must be 'user' or 'second_review'."));
		}

		TSharedPtr<FBridgeChange> Change = FindChange(ChangeId);
		if (!Change.IsValid())
		{
			return Fail(TEXT("not_found"), TEXT("Unknown change_id."));
		}
		if (Change->bExecuted || Change->Status == TEXT("executed") || Change->Status == TEXT("rejected"))
		{
			return FailAudit(TEXT("immutable"), TEXT("This change can no longer be approved."), Change);
		}

		FBridgeApproval* Slot = (Role == TEXT("user")) ? &Change->User : &Change->SecondReview;
		const FBridgeApproval* Other = (Role == TEXT("user")) ? &Change->SecondReview : &Change->User;
		if (Other->bApproved && Other->Identity.Equals(Identity, ESearchCase::IgnoreCase))
		{
			return FailAudit(
				TEXT("identity_reuse"),
				TEXT("User and second-review approvals must be recorded with different identities."),
				Change);
		}
		Slot->bApproved = true;
		Slot->Identity = Identity;
		Slot->Timestamp = NowIso();
		if (Change->User.bApproved && Change->SecondReview.bApproved)
		{
			Change->Status = TEXT("dual_approved_awaiting_execute");
		}
		else if (Change->User.bApproved)
		{
			Change->Status = TEXT("user_approved_awaiting_second_review");
		}
		else
		{
			Change->Status = TEXT("second_review_approved_awaiting_user");
		}
		LogAudit(TEXT("approve"), *Change);
		return Ok(AuditBase(*Change));
	}

	TSharedRef<FJsonObject> CmdRejectWrite(const TSharedPtr<FJsonObject>& Args)
	{
		const FString ChangeId = GetString(Args, TEXT("change_id"));
		TSharedPtr<FBridgeChange> Change = FindChange(ChangeId);
		if (!Change.IsValid())
		{
			return Fail(TEXT("not_found"), TEXT("Unknown change_id."));
		}
		if (Change->bExecuted)
		{
			return FailAudit(TEXT("already_executed"), TEXT("Cannot reject an executed change."), Change);
		}
		Change->Status = TEXT("rejected");
		LogAudit(TEXT("reject"), *Change);
		return Ok(AuditBase(*Change));
	}

	TSharedRef<FJsonObject> ExecuteDelete(FBridgeChange& Change)
	{
		TArray<AActor*> Actors;
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString Error = PreflightDeleteActors(Change.Args, Actors, Before, Proposed);
		if (!Error.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *Error), MakeShared<FBridgeChange>(Change));
		}

		UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
		if (!ActorSub)
		{
			return Fail(TEXT("no_subsystem"), TEXT("EditorActorSubsystem unavailable. ZERO writes."));
		}

		TArray<TSharedPtr<FJsonValue>> Deleted;
		for (AActor* Actor : Actors)
		{
			const FString Label = ActorLabel(Actor);
			const FString Path = Actor->GetPathName();
			const bool bDestroyed = ActorSub->DestroyActor(Actor);
			if (!bDestroyed)
			{
				Change.Status = TEXT("execute_partial_failure");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(TEXT("deleted"), Deleted);
				Change.After->SetStringField(TEXT("failed"), Label);
				return FailAudit(
					TEXT("destroy_failed"),
					FString::Printf(TEXT("destroy_actor failed for %s path=%s. Later targets not deleted. Package not saved."), *Label, *Path),
					MakeShared<FBridgeChange>(Change));
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Label);
			Row->SetStringField(TEXT("path"), Path);
			Row->SetStringField(TEXT("state"), TEXT("deleted"));
			Deleted.Add(MakeShared<FJsonValueObject>(Row));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.Status = TEXT("executed");
		Change.bSavePerformed = false;
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetArrayField(TEXT("deleted"), Deleted);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("Actors destroyed in editor memory. Package not saved by this change."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	UPackage* FindPackageByName(const FString& PackageName)
	{
		const FString Normalized = NormalizePackage(PackageName);
		return FindPackage(nullptr, *Normalized);
	}

	TSharedRef<FJsonObject> ExecuteSaveAsset(FBridgeChange& Change)
	{
		const FString PackageName = Change.Package;
		if (IsMapPackageName(PackageName))
		{
			return FailAudit(
				TEXT("use_save_maps"),
				TEXT("save_asset cannot target map packages. Use gated save_maps on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}
		UPackage* Package = FindPackageByName(PackageName);
		if (!Package)
		{
			Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		}
		if (!Package)
		{
			return FailAudit(TEXT("not_found"), FString::Printf(TEXT("Package '%s' not found."), *PackageName), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(Package->GetName(), PackageName))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Loaded package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<UPackage*> Packages;
		Packages.Add(Package);
		// -unattended makes PromptForCheckoutAndSave return PR_Cancelled before it writes.
		// SavePackages is the same API save_maps already uses, and it does not open a dialog.
		bool bSaved = false;
		if (FApp::IsUnattended())
		{
			bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, /*bOnlyDirty=*/false);
		}
		else
		{
			const FEditorFileUtils::EPromptReturnCode SaveResult = FEditorFileUtils::PromptForCheckoutAndSave(
				Packages, /*bCheckDirty=*/false, /*bPromptToSave=*/false);
			bSaved = (SaveResult == FEditorFileUtils::PR_Success);
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = bSaved;
		Change.Status = bSaved ? TEXT("executed") : TEXT("execute_save_failed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("saved"), bSaved);
		Change.After->SetStringField(TEXT("package"), Package->GetName());
		LogAudit(TEXT("execute"), Change);
		if (!bSaved)
		{
			return FailAudit(TEXT("save_failed"), TEXT("Save did not complete."), MakeShared<FBridgeChange>(Change));
		}
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteCompileBlueprint(FBridgeChange& Change)
	{
		const FString Path = GetString(Change.Args, TEXT("path"));
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Path);
		if (!Blueprint)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint not found."), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(Blueprint->GetOutermost()->GetName(), Change.Package)
			&& !Blueprint->GetPathName().StartsWith(Change.Package))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Blueprint package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("path"), Blueprint->GetPathName());
		Change.After->SetStringField(TEXT("status"), TEXT("compile requested"));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	UPrimitiveComponent* FindPrimitive(AActor* Actor, const FString& ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}
		TArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents<UPrimitiveComponent>(Primitives);
		if (ComponentName.IsEmpty() && Primitives.Num() > 0)
		{
			return Primitives[0];
		}
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive && Primitive->GetName().Equals(ComponentName, ESearchCase::IgnoreCase))
			{
				return Primitive;
			}
		}
		return nullptr;
	}

	AActor* FindUniqueLabel(UWorld* World, const FString& Label)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		return Matches.Num() == 1 ? Matches[0] : nullptr;
	}

	TSharedRef<FJsonObject> ExecuteComponentMutations(FBridgeChange& Change)
	{
		if (GetPieWorld())
		{
			return FailAudit(TEXT("pie_running"), TEXT("Stop PIE before executing writes."), MakeShared<FBridgeChange>(Change));
		}
		UWorld* World = GetEditorWorld();
		const FString ActorQuery = GetString(Change.Args, TEXT("actor"), GetString(Change.Args, TEXT("name")));
		AActor* Actor = FindUniqueLabel(World, ActorQuery);
		if (!Actor)
		{
			return FailAudit(TEXT("not_found"), TEXT("Actor not found or not unique."), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), Change.Package))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Actor owning package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UPrimitiveComponent* Primitive = FindPrimitive(Actor, GetString(Change.Args, TEXT("component")));
		TSharedRef<FJsonObject> After = MakeShared<FJsonObject>();
		After->SetStringField(TEXT("label"), ActorLabel(Actor));

		if (Change.Action == TEXT("set_visibility"))
		{
			const bool bVisible = GetBool(Change.Args, TEXT("visible"), true);
			if (Primitive)
			{
				Primitive->SetVisibility(bVisible, true);
				Primitive->SetHiddenInGame(!bVisible);
			}
			Actor->SetActorHiddenInGame(!bVisible);
			After->SetBoolField(TEXT("visible"), bVisible);
		}
		else if (Change.Action == TEXT("set_collision"))
		{
			if (!Primitive)
			{
				return FailAudit(TEXT("not_found"), TEXT("Component not found."), MakeShared<FBridgeChange>(Change));
			}
			const FName Profile(*GetString(Change.Args, TEXT("profile"), GetString(Change.Args, TEXT("collision_profile"))));
			Primitive->SetCollisionProfileName(Profile);
			After->SetStringField(TEXT("collision_profile"), Primitive->GetCollisionProfileName().ToString());
		}
		else if (Change.Action == TEXT("set_transform"))
		{
			FVector Location = Actor->GetActorLocation();
			GetVector(Change.Args, TEXT("location"), Location);
			FRotator Rotation = Actor->GetActorRotation();
			FVector RotVec;
			if (GetVector(Change.Args, TEXT("rotation"), RotVec))
			{
				Rotation = FRotator(RotVec.X, RotVec.Y, RotVec.Z);
			}
			FVector Scale = Actor->GetActorScale3D();
			GetVector(Change.Args, TEXT("scale"), Scale);
			Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
			Actor->SetActorRotation(Rotation);
			Actor->SetActorScale3D(Scale);
			After->SetArrayField(TEXT("location"), Vec(Actor->GetActorLocation()));
			After->SetArrayField(TEXT("rotation"), Rot(Actor->GetActorRotation()));
			After->SetArrayField(TEXT("scale"), Vec(Actor->GetActorScale3D()));
		}
		else if (Change.Action == TEXT("set_component_property"))
		{
			FVector NewLocation;
			FVector NewExtent;
			const bool bHasLocation = GetVector(Change.Args, TEXT("relative_location"), NewLocation);
			const bool bHasExtent = GetVector(Change.Args, TEXT("box_extent"), NewExtent);
			if (bHasLocation || bHasExtent)
			{
				return FailAudit(
					TEXT("instance_override_forbidden"),
					TEXT("AccessTrigger geometry writes must use scope=blueprint_template. Instance overrides are not durable."),
					MakeShared<FBridgeChange>(Change));
			}
			else
			{
				const FString PropertyName = GetString(Change.Args, TEXT("property"));
				if (!AllowlistedProperties.Contains(PropertyName))
				{
					return FailAudit(TEXT("property_not_allowlisted"), TEXT("Component property is not allowlisted."), MakeShared<FBridgeChange>(Change));
				}
				if (!Primitive)
				{
					return FailAudit(TEXT("not_found"), TEXT("Component not found."), MakeShared<FBridgeChange>(Change));
				}
				if (PropertyName == TEXT("CollisionProfileName"))
				{
					Primitive->SetCollisionProfileName(FName(*GetString(Change.Args, TEXT("value"))));
				}
				else if (PropertyName == TEXT("bHiddenInGame") || PropertyName == TEXT("bVisible"))
				{
					const bool bValue = GetBool(Change.Args, TEXT("bool_value"), GetString(Change.Args, TEXT("value")).ToBool());
					if (PropertyName == TEXT("bHiddenInGame"))
					{
						Primitive->SetHiddenInGame(bValue);
					}
					else
					{
						Primitive->SetVisibility(bValue, true);
					}
				}
				else
				{
					return FailAudit(TEXT("unsupported_property"), TEXT("Use set_collision / set_visibility / set_transform for this property."), MakeShared<FBridgeChange>(Change));
				}
				After->SetStringField(TEXT("property"), PropertyName);
			}
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteBlueprintAccessTriggerTemplate(FBridgeChange& Change)
	{
		if (GetPieWorld())
		{
			return FailAudit(TEXT("pie_running"), TEXT("Stop PIE before executing writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> GeoBefore = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> GeoProposed = MakeShared<FJsonObject>();
		const FString GeoError = PreflightAccessTriggerBlueprintTemplate(Change.Args, GeoBefore, GeoProposed);
		if (!GeoError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), GeoError, MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadAccessDoorBlueprint();
		UBoxComponent* Primary = FindAccessTriggerTemplate(Blueprint);
		if (!Blueprint || !Primary)
		{
			return FailAudit(TEXT("not_found"), TEXT("AccessTrigger SCS template not found."), MakeShared<FBridgeChange>(Change));
		}

		FVector NewLocation;
		FVector NewExtent;
		GetVector(Change.Args, TEXT("relative_location"), NewLocation);
		GetVector(Change.Args, TEXT("box_extent"), NewExtent);

		TSet<UBoxComponent*> Templates;
		UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass);
		if (Blueprint->SimpleConstructionScript)
		{
			for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (!Node || !Node->GetVariableName().ToString().Equals(TEXT("AccessTrigger"), ESearchCase::IgnoreCase))
				{
					continue;
				}
				if (UBoxComponent* Generated = Cast<UBoxComponent>(Node->GetActualComponentTemplate(BPGC)))
				{
					Templates.Add(Generated);
				}
				if (UBoxComponent* NodeTemplate = Cast<UBoxComponent>(Node->ComponentTemplate))
				{
					Templates.Add(NodeTemplate);
				}
			}
		}
		Templates.Add(Primary);

		Blueprint->Modify();
		for (UBoxComponent* Box : Templates)
		{
			if (!Box)
			{
				continue;
			}
			Box->Modify();
			Box->SetRelativeLocation(NewLocation);
			Box->SetBoxExtent(NewExtent, true);
			Box->UpdateBounds();
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

		TSharedRef<FJsonObject> After = MakeShared<FJsonObject>();
		After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		After->SetStringField(TEXT("scope"), TEXT("blueprint_template"));
		After->SetStringField(TEXT("component"), TEXT("AccessTrigger"));
		After->SetObjectField(TEXT("component_after"), BoxComponentSnapshot(FindAccessTriggerTemplate(Blueprint)));
		After->SetBoolField(TEXT("save"), false);
		After->SetBoolField(TEXT("compile"), false);

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteRerunConstruction(FBridgeChange& Change)
	{
		if (GetPieWorld())
		{
			return FailAudit(TEXT("pie_running"), TEXT("Stop PIE before executing writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightRerunConstruction(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), PreflightError, MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		AActor* Actor = FindUniqueLabel(World, GetString(Change.Args, TEXT("actor"), GetString(Change.Args, TEXT("name"))));
		if (!Actor)
		{
			return FailAudit(TEXT("not_found"), TEXT("Actor not found or not unique."), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), Change.Package))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Actor owning package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		Actor->RerunConstructionScripts();

		TSharedRef<FJsonObject> After = SnapshotDoorComponents(Actor);
		After->SetBoolField(TEXT("save"), false);
		After->SetStringField(TEXT("note"), TEXT("RerunConstructionScripts only. Package not saved."));

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteAddBlueprintVariable(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAddBlueprintVariable(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		const FString Path = GetString(Change.Args, TEXT("path"));
		UBlueprint* Blueprint = LoadBlueprintAsset(Path);
		if (!Blueprint)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (!PackagesEqual(Blueprint->GetOutermost()->GetName(), Change.Package)
			&& !Blueprint->GetPathName().StartsWith(Change.Package))
		{
			return FailAudit(TEXT("wrong_package"), TEXT("Blueprint package does not match approved package. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UUserDefinedEnum* EnumAsset = LoadEnumAsset(
			GetString(Change.Args, TEXT("enum_path"), GetString(Change.Args, TEXT("type"))));
		if (!EnumAsset)
		{
			return FailAudit(TEXT("not_found"), TEXT("UserDefinedEnum not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FName VarFName(*GetString(Change.Args, TEXT("variable_name")));
		FEdGraphPinType PinType;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
		PinType.PinSubCategoryObject = EnumAsset;

		if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarFName, PinType))
		{
			return FailAudit(
				TEXT("add_failed"),
				TEXT("FBlueprintEditorUtils::AddMemberVariable rejected the enum type. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		if (GetBool(Change.Args, TEXT("instance_editable"), true))
		{
			FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarFName, false);
		}

		const FString Category = GetString(Change.Args, TEXT("category"));
		FBlueprintEditorUtils::SetBlueprintVariableCategory(
			Blueprint, VarFName, nullptr, FText::FromString(Category), /*bDontRecompile=*/true);

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		TSharedRef<FJsonObject> After = VariableListSnapshot(Blueprint);
		After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		After->SetBoolField(TEXT("save"), false);
		After->SetBoolField(TEXT("compile"), false);
		After->SetStringField(TEXT("note"), TEXT("Enum member added. Package not saved. Full compile not performed."));
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteMoveActorToLevel(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("move_actor_to_level must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightMoveActorToLevel(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		const FString Label = GetString(Change.Args, TEXT("actor"), GetString(Change.Args, TEXT("label"), ReceptionLabel));
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		AActor* Actor = (Matches.Num() == 1) ? Matches[0] : nullptr;
		if (!Actor)
		{
			return FailAudit(TEXT("not_found"), TEXT("Actor not found or not unique at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		ULevelStreaming* DestStreaming = FindStreamingLevel(World, AdminPackage);
		ULevel* DestLevel = DestStreaming ? DestStreaming->GetLoadedLevel() : nullptr;
		if (!DestLevel)
		{
			return FailAudit(TEXT("not_loaded"), TEXT("Admin streaming level is not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const TSharedPtr<FJsonObject> DoorBefore = Change.Before.IsValid() ? Change.Before : Before;
		const TSharedPtr<FJsonObject>* DoorSnap = nullptr;
		TSharedPtr<FJsonObject> DoorBeforeObj;
		if (DoorBefore.IsValid() && DoorBefore->TryGetObjectField(TEXT("access_door"), DoorSnap) && DoorSnap)
		{
			DoorBeforeObj = *DoorSnap;
		}

		TArray<AActor*> ToMove;
		ToMove.Add(Actor);
		Actor = nullptr;
		Matches.Reset();

		const int32 Moved = UEditorLevelUtils::MoveActorsToLevel(
			ToMove,
			DestLevel,
			/*bWarnAboutReferences=*/false,
			/*bWarnAboutRenaming=*/false,
			/*bMoveAllOrFail=*/true,
			/*OutActors=*/nullptr);
		ToMove.Reset();

		TArray<AActor*> AfterMatches = FindByExactLabel(World, Label);
		if (Moved != 1 || AfterMatches.Num() != 1)
		{
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetNumberField(TEXT("moved"), Moved);
			Change.After->SetNumberField(TEXT("found_after"), AfterMatches.Num());
			return FailAudit(
				TEXT("move_failed"),
				FString::Printf(TEXT("MoveActorsToLevel returned %d; found %d '%s' after re-find. Maps not saved."), Moved, AfterMatches.Num(), *Label),
				MakeShared<FBridgeChange>(Change));
		}

		AActor* MovedActor = AfterMatches[0];
		const FString Owner = NormalizePackage(ActorOwningPackage(MovedActor));
		if (!PackagesEqual(Owner, AdminPackage))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("wrong_owner"),
				FString::Printf(TEXT("After move, %s owning package is '%s' expected Admin. Maps not saved."), *Label, *Owner),
				MakeShared<FBridgeChange>(Change));
		}
		const FString XformError = TransformMismatch(MovedActor, ReceptionLocation, ReceptionRotation, ReceptionScale);
		if (!XformError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("transform_changed"),
				FString::Printf(TEXT("After move, %s %s. Maps not saved."), *Label, *XformError),
				MakeShared<FBridgeChange>(Change));
		}

		AActor* Door = FindUniqueLabel(World, GetString(Change.Args, TEXT("door_label"), AccessDoorLabel));
		const FString DoorError = DoorDelta(Door, DoorBeforeObj);
		if (!DoorError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("door_changed"), FString::Printf(TEXT("%s Maps not saved."), *DoorError), MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Change.After->SetNumberField(TEXT("moved"), Moved);
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(MovedActor));
		Change.After->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("Native MoveActorsToLevel on game thread. Source actor was destroyed by cut/paste. Re-found by label. Maps not saved."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteSaveMaps(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_maps must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveMaps(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> Packages;
		bool bAdminOnly = false;
		bool bNeuroOnly = false;
		bool bEpitopeOnly = false;
		bool bCryoOnly = false;
		bool bComputeOnly = false;
		bool bReactorOnly = false;
		FString ParseError;
		if (!ParseSaveMapsPackages(Change.Args, Packages, bAdminOnly, bNeuroOnly, bEpitopeOnly, bCryoOnly, bComputeOnly, bReactorOnly, ParseError))
		{
			return FailAudit(TEXT("bad_args"), ParseError, MakeShared<FBridgeChange>(Change));
		}

		if (bCryoOnly)
		{
			UPackage* CryoPkg = FindPackageByName(CryoPackage);
			if (!CryoPkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("Cryo map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> CryoOnly;
			CryoOnly.Add(CryoPkg);
			const bool bCryoSaved = UEditorLoadingAndSavingUtils::SavePackages(CryoOnly, /*bOnlyDirty=*/false);
			Change.bExecuted = bCryoSaved;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = bCryoSaved;
			Change.Status = bCryoSaved ? TEXT("executed") : TEXT("execute_save_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Change.After->SetBoolField(TEXT("cryo_saved"), bCryoSaved);
			Change.After->SetBoolField(TEXT("cryo_only"), true);
			Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Change.After->SetBoolField(TEXT("dialog"), false);
			Change.After->SetBoolField(TEXT("save_all"), false);
			Change.After->SetBoolField(TEXT("compile"), false);
			TArray<TSharedPtr<FJsonValue>> SavedCryo;
			if (bCryoSaved)
			{
				SavedCryo.Add(MakeShared<FJsonValueString>(FString(CryoPackage)));
			}
			Change.After->SetArrayField(TEXT("packages_saved"), SavedCryo);
			LogAudit(TEXT("execute"), Change);
			return bCryoSaved ? Ok(AuditBase(Change)) : FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for SL_Epitope_Cryo."), MakeShared<FBridgeChange>(Change));
		}

		if (bComputeOnly)
		{
			UPackage* ComputePkg = FindPackageByName(ComputePackage);
			if (!ComputePkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("Compute map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> ComputeOnly;
			ComputeOnly.Add(ComputePkg);
			const bool bComputeSaved = UEditorLoadingAndSavingUtils::SavePackages(ComputeOnly, /*bOnlyDirty=*/false);
			Change.bExecuted = bComputeSaved;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = bComputeSaved;
			Change.Status = bComputeSaved ? TEXT("executed") : TEXT("execute_save_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Change.After->SetBoolField(TEXT("compute_saved"), bComputeSaved);
			Change.After->SetBoolField(TEXT("compute_only"), true);
			Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Change.After->SetBoolField(TEXT("dialog"), false);
			Change.After->SetBoolField(TEXT("save_all"), false);
			Change.After->SetBoolField(TEXT("compile"), false);
			TArray<TSharedPtr<FJsonValue>> SavedCompute;
			if (bComputeSaved)
			{
				SavedCompute.Add(MakeShared<FJsonValueString>(FString(ComputePackage)));
			}
			Change.After->SetArrayField(TEXT("packages_saved"), SavedCompute);
			LogAudit(TEXT("execute"), Change);
			return bComputeSaved ? Ok(AuditBase(Change)) : FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for SL_Epitope_Compute."), MakeShared<FBridgeChange>(Change));
		}

		if (bReactorOnly)
		{
			UPackage* ReactorPkg = FindPackageByName(ReactorPackage);
			if (!ReactorPkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("Reactor map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> ReactorOnly;
			ReactorOnly.Add(ReactorPkg);
			const bool bReactorSaved = UEditorLoadingAndSavingUtils::SavePackages(ReactorOnly, /*bOnlyDirty=*/false);
			Change.bExecuted = bReactorSaved;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = bReactorSaved;
			Change.Status = bReactorSaved ? TEXT("executed") : TEXT("execute_save_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Change.After->SetBoolField(TEXT("reactor_saved"), bReactorSaved);
			Change.After->SetBoolField(TEXT("reactor_only"), true);
			Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Change.After->SetBoolField(TEXT("dialog"), false);
			Change.After->SetBoolField(TEXT("save_all"), false);
			Change.After->SetBoolField(TEXT("compile"), false);
			TArray<TSharedPtr<FJsonValue>> SavedReactor;
			if (bReactorSaved)
			{
				SavedReactor.Add(MakeShared<FJsonValueString>(FString(ReactorPackage)));
			}
			Change.After->SetArrayField(TEXT("packages_saved"), SavedReactor);
			LogAudit(TEXT("execute"), Change);
			return bReactorSaved ? Ok(AuditBase(Change)) : FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for SL_Epitope_Reactor."), MakeShared<FBridgeChange>(Change));
		}

		if (bEpitopeOnly)
		{
			UPackage* EpitopeOnlyPkg = FindPackageByName(EpitopePackage);
			if (!EpitopeOnlyPkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("Lvl_Epitope map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> EpitopeOnly;
			EpitopeOnly.Add(EpitopeOnlyPkg);
			const bool bEpitopeOnlySaved = UEditorLoadingAndSavingUtils::SavePackages(EpitopeOnly, /*bOnlyDirty=*/false);
			Change.bExecuted = bEpitopeOnlySaved;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = bEpitopeOnlySaved;
			Change.Status = bEpitopeOnlySaved ? TEXT("executed") : TEXT("execute_save_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Change.After->SetBoolField(TEXT("epitope_saved"), bEpitopeOnlySaved);
			Change.After->SetBoolField(TEXT("epitope_only"), true);
			Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Change.After->SetBoolField(TEXT("dialog"), false);
			Change.After->SetBoolField(TEXT("save_all"), false);
			Change.After->SetBoolField(TEXT("compile"), false);
			TArray<TSharedPtr<FJsonValue>> SavedEpitope;
			if (bEpitopeOnlySaved)
			{
				SavedEpitope.Add(MakeShared<FJsonValueString>(FString(EpitopePackage)));
			}
			Change.After->SetArrayField(TEXT("packages_saved"), SavedEpitope);
			LogAudit(TEXT("execute"), Change);
			return bEpitopeOnlySaved
				? Ok(AuditBase(Change))
				: FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for Lvl_Epitope."), MakeShared<FBridgeChange>(Change));
		}

		if (bNeuroOnly)
		{
			UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
			if (!NeuroPkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("NeuroGenetics map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> NeuroOnly;
			NeuroOnly.Add(NeuroPkg);
			const bool bNeuroSaved = UEditorLoadingAndSavingUtils::SavePackages(NeuroOnly, /*bOnlyDirty=*/false);
			Change.bExecuted = bNeuroSaved;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = bNeuroSaved;
			Change.Status = bNeuroSaved ? TEXT("executed") : TEXT("execute_save_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
			Change.After->SetBoolField(TEXT("neuro_saved"), bNeuroSaved);
			Change.After->SetBoolField(TEXT("neuro_only"), true);
			Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
			Change.After->SetBoolField(TEXT("dialog"), false);
			Change.After->SetBoolField(TEXT("save_all"), false);
			Change.After->SetBoolField(TEXT("compile"), false);
			TArray<TSharedPtr<FJsonValue>> SavedNeuro;
			if (bNeuroSaved)
			{
				SavedNeuro.Add(MakeShared<FJsonValueString>(FString(NeuroPackage)));
			}
			Change.After->SetArrayField(TEXT("packages_saved"), SavedNeuro);
			LogAudit(TEXT("execute"), Change);
			return bNeuroSaved ? Ok(AuditBase(Change)) : FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for SL_Epitope_NeuroGenetics."), MakeShared<FBridgeChange>(Change));
		}

		UPackage* AdminPkg = FindPackageByName(AdminPackage);
		if (!AdminPkg)
		{
			return FailAudit(TEXT("not_found"), TEXT("Admin map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<UPackage*> AdminOnly;
		AdminOnly.Add(AdminPkg);
		const bool bAdminSaved = UEditorLoadingAndSavingUtils::SavePackages(AdminOnly, /*bOnlyDirty=*/false);

		bool bEpitopeSaved = false;
		if (!bAdminOnly)
		{
			UPackage* EpitopePkg = FindPackageByName(EpitopePackage);
			if (!EpitopePkg)
			{
				return FailAudit(TEXT("not_found"), TEXT("Lvl_Epitope map package not loaded. Admin may already have been saved."), MakeShared<FBridgeChange>(Change));
			}
			TArray<UPackage*> EpitopeOnly;
			EpitopeOnly.Add(EpitopePkg);
			bEpitopeSaved = UEditorLoadingAndSavingUtils::SavePackages(EpitopeOnly, /*bOnlyDirty=*/false);
		}

		const bool bSaved = bAdminOnly ? bAdminSaved : (bAdminSaved && bEpitopeSaved);
		Change.bExecuted = bSaved;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = bSaved;
		Change.Status = bSaved ? TEXT("executed") : TEXT("execute_save_failed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Change.After->SetBoolField(TEXT("admin_saved"), bAdminSaved);
		Change.After->SetBoolField(TEXT("epitope_saved"), bEpitopeSaved);
		Change.After->SetBoolField(TEXT("admin_only"), bAdminOnly);
		Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
		Change.After->SetBoolField(TEXT("dialog"), false);
		Change.After->SetBoolField(TEXT("save_all"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		TArray<TSharedPtr<FJsonValue>> SavedPkgs;
		if (bAdminSaved)
		{
			SavedPkgs.Add(MakeShared<FJsonValueString>(FString(AdminPackage)));
		}
		if (bEpitopeSaved)
		{
			SavedPkgs.Add(MakeShared<FJsonValueString>(FString(EpitopePackage)));
		}
		Change.After->SetArrayField(TEXT("packages_saved"), SavedPkgs);

		UWorld* World = GetEditorWorld();
		AActor* SecurityAfter = FindUniqueLabel(World, SecurityTerminalLabel);
		AActor* Actor = FindUniqueLabel(World, ReceptionLabel);
		AActor* Door = FindUniqueLabel(World, AccessDoorLabel);
		if (SecurityAfter)
		{
			Change.After->SetObjectField(TEXT("security"), ActorSnapshot(SecurityAfter));
		}
		if (Actor)
		{
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
			Change.After->SetObjectField(TEXT("reception_properties"), SnapshotReceptionConfig(Actor));
		}
		if (Door)
		{
			Change.After->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		}

		LogAudit(TEXT("execute"), Change);
		if (!bSaved)
		{
			return FailAudit(
				TEXT("save_failed"),
				FString::Printf(
					TEXT("Game-thread save incomplete. admin_saved=%s epitope_saved=%s admin_only=%s. Not Save All."),
					bAdminSaved ? TEXT("true") : TEXT("false"),
					bEpitopeSaved ? TEXT("true") : TEXT("false"),
					bAdminOnly ? TEXT("true") : TEXT("false")),
				MakeShared<FBridgeChange>(Change));
		}
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> ExecuteSetActorProperty(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_actor_property must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetActorProperty(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		const FString Label = GetString(Change.Args, TEXT("actor"), GetString(Change.Args, TEXT("label"), GetString(Change.Args, TEXT("name"))));
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		AActor* Actor = (Matches.Num() == 1) ? Matches[0] : nullptr;
		if (!Actor)
		{
			return FailAudit(TEXT("not_found"), TEXT("Actor not found or not unique at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const TSharedPtr<FJsonObject> DoorBeforeRoot = Change.Before.IsValid() ? Change.Before : Before;
		const TSharedPtr<FJsonObject>* DoorSnap = nullptr;
		TSharedPtr<FJsonObject> DoorBeforeObj;
		if (DoorBeforeRoot.IsValid() && DoorBeforeRoot->TryGetObjectField(TEXT("access_door"), DoorSnap) && DoorSnap)
		{
			DoorBeforeObj = *DoorSnap;
		}

		TArray<FActorPropertyMutation> Mutations;
		FString ParseError;
		if (!ParseMutations(Change.Args, Mutations, ParseError))
		{
			return FailAudit(TEXT("bad_args"), ParseError, MakeShared<FBridgeChange>(Change));
		}

		AActor* Door = FindUniqueLabel(World, GetString(Change.Args, TEXT("door_label"), AccessDoorLabel));
		const FString DoorBeforeError = DoorDelta(Door, DoorBeforeObj);
		if (!DoorBeforeError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("door_changed"), FString::Printf(TEXT("%s ZERO writes."), *DoorBeforeError), MakeShared<FBridgeChange>(Change));
		}

		TArray<TPair<FProperty*, FString>> Applied;
		FString SetError;
		Actor->Modify();
		for (const FActorPropertyMutation& Mutation : Mutations)
		{
			FProperty* Property = FindInstanceProperty(Actor, Mutation.PropertyName);
			if (!SetNameOrTextValue(Actor, Property, Mutation.NewValue, SetError))
			{
				for (int32 Index = Applied.Num() - 1; Index >= 0; --Index)
				{
					FString RestoreError;
					SetNameOrTextValue(Actor, Applied[Index].Key, Applied[Index].Value, RestoreError);
				}
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("set_failed"),
					FString::Printf(TEXT("%s Rolled back prior mutations in this change. Maps not saved."), *SetError),
					MakeShared<FBridgeChange>(Change));
			}
			Applied.Add(TPair<FProperty*, FString>(Property, Mutation.Expected));
		}

		TArray<TSharedPtr<FJsonValue>> AfterMutations;
		for (const FActorPropertyMutation& Mutation : Mutations)
		{
			FProperty* Property = FindInstanceProperty(Actor, Mutation.PropertyName);
			const FString Live = ReadNameOrTextValue(Actor, Property);
			if (!Live.Equals(Mutation.NewValue, ESearchCase::CaseSensitive))
			{
				for (int32 Index = Applied.Num() - 1; Index >= 0; --Index)
				{
					FString RestoreError;
					SetNameOrTextValue(Actor, Applied[Index].Key, Applied[Index].Value, RestoreError);
				}
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("verify_failed"),
					FString::Printf(TEXT("After set, '%s' is '%s' expected '%s'. Rolled back. Maps not saved."),
						*Mutation.PropertyName, *Live, *Mutation.NewValue),
					MakeShared<FBridgeChange>(Change));
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("property"), Mutation.PropertyName);
			Row->SetStringField(TEXT("value"), Live);
			AfterMutations.Add(MakeShared<FJsonValueObject>(Row));
		}

		const FString DoorAfterError = DoorDelta(Door, DoorBeforeObj);
		if (!DoorAfterError.IsEmpty())
		{
			for (int32 Index = Applied.Num() - 1; Index >= 0; --Index)
			{
				FString RestoreError;
				SetNameOrTextValue(Actor, Applied[Index].Key, Applied[Index].Value, RestoreError);
			}
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("door_changed"),
				FString::Printf(TEXT("%s Rolled back property mutations. Maps not saved."), *DoorAfterError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Actor));
		Change.After->SetArrayField(TEXT("mutations"), AfterMutations);
		Change.After->SetObjectField(TEXT("access_door"), SnapshotDoorComponents(Door));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("Instance FName/FText properties set on game thread. Maps not saved. Blueprint not compiled."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> CmdExecuteWrite(const TSharedPtr<FJsonObject>& Args, const TSharedPtr<FJsonObject>& Session)
	{
		const bool bReadOnlySession = !Session.IsValid() || GetBool(Session, TEXT("read_only"), true);
		if (bReadOnlySession)
		{
			return Fail(TEXT("read_only"), TEXT("execute_write requires session.read_only=false. Dual approval is still required."));
		}

		const FString ChangeId = GetString(Args, TEXT("change_id"), GetString(Session, TEXT("change_id")));
		TSharedPtr<FBridgeChange> Change = FindChange(ChangeId);
		if (!Change.IsValid())
		{
			return Fail(TEXT("not_found"), TEXT("Unknown change_id. Prepare and dual-approve first."));
		}
		if (Change->Status == TEXT("rejected"))
		{
			return FailAudit(TEXT("rejected"), TEXT("Change was rejected. ZERO writes."), Change);
		}
		if (Change->bExecuted)
		{
			return FailAudit(TEXT("already_executed"), TEXT("Change already executed."), Change);
		}
		if (!Change->User.bApproved || !Change->SecondReview.bApproved)
		{
			return FailAudit(
				TEXT("approval_required"),
				TEXT("Both user and second-review approvals are required on this change_id before execute."),
				Change);
		}
		if (Change->User.Identity.Equals(Change->SecondReview.Identity, ESearchCase::IgnoreCase))
		{
			return FailAudit(TEXT("identity_reuse"), TEXT("Dual approval identities must differ. ZERO writes."), Change);
		}

		const FString SessionPackage = GetString(Session, TEXT("required_world_package"));
		if (!SessionPackage.IsEmpty() && !PackagesEqual(SessionPackage, Change->Package))
		{
			const bool bBlueprintTemplate = GetString(Change->Args, TEXT("scope"))
				.Equals(TEXT("blueprint_template"), ESearchCase::IgnoreCase);
			const bool bBlueprintVariable = Change->Action == TEXT("add_blueprint_variable")
				|| Change->Action == TEXT("add_scs_component")
				|| Change->Action == TEXT("author_hologram_apply_state")
				|| Change->Action == TEXT("author_light_controller_set_zone")
				|| Change->Action == TEXT("author_admin_room_trigger_lighting_hook")
				|| Change->Action == TEXT("author_access_door_facility_state_listener")
				|| Change->Action == TEXT("author_light_controller_facility_state_listener");
			const bool bBlueprintGraph = Change->Action == TEXT("connect_blueprint_pins")
				|| Change->Action == TEXT("compile_blueprint")
				|| Change->Action == TEXT("save_asset");
			const bool bAdminSession = PackagesEqual(SessionPackage, AdminPackage);
			const bool bMoveOrSaveMaps = Change->Action == TEXT("move_actor_to_level")
				|| Change->Action == TEXT("save_maps")
				|| Change->Action == TEXT("set_actor_property")
				|| Change->Action == TEXT("spawn_blueprint_actor")
				|| Change->Action == TEXT("spawn_s20_zone_light")
				|| Change->Action == TEXT("set_s20_light_intensity")
				|| Change->Action == TEXT("invoke_s20_set_lighting_zone")
				|| Change->Action == TEXT("delete_s22_legacy_admin_ambience")
				|| Change->Action == TEXT("spawn_s22_admin_audio_zones")
				|| Change->Action == TEXT("spawn_neuro_navmesh_bounds")
				|| Change->Action == TEXT("spawn_neuro_research_station")
				|| Change->Action == TEXT("spawn_admin_research_wing_connector")
				|| Change->Action == TEXT("spawn_admin_research_wing_keycard")
				|| Change->Action == TEXT("trim_spine_landing_admin")
				|| Change->Action == TEXT("set_admin_doorlock_interactable")
				|| Change->Action == TEXT("spawn_admin_block2_dressing")
				|| Change->Action == TEXT("spawn_admin_block3_resources")
				|| Change->Action == TEXT("spawn_admin_block4_security_officer")
				|| Change->Action == TEXT("spawn_admin_block4_navmesh_bounds")
				|| Change->Action == TEXT("save_admin_block4_navmesh_prerequisite")
				|| Change->Action == TEXT("spawn_neuro_arrival_lab_dressing")
				|| Change->Action == TEXT("spawn_neuro_ch3_containment_evidence")
				|| Change->Action == TEXT("spawn_neuro_ch4_transformed_personnel")
				|| Change->Action == TEXT("configure_neuro_power_failure_discovery")
				|| Change->Action == TEXT("spawn_neuro_power_diagnostic")
				|| Change->Action == TEXT("create_neurogenetics_mission")
				|| Change->Action == TEXT("expand_neurogenetics_mission_beat3")
				|| Change->Action == TEXT("expand_neurogenetics_mission_beat4")
				|| Change->Action == TEXT("expand_neurogenetics_mission_beat5")
				|| Change->Action == TEXT("expand_neurogenetics_mission_beat6")
				|| Change->Action == TEXT("create_neuro_power_restore_mission")
				|| Change->Action == TEXT("set_neurogenetics_next_mission_power_restore")
				|| Change->Action == TEXT("configure_neuro_backup_power_restore")
				|| Change->Action == TEXT("create_neuro_targeting_why_mission")
				|| Change->Action == TEXT("set_neuro_power_restore_next_targeting_why")
				|| Change->Action == TEXT("configure_neuro_researcher_targeting_why")
				|| Change->Action == TEXT("create_neuro_research_station_intro_mission")
				|| Change->Action == TEXT("set_neuro_targeting_why_next_research_station")
				|| Change->Action == TEXT("configure_neuro_research_station_intro")
				|| Change->Action == TEXT("create_neural_slow_adaptation_asset")
				|| Change->Action == TEXT("create_neuro_neural_slow_use_mission")
				|| Change->Action == TEXT("set_neuro_research_station_intro_next_neural_slow")
				|| Change->Action == TEXT("create_neuro_adaptation_connection_mission")
				|| Change->Action == TEXT("set_neuro_neural_slow_use_next_adaptation_connection")
				|| Change->Action == TEXT("configure_neuro_live_adaptation_connection")
				|| Change->Action == TEXT("create_neuro_revelation_mission")
				|| Change->Action == TEXT("set_neuro_adaptation_connection_next_revelation")
				|| Change->Action == TEXT("configure_neuro_revelation_observation")
				|| Change->Action == TEXT("create_cryo_access_mission")
				|| Change->Action == TEXT("set_neuro_revelation_next_cryo_access")
				|| Change->Action == TEXT("create_cryo_entry_mission")
				|| Change->Action == TEXT("set_cryo_access_next_cryo_entry")
				|| Change->Action == TEXT("configure_cryo_entry_checkpoint")
				|| Change->Action == TEXT("create_cryo_evidence_mission")
				|| Change->Action == TEXT("set_cryo_entry_next_cryo_evidence")
				|| Change->Action == TEXT("configure_cryo_evidence_datapads")
				|| Change->Action == TEXT("create_compute_entry_mission")
				|| Change->Action == TEXT("set_cryo_evidence_next_compute_entry")
				|| Change->Action == TEXT("configure_compute_entry_checkpoint")
				|| Change->Action == TEXT("create_compute_handover_mission")
				|| Change->Action == TEXT("set_compute_entry_next_compute_handover")
				|| Change->Action == TEXT("configure_compute_handover_terminals_and_datapad")
				|| Change->Action == TEXT("create_the_conclusion_mission")
				|| Change->Action == TEXT("set_compute_handover_next_the_conclusion")
				|| Change->Action == TEXT("configure_reactor_control_spine")
				|| Change->Action == TEXT("create_research_station_mission")
				|| Change->Action == TEXT("set_the_conclusion_next_research_station")
				|| Change->Action == TEXT("configure_research_station")
				|| Change->Action == TEXT("create_locomotor_disrupt_adaptation")
				|| Change->Action == TEXT("create_optical_disrupt_adaptation")
				|| Change->Action == TEXT("create_syringe_kit_mission")
				|| Change->Action == TEXT("set_research_station_next_syringe_kit")
				|| Change->Action == TEXT("configure_research_station_syringe_kit")
				|| Change->Action == TEXT("create_nathan_grant_look")
				|| Change->Action == TEXT("configure_cryo_backup_power_panel")
				|| Change->Action == TEXT("spawn_neuro_adaptation_subject")
				|| Change->Action == TEXT("spawn_neuro_neural_mapping_array")
				|| Change->Action == TEXT("spawn_neuro_research_load_cutoff")
				|| Change->Action == TEXT("spawn_neuro_neural_mapping_terminal")
				|| Change->Action == TEXT("spawn_neuro_neural_signature_observation_node")
				|| Change->Action == TEXT("spawn_neuro_neural_change_evidence_instrument");
			const bool bEpitopeOrAdminSession = PackagesEqual(SessionPackage, AdminPackage)
				|| PackagesEqual(SessionPackage, EpitopePackage);
			const bool bNeuroSession = PackagesEqual(SessionPackage, NeuroPackage)
				|| PackagesEqual(SessionPackage, MainMenuPackage)
				|| PackagesEqual(Change->Package, NeuroPackage)
				|| PackagesEqual(SessionPackage, CryoPackage)
				|| PackagesEqual(Change->Package, CryoPackage)
				|| PackagesEqual(SessionPackage, ComputePackage)
				|| PackagesEqual(Change->Package, ComputePackage);
			if (!((bBlueprintTemplate || bBlueprintVariable || bBlueprintGraph) && bAdminSession)
				&& !(bMoveOrSaveMaps && (bEpitopeOrAdminSession || bNeuroSession)))
			{
				return FailAudit(TEXT("wrong_package"), TEXT("session.required_world_package does not match the approved package. ZERO writes."), Change);
			}
		}

		if (Change->Action == TEXT("delete_actor"))
		{
			return ExecuteDelete(*Change);
		}
		if (Change->Action == TEXT("save_asset"))
		{
			return ExecuteSaveAsset(*Change);
		}
		if (Change->Action == TEXT("save_maps"))
		{
			return ExecuteSaveMaps(*Change);
		}
		if (Change->Action == TEXT("move_actor_to_level"))
		{
			return ExecuteMoveActorToLevel(*Change);
		}
		if (Change->Action == TEXT("set_actor_property"))
		{
			return ExecuteSetActorProperty(*Change);
		}
		if (Change->Action == TEXT("compile_blueprint"))
		{
			return ExecuteCompileBlueprint(*Change);
		}
		if (Change->Action == TEXT("rerun_construction"))
		{
			return ExecuteRerunConstruction(*Change);
		}
		if (Change->Action == TEXT("add_blueprint_variable"))
		{
			const FString PinCategory = GetString(Change->Args, TEXT("pin_category")).ToLower();
			const FString Spec = GetString(Change->Args, TEXT("spec"));
			if (PinCategory == TEXT("bool") || PinCategory == TEXT("boolean") || Spec == TEXT("s19_facility_hologram_bools_v1"))
			{
				return ExecuteAddHologramBoolVariables(*Change);
			}
			if (Spec == TEXT("s20_admin_lighting_v1"))
			{
				return ExecuteAddLightControllerVariables(*Change);
			}
			return ExecuteAddBlueprintVariable(*Change);
		}
		if (Change->Action == TEXT("add_scs_component"))
		{
			return ExecuteAddScsComponent(*Change);
		}
		if (Change->Action == TEXT("author_hologram_apply_state"))
		{
			return ExecuteAuthorHologramApplyState(*Change);
		}
		if (Change->Action == TEXT("author_light_controller_set_zone"))
		{
			return ExecuteAuthorLightControllerSetZone(*Change);
		}
		if (Change->Action == TEXT("author_admin_room_trigger_lighting_hook"))
		{
			return ExecuteAuthorAdminRoomTriggerLightingHook(*Change);
		}
		if (Change->Action == TEXT("author_access_door_facility_state_listener"))
		{
			return ExecuteAuthorAccessDoorFacilityStateListener(*Change);
		}
		if (Change->Action == TEXT("author_light_controller_facility_state_listener"))
		{
			return ExecuteAuthorLightControllerFacilityStateListener(*Change);
		}
		if (Change->Action == TEXT("invoke_s20_set_lighting_zone"))
		{
			return ExecuteInvokeS20SetLightingZone(*Change);
		}
		if (Change->Action == TEXT("delete_s22_legacy_admin_ambience"))
		{
			return ExecuteDeleteS22LegacyAdminAmbience(*Change);
		}
		if (Change->Action == TEXT("spawn_s22_admin_audio_zones"))
		{
			return ExecuteSpawnS22AdminAudioZones(*Change);
		}
		if (Change->Action == TEXT("connect_blueprint_pins"))
		{
			return ExecuteConnectBlueprintPins(*Change);
		}
		if (Change->Action == TEXT("spawn_blueprint_actor"))
		{
			return ExecuteSpawnBlueprintActor(*Change);
		}
		if (Change->Action == TEXT("spawn_s20_zone_light"))
		{
			return ExecuteSpawnS20ZoneLight(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_navmesh_bounds"))
		{
			return ExecuteSpawnNeuroNavMeshBounds(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_research_station"))
		{
			return ExecuteSpawnNeuroResearchStation(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_research_wing_connector"))
		{
			return ExecuteSpawnAdminResearchWingConnector(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_research_wing_keycard"))
		{
			return ExecuteSpawnAdminResearchWingKeycard(*Change);
		}
		if (Change->Action == TEXT("trim_spine_landing_admin"))
		{
			return ExecuteTrimSpineLandingAdmin(*Change);
		}
		if (Change->Action == TEXT("set_admin_doorlock_interactable"))
		{
			return ExecuteSetAdminDoorlockInteractable(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_block2_dressing"))
		{
			return ExecuteSpawnAdminBlock2Dressing(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_block3_resources"))
		{
			return ExecuteSpawnAdminBlock3Resources(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_block4_security_officer"))
		{
			return ExecuteSpawnAdminBlock4SecurityOfficer(*Change);
		}
		if (Change->Action == TEXT("spawn_admin_block4_navmesh_bounds"))
		{
			return ExecuteSpawnAdminBlock4NavMeshBounds(*Change);
		}
		if (Change->Action == TEXT("save_admin_block4_navmesh_prerequisite"))
		{
			return ExecuteSaveAdminBlock4NavMeshPrerequisite(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_arrival_lab_dressing"))
		{
			return ExecuteSpawnNeuroArrivalLabDressing(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_ch3_containment_evidence"))
		{
			return ExecuteSpawnNeuroCh3ContainmentEvidence(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_ch4_transformed_personnel"))
		{
			return ExecuteSpawnNeuroCh4TransformedPersonnel(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_power_failure_discovery"))
		{
			return ExecuteConfigureNeuroPowerFailureDiscovery(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_power_diagnostic"))
		{
			return ExecuteSpawnNeuroPowerDiagnostic(*Change);
		}
		if (Change->Action == TEXT("create_neurogenetics_mission"))
		{
			return ExecuteCreateNeuroGeneticsMission(*Change);
		}
		if (Change->Action == TEXT("expand_neurogenetics_mission_beat3"))
		{
			return ExecuteExpandNeuroGeneticsMissionBeat3(*Change);
		}
		if (Change->Action == TEXT("expand_neurogenetics_mission_beat4"))
		{
			return ExecuteExpandNeuroGeneticsMissionBeat4(*Change);
		}
		if (Change->Action == TEXT("expand_neurogenetics_mission_beat5"))
		{
			return ExecuteExpandNeuroGeneticsMissionBeat5(*Change);
		}
		if (Change->Action == TEXT("expand_neurogenetics_mission_beat6"))
		{
			return ExecuteExpandNeuroGeneticsMissionBeat6(*Change);
		}
		if (Change->Action == TEXT("create_neuro_power_restore_mission"))
		{
			return ExecuteCreateNeuroPowerRestoreMission(*Change);
		}
		if (Change->Action == TEXT("set_neurogenetics_next_mission_power_restore"))
		{
			return ExecuteSetNeuroGeneticsNextMissionPowerRestore(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_backup_power_restore"))
		{
			return ExecuteConfigureNeuroBackupPowerRestore(*Change);
		}
		if (Change->Action == TEXT("create_neuro_targeting_why_mission"))
		{
			return ExecuteCreateNeuroTargetingWhyMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_power_restore_next_targeting_why"))
		{
			return ExecuteSetNeuroPowerRestoreNextTargetingWhy(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_researcher_targeting_why"))
		{
			return ExecuteConfigureNeuroResearcherTargetingWhy(*Change);
		}
		if (Change->Action == TEXT("create_neuro_research_station_intro_mission"))
		{
			return ExecuteCreateNeuroResearchStationIntroMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_targeting_why_next_research_station"))
		{
			return ExecuteSetNeuroTargetingWhyNextResearchStation(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_research_station_intro"))
		{
			return ExecuteConfigureNeuroResearchStationIntro(*Change);
		}
		if (Change->Action == TEXT("create_neural_slow_adaptation_asset"))
		{
			return ExecuteCreateNeuralSlowAdaptationAsset(*Change);
		}
		if (Change->Action == TEXT("create_neuro_neural_slow_use_mission"))
		{
			return ExecuteCreateNeuroNeuralSlowUseMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_research_station_intro_next_neural_slow"))
		{
			return ExecuteSetNeuroResearchStationIntroNextNeuralSlow(*Change);
		}
		if (Change->Action == TEXT("create_neuro_adaptation_connection_mission"))
		{
			return ExecuteCreateNeuroAdaptationConnectionMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_neural_slow_use_next_adaptation_connection"))
		{
			return ExecuteSetNeuroNeuralSlowUseNextAdaptationConnection(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_live_adaptation_connection"))
		{
			return ExecuteConfigureNeuroLiveAdaptationConnection(*Change);
		}
		if (Change->Action == TEXT("create_neuro_revelation_mission"))
		{
			return ExecuteCreateNeuroRevelationMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_adaptation_connection_next_revelation"))
		{
			return ExecuteSetNeuroAdaptationConnectionNextRevelation(*Change);
		}
		if (Change->Action == TEXT("configure_neuro_revelation_observation"))
		{
			return ExecuteConfigureNeuroRevelationObservation(*Change);
		}
		if (Change->Action == TEXT("create_cryo_access_mission"))
		{
			return ExecuteCreateCryoAccessMission(*Change);
		}
		if (Change->Action == TEXT("set_neuro_revelation_next_cryo_access"))
		{
			return ExecuteSetNeuroRevelationNextCryoAccess(*Change);
		}
		if (Change->Action == TEXT("configure_cryo_backup_power_panel"))
		{
			return ExecuteConfigureCryoBackupPowerPanel(*Change);
		}
		if (Change->Action == TEXT("create_cryo_entry_mission"))
		{
			return ExecuteCreateCryoEntryMission(*Change);
		}
		if (Change->Action == TEXT("set_cryo_access_next_cryo_entry"))
		{
			return ExecuteSetCryoAccessNextCryoEntry(*Change);
		}
		if (Change->Action == TEXT("configure_cryo_entry_checkpoint"))
		{
			return ExecuteConfigureCryoEntryCheckpoint(*Change);
		}
		if (Change->Action == TEXT("create_cryo_evidence_mission"))
		{
			return ExecuteCreateCryoEvidenceMission(*Change);
		}
		if (Change->Action == TEXT("set_cryo_entry_next_cryo_evidence"))
		{
			return ExecuteSetCryoEntryNextCryoEvidence(*Change);
		}
		if (Change->Action == TEXT("configure_cryo_evidence_datapads"))
		{
			return ExecuteConfigureCryoEvidenceDatapads(*Change);
		}
		if (Change->Action == TEXT("create_compute_entry_mission"))
		{
			return ExecuteCreateComputeEntryMission(*Change);
		}
		if (Change->Action == TEXT("set_cryo_evidence_next_compute_entry"))
		{
			return ExecuteSetCryoEvidenceNextComputeEntry(*Change);
		}
		if (Change->Action == TEXT("configure_compute_entry_checkpoint"))
		{
			return ExecuteConfigureComputeEntryCheckpoint(*Change);
		}
		if (Change->Action == TEXT("create_compute_handover_mission"))
		{
			return ExecuteCreateComputeHandoverMission(*Change);
		}
		if (Change->Action == TEXT("set_compute_entry_next_compute_handover"))
		{
			return ExecuteSetComputeEntryNextComputeHandover(*Change);
		}
		if (Change->Action == TEXT("configure_compute_handover_terminals_and_datapad"))
		{
			return ExecuteConfigureComputeHandoverActors(*Change);
		}
		if (Change->Action == TEXT("create_the_conclusion_mission"))
		{
			return ExecuteCreateTheConclusionMission(*Change);
		}
		if (Change->Action == TEXT("set_compute_handover_next_the_conclusion"))
		{
			return ExecuteSetComputeHandoverNextTheConclusion(*Change);
		}
		if (Change->Action == TEXT("configure_reactor_control_spine"))
		{
			return ExecuteConfigureReactorControlSpine(*Change);
		}
		if (Change->Action == TEXT("create_research_station_mission"))
		{
			return ExecuteCreateRespecBeatMission(*Change);
		}
		if (Change->Action == TEXT("set_the_conclusion_next_research_station"))
		{
			return ExecuteSetTheConclusionNextResearchStation(*Change);
		}
		if (Change->Action == TEXT("configure_research_station"))
		{
			return ExecuteConfigureResearchStation(*Change);
		}
		if (Change->Action == TEXT("create_locomotor_disrupt_adaptation"))
		{
			return ExecuteCreateLocomotorDisruptAdaptation(*Change);
		}
		if (Change->Action == TEXT("create_optical_disrupt_adaptation"))
		{
			return ExecuteCreateOpticalDisruptAdaptation(*Change);
		}
		if (Change->Action == TEXT("create_syringe_kit_mission"))
		{
			return ExecuteCreateSyringeKitMission(*Change);
		}
		if (Change->Action == TEXT("set_research_station_next_syringe_kit"))
		{
			return ExecuteSetResearchStationNextSyringeKit(*Change);
		}
		if (Change->Action == TEXT("configure_research_station_syringe_kit"))
		{
			return ExecuteConfigureResearchStationSyringeKit(*Change);
		}
		if (Change->Action == TEXT("create_nathan_grant_look"))
		{
			return ExecuteCreateNathanGrantLook(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_adaptation_subject"))
		{
			return ExecuteSpawnNeuroAdaptationSubject(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_neural_mapping_array"))
		{
			return ExecuteSpawnNeuroNeuralMappingArray(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_research_load_cutoff"))
		{
			return ExecuteSpawnNeuroResearchLoadCutoff(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_neural_mapping_terminal"))
		{
			return ExecuteSpawnNeuroNeuralMappingTerminal(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_neural_signature_observation_node"))
		{
			return ExecuteSpawnNeuroNeuralSignatureObservationNode(*Change);
		}
		if (Change->Action == TEXT("spawn_neuro_neural_change_evidence_instrument"))
		{
			return ExecuteSpawnNeuroNeuralChangeEvidenceInstrument(*Change);
		}
		if (Change->Action == TEXT("set_s20_light_intensity"))
		{
			return ExecuteSetS20LightIntensity(*Change);
		}
		if (Change->Action == TEXT("set_component_property")
			&& GetString(Change->Args, TEXT("scope")).Equals(TEXT("blueprint_template"), ESearchCase::IgnoreCase))
		{
			return ExecuteBlueprintAccessTriggerTemplate(*Change);
		}
		if (Change->Action == TEXT("set_visibility")
			|| Change->Action == TEXT("set_collision")
			|| Change->Action == TEXT("set_transform")
			|| Change->Action == TEXT("set_component_property"))
		{
			return ExecuteComponentMutations(*Change);
		}
		return FailAudit(TEXT("action_not_allowlisted"), TEXT("Action cannot be executed."), Change);
	}
}

namespace OrganoidAIBridgeWrites
{
	bool IsLifecycleCommand(const FString& NormalizedCommand)
	{
		return NormalizedCommand == TEXT("prepare_write")
			|| NormalizedCommand == TEXT("approve_write")
			|| NormalizedCommand == TEXT("reject_write")
			|| NormalizedCommand == TEXT("get_change")
			|| NormalizedCommand == TEXT("list_changes")
			|| NormalizedCommand == TEXT("execute_write");
	}

	TSharedRef<FJsonObject> InspectAdminBlock4NavMesh(const TSharedPtr<FJsonObject>& Args)
	{
		return CmdInspectAdminBlock4NavMesh(Args);
	}

	TSharedRef<FJsonObject> Dispatch(
		const FString& NormalizedCommand,
		const TSharedPtr<FJsonObject>& Args,
		const TSharedPtr<FJsonObject>& Session,
		FOrganoidAIBridgeLogSink* /*LogSink*/)
	{
		if (NormalizedCommand == TEXT("prepare_write"))
		{
			return CmdPrepareWrite(Args);
		}
		if (NormalizedCommand == TEXT("approve_write"))
		{
			return CmdApproveWrite(Args);
		}
		if (NormalizedCommand == TEXT("reject_write"))
		{
			return CmdRejectWrite(Args);
		}
		if (NormalizedCommand == TEXT("get_change"))
		{
			return CmdGetChange(Args);
		}
		if (NormalizedCommand == TEXT("list_changes"))
		{
			return CmdListChanges();
		}
		if (NormalizedCommand == TEXT("execute_write"))
		{
			return CmdExecuteWrite(Args, Session);
		}
		return Fail(TEXT("unknown_command"), TEXT("Write lifecycle command not recognized."));
	}
}
