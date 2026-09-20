	// Fixed NeuroGenetics neural-mapping terminal spawn —
	// spawn_neuro_neural_mapping_terminal / neuro_neural_mapping_terminal_v1.
	// Temporary Engine Cube blockout presentation — replaceable, not final art.
	// Requires persisted clean Beat 4 DA_Mission_NeuroGenetics (rejects Beat 3).
	// Reuses Cutoff streaming/nav-bounds metadata classifiers; does not modify Beat 1–4 actions.
	const TCHAR* NeuroMappingTerminalSpec = TEXT("neuro_neural_mapping_terminal_v1");
	const TCHAR* NeuroMappingTerminalAction = TEXT("spawn_neuro_neural_mapping_terminal");
	const TCHAR* NeuroMappingTerminalLabel = TEXT("NeuralMappingTerminal_NeuroGenetics");
	const TCHAR* NeuroMappingTerminalClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroMappingTerminalClassName = TEXT("ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroMappingTerminalCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroMappingTerminalRequiredActive = TEXT("Obj_TraceNeuralMappingSignal");
	const TCHAR* NeuroMappingTerminalObjectiveEvent = TEXT("Event_NeuralMappingSignalTraced");
	const TCHAR* NeuroMappingTerminalReplayGuard = TEXT("Obj_TraceNeuralMappingSignal");
	const TCHAR* NeuroMappingTerminalInspectPrompt = TEXT("Trace Neural Mapping Signal");
	const TCHAR* NeuroMappingTerminalReviewPrompt = TEXT("Signal Trace Complete");
	const TCHAR* NeuroMappingTerminalSpeaker = TEXT("Nathan");
	// Curly apostrophe as ASCII \u2019 escape (victims' / Something's).
	const TCHAR* NeuroMappingTerminalResponse =
		TEXT("These scans line up with the victims\u2019 neural changes. Something\u2019s been tracking the same pattern across all of them.");
	const FVector NeuroMappingTerminalLocation(300.f, -600.f, -1100.f);
	const FRotator NeuroMappingTerminalRotation = FRotator::ZeroRotator;
	const FVector NeuroMappingTerminalScale = FVector::OneVector;
	constexpr float NeuroMappingTerminalInteractionRange = 175.f;
	constexpr float NeuroMappingTerminalNotifySeconds = 4.f;
	const FVector NeuroMappingTerminalPedestalRel(0.f, 0.f, 30.f);
	const FVector NeuroMappingTerminalPedestalScale(0.55f, 0.45f, 0.60f);
	const FVector NeuroMappingTerminalColumnRel(0.f, 0.f, 90.f);
	const FVector NeuroMappingTerminalColumnScale(0.50f, 0.25f, 0.60f);
	const FVector NeuroMappingTerminalHeadRel(0.f, 0.f, 145.f);
	const FVector NeuroMappingTerminalHeadScale(0.75f, 0.35f, 0.25f);
	constexpr float NeuroMappingTerminalHostKeepout = 400.f;
	constexpr float NeuroMappingTerminalDoorKeepout = 400.f;
	constexpr float NeuroMappingTerminalHazardKeepout = 400.f;
	constexpr float NeuroMappingTerminalFloorZTarget = -1100.f;
	constexpr float NeuroMappingTerminalFloorZEps = 150.f;
	constexpr float NeuroMappingTerminalNormalUpMin = 0.7f;
	constexpr float NeuroMappingTerminalCutoffInteractionRange = 175.f;
	constexpr float NeuroMappingTerminalArrayInteractionRange = 200.f;
	constexpr float NeuroMappingTerminalCutoffMarginMin = 50.f;
	constexpr float NeuroMappingTerminalArrayMarginMin = 425.f;
	constexpr float NeuroMappingTerminalCubeHalf = 50.f;
	const FVector NeuroMappingTerminalCutoffExpectedLoc(-100.f, -600.f, -1100.f);
	const FVector NeuroMappingTerminalArrayExpectedLoc(-500.f, -600.f, -1100.f);

	UStaticMeshComponent* NeuroMappingTerminal_FindMeshComponent(AActor* Actor, const TCHAR* PropertyName)
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

	FString NeuroMappingTerminal_RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
					TEXT("Arbitrary argument '%s' is refused. spawn_neuro_neural_mapping_terminal uses fixed native constants only."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroMappingTerminalSpec);
		if (!Spec.Equals(NeuroMappingTerminalSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_neural_mapping_terminal_v1.");
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_RequireBeat4MissionPersistedClean()
	{
		UObject* Mission = FindNeuroGeneticsMissionAssetExact();
		if (!Mission)
		{
			return TEXT(
				"DA_Mission_NeuroGenetics is required and must already exist (persisted, clean) matching neurogenetics_mission_beat4_v1. ZERO writes.");
		}
		if (const FString Beat4Mismatch = NeuroGeneticsMissionBeat4MismatchReason(Mission); !Beat4Mismatch.IsEmpty())
		{
			if (NeuroGeneticsMissionBeat3MismatchReason(Mission).IsEmpty())
			{
				return TEXT(
					"DA_Mission_NeuroGenetics is still exact Beat 3. Require persisted clean Beat 4 (expand_neurogenetics_mission_beat4 + save) before spawn_neuro_neural_mapping_terminal. ZERO writes.");
			}
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 4: %s. Fail closed — no opportunistic repair."),
				*Beat4Mismatch);
		}
		if (UPackage* MissionPkg = Mission->GetOutermost())
		{
			if (MissionPkg->IsDirty())
			{
				return TEXT("DA_Mission_NeuroGenetics package is dirty. Require persisted clean Beat 4 mission asset. ZERO writes.");
			}
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_RequireLevelsLoadedVisible(UWorld* World)
	{
		return NeuroMappingArray_RequireLevelsLoadedVisible(World);
	}

	FBox NeuroMappingTerminal_ExpectedMeshWorldBox(const FVector& RelLoc, const FVector& RelScale)
	{
		const FVector Half = FVector(
			NeuroMappingTerminalCubeHalf * RelScale.X,
			NeuroMappingTerminalCubeHalf * RelScale.Y,
			NeuroMappingTerminalCubeHalf * RelScale.Z);
		const FVector Center = NeuroMappingTerminalLocation + RelLoc;
		return FBox(Center - Half, Center + Half);
	}

	FString NeuroMappingTerminal_MeshPresentationMismatch(
		AActor* Actor,
		const TCHAR* ComponentName,
		const FVector& RelLocation,
		const FVector& RelScale)
	{
		return NeuroMappingArray_MeshPresentationMismatch(
			Actor, ComponentName, TEXT("/Engine/BasicShapes/Cube"), RelLocation, RelScale);
	}

	FString NeuroMappingTerminal_ActorMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ClassName(Actor).Equals(NeuroMappingTerminalClassName, ESearchCase::CaseSensitive)
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
		if (!ActorLabel(Actor).Equals(NeuroMappingTerminalLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' expected NeuralMappingTerminal_NeuroGenetics"), *ActorLabel(Actor));
		}
		if (const FString Xform = TransformMismatch(
				Actor, NeuroMappingTerminalLocation, NeuroMappingTerminalRotation, NeuroMappingTerminalScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("InteractionRange"), NeuroMappingTerminalInteractionRange);
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
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroMappingTerminalRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ObjectiveEventId"), NeuroMappingTerminalObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroMappingTerminalReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionPrompt"), NeuroMappingTerminalInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ReviewPrompt"), NeuroMappingTerminalReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("SpeakerLabel"), NeuroMappingTerminalSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionResponseText"), NeuroMappingTerminalResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("NotificationDurationSeconds"), NeuroMappingTerminalNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingTerminal_MeshPresentationMismatch(
				Actor, TEXT("PedestalMesh"), NeuroMappingTerminalPedestalRel, NeuroMappingTerminalPedestalScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingTerminal_MeshPresentationMismatch(
				Actor, TEXT("ColumnMesh"), NeuroMappingTerminalColumnRel, NeuroMappingTerminalColumnScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingTerminal_MeshPresentationMismatch(
				Actor, TEXT("ArrayHeadMesh"), NeuroMappingTerminalHeadRel, NeuroMappingTerminalHeadScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_ApplyFields(AActor* Actor)
	{
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("InteractionRange"), NeuroMappingTerminalInteractionRange);
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
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroMappingTerminalRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("ObjectiveEventId"), NeuroMappingTerminalObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroMappingTerminalReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionPrompt"), NeuroMappingTerminalInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("ReviewPrompt"), NeuroMappingTerminalReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_SetTextProperty(Actor, TEXT("SpeakerLabel"), NeuroMappingTerminalSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionResponseText"), NeuroMappingTerminalResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("NotificationDurationSeconds"), NeuroMappingTerminalNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InteractionPrompt"), NeuroMappingTerminalInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}

		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingTerminal_FindMeshComponent(Actor, TEXT("PedestalMesh")),
				NeuroMappingTerminalCubePath,
				NeuroMappingTerminalPedestalRel,
				NeuroMappingTerminalPedestalScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("PedestalMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingTerminal_FindMeshComponent(Actor, TEXT("ColumnMesh")),
				NeuroMappingTerminalCubePath,
				NeuroMappingTerminalColumnRel,
				NeuroMappingTerminalColumnScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ColumnMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingTerminal_FindMeshComponent(Actor, TEXT("ArrayHeadMesh")),
				NeuroMappingTerminalCubePath,
				NeuroMappingTerminalHeadRel,
				NeuroMappingTerminalHeadScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ArrayHeadMesh (temporary Cube blockout): %s"), *Error);
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_FloorTraceAt(UWorld* World, const FVector& SampleXY)
	{
		const FVector Start(SampleXY.X, SampleXY.Y, NeuroMappingTerminalLocation.Z + 80.f);
		const FVector End(SampleXY.X, SampleXY.Y, NeuroMappingTerminalLocation.Z - 250.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuroMappingTerminalFloor), false);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
		if (!bHit || !Hit.bBlockingHit)
		{
			return FString::Printf(
				TEXT("Downward floor trace missed at (%.0f,%.0f). Unsupported placement."),
				SampleXY.X,
				SampleXY.Y);
		}
		if (Hit.ImpactNormal.Z < NeuroMappingTerminalNormalUpMin)
		{
			return FString::Printf(
				TEXT("Floor normal Z=%.3f at (%.0f,%.0f) is not upward-facing."),
				Hit.ImpactNormal.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		if (FMath::Abs(Hit.ImpactPoint.Z - NeuroMappingTerminalFloorZTarget) > NeuroMappingTerminalFloorZEps
			&& FMath::Abs(Hit.ImpactPoint.Z - (-1200.f)) > NeuroMappingTerminalFloorZEps)
		{
			return FString::Printf(
				TEXT("Supported surface Z=%.1f at (%.0f,%.0f) is not near -1100."),
				Hit.ImpactPoint.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_NavProjection(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSystem)
		{
			return TEXT("Neuro navigation system unavailable. ZERO writes.");
		}
		const FVector Samples[] = {
			NeuroMappingTerminalLocation,
			NeuroMappingTerminalLocation + FVector(40.f, 0.f, 0.f),
			NeuroMappingTerminalLocation + FVector(-40.f, 0.f, 0.f),
			NeuroMappingTerminalLocation + FVector(0.f, 40.f, 0.f),
			NeuroMappingTerminalLocation + FVector(0.f, -40.f, 0.f),
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

	FString NeuroMappingTerminal_MeshBoundsClearance(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FBox Boxes[] = {
			NeuroMappingTerminal_ExpectedMeshWorldBox(
				NeuroMappingTerminalPedestalRel, NeuroMappingTerminalPedestalScale),
			NeuroMappingTerminal_ExpectedMeshWorldBox(
				NeuroMappingTerminalColumnRel, NeuroMappingTerminalColumnScale),
			NeuroMappingTerminal_ExpectedMeshWorldBox(
				NeuroMappingTerminalHeadRel, NeuroMappingTerminalHeadScale),
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
			if (Label.Equals(NeuroMappingTerminalLabel, ESearchCase::CaseSensitive)
				|| Label.Equals(TEXT("NeuroGenetics_FloorPlate"), ESearchCase::CaseSensitive)
				|| Label.Contains(TEXT("FloorPlate")))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroMappingTerminalLocation) > 800.f)
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

	FString NeuroMappingTerminal_PlacementGeometry(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FVector FootSamples[] = {
			NeuroMappingTerminalLocation,
			NeuroMappingTerminalLocation + FVector(40.f, 40.f, 0.f),
			NeuroMappingTerminalLocation + FVector(40.f, -40.f, 0.f),
			NeuroMappingTerminalLocation + FVector(-40.f, 40.f, 0.f),
			NeuroMappingTerminalLocation + FVector(-40.f, -40.f, 0.f),
		};
		for (const FVector& Sample : FootSamples)
		{
			if (const FString TraceError = NeuroMappingTerminal_FloorTraceAt(World, Sample); !TraceError.IsEmpty())
			{
				return TraceError;
			}
		}

		if (const FString BoundsError = NeuroMappingTerminal_MeshBoundsClearance(World, OutMargins); !BoundsError.IsEmpty())
		{
			return BoundsError;
		}
		if (const FString NavError = NeuroMappingTerminal_NavProjection(World, OutMargins); !NavError.IsEmpty())
		{
			return NavError;
		}

		// Cutoff interaction clearance: dist 400 must exceed 175+175 with margin 50.
		{
			TArray<AActor*> CutoffMatches =
				FindOwnedByExactLabel(World, TEXT("EmergencyCutoff_NeuroResearchLoad"), NeuroPackage);
			if (CutoffMatches.Num() != 1)
			{
				return FString::Printf(
					TEXT("EmergencyCutoff_NeuroResearchLoad Neuro count=%d expected=1 for interaction clearance."),
					CutoffMatches.Num());
			}
			const float Dist = FVector::Dist(CutoffMatches[0]->GetActorLocation(), NeuroMappingTerminalLocation);
			const float Combined =
				NeuroMappingTerminalInteractionRange + NeuroMappingTerminalCutoffInteractionRange;
			const float Margin = Dist - Combined;
			OutMargins->SetNumberField(TEXT("cutoff_distance"), Dist);
			OutMargins->SetNumberField(TEXT("cutoff_combined_radii"), Combined);
			OutMargins->SetNumberField(TEXT("cutoff_clearance_margin"), Margin);
			OutMargins->SetNumberField(TEXT("cutoff_required_margin"), NeuroMappingTerminalCutoffMarginMin);
			if (Margin < NeuroMappingTerminalCutoffMarginMin)
			{
				return FString::Printf(
					TEXT("Interaction space vs EmergencyCutoff_NeuroResearchLoad insufficient (margin %.1f < required %.0f; dist %.1f, combined %.0f). Fail closed — no C1 fallback."),
					Margin,
					NeuroMappingTerminalCutoffMarginMin,
					Dist,
					Combined);
			}
		}

		// Array interaction clearance: dist 800 must exceed 175+200 with margin 425.
		{
			TArray<AActor*> ArrayMatches =
				FindOwnedByExactLabel(World, TEXT("NeuralMappingArray_NeuroGenetics"), NeuroPackage);
			if (ArrayMatches.Num() != 1)
			{
				return FString::Printf(
					TEXT("NeuralMappingArray_NeuroGenetics Neuro count=%d expected=1 for interaction clearance."),
					ArrayMatches.Num());
			}
			const float Dist = FVector::Dist(ArrayMatches[0]->GetActorLocation(), NeuroMappingTerminalLocation);
			const float Combined =
				NeuroMappingTerminalInteractionRange + NeuroMappingTerminalArrayInteractionRange;
			const float Margin = Dist - Combined;
			OutMargins->SetNumberField(TEXT("array_distance"), Dist);
			OutMargins->SetNumberField(TEXT("array_combined_radii"), Combined);
			OutMargins->SetNumberField(TEXT("array_clearance_margin"), Margin);
			OutMargins->SetNumberField(TEXT("array_required_margin"), NeuroMappingTerminalArrayMarginMin);
			if (Margin < NeuroMappingTerminalArrayMarginMin)
			{
				return FString::Printf(
					TEXT("Interaction space vs NeuralMappingArray_NeuroGenetics insufficient (margin %.1f < required %.0f; dist %.1f, combined %.0f). Fail closed — no C1 fallback."),
					Margin,
					NeuroMappingTerminalArrayMarginMin,
					Dist,
					Combined);
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
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroMappingTerminalLocation);
			const float Combined = NeuroMappingTerminalInteractionRange + Entry.Range;
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
			{TEXT("Host_Neuro_1"), NeuroMappingTerminalHostKeepout},
			{TEXT("Host_Neuro_2"), NeuroMappingTerminalHostKeepout},
			{TEXT("Host_Neuro_3"), NeuroMappingTerminalHostKeepout},
			{TEXT("Host_Neuro_Researcher"), NeuroMappingTerminalHostKeepout},
			{TEXT("Hazard_ScrubberLeak"), NeuroMappingTerminalHazardKeepout},
		};
		TArray<TSharedPtr<FJsonValue>> KeepoutRows;
		for (const FKeepout& Entry : HostsAndHazard)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1 for keepout."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroMappingTerminalLocation);
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
			const FVector Delta = (NeuroMappingTerminalLocation - TrapLoc).GetAbs();
			if (Delta.X <= TrapExtent.X + SphereExtent.X
				&& Delta.Y <= TrapExtent.Y + SphereExtent.Y
				&& Delta.Z <= TrapExtent.Z + SphereExtent.Z)
			{
				return TEXT("Approved terminal location overlaps CorridorTraps_GowningRing keepout. Fail closed — no C1 fallback.");
			}
		}

		TArray<AActor*> GateMatches = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
		if (GateMatches.Num() != 1)
		{
			return FString::Printf(TEXT("Gate_ResearchWing count=%d expected=1 for door keepout."), GateMatches.Num());
		}
		if (FVector::Dist(GateMatches[0]->GetActorLocation(), NeuroMappingTerminalLocation)
			< NeuroMappingTerminalDoorKeepout)
		{
			return TEXT("Keepout violated vs Gate_ResearchWing (door). Fail closed — no C1 fallback.");
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other || ActorLabel(Other).Equals(NeuroMappingTerminalLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroMappingTerminalLocation) <= 5.f)
			{
				return FString::Printf(
					TEXT("Conflicting actor '%s' within 5uu of terminal C1. Fail closed — no fallback."),
					*ActorLabel(Other));
			}
		}
		return TEXT("");
	}

	FString NeuroMappingTerminal_KeepList(UWorld* World)
	{
		if (const FString Error = NeuroResearchLoadCutoff_KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(
				World,
				TEXT("EmergencyCutoff_NeuroResearchLoad"),
				NeuroMappingTerminalCutoffExpectedLoc,
				NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list EmergencyCutoff_NeuroResearchLoad: %s"), *Error);
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> NeuroMappingTerminal_KeepListSnapshot(UWorld* World)
	{
		TSharedRef<FJsonObject> Snap = MakeShared<FJsonObject>();
		const TCHAR* Labels[] = {
			TEXT("PowerPanel_NeuroBackup"), TEXT("DataPad_NeuroPowerDiagnostics"), TEXT("DataPad_NeuroContainment"),
			TEXT("DataPad_NeuroResearchFailure"), TEXT("ResearchStation_NeuroGenetics"),
			TEXT("Host_Neuro_1"), TEXT("Host_Neuro_2"), TEXT("Host_Neuro_3"), TEXT("Host_Neuro_Researcher"),
			TEXT("Hazard_ScrubberLeak"), TEXT("CorridorTraps_GowningRing"), TEXT("NeuralMappingArray_NeuroGenetics"),
			TEXT("EmergencyCutoff_NeuroResearchLoad"), TEXT("Gate_ResearchWing"), TEXT("NeuroGenetics_FloorPlate"),
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

	TSharedRef<FJsonObject> NeuroMappingTerminal_ProposedComponentState()
	{
		auto MeshObj = [](const TCHAR* Name, const FVector& Rel, const FVector& Scale)
		{
			TSharedRef<FJsonObject> M = MakeShared<FJsonObject>();
			M->SetStringField(TEXT("component"), Name);
			M->SetStringField(TEXT("mesh"), NeuroMappingTerminalCubePath);
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
			TEXT("PedestalMesh"), NeuroMappingTerminalPedestalRel, NeuroMappingTerminalPedestalScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ColumnMesh"), NeuroMappingTerminalColumnRel, NeuroMappingTerminalColumnScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ArrayHeadMesh"), NeuroMappingTerminalHeadRel, NeuroMappingTerminalHeadScale)));

		TSharedRef<FJsonObject> Actor = MakeShared<FJsonObject>();
		Actor->SetStringField(TEXT("label"), NeuroMappingTerminalLabel);
		Actor->SetStringField(TEXT("class"), NeuroMappingTerminalClassName);
		Actor->SetStringField(TEXT("persistent_package"), EpitopePackage);
		Actor->SetStringField(TEXT("destination_package"), NeuroPackage);
		Actor->SetArrayField(TEXT("location"), Vec(NeuroMappingTerminalLocation));
		Actor->SetArrayField(TEXT("rotation"), Vec(FVector::ZeroVector));
		Actor->SetArrayField(TEXT("scale"), Vec(NeuroMappingTerminalScale));
		Actor->SetNumberField(TEXT("interaction_range"), NeuroMappingTerminalInteractionRange);
		Actor->SetStringField(TEXT("required_active_objective_id"), NeuroMappingTerminalRequiredActive);
		Actor->SetStringField(TEXT("objective_event_id"), NeuroMappingTerminalObjectiveEvent);
		Actor->SetStringField(TEXT("completed_objective_id_for_replay_guard"), NeuroMappingTerminalReplayGuard);
		Actor->SetStringField(TEXT("inspection_prompt"), NeuroMappingTerminalInspectPrompt);
		Actor->SetStringField(TEXT("review_prompt"), NeuroMappingTerminalReviewPrompt);
		Actor->SetStringField(TEXT("speaker_label"), NeuroMappingTerminalSpeaker);
		Actor->SetStringField(TEXT("inspection_response_text"), NeuroMappingTerminalResponse);
		Actor->SetNumberField(TEXT("notification_duration_seconds"), NeuroMappingTerminalNotifySeconds);
		Actor->SetBoolField(TEXT("b_has_been_inspected"), false);
		Actor->SetArrayField(TEXT("presentation_meshes"), Meshes);
		Actor->SetStringField(
			TEXT("visual_note"),
			TEXT("PedestalMesh/ColumnMesh/ArrayHeadMesh are temporary replaceable Engine Cube blockouts — not final art."));
		return Actor;
	}

	FString NeuroMappingTerminal_DestroyAndRestore(AActor* Actor, UPackage* NeuroPkg, bool bNeuroWasDirtyBefore)
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

	FString PreflightSpawnNeuroNeuralMappingTerminal(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. spawn_neuro_neural_mapping_terminal is an Unreal Editor write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_neural_mapping_terminal does not save or compile.");
		}
		if (const FString OverrideError = NeuroMappingTerminal_RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_neural_mapping_terminal preflight must run on the game thread.");
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
		if (const FString VisError = NeuroMappingTerminal_RequireLevelsLoadedVisible(World); !VisError.IsEmpty())
		{
			return VisError;
		}
		if (const FString MissionError = NeuroMappingTerminal_RequireBeat4MissionPersistedClean(); !MissionError.IsEmpty())
		{
			return MissionError;
		}
		if (!LoadClass<AActor>(nullptr, NeuroMappingTerminalClassPath))
		{
			return TEXT("AProjectOrganoidInspectableInstrument class is not loaded.");
		}
		if (const FString KeepError = NeuroMappingTerminal_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroMappingTerminal_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
		{
			return PlaceError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroMappingTerminalLabel);
		bool bAlreadyExact = false;
		if (Matches.Num() > 1)
		{
			return FString::Printf(
				TEXT("%s count=%d. Abort rather than stack."), NeuroMappingTerminalLabel, Matches.Num());
		}
		if (Matches.Num() == 1)
		{
			const FString Mismatch = NeuroMappingTerminal_ActorMismatch(Matches[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("%s exists but mismatches: %s. Fail closed — no opportunistic repair."),
					NeuroMappingTerminalLabel,
					*Mismatch);
			}
			bAlreadyExact = true;
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroMappingTerminalSpec);
		Before->SetStringField(TEXT("action"), NeuroMappingTerminalAction);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("mission_beat4_ready"), true);
		Before->SetStringField(TEXT("mission_object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetObjectField(TEXT("keep_list"), NeuroMappingTerminal_KeepListSnapshot(World));
		Before->SetObjectField(TEXT("placement_margins"), Margins);
		if (Matches.Num() == 1)
		{
			Before->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
		}

		Proposed->SetStringField(TEXT("spec"), NeuroMappingTerminalSpec);
		Proposed->SetObjectField(TEXT("actor"), NeuroMappingTerminal_ProposedComponentState());
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
				? TEXT("NeuralMappingTerminal_NeuroGenetics already matches neuro_neural_mapping_terminal_v1. Execute is a clean no-op. Requires all packages clean. Temporary Cube blockout — not final art.")
				: TEXT("Spawn one temporary Cube-blockout NeuralMappingTerminal_NeuroGenetics (InspectableInstrument) on Neuro at C1 (300,-600,-1100). Requires persisted clean Beat 4 DA_Mission_NeuroGenetics. Does not interact, fire events, change power/Hosts/pads, save, or compile. Not final art."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroNeuralMappingTerminal(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("spawn_neuro_neural_mapping_terminal must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroNeuralMappingTerminal(Change.Args, Before, Proposed);
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
		if (const FString MissionError = NeuroMappingTerminal_RequireBeat4MissionPersistedClean(); !MissionError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mission_required"),
				FString::Printf(TEXT("ZERO writes. %s"), *MissionError),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroMappingTerminal_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("keep_list"),
				FString::Printf(TEXT("ZERO writes. %s"), *KeepError),
				MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroMappingTerminal_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
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

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroMappingTerminalLabel);
		if (Matches.Num() == 1 && NeuroMappingTerminal_ActorMismatch(Matches[0]).IsEmpty())
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
			Change.After->SetStringField(TEXT("label"), NeuroMappingTerminalLabel);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
			Change.After->SetObjectField(TEXT("proposed"), NeuroMappingTerminal_ProposedComponentState());
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
				TEXT("NeuralMappingTerminal_NeuroGenetics exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UClass* InstrumentClass = LoadClass<AActor>(nullptr, NeuroMappingTerminalClassPath);
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
				"SpawnNeuroNeuralMappingTerminal",
				"Spawn Neuro Neural Mapping Terminal (temporary blockout)"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Spawned = World->SpawnActor<AActor>(
				InstrumentClass, NeuroMappingTerminalLocation, NeuroMappingTerminalRotation, Params);
			if (!Spawned)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("spawn_failed"),
					TEXT("SpawnActor returned null. ZERO remaining writes."),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(NeuroMappingTerminalLabel, true);
			Spawned->SetActorScale3D(NeuroMappingTerminalScale);
			if (const FString ApplyError = NeuroMappingTerminal_ApplyFields(Spawned); !ApplyError.IsEmpty())
			{
				const FString CleanupError =
					NeuroMappingTerminal_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
			if (const FString AfterError = NeuroMappingTerminal_ActorMismatch(Spawned); !AfterError.IsEmpty())
			{
				const FString CleanupError =
					NeuroMappingTerminal_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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

		if (const FString KeepError = NeuroMappingTerminal_KeepList(World); !KeepError.IsEmpty())
		{
			const FString CleanupError =
				NeuroMappingTerminal_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
				NeuroMappingTerminal_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
		Change.After->SetStringField(TEXT("label"), NeuroMappingTerminalLabel);
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
		Change.After->SetObjectField(TEXT("proposed"), NeuroMappingTerminal_ProposedComponentState());
		Change.After->SetObjectField(TEXT("placement_margins"), Margins);
		Change.After->SetStringField(
			TEXT("visual_note"),
			TEXT("Temporary Engine Cube blockout — PedestalMesh/ColumnMesh/ArrayHeadMesh replaceable, not final art."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}