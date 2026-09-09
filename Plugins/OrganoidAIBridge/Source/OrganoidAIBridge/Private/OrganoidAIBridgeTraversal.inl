	const TCHAR* ResearchWingSpec = TEXT("admin_research_wing_connector_v1");
	const TCHAR* EastWallLabel = TEXT("Admin_ServiceCorridor_Wall_East");
	const TCHAR* TransitEastLabel = TEXT("Admin_Transit_Wall_East");
	const TCHAR* TransitSouthLabel = TEXT("Admin_Transit_Wall_South");
	const TCHAR* TransitFloorLabel = TEXT("Admin_Transit_Floor");
	const TCHAR* ServiceFloorLabel = TEXT("Admin_ServiceCorridor_Floor");
	const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	const FVector EastWallLocation(4492.5f, -925.f, 225.f);
	const FVector EastWallScale(0.25f, 6.f, 4.5f);
	const FVector ServiceFloorLocation(3712.5f, -912.5f, 10.f);
	const FVector ServiceFloorScale(15.35f, 5.75f, 0.2f);
	const FVector TransitEastLocation(5467.5f, 0.f, 225.f);
	const FVector TransitEastScale(0.25f, 7.f, 4.5f);
	const FVector TransitSouthLocation(5005.f, -362.5f, 225.f);
	const FVector TransitSouthScale(9.f, 0.25f, 4.5f);
	const FVector TransitFloorLocation(5005.f, 0.f, 10.f);

	struct FResearchWingCubeSpec
	{
		const TCHAR* Label;
		FVector Location;
		FVector Scale;
	};

	const FResearchWingCubeSpec ResearchWingCubes[] = {
		{TEXT("Admin_ServiceCorridor_Wall_East_South"), FVector(4492.5f, -1127.5f, 225.f), FVector(0.25f, 1.95f, 4.5f)},
		{TEXT("Admin_ServiceCorridor_Wall_East_North"), FVector(4492.5f, -697.5f, 225.f), FVector(0.25f, 1.45f, 4.5f)},
		{TEXT("Admin_ServiceCorridor_Wall_East_Header"), FVector(4492.5f, -900.f, 375.f), FVector(0.25f, 2.6f, 1.5f)},
		{TEXT("Admin_ResearchWing_Connector_Floor"), FVector(4752.5f, -912.5f, 10.f), FVector(4.95f, 5.75f, 0.2f)},
		{TEXT("Admin_ResearchWing_Connector_Ceiling"), FVector(4752.5f, -912.5f, 470.f), FVector(4.95f, 5.75f, 0.2f)},
		{TEXT("Admin_ResearchWing_Connector_Wall_South"), FVector(4752.5f, -1212.5f, 225.f), FVector(4.95f, 0.25f, 4.5f)},
		{TEXT("Admin_ResearchWing_Connector_Wall_North"), FVector(4752.5f, -612.5f, 225.f), FVector(4.95f, 0.25f, 4.5f)},
		{TEXT("Admin_ResearchWing_Connector_Threshold"), FVector(4492.5f, -900.f, 10.f), FVector(0.5f, 2.6f, 0.2f)},
	};

	FString GuardExactCube(UWorld* World, const TCHAR* Label, const FVector& Location, const FVector& Scale)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
		{
			return FString::Printf(TEXT("%s must remain on SL_Epitope_Admin."), Label);
		}
		if (!TransformMatches(Actor, Location, FRotator::ZeroRotator, Scale))
		{
			return TransformMismatch(Actor, Location, FRotator::ZeroRotator, Scale);
		}
		return TEXT("");
	}

	FString GuardExactLocation(UWorld* World, const TCHAR* Label, const FVector& Location)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
		{
			return FString::Printf(TEXT("%s must remain on SL_Epitope_Admin."), Label);
		}
		if (!LocationMatches(Actor->GetActorLocation(), Location))
		{
			const FVector Loc = Actor->GetActorLocation();
			return FString::Printf(
				TEXT("%s location (%.2f, %.2f, %.2f) expected (%.2f, %.2f, %.2f)"),
				Label, Loc.X, Loc.Y, Loc.Z, Location.X, Location.Y, Location.Z);
		}
		return TEXT("");
	}

	FString PreflightSpawnAdminResearchWingConnector(
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
			return TEXT("save/compile must be false. spawn_admin_research_wing_connector does not save or compile.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ResearchWingSpec);
		if (!Spec.Equals(ResearchWingSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be admin_research_wing_connector_v1.");
		}
		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("level_package"), GetString(Args, TEXT("required_package"), AdminPackage))));
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("destination must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, Destination))
		{
			return TEXT("SL_Epitope_Admin is not loaded. ZERO writes.");
		}

		if (FString Error = GuardExactCube(World, EastWallLabel, EastWallLocation, EastWallScale); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExactCube(World, ServiceFloorLabel, ServiceFloorLocation, ServiceFloorScale); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExactCube(World, TransitEastLabel, TransitEastLocation, TransitEastScale); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExactCube(World, TransitSouthLabel, TransitSouthLocation, TransitSouthScale); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExactLocation(World, TransitFloorLabel, TransitFloorLocation); !Error.IsEmpty())
		{
			return Error;
		}

		for (const FResearchWingCubeSpec& Cube : ResearchWingCubes)
		{
			if (FindByExactLabel(World, Cube.Label).Num() != 0)
			{
				return FString::Printf(TEXT("%s already exists. Fail closed."), Cube.Label);
			}
		}
		if (FindByExactLabel(World, TEXT("Gate_ResearchWing")).Num() != 0)
		{
			const TArray<AActor*> Gates = FindByExactLabel(World, TEXT("Gate_ResearchWing"));
			for (AActor* Gate : Gates)
			{
				if (PackagesEqual(ActorOwningPackage(Gate), AdminPackage))
				{
					return TEXT("Gate_ResearchWing must not be duplicated onto Admin.");
				}
			}
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("east_wall_present"), true);
		Before->SetBoolField(TEXT("transit_east_sealed"), true);
		Proposed->SetStringField(TEXT("spec"), ResearchWingSpec);
		Proposed->SetStringField(TEXT("destination_package"), Destination);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Open S12 east wall at Y=-900 and spawn the Research Wing connector onto SL_Epitope_Admin. Does not save. Does not touch Transit, spine, Neuro, or Gate_ResearchWing."));
		return TEXT("");
	}

	bool ApplyAdminCube(AStaticMeshActor* Actor, const FResearchWingCubeSpec& Spec, UStaticMesh* Cube)
	{
		if (!Actor || !Cube)
		{
			return false;
		}
		Actor->SetActorLabel(Spec.Label, true);
		Actor->Tags.AddUnique(FName(TEXT("Admin_ResearchWing_Connector")));
		Actor->SetActorLocation(Spec.Location, false, nullptr, ETeleportType::None);
		Actor->SetActorRotation(FRotator::ZeroRotator);
		Actor->SetActorScale3D(Spec.Scale);
		UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
		if (!Mesh)
		{
			return false;
		}
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Actor->MarkPackageDirty();
		return TransformMatches(Actor, Spec.Location, FRotator::ZeroRotator, Spec.Scale);
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminResearchWingConnector(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_admin_research_wing_connector must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnAdminResearchWingConnector(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, AdminPackage) : nullptr;
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, CubeMeshPath);
		if (!World || !TargetLevel || !Cube)
		{
			return FailAudit(TEXT("not_found"), TEXT("World, Admin level, or Cube mesh vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnResearchWingConnector", "Spawn Admin Research Wing Connector"));
		TArray<AStaticMeshActor*> Spawned;
		for (const FResearchWingCubeSpec& Spec : ResearchWingCubes)
		{
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Spec.Location, FRotator::ZeroRotator, Params);
			if (!Actor || !ApplyAdminCube(Actor, Spec, Cube))
			{
				for (AStaticMeshActor* Created : Spawned)
				{
					if (IsValid(Created))
					{
						Created->Destroy();
					}
				}
				if (IsValid(Actor))
				{
					Actor->Destroy();
				}
				return FailAudit(
					TEXT("spawn_failed"),
					FString::Printf(TEXT("Failed to spawn %s. Original east wall left in place. ZERO remaining writes."), Spec.Label),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned.Add(Actor);
		}

		TArray<AActor*> EastWall = FindByExactLabel(World, EastWallLabel);
		if (EastWall.Num() != 1)
		{
			for (AStaticMeshActor* Created : Spawned)
			{
				if (IsValid(Created))
				{
					Created->Destroy();
				}
			}
			return FailAudit(TEXT("east_wall_vanished"), TEXT("Admin_ServiceCorridor_Wall_East vanished before delete. New cubes destroyed."), MakeShared<FBridgeChange>(Change));
		}
		EastWall[0]->Destroy();

		if (FString Error = GuardExactCube(World, TransitEastLabel, TransitEastLocation, TransitEastScale); !Error.IsEmpty())
		{
			return FailAudit(TEXT("transit_mutated"), FString::Printf(TEXT("Transit east wall changed. %s"), *Error), MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("spec"), ResearchWingSpec);
		Change.After->SetStringField(TEXT("owning_package"), AdminPackage);
		Change.After->SetBoolField(TEXT("east_wall_removed"), FindByExactLabel(World, EastWallLabel).Num() == 0);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		TArray<TSharedPtr<FJsonValue>> Labels;
		for (const FResearchWingCubeSpec& Spec : ResearchWingCubes)
		{
			Labels.Add(MakeShared<FJsonValueString>(FString(Spec.Label)));
		}
		Change.After->SetArrayField(TEXT("spawned_labels"), Labels);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	const TCHAR* TrimSpineSpec = TEXT("trim_spine_landing_admin_v1");
	const TCHAR* SpineLandingLabel = TEXT("Spine_Landing_Admin");
	const TCHAR* SpineBridgeLabel = TEXT("Spine_Bridge_Admin");
	const TCHAR* SpineRampLabel = TEXT("Spine_Ramp_Admin_To_NeuroGenetics");
	const TCHAR* ResearchGateLabel = TEXT("Gate_ResearchWing");
	const FVector SpineLandingLocation(5000.f, -900.f, 0.f);
	const FVector SpineLandingScaleBefore(16.f, 10.f, 0.2f);
	const FVector SpineLandingScaleAfter(16.f, 1.2f, 0.2f);
	const FVector SpineBridgeLocation(4000.f, -900.f, 0.f);
	const FVector SpineBridgeScaleBefore(28.f, 6.f, 0.2f);
	const FVector SpineBridgeScaleAfter(28.f, 1.2f, 0.2f);
	const FVector SpineRampLocation(5000.f, 0.f, -600.f);
	const FRotator SpineRampRotation(0.f, 0.f, -33.69f);
	const FVector SpineRampScale(16.f, 21.63f, 0.2f);
	const FVector ResearchGateLocation(2900.f, 900.f, -1060.f);

	FString DirtyWorldPackageList()
	{
		TArray<UPackage*> DirtyWorld;
		FEditorFileUtils::GetDirtyWorldPackages(DirtyWorld);
		TArray<FString> Names;
		for (UPackage* Package : DirtyWorld)
		{
			if (Package)
			{
				Names.Add(Package->GetName());
			}
		}
		return FString::Join(Names, TEXT(", "));
	}

	FString GuardEpitopeTransform(
		UWorld* World,
		const TCHAR* Label,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (!PackagesEqual(ActorOwningPackage(Actor), EpitopePackage))
		{
			return FString::Printf(TEXT("%s must remain on Lvl_Epitope."), Label);
		}
		if (!TransformMatches(Actor, Location, Rotation, Scale))
		{
			return FString::Printf(TEXT("%s %s"), Label, *TransformMismatch(Actor, Location, Rotation, Scale));
		}
		return TEXT("");
	}

	FString GuardEpitopeLocation(UWorld* World, const TCHAR* Label, const FVector& Location)
	{
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), Label, Matches.Num());
		}
		AActor* Actor = Matches[0];
		if (!PackagesEqual(ActorOwningPackage(Actor), EpitopePackage))
		{
			return FString::Printf(TEXT("%s must remain on Lvl_Epitope."), Label);
		}
		if (!LocationMatches(Actor->GetActorLocation(), Location))
		{
			const FVector Loc = Actor->GetActorLocation();
			return FString::Printf(
				TEXT("%s location (%.2f, %.2f, %.2f) expected (%.2f, %.2f, %.2f)"),
				Label, Loc.X, Loc.Y, Loc.Z, Location.X, Location.Y, Location.Z);
		}
		return TEXT("");
	}

	FString GuardTrimSpineActors(UWorld* World)
	{
		if (FString Error = GuardEpitopeTransform(
				World, SpineLandingLabel, SpineLandingLocation, FRotator::ZeroRotator, SpineLandingScaleBefore);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardEpitopeTransform(
				World, SpineBridgeLabel, SpineBridgeLocation, FRotator::ZeroRotator, SpineBridgeScaleBefore);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardEpitopeTransform(
				World, SpineRampLabel, SpineRampLocation, SpineRampRotation, SpineRampScale);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardEpitopeLocation(World, ResearchGateLabel, ResearchGateLocation); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	bool OpenLvlEpitopeIfNeeded(FString& OutError)
	{
		UWorld* World = GetEditorWorld();
		if (World && IsEpitopeWorldPackage(WorldPackageName(World)))
		{
			return true;
		}
		if (!World || !PackagesEqual(WorldPackageName(World), MainMenuPackage))
		{
			OutError = FString::Printf(
				TEXT("Persistent map must be Lvl_MainMenu or Lvl_Epitope (current '%s')."),
				World ? *NormalizePackage(WorldPackageName(World)) : TEXT("none"));
			return false;
		}
		const FString Dirty = DirtyWorldPackageList();
		if (!Dirty.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Dirty world packages [%s]. Save them before opening Lvl_Epitope."), *Dirty);
			return false;
		}
		FString MapFilename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(
				FString(EpitopePackage), MapFilename, FPackageName::GetMapPackageExtension()))
		{
			OutError = TEXT("Could not resolve Lvl_Epitope filename.");
			return false;
		}
		if (!FEditorFileUtils::LoadMap(MapFilename, false, true))
		{
			OutError = TEXT("FEditorFileUtils::LoadMap failed for Lvl_Epitope.");
			return false;
		}
		World = GetEditorWorld();
		if (!World || !IsEpitopeWorldPackage(WorldPackageName(World)))
		{
			OutError = TEXT("Persistent map is not Lvl_Epitope after LoadMap.");
			return false;
		}
		return true;
	}

	FString PreflightTrimSpineLandingAdmin(
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
			return TEXT("save/compile must be false. trim_spine_landing_admin does not save or compile.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), TrimSpineSpec);
		if (!Spec.Equals(TrimSpineSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be trim_spine_landing_admin_v1.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		const FString Persistent = NormalizePackage(WorldPackageName(World));
		const bool bOnEpitope = IsEpitopeWorldPackage(Persistent);
		const bool bOnMainMenu = PackagesEqual(Persistent, MainMenuPackage);
		if (!bOnEpitope && !bOnMainMenu)
		{
			return FString::Printf(TEXT("Persistent map must be Lvl_MainMenu or Lvl_Epitope (current '%s')."), *Persistent);
		}
		if (!bOnEpitope)
		{
			const FString Dirty = DirtyWorldPackageList();
			if (!Dirty.IsEmpty())
			{
				return FString::Printf(TEXT("Dirty world packages [%s]. Save them before opening Lvl_Epitope."), *Dirty);
			}
		}
		else if (FString Error = GuardTrimSpineActors(World); !Error.IsEmpty())
		{
			return Error;
		}

		Before->SetStringField(TEXT("persistent_package"), Persistent);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("will_open_lvl_epitope"), !bOnEpitope);
		if (bOnEpitope)
		{
			Before->SetObjectField(TEXT("landing"), ActorSnapshot(FindByExactLabel(World, SpineLandingLabel)[0]));
			Before->SetObjectField(TEXT("bridge"), ActorSnapshot(FindByExactLabel(World, SpineBridgeLabel)[0]));
			Before->SetObjectField(TEXT("ramp"), ActorSnapshot(FindByExactLabel(World, SpineRampLabel)[0]));
			Before->SetObjectField(TEXT("gate"), ActorSnapshot(FindByExactLabel(World, ResearchGateLabel)[0]));
		}
		Proposed->SetStringField(TEXT("spec"), TrimSpineSpec);
		Proposed->SetStringField(TEXT("required_package"), EpitopePackage);
		Proposed->SetArrayField(TEXT("landing_scale"), Vec(SpineLandingScaleAfter));
		Proposed->SetArrayField(TEXT("bridge_scale"), Vec(SpineBridgeScaleAfter));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Shrink Spine_Landing_Admin and Spine_Bridge_Admin Y scale on Lvl_Epitope so the existing ramp is exposed south of Transit. Opens Lvl_Epitope if persistent is Lvl_MainMenu and no world packages are dirty. Does not move actors. Does not touch the ramp, gate, Transit, or Admin connector. Does not save."));
		return TEXT("");
	}

	bool ApplyScaleOnly(AActor* Actor, const FVector& ExpectedLocation, const FVector& NewScale)
	{
		if (!Actor || !LocationMatches(Actor->GetActorLocation(), ExpectedLocation))
		{
			return false;
		}
		Actor->Modify();
		Actor->SetActorScale3D(NewScale);
		Actor->MarkPackageDirty();
		return LocationMatches(Actor->GetActorLocation(), ExpectedLocation)
			&& ScaleMatches(Actor->GetActorScale3D(), NewScale);
	}

	TSharedRef<FJsonObject> ExecuteTrimSpineLandingAdmin(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("trim_spine_landing_admin must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightTrimSpineLandingAdmin(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		FString OpenError;
		if (!OpenLvlEpitopeIfNeeded(OpenError))
		{
			return FailAudit(TEXT("open_failed"), FString::Printf(TEXT("ZERO writes. %s"), *OpenError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		if (FString Error = GuardTrimSpineActors(World); !Error.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes after map open. %s"), *Error), MakeShared<FBridgeChange>(Change));
		}

		AActor* Landing = FindByExactLabel(World, SpineLandingLabel)[0];
		AActor* Bridge = FindByExactLabel(World, SpineBridgeLabel)[0];
		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "TrimSpineLandingAdmin", "Trim Admin Spine Plates"));
		if (!ApplyScaleOnly(Landing, SpineLandingLocation, SpineLandingScaleAfter))
		{
			return FailAudit(TEXT("scale_failed"), TEXT("Failed to scale Spine_Landing_Admin. Bridge left unchanged."), MakeShared<FBridgeChange>(Change));
		}
		if (!ApplyScaleOnly(Bridge, SpineBridgeLocation, SpineBridgeScaleAfter))
		{
			Landing->SetActorScale3D(SpineLandingScaleBefore);
			Landing->MarkPackageDirty();
			return FailAudit(TEXT("scale_failed"), TEXT("Failed to scale Spine_Bridge_Admin. Landing scale reverted."), MakeShared<FBridgeChange>(Change));
		}

		if (FString Error = GuardEpitopeTransform(
				World, SpineRampLabel, SpineRampLocation, SpineRampRotation, SpineRampScale);
			!Error.IsEmpty())
		{
			return FailAudit(TEXT("ramp_mutated"), FString::Printf(TEXT("Ramp changed. %s"), *Error), MakeShared<FBridgeChange>(Change));
		}
		if (FString Error = GuardEpitopeLocation(World, ResearchGateLabel, ResearchGateLocation); !Error.IsEmpty())
		{
			return FailAudit(TEXT("gate_mutated"), FString::Printf(TEXT("Gate changed. %s"), *Error), MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("spec"), TrimSpineSpec);
		Change.After->SetStringField(TEXT("owning_package"), EpitopePackage);
		Change.After->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Change.After->SetObjectField(TEXT("landing"), ActorSnapshot(Landing));
		Change.After->SetObjectField(TEXT("bridge"), ActorSnapshot(Bridge));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
