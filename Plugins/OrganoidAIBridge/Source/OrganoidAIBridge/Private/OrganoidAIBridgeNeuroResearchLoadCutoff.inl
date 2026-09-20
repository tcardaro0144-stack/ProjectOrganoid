} // leave anonymous namespace so BrushComponent.h is included at global scope

#include "Components/BrushComponent.h"

namespace
{
	// Fixed Neuro research-load cutoff spawn —
	// spawn_neuro_research_load_cutoff / neuro_research_load_cutoff_v1.
	// Temporary Engine Cube blockout presentation — replaceable, not final art.
	// Requires persisted clean Beat 3 DA_Mission_NeuroGenetics (rejects Beat 2).
	const TCHAR* NeuroResearchLoadCutoffSpec = TEXT("neuro_research_load_cutoff_v1");
	const TCHAR* NeuroResearchLoadCutoffAction = TEXT("spawn_neuro_research_load_cutoff");
	const TCHAR* NeuroResearchLoadCutoffLabel = TEXT("EmergencyCutoff_NeuroResearchLoad");
	const TCHAR* NeuroResearchLoadCutoffClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroResearchLoadCutoffClassName = TEXT("ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroResearchLoadCutoffCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroResearchLoadCutoffRequiredActive = TEXT("Obj_IsolateNeuroResearchLoad");
	const TCHAR* NeuroResearchLoadCutoffObjectiveEvent = TEXT("Event_NeuroResearchLoadIsolated");
	const TCHAR* NeuroResearchLoadCutoffReplayGuard = TEXT("Obj_IsolateNeuroResearchLoad");
	const TCHAR* NeuroResearchLoadCutoffInspectPrompt = TEXT("Isolate Research Load");
	const TCHAR* NeuroResearchLoadCutoffReviewPrompt = TEXT("Research Load Isolated");
	const TCHAR* NeuroResearchLoadCutoffSpeaker = TEXT("Nathan");
	const TCHAR* NeuroResearchLoadCutoffResponse =
		TEXT("That cut the feed. The array\u2019s offline, but its last mapping data should still be here.");
	const FVector NeuroResearchLoadCutoffLocation(-100.f, -600.f, -1100.f);
	const FRotator NeuroResearchLoadCutoffRotation = FRotator::ZeroRotator;
	const FVector NeuroResearchLoadCutoffScale = FVector::OneVector;
	constexpr float NeuroResearchLoadCutoffInteractionRange = 175.f;
	constexpr float NeuroResearchLoadCutoffNotifySeconds = 4.f;
	const FVector NeuroResearchLoadCutoffPedestalRel(0.f, 0.f, 35.f);
	const FVector NeuroResearchLoadCutoffPedestalScale(0.45f, 0.45f, 0.70f);
	const FVector NeuroResearchLoadCutoffColumnRel(0.f, 0.f, 100.f);
	const FVector NeuroResearchLoadCutoffColumnScale(0.65f, 0.30f, 0.60f);
	const FVector NeuroResearchLoadCutoffHeadRel(0.f, 0.f, 150.f);
	const FVector NeuroResearchLoadCutoffHeadScale(0.80f, 0.40f, 0.25f);
	constexpr float NeuroResearchLoadCutoffHostKeepout = 400.f;
	constexpr float NeuroResearchLoadCutoffDoorKeepout = 400.f;
	constexpr float NeuroResearchLoadCutoffHazardKeepout = 400.f;
	constexpr float NeuroResearchLoadCutoffFloorZTarget = -1100.f;
	constexpr float NeuroResearchLoadCutoffFloorZEps = 150.f;
	constexpr float NeuroResearchLoadCutoffNormalUpMin = 0.7f;
	constexpr float NeuroResearchLoadCutoffArrayInteractionRange = 200.f;
	constexpr float NeuroResearchLoadCutoffCubeHalf = 50.f;

	UStaticMeshComponent* NeuroResearchLoadCutoff_FindMeshComponent(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Actor->GetClass(), PropertyName))
		{
			return Cast<UStaticMeshComponent>(Prop->GetObjectPropertyValue_InContainer(Actor));
		}
		return nullptr;
	}

	FString NeuroResearchLoadCutoff_RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Forbidden[] = {
			TEXT("label"), TEXT("class"), TEXT("class_path"), TEXT("location"), TEXT("rotation"), TEXT("scale"),
			TEXT("mesh"), TEXT("pedestal"), TEXT("column"), TEXT("array_head"), TEXT("package"),
			TEXT("destination_package"), TEXT("level"), TEXT("copy"), TEXT("copy_from"), TEXT("source_actor"),
			TEXT("interaction_range"), TEXT("objective_event"), TEXT("objective_event_id"),
			TEXT("required_active"), TEXT("required_active_objective_id"),
			TEXT("completed_objective"), TEXT("prompt"), TEXT("inspection_prompt"), TEXT("review_prompt"),
			TEXT("speaker"), TEXT("response"), TEXT("notification_duration"), TEXT("transform"),
		};
		for (const TCHAR* Key : Forbidden)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(
					TEXT("Arbitrary argument '%s' is refused. spawn_neuro_research_load_cutoff uses fixed native constants only."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroResearchLoadCutoffSpec);
		if (!Spec.Equals(NeuroResearchLoadCutoffSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_research_load_cutoff_v1.");
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_RequireBeat3MissionPersistedClean()
	{
		UObject* Mission = FindNeuroGeneticsMissionAssetExact();
		if (!Mission)
		{
			return TEXT(
				"DA_Mission_NeuroGenetics is required and must already exist (persisted, clean) matching neurogenetics_mission_beat3_v1. ZERO writes.");
		}
		if (const FString Beat3Mismatch = NeuroGeneticsMissionBeat3MismatchReason(Mission); !Beat3Mismatch.IsEmpty())
		{
			const FString Beat2Mismatch = NeuroGeneticsMissionMismatchReason(Mission);
			if (Beat2Mismatch.IsEmpty())
			{
				return TEXT(
					"DA_Mission_NeuroGenetics is still exact Beat 2. Require persisted clean Beat 3 (expand_neurogenetics_mission_beat3 + save) before spawn_neuro_research_load_cutoff. ZERO writes.");
			}
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 3: %s. Fail closed — no opportunistic repair."),
				*Beat3Mismatch);
		}
		if (UPackage* MissionPkg = Mission->GetOutermost())
		{
			if (MissionPkg->IsDirty())
			{
				return TEXT("DA_Mission_NeuroGenetics package is dirty. Require persisted clean Beat 3 mission asset. ZERO writes.");
			}
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_RequireLevelsLoadedVisible(UWorld* World)
	{
		return NeuroMappingArray_RequireLevelsLoadedVisible(World);
	}

	FBox NeuroResearchLoadCutoff_ExpectedMeshWorldBox(const FVector& RelLoc, const FVector& RelScale)
	{
		const FVector Half = FVector(
			NeuroResearchLoadCutoffCubeHalf * RelScale.X,
			NeuroResearchLoadCutoffCubeHalf * RelScale.Y,
			NeuroResearchLoadCutoffCubeHalf * RelScale.Z);
		const FVector Center = NeuroResearchLoadCutoffLocation + RelLoc;
		return FBox(Center - Half, Center + Half);
	}

	FString NeuroResearchLoadCutoff_MeshPresentationMismatch(
		AActor* Actor,
		const TCHAR* ComponentName,
		const FVector& RelLocation,
		const FVector& RelScale)
	{
		return NeuroMappingArray_MeshPresentationMismatch(
			Actor, ComponentName, TEXT("/Engine/BasicShapes/Cube"), RelLocation, RelScale);
	}

	FString NeuroResearchLoadCutoff_ActorMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ClassName(Actor).Equals(NeuroResearchLoadCutoffClassName, ESearchCase::CaseSensitive)
			&& !ClassName(Actor).Contains(TEXT("ProjectOrganoidInspectableInstrument")))
		{
			return FString::Printf(
				TEXT("class '%s' expected ProjectOrganoidInspectableInstrument"),
				*ClassName(Actor));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (!ActorLabel(Actor).Equals(NeuroResearchLoadCutoffLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' expected EmergencyCutoff_NeuroResearchLoad"), *ActorLabel(Actor));
		}
		if (const FString Xform = TransformMismatch(
				Actor, NeuroResearchLoadCutoffLocation, NeuroResearchLoadCutoffRotation, NeuroResearchLoadCutoffScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("InteractionRange"), NeuroResearchLoadCutoffInteractionRange);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bIsInteractable"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bHasBeenInspected"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroResearchLoadCutoffRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ObjectiveEventId"), NeuroResearchLoadCutoffObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroResearchLoadCutoffReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionPrompt"), NeuroResearchLoadCutoffInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ReviewPrompt"), NeuroResearchLoadCutoffReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("SpeakerLabel"), NeuroResearchLoadCutoffSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionResponseText"), NeuroResearchLoadCutoffResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("NotificationDurationSeconds"), NeuroResearchLoadCutoffNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearchLoadCutoff_MeshPresentationMismatch(
				Actor, TEXT("PedestalMesh"), NeuroResearchLoadCutoffPedestalRel, NeuroResearchLoadCutoffPedestalScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearchLoadCutoff_MeshPresentationMismatch(
				Actor, TEXT("ColumnMesh"), NeuroResearchLoadCutoffColumnRel, NeuroResearchLoadCutoffColumnScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearchLoadCutoff_MeshPresentationMismatch(
				Actor, TEXT("ArrayHeadMesh"), NeuroResearchLoadCutoffHeadRel, NeuroResearchLoadCutoffHeadScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_ApplyFields(AActor* Actor)
	{
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("InteractionRange"), NeuroResearchLoadCutoffInteractionRange);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Actor, TEXT("bIsInteractable"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Actor, TEXT("bHasBeenInspected"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroResearchLoadCutoffRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("ObjectiveEventId"), NeuroResearchLoadCutoffObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroResearchLoadCutoffReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionPrompt"), NeuroResearchLoadCutoffInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("ReviewPrompt"), NeuroResearchLoadCutoffReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_SetTextProperty(Actor, TEXT("SpeakerLabel"), NeuroResearchLoadCutoffSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionResponseText"), NeuroResearchLoadCutoffResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("NotificationDurationSeconds"), NeuroResearchLoadCutoffNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InteractionPrompt"), NeuroResearchLoadCutoffInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}

		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroResearchLoadCutoff_FindMeshComponent(Actor, TEXT("PedestalMesh")),
				NeuroResearchLoadCutoffCubePath,
				NeuroResearchLoadCutoffPedestalRel,
				NeuroResearchLoadCutoffPedestalScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("PedestalMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroResearchLoadCutoff_FindMeshComponent(Actor, TEXT("ColumnMesh")),
				NeuroResearchLoadCutoffCubePath,
				NeuroResearchLoadCutoffColumnRel,
				NeuroResearchLoadCutoffColumnScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ColumnMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroResearchLoadCutoff_FindMeshComponent(Actor, TEXT("ArrayHeadMesh")),
				NeuroResearchLoadCutoffCubePath,
				NeuroResearchLoadCutoffHeadRel,
				NeuroResearchLoadCutoffHeadScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ArrayHeadMesh (temporary Cube blockout): %s"), *Error);
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_FloorTraceAt(UWorld* World, const FVector& SampleXY)
	{
		const FVector Start(SampleXY.X, SampleXY.Y, NeuroResearchLoadCutoffLocation.Z + 80.f);
		const FVector End(SampleXY.X, SampleXY.Y, NeuroResearchLoadCutoffLocation.Z - 250.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuroResearchLoadCutoffFloor), false);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
		if (!bHit || !Hit.bBlockingHit)
		{
			return FString::Printf(
				TEXT("Downward floor trace missed at (%.0f,%.0f). Unsupported placement."),
				SampleXY.X,
				SampleXY.Y);
		}
		if (Hit.ImpactNormal.Z < NeuroResearchLoadCutoffNormalUpMin)
		{
			return FString::Printf(
				TEXT("Floor normal Z=%.3f at (%.0f,%.0f) is not upward-facing."),
				Hit.ImpactNormal.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		if (FMath::Abs(Hit.ImpactPoint.Z - NeuroResearchLoadCutoffFloorZTarget) > NeuroResearchLoadCutoffFloorZEps
			&& FMath::Abs(Hit.ImpactPoint.Z - (-1200.f)) > NeuroResearchLoadCutoffFloorZEps)
		{
			return FString::Printf(
				TEXT("Supported surface Z=%.1f at (%.0f,%.0f) is not near -1100."),
				Hit.ImpactPoint.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_NavProjection(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSystem)
		{
			return TEXT("Neuro navigation system unavailable. ZERO writes.");
		}
		const FVector Samples[] = {
			NeuroResearchLoadCutoffLocation,
			NeuroResearchLoadCutoffLocation + FVector(40.f, 0.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(-40.f, 0.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(0.f, 40.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(0.f, -40.f, 0.f),
		};
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Samples); ++Index)
		{
			FNavLocation Projected;
			if (!NavSystem->ProjectPointToNavigation(Samples[Index], Projected, FVector(200.f, 200.f, 300.f)))
			{
				return FString::Printf(
					TEXT("Nav projection failed at sample %d (%.0f,%.0f,%.0f). Traversal obstruction or missing nav."),
					Index,
					Samples[Index].X,
					Samples[Index].Y,
					Samples[Index].Z);
			}
			const float XY = FVector::Dist2D(Samples[Index], Projected.Location);
			const float Z = FMath::Abs(Samples[Index].Z - Projected.Location.Z);
			if (XY > 100.f || Z > 150.f)
			{
				return FString::Printf(
					TEXT("Nav projection too far at sample %d (xy=%.1f z=%.1f)."), Index, XY, Z);
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetNumberField(TEXT("sample_index"), Index);
			Row->SetArrayField(TEXT("requested"), Vec(Samples[Index]));
			Row->SetArrayField(TEXT("projected"), Vec(Projected.Location));
			Row->SetNumberField(TEXT("xy_delta"), XY);
			Row->SetNumberField(TEXT("z_delta"), Z);
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		OutMargins->SetArrayField(TEXT("nav_projections"), Rows);
		return TEXT("");
	}

	// Exact ProjectOrganoidStreamingVolume class path — logical region/streaming metadata only.
	// Not a label exemption. Not a blanket AVolume / QueryOnly allowlist.
	const TCHAR* NeuroResearchLoadCutoffStreamingVolumeClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidStreamingVolume");

	/**
	 * Classify an intersecting actor for MeshBoundsClearance.
	 * Returns empty if the actor is proven nonphysical streaming metadata and may be ignored.
	 * Returns a fail-closed reason if the actor is that exact class but has unexpected blocking
	 * collision semantics. Returns "reject" if the actor is not that class (caller keeps reject path).
	 *
	 * Static matrix (compile-time contract of this helper):
	 *  1. exact ProjectOrganoidStreamingVolume + TriggerVolume QueryOnly + Pawn Overlap
	 *     → categorized nonphysical metadata → ignore region-sized AABB
	 *  2. same class with Pawn Block / non-QueryOnly / wrong component → fail closed
	 *  3. generic/unknown Volume (not that class path) → reject (caller fails AABB intersect)
	 *  4. keep-list / physical / BlockingVolume / nav / hazard / interactable → unchanged reject
	 */
	FString NeuroResearchLoadCutoff_StreamingVolumeIgnoreOrFail(AActor* Other)
	{
		if (!Other)
		{
			return TEXT("reject");
		}
		UClass* OtherClass = Other->GetClass();
		if (!OtherClass
			|| !OtherClass->GetPathName().Equals(NeuroResearchLoadCutoffStreamingVolumeClassPath, ESearchCase::CaseSensitive))
		{
			// Not the established streaming-volume class — caller rejects on AABB intersect.
			return TEXT("reject");
		}

		UBoxComponent* Trigger = nullptr;
		{
			TArray<UBoxComponent*> Boxes;
			Other->GetComponents<UBoxComponent>(Boxes);
			for (UBoxComponent* Box : Boxes)
			{
				if (Box && Box->GetName().Equals(TEXT("TriggerVolume"), ESearchCase::CaseSensitive)
					&& Box->GetClass() == UBoxComponent::StaticClass())
				{
					Trigger = Box;
					break;
				}
			}
		}
		if (!Trigger)
		{
			return FString::Printf(
				TEXT("ProjectOrganoidStreamingVolume '%s' lacks expected TriggerVolume box component. Fail closed — not ignored."),
				*ActorLabel(Other));
		}
		if (Trigger->GetCollisionEnabled() != ECollisionEnabled::QueryOnly)
		{
			return FString::Printf(
				TEXT("ProjectOrganoidStreamingVolume '%s' TriggerVolume is not QueryOnly. Fail closed — unexpected collision semantics."),
				*ActorLabel(Other));
		}
		if (Trigger->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Overlap)
		{
			return FString::Printf(
				TEXT("ProjectOrganoidStreamingVolume '%s' TriggerVolume Pawn response is not Overlap (blocks or ignores). Fail closed — unexpected collision semantics."),
				*ActorLabel(Other));
		}
		// Exact expected nonphysical streaming semantics — ignore region-sized actor bounds only.
		return TEXT("");
	}

	// Exact engine NavMeshBoundsVolume class path — navigation-generation bounds metadata only.
	// Not a label exemption. Not a blanket AVolume / navigation-volume / QueryOnly allowlist.
	// Does not bypass separate nav projection / traversal gates in PlacementGeometry.
	const TCHAR* NeuroResearchLoadCutoffNavMeshBoundsClassPath =
		TEXT("/Script/NavigationSystem.NavMeshBoundsVolume");

	/**
	 * Classify an intersecting actor for MeshBoundsClearance (nav-bounds path).
	 * Returns empty if proven nonblocking NavMeshBoundsVolume metadata and may be ignored.
	 * Returns fail-closed reason if exact class has unexpected blocking collision semantics.
	 * Returns "reject" if not that class (caller keeps reject / other-ignore paths).
	 *
	 * Static matrix contract:
	 *  1. exact NavMeshBoundsVolume + brush + NoCollision (does not block Pawn) → ignore metadata AABB
	 *  2. same class with enabled collision that Blocks Pawn → fail closed
	 *  3. BlockingVolume → reject (not this class path)
	 *  4. NavModifierVolume / unknown nav volume → reject
	 *  5. ProjectOrganoidStreamingVolume → handled exclusively by StreamingVolumeIgnoreOrFail (R6C)
	 *  6. physical / keep-list → unchanged reject
	 *  7. native nav projection/traversal still run after MeshBoundsClearance
	 */
	FString NeuroResearchLoadCutoff_NavMeshBoundsIgnoreOrFail(AActor* Other)
	{
		if (!Other)
		{
			return TEXT("reject");
		}
		UClass* OtherClass = Other->GetClass();
		if (!OtherClass
			|| !OtherClass->GetPathName().Equals(NeuroResearchLoadCutoffNavMeshBoundsClassPath, ESearchCase::CaseSensitive))
		{
			return TEXT("reject");
		}

		ANavMeshBoundsVolume* NavBounds = Cast<ANavMeshBoundsVolume>(Other);
		if (!NavBounds)
		{
			return FString::Printf(
				TEXT("Actor '%s' class path is NavMeshBoundsVolume but Cast<ANavMeshBoundsVolume> failed. Fail closed."),
				*ActorLabel(Other));
		}
		UBrushComponent* Brush = NavBounds->GetBrushComponent();
		if (!Brush)
		{
			return FString::Printf(
				TEXT("NavMeshBoundsVolume '%s' lacks expected brush component. Fail closed — not ignored."),
				*ActorLabel(Other));
		}

		const ECollisionEnabled::Type Enabled = Brush->GetCollisionEnabled();
		if (Enabled == ECollisionEnabled::NoCollision)
		{
			// Expected nav-generation bounds metadata — region AABB may be ignored.
			return TEXT("");
		}

		// Collision is enabled — only ignore if Pawn is explicitly non-blocking.
		const ECollisionResponse PawnResponse = Brush->GetCollisionResponseToChannel(ECC_Pawn);
		if (PawnResponse == ECR_Block)
		{
			return FString::Printf(
				TEXT("NavMeshBoundsVolume '%s' brush collision is enabled and Blocks Pawn. Fail closed — unexpected blocking semantics."),
				*ActorLabel(Other));
		}
		if (Enabled == ECollisionEnabled::QueryAndPhysics || Enabled == ECollisionEnabled::PhysicsOnly)
		{
			return FString::Printf(
				TEXT("NavMeshBoundsVolume '%s' brush has physics-enabled collision (%d). Fail closed — not navigation-bounds metadata."),
				*ActorLabel(Other),
				static_cast<int32>(Enabled));
		}
		// QueryOnly without Pawn Block — still atypical for NavMeshBoundsVolume; fail closed.
		return FString::Printf(
			TEXT("NavMeshBoundsVolume '%s' brush collision is enabled (not NoCollision). Fail closed — unexpected collision semantics."),
			*ActorLabel(Other));
	}

	FString NeuroResearchLoadCutoff_MeshBoundsClearance(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FBox Boxes[] = {
			NeuroResearchLoadCutoff_ExpectedMeshWorldBox(
				NeuroResearchLoadCutoffPedestalRel, NeuroResearchLoadCutoffPedestalScale),
			NeuroResearchLoadCutoff_ExpectedMeshWorldBox(
				NeuroResearchLoadCutoffColumnRel, NeuroResearchLoadCutoffColumnScale),
			NeuroResearchLoadCutoff_ExpectedMeshWorldBox(
				NeuroResearchLoadCutoffHeadRel, NeuroResearchLoadCutoffHeadScale),
		};
		FBox Combined = Boxes[0] + Boxes[1] + Boxes[2];
		OutMargins->SetArrayField(TEXT("combined_aabb_min"), Vec(Combined.Min));
		OutMargins->SetArrayField(TEXT("combined_aabb_max"), Vec(Combined.Max));

		TArray<TSharedPtr<FJsonValue>> IgnoredStreaming;
		TArray<TSharedPtr<FJsonValue>> IgnoredNavBounds;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other)
			{
				continue;
			}
			const FString Label = ActorLabel(Other);
			if (Label.Equals(NeuroResearchLoadCutoffLabel, ESearchCase::CaseSensitive)
				|| Label.Equals(TEXT("NeuroGenetics_FloorPlate"), ESearchCase::CaseSensitive)
				|| Label.Contains(TEXT("FloorPlate")))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroResearchLoadCutoffLocation) > 800.f)
			{
				continue;
			}
			FVector Origin, Extent;
			Other->GetActorBounds(false, Origin, Extent);
			const FBox OtherBox(Origin - Extent, Origin + Extent);
			if (!Combined.Intersect(OtherBox))
			{
				continue;
			}

			const FString StreamingDecision = NeuroResearchLoadCutoff_StreamingVolumeIgnoreOrFail(Other);
			if (StreamingDecision.IsEmpty())
			{
				TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("label"), Label);
				Row->SetStringField(TEXT("class_path"), NeuroResearchLoadCutoffStreamingVolumeClassPath);
				Row->SetStringField(TEXT("category"), TEXT("nonphysical_streaming_metadata"));
				Row->SetStringField(
					TEXT("reason"),
					TEXT("exact ProjectOrganoidStreamingVolume + TriggerVolume QueryOnly + Pawn Overlap"));
				IgnoredStreaming.Add(MakeShared<FJsonValueObject>(Row));
				continue;
			}
			if (!StreamingDecision.Equals(TEXT("reject"), ESearchCase::CaseSensitive))
			{
				// Exact streaming class with unexpected blocking/collision semantics.
				return StreamingDecision;
			}

			const FString NavBoundsDecision = NeuroResearchLoadCutoff_NavMeshBoundsIgnoreOrFail(Other);
			if (NavBoundsDecision.IsEmpty())
			{
				TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("label"), Label);
				Row->SetStringField(TEXT("class_path"), NeuroResearchLoadCutoffNavMeshBoundsClassPath);
				Row->SetStringField(TEXT("category"), TEXT("navigation_generation_bounds_metadata"));
				Row->SetStringField(
					TEXT("reason"),
					TEXT("exact NavMeshBoundsVolume + brush NoCollision (does not block Pawn)"));
				IgnoredNavBounds.Add(MakeShared<FJsonValueObject>(Row));
				continue;
			}
			if (!NavBoundsDecision.Equals(TEXT("reject"), ESearchCase::CaseSensitive))
			{
				return NavBoundsDecision;
			}

			return FString::Printf(
				TEXT("Configured Cube blockout AABB intersects authored actor '%s'. Fail closed — no fallback transform."),
				*Label);
		}
		OutMargins->SetArrayField(TEXT("ignored_streaming_volumes"), IgnoredStreaming);
		OutMargins->SetArrayField(TEXT("ignored_nav_mesh_bounds"), IgnoredNavBounds);
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_PlacementGeometry(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FVector FootSamples[] = {
			NeuroResearchLoadCutoffLocation,
			NeuroResearchLoadCutoffLocation + FVector(40.f, 40.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(40.f, -40.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(-40.f, 40.f, 0.f),
			NeuroResearchLoadCutoffLocation + FVector(-40.f, -40.f, 0.f),
		};
		for (const FVector& Sample : FootSamples)
		{
			if (const FString TraceError = NeuroResearchLoadCutoff_FloorTraceAt(World, Sample); !TraceError.IsEmpty())
			{
				return TraceError;
			}
		}

		if (const FString BoundsError = NeuroResearchLoadCutoff_MeshBoundsClearance(World, OutMargins); !BoundsError.IsEmpty())
		{
			return BoundsError;
		}
		if (const FString NavError = NeuroResearchLoadCutoff_NavProjection(World, OutMargins); !NavError.IsEmpty())
		{
			return NavError;
		}

		// Mapping array interaction clearance: dist must remain > 200+175.
		{
			TArray<AActor*> ArrayMatches = FindOwnedByExactLabel(World, TEXT("NeuralMappingArray_NeuroGenetics"), NeuroPackage);
			if (ArrayMatches.Num() != 1)
			{
				return FString::Printf(
					TEXT("NeuralMappingArray_NeuroGenetics Neuro count=%d expected=1 for interaction clearance."),
					ArrayMatches.Num());
			}
			const float Dist = FVector::Dist(ArrayMatches[0]->GetActorLocation(), NeuroResearchLoadCutoffLocation);
			const float Combined =
				NeuroResearchLoadCutoffInteractionRange + NeuroResearchLoadCutoffArrayInteractionRange;
			OutMargins->SetNumberField(TEXT("array_distance"), Dist);
			OutMargins->SetNumberField(TEXT("array_combined_radii"), Combined);
			OutMargins->SetNumberField(TEXT("array_clearance_margin"), Dist - Combined);
			if (Combined >= Dist)
			{
				return FString::Printf(
					TEXT("Interaction space overlaps NeuralMappingArray_NeuroGenetics (combined radii %.0f >= dist %.1f). Fail closed."),
					Combined,
					Dist);
			}
		}

		struct FDisjoint
		{
			const TCHAR* Label;
			float Range;
		};
		const FDisjoint Interactables[] = {
			{TEXT("PowerPanel_NeuroBackup"), 220.f},
			{TEXT("DataPad_NeuroPowerDiagnostics"), 200.f},
			{TEXT("DataPad_NeuroContainment"), 200.f},
			{TEXT("DataPad_NeuroResearchFailure"), 200.f},
			{TEXT("ResearchStation_NeuroGenetics"), 250.f},
		};
		TArray<TSharedPtr<FJsonValue>> ClearanceRows;
		for (const FDisjoint& Entry : Interactables)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(
					TEXT("%s Neuro count=%d expected=1 for interaction clearance."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroResearchLoadCutoffLocation);
			const float Combined = NeuroResearchLoadCutoffInteractionRange + Entry.Range;
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Entry.Label);
			Row->SetNumberField(TEXT("distance"), Dist);
			Row->SetNumberField(TEXT("combined_radii"), Combined);
			Row->SetNumberField(TEXT("margin"), Dist - Combined);
			ClearanceRows.Add(MakeShared<FJsonValueObject>(Row));
			if (Combined >= Dist)
			{
				return FString::Printf(
					TEXT("Interaction space overlaps %s (combined radii %.0f >= dist %.1f)."),
					Entry.Label,
					Combined,
					Dist);
			}
		}
		OutMargins->SetArrayField(TEXT("interactable_clearance"), ClearanceRows);

		struct FKeepout
		{
			const TCHAR* Label;
			float MinDist;
		};
		const FKeepout HostsAndHazard[] = {
			{TEXT("Host_Neuro_1"), NeuroResearchLoadCutoffHostKeepout},
			{TEXT("Host_Neuro_2"), NeuroResearchLoadCutoffHostKeepout},
			{TEXT("Host_Neuro_3"), NeuroResearchLoadCutoffHostKeepout},
			{TEXT("Host_Neuro_Researcher"), NeuroResearchLoadCutoffHostKeepout},
			{TEXT("Hazard_ScrubberLeak"), NeuroResearchLoadCutoffHazardKeepout},
		};
		TArray<TSharedPtr<FJsonValue>> KeepoutRows;
		for (const FKeepout& Entry : HostsAndHazard)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1 for keepout."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroResearchLoadCutoffLocation);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Entry.Label);
			Row->SetNumberField(TEXT("distance"), Dist);
			Row->SetNumberField(TEXT("min_dist"), Entry.MinDist);
			KeepoutRows.Add(MakeShared<FJsonValueObject>(Row));
			if (Dist < Entry.MinDist)
			{
				return FString::Printf(
					TEXT("Keepout violated vs %s (dist %.1f < %.0f)."), Entry.Label, Dist, Entry.MinDist);
			}
		}
		OutMargins->SetArrayField(TEXT("host_hazard_keepouts"), KeepoutRows);

		TArray<AActor*> TrapMatches = FindOwnedByExactLabel(World, TEXT("CorridorTraps_GowningRing"), NeuroPackage);
		if (TrapMatches.Num() != 1)
		{
			return FString::Printf(
				TEXT("CorridorTraps_GowningRing Neuro count=%d expected=1."), TrapMatches.Num());
		}
		{
			const FVector TrapLoc = TrapMatches[0]->GetActorLocation();
			const FVector TrapExtent(1600.f, 340.f, 200.f);
			const FVector SphereExtent(120.f, 120.f, 120.f);
			const FVector Delta = (NeuroResearchLoadCutoffLocation - TrapLoc).GetAbs();
			if (Delta.X <= TrapExtent.X + SphereExtent.X
				&& Delta.Y <= TrapExtent.Y + SphereExtent.Y
				&& Delta.Z <= TrapExtent.Z + SphereExtent.Z)
			{
				return TEXT("Approved cutoff location overlaps CorridorTraps_GowningRing keepout.");
			}
		}

		TArray<AActor*> GateMatches = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
		if (GateMatches.Num() != 1)
		{
			return FString::Printf(TEXT("Gate_ResearchWing count=%d expected=1 for door keepout."), GateMatches.Num());
		}
		if (FVector::Dist(GateMatches[0]->GetActorLocation(), NeuroResearchLoadCutoffLocation)
			< NeuroResearchLoadCutoffDoorKeepout)
		{
			return TEXT("Keepout violated vs Gate_ResearchWing (door).");
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other || ActorLabel(Other).Equals(NeuroResearchLoadCutoffLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroResearchLoadCutoffLocation) <= 5.f)
			{
				return FString::Printf(
					TEXT("Conflicting actor '%s' within 5uu of cutoff target."),
					*ActorLabel(Other));
			}
		}
		return TEXT("");
	}

	FString NeuroResearchLoadCutoff_KeepList(UWorld* World)
	{
		if (const FString Error = NeuroMappingArray_KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(
				World, TEXT("NeuralMappingArray_NeuroGenetics"), FVector(-500.f, -600.f, -1100.f), NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list NeuralMappingArray_NeuroGenetics: %s"), *Error);
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> NeuroResearchLoadCutoff_KeepListSnapshot(UWorld* World)
	{
		TSharedRef<FJsonObject> Snap = MakeShared<FJsonObject>();
		const TCHAR* Labels[] = {
			TEXT("PowerPanel_NeuroBackup"), TEXT("DataPad_NeuroPowerDiagnostics"), TEXT("DataPad_NeuroContainment"),
			TEXT("DataPad_NeuroResearchFailure"), TEXT("ResearchStation_NeuroGenetics"),
			TEXT("Host_Neuro_1"), TEXT("Host_Neuro_2"), TEXT("Host_Neuro_3"), TEXT("Host_Neuro_Researcher"),
			TEXT("Hazard_ScrubberLeak"), TEXT("CorridorTraps_GowningRing"), TEXT("NeuralMappingArray_NeuroGenetics"),
			TEXT("Gate_ResearchWing"), TEXT("NeuroGenetics_FloorPlate"),
		};
		TArray<TSharedPtr<FJsonValue>> Actors;
		for (const TCHAR* Label : Labels)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Label);
			Row->SetNumberField(TEXT("count"), Matches.Num());
			if (Matches.Num() == 1)
			{
				Row->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
				Row->SetStringField(TEXT("owning_package"), ActorOwningPackage(Matches[0]));
			}
			Actors.Add(MakeShared<FJsonValueObject>(Row));
		}
		Snap->SetArrayField(TEXT("actors"), Actors);
		return Snap;
	}

	TSharedRef<FJsonObject> NeuroResearchLoadCutoff_ProposedComponentState()
	{
		auto MeshObj = [](const TCHAR* Name, const FVector& Rel, const FVector& Scale)
		{
			TSharedRef<FJsonObject> M = MakeShared<FJsonObject>();
			M->SetStringField(TEXT("component"), Name);
			M->SetStringField(TEXT("mesh"), NeuroResearchLoadCutoffCubePath);
			M->SetArrayField(TEXT("relative_location"), Vec(Rel));
			M->SetArrayField(TEXT("relative_rotation"), Vec(FVector::ZeroVector));
			M->SetArrayField(TEXT("relative_scale"), Vec(Scale));
			M->SetStringField(TEXT("collision"), TEXT("NoCollision"));
			M->SetBoolField(TEXT("generate_overlap_events"), false);
			M->SetStringField(TEXT("visual_note"), TEXT("Temporary replaceable Engine Cube blockout — not final art."));
			return M;
		};
		TArray<TSharedPtr<FJsonValue>> Meshes;
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("PedestalMesh"), NeuroResearchLoadCutoffPedestalRel, NeuroResearchLoadCutoffPedestalScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ColumnMesh"), NeuroResearchLoadCutoffColumnRel, NeuroResearchLoadCutoffColumnScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ArrayHeadMesh"), NeuroResearchLoadCutoffHeadRel, NeuroResearchLoadCutoffHeadScale)));

		TSharedRef<FJsonObject> Actor = MakeShared<FJsonObject>();
		Actor->SetStringField(TEXT("label"), NeuroResearchLoadCutoffLabel);
		Actor->SetStringField(TEXT("class"), NeuroResearchLoadCutoffClassName);
		Actor->SetStringField(TEXT("persistent_package"), EpitopePackage);
		Actor->SetStringField(TEXT("destination_package"), NeuroPackage);
		Actor->SetArrayField(TEXT("location"), Vec(NeuroResearchLoadCutoffLocation));
		Actor->SetArrayField(TEXT("rotation"), Vec(FVector::ZeroVector));
		Actor->SetArrayField(TEXT("scale"), Vec(NeuroResearchLoadCutoffScale));
		Actor->SetNumberField(TEXT("interaction_range"), NeuroResearchLoadCutoffInteractionRange);
		Actor->SetStringField(TEXT("required_active_objective_id"), NeuroResearchLoadCutoffRequiredActive);
		Actor->SetStringField(TEXT("objective_event_id"), NeuroResearchLoadCutoffObjectiveEvent);
		Actor->SetStringField(TEXT("completed_objective_id_for_replay_guard"), NeuroResearchLoadCutoffReplayGuard);
		Actor->SetStringField(TEXT("inspection_prompt"), NeuroResearchLoadCutoffInspectPrompt);
		Actor->SetStringField(TEXT("review_prompt"), NeuroResearchLoadCutoffReviewPrompt);
		Actor->SetStringField(TEXT("speaker_label"), NeuroResearchLoadCutoffSpeaker);
		Actor->SetStringField(TEXT("inspection_response_text"), NeuroResearchLoadCutoffResponse);
		Actor->SetNumberField(TEXT("notification_duration_seconds"), NeuroResearchLoadCutoffNotifySeconds);
		Actor->SetBoolField(TEXT("b_has_been_inspected"), false);
		Actor->SetArrayField(TEXT("presentation_meshes"), Meshes);
		Actor->SetStringField(
			TEXT("visual_note"),
			TEXT("PedestalMesh/ColumnMesh/ArrayHeadMesh are temporary replaceable Engine Cube blockouts — not final art."));
		return Actor;
	}

	FString NeuroResearchLoadCutoff_DestroyAndRestore(AActor* Actor, UPackage* NeuroPkg, bool bNeuroWasDirtyBefore)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
		TArray<FString> Restored;
		RestorePackageCleanIfWasClean(NeuroPkg, bNeuroWasDirtyBefore, Restored);
		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Cleanup incomplete; dirty remains: %s (restored=%s)."),
				*FString::Join(Dirty, TEXT(",")),
				*FString::Join(Restored, TEXT(",")));
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroResearchLoadCutoff(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. spawn_neuro_research_load_cutoff is an Unreal Editor write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_research_load_cutoff does not save or compile.");
		}
		if (const FString OverrideError = NeuroResearchLoadCutoff_RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_research_load_cutoff preflight must run on the game thread.");
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			return CleanError;
		}
		if (!PackagesEqual(WorldPackageName(World), EpitopePackage))
		{
			return TEXT("Persistent map must be /Game/Maps/Lvl_Epitope. ZERO writes.");
		}
		if (const FString VisError = NeuroResearchLoadCutoff_RequireLevelsLoadedVisible(World); !VisError.IsEmpty())
		{
			return VisError;
		}
		if (const FString MissionError = NeuroResearchLoadCutoff_RequireBeat3MissionPersistedClean(); !MissionError.IsEmpty())
		{
			return MissionError;
		}
		if (!LoadClass<AActor>(nullptr, NeuroResearchLoadCutoffClassPath))
		{
			return TEXT("AProjectOrganoidInspectableInstrument class is not loaded.");
		}
		if (const FString KeepError = NeuroResearchLoadCutoff_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroResearchLoadCutoff_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
		{
			return PlaceError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearchLoadCutoffLabel);
		bool bAlreadyExact = false;
		if (Matches.Num() > 1)
		{
			return FString::Printf(
				TEXT("%s count=%d. Abort rather than stack."), NeuroResearchLoadCutoffLabel, Matches.Num());
		}
		if (Matches.Num() == 1)
		{
			const FString Mismatch = NeuroResearchLoadCutoff_ActorMismatch(Matches[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("%s exists but mismatches: %s. Fail closed — no opportunistic repair."),
					NeuroResearchLoadCutoffLabel,
					*Mismatch);
			}
			bAlreadyExact = true;
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroResearchLoadCutoffSpec);
		Before->SetStringField(TEXT("action"), NeuroResearchLoadCutoffAction);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("mission_beat3_ready"), true);
		Before->SetStringField(TEXT("mission_object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetObjectField(TEXT("keep_list"), NeuroResearchLoadCutoff_KeepListSnapshot(World));
		Before->SetObjectField(TEXT("placement_margins"), Margins);
		if (Matches.Num() == 1)
		{
			Before->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
		}

		Proposed->SetStringField(TEXT("spec"), NeuroResearchLoadCutoffSpec);
		Proposed->SetObjectField(TEXT("actor"), NeuroResearchLoadCutoff_ProposedComponentState());
		Proposed->SetObjectField(TEXT("placement_margins"), Margins);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("invoke_interact"), false);
		Proposed->SetBoolField(TEXT("fire_objective_event"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyExact
				? TEXT("EmergencyCutoff_NeuroResearchLoad already matches neuro_research_load_cutoff_v1. Execute is a clean no-op. Requires all packages clean. Temporary Cube blockout — not final art.")
				: TEXT("Spawn one temporary Cube-blockout EmergencyCutoff_NeuroResearchLoad (InspectableInstrument) on Neuro at C1. Requires persisted clean Beat 3 DA_Mission_NeuroGenetics. Does not interact, fire events, change power/Hosts/pads, save, or compile. Not final art."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroResearchLoadCutoff(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("spawn_neuro_research_load_cutoff must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroResearchLoadCutoff(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("packages_dirty"),
				FString::Printf(TEXT("ZERO writes. %s"), *CleanError),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString MissionError = NeuroResearchLoadCutoff_RequireBeat3MissionPersistedClean(); !MissionError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mission_required"),
				FString::Printf(TEXT("ZERO writes. %s"), *MissionError),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroResearchLoadCutoff_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("keep_list"),
				FString::Printf(TEXT("ZERO writes. %s"), *KeepError),
				MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroResearchLoadCutoff_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("placement"),
				FString::Printf(TEXT("ZERO writes. %s"), *PlaceError),
				MakeShared<FBridgeChange>(Change));
		}

		ULevel* TargetLevel = FindLoadedLevelByPackage(World, NeuroPackage);
		if (!TargetLevel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
		const bool bNeuroWasDirtyBefore = NeuroPkg && NeuroPkg->IsDirty();

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearchLoadCutoffLabel);
		if (Matches.Num() == 1 && NeuroResearchLoadCutoff_ActorMismatch(Matches[0]).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact actor path found dirty packages: %s. Hard stop. Do not save."),
						*FString::Join(DirtyAfter, TEXT(","))),
					MakeShared<FBridgeChange>(Change));
			}
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), NeuroResearchLoadCutoffLabel);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
			Change.After->SetObjectField(TEXT("proposed"), NeuroResearchLoadCutoff_ProposedComponentState());
			Change.After->SetObjectField(TEXT("placement_margins"), Margins);
			Change.After->SetStringField(
				TEXT("visual_note"),
				TEXT("Temporary Engine Cube blockout — replaceable presentation, not final art."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Matches.Num() != 0)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("EmergencyCutoff_NeuroResearchLoad exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UClass* InstrumentClass = LoadClass<AActor>(nullptr, NeuroResearchLoadCutoffClassPath);
		if (!InstrumentClass)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("class_missing"),
				TEXT("AProjectOrganoidInspectableInstrument vanished. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		AActor* Spawned = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SpawnNeuroResearchLoadCutoff",
				"Spawn Neuro Research Load Cutoff (temporary blockout)"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Spawned = World->SpawnActor<AActor>(
				InstrumentClass, NeuroResearchLoadCutoffLocation, NeuroResearchLoadCutoffRotation, Params);
			if (!Spawned)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("spawn_failed"),
					TEXT("SpawnActor returned null. ZERO remaining writes."),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(NeuroResearchLoadCutoffLabel, true);
			Spawned->SetActorScale3D(NeuroResearchLoadCutoffScale);
			if (const FString ApplyError = NeuroResearchLoadCutoff_ApplyFields(Spawned); !ApplyError.IsEmpty())
			{
				const FString CleanupError =
					NeuroResearchLoadCutoff_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
				Spawned = nullptr;
				Change.Status = TEXT("execute_failed_rolled_back");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				Change.After->SetBoolField(TEXT("cleanup_ok"), CleanupError.IsEmpty());
				return FailAudit(
					TEXT("configure_failed"),
					FString::Printf(
						TEXT("%s. Spawned actor destroyed. %s Do not save."),
						*ApplyError,
						CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->MarkPackageDirty();
			if (const FString AfterError = NeuroResearchLoadCutoff_ActorMismatch(Spawned); !AfterError.IsEmpty())
			{
				const FString CleanupError =
					NeuroResearchLoadCutoff_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
				Spawned = nullptr;
				Change.Status = TEXT("execute_failed_rolled_back");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				Change.After->SetBoolField(TEXT("cleanup_ok"), CleanupError.IsEmpty());
				return FailAudit(
					TEXT("postcondition_failed"),
					FString::Printf(
						TEXT("%s. Spawned actor destroyed. %s Do not save."),
						*AfterError,
						CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString KeepError = NeuroResearchLoadCutoff_KeepList(World); !KeepError.IsEmpty())
		{
			const FString CleanupError =
				NeuroResearchLoadCutoff_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
			Spawned = nullptr;
			Change.Status = TEXT("execute_failed_rolled_back");
			return FailAudit(
				TEXT("keep_list_failed"),
				FString::Printf(
					TEXT("%s. Spawned actor destroyed. %s Do not save."),
					*KeepError,
					CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		TArray<FString> ExpectedDirty;
		ExpectedDirty.Add(NormalizePackage(NeuroPackage));
		if (DirtyAfter != ExpectedDirty)
		{
			const FString CleanupError =
				NeuroResearchLoadCutoff_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
			Spawned = nullptr;
			Change.Status = TEXT("execute_failed_rolled_back");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetBoolField(TEXT("cleanup_ok"), CleanupError.IsEmpty());
			return FailAudit(
				TEXT("unexpected_dirt"),
				FString::Printf(
					TEXT("Expected only Neuro dirty. Live dirty=[%s]. Spawned actor destroyed. %s Do not save."),
					*FString::Join(DirtyAfter, TEXT(",")),
					CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("spawned"));
		Change.After->SetStringField(TEXT("label"), NeuroResearchLoadCutoffLabel);
		Change.After->SetStringField(TEXT("class"), ClassName(Spawned));
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
		Change.After->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("interacted"), false);
		Change.After->SetBoolField(TEXT("objective_event_fired"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Spawned));
		Change.After->SetObjectField(TEXT("proposed"), NeuroResearchLoadCutoff_ProposedComponentState());
		Change.After->SetObjectField(TEXT("placement_margins"), Margins);
		Change.After->SetStringField(
			TEXT("visual_note"),
			TEXT("Temporary Engine Cube blockout — PedestalMesh/ColumnMesh/ArrayHeadMesh replaceable, not final art."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
