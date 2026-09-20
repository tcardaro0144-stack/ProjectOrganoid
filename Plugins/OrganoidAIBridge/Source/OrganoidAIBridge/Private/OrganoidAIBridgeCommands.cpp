#include "OrganoidAIBridgeCommands.h"
#include "OrganoidAIBridgeJson.h"
#include "OrganoidAIBridgeLogSink.h"
#include "OrganoidAIBridgePlaytest.h"
#include "OrganoidAIBridgeWrites.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectIterator.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/LevelStreaming.h"
#include "Engine/Level.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/OverlapResult.h"
#include "Engine/SCS_Node.h"
#include "Engine/Selection.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_Timeline.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"

using namespace OrganoidAIBridgeJson;

namespace
{
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

	UWorld* GetPreferredWorld(bool bPreferPie)
	{
		if (bPreferPie)
		{
			if (UWorld* Pie = GetPieWorld())
			{
				return Pie;
			}
		}
		return GetEditorWorld();
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

	FString ClassPath(const UObject* Object)
	{
		return Object && Object->GetClass() ? Object->GetClass()->GetPathName() : TEXT("");
	}

	FString CollisionEnabledName(ECollisionEnabled::Type Value)
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

	FString CollisionResponseName(ECollisionResponse Value)
	{
		switch (Value)
		{
		case ECR_Ignore: return TEXT("Ignore");
		case ECR_Overlap: return TEXT("Overlap");
		case ECR_Block: return TEXT("Block");
		default: return TEXT("Unknown");
		}
	}

	void PutActor(TSharedRef<FJsonObject> Out, AActor* Actor)
	{
		if (!Actor)
		{
			Out->SetField(TEXT("actor"), MakeShared<FJsonValueNull>());
			return;
		}
		Out->SetStringField(TEXT("label"), ActorLabel(Actor));
		Out->SetStringField(TEXT("name"), Actor->GetName());
		Out->SetStringField(TEXT("class"), ClassName(Actor));
		Out->SetStringField(TEXT("class_path"), ClassPath(Actor));
		Out->SetArrayField(TEXT("location"), Vec(Actor->GetActorLocation()));
		Out->SetArrayField(TEXT("rotation"), Rot(Actor->GetActorRotation()));
		Out->SetArrayField(TEXT("scale"), Vec(Actor->GetActorScale3D()));
	}

	void PutComponentCollision(TSharedRef<FJsonObject> Out, UPrimitiveComponent* Primitive)
	{
		if (!Primitive)
		{
			return;
		}
		Out->SetStringField(TEXT("component"), Primitive->GetName());
		Out->SetStringField(TEXT("component_class"), ClassName(Primitive));
		Out->SetStringField(TEXT("collision_enabled"), CollisionEnabledName(Primitive->GetCollisionEnabled()));
		Out->SetStringField(TEXT("collision_profile"), Primitive->GetCollisionProfileName().ToString());
		Out->SetStringField(TEXT("object_type"), UEnum::GetValueAsString(Primitive->GetCollisionObjectType()));
		Out->SetStringField(TEXT("pawn_response"), CollisionResponseName(Primitive->GetCollisionResponseToChannel(ECC_Pawn)));
		const FBoxSphereBounds Bounds = Primitive->Bounds;
		Out->SetArrayField(TEXT("bounds_origin"), Vec(Bounds.Origin));
		Out->SetArrayField(TEXT("bounds_extent"), Vec(Bounds.BoxExtent));
		Out->SetNumberField(TEXT("west_x"), Bounds.Origin.X - FMath::Abs(Bounds.BoxExtent.X));
		Out->SetNumberField(TEXT("east_x"), Bounds.Origin.X + FMath::Abs(Bounds.BoxExtent.X));
		if (const UBoxComponent* Box = Cast<UBoxComponent>(Primitive))
		{
			Out->SetArrayField(TEXT("box_extent"), Vec(Box->GetUnscaledBoxExtent()));
		}
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Primitive))
		{
			Out->SetNumberField(TEXT("capsule_radius"), Capsule->GetUnscaledCapsuleRadius());
			Out->SetNumberField(TEXT("capsule_half_height"), Capsule->GetUnscaledCapsuleHalfHeight());
		}
		Out->SetArrayField(TEXT("relative_location"), Vec(Primitive->GetRelativeLocation()));
	}

	AActor* FindActor(UWorld* World, const FString& Query)
	{
		if (!World || Query.IsEmpty())
		{
			return nullptr;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}
			if (Actor->GetName().Equals(Query, ESearchCase::IgnoreCase)
				|| ActorLabel(Actor).Equals(Query, ESearchCase::IgnoreCase)
				|| ClassName(Actor).Contains(Query)
				|| Actor->GetPathName().Contains(Query))
			{
				return Actor;
			}
		}
		return nullptr;
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

	TArray<AActor*> FindUniqueActorCandidates(UWorld* World, const FString& Query)
	{
		TArray<AActor*> Matches;
		if (!World || Query.IsEmpty())
		{
			return Matches;
		}
		TSet<AActor*> Seen;
		const bool bPathQuery = Query.Contains(TEXT("/")) || Query.Contains(TEXT("."));
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}
			const bool bLabel = ActorLabel(Actor).Equals(Query, ESearchCase::CaseSensitive);
			const bool bName = Actor->GetName().Equals(Query, ESearchCase::CaseSensitive);
			const bool bExactPath = Actor->GetPathName().Equals(Query, ESearchCase::CaseSensitive);
			const bool bPathContains = bPathQuery && Actor->GetPathName().Contains(Query);
			if (bLabel || bName || bExactPath || bPathContains)
			{
				if (!Seen.Contains(Actor))
				{
					Seen.Add(Actor);
					Matches.Add(Actor);
				}
			}
		}
		return Matches;
	}

	FProperty* FindInstanceProperty(UObject* Object, const FString& PropertyName)
	{
		if (!Object || PropertyName.IsEmpty() || !Object->GetClass())
		{
			return nullptr;
		}
		if (FProperty* Exact = FindFProperty<FProperty>(Object->GetClass(), FName(*PropertyName)))
		{
			return Exact;
		}
		TArray<FProperty*> CaseInsensitive;
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (Property && Property->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
			{
				CaseInsensitive.Add(Property);
			}
		}
		return CaseInsensitive.Num() == 1 ? CaseInsensitive[0] : nullptr;
	}

	void PutEnumFields(TSharedRef<FJsonObject> Out, const UEnum* Enum, int64 Value)
	{
		Out->SetNumberField(TEXT("numeric_value"), static_cast<double>(Value));
		if (!Enum)
		{
			Out->SetField(TEXT("internal_name"), MakeShared<FJsonValueNull>());
			Out->SetField(TEXT("display_name"), MakeShared<FJsonValueNull>());
			Out->SetField(TEXT("enum_type"), MakeShared<FJsonValueNull>());
			return;
		}
		Out->SetStringField(TEXT("internal_name"), Enum->GetNameStringByValue(Value));
		Out->SetStringField(TEXT("display_name"), Enum->GetDisplayNameTextByValue(Value).ToString());
		Out->SetStringField(TEXT("enum_type"), Enum->GetPathName());
	}

	TSharedRef<FJsonObject> CmdGetActorProperty(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Query = GetString(Args, TEXT("actor"),
			GetString(Args, TEXT("name"),
				GetString(Args, TEXT("label"), GetString(Args, TEXT("path")))));
		const FString PropertyName = GetString(Args, TEXT("property"), GetString(Args, TEXT("property_name")));
		if (Query.IsEmpty())
		{
			return Fail(TEXT("bad_args"), TEXT("actor label or unique actor path is required."));
		}
		if (PropertyName.IsEmpty())
		{
			return Fail(TEXT("bad_args"), TEXT("property name is required."));
		}

		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), false));
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No editor world."));
		}
		TArray<AActor*> Matches = FindUniqueActorCandidates(World, Query);
		if (Matches.Num() == 0)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("No actor matching '%s'."), *Query));
		}
		if (Matches.Num() != 1)
		{
			return Fail(
				TEXT("not_unique"),
				FString::Printf(TEXT("Actor query '%s' matched %d actors. Need exactly 1."), *Query, Matches.Num()));
		}

		AActor* Actor = Matches[0];
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (!Property)
		{
			return Fail(
				TEXT("property_not_found"),
				FString::Printf(TEXT("Property '%s' not found on instance %s."), *PropertyName, *Actor->GetPathName()));
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Data->SetStringField(TEXT("label"), ActorLabel(Actor));
		Data->SetStringField(TEXT("name"), Actor->GetName());
		Data->SetStringField(TEXT("class"), ClassName(Actor));
		Data->SetStringField(TEXT("path"), Actor->GetPathName());
		Data->SetStringField(TEXT("owning_package"), ActorOwningPackage(Actor));
		Data->SetStringField(TEXT("property"), Property->GetName());
		Data->SetStringField(TEXT("property_class"), Property->GetClass()->GetName());
		Data->SetBoolField(TEXT("from_cdo"), false);

		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			const bool bValue = BoolProp->GetPropertyValue_InContainer(Actor);
			Data->SetStringField(TEXT("value_type"), TEXT("bool"));
			Data->SetBoolField(TEXT("value"), bValue);
		}
		else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			const void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Actor);
			const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			Data->SetStringField(TEXT("value_type"), TEXT("enum"));
			Data->SetNumberField(TEXT("value"), static_cast<double>(Value));
			PutEnumFields(Data, EnumProp->GetEnum(), Value);
		}
		else if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			const uint8 Value = ByteProp->GetPropertyValue_InContainer(Actor);
			UEnum* Enum = ByteProp->GetIntPropertyEnum();
			if (Enum)
			{
				Data->SetStringField(TEXT("value_type"), TEXT("enum"));
				Data->SetNumberField(TEXT("value"), static_cast<double>(Value));
				PutEnumFields(Data, Enum, Value);
			}
			else
			{
				Data->SetStringField(TEXT("value_type"), TEXT("byte"));
				Data->SetNumberField(TEXT("value"), static_cast<double>(Value));
				Data->SetNumberField(TEXT("numeric_value"), static_cast<double>(Value));
			}
		}
		else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			const FName Value = NameProp->GetPropertyValue_InContainer(Actor);
			Data->SetStringField(TEXT("value_type"), TEXT("name"));
			Data->SetStringField(TEXT("value"), Value.ToString());
		}
		else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			const FString Value = StrProp->GetPropertyValue_InContainer(Actor);
			Data->SetStringField(TEXT("value_type"), TEXT("string"));
			Data->SetStringField(TEXT("value"), Value);
		}
		else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			const FText Value = TextProp->GetPropertyValue_InContainer(Actor);
			Data->SetStringField(TEXT("value_type"), TEXT("text"));
			Data->SetStringField(TEXT("value"), Value.ToString());
		}
		else if (const FNumericProperty* NumProp = CastField<FNumericProperty>(Property))
		{
			Data->SetStringField(TEXT("value_type"), TEXT("number"));
			if (NumProp->IsFloatingPoint())
			{
				const double Value = NumProp->GetFloatingPointPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Actor));
				Data->SetNumberField(TEXT("value"), Value);
				Data->SetNumberField(TEXT("numeric_value"), Value);
			}
			else
			{
				const int64 Value = NumProp->GetSignedIntPropertyValue(NumProp->ContainerPtrToValuePtr<void>(Actor));
				Data->SetNumberField(TEXT("value"), static_cast<double>(Value));
				Data->SetNumberField(TEXT("numeric_value"), static_cast<double>(Value));
			}
		}
		else
		{
			return Fail(
				TEXT("unsupported_property"),
				FString::Printf(TEXT("Property '%s' type '%s' is not supported by get_actor_property."),
					*Property->GetName(), *Property->GetClass()->GetName()));
		}

		return Ok(Data);
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

	TSharedRef<FJsonObject> HitToJson(const FHitResult& Hit)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetBoolField(TEXT("blocking_hit"), Hit.bBlockingHit);
		Out->SetBoolField(TEXT("start_penetrating"), Hit.bStartPenetrating);
		Out->SetNumberField(TEXT("distance"), Hit.Distance);
		Out->SetNumberField(TEXT("time"), Hit.Time);
		Out->SetNumberField(TEXT("penetration_depth"), Hit.PenetrationDepth);
		Out->SetArrayField(TEXT("impact_point"), Vec(Hit.ImpactPoint));
		Out->SetArrayField(TEXT("impact_normal"), Vec(Hit.ImpactNormal));
		Out->SetArrayField(TEXT("location"), Vec(Hit.Location));
		AActor* Actor = Hit.GetActor();
		UPrimitiveComponent* Primitive = Hit.GetComponent();
		PutActor(Out, Actor);
		if (Primitive)
		{
			PutComponentCollision(Out, Primitive);
			if (Actor)
			{
				Out->SetBoolField(
					TEXT("belongs_to_admin_access_door"),
					ClassName(Actor).Contains(TEXT("AdminAccessDoor"))
					|| ActorLabel(Actor).Contains(TEXT("AdminAccessDoor")));
			}
		}
		return Out;
	}

	TSharedRef<FJsonObject> CmdPing()
	{
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("service"), TEXT("OrganoidAIBridge"));
		Data->SetStringField(TEXT("version"), OrganoidAIBridgeJson::BridgeVersion());
		Data->SetStringField(TEXT("mode"), TEXT("read_only"));
		Data->SetStringField(TEXT("writes"), TEXT("approval_gated"));
		Data->SetBoolField(TEXT("dual_approval_required"), true);
		Data->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Data->SetBoolField(TEXT("on_game_thread"), IsInGameThread());
		Data->SetBoolField(TEXT("game_thread_marshal"), true);
		TArray<TSharedPtr<FJsonValue>> NativeActions;
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("move_actor_to_level")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("save_maps")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("set_actor_property")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_blueprint_actor")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("add_scs_component")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("author_hologram_apply_state")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("author_light_controller_set_zone")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("author_admin_room_trigger_lighting_hook")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("author_access_door_facility_state_listener")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("author_light_controller_facility_state_listener")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_s20_zone_light")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("set_s20_light_intensity")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("invoke_s20_set_lighting_zone")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("delete_s22_legacy_admin_ambience")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_s22_admin_audio_zones")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_navmesh_bounds")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_research_station")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_research_wing_connector")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_research_wing_keycard")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("trim_spine_landing_admin")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("set_admin_doorlock_interactable")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block2_dressing")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block3_resources")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block4_security_officer")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block4_navmesh_bounds")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("save_admin_block4_navmesh_prerequisite")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_arrival_lab_dressing")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_ch3_containment_evidence")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_ch4_transformed_personnel")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("configure_neuro_power_failure_discovery")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_power_diagnostic")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("create_neurogenetics_mission")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("expand_neurogenetics_mission_beat3")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("expand_neurogenetics_mission_beat4")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_neural_mapping_array")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_research_load_cutoff")));
		NativeActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_neural_mapping_terminal")));
		Data->SetArrayField(TEXT("native_game_thread_actions"), NativeActions);
		Data->SetStringField(TEXT("map_save"), TEXT("use save_maps; save_asset rejects map packages"));
		Data->SetBoolField(TEXT("save_maps_admin_only_requires_epitope_persistent"), false);
		Data->SetStringField(
			TEXT("save_maps_admin_only"),
			TEXT("SavePackages on loaded SL_Epitope_Admin; persistent may be Lvl_MainMenu. Lvl_Epitope persistent required only when that package is in the transaction."));
		Data->SetStringField(
			TEXT("save_maps_neuro_only"),
			TEXT("SavePackages on loaded SL_Epitope_NeuroGenetics only. Persistent may be Lvl_MainMenu. Does not save Admin or Lvl_Epitope."));
		Data->SetStringField(
			TEXT("save_maps_epitope_only"),
			TEXT("SavePackages on loaded Lvl_Epitope only. Persistent must be Lvl_Epitope. Does not save Admin, Neuro, or Lvl_MainMenu."));
		Data->SetStringField(
			TEXT("approved_editor_automation"),
			TEXT("prepare_write + dual approve_write + execute_write. Agent executes native APIs after approval. Tom does not click editor UI for allowlisted operations."));
		Data->SetBoolField(TEXT("get_actor_property"), true);
		Data->SetBoolField(TEXT("get_light_component"), true);
		Data->SetBoolField(TEXT("inspect_playing_audio"), true);
		Data->SetBoolField(TEXT("inspect_admin_block4_navmesh"), true);
		TArray<TSharedPtr<FJsonValue>> PlaytestCommands;
		PlaytestCommands.Add(MakeShared<FJsonValueString>(TEXT("list_playtests")));
		PlaytestCommands.Add(MakeShared<FJsonValueString>(TEXT("run_playtest")));
		PlaytestCommands.Add(MakeShared<FJsonValueString>(TEXT("get_playtest_status")));
		PlaytestCommands.Add(MakeShared<FJsonValueString>(TEXT("get_playtest_result")));
		PlaytestCommands.Add(MakeShared<FJsonValueString>(TEXT("stop_playtest")));
		Data->SetArrayField(TEXT("playtest_commands"), PlaytestCommands);
		Data->SetBoolField(TEXT("playtest_mutates_assets"), false);
		TArray<TSharedPtr<FJsonValue>> GraphActions;
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("get_blueprint_graph")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("connect_blueprint_pins")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("add_blueprint_variable")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("add_scs_component")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("author_hologram_apply_state")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("author_light_controller_set_zone")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("author_admin_room_trigger_lighting_hook")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("author_access_door_facility_state_listener")));
		GraphActions.Add(MakeShared<FJsonValueString>(TEXT("author_light_controller_facility_state_listener")));
		Data->SetArrayField(TEXT("blueprint_graph_actions"), GraphActions);
		TArray<TSharedPtr<FJsonValue>> SpawnActions;
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_blueprint_actor")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_s20_zone_light")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_s22_admin_audio_zones")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("delete_s22_legacy_admin_ambience")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_navmesh_bounds")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_research_station")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_research_wing_connector")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_research_wing_keycard")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block2_dressing")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block3_resources")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block4_security_officer")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_admin_block4_navmesh_bounds")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("set_admin_doorlock_interactable")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_arrival_lab_dressing")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_ch3_containment_evidence")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_ch4_transformed_personnel")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("configure_neuro_power_failure_discovery")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_power_diagnostic")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("create_neurogenetics_mission")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("expand_neurogenetics_mission_beat3")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("expand_neurogenetics_mission_beat4")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_neural_mapping_array")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_research_load_cutoff")));
		SpawnActions.Add(MakeShared<FJsonValueString>(TEXT("spawn_neuro_neural_mapping_terminal")));
		Data->SetArrayField(TEXT("spawn_actions"), SpawnActions);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdEditorState()
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("has_editor_world"), World != nullptr);
		Data->SetStringField(TEXT("map"), World ? World->GetMapName() : TEXT(""));
		Data->SetStringField(TEXT("package"), WorldPackageName(World));
		TArray<TSharedPtr<FJsonValue>> Streaming;
		if (World)
		{
			for (ULevelStreaming* Level : World->GetStreamingLevels())
			{
				if (!Level)
				{
					continue;
				}
				TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("name"), Level->GetWorldAssetPackageFName().ToString());
				Entry->SetBoolField(TEXT("loaded"), Level->IsLevelLoaded());
				Entry->SetBoolField(TEXT("visible"), Level->IsLevelVisible());
				Streaming.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		Data->SetArrayField(TEXT("streaming_levels"), Streaming);

		TArray<TSharedPtr<FJsonValue>> Selected;
		if (GEditor)
		{
			if (USelection* Selection = GEditor->GetSelectedActors())
			{
				for (FSelectionIterator It(*Selection); It; ++It)
				{
					if (AActor* Actor = Cast<AActor>(*It))
					{
						TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
						PutActor(Entry, Actor);
						Selected.Add(MakeShared<FJsonValueObject>(Entry));
					}
				}
			}
		}
		Data->SetArrayField(TEXT("selected_actors"), Selected);

		TArray<TSharedPtr<FJsonValue>> Dirty;
		TArray<UPackage*> DirtyWorld;
		TArray<UPackage*> DirtyContent;
		FEditorFileUtils::GetDirtyWorldPackages(DirtyWorld);
		FEditorFileUtils::GetDirtyContentPackages(DirtyContent);
		auto AppendDirty = [&Dirty](const TArray<UPackage*>& Packages, const TCHAR* Kind)
		{
			for (UPackage* Package : Packages)
			{
				if (!Package)
				{
					continue;
				}
				TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("package"), Package->GetName());
				Entry->SetStringField(TEXT("kind"), Kind);
				Dirty.Add(MakeShared<FJsonValueObject>(Entry));
			}
		};
		AppendDirty(DirtyWorld, TEXT("world"));
		AppendDirty(DirtyContent, TEXT("content"));
		Data->SetArrayField(TEXT("dirty_packages"), Dirty);
		Data->SetNumberField(TEXT("dirty_count"), Dirty.Num());
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdPieState()
	{
		UWorld* Pie = GetPieWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("pie_running"), Pie != nullptr);
		Data->SetStringField(TEXT("map"), Pie ? Pie->GetMapName() : TEXT(""));
		Data->SetStringField(TEXT("package"), WorldPackageName(Pie));
		if (Pie)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(Pie, 0);
			Data->SetStringField(TEXT("player_controller"), PC ? PC->GetName() : TEXT(""));
			Data->SetStringField(TEXT("player_controller_class"), ClassName(PC));
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			Data->SetStringField(TEXT("pawn"), Pawn ? Pawn->GetName() : TEXT(""));
			Data->SetStringField(TEXT("pawn_class"), ClassName(Pawn));
		}
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdPlayerPawn(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPieWorld();
		if (!World)
		{
			return Fail(TEXT("no_pie"), TEXT("PIE is not running."));
		}
		const int32 PlayerIndex = GetInt(Args, TEXT("player_index"), 0);
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, PlayerIndex);
		APawn* Pawn = nullptr;
		if (PC)
		{
			Pawn = PC->GetPawn();
		}
		else
		{
			Pawn = UGameplayStatics::GetPlayerPawn(World, PlayerIndex);
		}
		if (!Pawn)
		{
			return Fail(TEXT("no_pawn"), TEXT("No possessed pawn for player 0."));
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("pie_running"), true);
		PutActor(Data, Pawn);
		Data->SetStringField(TEXT("pawn"), Pawn->GetName());
		const FVector Location = Pawn->GetActorLocation();
		Data->SetArrayField(TEXT("location"), Vec(Location));
		Data->SetArrayField(TEXT("velocity"), Vec(Pawn->GetVelocity()));
		Data->SetArrayField(TEXT("forward"), Vec(Pawn->GetActorForwardVector()));

		float Radius = 0.f;
		float HalfHeight = 0.f;
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				Radius = Capsule->GetUnscaledCapsuleRadius();
				HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
				Data->SetNumberField(TEXT("capsule_radius"), Radius);
				Data->SetNumberField(TEXT("capsule_half_height"), HalfHeight);
				Data->SetNumberField(TEXT("capsule_plus_x"), Location.X + Radius);
			}
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				Data->SetStringField(TEXT("movement_mode"), Move->GetMovementName());
				Data->SetBoolField(TEXT("is_moving_on_ground"), Move->IsMovingOnGround());
				Data->SetBoolField(TEXT("is_falling"), Move->IsFalling());
				Data->SetNumberField(TEXT("gravity_scale"), Move->GravityScale);
				Data->SetNumberField(TEXT("max_step_height"), Move->MaxStepHeight);
				Data->SetNumberField(TEXT("walkable_floor_z"), Move->GetWalkableFloorZ());
				Data->SetBoolField(TEXT("floor_blocking_hit"), Move->CurrentFloor.bBlockingHit);
				Data->SetBoolField(TEXT("floor_walkable"), Move->CurrentFloor.bWalkableFloor);
				Data->SetNumberField(TEXT("floor_dist"), Move->CurrentFloor.FloorDist);
				if (Move->CurrentFloor.HitResult.GetActor())
				{
					Data->SetStringField(TEXT("floor_actor"), ActorLabel(Move->CurrentFloor.HitResult.GetActor()));
					if (Move->CurrentFloor.HitResult.GetComponent())
					{
						Data->SetStringField(TEXT("floor_component"), Move->CurrentFloor.HitResult.GetComponent()->GetName());
					}
				}
			}
		}

		const bool bSweep = GetBool(Args, TEXT("include_sweep"), true);
		if (bSweep && Radius > 0.f)
		{
			FVector Direction;
			if (!GetVector(Args, TEXT("direction"), Direction) || Direction.IsNearlyZero())
			{
				Direction = Pawn->GetActorForwardVector();
			}
			Direction.Normalize();
			const float Distance = static_cast<float>(GetNumber(Args, TEXT("distance"), 150.0));
			const FVector Start = Location;
			const FVector End = Location + Direction * Distance;
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(OrganoidAIBridgePawnSweep), false, Pawn);
			const bool bHit = World->SweepSingleByProfile(
				Hit,
				Start,
				End,
				FQuat::Identity,
				TEXT("Pawn"),
				FCollisionShape::MakeCapsule(Radius, HalfHeight),
				Params);
			Data->SetBoolField(TEXT("sweep_hit"), bHit);
			if (bHit)
			{
				Data->SetObjectField(TEXT("blocking_hit"), HitToJson(Hit));
			}
			else
			{
				Data->SetField(TEXT("blocking_hit"), MakeShared<FJsonValueNull>());
			}
		}
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdGetActor(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Query = GetString(Args, TEXT("name"), GetString(Args, TEXT("label")));
		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), true));
		AActor* Actor = FindActor(World, Query);
		if (!Actor)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("No actor matching '%s'."), *Query));
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		PutActor(Data, Actor);
		FVector Origin;
		FVector Extent;
		Actor->GetActorBounds(true, Origin, Extent);
		Data->SetArrayField(TEXT("bounds_origin"), Vec(Origin));
		Data->SetArrayField(TEXT("bounds_extent"), Vec(Extent));

		TArray<TSharedPtr<FJsonValue>> Components;
		TArray<UActorComponent*> All;
		Actor->GetComponents(All);
		for (UActorComponent* Component : All)
		{
			if (!Component)
			{
				continue;
			}
			TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Component->GetName());
			Entry->SetStringField(TEXT("class"), ClassName(Component));
			if (USceneComponent* Scene = Cast<USceneComponent>(Component))
			{
				Entry->SetArrayField(TEXT("relative_location"), Vec(Scene->GetRelativeLocation()));
				Entry->SetStringField(TEXT("attach_parent"), Scene->GetAttachParent() ? Scene->GetAttachParent()->GetName() : TEXT(""));
			}
			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
			{
				PutComponentCollision(Entry, Primitive);
			}
			Components.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Data->SetArrayField(TEXT("components"), Components);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdGetLightComponent(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Query = GetString(Args, TEXT("actor"), GetString(Args, TEXT("name"), GetString(Args, TEXT("label"))));
		if (Query.IsEmpty())
		{
			return Fail(TEXT("bad_args"), TEXT("actor label or unique actor path is required."));
		}
		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), false));
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No editor world."));
		}
		TArray<AActor*> Matches = FindUniqueActorCandidates(World, Query);
		if (Matches.Num() == 0)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("No actor matching '%s'."), *Query));
		}
		if (Matches.Num() != 1)
		{
			return Fail(
				TEXT("not_unique"),
				FString::Printf(TEXT("Actor query '%s' matched %d actors. Need exactly 1."), *Query, Matches.Num()));
		}
		AActor* Actor = Matches[0];
		const FString ComponentName = GetString(Args, TEXT("component"));
		ULightComponent* Light = nullptr;
		TArray<ULightComponent*> Lights;
		Actor->GetComponents<ULightComponent>(Lights);
		if (ComponentName.IsEmpty() && Lights.Num() == 1)
		{
			Light = Lights[0];
		}
		else
		{
			for (ULightComponent* Candidate : Lights)
			{
				if (Candidate && (ComponentName.IsEmpty() || Candidate->GetName().Equals(ComponentName, ESearchCase::IgnoreCase)))
				{
					if (Light)
					{
						return Fail(TEXT("not_unique"), TEXT("Multiple light components. Pass component name."));
					}
					Light = Candidate;
				}
			}
		}
		if (!Light)
		{
			return Fail(TEXT("not_found"), TEXT("Light component not found."));
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		PutActor(Data, Actor);
		Data->SetStringField(TEXT("owning_package"), ActorOwningPackage(Actor));
		Data->SetStringField(TEXT("component"), Light->GetName());
		Data->SetStringField(TEXT("component_class"), ClassName(Light));
		Data->SetNumberField(TEXT("intensity"), Light->Intensity);
		const FLinearColor Color = Light->GetLightColor();
		Data->SetArrayField(TEXT("color"), Vec(FVector(Color.R, Color.G, Color.B)));
		Data->SetBoolField(TEXT("visible"), Light->GetVisibleFlag());
		Data->SetBoolField(TEXT("hidden_in_game"), Light->bHiddenInGame);
		Data->SetBoolField(TEXT("cast_shadows"), Light->CastShadows);
		Data->SetStringField(TEXT("mobility"), UEnum::GetValueAsString(Light->Mobility));
		if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light))
		{
			Data->SetNumberField(TEXT("attenuation_radius"), Point->AttenuationRadius);
		}
		TArray<TSharedPtr<FJsonValue>> ActorTags;
		for (const FName& Tag : Actor->Tags)
		{
			ActorTags.Add(MakeShared<FJsonValueString>(Tag.ToString()));
		}
		Data->SetArrayField(TEXT("actor_tags"), ActorTags);
		TArray<TSharedPtr<FJsonValue>> CompTags;
		for (const FName& Tag : Light->ComponentTags)
		{
			CompTags.Add(MakeShared<FJsonValueString>(Tag.ToString()));
		}
		Data->SetArrayField(TEXT("component_tags"), CompTags);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdGetComponent(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), true));
		AActor* Actor = FindActor(World, GetString(Args, TEXT("actor")));
		if (!Actor)
		{
			return Fail(TEXT("not_found"), TEXT("Actor not found."));
		}
		UPrimitiveComponent* Primitive = FindPrimitive(Actor, GetString(Args, TEXT("component")));
		if (!Primitive)
		{
			return Fail(TEXT("not_found"), TEXT("Component not found."));
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		PutActor(Data, Actor);
		PutComponentCollision(Data, Primitive);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdListActorsNear(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), true));
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No editor or PIE world."));
		}
		FVector Origin;
		if (!GetVector(Args, TEXT("origin"), Origin))
		{
			if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
			{
				Origin = Pawn->GetActorLocation();
			}
		}
		const float Radius = static_cast<float>(GetNumber(Args, TEXT("radius"), 400.0));
		const int32 MaxCount = GetInt(Args, TEXT("max"), 40);
		TArray<TSharedPtr<FJsonValue>> Actors;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}
			const float Dist = FVector::Dist(Actor->GetActorLocation(), Origin);
			if (Dist > Radius)
			{
				continue;
			}
			TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			PutActor(Entry, Actor);
			Entry->SetNumberField(TEXT("distance"), Dist);
			Actors.Add(MakeShared<FJsonValueObject>(Entry));
			if (Actors.Num() >= MaxCount)
			{
				break;
			}
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetArrayField(TEXT("origin"), Vec(Origin));
		Data->SetNumberField(TEXT("radius"), Radius);
		Data->SetArrayField(TEXT("actors"), Actors);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdGetCollision(const TSharedPtr<FJsonObject>& Args)
	{
		return CmdGetComponent(Args);
	}

	TSharedRef<FJsonObject> CmdCapsuleSweep(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPieWorld();
		if (!World)
		{
			World = GetEditorWorld();
		}
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No world for sweep."));
		}
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		FVector Start;
		if (!GetVector(Args, TEXT("start"), Start))
		{
			Start = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		}
		FVector Direction;
		if (!GetVector(Args, TEXT("direction"), Direction) || Direction.IsNearlyZero())
		{
			Direction = Pawn ? Pawn->GetActorForwardVector() : FVector::ForwardVector;
		}
		Direction.Normalize();
		const float Distance = static_cast<float>(GetNumber(Args, TEXT("distance"), 150.0));
		float Radius = static_cast<float>(GetNumber(Args, TEXT("radius"), 42.0));
		float HalfHeight = static_cast<float>(GetNumber(Args, TEXT("half_height"), 96.0));
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				if (!Args.IsValid() || !Args->HasField(TEXT("radius")))
				{
					Radius = Capsule->GetUnscaledCapsuleRadius();
				}
				if (!Args.IsValid() || !Args->HasField(TEXT("half_height")))
				{
					HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
				}
			}
		}
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(OrganoidAIBridgeSweep), false, Pawn);
		const bool bHit = World->SweepSingleByProfile(
			Hit,
			Start,
			Start + Direction * Distance,
			FQuat::Identity,
			TEXT("Pawn"),
			FCollisionShape::MakeCapsule(Radius, HalfHeight),
			Params);
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("hit"), bHit);
		Data->SetArrayField(TEXT("start"), Vec(Start));
		Data->SetArrayField(TEXT("direction"), Vec(Direction));
		Data->SetNumberField(TEXT("distance"), Distance);
		Data->SetNumberField(TEXT("radius"), Radius);
		Data->SetNumberField(TEXT("half_height"), HalfHeight);
		if (bHit)
		{
			Data->SetObjectField(TEXT("blocking_hit"), HitToJson(Hit));
		}
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdOverlapQuery(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPieWorld();
		if (!World)
		{
			World = GetEditorWorld();
		}
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No world for overlap."));
		}
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		FVector Origin;
		if (!GetVector(Args, TEXT("origin"), Origin))
		{
			Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
		}
		float Radius = static_cast<float>(GetNumber(Args, TEXT("radius"), 42.0));
		float HalfHeight = static_cast<float>(GetNumber(Args, TEXT("half_height"), 96.0));
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(OrganoidAIBridgeOverlap), false, Pawn);
		World->OverlapMultiByProfile(
			Overlaps,
			Origin,
			FQuat::Identity,
			TEXT("Pawn"),
			FCollisionShape::MakeCapsule(Radius, HalfHeight),
			Params);
		TArray<TSharedPtr<FJsonValue>> Hits;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetBoolField(TEXT("blocking"), Overlap.bBlockingHit);
			PutActor(Entry, Overlap.GetActor());
			if (UPrimitiveComponent* Primitive = Overlap.GetComponent())
			{
				PutComponentCollision(Entry, Primitive);
			}
			Hits.Add(MakeShared<FJsonValueObject>(Entry));
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetArrayField(TEXT("origin"), Vec(Origin));
		Data->SetNumberField(TEXT("count"), Hits.Num());
		Data->SetArrayField(TEXT("overlaps"), Hits);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdOutputLog(const TSharedPtr<FJsonObject>& Args, FOrganoidAIBridgeLogSink* LogSink)
	{
		if (!LogSink)
		{
			return Fail(TEXT("no_log_sink"), TEXT("Log sink is not attached."));
		}
		const FString Filter = GetString(Args, TEXT("filter"));
		const int32 MaxReturn = GetInt(Args, TEXT("max"), 200);
		TArray<FString> Lines = LogSink->CopyFiltered(Filter, MaxReturn);
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FString& Line : Lines)
		{
			Arr.Add(MakeShared<FJsonValueString>(Line));
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("filter"), Filter);
		Data->SetNumberField(TEXT("count"), Arr.Num());
		Data->SetArrayField(TEXT("lines"), Arr);
		return Ok(Data);
	}

	UBlueprint* FindBlueprintAsset(const FString& PathOrName)
	{
		FString Path = PathOrName;
		if (!Path.StartsWith(TEXT("/")))
		{
			Path = TEXT("/Game/") + Path;
		}
		if (UBlueprint* Direct = LoadObject<UBlueprint>(nullptr, *Path))
		{
			return Direct;
		}
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Equals(PathOrName, ESearchCase::IgnoreCase)
				|| Asset.GetObjectPathString().Contains(PathOrName))
			{
				return Cast<UBlueprint>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	TSharedRef<FJsonObject> CmdFindBlueprint(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Query = GetString(Args, TEXT("path"), GetString(Args, TEXT("name")));
		UBlueprint* Blueprint = FindBlueprintAsset(Query);
		if (!Blueprint)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("Blueprint '%s' not found."), *Query));
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
		Data->SetStringField(TEXT("name"), Blueprint->GetName());
		Data->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Data->SetStringField(TEXT("status"), UEnum::GetValueAsString(Blueprint->Status.GetValue()));
		Data->SetBoolField(TEXT("generated_class"), Blueprint->GeneratedClass != nullptr);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdBlueprintComponents(const TSharedPtr<FJsonObject>& Args)
	{
		UBlueprint* Blueprint = FindBlueprintAsset(GetString(Args, TEXT("path"), GetString(Args, TEXT("name"))));
		if (!Blueprint)
		{
			return Fail(TEXT("not_found"), TEXT("Blueprint not found."));
		}
		TArray<TSharedPtr<FJsonValue>> Components;
		if (USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript)
		{
			TArray<USCS_Node*> Nodes = SCS->GetAllNodes();
			for (USCS_Node* Node : Nodes)
			{
				if (!Node)
				{
					continue;
				}
				TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
				Entry->SetStringField(TEXT("class"), Node->ComponentClass ? Node->ComponentClass->GetName() : TEXT(""));
				if (UActorComponent* Template = Node->GetActualComponentTemplate(Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass)))
				{
					Entry->SetStringField(TEXT("template"), Template->GetName());
					if (USceneComponent* Scene = Cast<USceneComponent>(Template))
					{
						Entry->SetArrayField(TEXT("relative_location"), Vec(Scene->GetRelativeLocation()));
						Entry->SetArrayField(TEXT("relative_scale"), Vec(Scene->GetRelativeScale3D()));
						Entry->SetBoolField(TEXT("visible"), Scene->GetVisibleFlag());
						Entry->SetBoolField(TEXT("hidden_in_game"), Scene->bHiddenInGame);
					}
					if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Template))
					{
						PutComponentCollision(Entry, Primitive);
					}
					if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Template))
					{
						Entry->SetStringField(TEXT("static_mesh"), Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName() : TEXT(""));
					}
					if (UPointLightComponent* Light = Cast<UPointLightComponent>(Template))
					{
						Entry->SetNumberField(TEXT("intensity"), Light->Intensity);
						Entry->SetArrayField(TEXT("color"), Vec(FVector(Light->GetLightColor().R, Light->GetLightColor().G, Light->GetLightColor().B)));
						Entry->SetNumberField(TEXT("attenuation_radius"), Light->AttenuationRadius);
						Entry->SetBoolField(TEXT("cast_shadows"), Light->CastShadows);
					}
				}
				Components.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
		Data->SetArrayField(TEXT("components"), Components);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdBlueprintMembers(const TSharedPtr<FJsonObject>& Args)
	{
		UBlueprint* Blueprint = FindBlueprintAsset(GetString(Args, TEXT("path"), GetString(Args, TEXT("name"))));
		if (!Blueprint)
		{
			return Fail(TEXT("not_found"), TEXT("Blueprint not found."));
		}
		TArray<TSharedPtr<FJsonValue>> Variables;
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
			Variables.Add(MakeShared<FJsonValueObject>(Entry));
		}
		TArray<TSharedPtr<FJsonValue>> Timelines;
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (!Graph)
			{
				continue;
			}
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_Timeline* Timeline = Cast<UK2Node_Timeline>(Node))
				{
					TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
					Entry->SetStringField(TEXT("name"), Timeline->TimelineName.ToString());
					Timelines.Add(MakeShared<FJsonValueObject>(Entry));
				}
			}
		}
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
		Data->SetArrayField(TEXT("variables"), Variables);
		TArray<TSharedPtr<FJsonValue>> Functions;
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (!Graph)
			{
				continue;
			}
			TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Graph->GetName());
			Entry->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
			Functions.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Data->SetArrayField(TEXT("functions"), Functions);
		TArray<TSharedPtr<FJsonValue>> UbergraphEvents;
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (!Graph)
			{
				continue;
			}
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
				if (!Event)
				{
					continue;
				}
				TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("name"), Event->GetName());
				Entry->SetStringField(TEXT("function"), Event->GetFunctionName().ToString());
				Entry->SetStringField(TEXT("title"), Event->GetNodeTitle(ENodeTitleType::ListView).ToString());
				Entry->SetStringField(TEXT("graph"), Graph->GetName());
				UbergraphEvents.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}
		Data->SetArrayField(TEXT("ubergraph_events"), UbergraphEvents);
		Data->SetArrayField(TEXT("timelines"), Timelines);
		return Ok(Data);
	}

	UUserDefinedEnum* FindUserDefinedEnumAsset(const FString& PathOrName)
	{
		FString Path = PathOrName;
		if (!Path.StartsWith(TEXT("/")))
		{
			Path = TEXT("/Game/") + Path;
		}
		if (UUserDefinedEnum* Direct = LoadObject<UUserDefinedEnum>(nullptr, *Path))
		{
			return Direct;
		}
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		AssetRegistry.Get().GetAssetsByClass(UUserDefinedEnum::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Equals(PathOrName, ESearchCase::IgnoreCase)
				|| Asset.GetObjectPathString().Contains(PathOrName))
			{
				return Cast<UUserDefinedEnum>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	TSharedRef<FJsonObject> EnumEntryJson(const UEnum* Enum, int32 Index)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetNumberField(TEXT("index"), Index);
		Entry->SetStringField(TEXT("internal_name"), Enum->GetNameStringByIndex(Index));
		Entry->SetStringField(TEXT("display_name"), Enum->GetDisplayNameTextByIndex(Index).ToString());
		Entry->SetNumberField(TEXT("value"), static_cast<double>(Enum->GetValueByIndex(Index)));
		const bool bHidden = Enum->HasMetaData(TEXT("Hidden"), Index) || Enum->HasMetaData(TEXT("Spacer"), Index);
		Entry->SetBoolField(TEXT("hidden"), bHidden);
		Entry->SetBoolField(TEXT("is_max"), Index == Enum->NumEnums() - 1);
		return Entry;
	}

	TSharedRef<FJsonObject> SwitchPinJson(const UEdGraphPin* Pin)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("pin_name"), Pin ? Pin->PinName.ToString() : TEXT(""));
		Entry->SetStringField(TEXT("pin_friendly_name"), Pin ? Pin->PinFriendlyName.ToString() : TEXT(""));
		Entry->SetStringField(TEXT("direction"), Pin && Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		Entry->SetStringField(TEXT("category"), Pin ? Pin->PinType.PinCategory.ToString() : TEXT(""));
		return Entry;
	}

	TSharedRef<FJsonObject> CmdGetUserDefinedEnum(const TSharedPtr<FJsonObject>& Args)
	{
		const FString EnumQuery = GetString(Args, TEXT("path"), GetString(Args, TEXT("enum_path")));
		const FString BlueprintQuery = GetString(Args, TEXT("blueprint_path"), GetString(Args, TEXT("blueprint")));
		if (EnumQuery.IsEmpty())
		{
			return Fail(TEXT("bad_args"), TEXT("path is required (UserDefinedEnum asset path or name)."));
		}

		UUserDefinedEnum* EnumAsset = FindUserDefinedEnumAsset(EnumQuery);
		if (!EnumAsset)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("UserDefinedEnum '%s' not found."), *EnumQuery));
		}

		TArray<TSharedPtr<FJsonValue>> Entries;
		TArray<TSharedPtr<FJsonValue>> CaseEntries;
		const int32 Num = EnumAsset->NumEnums();
		for (int32 Index = 0; Index < Num; ++Index)
		{
			TSharedRef<FJsonObject> Entry = EnumEntryJson(EnumAsset, Index);
			Entries.Add(MakeShared<FJsonValueObject>(Entry));
			const bool bHidden = Entry->GetBoolField(TEXT("hidden"));
			const bool bIsMax = Entry->GetBoolField(TEXT("is_max"));
			if (!bHidden && !bIsMax)
			{
				CaseEntries.Add(MakeShared<FJsonValueObject>(Entry));
			}
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("path"), EnumAsset->GetPathName());
		Data->SetStringField(TEXT("name"), EnumAsset->GetName());
		Data->SetNumberField(TEXT("num_enums"), Num);
		Data->SetArrayField(TEXT("entries"), Entries);
		Data->SetArrayField(TEXT("switch_cases"), CaseEntries);
		Data->SetStringField(
			TEXT("note"),
			TEXT("internal_name is UEnum::GetNameStringByIndex. display_name is GetDisplayNameTextByIndex (DisplayNameMap). value is GetValueByIndex. switch_cases omits Hidden/Spacer/_MAX the same way UK2Node_SwitchEnum::SetEnum does. Do not infer mapping from array order alone; use internal_name and display_name together."));

		if (!BlueprintQuery.IsEmpty())
		{
			UBlueprint* Blueprint = FindBlueprintAsset(BlueprintQuery);
			if (!Blueprint)
			{
				return Fail(TEXT("not_found"), FString::Printf(TEXT("Blueprint '%s' not found."), *BlueprintQuery));
			}

			TArray<UEdGraph*> Graphs;
			Graphs.Append(Blueprint->UbergraphPages);
			Graphs.Append(Blueprint->FunctionGraphs);

			TArray<TSharedPtr<FJsonValue>> Switches;
			for (UEdGraph* Graph : Graphs)
			{
				if (!Graph)
				{
					continue;
				}
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					UK2Node_SwitchEnum* SwitchNode = Cast<UK2Node_SwitchEnum>(Node);
					if (!SwitchNode)
					{
						continue;
					}
					if (SwitchNode->Enum != EnumAsset)
					{
						continue;
					}

					TSharedRef<FJsonObject> SwitchJson = MakeShared<FJsonObject>();
					SwitchJson->SetStringField(TEXT("node_name"), SwitchNode->GetName());
					SwitchJson->SetStringField(TEXT("node_title"), SwitchNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
					SwitchJson->SetStringField(TEXT("graph"), Graph->GetName());
					SwitchJson->SetStringField(
						TEXT("enum_path"),
						SwitchNode->Enum ? SwitchNode->Enum->GetPathName() : TEXT(""));

					TArray<TSharedPtr<FJsonValue>> EnumEntryNames;
					for (const FName& EntryName : SwitchNode->EnumEntries)
					{
						EnumEntryNames.Add(MakeShared<FJsonValueString>(EntryName.ToString()));
					}
					SwitchJson->SetArrayField(TEXT("enum_entries"), EnumEntryNames);

					TArray<TSharedPtr<FJsonValue>> FriendlyNames;
					for (const FText& Friendly : SwitchNode->EnumFriendlyNames)
					{
						FriendlyNames.Add(MakeShared<FJsonValueString>(Friendly.ToString()));
					}
					SwitchJson->SetArrayField(TEXT("enum_friendly_names_transient"), FriendlyNames);

					TArray<TSharedPtr<FJsonValue>> Pins;
					for (const UEdGraphPin* Pin : SwitchNode->Pins)
					{
						if (!Pin)
						{
							continue;
						}
						Pins.Add(MakeShared<FJsonValueObject>(SwitchPinJson(Pin)));
					}
					SwitchJson->SetArrayField(TEXT("pins"), Pins);
					Switches.Add(MakeShared<FJsonValueObject>(SwitchJson));
				}
			}

			Data->SetStringField(TEXT("blueprint_path"), Blueprint->GetPathName());
			Data->SetArrayField(TEXT("switch_nodes"), Switches);
		}

		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdGetBlueprintGraph(const TSharedPtr<FJsonObject>& Args)
	{
		const FString Query = GetString(Args, TEXT("path"), GetString(Args, TEXT("name")));
		const FString GraphName = GetString(Args, TEXT("graph"), GetString(Args, TEXT("function")));
		UBlueprint* Blueprint = FindBlueprintAsset(Query);
		if (!Blueprint)
		{
			return Fail(TEXT("not_found"), FString::Printf(TEXT("Blueprint '%s' not found."), *Query));
		}
		if (GraphName.IsEmpty())
		{
			return Fail(TEXT("bad_args"), TEXT("graph (function graph name) is required."));
		}

		UEdGraph* Target = nullptr;
		TArray<TSharedPtr<FJsonValue>> FunctionNames;
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (!Graph)
			{
				continue;
			}
			FunctionNames.Add(MakeShared<FJsonValueString>(Graph->GetName()));
			if (Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				Target = Graph;
			}
		}
		FString GraphKind = TEXT("function");
		if (!Target && (GraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase)
			|| GraphName.Equals(TEXT("Ubergraph"), ESearchCase::IgnoreCase)))
		{
			for (UEdGraph* Graph : Blueprint->UbergraphPages)
			{
				if (!Graph)
				{
					continue;
				}
				if (Graph->GetFName() == UEdGraphSchema_K2::GN_EventGraph
					|| Graph->GetName().Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase)
					|| Blueprint->UbergraphPages.Num() == 1)
				{
					Target = Graph;
					GraphKind = TEXT("eventgraph");
					break;
				}
			}
		}
		if (!Target)
		{
			TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
			Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
			Data->SetArrayField(TEXT("function_graphs"), FunctionNames);
			return Fail(TEXT("graph_not_found"), FString::Printf(
				TEXT("Graph '%s' not found."), *GraphName));
		}

		TArray<TSharedPtr<FJsonValue>> NodesJson;
		for (UEdGraphNode* Node : Target->Nodes)
		{
			if (!Node)
			{
				continue;
			}
			TSharedRef<FJsonObject> NodeJson = MakeShared<FJsonObject>();
			NodeJson->SetStringField(TEXT("name"), Node->GetName());
			NodeJson->SetStringField(TEXT("class"), Node->GetClass() ? Node->GetClass()->GetName() : TEXT(""));
			NodeJson->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
			if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
			{
				NodeJson->SetStringField(TEXT("function"), Call->GetFunctionName().ToString());
			}
			if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
			{
				NodeJson->SetStringField(TEXT("function"), Event->GetFunctionName().ToString());
			}
			TArray<TSharedPtr<FJsonValue>> PinsJson;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin)
				{
					continue;
				}
				TSharedRef<FJsonObject> PinJson = MakeShared<FJsonObject>();
				PinJson->SetStringField(TEXT("name"), Pin->PinName.ToString());
				PinJson->SetStringField(TEXT("friendly"), Pin->PinFriendlyName.ToString());
				PinJson->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"));
				PinJson->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
				TArray<TSharedPtr<FJsonValue>> Links;
				for (UEdGraphPin* Linked : Pin->LinkedTo)
				{
					if (!Linked || !Linked->GetOwningNode())
					{
						continue;
					}
					TSharedRef<FJsonObject> Link = MakeShared<FJsonObject>();
					Link->SetStringField(TEXT("node"), Linked->GetOwningNode()->GetName());
					Link->SetStringField(TEXT("pin"), Linked->PinName.ToString());
					Links.Add(MakeShared<FJsonValueObject>(Link));
				}
				PinJson->SetArrayField(TEXT("linked_to"), Links);
				PinsJson.Add(MakeShared<FJsonValueObject>(PinJson));
			}
			NodeJson->SetArrayField(TEXT("pins"), PinsJson);
			NodesJson.Add(MakeShared<FJsonValueObject>(NodeJson));
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
		Data->SetStringField(TEXT("graph"), Target->GetName());
		Data->SetStringField(TEXT("graph_kind"), GraphKind);
		Data->SetArrayField(TEXT("function_graphs"), FunctionNames);
		Data->SetNumberField(TEXT("node_count"), NodesJson.Num());
		Data->SetArrayField(TEXT("nodes"), NodesJson);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> AudioComponentToJson(UAudioComponent* Component, const FVector& Listener)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		if (!Component)
		{
			return Entry;
		}
		AActor* Owner = Component->GetOwner();
		PutActor(Entry, Owner);
		Entry->SetStringField(TEXT("component"), Component->GetName());
		Entry->SetStringField(TEXT("component_class"), ClassName(Component));
		USoundBase* Sound = Component->GetSound();
		Entry->SetStringField(TEXT("sound"), Sound ? Sound->GetName() : TEXT(""));
		Entry->SetStringField(TEXT("sound_path"), Sound ? Sound->GetPathName() : TEXT(""));
		Entry->SetBoolField(TEXT("is_playing"), Component->IsPlaying());
		Entry->SetNumberField(TEXT("volume"), Component->VolumeMultiplier);
		Entry->SetNumberField(TEXT("pitch"), Component->PitchMultiplier);
		Entry->SetBoolField(TEXT("spatialized"), Component->bAllowSpatialization);
		Entry->SetBoolField(TEXT("ui_sound"), Component->bIsUISound);
		bool bLooping = false;
		float Duration = Sound ? Sound->Duration : 0.0f;
		if (USoundWave* Wave = Cast<USoundWave>(Sound))
		{
			bLooping = Wave->bLooping;
			Duration = Wave->Duration;
		}
		Entry->SetBoolField(TEXT("looping"), bLooping);
		Entry->SetNumberField(TEXT("duration"), Duration);
		const FVector Location = Component->GetComponentLocation();
		Entry->SetArrayField(TEXT("world_location"), Vec(Location));
		Entry->SetNumberField(TEXT("distance_to_listener"), FVector::Dist(Location, Listener));
		return Entry;
	}

	TSharedRef<FJsonObject> CmdInspectPlayingAudio(const TSharedPtr<FJsonObject>& Args)
	{
		UWorld* World = GetPreferredWorld(GetBool(Args, TEXT("prefer_pie"), true));
		if (!World)
		{
			return Fail(TEXT("no_world"), TEXT("No editor or PIE world."));
		}
		const bool bPlayingOnly = GetBool(Args, TEXT("playing_only"), true);
		const int32 MaxCount = GetInt(Args, TEXT("max"), 80);
		FVector Listener = FVector::ZeroVector;
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			Listener = Pawn->GetActorLocation();
		}

		TArray<TSharedPtr<FJsonValue>> Sources;
		int32 PlayingCount = 0;
		int32 Visited = 0;
		for (TObjectIterator<UAudioComponent> It; It; ++It)
		{
			UAudioComponent* Component = *It;
			if (!Component || Component->GetWorld() != World)
			{
				continue;
			}
			++Visited;
			const bool bPlaying = Component->IsPlaying();
			if (bPlaying)
			{
				++PlayingCount;
			}
			if (bPlayingOnly && !bPlaying)
			{
				continue;
			}
			if (Sources.Num() >= MaxCount)
			{
				continue;
			}
			Sources.Add(MakeShared<FJsonValueObject>(AudioComponentToJson(Component, Listener)));
		}

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("pie"), World->IsPlayInEditor() || World->IsGameWorld());
		Data->SetStringField(TEXT("world"), World->GetMapName());
		Data->SetArrayField(TEXT("listener"), Vec(Listener));
		Data->SetNumberField(TEXT("visited"), Visited);
		Data->SetNumberField(TEXT("playing_count"), PlayingCount);
		Data->SetNumberField(TEXT("returned"), Sources.Num());
		Data->SetArrayField(TEXT("sources"), Sources);
		return Ok(Data);
	}
}

