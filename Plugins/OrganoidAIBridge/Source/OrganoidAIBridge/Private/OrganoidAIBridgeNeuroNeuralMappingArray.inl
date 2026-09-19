	// Fixed NeuroGenetics neural-mapping-array spawn — spawn_neuro_neural_mapping_array / neuro_neural_mapping_array_v1.
	// Temporary Engine Cube/Cylinder blockout presentation — replaceable, not final art.
	const TCHAR* NeuroMappingArraySpec = TEXT("neuro_neural_mapping_array_v1");
	const TCHAR* NeuroMappingArrayAction = TEXT("spawn_neuro_neural_mapping_array");
	const TCHAR* NeuroMappingArrayLabel = TEXT("NeuralMappingArray_NeuroGenetics");
	const TCHAR* NeuroMappingArrayClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroMappingArrayClassName = TEXT("ProjectOrganoidInspectableInstrument");
	const TCHAR* NeuroMappingArrayCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroMappingArrayCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* NeuroMappingArrayObjectiveEvent = TEXT("Event_NeuroResearchArrayLocated");
	const TCHAR* NeuroMappingArrayReplayGuard = TEXT("Obj_InvestigateNeuroResearchFloor");
	const TCHAR* NeuroMappingArrayInspectPrompt = TEXT("Inspect Neural Mapping Array");
	const TCHAR* NeuroMappingArrayReviewPrompt = TEXT("Review Neural Mapping Array");
	const TCHAR* NeuroMappingArraySpeaker = TEXT("Nathan");
	// Curly apostrophe as ASCII \u2019 escape — matches AProjectOrganoidInspectableInstrument default.
	const TCHAR* NeuroMappingArrayResponse =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	const FVector NeuroMappingArrayLocation(-500.f, -600.f, -1100.f);
	const FRotator NeuroMappingArrayRotation = FRotator::ZeroRotator;
	const FVector NeuroMappingArrayScale = FVector::OneVector;
	constexpr float NeuroMappingArrayInteractionRange = 200.f;
	constexpr float NeuroMappingArrayNotifySeconds = 4.f;
	const FVector NeuroMappingArrayPedestalRel(0.f, 0.f, 40.f);
	const FVector NeuroMappingArrayPedestalScale(1.2f, 1.2f, 0.8f);
	const FVector NeuroMappingArrayColumnRel(0.f, 0.f, 110.f);
	const FVector NeuroMappingArrayColumnScale(0.35f, 0.35f, 1.4f);
	const FVector NeuroMappingArrayHeadRel(0.f, 0.f, 192.5f);
	const FVector NeuroMappingArrayHeadScale(1.6f, 1.6f, 0.25f);
	constexpr float NeuroMappingArrayHostKeepout = 400.f;
	constexpr float NeuroMappingArrayDoorKeepout = 400.f;
	constexpr float NeuroMappingArrayHazardKeepout = 400.f;
	constexpr float NeuroMappingArrayFloorZTarget = -1100.f;
	constexpr float NeuroMappingArrayFloorZEps = 150.f;
	constexpr float NeuroMappingArrayNormalUpMin = 0.7f;

	UStaticMeshComponent* NeuroMappingArray_FindMeshComponent(AActor* Actor, const TCHAR* PropertyName)
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

	FString NeuroMappingArray_RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
			TEXT("completed_objective"), TEXT("prompt"), TEXT("inspection_prompt"), TEXT("review_prompt"),
			TEXT("speaker"), TEXT("response"), TEXT("notification_duration"), TEXT("transform"),
		};
		for (const TCHAR* Key : Forbidden)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(
					TEXT("Arbitrary argument '%s' is refused. spawn_neuro_neural_mapping_array uses fixed native constants only."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroMappingArraySpec);
		if (!Spec.Equals(NeuroMappingArraySpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_neural_mapping_array_v1.");
		}
		return TEXT("");
	}

	FString NeuroMappingArray_RequireMissionAssetPersistedClean()
	{
		UObject* Mission = FindNeuroGeneticsMissionAssetExact();
		if (!Mission)
		{
			return TEXT(
				"DA_Mission_NeuroGenetics is required and must already exist (persisted, clean) matching neurogenetics_mission_v1. ZERO writes.");
		}
		if (const FString Mismatch = NeuroGeneticsMissionMismatchReason(Mission); !Mismatch.IsEmpty())
		{
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics exists but mismatches S3B contract: %s. Fail closed."),
				*Mismatch);
		}
		if (UPackage* MissionPkg = Mission->GetOutermost())
		{
			if (MissionPkg->IsDirty())
			{
				return TEXT("DA_Mission_NeuroGenetics package is dirty. Require persisted clean mission asset. ZERO writes.");
			}
		}
		return TEXT("");
	}

	FString NeuroMappingArray_RequireLevelsLoadedVisible(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded and visible. ZERO writes.");
		}
		if (!FindLoadedLevelByPackage(World, NeuroPackage))
		{
			return TEXT("SL_Epitope_NeuroGenetics must be loaded and visible. ZERO writes.");
		}
		auto StreamVisible = [World](const TCHAR* PackageName) -> FString
		{
			for (ULevelStreaming* Streaming : World->GetStreamingLevels())
			{
				if (!Streaming)
				{
					continue;
				}
				const FString StreamPkg = NormalizePackage(Streaming->GetWorldAssetPackageFName().ToString());
				if (!PackagesEqual(StreamPkg, PackageName))
				{
					continue;
				}
				if (!Streaming->IsLevelVisible())
				{
					return FString::Printf(TEXT("%s is loaded but not visible. ZERO writes."), PackageName);
				}
				return TEXT("");
			}
			return TEXT("");
		};
		if (const FString AdminVis = StreamVisible(AdminPackage); !AdminVis.IsEmpty())
		{
			return AdminVis;
		}
		if (const FString NeuroVis = StreamVisible(NeuroPackage); !NeuroVis.IsEmpty())
		{
			return NeuroVis;
		}
		return TEXT("");
	}

	FString NeuroMappingArray_MeshPresentationMismatch(
		AActor* Actor,
		const TCHAR* ComponentName,
		const TCHAR* ExpectedMeshPath,
		const FVector& RelLocation,
		const FVector& RelScale)
	{
		UStaticMeshComponent* Mesh = NeuroMappingArray_FindMeshComponent(Actor, ComponentName);
		if (!Mesh)
		{
			return FString::Printf(TEXT("%s component missing."), ComponentName);
		}
		const FString Path = Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName() : FString();
		if (!Path.Contains(ExpectedMeshPath) && !Path.Equals(ExpectedMeshPath))
		{
			// Engine meshes report as /Engine/BasicShapes/Cube.Cube — accept Contains on asset name.
			const FString Short = FString(ExpectedMeshPath);
			int32 Dot = INDEX_NONE;
			FString AssetName = Short;
			if (Short.FindLastChar(TEXT('.'), Dot))
			{
				AssetName = Short.Mid(Dot + 1);
			}
			if (!Path.Contains(AssetName))
			{
				return FString::Printf(
					TEXT("%s asset '%s' expected temporary %s blockout (not final art)."),
					ComponentName,
					*Path,
					ExpectedMeshPath);
			}
		}
		if (!LocationMatches(Mesh->GetRelativeLocation(), RelLocation))
		{
			return FString::Printf(TEXT("%s relative location mismatch."), ComponentName);
		}
		if (!RotationMatches(Mesh->GetRelativeRotation(), FRotator::ZeroRotator))
		{
			return FString::Printf(TEXT("%s relative rotation must be (0,0,0)."), ComponentName);
		}
		if (!ScaleMatches(Mesh->GetRelativeScale3D(), RelScale))
		{
			return FString::Printf(TEXT("%s relative scale mismatch (temporary blockout)."), ComponentName);
		}
		if (Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			return FString::Printf(TEXT("%s collision must be NoCollision."), ComponentName);
		}
		if (Mesh->GetGenerateOverlapEvents())
		{
			return FString::Printf(TEXT("%s GenerateOverlapEvents must be false."), ComponentName);
		}
		return TEXT("");
	}

	FString NeuroMappingArray_ActorMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ClassName(Actor).Equals(NeuroMappingArrayClassName, ESearchCase::CaseSensitive)
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
		if (!ActorLabel(Actor).Equals(NeuroMappingArrayLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' expected NeuralMappingArray_NeuroGenetics"), *ActorLabel(Actor));
		}
		if (const FString Xform = TransformMismatch(
				Actor, NeuroMappingArrayLocation, NeuroMappingArrayRotation, NeuroMappingArrayScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckFloat(Actor, TEXT("InteractionRange"), NeuroMappingArrayInteractionRange);
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
				Actor, TEXT("ObjectiveEventId"), NeuroMappingArrayObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroMappingArrayReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionPrompt"), NeuroMappingArrayInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("ReviewPrompt"), NeuroMappingArrayReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("SpeakerLabel"), NeuroMappingArraySpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(
				Actor, TEXT("InspectionResponseText"), NeuroMappingArrayResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_CheckFloat(Actor, TEXT("NotificationDurationSeconds"), NeuroMappingArrayNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingArray_MeshPresentationMismatch(
				Actor,
				TEXT("PedestalMesh"),
				TEXT("/Engine/BasicShapes/Cube"),
				NeuroMappingArrayPedestalRel,
				NeuroMappingArrayPedestalScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingArray_MeshPresentationMismatch(
				Actor,
				TEXT("ColumnMesh"),
				TEXT("/Engine/BasicShapes/Cylinder"),
				NeuroMappingArrayColumnRel,
				NeuroMappingArrayColumnScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroMappingArray_MeshPresentationMismatch(
				Actor,
				TEXT("ArrayHeadMesh"),
				TEXT("/Engine/BasicShapes/Cylinder"),
				NeuroMappingArrayHeadRel,
				NeuroMappingArrayHeadScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString NeuroMappingArray_ApplyPresentationMesh(
		UStaticMeshComponent* Mesh,
		const TCHAR* MeshPath,
		const FVector& RelLocation,
		const FVector& RelScale)
	{
		if (!Mesh)
		{
			return TEXT("presentation mesh component missing.");
		}
		UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, MeshPath);
		if (!Asset)
		{
			return FString::Printf(TEXT("Failed to load temporary blockout mesh '%s'."), MeshPath);
		}
		Mesh->SetStaticMesh(Asset);
		Mesh->SetRelativeLocation(RelLocation);
		Mesh->SetRelativeRotation(FRotator::ZeroRotator);
		Mesh->SetRelativeScale3D(RelScale);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		return TEXT("");
	}

	FString NeuroMappingArray_ApplyFields(AActor* Actor)
	{
		if (const FString Error =
				NeuroPowerDiagnosis_SetFloatProperty(Actor, TEXT("InteractionRange"), NeuroMappingArrayInteractionRange);
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
				Actor, TEXT("ObjectiveEventId"), NeuroMappingArrayObjectiveEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(
				Actor, TEXT("CompletedObjectiveIdForReplayGuard"), NeuroMappingArrayReplayGuard);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionPrompt"), NeuroMappingArrayInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("ReviewPrompt"), NeuroMappingArrayReviewPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				NeuroPowerDiagnosis_SetTextProperty(Actor, TEXT("SpeakerLabel"), NeuroMappingArraySpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InspectionResponseText"), NeuroMappingArrayResponse);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(
				Actor, TEXT("NotificationDurationSeconds"), NeuroMappingArrayNotifySeconds);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(
				Actor, TEXT("InteractionPrompt"), NeuroMappingArrayInspectPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}

		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingArray_FindMeshComponent(Actor, TEXT("PedestalMesh")),
				NeuroMappingArrayCubePath,
				NeuroMappingArrayPedestalRel,
				NeuroMappingArrayPedestalScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("PedestalMesh (temporary Cube blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingArray_FindMeshComponent(Actor, TEXT("ColumnMesh")),
				NeuroMappingArrayCylinderPath,
				NeuroMappingArrayColumnRel,
				NeuroMappingArrayColumnScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ColumnMesh (temporary Cylinder blockout): %s"), *Error);
		}
		if (const FString Error = NeuroMappingArray_ApplyPresentationMesh(
				NeuroMappingArray_FindMeshComponent(Actor, TEXT("ArrayHeadMesh")),
				NeuroMappingArrayCylinderPath,
				NeuroMappingArrayHeadRel,
				NeuroMappingArrayHeadScale);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("ArrayHeadMesh (temporary Cylinder blockout): %s"), *Error);
		}
		return TEXT("");
	}

	FString NeuroMappingArray_FloorTraceAt(UWorld* World, const FVector& SampleXY)
	{
		const FVector Start(SampleXY.X, SampleXY.Y, NeuroMappingArrayLocation.Z + 80.f);
		const FVector End(SampleXY.X, SampleXY.Y, NeuroMappingArrayLocation.Z - 250.f);
		FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuroMappingArrayFloor), false);
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
		if (!bHit || !Hit.bBlockingHit)
		{
			return FString::Printf(
				TEXT("Downward floor trace missed at (%.0f,%.0f). Unsupported placement."),
				SampleXY.X,
				SampleXY.Y);
		}
		if (Hit.ImpactNormal.Z < NeuroMappingArrayNormalUpMin)
		{
			return FString::Printf(
				TEXT("Floor normal Z=%.3f at (%.0f,%.0f) is not upward-facing."),
				Hit.ImpactNormal.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		if (FMath::Abs(Hit.ImpactPoint.Z - NeuroMappingArrayFloorZTarget) > NeuroMappingArrayFloorZEps
			&& FMath::Abs(Hit.ImpactPoint.Z - (-1200.f)) > NeuroMappingArrayFloorZEps)
		{
			return FString::Printf(
				TEXT("Supported surface Z=%.1f at (%.0f,%.0f) is not near -1100."),
				Hit.ImpactPoint.Z,
				SampleXY.X,
				SampleXY.Y);
		}
		return TEXT("");
	}

	FString NeuroMappingArray_PlacementGeometry(UWorld* World)
	{
		// Center + base-footprint samples (pedestal ~1.2 scale).
		const FVector FootSamples[] = {
			NeuroMappingArrayLocation,
			NeuroMappingArrayLocation + FVector(50.f, 50.f, 0.f),
			NeuroMappingArrayLocation + FVector(50.f, -50.f, 0.f),
			NeuroMappingArrayLocation + FVector(-50.f, 50.f, 0.f),
			NeuroMappingArrayLocation + FVector(-50.f, -50.f, 0.f),
		};
		for (const FVector& Sample : FootSamples)
		{
			if (const FString TraceError = NeuroMappingArray_FloorTraceAt(World, Sample); !TraceError.IsEmpty())
			{
				return TraceError;
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
		for (const FDisjoint& Entry : Interactables)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1 for interaction clearance."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroMappingArrayLocation);
			const float Combined = NeuroMappingArrayInteractionRange + Entry.Range;
			if (Combined >= Dist)
			{
				return FString::Printf(
					TEXT("Interaction space overlaps %s (combined radii %.0f >= dist %.1f)."),
					Entry.Label,
					Combined,
					Dist);
			}
		}

		struct FKeepout
		{
			const TCHAR* Label;
			float MinDist;
		};
		const FKeepout HostsAndHazard[] = {
			{TEXT("Host_Neuro_1"), NeuroMappingArrayHostKeepout},
			{TEXT("Host_Neuro_2"), NeuroMappingArrayHostKeepout},
			{TEXT("Host_Neuro_3"), NeuroMappingArrayHostKeepout},
			{TEXT("Host_Neuro_Researcher"), NeuroMappingArrayHostKeepout},
			{TEXT("Hazard_ScrubberLeak"), NeuroMappingArrayHazardKeepout},
		};
		for (const FKeepout& Entry : HostsAndHazard)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Entry.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1 for keepout."), Entry.Label, Matches.Num());
			}
			const float Dist = FVector::Dist(Matches[0]->GetActorLocation(), NeuroMappingArrayLocation);
			if (Dist < Entry.MinDist)
			{
				return FString::Printf(
					TEXT("Keepout violated vs %s (dist %.1f < %.0f)."),
					Entry.Label,
					Dist,
					Entry.MinDist);
			}
		}

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
			const FVector Delta = (NeuroMappingArrayLocation - TrapLoc).GetAbs();
			if (Delta.X <= TrapExtent.X + SphereExtent.X
				&& Delta.Y <= TrapExtent.Y + SphereExtent.Y
				&& Delta.Z <= TrapExtent.Z + SphereExtent.Z)
			{
				return TEXT("Approved array location overlaps CorridorTraps_GowningRing keepout.");
			}
		}

		TArray<AActor*> GateMatches = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
		if (GateMatches.Num() != 1)
		{
			return FString::Printf(TEXT("Gate_ResearchWing count=%d expected=1 for door keepout."), GateMatches.Num());
		}
		if (FVector::Dist(GateMatches[0]->GetActorLocation(), NeuroMappingArrayLocation) < NeuroMappingArrayDoorKeepout)
		{
			return TEXT("Keepout violated vs Gate_ResearchWing (door).");
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other || ActorLabel(Other).Equals(NeuroMappingArrayLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroMappingArrayLocation) <= 5.f)
			{
				return FString::Printf(
					TEXT("Conflicting actor '%s' within 5uu of mapping-array target."),
					*ActorLabel(Other));
			}
		}
		return TEXT("");
	}

	FString NeuroMappingArray_KeepList(UWorld* World)
	{
		if (const FString Error = NeuroPowerDiagnosis_KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(
				World, TEXT("DataPad_NeuroPowerDiagnostics"), NeuroPowerDiagnosisLocation, NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list DataPad_NeuroPowerDiagnostics: %s"), *Error);
		}
		{
			TArray<AActor*> Pads = FindOwnedByExactLabel(World, TEXT("DataPad_NeuroPowerDiagnostics"), NeuroPackage);
			if (Pads.Num() != 1)
			{
				return FString::Printf(
					TEXT("Keep-list DataPad_NeuroPowerDiagnostics count=%d."), Pads.Num());
			}
			if (const FString Mismatch = NeuroPowerDiagnosis_ActorMismatch(Pads[0]); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Keep-list DataPad_NeuroPowerDiagnostics state: %s"), *Mismatch);
			}
		}
		if (const FString Error = GuardExistingActor(
				World, TEXT("CorridorTraps_GowningRing"), FVector(-1145.f, 0.f, -1060.f), NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list CorridorTraps_GowningRing: %s"), *Error);
		}
		if (const FString Error =
				GuardExistingActor(World, TEXT("Gate_ResearchWing"), FVector(2900.f, 900.f, -1060.f), EpitopePackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list Gate_ResearchWing: %s"), *Error);
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> NeuroMappingArray_ProposedComponentState()
	{
		auto MeshObj = [](const TCHAR* Name, const TCHAR* Path, const FVector& Rel, const FVector& Scale)
		{
			TSharedRef<FJsonObject> M = MakeShared<FJsonObject>();
			M->SetStringField(TEXT("component"), Name);
			M->SetStringField(TEXT("mesh"), Path);
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
			TEXT("PedestalMesh"), NeuroMappingArrayCubePath, NeuroMappingArrayPedestalRel, NeuroMappingArrayPedestalScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ColumnMesh"), NeuroMappingArrayCylinderPath, NeuroMappingArrayColumnRel, NeuroMappingArrayColumnScale)));
		Meshes.Add(MakeShared<FJsonValueObject>(MeshObj(
			TEXT("ArrayHeadMesh"), NeuroMappingArrayCylinderPath, NeuroMappingArrayHeadRel, NeuroMappingArrayHeadScale)));

		TSharedRef<FJsonObject> Actor = MakeShared<FJsonObject>();
		Actor->SetStringField(TEXT("label"), NeuroMappingArrayLabel);
		Actor->SetStringField(TEXT("class"), NeuroMappingArrayClassName);
		Actor->SetStringField(TEXT("destination_package"), NeuroPackage);
		Actor->SetArrayField(TEXT("location"), Vec(NeuroMappingArrayLocation));
		Actor->SetArrayField(TEXT("rotation"), Vec(FVector::ZeroVector));
		Actor->SetArrayField(TEXT("scale"), Vec(NeuroMappingArrayScale));
		Actor->SetNumberField(TEXT("interaction_range"), NeuroMappingArrayInteractionRange);
		Actor->SetStringField(TEXT("objective_event_id"), NeuroMappingArrayObjectiveEvent);
		Actor->SetStringField(TEXT("completed_objective_id_for_replay_guard"), NeuroMappingArrayReplayGuard);
		Actor->SetStringField(TEXT("inspection_prompt"), NeuroMappingArrayInspectPrompt);
		Actor->SetStringField(TEXT("review_prompt"), NeuroMappingArrayReviewPrompt);
		Actor->SetStringField(TEXT("speaker_label"), NeuroMappingArraySpeaker);
		Actor->SetStringField(TEXT("inspection_response_text"), NeuroMappingArrayResponse);
		Actor->SetNumberField(TEXT("notification_duration_seconds"), NeuroMappingArrayNotifySeconds);
		Actor->SetBoolField(TEXT("b_has_been_inspected"), false);
		Actor->SetArrayField(TEXT("presentation_meshes"), Meshes);
		Actor->SetStringField(
			TEXT("visual_note"),
			TEXT("PedestalMesh/ColumnMesh/ArrayHeadMesh are temporary replaceable Engine blockouts — not final art."));
		return Actor;
	}

	FString NeuroMappingArray_DestroyAndRestore(AActor* Actor, UPackage* NeuroPkg, bool bNeuroWasDirtyBefore)
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

	FString PreflightSpawnNeuroNeuralMappingArray(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. spawn_neuro_neural_mapping_array is an Unreal Editor write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_neural_mapping_array does not save or compile.");
		}
		if (const FString OverrideError = NeuroMappingArray_RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_neural_mapping_array preflight must run on the game thread.");
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			return CleanError;
		}
		if (const FString VisError = NeuroMappingArray_RequireLevelsLoadedVisible(World); !VisError.IsEmpty())
		{
			return VisError;
		}
		if (const FString MissionError = NeuroMappingArray_RequireMissionAssetPersistedClean(); !MissionError.IsEmpty())
		{
			return MissionError;
		}
		if (!LoadClass<AActor>(nullptr, NeuroMappingArrayClassPath))
		{
			return TEXT("AProjectOrganoidInspectableInstrument class is not loaded.");
		}
		if (const FString KeepError = NeuroMappingArray_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}
		if (const FString PlaceError = NeuroMappingArray_PlacementGeometry(World); !PlaceError.IsEmpty())
		{
			return PlaceError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroMappingArrayLabel);
		bool bAlreadyExact = false;
		if (Matches.Num() > 1)
		{
			return FString::Printf(
				TEXT("%s count=%d. Abort rather than stack."), NeuroMappingArrayLabel, Matches.Num());
		}
		if (Matches.Num() == 1)
		{
			const FString Mismatch = NeuroMappingArray_ActorMismatch(Matches[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("%s exists but mismatches: %s. Fail closed — no opportunistic repair."),
					NeuroMappingArrayLabel,
					*Mismatch);
			}
			bAlreadyExact = true;
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroMappingArraySpec);
		Before->SetStringField(TEXT("action"), NeuroMappingArrayAction);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("mission_asset_ready"), true);
		Before->SetStringField(TEXT("mission_object_path"), NeuroGeneticsMissionObjectPath);
		if (Matches.Num() == 1)
		{
			Before->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
		}

		Proposed->SetStringField(TEXT("spec"), NeuroMappingArraySpec);
		Proposed->SetObjectField(TEXT("actor"), NeuroMappingArray_ProposedComponentState());
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("invoke_interact"), false);
		Proposed->SetBoolField(TEXT("fire_objective_event"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyExact
				? TEXT("NeuralMappingArray_NeuroGenetics already matches neuro_neural_mapping_array_v1. Execute is a clean no-op. Requires all packages clean. Temporary Cube/Cylinder blockout — not final art.")
				: TEXT("Spawn one temporary blockout NeuralMappingArray_NeuroGenetics (InspectableInstrument + Engine Cube/Cylinder meshes) on Neuro. Requires persisted clean DA_Mission_NeuroGenetics. Does not interact, fire events, change power/Hosts/pads, save, or compile. Not final art."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroNeuralMappingArray(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("spawn_neuro_neural_mapping_array must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroNeuralMappingArray(Change.Args, Before, Proposed);
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
			return FailAudit(TEXT("packages_dirty"), FString::Printf(TEXT("ZERO writes. %s"), *CleanError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString MissionError = NeuroMappingArray_RequireMissionAssetPersistedClean(); !MissionError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("mission_required"), FString::Printf(TEXT("ZERO writes. %s"), *MissionError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroMappingArray_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("keep_list"), FString::Printf(TEXT("ZERO writes. %s"), *KeepError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PlaceError = NeuroMappingArray_PlacementGeometry(World); !PlaceError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("placement"), FString::Printf(TEXT("ZERO writes. %s"), *PlaceError), MakeShared<FBridgeChange>(Change));
		}

		ULevel* TargetLevel = FindLoadedLevelByPackage(World, NeuroPackage);
		if (!TargetLevel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
		const bool bNeuroWasDirtyBefore = NeuroPkg && NeuroPkg->IsDirty();

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroMappingArrayLabel);
		if (Matches.Num() == 1 && NeuroMappingArray_ActorMismatch(Matches[0]).IsEmpty())
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
			Change.After->SetStringField(TEXT("label"), NeuroMappingArrayLabel);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
			Change.After->SetObjectField(TEXT("proposed"), NeuroMappingArray_ProposedComponentState());
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
				TEXT("NeuralMappingArray_NeuroGenetics exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UClass* InstrumentClass = LoadClass<AActor>(nullptr, NeuroMappingArrayClassPath);
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
			const FScopedTransaction Transaction(
				NSLOCTEXT(
					"OrganoidAIBridge",
					"SpawnNeuroNeuralMappingArray",
					"Spawn Neuro Neural Mapping Array (temporary blockout)"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Spawned = World->SpawnActor<AActor>(
				InstrumentClass, NeuroMappingArrayLocation, NeuroMappingArrayRotation, Params);
			if (!Spawned)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("spawn_failed"),
					TEXT("SpawnActor returned null. ZERO remaining writes."),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(NeuroMappingArrayLabel, true);
			Spawned->SetActorScale3D(NeuroMappingArrayScale);
			if (const FString ApplyError = NeuroMappingArray_ApplyFields(Spawned); !ApplyError.IsEmpty())
			{
				const FString CleanupError = NeuroMappingArray_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
			if (const FString AfterError = NeuroMappingArray_ActorMismatch(Spawned); !AfterError.IsEmpty())
			{
				const FString CleanupError = NeuroMappingArray_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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

		if (const FString KeepError = NeuroMappingArray_KeepList(World); !KeepError.IsEmpty())
		{
			const FString CleanupError = NeuroMappingArray_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
			const FString CleanupError = NeuroMappingArray_DestroyAndRestore(Spawned, NeuroPkg, bNeuroWasDirtyBefore);
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
		Change.After->SetStringField(TEXT("label"), NeuroMappingArrayLabel);
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
		Change.After->SetObjectField(TEXT("proposed"), NeuroMappingArray_ProposedComponentState());
		Change.After->SetStringField(
			TEXT("visual_note"),
			TEXT("Temporary Engine Cube/Cylinder blockout — PedestalMesh/ColumnMesh/ArrayHeadMesh replaceable, not final art."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
