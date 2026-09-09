	const TCHAR* NeuroNavLabel = TEXT("NavMeshBounds_NeuroGenetics");
	const FVector NeuroNavLocation(0.f, 0.f, -1000.f);
	const FVector NeuroNavBrushSize(6000.f, 6000.f, 800.f);

	FString PreflightSpawnNeuroNavMeshBounds(
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
			return TEXT("save/compile must be false. spawn_neuro_navmesh_bounds does not save or compile.");
		}

		const FString Label = GetString(Args, TEXT("label"), NeuroNavLabel);
		if (!Label.Equals(NeuroNavLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("label must be NavMeshBounds_NeuroGenetics.");
		}

		FVector Location = NeuroNavLocation;
		if (GetVector(Args, TEXT("location"), Location) && !LocationMatches(Location, NeuroNavLocation))
		{
			return TEXT("location must be (0, 0, -1000) covering the Neuro plate.");
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
		if (FindByExactLabel(World, Label).Num() != 0)
		{
			return TEXT("NavMeshBounds_NeuroGenetics already exists. Fail closed.");
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("existing_nav_bounds"), 0);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetStringField(TEXT("class"), TEXT("NavMeshBoundsVolume"));
		Proposed->SetArrayField(TEXT("location"), Vec(NeuroNavLocation));
		Proposed->SetArrayField(TEXT("brush_size"), Vec(NeuroNavBrushSize));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Spawn one NavMeshBounds_NeuroGenetics into SL_Epitope_NeuroGenetics and rebuild Recast. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroNavMeshBounds(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_navmesh_bounds must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroNavMeshBounds(Change.Args, Before, Proposed);
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

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnNeuroNavMesh", "Spawn Neuro NavMesh Bounds"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		ANavMeshBoundsVolume* Spawned = World->SpawnActor<ANavMeshBoundsVolume>(NeuroNavLocation, FRotator::ZeroRotator, Params);
		if (!Spawned)
		{
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor ANavMeshBoundsVolume returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		Spawned->SetActorLabel(NeuroNavLabel, true);
		Spawned->Tags.AddUnique(FName(NeuroNavLabel));
		UCubeBuilder* Builder = NewObject<UCubeBuilder>();
		Builder->X = NeuroNavBrushSize.X;
		Builder->Y = NeuroNavBrushSize.Y;
		Builder->Z = NeuroNavBrushSize.Z;
		UActorFactory::CreateBrushForVolumeActor(Spawned, Builder);
		Spawned->MarkPackageDirty();
		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		Spawned->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		if (BoundsExtent.X < 2000.f || BoundsExtent.Y < 2000.f || BoundsExtent.Z < 200.f)
		{
			Spawned->Destroy();
			return FailAudit(
				TEXT("nav_bounds_invalid"),
				TEXT("NavMeshBounds_NeuroGenetics brush bounds were too small. Actor destroyed. ZERO remaining writes."),
				MakeShared<FBridgeChange>(Change));
		}

		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			NavSys->Build();
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), NeuroNavLabel);
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
		Change.After->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
		Change.After->SetArrayField(TEXT("scale"), Vec(Spawned->GetActorScale3D()));
		Change.After->SetArrayField(TEXT("bounds_extent"), Vec(BoundsExtent));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
