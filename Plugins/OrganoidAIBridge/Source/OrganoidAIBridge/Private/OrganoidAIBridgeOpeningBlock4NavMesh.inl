	const TCHAR* AdminBlock4NavAction = TEXT("spawn_admin_block4_navmesh_bounds");
	const TCHAR* AdminBlock4NavSaveAction = TEXT("save_admin_block4_navmesh_prerequisite");
	const TCHAR* AdminBlock4NavSpec = TEXT("opening_block4_admin_navmesh_v1");
	const TCHAR* AdminBlock4NavLabel = TEXT("NavMeshBounds_Admin_Security");
	const TCHAR* AdminBlock4HostLabel = TEXT("Host_Admin_SecurityOfficer");
	const FVector AdminBlock4NavLocation(2595.f, -450.f, 100.f);
	const FVector AdminBlock4NavBrushSize(570.f, 800.f, 400.f);
	const FVector AdminBlock4NavHostPoint(2820.f, -600.f, 100.f);
	const FVector AdminBlock4NavClosePoint(2820.f, -420.f, 100.f);
	const FVector AdminBlock4NavAmmoPoint(2560.f, -340.f, 100.f);
	const FVector AdminBlock4NavProjectExtent(200.f, 200.f, 300.f);
	constexpr float AdminBlock4NavMaxXYDelta = 75.f;
	constexpr float AdminBlock4NavMaxZDelta = 150.f;
	constexpr float AdminBlock4NavBrushEps = 1.f;
	// Admin nav only: UE can snap NavMeshBoundsVolume by a few uu after brush create.
	// Host Block 4 spawn continues to use global LocationEps (0.51f).
	constexpr float AdminBlock4NavLocationEps = 10.0f;

	TArray<FString> CollectDirtyPackageNamesSorted()
	{
		TArray<FString> Names;
		TArray<UPackage*> DirtyWorld;
		TArray<UPackage*> DirtyContent;
		FEditorFileUtils::GetDirtyWorldPackages(DirtyWorld);
		FEditorFileUtils::GetDirtyContentPackages(DirtyContent);
		auto Append = [&Names](const TArray<UPackage*>& Packages)
		{
			for (UPackage* Package : Packages)
			{
				if (Package)
				{
					Names.AddUnique(NormalizePackage(Package->GetName()));
				}
			}
		};
		Append(DirtyWorld);
		Append(DirtyContent);
		Names.Sort();
		return Names;
	}

	void RestorePackageCleanIfWasClean(UPackage* Package, bool bWasDirtyBefore, TArray<FString>& OutRestored)
	{
		if (!Package || bWasDirtyBefore || !Package->IsDirty())
		{
			return;
		}
		Package->SetDirtyFlag(false);
		OutRestored.AddUnique(NormalizePackage(Package->GetName()));
	}

	TArray<TSharedPtr<FJsonValue>> DirtyPackageJsonArray(const TArray<FString>& Names)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FString& Name : Names)
		{
			Out.Add(MakeShared<FJsonValueString>(Name));
		}
		return Out;
	}

	FString FormatPackageList(const TArray<FString>& Names)
	{
		if (Names.Num() == 0)
		{
			return TEXT("(none)");
		}
		return FString::Join(Names, TEXT(", "));
	}

	FString RequireAdminBlock4NavMapsLoadedAndClean(UWorld*& OutWorld)
	{
		if (!IsInGameThread())
		{
			return TEXT("Admin Block 4 nav must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before Admin Block 4 nav.");
		}
		const FString WorldError = RequireEpitopeEditorWorld(OutWorld);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}
		if (!FindLoadedLevelByPackage(OutWorld, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded. ZERO writes.");
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
			return TEXT("Lvl_Epitope is dirty. Close without save or clear unrelated edits first. ZERO writes.");
		}
		if (AdminPkg->IsDirty())
		{
			return TEXT("SL_Epitope_Admin is dirty. Close without save or clear unrelated edits first. ZERO writes.");
		}
		if (NeuroPkg->IsDirty())
		{
			return TEXT("SL_Epitope_NeuroGenetics is dirty. Close without save. Do not save Neuro Recast dirt. ZERO writes.");
		}
		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Unexpected dirty packages before Admin Block 4 nav: %s. Close without save. ZERO writes."),
				*FormatPackageList(Dirty));
		}
		return TEXT("");
	}

	bool AdminBlock4NavSizeMatchesHalfOrFull(const FVector& Measured, const FVector& ExpectedFull)
	{
		const FVector ExpectedHalf = ExpectedFull * 0.5f;
		return VecMatches(Measured, ExpectedHalf, AdminBlock4NavBrushEps)
			|| VecMatches(Measured, ExpectedFull, AdminBlock4NavBrushEps);
	}

	FString AdminBlock4NavBrushMatchFailure(ANavMeshBoundsVolume* Volume, FVector& OutExtent)
	{
		OutExtent = FVector::ZeroVector;
		if (!Volume)
		{
			return TEXT("volume is null.");
		}

		const FVector ExpectedFull = AdminBlock4NavBrushSize;
		const FVector ExpectedHalf = ExpectedFull * 0.5f;
		FString BuilderClass = TEXT("(none)");
		FVector CubeSize(0.f, 0.f, 0.f);
		bool bHasCube = false;
		bool bCubeOk = false;
		if (Volume->BrushBuilder)
		{
			BuilderClass = Volume->BrushBuilder->GetClass()->GetName();
			if (const UCubeBuilder* Cube = Cast<UCubeBuilder>(Volume->BrushBuilder))
			{
				bHasCube = true;
				CubeSize = FVector(Cube->X, Cube->Y, Cube->Z);
				bCubeOk = AdminBlock4NavSizeMatchesHalfOrFull(CubeSize, ExpectedFull);
			}
		}
		else
		{
			// CreateBrushForVolumeActor may leave BrushBuilder unset; accept bounds-only in that case.
			bCubeOk = true;
		}

		FVector Origin = FVector::ZeroVector;
		Volume->GetActorBounds(false, Origin, OutExtent);
		const bool bExtentOk = AdminBlock4NavSizeMatchesHalfOrFull(OutExtent, ExpectedFull);
		const FVector Loc = Volume->GetActorLocation();
		const FString Package = ActorOwningPackage(Volume);
		const FString Label = ActorLabel(Volume);
		const bool bLocOk = VecMatches(Loc, AdminBlock4NavLocation, AdminBlock4NavLocationEps);
		const bool bPkgOk = PackagesEqual(Package, AdminPackage);
		const bool bLabelOk = Label.Equals(AdminBlock4NavLabel, ESearchCase::CaseSensitive);

		if (bCubeOk && bExtentOk && bLocOk && bPkgOk && bLabelOk)
		{
			return TEXT("");
		}

		return FString::Printf(
			TEXT("measured builder=%s cube=(%.3f,%.3f,%.3f) has_cube=%d cube_ok=%d extent=(%.3f,%.3f,%.3f) extent_ok=%d expected_full=(%.3f,%.3f,%.3f) expected_half=(%.3f,%.3f,%.3f) loc=(%.3f,%.3f,%.3f) loc_ok=%d package=%s pkg_ok=%d label=%s label_ok=%d."),
			*BuilderClass,
			CubeSize.X,
			CubeSize.Y,
			CubeSize.Z,
			bHasCube ? 1 : 0,
			bCubeOk ? 1 : 0,
			OutExtent.X,
			OutExtent.Y,
			OutExtent.Z,
			bExtentOk ? 1 : 0,
			ExpectedFull.X,
			ExpectedFull.Y,
			ExpectedFull.Z,
			ExpectedHalf.X,
			ExpectedHalf.Y,
			ExpectedHalf.Z,
			Loc.X,
			Loc.Y,
			Loc.Z,
			bLocOk ? 1 : 0,
			*Package,
			bPkgOk ? 1 : 0,
			*Label,
			bLabelOk ? 1 : 0);
	}

	bool AdminBlock4NavBrushMatches(ANavMeshBoundsVolume* Volume, FVector& OutExtent)
	{
		return AdminBlock4NavBrushMatchFailure(Volume, OutExtent).IsEmpty();
	}

	TSharedRef<FJsonObject> AdminBlock4NavProjectionRow(const TCHAR* Name, const FVector& Point, UNavigationSystemV1* NavSys)
	{
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("name"), Name);
		Row->SetArrayField(TEXT("query"), Vec(Point));
		FNavLocation Projected;
		const bool bHasNav = NavSys && NavSys->ProjectPointToNavigation(Point, Projected, AdminBlock4NavProjectExtent);
		Row->SetBoolField(TEXT("projected"), bHasNav);
		if (!bHasNav)
		{
			Row->SetField(TEXT("projected_location"), MakeShared<FJsonValueNull>());
			Row->SetField(TEXT("xy_delta"), MakeShared<FJsonValueNull>());
			Row->SetField(TEXT("z_delta"), MakeShared<FJsonValueNull>());
			Row->SetBoolField(TEXT("within_tolerance"), false);
			return Row;
		}
		const float XYDelta = FVector::Dist2D(Point, Projected.Location);
		const float ZDelta = FMath::Abs(Point.Z - Projected.Location.Z);
		Row->SetArrayField(TEXT("projected_location"), Vec(Projected.Location));
		Row->SetNumberField(TEXT("xy_delta"), XYDelta);
		Row->SetNumberField(TEXT("z_delta"), ZDelta);
		Row->SetBoolField(
			TEXT("within_tolerance"),
			XYDelta <= AdminBlock4NavMaxXYDelta && ZDelta <= AdminBlock4NavMaxZDelta);
		return Row;
	}

	FString AdminBlock4NavProjectionFailure(UNavigationSystemV1* NavSys)
	{
		if (!NavSys)
		{
			return TEXT("Navigation system is unavailable.");
		}
		const TCHAR* PointNames[] = {TEXT("host"), TEXT("close"), TEXT("ammo")};
		const FVector PointValues[] = {AdminBlock4NavHostPoint, AdminBlock4NavClosePoint, AdminBlock4NavAmmoPoint};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FNavLocation Projected;
			const bool bHasNav = NavSys->ProjectPointToNavigation(PointValues[Index], Projected, AdminBlock4NavProjectExtent);
			if (!bHasNav)
			{
				return FString::Printf(TEXT("%s point is not projectable onto Admin navigation."), PointNames[Index]);
			}
			const float XYDelta = FVector::Dist2D(PointValues[Index], Projected.Location);
			const float ZDelta = FMath::Abs(PointValues[Index].Z - Projected.Location.Z);
			if (XYDelta > AdminBlock4NavMaxXYDelta || ZDelta > AdminBlock4NavMaxZDelta)
			{
				return FString::Printf(
					TEXT("%s projection outside Block 4 tolerances (xy=%.1f z=%.1f; max xy=%.0f z=%.0f)."),
					PointNames[Index],
					XYDelta,
					ZDelta,
					AdminBlock4NavMaxXYDelta,
					AdminBlock4NavMaxZDelta);
			}
		}
		return TEXT("");
	}

	TSharedRef<FJsonObject> SnapshotAdminBlock4NavVolume(ANavMeshBoundsVolume* Volume)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Volume)
		{
			Out->SetBoolField(TEXT("present"), false);
			return Out;
		}
		FVector Extent = FVector::ZeroVector;
		const FString MatchFailure = AdminBlock4NavBrushMatchFailure(Volume, Extent);
		const bool bBrushOk = MatchFailure.IsEmpty();
		Out->SetBoolField(TEXT("present"), true);
		Out->SetStringField(TEXT("label"), ActorLabel(Volume));
		Out->SetStringField(TEXT("class"), ClassName(Volume));
		Out->SetStringField(TEXT("owning_package"), ActorOwningPackage(Volume));
		Out->SetArrayField(TEXT("location"), Vec(Volume->GetActorLocation()));
		Out->SetArrayField(TEXT("brush_size"), Vec(AdminBlock4NavBrushSize));
		Out->SetArrayField(TEXT("expected_full"), Vec(AdminBlock4NavBrushSize));
		Out->SetArrayField(TEXT("expected_half"), Vec(AdminBlock4NavBrushSize * 0.5f));
		if (Volume->BrushBuilder)
		{
			Out->SetStringField(TEXT("brush_builder_class"), Volume->BrushBuilder->GetClass()->GetName());
		}
		else
		{
			Out->SetField(TEXT("brush_builder_class"), MakeShared<FJsonValueNull>());
		}
		if (const UCubeBuilder* Cube = Cast<UCubeBuilder>(Volume->BrushBuilder))
		{
			Out->SetArrayField(TEXT("cube_builder"), Vec(FVector(Cube->X, Cube->Y, Cube->Z)));
		}
		else
		{
			Out->SetField(TEXT("cube_builder"), MakeShared<FJsonValueNull>());
		}
		Out->SetArrayField(TEXT("bounds_extent"), Vec(Extent));
		Out->SetBoolField(TEXT("identity_ok"), bBrushOk);
		if (bBrushOk)
		{
			Out->SetField(TEXT("match_failure"), MakeShared<FJsonValueNull>());
		}
		else
		{
			Out->SetStringField(TEXT("match_failure"), MatchFailure);
		}
		return Out;
	}

	TSharedRef<FJsonObject> SnapshotAdminBlock4NavData(UWorld* World)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		Out->SetBoolField(TEXT("nav_system_present"), NavSys != nullptr);
		ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
		if (!NavData)
		{
			Out->SetField(TEXT("nav_data_label"), MakeShared<FJsonValueNull>());
			Out->SetField(TEXT("nav_data_class"), MakeShared<FJsonValueNull>());
			Out->SetField(TEXT("nav_data_package"), MakeShared<FJsonValueNull>());
			return Out;
		}
		Out->SetStringField(TEXT("nav_data_label"), ActorLabel(NavData));
		Out->SetStringField(TEXT("nav_data_class"), ClassName(NavData));
		Out->SetStringField(TEXT("nav_data_package"), ActorOwningPackage(NavData));
		return Out;
	}

	FString GuardNoAdminBlock4Host(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (FindByExactLabel(World, AdminBlock4HostLabel).Num() != 0)
		{
			return TEXT("Host_Admin_SecurityOfficer already exists. Admin nav prerequisite must complete before Host spawn. ZERO writes.");
		}
		return TEXT("");
	}

	FString PreflightSpawnAdminBlock4NavMeshBounds(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false)
			|| GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("save_dirty"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/save_all/save_dirty/compile must be false. spawn_admin_block4_navmesh_bounds does not save or compile.");
		}
		if (!GetBool(Args, TEXT("require_pie_stopped"), true))
		{
			return TEXT("require_pie_stopped must be true.");
		}
		if (!GetString(Args, TEXT("spec")).Equals(AdminBlock4NavSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be exactly '%s'."), AdminBlock4NavSpec);
		}
		const FString Label = GetString(Args, TEXT("label"), AdminBlock4NavLabel);
		if (!Label.Equals(AdminBlock4NavLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("label must be NavMeshBounds_Admin_Security.");
		}
		FVector Location = AdminBlock4NavLocation;
		if (GetVector(Args, TEXT("location"), Location) && !LocationMatches(Location, AdminBlock4NavLocation))
		{
			return TEXT("location must be (2595, -450, 100).");
		}
		FVector Brush = AdminBlock4NavBrushSize;
		if (GetVector(Args, TEXT("brush_size"), Brush) && !VecMatches(Brush, AdminBlock4NavBrushSize, AdminBlock4NavBrushEps))
		{
			return TEXT("brush_size must be (570, 800, 400).");
		}
		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("required_package"), AdminPackage)));
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("destination/required_package must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}

		UWorld* World = nullptr;
		if (const FString Guard = RequireAdminBlock4NavMapsLoadedAndClean(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		if (const FString HostGuard = GuardNoAdminBlock4Host(World); !HostGuard.IsEmpty())
		{
			return HostGuard;
		}
		if (FindByExactLabel(World, AdminBlock4NavLabel).Num() != 0)
		{
			return TEXT("NavMeshBounds_Admin_Security already exists. Fail closed.");
		}

		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("destination_loaded"), true);
		Before->SetBoolField(TEXT("destination_dirty"), false);
		Before->SetBoolField(TEXT("epitope_dirty"), false);
		Before->SetBoolField(TEXT("neuro_dirty"), false);
		Before->SetBoolField(TEXT("pie_running"), false);
		Before->SetBoolField(TEXT("on_game_thread"), true);
		Before->SetNumberField(TEXT("existing_nav_bounds"), 0);
		Before->SetNumberField(TEXT("admin_host_count"), 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));

		Proposed->SetStringField(TEXT("spec"), AdminBlock4NavSpec);
		Proposed->SetStringField(TEXT("action"), AdminBlock4NavAction);
		Proposed->SetStringField(TEXT("label"), AdminBlock4NavLabel);
		Proposed->SetStringField(TEXT("class"), TEXT("NavMeshBoundsVolume"));
		Proposed->SetStringField(TEXT("destination_package"), FString(AdminPackage));
		Proposed->SetArrayField(TEXT("location"), Vec(AdminBlock4NavLocation));
		Proposed->SetArrayField(TEXT("brush_size"), Vec(AdminBlock4NavBrushSize));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Spawn one NavMeshBounds_Admin_Security into SL_Epitope_Admin, rebuild Recast, verify Host/close/ammo projections. Does not save. Any dirty package outside Admin is a hard stop."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminBlock4NavMeshBounds(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_admin_block4_navmesh_bounds must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnAdminBlock4NavMeshBounds(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, AdminPackage) : nullptr;
		if (!World || !TargetLevel)
		{
			return FailAudit(TEXT("not_found"), TEXT("World or Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnAdminBlock4NavMesh", "Spawn Admin Block 4 NavMesh Bounds"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		ANavMeshBoundsVolume* Spawned = World->SpawnActor<ANavMeshBoundsVolume>(AdminBlock4NavLocation, FRotator::ZeroRotator, Params);
		if (!Spawned)
		{
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor ANavMeshBoundsVolume returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		auto DestroySpawned = [Spawned]()
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		};

		Spawned->SetActorLabel(AdminBlock4NavLabel, true);
		Spawned->Tags.AddUnique(FName(AdminBlock4NavLabel));
		UCubeBuilder* Builder = NewObject<UCubeBuilder>();
		Builder->X = AdminBlock4NavBrushSize.X;
		Builder->Y = AdminBlock4NavBrushSize.Y;
		Builder->Z = AdminBlock4NavBrushSize.Z;
		UActorFactory::CreateBrushForVolumeActor(Spawned, Builder);
		Spawned->SetActorLocation(AdminBlock4NavLocation, false, nullptr, ETeleportType::TeleportPhysics);
		Spawned->MarkPackageDirty();

		FVector BoundsExtent = FVector::ZeroVector;
		if (const FString BrushError = AdminBlock4NavBrushMatchFailure(Spawned, BoundsExtent); !BrushError.IsEmpty())
		{
			DestroySpawned();
			return FailAudit(
				TEXT("nav_bounds_invalid"),
				FString::Printf(
					TEXT("NavMeshBounds_Admin_Security identity/brush verification failed. %s Actor destroyed. ZERO remaining writes."),
					*BrushError),
				MakeShared<FBridgeChange>(Change));
		}

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSys)
		{
			DestroySpawned();
			return FailAudit(
				TEXT("nav_system_missing"),
				TEXT("Navigation system missing after Admin nav spawn. Actor destroyed. ZERO remaining writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UPackage* EpitopePkgForBuild = FindPackageByName(EpitopePackage);
		UPackage* NeuroPkgForBuild = FindPackageByName(NeuroPackage);
		const bool bEpitopeDirtyBeforeBuild = EpitopePkgForBuild && EpitopePkgForBuild->IsDirty();
		const bool bNeuroDirtyBeforeBuild = NeuroPkgForBuild && NeuroPkgForBuild->IsDirty();
		const TArray<FString> PreBuildDirty = CollectDirtyPackageNamesSorted();
		NavSys->Build();
		const TArray<FString> PostBuildDirty = CollectDirtyPackageNamesSorted();
		TArray<FString> RestoredDirty;
		// RecastNavMesh lives on Lvl_Epitope and may dirty Neuro; clear only packages that were clean pre-Build.
		RestorePackageCleanIfWasClean(EpitopePkgForBuild, bEpitopeDirtyBeforeBuild, RestoredDirty);
		RestorePackageCleanIfWasClean(NeuroPkgForBuild, bNeuroDirtyBeforeBuild, RestoredDirty);
		RestoredDirty.Sort();
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString RecastDirtyAudit = FString::Printf(
			TEXT("pre_build_dirty: [%s], post_build_dirty: [%s], restored: [%s]"),
			*FormatPackageList(PreBuildDirty),
			*FormatPackageList(PostBuildDirty),
			*FormatPackageList(RestoredDirty));

		TSharedRef<FJsonObject> After = MakeShared<FJsonObject>();
		After->SetStringField(TEXT("label"), AdminBlock4NavLabel);
		After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
		After->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
		After->SetArrayField(TEXT("brush_size"), Vec(AdminBlock4NavBrushSize));
		After->SetArrayField(TEXT("bounds_extent"), Vec(BoundsExtent));
		After->SetBoolField(TEXT("save_performed"), false);
		After->SetArrayField(TEXT("pre_build_dirty"), DirtyPackageJsonArray(PreBuildDirty));
		After->SetArrayField(TEXT("post_build_dirty"), DirtyPackageJsonArray(PostBuildDirty));
		After->SetArrayField(TEXT("restored"), DirtyPackageJsonArray(RestoredDirty));
		After->SetStringField(TEXT("recast_dirty_audit"), RecastDirtyAudit);
		After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		After->SetObjectField(TEXT("nav_data"), SnapshotAdminBlock4NavData(World));

		TArray<TSharedPtr<FJsonValue>> Projections;
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("host"), AdminBlock4NavHostPoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("close"), AdminBlock4NavClosePoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("ammo"), AdminBlock4NavAmmoPoint, NavSys)));
		After->SetArrayField(TEXT("projections"), Projections);

		const bool bOnlyAdminDirty = DirtyAfter.Num() == 1 && PackagesEqual(DirtyAfter[0], AdminPackage);
		if (!bOnlyAdminDirty)
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("execute_postcondition_failed");
			Change.After = After;
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("unexpected_dirty_packages"),
				FString::Printf(
					TEXT("Admin nav spawn/build dirtied packages outside Admin: %s. %s Hard stop. Close Unreal without saving. Do not save Lvl_Epitope or Neuro. Actor left for inspection; maps not saved."),
					*FormatPackageList(DirtyAfter),
					*RecastDirtyAudit),
				MakeShared<FBridgeChange>(Change));
		}

		if (const FString ProjectionError = AdminBlock4NavProjectionFailure(NavSys); !ProjectionError.IsEmpty())
		{
			DestroySpawned();
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("execute_postcondition_failed");
			Change.After = After;
			Change.After->SetStringField(TEXT("projection_error"), ProjectionError);
			Change.After->SetBoolField(TEXT("actor_destroyed"), true);
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("projection_failed"),
				FString::Printf(TEXT("%s Actor destroyed. Maps not saved."), *ProjectionError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSaveAdminBlock4NavMeshPrerequisite(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("save_dirty"), false))
		{
			return TEXT("Save All / save_dirty is forbidden. save_admin_block4_navmesh_prerequisite saves Admin only.");
		}
		if (GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("compile must be false.");
		}
		if (!GetBool(Args, TEXT("require_pie_stopped"), true))
		{
			return TEXT("require_pie_stopped must be true.");
		}
		if (!GetString(Args, TEXT("spec")).Equals(AdminBlock4NavSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be exactly '%s'."), AdminBlock4NavSpec);
		}

		UWorld* World = nullptr;
		if (!IsInGameThread())
		{
			return TEXT("Admin Block 4 nav save must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before Admin Block 4 nav save.");
		}
		const FString WorldError = RequireEpitopeEditorWorld(World);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}
		if (!FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded.");
		}
		UPackage* EpitopePkg = FindPackageByName(EpitopePackage);
		UPackage* AdminPkg = FindPackageByName(AdminPackage);
		UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
		if (!EpitopePkg || !AdminPkg || !NeuroPkg)
		{
			return TEXT("Lvl_Epitope, Admin, and NeuroGenetics packages must be loaded.");
		}
		if (EpitopePkg->IsDirty())
		{
			return TEXT("Lvl_Epitope is dirty. Hard stop. Close without save. Never auto-save root.");
		}
		if (NeuroPkg->IsDirty())
		{
			return TEXT("NeuroGenetics is dirty. Hard stop. Close without save. Never auto-save Neuro.");
		}
		if (const FString HostGuard = GuardNoAdminBlock4Host(World); !HostGuard.IsEmpty())
		{
			return HostGuard;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, AdminBlock4NavLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("NavMeshBounds_Admin_Security count=%d expected=1 before save."), Matches.Num());
		}
		ANavMeshBoundsVolume* Volume = Cast<ANavMeshBoundsVolume>(Matches[0]);
		FVector Extent = FVector::ZeroVector;
		if (!Volume)
		{
			return TEXT("Saved candidate NavMeshBounds_Admin_Security is not a NavMeshBoundsVolume.");
		}
		if (const FString BrushError = AdminBlock4NavBrushMatchFailure(Volume, Extent); !BrushError.IsEmpty())
		{
			return FString::Printf(
				TEXT("Saved candidate NavMeshBounds_Admin_Security identity/brush/package/transform mismatch. %s"),
				*BrushError);
		}

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (const FString ProjectionError = AdminBlock4NavProjectionFailure(NavSys); !ProjectionError.IsEmpty())
		{
			return ProjectionError;
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (!(Dirty.Num() == 1 && PackagesEqual(Dirty[0], AdminPackage)))
		{
			return FString::Printf(
				TEXT("Dirty packages must be exactly [/Game/Maps/Epitope/SL_Epitope_Admin]. Found: %s. Hard stop. Close without save."),
				*FormatPackageList(Dirty));
		}
		if (!AdminPkg->IsDirty())
		{
			return TEXT("Admin package is not dirty; refusing empty/no-op Admin nav save.");
		}

		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), false);
		Before->SetBoolField(TEXT("on_game_thread"), true);
		Before->SetBoolField(TEXT("admin_only"), true);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetArrayField(TEXT("packages"), DirtyPackageJsonArray(TArray<FString>{FString(AdminPackage)}));
		Before->SetObjectField(TEXT("volume"), SnapshotAdminBlock4NavVolume(Volume));
		Before->SetObjectField(TEXT("nav_data"), SnapshotAdminBlock4NavData(World));
		TArray<TSharedPtr<FJsonValue>> Projections;
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("host"), AdminBlock4NavHostPoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("close"), AdminBlock4NavClosePoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("ammo"), AdminBlock4NavAmmoPoint, NavSys)));
		Before->SetArrayField(TEXT("projections"), Projections);
		Before->SetNumberField(TEXT("admin_host_count"), 0);

		Proposed->SetStringField(TEXT("spec"), AdminBlock4NavSpec);
		Proposed->SetStringField(TEXT("action"), AdminBlock4NavSaveAction);
		Proposed->SetArrayField(TEXT("packages"), DirtyPackageJsonArray(TArray<FString>{FString(AdminPackage)}));
		Proposed->SetBoolField(TEXT("admin_only"), true);
		Proposed->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
		Proposed->SetBoolField(TEXT("dialog"), false);
		Proposed->SetBoolField(TEXT("save_all"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("label"), AdminBlock4NavLabel);
		Proposed->SetArrayField(TEXT("location"), Vec(AdminBlock4NavLocation));
		Proposed->SetArrayField(TEXT("brush_size"), Vec(AdminBlock4NavBrushSize));
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Save SL_Epitope_Admin only after Admin Block 4 nav projections verify. Never save Lvl_Epitope or Neuro."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSaveAdminBlock4NavMeshPrerequisite(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_admin_block4_navmesh_prerequisite must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveAdminBlock4NavMeshPrerequisite(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UPackage* AdminPkg = FindPackageByName(AdminPackage);
		if (!AdminPkg)
		{
			return FailAudit(TEXT("not_found"), TEXT("Admin map package not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TArray<UPackage*> AdminOnly;
		AdminOnly.Add(AdminPkg);
		const bool bAdminSaved = UEditorLoadingAndSavingUtils::SavePackages(AdminOnly, /*bOnlyDirty=*/false);

		UWorld* World = GetEditorWorld();
		UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		TArray<AActor*> Matches = World ? FindByExactLabel(World, AdminBlock4NavLabel) : TArray<AActor*>();
		ANavMeshBoundsVolume* Volume = Matches.Num() == 1 ? Cast<ANavMeshBoundsVolume>(Matches[0]) : nullptr;
		FVector Extent = FVector::ZeroVector;
		const FString VolumeError = Volume
			? AdminBlock4NavBrushMatchFailure(Volume, Extent)
			: FString(TEXT("volume missing after save."));
		const bool bVolumeOk = VolumeError.IsEmpty();
		const FString ProjectionError = AdminBlock4NavProjectionFailure(NavSys);
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();

		UPackage* EpitopePkg = FindPackageByName(EpitopePackage);
		UPackage* NeuroPkg = FindPackageByName(NeuroPackage);
		const bool bRootClean = EpitopePkg && !EpitopePkg->IsDirty();
		const bool bNeuroClean = NeuroPkg && !NeuroPkg->IsDirty();
		const bool bAdminClean = AdminPkg && !AdminPkg->IsDirty();
		const bool bDirtyEmpty = DirtyAfter.Num() == 0;

		Change.bExecuted = bAdminSaved;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = bAdminSaved;
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("on_game_thread"), true);
		Change.After->SetBoolField(TEXT("admin_saved"), bAdminSaved);
		Change.After->SetBoolField(TEXT("admin_only"), true);
		Change.After->SetStringField(TEXT("api"), TEXT("UEditorLoadingAndSavingUtils::SavePackages"));
		Change.After->SetBoolField(TEXT("dialog"), false);
		Change.After->SetBoolField(TEXT("save_all"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetArrayField(TEXT("packages_saved"), DirtyPackageJsonArray(bAdminSaved ? TArray<FString>{FString(AdminPackage)} : TArray<FString>()));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		Change.After->SetBoolField(TEXT("epitope_clean"), bRootClean);
		Change.After->SetBoolField(TEXT("neuro_clean"), bNeuroClean);
		Change.After->SetBoolField(TEXT("admin_clean"), bAdminClean);
		Change.After->SetObjectField(TEXT("volume"), SnapshotAdminBlock4NavVolume(Volume));
		Change.After->SetObjectField(TEXT("nav_data"), SnapshotAdminBlock4NavData(World));
		TArray<TSharedPtr<FJsonValue>> Projections;
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("host"), AdminBlock4NavHostPoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("close"), AdminBlock4NavClosePoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("ammo"), AdminBlock4NavAmmoPoint, NavSys)));
		Change.After->SetArrayField(TEXT("projections"), Projections);

		if (!bAdminSaved)
		{
			Change.Status = TEXT("execute_save_failed");
			LogAudit(TEXT("execute"), Change);
			return FailAudit(TEXT("save_failed"), TEXT("SavePackages failed for SL_Epitope_Admin."), MakeShared<FBridgeChange>(Change));
		}
		if (!bVolumeOk || !ProjectionError.IsEmpty() || !bRootClean || !bNeuroClean || !bAdminClean || !bDirtyEmpty)
		{
			Change.Status = TEXT("execute_postcondition_failed");
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("post_save_verify_failed"),
				FString::Printf(
					TEXT("Admin was saved but post-verify failed (volume_ok=%s volume_error='%s' projection='%s' epitope_clean=%s neuro_clean=%s admin_clean=%s dirty=%s). Hard stop. Do not save root/Neuro."),
					bVolumeOk ? TEXT("true") : TEXT("false"),
					*VolumeError,
					ProjectionError.IsEmpty() ? TEXT("") : *ProjectionError,
					bRootClean ? TEXT("true") : TEXT("false"),
					bNeuroClean ? TEXT("true") : TEXT("false"),
					bAdminClean ? TEXT("true") : TEXT("false"),
					*FormatPackageList(DirtyAfter)),
				MakeShared<FBridgeChange>(Change));
		}

		Change.Status = TEXT("executed");
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> CmdInspectAdminBlock4NavMesh(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		if (!IsInGameThread())
		{
			return Fail(TEXT("wrong_thread"), TEXT("inspect_admin_block4_navmesh must run on the game thread."));
		}
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return Fail(TEXT("no_editor"), TEXT("No editor world."));
		}

		TArray<AActor*> Matches = FindByExactLabel(World, AdminBlock4NavLabel);
		ANavMeshBoundsVolume* Volume = Matches.Num() == 1 ? Cast<ANavMeshBoundsVolume>(Matches[0]) : nullptr;
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();

		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("spec"), AdminBlock4NavSpec);
		Data->SetStringField(TEXT("expected_label"), AdminBlock4NavLabel);
		Data->SetStringField(TEXT("expected_package"), AdminPackage);
		Data->SetArrayField(TEXT("expected_location"), Vec(AdminBlock4NavLocation));
		Data->SetArrayField(TEXT("expected_brush_size"), Vec(AdminBlock4NavBrushSize));
		Data->SetNumberField(TEXT("volume_count"), Matches.Num());
		Data->SetObjectField(TEXT("volume"), SnapshotAdminBlock4NavVolume(Volume));
		Data->SetObjectField(TEXT("nav_data"), SnapshotAdminBlock4NavData(World));
		TArray<TSharedPtr<FJsonValue>> Projections;
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("host"), AdminBlock4NavHostPoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("close"), AdminBlock4NavClosePoint, NavSys)));
		Projections.Add(MakeShared<FJsonValueObject>(AdminBlock4NavProjectionRow(TEXT("ammo"), AdminBlock4NavAmmoPoint, NavSys)));
		Data->SetArrayField(TEXT("projections"), Projections);
		Data->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Data->SetNumberField(TEXT("dirty_count"), Dirty.Num());
		Data->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Data->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Data->SetNumberField(TEXT("admin_host_count"), FindByExactLabel(World, AdminBlock4HostLabel).Num());
		return Ok(Data);
	}
