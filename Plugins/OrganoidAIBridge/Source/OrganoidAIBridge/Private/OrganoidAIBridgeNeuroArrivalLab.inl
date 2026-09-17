	const TCHAR* NeuroArrivalCubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroArrivalCylinderMesh = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	enum class ENeuroArrivalMesh : uint8
	{
		Cube,
		Cylinder
	};

	struct FNeuroArrivalPropSpec
	{
		const TCHAR* Label;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		ENeuroArrivalMesh Mesh;
	};

	struct FNeuroArrivalStaleSpec
	{
		const TCHAR* Label;
		FVector Location;
		const TCHAR* ClassContains;
	};

	const FNeuroArrivalPropSpec NeuroArrivalProps[] = {
		{TEXT("Neuro_Lab_Bench_1"), FVector(200.f, -1850.f, -1160.f), FRotator::ZeroRotator, FVector(2.2f, 0.85f, 0.80f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Lab_Bench_2"), FVector(500.f, -2000.f, -1160.f), FRotator::ZeroRotator, FVector(2.2f, 0.85f, 0.80f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Lab_Bench_3"), FVector(-100.f, -2000.f, -1160.f), FRotator::ZeroRotator, FVector(2.2f, 0.85f, 0.80f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Lab_Desk_1"), FVector(550.f, -1400.f, -1162.f), FRotator::ZeroRotator, FVector(1.4f, 0.70f, 0.75f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Lab_Desk_2"), FVector(250.f, -1450.f, -1162.f), FRotator::ZeroRotator, FVector(1.4f, 0.70f, 0.75f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Lab_Microscope_1"), FVector(550.f, -1380.f, -1100.f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.40f), ENeuroArrivalMesh::Cylinder},
	};

	const FNeuroArrivalStaleSpec NeuroArrivalStale[] = {
		{TEXT("DataPad_EthicsObjection"), FVector(0.f, -1650.f, -1110.f), TEXT("ProjectOrganoidDataPad")},
		{TEXT("DataPad_SpecimenBadge"), FVector(-360.f, -2150.f, -1110.f), TEXT("ProjectOrganoidDataPad")},
		{TEXT("NPC_IncineratorSurvivor"), FVector(440.f, -1100.f, -1100.f), TEXT("ProjectOrganoidDialogueNPC")},
	};

	const TCHAR* NeuroArrivalMeshPath(ENeuroArrivalMesh Kind)
	{
		return Kind == ENeuroArrivalMesh::Cylinder ? NeuroArrivalCylinderMesh : NeuroArrivalCubeMesh;
	}

	FString ApplyNeuroArrivalMesh(AActor* Actor, ENeuroArrivalMesh Kind)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor/component missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, NeuroArrivalMeshPath(Kind));
		if (!Mesh)
		{
			return FString::Printf(TEXT("Failed to load %s."), NeuroArrivalMeshPath(Kind));
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		return TEXT("");
	}

	bool NeuroArrivalMeshMatches(AActor* Actor, ENeuroArrivalMesh Kind)
	{
		const FString Path = MeshPath(Actor);
		if (Kind == ENeuroArrivalMesh::Cylinder)
		{
			return Path.Contains(TEXT("/Engine/BasicShapes/Cylinder"));
		}
		return Path.Contains(TEXT("/Engine/BasicShapes/Cube"));
	}

	FString NeuroArrivalPropMismatch(AActor* Actor, const FNeuroArrivalPropSpec& Spec)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (!Actor->IsA(AStaticMeshActor::StaticClass()))
		{
			return FString::Printf(TEXT("class '%s' is not StaticMeshActor"), *ClassName(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, Spec.Location, Spec.Rotation, Spec.Scale); !Xform.IsEmpty())
		{
			return Xform;
		}
		if (!NeuroArrivalMeshMatches(Actor, Spec.Mesh))
		{
			return FString::Printf(TEXT("mesh '%s' expected %s"), *MeshPath(Actor), NeuroArrivalMeshPath(Spec.Mesh));
		}
		return TEXT("");
	}

	FString GuardNeuroArrivalKeepList(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!FindLoadedLevelByPackage(World, NeuroPackage))
		{
			return TEXT("SL_Epitope_NeuroGenetics is not loaded. ZERO writes.");
		}
		if (FString Error = GuardExistingActor(World, TEXT("Checkpoint_NeuroAirlock"), FVector(1950.f, 0.f, -1140.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Spine_Landing_NeuroGenetics"), FVector(5000.f, 900.f, -1200.f), EpitopePackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("NeuroGenetics_FloorPlate"), FVector(0.f, 0.f, -1200.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("ResearchStation_NeuroGenetics"), FVector(800.f, -1600.f, -1100.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("PowerPanel_NeuroBackup"), FVector(-500.f, -2275.f, -1100.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Hazard_ScrubberLeak"), FVector(-1950.f, -1650.f, -1000.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("CorridorTraps_GowningRing"), FVector(-1145.f, 0.f, -1060.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Ambience_GowningCorridor"), FVector(-950.f, 0.f, -1000.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("NavMeshBounds_NeuroGenetics"), FVector(0.f, 0.f, -1000.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Scannable_OrganoidMatrix_1"), FVector(-2425.f, 1900.f, -1080.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Scannable_OrganoidMatrix_2"), FVector(-1950.f, 1900.f, -1080.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		if (FString Error = GuardExistingActor(World, TEXT("Scannable_OrganoidMatrix_3"), FVector(-1475.f, 1900.f, -1080.f), NeuroPackage); !Error.IsEmpty())
		{
			return Error;
		}
		for (const TCHAR* HostLabel : {TEXT("Host_Neuro_1"), TEXT("Host_Neuro_2"), TEXT("Host_Neuro_3")})
		{
			TArray<AActor*> Hosts = FindOwnedByExactLabel(World, HostLabel, NeuroPackage);
			if (Hosts.Num() != 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d expected=1. Do not delete Hosts."), HostLabel, Hosts.Num());
			}
		}
		return TEXT("");
	}

	FString GuardNeuroArrivalStale(UWorld* World, int32& OutPresent)
	{
		OutPresent = 0;
		for (const FNeuroArrivalStaleSpec& Spec : NeuroArrivalStale)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage);
			if (Matches.Num() > 1)
			{
				return FString::Printf(TEXT("%s Neuro count=%d. Abort rather than guess."), Spec.Label, Matches.Num());
			}
			if (Matches.Num() == 0)
			{
				continue;
			}
			AActor* Actor = Matches[0];
			if (!LocationMatches(Actor->GetActorLocation(), Spec.Location))
			{
				const FVector Loc = Actor->GetActorLocation();
				return FString::Printf(
					TEXT("%s location (%.2f, %.2f, %.2f) expected (%.2f, %.2f, %.2f). Abort."),
					Spec.Label, Loc.X, Loc.Y, Loc.Z, Spec.Location.X, Spec.Location.Y, Spec.Location.Z);
			}
			if (!ClassName(Actor).Contains(Spec.ClassContains))
			{
				return FString::Printf(TEXT("%s class '%s' expected to contain %s."), Spec.Label, *ClassName(Actor), Spec.ClassContains);
			}
			++OutPresent;
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroArrivalLabDressing(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_arrival_lab_dressing does not save or compile.");
		}

		UWorld* World = GetEditorWorld();
		const FString KeepError = GuardNeuroArrivalKeepList(World);
		if (!KeepError.IsEmpty())
		{
			return KeepError;
		}

		int32 StalePresent = 0;
		if (const FString StaleError = GuardNeuroArrivalStale(World, StalePresent); !StaleError.IsEmpty())
		{
			return StaleError;
		}

		int32 PropsPresent = 0;
		for (const FNeuroArrivalPropSpec& Spec : NeuroArrivalProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() > 1)
			{
				return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), Spec.Label, Matches.Num());
			}
			if (Matches.Num() == 1)
			{
				if (const FString Mismatch = NeuroArrivalPropMismatch(Matches[0], Spec); !Mismatch.IsEmpty())
				{
					return FString::Printf(TEXT("Mismatched %s: %s. Abort."), Spec.Label, *Mismatch);
				}
				++PropsPresent;
			}
		}

		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("stale_present"), StalePresent);
		Before->SetNumberField(TEXT("props_present"), PropsPresent);
		Proposed->SetNumberField(TEXT("prop_count"), UE_ARRAY_COUNT(NeuroArrivalProps));
		Proposed->SetNumberField(TEXT("stale_count"), UE_ARRAY_COUNT(NeuroArrivalStale));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Remove stale Avery/Sterling Neuro pads + incinerator NPC. Spawn blockout benches/desks/microscope on Neuro. No collision. Does not save. Does not change power."));
		return TEXT("");
	}

	AActor* SpawnNeuroArrivalProp(UWorld* World, ULevel* TargetLevel, const FNeuroArrivalPropSpec& Spec, FString& OutError)
	{
		OutError.Reset();
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Spec.Location, Spec.Rotation, Params);
		if (!Actor)
		{
			OutError = FString::Printf(TEXT("SpawnActor failed for %s."), Spec.Label);
			return nullptr;
		}
		Actor->SetActorLabel(Spec.Label, true);
		Actor->SetActorScale3D(Spec.Scale);
		if (const FString MeshError = ApplyNeuroArrivalMesh(Actor, Spec.Mesh); !MeshError.IsEmpty())
		{
			Actor->Destroy();
			OutError = MeshError;
			return nullptr;
		}
		DisableCollision(Actor);
		Actor->MarkPackageDirty();
		if (const FString AfterError = NeuroArrivalPropMismatch(Actor, Spec); !AfterError.IsEmpty())
		{
			Actor->Destroy();
			OutError = AfterError;
			return nullptr;
		}
		return Actor;
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroArrivalLabDressing(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_arrival_lab_dressing must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroArrivalLabDressing(Change.Args, Before, Proposed);
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

		int32 Deleted = 0;
		int32 Spawned = 0;
		int32 Already = 0;
		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "NeuroArrivalLab", "Neuro arrival lab dressing"));

		for (const FNeuroArrivalStaleSpec& Spec : NeuroArrivalStale)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage);
			if (Matches.Num() == 0)
			{
				continue;
			}
			if (Matches.Num() != 1 || !LocationMatches(Matches[0]->GetActorLocation(), Spec.Location))
			{
				return FailAudit(TEXT("stale_mismatch"), FString::Printf(TEXT("%s changed during execute. ZERO remaining writes."), Spec.Label), MakeShared<FBridgeChange>(Change));
			}
			AActor* Actor = Matches[0];
			Actor->Modify();
			if (!Actor->Destroy())
			{
				return FailAudit(TEXT("destroy_failed"), FString::Printf(TEXT("Failed to destroy %s. ZERO remaining writes."), Spec.Label), MakeShared<FBridgeChange>(Change));
			}
			++Deleted;
		}

		for (const FNeuroArrivalPropSpec& Spec : NeuroArrivalProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && NeuroArrivalPropMismatch(Matches[0], Spec).IsEmpty())
			{
				++Already;
				continue;
			}
			FString SpawnError;
			if (!SpawnNeuroArrivalProp(World, TargetLevel, Spec, SpawnError))
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("%s %s ZERO remaining writes."), Spec.Label, *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++Spawned;
		}

		if (const FString KeepError = GuardNeuroArrivalKeepList(World); !KeepError.IsEmpty())
		{
			return FailAudit(TEXT("keep_list_failed"), FString::Printf(TEXT("%s"), *KeepError), MakeShared<FBridgeChange>(Change));
		}
		int32 StaleLeft = 0;
		if (const FString StaleError = GuardNeuroArrivalStale(World, StaleLeft); !StaleError.IsEmpty() || StaleLeft != 0)
		{
			return FailAudit(TEXT("stale_remain"), StaleError.IsEmpty() ? TEXT("Stale Avery/Sterling actors remain.") : StaleError, MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetNumberField(TEXT("deleted_stale"), Deleted);
		Change.After->SetNumberField(TEXT("spawned"), Spawned);
		Change.After->SetNumberField(TEXT("already_present"), Already);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
