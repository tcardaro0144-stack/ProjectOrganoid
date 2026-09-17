	const TCHAR* NeuroResearchStationLabel = TEXT("ResearchStation_NeuroGenetics");
	const TCHAR* NeuroResearchStationClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidResearchStation");
	const FVector NeuroResearchStationLocation(800.f, -1600.f, -1100.f);
	const FRotator NeuroResearchStationRotation(0.f, 180.f, 0.f);
	const FVector NeuroResearchStationScale(1.f, 1.f, 1.f);
	const FVector NeuroWallEntryHallExpected(1000.f, -1575.f, -1000.f);
	const FVector NeuroCheckpointExpected(1950.f, 0.f, -1140.f);

	bool NeuroYawMatches(const FRotator& Actual, const FRotator& Expected)
	{
		return FMath::Abs(FMath::FindDeltaAngleDegrees(Actual.Yaw, Expected.Yaw)) <= RotationEps
			&& FMath::Abs(Actual.Pitch - Expected.Pitch) <= RotationEps
			&& FMath::Abs(Actual.Roll - Expected.Roll) <= RotationEps;
	}

	bool NeuroStationTransformMatches(AActor* Actor)
	{
		return Actor
			&& LocationMatches(Actor->GetActorLocation(), NeuroResearchStationLocation)
			&& NeuroYawMatches(Actor->GetActorRotation(), NeuroResearchStationRotation)
			&& ScaleMatches(Actor->GetActorScale3D(), NeuroResearchStationScale);
	}

	TArray<AActor*> FindOwnedByExactLabel(UWorld* World, const FString& Label, const FString& PackageName)
	{
		TArray<AActor*> Owned;
		for (AActor* Actor : FindByExactLabel(World, Label))
		{
			if (Actor && PackagesEqual(ActorOwningPackage(Actor), PackageName))
			{
				Owned.Add(Actor);
			}
		}
		return Owned;
	}

	AActor* FindNeuroZByLabel(UWorld* World, const FString& Label, const FVector& Expected, float ZEps = 80.f)
	{
		TArray<AActor*> Owned = FindOwnedByExactLabel(World, Label, NeuroPackage);
		for (AActor* Actor : Owned)
		{
			if (Actor && FMath::Abs(Actor->GetActorLocation().Z - Expected.Z) <= ZEps)
			{
				return Actor;
			}
		}
		return nullptr;
	}

	bool NeuroStationClassOk(AActor* Actor)
	{
		return Actor && Actor->GetClass() && Actor->GetClass()->GetPathName().Contains(TEXT("ProjectOrganoidResearchStation"));
	}

	FString NeuroStationMismatchReason(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!NeuroStationClassOk(Actor))
		{
			return FString::Printf(TEXT("class '%s' is not AProjectOrganoidResearchStation"), *ClassName(Actor));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owning package '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (!NeuroStationTransformMatches(Actor))
		{
			return TransformMismatch(Actor, NeuroResearchStationLocation, NeuroResearchStationRotation, NeuroResearchStationScale);
		}
		return TEXT("");
	}

	FString ValidateNeuroStationNeighborhood(UWorld* World)
	{
		AActor* Checkpoint = FindNeuroZByLabel(World, TEXT("Checkpoint_NeuroAirlock"), NeuroCheckpointExpected);
		if (!Checkpoint || !LocationMatches(Checkpoint->GetActorLocation(), NeuroCheckpointExpected))
		{
			return TEXT("Checkpoint_NeuroAirlock missing or moved on Neuro. Abort.");
		}
		if (FVector::Dist(Checkpoint->GetActorLocation(), NeuroResearchStationLocation) <= 280.f)
		{
			return TEXT("Approved station location overlaps Checkpoint_NeuroAirlock. Abort.");
		}

		AActor* Wall = FindNeuroZByLabel(World, TEXT("Wall_EntryHall_0"), NeuroWallEntryHallExpected);
		if (!Wall || !LocationMatches(Wall->GetActorLocation(), NeuroWallEntryHallExpected))
		{
			return TEXT("Neuro-owned Wall_EntryHall_0 at expected SE east wall is missing. Label-only lookup is unsafe. Abort.");
		}

		const TArray<TPair<const TCHAR*, float>> ClearOf = {
			{TEXT("NPC_IncineratorSurvivor"), 420.f},
			{TEXT("DataPad_EthicsObjection"), 400.f},
			{TEXT("DataPad_SpecimenBadge"), 400.f},
			{TEXT("PowerPanel_NeuroBackup"), 400.f},
		};
		for (const TPair<const TCHAR*, float>& Entry : ClearOf)
		{
			TArray<AActor*> Owned = FindOwnedByExactLabel(World, Entry.Key, NeuroPackage);
			if (Owned.Num() > 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected 0 or 1. Abort."), Entry.Key, Owned.Num());
			}
			if (Owned.Num() == 1
				&& FVector::Dist(Owned[0]->GetActorLocation(), NeuroResearchStationLocation) < Entry.Value)
			{
				return FString::Printf(TEXT("Approved station location overlaps %s. Abort."), Entry.Key);
			}
		}

		TArray<AActor*> TrapMatches = FindOwnedByExactLabel(World, TEXT("CorridorTraps_GowningRing"), NeuroPackage);
		if (TrapMatches.Num() != 1)
		{
			return FString::Printf(TEXT("CorridorTraps_GowningRing Neuro count=%d expected=1. Abort."), TrapMatches.Num());
		}
		const FVector TrapLoc = TrapMatches[0]->GetActorLocation();
		const FVector TrapExtent(1600.f, 340.f, 200.f);
		const FVector SphereExtent(200.f, 200.f, 200.f);
		const FVector Delta = (NeuroResearchStationLocation - TrapLoc).GetAbs();
		if (Delta.X <= TrapExtent.X + SphereExtent.X
			&& Delta.Y <= TrapExtent.Y + SphereExtent.Y
			&& Delta.Z <= TrapExtent.Z + SphereExtent.Z)
		{
			return TEXT("Approved station location overlaps CorridorTraps_GowningRing. Abort.");
		}

		for (const TCHAR* HostLabel : {TEXT("Host_Neuro_1"), TEXT("Host_Neuro_2"), TEXT("Host_Neuro_3")})
		{
			if (FindOwnedByExactLabel(World, HostLabel, NeuroPackage).Num() != 1)
			{
				return FString::Printf(TEXT("%s missing or not unique on Neuro. Abort."), HostLabel);
			}
		}

		TArray<AActor*> NavMatches = FindOwnedByExactLabel(World, TEXT("NavMeshBounds_NeuroGenetics"), NeuroPackage);
		if (NavMatches.Num() != 1)
		{
			return TEXT("NavMeshBounds_NeuroGenetics missing or not unique on Neuro. Abort.");
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroResearchStation(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_research_station does not save or compile.");
		}

		const FString Label = GetString(Args, TEXT("label"), NeuroResearchStationLabel);
		if (!Label.Equals(NeuroResearchStationLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("label must be ResearchStation_NeuroGenetics.");
		}

		FVector Location = NeuroResearchStationLocation;
		if (GetVector(Args, TEXT("location"), Location) && !LocationMatches(Location, NeuroResearchStationLocation))
		{
			return TEXT("location must be (800, -1600, -1100).");
		}

		FVector RotationVec = FVector(NeuroResearchStationRotation.Pitch, NeuroResearchStationRotation.Yaw, NeuroResearchStationRotation.Roll);
		if (GetVector(Args, TEXT("rotation"), RotationVec)
			&& !NeuroYawMatches(FRotator(RotationVec.X, RotationVec.Y, RotationVec.Z), NeuroResearchStationRotation))
		{
			return TEXT("rotation must be (0, 180, 0).");
		}

		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("level_package"), GetString(Args, TEXT("required_package"), NeuroPackage))));
		if (!PackagesEqual(Destination, NeuroPackage))
		{
			return TEXT("destination must be /Game/Maps/Epitope/SL_Epitope_NeuroGenetics.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, Destination))
		{
			return TEXT("SL_Epitope_NeuroGenetics is not loaded. ZERO writes.");
		}

		const FString NeighborhoodError = ValidateNeuroStationNeighborhood(World);
		if (!NeighborhoodError.IsEmpty())
		{
			return NeighborhoodError;
		}

		TArray<AActor*> Existing = FindByExactLabel(World, Label);
		TArray<AActor*> Owned = FindOwnedByExactLabel(World, Label, NeuroPackage);
		if (Existing.Num() > 1 || Owned.Num() > 1)
		{
			return FString::Printf(TEXT("ResearchStation_NeuroGenetics count=%d. Abort rather than stack."), Existing.Num());
		}
		if (Existing.Num() == 1 && Owned.Num() == 0)
		{
			return TEXT("ResearchStation_NeuroGenetics exists outside NeuroGenetics. Abort.");
		}
		if (Owned.Num() == 1)
		{
			const FString Mismatch = NeuroStationMismatchReason(Owned[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Mismatched ResearchStation_NeuroGenetics: %s. Abort."), *Mismatch);
			}
		}

		UClass* StationClass = LoadClass<AActor>(nullptr, NeuroResearchStationClassPath);
		if (!StationClass && Owned.Num() == 0)
		{
			return TEXT("AProjectOrganoidResearchStation class is not loaded.");
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("existing_station_count"), Existing.Num());
		Before->SetBoolField(TEXT("already_present"), Owned.Num() == 1);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetStringField(TEXT("class"), TEXT("ProjectOrganoidResearchStation"));
		Proposed->SetArrayField(TEXT("location"), Vec(NeuroResearchStationLocation));
		Proposed->SetArrayField(TEXT("rotation"), Vec(FVector(0.f, 180.f, 0.f)));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("already_present"), Owned.Num() == 1);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			Owned.Num() == 1
				? TEXT("Validate existing ResearchStation_NeuroGenetics. No additional spawn. Does not save.")
				: TEXT("Spawn one ResearchStation_NeuroGenetics into SL_Epitope_NeuroGenetics. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroResearchStation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_research_station must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroResearchStation(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		if (!World || !TargetLevel)
		{
			return FailAudit(TEXT("not_found"), TEXT("World or Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<AActor*> Owned = FindOwnedByExactLabel(World, NeuroResearchStationLabel, NeuroPackage);
		AActor* Station = nullptr;
		bool bSpawnedNow = false;
		if (Owned.Num() == 1 && NeuroStationMismatchReason(Owned[0]).IsEmpty())
		{
			Station = Owned[0];
		}
		else
		{
			UClass* StationClass = LoadClass<AActor>(nullptr, NeuroResearchStationClassPath);
			if (!StationClass)
			{
				return FailAudit(TEXT("class_missing"), TEXT("AProjectOrganoidResearchStation class vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}

			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnNeuroResearchStation", "Spawn Neuro Research Station"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Station = World->SpawnActor<AActor>(StationClass, NeuroResearchStationLocation, NeuroResearchStationRotation, Params);
			if (!Station)
			{
				return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor AProjectOrganoidResearchStation returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
			}
			Station->SetActorLabel(NeuroResearchStationLabel, true);
			Station->SetActorScale3D(NeuroResearchStationScale);
			Station->MarkPackageDirty();
			bSpawnedNow = true;

			const FString AfterError = NeuroStationMismatchReason(Station);
			if (!AfterError.IsEmpty())
			{
				Station->Destroy();
				return FailAudit(
					TEXT("spawn_invalid"),
					FString::Printf(TEXT("%s. Actor destroyed. ZERO remaining writes."), *AfterError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), NeuroResearchStationLabel);
		Change.After->SetStringField(TEXT("class"), ClassName(Station));
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Station));
		Change.After->SetArrayField(TEXT("location"), Vec(Station->GetActorLocation()));
		Change.After->SetArrayField(TEXT("rotation"), Vec(FVector(Station->GetActorRotation().Pitch, Station->GetActorRotation().Yaw, Station->GetActorRotation().Roll)));
		Change.After->SetBoolField(TEXT("already_present"), !bSpawnedNow);
		Change.After->SetBoolField(TEXT("spawned"), bSpawnedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
