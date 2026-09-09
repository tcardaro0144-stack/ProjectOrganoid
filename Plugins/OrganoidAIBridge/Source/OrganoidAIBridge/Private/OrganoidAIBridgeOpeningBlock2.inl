	const TCHAR* DoorLockLabel = TEXT("DoorLock_VestibuleToAtrium");
	const FVector DoorLockLocation(1000.f, 0.f, 100.f);
	const FRotator DoorLockRotation(0.f, 0.f, 0.f);
	const FVector DoorLockScale(1.f, 1.f, 1.f);
	const TCHAR* Block2CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

	struct FBlock2PropSpec
	{
		const TCHAR* Label;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bText;
		const TCHAR* Text;
	};

	const FBlock2PropSpec Block2Props[] = {
		{TEXT("Admin_Block2_Reception_Chair"), FVector(1380.f, -280.f, 50.f), FRotator(0.f, 0.f, 0.f), FVector(0.35f, 0.35f, 0.80f), false, nullptr},
		{TEXT("Admin_Block2_Reception_Mug"), FVector(1485.f, -220.f, 112.f), FRotator(0.f, 0.f, 0.f), FVector(0.08f, 0.08f, 0.10f), false, nullptr},
		{TEXT("Admin_Block2_Security_Chair"), FVector(2680.f, -510.f, 50.f), FRotator(0.f, 0.f, 0.f), FVector(0.35f, 0.35f, 0.80f), false, nullptr},
		{TEXT("Admin_Block2_Security_Mug"), FVector(2740.f, -360.f, 112.f), FRotator(0.f, 0.f, 0.f), FVector(0.08f, 0.08f, 0.10f), false, nullptr},
		{TEXT("Admin_Block2_Security_Headset"), FVector(2620.f, -360.f, 112.f), FRotator(0.f, 0.f, 0.f), FVector(0.22f, 0.14f, 0.06f), false, nullptr},
		{TEXT("Admin_Brand_PreparedImmunity"), FVector(1450.f, 495.f, 250.f), FRotator(0.f, -90.f, 0.f), FVector(1.f, 1.f, 1.f), true, TEXT("EPITOPE — Prepared Immunity.")},
		{TEXT("Admin_Brand_VisitExpected"), FVector(450.f, 380.f, 210.f), FRotator(0.f, -90.f, 0.f), FVector(1.f, 1.f, 1.f), true, TEXT("Your visit is expected. Your safety is routine.")},
	};

	FString DisableCollision(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Actor is null.");
		}
		Actor->SetActorEnableCollision(false);
		TArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive)
			{
				Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		return TEXT("");
	}

	FString ReadTextRender(AActor* Actor)
	{
		if (!Actor)
		{
			return FString();
		}
		if (UTextRenderComponent* Text = Actor->FindComponentByClass<UTextRenderComponent>())
		{
			return Text->Text.ToString();
		}
		return FString();
	}

	FString ApplyBrandText(AActor* Actor, const TCHAR* Text)
	{
		if (!Actor || !Text)
		{
			return TEXT("Brand actor or text is null.");
		}
		UTextRenderComponent* Render = Actor->FindComponentByClass<UTextRenderComponent>();
		if (!Render)
		{
			return TEXT("TextRenderComponent missing.");
		}
		Render->SetText(FText::FromString(Text));
		Render->SetWorldSize(28.f);
		Render->SetTextRenderColor(FColor(220, 224, 228));
		Render->SetHorizontalAlignment(EHTA_Center);
		Render->SetVerticalAlignment(EVRTA_TextCenter);
		return TEXT("");
	}

	FString ApplyCubeMesh(AActor* Actor)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor/component missing.");
		}
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, Block2CubeMeshPath);
		if (!Cube)
		{
			return TEXT("Failed to load /Engine/BasicShapes/Cube.Cube.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Cube);
		return TEXT("");
	}

	FString Block2PropMismatch(AActor* Actor, const FBlock2PropSpec& Spec)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not Admin"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, Spec.Location, Spec.Rotation, Spec.Scale); !Xform.IsEmpty())
		{
			return Xform;
		}
		if (Spec.bText)
		{
			const FString Live = ReadTextRender(Actor);
			if (!Live.Equals(Spec.Text, ESearchCase::CaseSensitive))
			{
				return FString::Printf(TEXT("text '%s' expected '%s'"), *Live, Spec.Text);
			}
		}
		return TEXT("");
	}

	FString GuardOpeningBlock2Shared(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin is not loaded. ZERO writes.");
		}
		FString GuardError = GuardExistingActor(World, ReceptionLabel, ReceptionLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, SecurityTerminalLabel, SecurityTerminalLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Security terminal guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door guard failed: %s"), *GuardError);
		}
		TArray<AActor*> Hologram = FindByExactLabel(World, TEXT("Admin_FacilityHologram"));
		if (Hologram.Num() != 1)
		{
			return FString::Printf(TEXT("Admin_FacilityHologram count=%d expected=1"), Hologram.Num());
		}
		return TEXT("");
	}

	FString PreflightSetAdminDoorlockInteractable(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. set_admin_doorlock_interactable does not save or compile.");
		}
		const FString Label = GetString(Args, TEXT("actor"), GetString(Args, TEXT("label"), DoorLockLabel));
		if (!Label.Equals(DoorLockLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("actor must be DoorLock_VestibuleToAtrium.");
		}

		UWorld* World = GetEditorWorld();
		const FString SharedError = GuardOpeningBlock2Shared(World);
		if (!SharedError.IsEmpty())
		{
			return SharedError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, DoorLockLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1. Keep the actor; do not spawn a duplicate."), DoorLockLabel, Matches.Num());
		}
		AActor* Lock = Matches[0];
		if (!PackagesEqual(ActorOwningPackage(Lock), AdminPackage))
		{
			return TEXT("DoorLock_VestibuleToAtrium must be owned by SL_Epitope_Admin.");
		}
		if (const FString Xform = TransformMismatch(Lock, DoorLockLocation, DoorLockRotation, DoorLockScale); !Xform.IsEmpty())
		{
			return FString::Printf(TEXT("DoorLock must stay in place. %s"), *Xform);
		}
		if (const FString BoolError = CheckBoolProperty(Lock, TEXT("bIsInteractable"), true); !BoolError.IsEmpty())
		{
			if (CheckBoolProperty(Lock, TEXT("bIsInteractable"), false).IsEmpty())
			{
				Before->SetBoolField(TEXT("already_applied"), true);
			}
			else
			{
				return FString::Printf(TEXT("bIsInteractable %s"), *BoolError);
			}
		}

		Before->SetStringField(TEXT("owning_package"), ActorOwningPackage(Lock));
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Lock));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetStringField(TEXT("label"), DoorLockLabel);
		Proposed->SetStringField(TEXT("property"), TEXT("bIsInteractable"));
		Proposed->SetBoolField(TEXT("expected"), true);
		Proposed->SetBoolField(TEXT("value"), false);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Set DoorLock_VestibuleToAtrium.bIsInteractable=false. Keep actor. Do not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetAdminDoorlockInteractable(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_admin_doorlock_interactable must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetAdminDoorlockInteractable(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		TArray<AActor*> Matches = World ? FindByExactLabel(World, DoorLockLabel) : TArray<AActor*>();
		if (Matches.Num() != 1)
		{
			return FailAudit(TEXT("not_found"), TEXT("DoorLock vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		AActor* Lock = Matches[0];
		if (CheckBoolProperty(Lock, TEXT("bIsInteractable"), false).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("already_applied"), true);
			Change.After->SetBoolField(TEXT("bIsInteractable"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "Block2DoorLock", "Block 2 DoorLock interactable"));
		Lock->Modify();
		if (const FString SetError = SetNamedPropertyFromBool(Lock, TEXT("bIsInteractable"), false); !SetError.IsEmpty())
		{
			return FailAudit(TEXT("write_failed"), FString::Printf(TEXT("%s ZERO remaining writes."), *SetError), MakeShared<FBridgeChange>(Change));
		}
		Lock->MarkPackageDirty();
		if (const FString AfterError = CheckBoolProperty(Lock, TEXT("bIsInteractable"), false); !AfterError.IsEmpty())
		{
			return FailAudit(TEXT("verify_failed"), FString::Printf(TEXT("%s"), *AfterError), MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), DoorLockLabel);
		Change.After->SetBoolField(TEXT("bIsInteractable"), false);
		Change.After->SetBoolField(TEXT("moved"), false);
		Change.After->SetBoolField(TEXT("deleted"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSpawnAdminBlock2Dressing(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_admin_block2_dressing does not save or compile.");
		}

		UWorld* World = GetEditorWorld();
		const FString SharedError = GuardOpeningBlock2Shared(World);
		if (!SharedError.IsEmpty())
		{
			return SharedError;
		}

		int32 Present = 0;
		for (const FBlock2PropSpec& Spec : Block2Props)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() > 1)
			{
				return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), Spec.Label, Matches.Num());
			}
			if (Matches.Num() == 1)
			{
				if (const FString Mismatch = Block2PropMismatch(Matches[0], Spec); !Mismatch.IsEmpty())
				{
					return FString::Printf(TEXT("Mismatched %s: %s. Abort."), Spec.Label, *Mismatch);
				}
				++Present;
			}
		}

		Before->SetStringField(TEXT("destination_package"), AdminPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("already_present"), Present);
		Proposed->SetNumberField(TEXT("prop_count"), UE_ARRAY_COUNT(Block2Props));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn approved Block 2 dressing and two branding lines into Admin. No collision. Does not save."));
		return TEXT("");
	}

	AActor* SpawnBlock2Prop(UWorld* World, ULevel* TargetLevel, const FBlock2PropSpec& Spec, FString& OutError)
	{
		OutError.Reset();
		UClass* Class = Spec.bText
			? ATextRenderActor::StaticClass()
			: AStaticMeshActor::StaticClass();
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AActor* Actor = World->SpawnActor<AActor>(Class, Spec.Location, Spec.Rotation, Params);
		if (!Actor)
		{
			OutError = FString::Printf(TEXT("SpawnActor failed for %s."), Spec.Label);
			return nullptr;
		}
		Actor->SetActorLabel(Spec.Label, true);
		Actor->SetActorScale3D(Spec.Scale);
		if (!Spec.bText)
		{
			if (const FString MeshError = ApplyCubeMesh(Actor); !MeshError.IsEmpty())
			{
				Actor->Destroy();
				OutError = MeshError;
				return nullptr;
			}
		}
		else if (const FString TextError = ApplyBrandText(Actor, Spec.Text); !TextError.IsEmpty())
		{
			Actor->Destroy();
			OutError = TextError;
			return nullptr;
		}
		DisableCollision(Actor);
		Actor->MarkPackageDirty();
		if (const FString AfterError = Block2PropMismatch(Actor, Spec); !AfterError.IsEmpty())
		{
			Actor->Destroy();
			OutError = AfterError;
			return nullptr;
		}
		return Actor;
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminBlock2Dressing(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_admin_block2_dressing must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnAdminBlock2Dressing(Change.Args, Before, Proposed);
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

		int32 Spawned = 0;
		int32 Already = 0;
		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "Block2Dressing", "Block 2 dressing and branding"));
		for (const FBlock2PropSpec& Spec : Block2Props)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && Block2PropMismatch(Matches[0], Spec).IsEmpty())
			{
				++Already;
				continue;
			}
			FString SpawnError;
			if (!SpawnBlock2Prop(World, TargetLevel, Spec, SpawnError))
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("%s %s ZERO remaining writes."), Spec.Label, *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++Spawned;
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetNumberField(TEXT("spawned"), Spawned);
		Change.After->SetNumberField(TEXT("already_present"), Already);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
