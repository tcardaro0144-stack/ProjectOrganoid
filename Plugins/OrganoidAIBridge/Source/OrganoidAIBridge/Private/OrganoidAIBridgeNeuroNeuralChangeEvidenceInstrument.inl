	// Fixed NeuroGenetics neural-change evidence instrument spawn —
	// spawn_neuro_neural_change_evidence_instrument / neuro_neural_change_evidence_instrument_v1.
	// Temporary Engine Cube/Cylinder blockout presentation — replaceable, not final art.
	// Requires persisted clean Beat 6 DA_Mission_NeuroGenetics (rejects Beat 5).
	// Reuses observation-node placement classifiers; treats NeuralSignatureObservationNode as peer keepout.
	const TCHAR* NeuroChangeEvSpec = TEXT("neuro_neural_change_evidence_instrument_v1");
	const TCHAR* NeuroChangeEvAction = TEXT("spawn_neuro_neural_change_evidence_instrument");
	const TCHAR* NeuroChangeEvLabel = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	const TCHAR* NeuroChangeEvClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroChangeEvClassName = TEXT("ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroChangeEvCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroChangeEvCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* NeuroChangeEvRequiredActive = TEXT("Obj_ExamineNeuralChangeEvidence");
	const TCHAR* NeuroChangeEvObjectiveEvent = TEXT("Event_NeuralChangeEvidenceExamined");
	const TCHAR* NeuroChangeEvReplayGuard = TEXT("Obj_ExamineNeuralChangeEvidence");
	const TCHAR* NeuroChangeEvInspectPrompt = TEXT("Examine neural-change evidence");
	const TCHAR* NeuroChangeEvReviewPrompt = TEXT("Review neural-change evidence");
	const TCHAR* NeuroChangeEvSpeaker = TEXT("Nathan");
	// InspectionResponseText uses exactly one U+2019 curly apostrophe escape.
	const TCHAR* NeuroChangeEvResponse =
		TEXT("These patterns match across multiple subjects. Epitope wasn\u2019t documenting isolated changes; they were tracking the same neural adaptation.");
	// Live spatial survey (2026-09-22): (500,-2450,-1100) fails peer margin vs observation (0 < 50).
	// Chosen (500,-2520,-1100): floor NeuroGenetics_FloorPlate, approach clear, peer margin 70, wall ~636.
	// Yaw 90 faces +Y (north) toward approach from NeuralSignatureObservationNode.
	const FVector NeuroChangeEvLocation(500.f, -2520.f, -1100.f);
	const FRotator NeuroChangeEvRotation(0.f, 90.f, 0.f);
	const FVector NeuroChangeEvScale = FVector::OneVector;
	constexpr float NeuroChangeEvInteractionRange = 175.f;
	constexpr float NeuroChangeEvNotifySeconds = 8.f;
	const FVector NeuroChangeEvPedestalRel(0.f, 0.f, 30.f);
	const FVector NeuroChangeEvPedestalScale(0.60f, 0.50f, 0.60f);
	const FVector NeuroChangeEvColumnRel(0.f, 0.f, 95.f);
	const FVector NeuroChangeEvColumnScale(0.30f, 0.30f, 1.00f);
	const FVector NeuroChangeEvHeadRel(0.f, 0.f, 155.f);
	const FVector NeuroChangeEvHeadScale(0.90f, 0.45f, 0.25f);
	constexpr float NeuroChangeEvHostKeepout = 400.f;
	constexpr float NeuroChangeEvDoorKeepout = 400.f;
	constexpr float NeuroChangeEvHazardKeepout = 400.f;
	constexpr float NeuroChangeEvFloorZTarget = -1100.f;
	constexpr float NeuroChangeEvFloorZEps = 150.f;
	constexpr float NeuroChangeEvNormalUpMin = 0.7f;
	constexpr float NeuroChangeEvPeerInteractionRange = 175.f;
	constexpr float NeuroChangeEvArrayInteractionRange = 200.f;
	constexpr float NeuroChangeEvPeerMarginMin = 50.f;
	constexpr float NeuroChangeEvCubeHalf = 50.f;
	const TCHAR* NeuroChangeEvObservationPeerLabel = TEXT("NeuralSignatureObservationNode_NeuroGenetics");

	UStaticMeshComponent* NeuroChangeEv_FindMeshComponent(AActor* Actor, const TCHAR* PropertyName)
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

	FString NeuroChangeEv_RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
					TEXT("Arbitrary argument '%s' is refused. spawn_neuro_neural_change_evidence_instrument uses fixed native constants only."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroChangeEvSpec);
		if (!Spec.Equals(NeuroChangeEvSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_neural_change_evidence_instrument_v1.");
		}
		return TEXT("");
	}

	FString NeuroChangeEv_RequireBeat6MissionPersistedClean()
	{
		UObject* Mission = FindNeuroGeneticsMissionAssetExact();
		if (!Mission)
		{
			return TEXT(
				"DA_Mission_NeuroGenetics is required and must already exist (persisted, clean) matching neurogenetics_mission_beat6_v1. ZERO writes.");
		}
		if (const FString Beat5Mismatch = NeuroGeneticsMissionBeat6MismatchReason(Mission); !Beat5Mismatch.IsEmpty())
		{
			if (NeuroGeneticsMissionBeat5MismatchReason(Mission).IsEmpty())
			{
				return TEXT(
					"DA_Mission_NeuroGenetics is still exact Beat 5. Require persisted clean Beat 6 (expand_neurogenetics_mission_beat6 + save) before spawn_neuro_neural_change_evidence_instrument. ZERO writes.");
			}
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 6: %s. Fail closed — no opportunistic repair."),
				*Beat5Mismatch);
		}
		if (UPackage* MissionPkg = Mission->GetOutermost())
		{
			if (MissionPkg->IsDirty())
			{
				return TEXT("DA_Mission_NeuroGenetics package is dirty. Require persisted clean Beat 6 mission asset. ZERO writes.");
			}
		}
		return TEXT("");
	}

	FString NeuroChangeEv_RequireLevelsLoadedVisible(UWorld* World)
	{
		return NeuroMappingArray_RequireLevelsLoadedVisible(World);
	}

	FBox NeuroChangeEv_ExpectedMeshWorldBox(const FVector& RelLoc, const FVector& RelScale)
	{
		const FVector Half = FVector(
			NeuroChangeEvCubeHalf * RelScale.X,
			NeuroChangeEvCubeHalf * RelScale.Y,
			NeuroChangeEvCubeHalf * RelScale.Z);
		const FVector Center = NeuroChangeEvLocation + RelLoc;
		return FBox(Center - Half, Center + Half);
	}

	FString NeuroChangeEv_MeshPresentationMismatch(
		AActor* Actor,
		const TCHAR* ComponentName,
		const TCHAR* ExpectedMeshPath,
		const FVector& RelLocation,
		const FVector& RelScale)
	{
		return NeuroMappingArray_MeshPresentationMismatch(
			Actor, ComponentName, ExpectedMeshPath, RelLocation, RelScale);
	}

	FString NeuroChangeEv_ActorMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ClassName(Actor).Equals(NeuroChangeEvClassName, ESearchCase::CaseSensitive)
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
		if (!ActorLabel(Actor).Equals(NeuroChangeEvLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' expected NeuralChangeEvidenceInstrument_NeuroGenetics"), *ActorLabel(Actor));
		}
		if (const FString Xform = TransformMismatch(
				Actor, NeuroChangeEvLocation, NeuroChangeEvRotation, NeuroChangeEvScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("InteractionRange"), NeuroChangeEvInteractionRange);
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
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroChangeEvRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ObjectiveEventId"), NeuroChangeEvObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroChangeEvReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionPrompt"), NeuroChangeEvInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ReviewPrompt"), NeuroChangeEvReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("SpeakerLabel"), NeuroChangeEvSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionResponseText"), NeuroChangeEvResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(
				Actor, TEXT("NotificationDurationSeconds"), NeuroChangeEvNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroChangeEv_MeshPresentationMismatch(
				Actor,
				TEXT("PedestalMesh"),
				TEXT("/Engine/BasicShapes/Cube"),
				NeuroChangeEvPedestalRel,
				NeuroChangeEvPedestalScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroChangeEv_MeshPresentationMismatch(
				Actor,
				TEXT("ColumnMesh"),
				TEXT("/Engine/BasicShapes/Cylinder"),
				NeuroChangeEvColumnRel,
				NeuroChangeEvColumnScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroChangeEv_MeshPresentationMismatch(
				Actor,
				TEXT("ArrayHeadMesh"),
				TEXT("/Engine/BasicShapes/Cube"),
				NeuroChangeEvHeadRel,
				NeuroChangeEvHeadScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString NeuroChangeEv_ApplyFields(AActor* Actor)
	{
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("InteractionRange"), NeuroChangeEvInteractionRange);
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
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroChangeEvRequiredActive);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("ObjectiveEventId"), NeuroChangeEvObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroChangeEvReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionPrompt"), NeuroChangeEvInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("ReviewPrompt"), NeuroChangeEvReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_SetTextProperty(Actor, TEXT("SpeakerLabel"), NeuroChangeEvSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionResponseText"), NeuroChangeEvResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("NotificationDurationSeconds"), NeuroChangeEvNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InteractionPrompt"), NeuroChangeEvInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}

		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroChangeEv_FindMeshComponent(Actor, TEXT("PedestalMesh")),
				NeuroChangeEvCubePath,
				NeuroChangeEvPedestalRel,
				NeuroChangeEvPedestalScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("PedestalMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroChangeEv_FindMeshComponent(Actor, TEXT("ColumnMesh")),
				NeuroChangeEvCylinderPath,
				NeuroChangeEvColumnRel,
				NeuroChangeEvColumnScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ColumnMesh (temporary Cylinder blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroChangeEv_FindMeshComponent(Actor, TEXT("ArrayHeadMesh")),
				NeuroChangeEvCubePath,
				NeuroChangeEvHeadRel,
				NeuroChangeEvHeadScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ArrayHeadMesh (temporary Cube blockout): %s"), *Error);
		}
		return TEXT("");
	}

	FString NeuroChangeEv_FloorTraceAt(UWorld* World, const FVector& SampleXY)
	{
		const FVector Start(SampleXY.X, SampleXY.Y, NeuroChangeEvLocation.Z + 80.f);
		const FVector End(SampleXY.X, SampleXY.Y, NeuroChangeEvLocation.Z - 250.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuroChangeEvFloor), false);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
		if (!bHit || !Hit.bBlockingHit)
		{
			return FString::Printf(
				TEXT("Downward floor trace missed at (%.0f,%.0f). Unsupported placement."),
				SampleXY.X,
				SampleXY.Y);
		}
		if (Hit.ImpactNormal.Z < NeuroChangeEvNormalUpMin)
		{
			return FString::Printf(
				TEXT("Floor normal Z=%.3f at (%.0f,%.0f) is not upward-facing."),
				Hit.ImpactNormal.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		if (FMath::Abs(Hit.ImpactPoint.Z - NeuroChangeEvFloorZTarget) > NeuroChangeEvFloorZEps
			&& FMath::Abs(Hit.ImpactPoint.Z - (-1200.f)) > NeuroChangeEvFloorZEps)
		{
			return FString::Printf(
				TEXT("Supported surface Z=%.1f at (%.0f,%.0f) is not near -1100."),
				Hit.ImpactPoint.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		return TEXT("");
	}

	FString NeuroChangeEv_NavProjection(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSystem)
		{
			return TEXT("Neuro navigation system unavailable. ZERO writes.");
		}
		const FVector Samples[] = {
			NeuroChangeEvLocation,
			NeuroChangeEvLocation + FVector(40.f, 0.f, 0.f),
			NeuroChangeEvLocation + FVector(-40.f, 0.f, 0.f),
			NeuroChangeEvLocation + FVector(0.f, 40.f, 0.f),
			NeuroChangeEvLocation + FVector(0.f, -40.f, 0.f),
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


	FString NeuroChangeEv_RouteFromObservationNode(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSystem)
		{
			return TEXT("Neuro navigation system unavailable for route proof. ZERO writes.");
		}
		ANavigationData* NavData = NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
		if (!NavData)
		{
			return TEXT("Default navigation data unavailable for route proof. ZERO writes.");
		}

		FNavLocation StartProjected;
		FNavLocation EndProjected;
		if (!NavSystem->ProjectPointToNavigation(
				NeuroSigObsLocation, StartProjected, FVector(200.f, 200.f, 300.f)))
		{
			return TEXT("Route start (NeuralSignatureObservationNode area) failed nav projection. Fail closed — no destination fallback.");
		}
		if (!NavSystem->ProjectPointToNavigation(NeuroChangeEvLocation, EndProjected, FVector(200.f, 200.f, 300.f)))
		{
			return TEXT("Route end (evidence instrument destination) failed nav projection. Fail closed — no destination fallback.");
		}

		const FPathFindingQuery Query(NavSystem, *NavData, StartProjected.Location, EndProjected.Location);
		const FPathFindingResult Result = NavSystem->FindPathSync(Query);
		TSharedRef<FJsonObject> Route = MakeShared<FJsonObject>();
		Route->SetArrayField(TEXT("start_requested"), Vec(NeuroSigObsLocation));
		Route->SetArrayField(TEXT("start_projected"), Vec(StartProjected.Location));
		Route->SetArrayField(TEXT("end_requested"), Vec(NeuroChangeEvLocation));
		Route->SetArrayField(TEXT("end_projected"), Vec(EndProjected.Location));
		Route->SetBoolField(TEXT("path_valid"), Result.IsSuccessful() && Result.Path.IsValid());
		Route->SetBoolField(TEXT("path_partial"), Result.IsPartial());
		if (!Result.IsSuccessful() || !Result.Path.IsValid() || Result.IsPartial())
		{
			OutMargins->SetObjectField(TEXT("route_evidence"), Route);
			return TEXT(
				"Native nav route from NeuralSignatureObservationNode area toward evidence instrument destination failed or is partial. Fail closed — no automatic fallback.");
		}

		const float PathLength = Result.Path->GetLength();
		const int32 PointCount = Result.Path->GetPathPoints().Num();
		Route->SetNumberField(TEXT("path_length"), PathLength);
		Route->SetNumberField(TEXT("path_point_count"), PointCount);
		Route->SetStringField(
			TEXT("note"),
			TEXT("Native FindPathSync from observation-node area toward fixed evidence-instrument destination."));
		OutMargins->SetObjectField(TEXT("route_evidence"), Route);
		if (PathLength <= 0.f || PointCount < 2)
		{
			return TEXT("Native route proof returned empty path. Fail closed — no automatic fallback.");
		}
		return TEXT("");
	}

	FString NeuroChangeEv_MeshBoundsClearance(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FBox Boxes[] = {
			NeuroChangeEv_ExpectedMeshWorldBox(
				NeuroChangeEvPedestalRel, NeuroChangeEvPedestalScale),
			NeuroChangeEv_ExpectedMeshWorldBox(
				NeuroChangeEvColumnRel, NeuroChangeEvColumnScale),
			NeuroChangeEv_ExpectedMeshWorldBox(
				NeuroChangeEvHeadRel, NeuroChangeEvHeadScale),
		};
		FBox Combined = Boxes[0] + Boxes[1] + Boxes[2];
		OutMargins->SetArrayField(TEXT("combined_aabb_min"), Vec(Combined.Min));
		OutMargins->SetArrayField(TEXT("combined_aabb_max"), Vec(Combined.Max));

		TArray<TSharedPtr<FJsonValue>> IgnoredStreaming;
		TArray<TSharedPtr<FJsonValue>> IgnoredNavBounds;
		TSharedPtr<FJsonObject> LabBench2Row;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other)
			{
				continue;
			}
			const FString Label = ActorLabel(Other);
			if (Label.Equals(NeuroChangeEvLabel, ESearchCase::CaseSensitive)
				|| Label.Equals(TEXT("NeuroGenetics_FloorPlate"), ESearchCase::CaseSensitive)
				|| Label.Contains(TEXT("FloorPlate")))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroChangeEvLocation) > 800.f)
			{
				continue;
			}
			FVector Origin, Extent;
			Other->GetActorBounds(false, Origin, Extent);
			const FBox OtherBox(Origin - Extent, Origin + Extent);
			const bool bIntersects = Combined.Intersect(OtherBox);
			if (Label.Equals(TEXT("Neuro_Lab_Bench_2"), ESearchCase::CaseSensitive))
			{
				LabBench2Row = MakeShared<FJsonObject>();
				LabBench2Row->SetStringField(TEXT("label"), Label);
				LabBench2Row->SetArrayField(TEXT("origin"), Vec(Origin));
				LabBench2Row->SetArrayField(TEXT("extent"), Vec(Extent));
				LabBench2Row->SetBoolField(TEXT("intersects_blockout_aabb"), bIntersects);
				LabBench2Row->SetNumberField(
					TEXT("center_distance"), FVector::Dist(Other->GetActorLocation(), NeuroChangeEvLocation));
			}
			if (!bIntersects)
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
				TEXT("Configured Cube/Cylinder blockout AABB intersects authored actor '%s'. Fail closed — no fallback transform."),
				*Label);
		}
		OutMargins->SetArrayField(TEXT("ignored_streaming_volumes"), IgnoredStreaming);
		OutMargins->SetArrayField(TEXT("ignored_nav_mesh_bounds"), IgnoredNavBounds);
		if (LabBench2Row.IsValid())
		{
			OutMargins->SetObjectField(TEXT("neuro_lab_bench_2"), LabBench2Row.ToSharedRef());
			if (LabBench2Row->GetBoolField(TEXT("intersects_blockout_aabb")))
			{
				return TEXT("Neuro_Lab_Bench_2 overlaps configured observation-node blockout AABB. Fail closed — no fallback transform.");
			}
		}
		else
		{
			return TEXT("Neuro_Lab_Bench_2 not found within proximity for nonoverlap proof. Fail closed.");
		}
		return TEXT("");
	}

	FString NeuroChangeEv_PlacementGeometry(UWorld* World, TSharedRef<FJsonObject> OutMargins)
	{
		const FVector FootSamples[] = {
			NeuroChangeEvLocation,
			NeuroChangeEvLocation + FVector(40.f, 40.f, 0.f),
			NeuroChangeEvLocation + FVector(40.f, -40.f, 0.f),
			NeuroChangeEvLocation + FVector(-40.f, 40.f, 0.f),
			NeuroChangeEvLocation + FVector(-40.f, -40.f, 0.f),
		};
		for (const FVector& Sample : FootSamples)
		{
			if (const FString TraceError = NeuroChangeEv_FloorTraceAt(World, Sample); !TraceError.IsEmpty())
			{
				return TraceError;
			}
		}

		if (const FString BoundsError = NeuroChangeEv_MeshBoundsClearance(World, OutMargins); !BoundsError.IsEmpty())
		{
			return BoundsError;
		}
		if (const FString NavError = NeuroChangeEv_NavProjection(World, OutMargins); !NavError.IsEmpty())
		{
			return NavError;
		}
		if (const FString RouteError = NeuroChangeEv_RouteFromObservationNode(World, OutMargins); !RouteError.IsEmpty())
		{
			return RouteError;
		}

		struct FDisjoint
		{
			const TCHAR* Label;
			float Range;
			float RequiredMargin;
		};
		const FDisjoint Interactables[] = {
			{NeuroChangeEvObservationPeerLabel, NeuroChangeEvPeerInteractionRange, NeuroChangeEvPeerMarginMin},
			{TEXT("NeuralMappingTerminal_NeuroGenetics"), NeuroChangeEvPeerInteractionRange, NeuroChangeEvPeerMarginMin},
			{TEXT("EmergencyCutoff_NeuroResearchLoad"), NeuroChangeEvPeerInteractionRange, NeuroChangeEvPeerMarginMin},
			{TEXT("NeuralMappingArray_NeuroGenetics"), NeuroChangeEvArrayInteractionRange, NeuroChangeEvPeerMarginMin},
			{TEXT("PowerPanel_NeuroBackup"), 220.f, 0.f},
			{TEXT("DataPad_NeuroPowerDiagnostics"), 200.f, 0.f},
			{TEXT("DataPad_NeuroContainment"), 200.f, 0.f},
			{TEXT("DataPad_NeuroResearchFailure"), 200.f, 0.f},
			{TEXT("ResearchStation_NeuroGenetics"), 250.f, 0.f},
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
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroChangeEvLocation);
			const float Combined = NeuroChangeEvInteractionRange + Entry.Range;
			const float Margin = Dist - Combined;
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Entry.Label);
			Row->SetNumberField(TEXT("distance"), Dist);
			Row->SetNumberField(TEXT("combined_radii"), Combined);
			Row->SetNumberField(TEXT("margin"), Margin);
			Row->SetNumberField(TEXT("required_margin"), Entry.RequiredMargin);
			ClearanceRows.Add(MakeShared<FJsonValueObject>(Row));
			if (Margin < Entry.RequiredMargin)
			{
				return FString::Printf(
					TEXT("Interaction space vs %s insufficient (margin %.1f < required %.0f; dist %.1f, combined %.0f). Fail closed — no destination fallback."),
					Entry.Label,
					Margin,
					Entry.RequiredMargin,
					Dist,
					Combined);
			}
		}
		OutMargins->SetArrayField(TEXT("interactable_clearance"), ClearanceRows);

		struct FKeepout
		{
			const TCHAR* Label;
			float MinDist;
		};
		const FKeepout HostsAndHazard[] = {
			{TEXT("Host_Neuro_1"), NeuroChangeEvHostKeepout},
			{TEXT("Host_Neuro_2"), NeuroChangeEvHostKeepout},
			{TEXT("Host_Neuro_3"), NeuroChangeEvHostKeepout},
			{TEXT("Host_Neuro_Researcher"), NeuroChangeEvHostKeepout},
			{TEXT("Hazard_ScrubberLeak"), NeuroChangeEvHazardKeepout},
		};
		TArray<TSharedPtr<FJsonValue>> KeepoutRows;
		for (const FKeepout& Entry : HostsAndHazard)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1 for keepout."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroChangeEvLocation);
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
			const FVector Delta = (NeuroChangeEvLocation - TrapLoc).GetAbs();
			if (Delta.X <= TrapExtent.X + SphereExtent.X
				&& Delta.Y <= TrapExtent.Y + SphereExtent.Y
				&& Delta.Z <= TrapExtent.Z + SphereExtent.Z)
			{
				return TEXT("Approved observation destination overlaps CorridorTraps_GowningRing keepout. Fail closed — no destination fallback.");
			}
		}

		TArray<AActor*> GateMatches = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
		if (GateMatches.Num() != 1)
		{
			return FString::Printf(TEXT("Gate_ResearchWing count=%d expected=1 for door keepout."), GateMatches.Num());
		}
		if (FVector::Dist(GateMatches[0]->GetActorLocation(), NeuroChangeEvLocation)
			< NeuroChangeEvDoorKeepout)
		{
			return TEXT("Keepout violated vs Gate_ResearchWing (door). Fail closed — no destination fallback.");
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other || ActorLabel(Other).Equals(NeuroChangeEvLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroChangeEvLocation) <= 5.f)
			{
				return FString::Printf(
					TEXT("Conflicting actor '%s' within 5uu of observation destination. Fail closed — no fallback."),
					*ActorLabel(Other));
			}
		}
		return TEXT("");
	}

	FString NeuroChangeEv_KeepList(UWorld* World)
	{
		if (const FString Error = NeuroMappingTerminal_KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(
				World,
				NeuroChangeEvObservationPeerLabel,
				NeuroSigObsLocation,
				NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list NeuralSignatureObservationNode_NeuroGenetics: %s"), *Error);
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> NeuroChangeEv_KeepListSnapshot(UWorld* World)
	{
		TSharedRef<FJsonObject> Snap = MakeShared<FJsonObject>();
		const TCHAR* Labels[] = {
			TEXT("PowerPanel_NeuroBackup"), TEXT("DataPad_NeuroPowerDiagnostics"), TEXT("DataPad_NeuroContainment"),
			TEXT("DataPad_NeuroResearchFailure"), TEXT("ResearchStation_NeuroGenetics"),
			TEXT("Host_Neuro_1"), TEXT("Host_Neuro_2"), TEXT("Host_Neuro_3"), TEXT("Host_Neuro_Researcher"),
			TEXT("Hazard_ScrubberLeak"), TEXT("CorridorTraps_GowningRing"), TEXT("NeuralMappingArray_NeuroGenetics"),
			TEXT("EmergencyCutoff_NeuroResearchLoad"), TEXT("NeuralMappingTerminal_NeuroGenetics"),
			TEXT("NeuralSignatureObservationNode_NeuroGenetics"),
			TEXT("Neuro_Lab_Bench_2"), TEXT("Gate_ResearchWing"), TEXT("NeuroGenetics_FloorPlate"),
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

	TSharedRef<FJsonObject> NeuroChangeEv_ProposedComponentState()
	{
		auto MeshObj = [](const TCHAR* Name, const TCHAR* MeshPath, const FVector& Rel, const FVector& Scale)
		{
			TSharedRef<FJsonObject> M = MakeShared<FJsonObject>();
			M->SetStringField(TEXT("component"), Name);
			M->SetStringField(TEXT("mesh"), MeshPath);
			M->SetArrayField(TEXT("relative_location"), Vec(Rel));
			M->SetArrayField(TEXT("relative_rotation"), Vec(FVector::ZeroVector));
			M->SetArrayField(TEXT("relative_scale"), Vec(Scale));
			M->SetStringField(TEXT("collision"), TEXT("NoCollision"));
			M->SetBoolField(TEXT("generate_overlap_events"), false);
			M->SetStringField(TEXT("visual_note"), TEXT("Temporary replaceable Engine blockout — not final art."));
			return M;
		};
		TArray<TSharedPtr<FJsonValue>> Meshes;
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("PedestalMesh"), NeuroChangeEvCubePath, NeuroChangeEvPedestalRel, NeuroChangeEvPedestalScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ColumnMesh"), NeuroChangeEvCylinderPath, NeuroChangeEvColumnRel, NeuroChangeEvColumnScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ArrayHeadMesh"), NeuroChangeEvCubePath, NeuroChangeEvHeadRel, NeuroChangeEvHeadScale)));

		TSharedRef<FJsonObject> Actor = MakeShared<FJsonObject>();
		Actor->SetStringField(TEXT("label"), NeuroChangeEvLabel);
		Actor->SetStringField(TEXT("class"), NeuroChangeEvClassName);
		Actor->SetStringField(TEXT("persistent_package"), EpitopePackage);
		Actor->SetStringField(TEXT("destination_package"), NeuroPackage);
		Actor->SetArrayField(TEXT("location"), Vec(NeuroChangeEvLocation));
		Actor->SetArrayField(TEXT("rotation"), Vec(FVector::ZeroVector));
		Actor->SetArrayField(TEXT("scale"), Vec(NeuroChangeEvScale));
		Actor->SetNumberField(TEXT("interaction_range"), NeuroChangeEvInteractionRange);
		Actor->SetStringField(TEXT("required_active_objective_id"), NeuroChangeEvRequiredActive);
		Actor->SetStringField(TEXT("objective_event_id"), NeuroChangeEvObjectiveEvent);
		Actor->SetStringField(TEXT("completed_objective_id_for_replay_guard"), NeuroChangeEvReplayGuard);
		Actor->SetStringField(TEXT("inspection_prompt"), NeuroChangeEvInspectPrompt);
		Actor->SetStringField(TEXT("review_prompt"), NeuroChangeEvReviewPrompt);
		Actor->SetStringField(TEXT("speaker_label"), NeuroChangeEvSpeaker);
		Actor->SetStringField(TEXT("inspection_response_text"), NeuroChangeEvResponse);
		Actor->SetNumberField(TEXT("notification_duration_seconds"), NeuroChangeEvNotifySeconds);
		Actor->SetBoolField(TEXT("b_has_been_inspected"), false);
		Actor->SetArrayField(TEXT("presentation_meshes"), Meshes);
		Actor->SetStringField(
			TEXT("visual_note"),
			TEXT("PedestalMesh Cube / ColumnMesh Cylinder / ArrayHeadMesh Cube are temporary replaceable Engine blockouts — not final art."));
		return Actor;
	}

	FString NeuroChangeEv_DestroyAndRestore(AActor* Actor, UPackage* NeuroPkg, bool bNeuroWasDirtyBefore)
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

	FString PreflightSpawnNeuroNeuralChangeEvidenceInstrument(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. spawn_neuro_neural_change_evidence_instrument is an Unreal Editor write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_neural_change_evidence_instrument does not save or compile.");
		}
		if (const FString OverrideError = NeuroChangeEv_RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_neural_change_evidence_instrument preflight must run on the game thread.");
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
		if (const FString VisError = NeuroChangeEv_RequireLevelsLoadedVisible(World); !VisError.IsEmpty())
		{
			return VisError;
		}
		if (const FString MissionError = NeuroChangeEv_RequireBeat6MissionPersistedClean(); !MissionError.IsEmpty())
		{
			return MissionError;
		}
		if (!LoadClass<AActor>(nullptr, NeuroChangeEvClassPath))
		{
			return TEXT("AProjectOrganoidInspectableInstrument class is not loaded.");
		}
		if (const FString KeepError = NeuroChangeEv_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroChangeEv_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
		{
			return PlaceError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroChangeEvLabel);
		bool bAlreadyExact = false;
		if (Matches.Num() > 1)
		{
			return FString::Printf(
				TEXT("%s count=%d. Abort rather than stack."), NeuroChangeEvLabel, Matches.Num());
		}
		if (Matches.Num() == 1)
		{
			const FString Mismatch = NeuroChangeEv_ActorMismatch(Matches[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("%s exists but mismatches: %s. Fail closed — no opportunistic repair."),
					NeuroChangeEvLabel,
					*Mismatch);
			}
			bAlreadyExact = true;
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroChangeEvSpec);
		Before->SetStringField(TEXT("action"), NeuroChangeEvAction);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("mission_beat6_ready"), true);
		Before->SetStringField(TEXT("mission_object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetObjectField(TEXT("keep_list"), NeuroChangeEv_KeepListSnapshot(World));
		Before->SetObjectField(TEXT("placement_margins"), Margins);
		if (Matches.Num() == 1)
		{
			Before->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
		}

		Proposed->SetStringField(TEXT("spec"), NeuroChangeEvSpec);
		Proposed->SetObjectField(TEXT("actor"), NeuroChangeEv_ProposedComponentState());
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
				? TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics already matches neuro_neural_change_evidence_instrument_v1. Execute is a clean no-op. Requires all packages clean. Temporary Cube/Cylinder blockout — not final art.")
				: TEXT("Spawn one temporary Cube/Cylinder-blockout NeuralChangeEvidenceInstrument_NeuroGenetics (InspectableInstrument) on Neuro at (500,-2520,-1100) yaw 90. Requires persisted clean Beat 6 DA_Mission_NeuroGenetics. Does not interact, fire events, change power/Hosts/pads, save, or compile. Not final art."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroNeuralChangeEvidenceInstrument(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("spawn_neuro_neural_change_evidence_instrument must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroNeuralChangeEvidenceInstrument(Change.Args, Before, Proposed);
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
		if (const FString MissionError = NeuroChangeEv_RequireBeat6MissionPersistedClean(); !MissionError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mission_required"),
				FString::Printf(TEXT("ZERO writes. %s"), *MissionError),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroChangeEv_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("keep_list"),
				FString::Printf(TEXT("ZERO writes. %s"), *KeepError),
				MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Margins = MakeShared<FJsonObject>();
		if (const FString PlaceError = NeuroChangeEv_PlacementGeometry(World, Margins); !PlaceError.IsEmpty())
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

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroChangeEvLabel);
		if (Matches.Num() == 1 && NeuroChangeEv_ActorMismatch(Matches[0]).IsEmpty())
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
			Change.After->SetStringField(TEXT("label"), NeuroChangeEvLabel);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
			Change.After->SetObjectField(TEXT("proposed"), NeuroChangeEv_ProposedComponentState());
			Change.After->SetObjectField(TEXT("placement_margins"), Margins);
			Change.After->SetStringField(
				TEXT("visual_note"),
				TEXT("Temporary Engine Cube/Cylinder blockout — replaceable presentation, not final art."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Matches.Num() != 0)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UClass* InstrumentClass = LoadClass<AActor>(nullptr, NeuroChangeEvClassPath);
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
				"SpawnNeuroNeuralChangeEvidenceInstrument",
				"Spawn Neuro Neural Change Evidence Instrument (temporary blockout)"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Spawned = World->SpawnActor<AActor>(
				InstrumentClass, NeuroChangeEvLocation, NeuroChangeEvRotation, Params);
			if (!Spawned)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("spawn_failed"),
					TEXT("SpawnActor returned null. ZERO remaining writes."),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(NeuroChangeEvLabel, true);
			Spawned->SetActorScale3D(NeuroChangeEvScale);
			if (const FString ApplyError = NeuroChangeEv_ApplyFields(Spawned); !ApplyError.IsEmpty())
			{
				const FString CleanupError =
					NeuroChangeEv_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
			if (const FString AfterError = NeuroChangeEv_ActorMismatch(Spawned); !AfterError.IsEmpty())
			{
				const FString CleanupError =
					NeuroChangeEv_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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

		if (const FString KeepError = NeuroChangeEv_KeepList(World); !KeepError.IsEmpty())
		{
			const FString CleanupError =
				NeuroChangeEv_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
				NeuroChangeEv_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
		Change.After->SetStringField(TEXT("label"), NeuroChangeEvLabel);
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
		Change.After->SetObjectField(TEXT("proposed"), NeuroChangeEv_ProposedComponentState());
		Change.After->SetObjectField(TEXT("placement_margins"), Margins);
		Change.After->SetStringField(
			TEXT("visual_note"),
			TEXT("Temporary Engine Cube/Cylinder blockout — PedestalMesh/ColumnMesh/ArrayHeadMesh replaceable, not final art."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}