TSharedRef<FJsonObject> FOrganoidAIBridgeCommands::Dispatch(
	const FString& Command,
	const TSharedPtr<FJsonObject>& Args,
	const TSharedPtr<FJsonObject>& Session,
	FOrganoidAIBridgeLogSink* LogSink)
{
	const FString Normalized = Command.ToLower().Replace(TEXT("unreal_"), TEXT(""));

	if (Normalized == TEXT("ping") || Normalized == TEXT("health"))
	{
		return CmdPing();
	}
	if (Normalized == TEXT("get_editor_state"))
	{
		return CmdEditorState();
	}
	if (Normalized == TEXT("get_pie_state"))
	{
		return CmdPieState();
	}
	if (Normalized == TEXT("get_player_pawn") || Normalized == TEXT("get_pawn"))
	{
		return CmdPlayerPawn(Args);
	}
	if (Normalized == TEXT("get_actor"))
	{
		return CmdGetActor(Args);
	}
	if (Normalized == TEXT("get_actor_property") || Normalized == TEXT("get_property"))
	{
		return CmdGetActorProperty(Args);
	}
	if (Normalized == TEXT("get_light_component"))
	{
		return CmdGetLightComponent(Args);
	}
	if (Normalized == TEXT("get_component"))
	{
		return CmdGetComponent(Args);
	}
	if (Normalized == TEXT("list_actors_near"))
	{
		return CmdListActorsNear(Args);
	}
	if (Normalized == TEXT("get_collision"))
	{
		return CmdGetCollision(Args);
	}
	if (Normalized == TEXT("capsule_sweep"))
	{
		return CmdCapsuleSweep(Args);
	}
	if (Normalized == TEXT("overlap_query"))
	{
		return CmdOverlapQuery(Args);
	}
	if (Normalized == TEXT("get_output_log"))
	{
		return CmdOutputLog(Args, LogSink);
	}
	if (Normalized == TEXT("find_blueprint"))
	{
		return CmdFindBlueprint(Args);
	}
	if (Normalized == TEXT("get_blueprint_components"))
	{
		return CmdBlueprintComponents(Args);
	}
	if (Normalized == TEXT("get_blueprint_members"))
	{
		return CmdBlueprintMembers(Args);
	}
	if (Normalized == TEXT("get_user_defined_enum") || Normalized == TEXT("get_enum"))
	{
		return CmdGetUserDefinedEnum(Args);
	}
	if (Normalized == TEXT("get_blueprint_graph"))
	{
		return CmdGetBlueprintGraph(Args);
	}
	if (Normalized == TEXT("inspect_playing_audio") || Normalized == TEXT("get_playing_audio"))
	{
		return CmdInspectPlayingAudio(Args);
	}
	if (Normalized == TEXT("inspect_admin_block4_navmesh"))
	{
		return OrganoidAIBridgeWrites::InspectAdminBlock4NavMesh(Args);
	}

	if (OrganoidAIBridgePlaytest::IsPlaytestCommand(Normalized))
	{
		return OrganoidAIBridgePlaytest::Dispatch(Normalized, Args);
	}

	if (OrganoidAIBridgeWrites::IsLifecycleCommand(Normalized))
	{
		return OrganoidAIBridgeWrites::Dispatch(Normalized, Args, Session, LogSink);
	}

	if (Normalized == TEXT("set_collision")
		|| Normalized == TEXT("set_visibility")
		|| Normalized == TEXT("set_transform")
		|| Normalized == TEXT("compile_blueprint")
		|| Normalized == TEXT("save_asset")
		|| Normalized == TEXT("save_maps")
		|| Normalized == TEXT("save_map")
		|| Normalized == TEXT("move_actor_to_level")
		|| Normalized == TEXT("set_actor_property")
		|| Normalized == TEXT("delete_actor")
		|| Normalized == TEXT("set_component_property")
		|| Normalized == TEXT("rerun_construction")
		|| Normalized == TEXT("add_blueprint_variable")
		|| Normalized == TEXT("add_scs_component")
		|| Normalized == TEXT("author_hologram_apply_state")
		|| Normalized == TEXT("author_light_controller_set_zone")
		|| Normalized == TEXT("author_admin_room_trigger_lighting_hook")
		|| Normalized == TEXT("author_access_door_facility_state_listener")
		|| Normalized == TEXT("author_light_controller_facility_state_listener")
		|| Normalized == TEXT("connect_blueprint_pins")
		|| Normalized == TEXT("spawn_blueprint_actor")
		|| Normalized == TEXT("spawn_s20_zone_light")
		|| Normalized == TEXT("set_s20_light_intensity")
		|| Normalized == TEXT("invoke_s20_set_lighting_zone")
		|| Normalized == TEXT("delete_s22_legacy_admin_ambience")
		|| Normalized == TEXT("spawn_s22_admin_audio_zones")
		|| Normalized == TEXT("spawn_neuro_navmesh_bounds")
		|| Normalized == TEXT("spawn_neuro_research_station")
		|| Normalized == TEXT("spawn_admin_research_wing_connector")
		|| Normalized == TEXT("spawn_admin_research_wing_keycard")
		|| Normalized == TEXT("trim_spine_landing_admin")
		|| Normalized == TEXT("spawn_admin_block4_security_officer")
		|| Normalized == TEXT("spawn_admin_block4_navmesh_bounds")
		|| Normalized == TEXT("save_admin_block4_navmesh_prerequisite")
		|| Normalized == TEXT("spawn_neuro_arrival_lab_dressing")
		|| Normalized == TEXT("spawn_neuro_ch3_containment_evidence")
		|| Normalized == TEXT("spawn_neuro_ch4_transformed_personnel")
		|| Normalized == TEXT("configure_neuro_power_failure_discovery")
		|| Normalized == TEXT("spawn_neuro_power_diagnostic")
		|| Normalized == TEXT("create_neurogenetics_mission")
		|| Normalized == TEXT("expand_neurogenetics_mission_beat3")
		|| Normalized == TEXT("expand_neurogenetics_mission_beat4")
		|| Normalized == TEXT("spawn_neuro_neural_mapping_array")
		|| Normalized == TEXT("spawn_neuro_research_load_cutoff")
		|| Normalized == TEXT("spawn_neuro_neural_mapping_terminal"))
	{
		return Fail(
			TEXT("needs_prepare"),
			FString::Printf(
				TEXT("Direct command '%s' is disabled. Use prepare_write, dual approve_write (user + second_review), then execute_write on that change_id."),
				*Normalized));
	}

	return Fail(TEXT("unknown_command"), FString::Printf(TEXT("Command '%s' is not allowlisted."), *Command));
}
