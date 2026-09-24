	// Spawn Host_Neuro_AdaptationSubject — spawn_neuro_adaptation_subject.
	const TCHAR* NeuroAdaptationSubjectSpec = TEXT("neuro_adaptation_subject_v1");
	const TCHAR* NeuroAdaptationSubjectAction = TEXT("spawn_neuro_adaptation_subject");
	const TCHAR* NeuroAdaptationSubjectLabel = TEXT("Host_Neuro_AdaptationSubject");
	const TCHAR* NeuroAdaptationSubjectObjectiveId = TEXT("Obj_ApplyNeuralSlow");
	const TCHAR* NeuroAdaptationSubjectEventId = TEXT("Event_NeuralSlowApplied");
	const TCHAR* NeuroAdaptationSubjectAdaptationPath =
		TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");
	const TCHAR* NeuroAdaptationSubjectSpeaker = TEXT("Nathan");
	const TCHAR* NeuroAdaptationSubjectLine =
		TEXT("Neural Slow took hold. The Host is moving slower, and it did not cost a shot.");
	constexpr double NeuroAdaptationSubjectDuration = 7.0;
	const FVector NeuroAdaptationSubjectLocation(850.f, -2050.f, -1100.f);
	const FRotator NeuroAdaptationSubjectRotation(0.f, 90.f, 0.f);
	const FVector NeuroAdaptationSubjectPlayerStand(850.f, -1850.f, -1100.f);
	const FVector NeuroAdaptationSubjectWalkTarget(850.f, -1900.f, -1100.f);

	FString NeuroAdaptationSubjectReadSoft(AActor* Actor)
	{
		FProperty* Prop = FindInstanceProperty(Actor, TEXT("AdaptationCampaignRequiredAdaptation"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Actor);
			return Soft.ToSoftObjectPath().IsValid() ? Soft.ToSoftObjectPath().ToString() : TEXT("");
		}
		return TEXT("");
	}

	FString NeuroAdaptationSubjectMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Host_Neuro_AdaptationSubject missing.");
		}
		if (!Actor->GetActorLabel().Equals(NeuroAdaptationSubjectLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("label mismatch");
		}
		if (!Actor->GetActorLocation().Equals(NeuroAdaptationSubjectLocation, 1.0f))
		{
			return FString::Printf(TEXT("location %s"), *Actor->GetActorLocation().ToString());
		}
		if (!Actor->GetActorRotation().Equals(NeuroAdaptationSubjectRotation, 1.0f))
		{
			return FString::Printf(TEXT("rotation %s"), *Actor->GetActorRotation().ToString());
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("AdaptationCampaignRequiredActiveObjectiveId"), NeuroAdaptationSubjectObjectiveId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (!NeuroAdaptationSubjectReadSoft(Actor).Equals(NeuroAdaptationSubjectAdaptationPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("adaptation '%s'"), *NeuroAdaptationSubjectReadSoft(Actor));
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("AdaptationCampaignSuccessEventId"), NeuroAdaptationSubjectEventId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("AdaptationCampaignReplayGuardObjectiveId"), NeuroAdaptationSubjectObjectiveId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("AdaptationCampaignNotificationSpeaker"), NeuroAdaptationSubjectSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("AdaptationCampaignNotificationText"), NeuroAdaptationSubjectLine);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckFloat(
				Actor, TEXT("AdaptationCampaignNotificationDurationSeconds"), NeuroAdaptationSubjectDuration);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckBool(Actor, TEXT("bRequiresEncounterActivation"), true);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckBool(Actor, TEXT("bAllowPhaseShiftMutations"), false);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return TEXT("owning package is not SL_Epitope_NeuroGenetics");
		}
		return TEXT("");
	}

	FString NeuroAdaptationSubjectSurvey(UWorld* World, TSharedRef<FJsonObject> Survey)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		const FVector Start = NeuroAdaptationSubjectLocation + FVector(0.f, 0.f, 80.f);
		const FVector End = NeuroAdaptationSubjectLocation - FVector(0.f, 0.f, 250.f);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(AdaptationSubjectFloor), false);
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params) || Hit.ImpactNormal.Z < 0.7f)
		{
			return TEXT("Floor trace at the Host point is not a walkable surface near Z -1100.");
		}
		if (FMath::Abs(Hit.ImpactPoint.Z - (-1100.f)) > 40.f && FMath::Abs(Hit.ImpactPoint.Z - (-1200.f)) > 40.f)
		{
			return FString::Printf(TEXT("Floor Z %.1f is not near -1100."), Hit.ImpactPoint.Z);
		}
		Survey->SetNumberField(TEXT("floor_z"), Hit.ImpactPoint.Z);

		FCollisionShape Capsule = FCollisionShape::MakeCapsule(42.f, 96.f);
		FHitResult SweepHit;
		const bool bBlocked = World->SweepSingleByChannel(
			SweepHit,
			NeuroAdaptationSubjectLocation + FVector(0.f, 0.f, 96.f),
			NeuroAdaptationSubjectLocation + FVector(0.f, 0.f, 96.f),
			FQuat::Identity,
			ECC_WorldStatic,
			Capsule,
			Params);
		if (bBlocked && SweepHit.bBlockingHit && SweepHit.PenetrationDepth > 1.f)
		{
			return FString::Printf(TEXT("Host capsule overlaps %s."), *GetNameSafe(SweepHit.GetActor()));
		}

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSystem)
		{
			return TEXT("Navigation system unavailable. Do not rebuild NavMesh.");
		}
		FNavLocation Projected;
		if (!NavSystem->ProjectPointToNavigation(NeuroAdaptationSubjectLocation, Projected, FVector(120.f, 120.f, 200.f)))
		{
			return TEXT("Host point does not project onto existing navigation. Stop. Do not rebuild NavMesh.");
		}
		const float ProjectedXY = FVector::Dist2D(Projected.Location, NeuroAdaptationSubjectLocation);
		if (ProjectedXY > 80.f)
		{
			return FString::Printf(TEXT("Nav projection is %.1f uu from the Host point."), ProjectedXY);
		}
		FNavLocation PlayerProjected;
		FNavLocation WalkProjected;
		if (!NavSystem->ProjectPointToNavigation(NeuroAdaptationSubjectPlayerStand, PlayerProjected, FVector(120.f, 120.f, 200.f))
			|| !NavSystem->ProjectPointToNavigation(NeuroAdaptationSubjectWalkTarget, WalkProjected, FVector(120.f, 120.f, 200.f)))
		{
			return TEXT("Player stand or walk target does not project onto existing navigation.");
		}
		double PathLength = 0.0;
		const ENavigationQueryResult::Type PathResult = NavSystem->GetPathLength(
			Projected.Location, WalkProjected.Location, PathLength);
		if (PathResult != ENavigationQueryResult::Success || PathLength < 80.f)
		{
			return FString::Printf(TEXT("Local walk path unavailable or too short (result=%d length=%.1f)."), static_cast<int32>(PathResult), PathLength);
		}
		Survey->SetNumberField(TEXT("projected_xy"), ProjectedXY);
		Survey->SetNumberField(TEXT("walk_path_length"), PathLength);
		Survey->SetStringField(TEXT("player_stand"), NeuroAdaptationSubjectPlayerStand.ToString());
		Survey->SetStringField(TEXT("host_location"), NeuroAdaptationSubjectLocation.ToString());

		struct FKeep
		{
			const TCHAR* Label;
			float MinDistance;
		};
		const FKeep Keeps[] = {
			{TEXT("ResearchStation_NeuroGenetics"), 320.f},
			{TEXT("NeuralSignatureObservationNode_NeuroGenetics"), 250.f},
			{TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics"), 400.f},
			{TEXT("Host_Neuro_Researcher"), 400.f},
			{TEXT("Host_Neuro_1"), 400.f},
			{TEXT("Host_Neuro_2"), 400.f},
			{TEXT("Host_Neuro_3"), 400.f},
		};
		for (const FKeep& Keep : Keeps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Keep.Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1."), Keep.Label, Matches.Num());
			}
			const float Distance = FVector::Dist2D(Matches[0]->GetActorLocation(), NeuroAdaptationSubjectLocation);
			Survey->SetNumberField(FString::Printf(TEXT("clearance_%s"), Keep.Label), Distance);
			if (Distance < Keep.MinDistance)
			{
				return FString::Printf(TEXT("%s clearance %.1f is below %.1f."), Keep.Label, Distance, Keep.MinDistance);
			}
		}
		if (FVector::Dist2D(NeuroAdaptationSubjectPlayerStand, NeuroAdaptationSubjectLocation) > 800.f)
		{
			return TEXT("Player stand is outside Neural Slow range.");
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroAdaptationSubject(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_adaptation_subject must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before spawning the Host.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_neuro_adaptation_subject does not save.");
		}
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		ULevel* NeuroLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		if (!World || !NeuroLevel)
		{
			return TEXT("SL_Epitope_NeuroGenetics is not loaded.");
		}
		if (const FString SurveyError = NeuroAdaptationSubjectSurvey(World, Before); !SurveyError.IsEmpty())
		{
			return SurveyError;
		}

		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (const FString SourceError = ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost); !SourceError.IsEmpty())
		{
			return SourceError;
		}
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroAdaptationSubjectLabel);
		if (Matches.Num() > 1)
		{
			return FString::Printf(TEXT("%s count=%d."), NeuroAdaptationSubjectLabel, Matches.Num());
		}
		const int32 NeuroHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, NeuroPackage);
		const bool bAlready = Matches.Num() == 1 && NeuroAdaptationSubjectMismatch(Matches[0]).IsEmpty();
		if (bAlready)
		{
			if (NeuroHostCount != 5)
			{
				return FString::Printf(TEXT("Configured Host exists but Neuro Host count=%d expected=5."), NeuroHostCount);
			}
		}
		else if (Matches.Num() == 1)
		{
			return FString::Printf(TEXT("Host exists but mismatches: %s"), *NeuroAdaptationSubjectMismatch(Matches[0]));
		}
		else if (NeuroHostCount != 4)
		{
			return FString::Printf(TEXT("Neuro Host count=%d expected=4 before spawn."), NeuroHostCount);
		}

		Before->SetStringField(TEXT("spec"), NeuroAdaptationSubjectSpec);
		Before->SetBoolField(TEXT("already_exact"), bAlready);
		Proposed->SetStringField(TEXT("spec"), NeuroAdaptationSubjectSpec);
		Proposed->SetStringField(TEXT("action"), NeuroAdaptationSubjectAction);
		Proposed->SetStringField(TEXT("label"), NeuroAdaptationSubjectLabel);
		Proposed->SetStringField(TEXT("location"), NeuroAdaptationSubjectLocation.ToString());
		Proposed->SetStringField(TEXT("rotation"), NeuroAdaptationSubjectRotation.ToString());
		Proposed->SetBoolField(TEXT("already_exact"), bAlready);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlready);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetNumberField(TEXT("walk_path_length"), Before->GetNumberField(TEXT("walk_path_length")));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroAdaptationSubject(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_adaptation_subject must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroAdaptationSubject(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		TArray<AActor*> Existing = FindByExactLabel(World, NeuroAdaptationSubjectLabel);
		if (Existing.Num() == 1 && NeuroAdaptationSubjectMismatch(Existing[0]).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (!World || !TargetLevel || !ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost).IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("Neuro level or source Host class vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnNeuroAdaptationSubject", "Spawn Host_Neuro_AdaptationSubject"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AActor* Spawned = World->SpawnActor<AActor>(SourceClass, NeuroAdaptationSubjectLocation, NeuroAdaptationSubjectRotation, Params);
		if (!Spawned)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				ActorSub->DestroyActor(Spawned);
			}
			else if (World)
			{
				World->DestroyActor(Spawned);
			}
			Spawned = nullptr;
		};

		Spawned->Modify();
		Spawned->SetActorLabel(NeuroAdaptationSubjectLabel, true);
		Spawned->SetActorLocation(NeuroAdaptationSubjectLocation);
		Spawned->SetActorRotation(NeuroAdaptationSubjectRotation);

		auto ApplyString = [Spawned](const TCHAR* Name, const TCHAR* Value) -> FString
		{
			return SetNamedPropertyFromString(Spawned, Name, Value);
		};
		auto ApplyBool = [Spawned](const TCHAR* Name, bool Value) -> FString
		{
			FProperty* Prop = FindInstanceProperty(Spawned, Name);
			FString Error;
			if (!Prop || !SetPropertyFromJson(Spawned, Prop, MakeShared<FJsonValueBoolean>(Value), Error))
			{
				return Error.IsEmpty() ? FString::Printf(TEXT("%s missing"), Name) : Error;
			}
			return TEXT("");
		};
		auto ApplyFloat = [Spawned](const TCHAR* Name, double Value) -> FString
		{
			FProperty* Prop = FindInstanceProperty(Spawned, Name);
			FString Error;
			if (!Prop || !SetPropertyFromJson(Spawned, Prop, MakeShared<FJsonValueNumber>(Value), Error))
			{
				return Error.IsEmpty() ? FString::Printf(TEXT("%s missing"), Name) : Error;
			}
			return TEXT("");
		};

		FString WriteError = ApplyBool(TEXT("bRequiresEncounterActivation"), true);
		if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("ProximityActivationRange"), 200.0);
		if (WriteError.IsEmpty()) WriteError = ApplyBool(TEXT("bAllowPhaseShiftMutations"), false);
		if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("MaxHealth"), 100.0);
		if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("MeleeDamage"), 15.0);
		if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("AdaptationCampaignRequiredActiveObjectiveId"), NeuroAdaptationSubjectObjectiveId);
		if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("AdaptationCampaignSuccessEventId"), NeuroAdaptationSubjectEventId);
		if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("AdaptationCampaignReplayGuardObjectiveId"), NeuroAdaptationSubjectObjectiveId);
		if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("AdaptationCampaignNotificationSpeaker"), NeuroAdaptationSubjectSpeaker);
		if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("AdaptationCampaignNotificationText"), NeuroAdaptationSubjectLine);
		if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("AdaptationCampaignNotificationDurationSeconds"), NeuroAdaptationSubjectDuration);
		if (WriteError.IsEmpty())
		{
			FProperty* Prop = FindInstanceProperty(Spawned, TEXT("AdaptationCampaignRequiredAdaptation"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop);
			if (!SoftProp)
			{
				WriteError = TEXT("AdaptationCampaignRequiredAdaptation missing.");
			}
			else
			{
				SoftProp->SetPropertyValue_InContainer(Spawned, FSoftObjectPtr(FSoftObjectPath(NeuroAdaptationSubjectAdaptationPath)));
			}
		}
		if (!WriteError.IsEmpty() || !NeuroAdaptationSubjectMismatch(Spawned).IsEmpty())
		{
			const FString Why = WriteError.IsEmpty() ? NeuroAdaptationSubjectMismatch(Spawned) : WriteError;
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("config_failed"), FString::Printf(TEXT("%s Spawned actor destroyed. Maps not saved."), *Why), MakeShared<FBridgeChange>(Change));
		}

		Spawned->MarkPackageDirty();
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const bool bOnlyNeuroDirty = DirtyAfter.Num() == 1 && PackagesEqual(DirtyAfter[0], NeuroPackage);
		if (!bOnlyNeuroDirty)
		{
			DestroySpawned();
			Change.Status = TEXT("execute_postcondition_failed");
			return FailAudit(
				TEXT("unexpected_dirty_packages"),
				FString::Printf(TEXT("Spawn dirtied packages other than NeuroGenetics: %s. Actor destroyed. Do not save."), *FormatPackageList(DirtyAfter)),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("spawned"));
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetStringField(TEXT("location"), Spawned->GetActorLocation().ToString());
		Change.After->SetNumberField(TEXT("neuro_host_count"), CountBlock4HostClassInPackage(World, HostBaseClass, NeuroPackage));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
