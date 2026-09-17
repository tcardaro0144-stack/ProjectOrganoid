	const TCHAR* NeuroCh4HostLabel = TEXT("Host_Neuro_Researcher");
	const TCHAR* NeuroCh4StreamVolumeLabel = TEXT("StreamVolume_Region_NeuroGenetics");
	const FVector NeuroCh4HostLocation(-1200.f, 800.f, -1100.f);
	const FRotator NeuroCh4HostRotation = FRotator::ZeroRotator;
	const FVector NeuroCh4HostScale = FVector::OneVector;

	FString NeuroCh4HostMismatch(AActor* Actor, UClass* ExpectedSpawnClass, UClass* HostBaseClass, UClass* ControllerClass)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ExpectedSpawnClass || !HostBaseClass || !ControllerClass)
		{
			return TEXT("required Host classes are unavailable");
		}
		if (Actor->GetClass() != ExpectedSpawnClass || !Actor->IsA(HostBaseClass))
		{
			return FString::Printf(
				TEXT("class '%s' expected exact source class '%s' derived from ProjectOrganoidHostBase"),
				*ClassName(Actor),
				*ExpectedSpawnClass->GetName());
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, NeuroCh4HostLocation, NeuroCh4HostRotation, NeuroCh4HostScale); !Xform.IsEmpty())
		{
			return Xform;
		}
		if (UClass* LiveControllerClass = ReadOpeningBlock4ClassProperty(Actor, TEXT("AIControllerClass")); LiveControllerClass != ControllerClass)
		{
			return FString::Printf(
				TEXT("AIControllerClass '%s' expected '%s'"),
				LiveControllerClass ? *LiveControllerClass->GetPathName() : TEXT("null"),
				*ControllerClass->GetPathName());
		}
		return Block4HostPropertyMismatch(Actor);
	}

	FString GuardNeuroCh4KeepList(UWorld* World)
	{
		if (const FString Error = GuardNeuroCh3KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(World, TEXT("Host_Admin_SecurityOfficer"), FVector(2820.f, -600.f, 100.f), AdminPackage); !Error.IsEmpty())
		{
			return FString::Printf(TEXT("Admin Security Officer must stay: %s"), *Error);
		}
		for (const FNeuroCh3MeshSpec& Spec : NeuroCh3Meshes)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("Keep-list %s Neuro count=%d expected=1."), Spec.Label, Matches.Num());
			}
			FNeuroArrivalPropSpec AsArrival;
			AsArrival.Label = Spec.Label;
			AsArrival.Location = Spec.Location;
			AsArrival.Rotation = Spec.Rotation;
			AsArrival.Scale = Spec.Scale;
			AsArrival.Mesh = Spec.Mesh;
			if (const FString Mismatch = NeuroArrivalPropMismatch(Matches[0], AsArrival); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Keep-list %s: %s"), Spec.Label, *Mismatch);
			}
		}
		for (const FNeuroCh3PadSpec& Spec : NeuroCh3Pads)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("Keep-list %s Neuro count=%d expected=1."), Spec.Label, Matches.Num());
			}
			if (const FString Mismatch = NeuroCh3PadMismatch(Matches[0], Spec); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Keep-list %s: %s"), Spec.Label, *Mismatch);
			}
		}
		return TEXT("");
	}

	FString GuardNeuroCh4PackagesLoadedAndClean(UWorld*& OutWorld)
	{
		OutWorld = nullptr;
		const FString WorldError = RequireEpitopeEditorWorld(OutWorld);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}
		if (!FindLoadedLevelByPackage(OutWorld, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded. ZERO writes.");
		}
		if (!FindLoadedLevelByPackage(OutWorld, NeuroPackage))
		{
			return TEXT("SL_Epitope_NeuroGenetics must be loaded. ZERO writes.");
		}

		UPackage* EpitopePkg = FindPackageByName(EpitopePackage);
		UPackage* AdminPkg = FindPackageByName(AdminPackage);
		UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
		if (!EpitopePkg)
		{
			return TEXT("Lvl_Epitope package is not loaded in memory.");
		}
		if (!AdminPkg)
		{
			return TEXT("SL_Epitope_Admin package is not loaded in memory.");
		}
		if (!NeuroPkg)
		{
			return TEXT("SL_Epitope_NeuroGenetics package is not loaded in memory.");
		}
		if (EpitopePkg->IsDirty())
		{
			return TEXT("Lvl_Epitope is dirty. Refusing Neuro Ch4 spawn. ZERO writes.");
		}
		if (AdminPkg->IsDirty())
		{
			return TEXT("SL_Epitope_Admin is dirty. Refusing Neuro Ch4 spawn. ZERO writes.");
		}
		if (NeuroPkg->IsDirty())
		{
			return TEXT("SL_Epitope_NeuroGenetics is dirty. Refusing Neuro Ch4 spawn. ZERO writes.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Dirty packages must be empty before Neuro Ch4 spawn. Dirty: %s. ZERO writes."),
				*FormatPackageList(Dirty));
		}
		return TEXT("");
	}

	bool IsAllowedNeuroCh4CapsuleOverlap(const FOverlapResult& Overlap)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor)
		{
			return true;
		}
		const FString Label = ActorLabel(OverlapActor);
		if (Label.Equals(TEXT("NeuroGenetics_FloorPlate"), ESearchCase::CaseSensitive)
			&& PackagesEqual(ActorOwningPackage(OverlapActor), NeuroPackage))
		{
			return true;
		}
		if (Label.Equals(NeuroCh4StreamVolumeLabel, ESearchCase::CaseSensitive) && !Overlap.bBlockingHit)
		{
			return true;
		}
		if (Label.Equals(NeuroCh4HostLabel, ESearchCase::CaseSensitive)
			&& PackagesEqual(ActorOwningPackage(OverlapActor), NeuroPackage))
		{
			return true;
		}
		return false;
	}

	FString PreflightSpawnNeuroCh4TransformedPersonnel(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_ch4_transformed_personnel does not save or compile.");
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			return CleanError;
		}
		if (const FString KeepError = GuardNeuroCh4KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		FCollisionQueryParams CollisionParams(FName(TEXT("NeuroCh4HostPreflight")), false);
		TArray<FOverlapResult> CapsuleOverlaps;
		World->OverlapMultiByProfile(
			CapsuleOverlaps,
			NeuroCh4HostLocation,
			NeuroCh4HostRotation.Quaternion(),
			FName(TEXT("Pawn")),
			FCollisionShape::MakeCapsule(42.0f, 96.0f),
			CollisionParams);
		for (const FOverlapResult& Overlap : CapsuleOverlaps)
		{
			if (!IsAllowedNeuroCh4CapsuleOverlap(Overlap))
			{
				AActor* OverlapActor = Overlap.GetActor();
				return FString::Printf(
					TEXT("Approved Host capsule unexpectedly overlaps '%s' in '%s'. ZERO writes."),
					OverlapActor ? *ActorLabel(OverlapActor) : TEXT("null"),
					OverlapActor ? *ActorOwningPackage(OverlapActor) : TEXT(""));
			}
		}

		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		FNavLocation ProjectedHostLocation;
		if (!NavSystem || !NavSystem->ProjectPointToNavigation(
			NeuroCh4HostLocation,
			ProjectedHostLocation,
			FVector(200.0f, 200.0f, 300.0f)))
		{
			return TEXT("Approved Host location is not projectable onto Neuro navigation. ZERO writes.");
		}
		const float ProjectedXYDelta = FVector::Dist2D(NeuroCh4HostLocation, ProjectedHostLocation.Location);
		const float ProjectedZDelta = FMath::Abs(NeuroCh4HostLocation.Z - ProjectedHostLocation.Location.Z);
		if (ProjectedXYDelta > 75.0f || ProjectedZDelta > 150.0f)
		{
			return FString::Printf(
				TEXT("Nearest Neuro navigation is too far from the approved Host location (xy=%.1f z=%.1f). ZERO writes."),
				ProjectedXYDelta,
				ProjectedZDelta);
		}

		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (const FString SourceError = ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost); !SourceError.IsEmpty())
		{
			return SourceError;
		}

		TArray<AActor*> LabelMatches = FindByExactLabel(World, NeuroCh4HostLabel);
		const int32 NeuroHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, NeuroPackage);
		const int32 AdminHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, AdminPackage);
		if (LabelMatches.Num() > 1)
		{
			return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), NeuroCh4HostLabel, LabelMatches.Num());
		}
		if (AdminHostCount != 1)
		{
			return FString::Printf(TEXT("Admin Host count=%d expected=1. Do not move Admin Security Officer."), AdminHostCount);
		}
		if (NeuroHostCount < 3 || NeuroHostCount > 4)
		{
			return FString::Printf(TEXT("Neuro Host count=%d expected 3 (system) or 4 (system+authored). Abort."), NeuroHostCount);
		}
		if (NeuroHostCount == 4 && LabelMatches.Num() != 1)
		{
			return TEXT("Neuro already has 4 Hosts but Host_Neuro_Researcher is missing. Abort rather than guess.");
		}
		if (NeuroHostCount == 3 && LabelMatches.Num() != 0)
		{
			return TEXT("Host_Neuro_Researcher exists but Neuro Host count is still 3. Abort.");
		}

		bool bAlreadyPresent = false;
		if (LabelMatches.Num() == 1)
		{
			if (const FString Mismatch = NeuroCh4HostMismatch(LabelMatches[0], SourceClass, HostBaseClass, ControllerClass); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Existing %s is mismatched: %s"), NeuroCh4HostLabel, *Mismatch);
			}
			bAlreadyPresent = true;
			Before->SetObjectField(TEXT("existing_actor"), ActorSnapshot(LabelMatches[0]));
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("neuro_host_count"), NeuroHostCount);
		Before->SetNumberField(TEXT("admin_host_count"), AdminHostCount);
		Before->SetBoolField(TEXT("already_present"), bAlreadyPresent);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetObjectField(TEXT("source_host"), ActorSnapshot(SourceHost));
		Before->SetStringField(TEXT("source_class"), SourceClass->GetPathName());
		Proposed->SetStringField(TEXT("label"), NeuroCh4HostLabel);
		Proposed->SetStringField(TEXT("class"), SourceClass->GetPathName());
		Proposed->SetArrayField(TEXT("location"), Vec(NeuroCh4HostLocation));
		Proposed->SetBoolField(TEXT("bRequiresEncounterActivation"), true);
		Proposed->SetNumberField(TEXT("ProximityActivationRange"), 200.0);
		Proposed->SetBoolField(TEXT("bAllowPhaseShiftMutations"), false);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyPresent
				? TEXT("Authorized Host_Neuro_Researcher already matches. Execute is a no-op and does not save. Requires all packages clean.")
				: TEXT("Spawn one dormant Host_Neuro_Researcher on Neuro west of Host_Neuro_3. Same Block 4 chassis/props. Requires clean start; after spawn only Neuro may be dirty. Does not save. Does not change power."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroCh4TransformedPersonnel(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_ch4_transformed_personnel must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroCh4TransformedPersonnel(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("packages_dirty"), FString::Printf(TEXT("ZERO writes. %s"), *CleanError), MakeShared<FBridgeChange>(Change));
		}

		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (const FString SourceError = ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost); !SourceError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("source_changed"), FString::Printf(TEXT("ZERO writes. %s"), *SourceError), MakeShared<FBridgeChange>(Change));
		}
		(void)SourceHost;

		TArray<AActor*> Existing = FindByExactLabel(World, NeuroCh4HostLabel);
		if (Existing.Num() == 1)
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.bExecuted = true;
				Change.ExecutedAt = NowIso();
				Change.bSavePerformed = false;
				Change.Status = TEXT("execute_postcondition_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetBoolField(TEXT("already_present"), true);
				Change.After->SetBoolField(TEXT("spawned"), false);
				Change.After->SetBoolField(TEXT("noop"), true);
				Change.After->SetBoolField(TEXT("save_performed"), false);
				Change.After->SetBoolField(TEXT("power_changed"), false);
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
				Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Existing[0]));
				LogAudit(TEXT("execute"), Change);
				return FailAudit(
					TEXT("unexpected_dirty_packages"),
					FString::Printf(
						TEXT("No-op Host_Neuro_Researcher path found dirty packages after zero mutation: %s. Hard stop. Do not save any map. Close/discard if unexpected dirt remains. Maps not saved."),
						*FormatPackageList(DirtyAfter)),
					MakeShared<FBridgeChange>(Change));
			}

			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("already_present"), true);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("noop"), true);
			Change.After->SetBoolField(TEXT("mutation"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("power_changed"), false);
			Change.After->SetBoolField(TEXT("packages_clean"), true);
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Existing[0]));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		if (!World || !TargetLevel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("World or Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "NeuroCh4Host", "Neuro Ch4 transformed personnel"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AActor* Spawned = World->SpawnActor<AActor>(SourceClass, NeuroCh4HostLocation, NeuroCh4HostRotation, Params);
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

		Spawned->Modify();
		Spawned->SetActorLabel(NeuroCh4HostLabel, true);
		Spawned->SetActorScale3D(NeuroCh4HostScale);

		const TSharedRef<FJsonObject> Required = Block4RequiredHostProperties();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			FProperty* Property = FindInstanceProperty(Spawned, Pair.Key);
			FString SetError;
			if (!SetPropertyFromJson(Spawned, Property, Pair.Value, SetError)
				|| !PropertyMatchesJson(Spawned, Property, Pair.Value, SetError))
			{
				DestroySpawned();
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("config_failed"),
					FString::Printf(TEXT("%s Spawned actor destroyed. Maps not saved."), *SetError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		FString VerifyError = NeuroCh4HostMismatch(Spawned, SourceClass, HostBaseClass, ControllerClass);
		if (VerifyError.IsEmpty())
		{
			const int32 NeuroHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, NeuroPackage);
			const int32 LabelCount = FindByExactLabel(World, NeuroCh4HostLabel).Num();
			const int32 AdminHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, AdminPackage);
			if (NeuroHostCount != 4 || LabelCount != 1 || AdminHostCount != 1)
			{
				VerifyError = FString::Printf(
					TEXT("post-spawn Neuro Hosts=%d label=%d Admin=%d expected 4/1/1"),
					NeuroHostCount,
					LabelCount,
					AdminHostCount);
			}
		}
		if (VerifyError.IsEmpty())
		{
			VerifyError = GuardNeuroCh4KeepList(World);
		}
		if (!VerifyError.IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Spawned actor destroyed. Maps not saved."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		Spawned->MarkPackageDirty();
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const bool bOnlyNeuroDirty = DirtyAfter.Num() == 1 && PackagesEqual(DirtyAfter[0], NeuroPackage);
		if (!bOnlyNeuroDirty)
		{
			DestroySpawned();
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("execute_postcondition_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("already_present"), false);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("spawned_then_destroyed"), true);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("power_changed"), false);
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetStringField(
				TEXT("note"),
				TEXT("Actor destruction does not clear package dirty flags. Do not save any map. Close/discard if unexpected dirt remains."));
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("unexpected_dirty_packages"),
				FString::Printf(
					TEXT("Neuro Ch4 spawn dirtied packages other than NeuroGenetics alone: %s. Spawned actor destroyed. Hard stop. Do not save any map. Close/discard if unexpected dirt remains. Maps not saved."),
					*FormatPackageList(DirtyAfter)),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("already_present"), false);
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetNumberField(TEXT("neuro_host_count"), CountBlock4HostClassInPackage(World, HostBaseClass, NeuroPackage));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Spawned));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
