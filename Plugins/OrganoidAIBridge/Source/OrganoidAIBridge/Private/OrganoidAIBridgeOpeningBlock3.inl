	const TCHAR* Block3AmmoPickupLabel = TEXT("Pickup_Block3_PistolAmmo");
	const TCHAR* Block3TraumaPickupLabel = TEXT("Pickup_Block3_TraumaStabilizer");
	const TCHAR* Block3PickupClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidItemPickup");
	const TCHAR* Block3ItemClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidItemData");
	const TCHAR* Block3PistolAmmoObjectPath = TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo");
	const TCHAR* Block3TraumaItemPackage = TEXT("/Game/Data/Items/DA_Item_TraumaStabilizer");
	const TCHAR* Block3TraumaItemObjectPath = TEXT("/Game/Data/Items/DA_Item_TraumaStabilizer.DA_Item_TraumaStabilizer");
	const TCHAR* Block3TraumaDescription =
		TEXT("Compact emergency pack for traumatic injury. Stabilizes bleeding until proper care is available.");
	const FVector Block3AmmoLocation(2560.f, -340.f, 80.f);
	const FVector Block3TraumaLocation(2760.f, -300.f, 80.f);
	const FVector Block3HostStaging(2680.f, -470.f, 100.f);
	const FVector Block3RwKeycardLocation(2580.f, -560.f, 80.f);
	const FRotator Block3PickupRotation(0.f, 0.f, 0.f);
	const FVector Block3PickupScale(1.f, 1.f, 1.f);

	UObject* LoadBlock3PistolAmmo()
	{
		return StaticLoadObject(UObject::StaticClass(), nullptr, Block3PistolAmmoObjectPath);
	}

	UObject* LoadBlock3TraumaItem()
	{
		return StaticLoadObject(UObject::StaticClass(), nullptr, Block3TraumaItemObjectPath);
	}

	FString Block3TraumaMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Contains(TEXT("ProjectOrganoidItemData")))
		{
			return FString::Printf(TEXT("class '%s' is not UProjectOrganoidItemData"), *ClassName(Asset));
		}

		auto MatchString = [Asset](const TCHAR* PropertyName, const TCHAR* Expected) -> FString
		{
			FProperty* Property = FindInstanceProperty(Asset, PropertyName);
			if (!Property)
			{
				return FString::Printf(TEXT("missing %s"), PropertyName);
			}
			FString Error;
			if (!PropertyMatchesJson(Asset, Property, MakeShared<FJsonValueString>(Expected), Error))
			{
				return Error;
			}
			return TEXT("");
		};
		auto MatchBool = [Asset](const TCHAR* PropertyName, bool bExpected) -> FString
		{
			FProperty* Property = FindInstanceProperty(Asset, PropertyName);
			if (!Property)
			{
				return FString::Printf(TEXT("missing %s"), PropertyName);
			}
			FString Error;
			if (!PropertyMatchesJson(Asset, Property, MakeShared<FJsonValueBoolean>(bExpected), Error))
			{
				return Error;
			}
			return TEXT("");
		};
		auto MatchNumber = [Asset](const TCHAR* PropertyName, double Expected) -> FString
		{
			FProperty* Property = FindInstanceProperty(Asset, PropertyName);
			if (!Property)
			{
				return FString::Printf(TEXT("missing %s"), PropertyName);
			}
			FString Error;
			if (!PropertyMatchesJson(Asset, Property, MakeShared<FJsonValueNumber>(Expected), Error))
			{
				return Error;
			}
			return TEXT("");
		};

		if (const FString Error = MatchString(TEXT("ItemName"), TEXT("Trauma Stabilizer")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchString(TEXT("Description"), Block3TraumaDescription); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchString(TEXT("ItemType"), TEXT("Consumable")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchNumber(TEXT("HealAmount"), 35.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchBool(TEXT("bCanStack"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchNumber(TEXT("MaxStackCount"), 3.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchNumber(TEXT("GridWidth"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchNumber(TEXT("GridHeight"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchNumber(TEXT("ItemWeight"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString ApplyBlock3TraumaDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Item asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("ItemName"), TEXT("Trauma Stabilizer")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("Description"), Block3TraumaDescription); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("ItemType"), TEXT("Consumable")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Asset, TEXT("HealAmount"), 35.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Asset, TEXT("bCanStack"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Asset, TEXT("MaxStackCount"), 3.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Asset, TEXT("GridWidth"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Asset, TEXT("GridHeight"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Asset, TEXT("ItemWeight"), 1.0); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString CreateBlock3TraumaItemAsset(UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* ItemClass = LoadClass<UObject>(nullptr, Block3ItemClassPath);
		if (!ItemClass)
		{
			return TEXT("UProjectOrganoidItemData class is not loaded.");
		}
		UPackage* Package = CreatePackage(Block3TraumaItemPackage);
		if (!Package)
		{
			return TEXT("Failed to create DA_Item_TraumaStabilizer package.");
		}
		UObject* Asset = NewObject<UObject>(
			Package, ItemClass, TEXT("DA_Item_TraumaStabilizer"), RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject DA_Item_TraumaStabilizer returned null.");
		}
		if (const FString ApplyError = ApplyBlock3TraumaDefaults(Asset); !ApplyError.IsEmpty())
		{
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		Asset->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString Block3PickupMismatchReason(AActor* Actor, UObject* ExpectedItem, const FVector& ExpectedLocation, int32 ExpectedQty)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!Actor->GetClass() || !Actor->GetClass()->GetPathName().Contains(TEXT("ProjectOrganoidItemPickup")))
		{
			return FString::Printf(TEXT("class '%s' is not AProjectOrganoidItemPickup"), *ClassName(Actor));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
		{
			return FString::Printf(TEXT("owning package '%s' is not Admin"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, ExpectedLocation, Block3PickupRotation, Block3PickupScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		UObject* LiveItem = ReadPickupItemData(Actor);
		if (!LiveItem)
		{
			return TEXT("ItemData is not assigned.");
		}
		if (ExpectedItem && LiveItem != ExpectedItem)
		{
			return FString::Printf(TEXT("ItemData '%s' is not the approved asset"), *LiveItem->GetPathName());
		}
		FProperty* QtyProp = FindInstanceProperty(Actor, TEXT("Quantity"));
		FString QtyError;
		if (!PropertyMatchesJson(Actor, QtyProp, MakeShared<FJsonValueNumber>(static_cast<double>(ExpectedQty)), QtyError))
		{
			return QtyError;
		}
		FProperty* DestroyProp = FindInstanceProperty(Actor, TEXT("bDestroyOnPickup"));
		FString DestroyError;
		if (!PropertyMatchesJson(Actor, DestroyProp, MakeShared<FJsonValueBoolean>(true), DestroyError))
		{
			return DestroyError;
		}
		return TEXT("");
	}

	bool Block3OverlapsHostStaging(const FVector& Location)
	{
		return FVector::Dist2D(Location, Block3HostStaging) < 80.f;
	}

	FString GuardOpeningBlock3Shared(UWorld* World)
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
			return FString::Printf(TEXT("Admin_FacilityHologram count=%d expected=1. Abort."), Hologram.Num());
		}
		TArray<AActor*> Rw = FindByExactLabel(World, TEXT("Pickup_ResearchWingKeycard"));
		if (Rw.Num() != 1)
		{
			return FString::Printf(TEXT("Pickup_ResearchWingKeycard count=%d expected=1. Abort."), Rw.Num());
		}
		if (!LocationMatches(Rw[0]->GetActorLocation(), Block3RwKeycardLocation))
		{
			return TEXT("Pickup_ResearchWingKeycard moved. Abort.");
		}
		TArray<AActor*> Chair = FindByExactLabel(World, TEXT("Admin_Block2_Security_Chair"));
		if (Chair.Num() != 1)
		{
			return FString::Printf(TEXT("Admin_Block2_Security_Chair count=%d. Do not move Block 2 dressing."), Chair.Num());
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || !Actor->GetClass())
			{
				continue;
			}
			if (Actor->GetClass()->GetName().Contains(TEXT("ProjectOrganoidHost")))
			{
				if (PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
				{
					return TEXT("Admin already contains a Host. Abort.");
				}
			}
		}
		if (Block3OverlapsHostStaging(Block3AmmoLocation) || Block3OverlapsHostStaging(Block3TraumaLocation))
		{
			return TEXT("Approved pickup occupies Host staging. Abort.");
		}
		return TEXT("");
	}

	FString GuardBlock3PickupSlot(UWorld* World, const TCHAR* Label, const FVector& Location, UObject* ExpectedItem, int32 ExpectedQty)
	{
		TArray<AActor*> Existing = FindByExactLabel(World, Label);
		TArray<AActor*> Owned = FindOwnedByExactLabel(World, Label, AdminPackage);
		if (Existing.Num() > 1 || Owned.Num() > 1)
		{
			return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), Label, Existing.Num());
		}
		if (Existing.Num() == 1 && Owned.Num() == 0)
		{
			return FString::Printf(TEXT("%s exists outside Admin. Abort."), Label);
		}
		if (Owned.Num() == 1)
		{
			if (const FString Mismatch = Block3PickupMismatchReason(Owned[0], ExpectedItem, Location, ExpectedQty); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Mismatched %s: %s. Abort."), Label, *Mismatch);
			}
		}
		return TEXT("");
	}

	FString PreflightSpawnAdminBlock3Resources(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_admin_block3_resources does not save or compile.");
		}

		UWorld* World = GetEditorWorld();
		const FString SharedError = GuardOpeningBlock3Shared(World);
		if (!SharedError.IsEmpty())
		{
			return SharedError;
		}

		UObject* AmmoAsset = LoadBlock3PistolAmmo();
		if (!AmmoAsset)
		{
			return TEXT("DA_Item_PistolAmmo is missing. Do not retune or recreate it in this action.");
		}

		UObject* TraumaAsset = LoadBlock3TraumaItem();
		if (TraumaAsset)
		{
			if (const FString ItemError = Block3TraumaMismatchReason(TraumaAsset); !ItemError.IsEmpty())
			{
				return FString::Printf(TEXT("Existing DA_Item_TraumaStabilizer mismatch: %s. Abort."), *ItemError);
			}
		}
		else if (!LoadClass<UObject>(nullptr, Block3ItemClassPath))
		{
			return TEXT("UProjectOrganoidItemData class is not loaded.");
		}

		if (const FString AmmoSlot = GuardBlock3PickupSlot(World, Block3AmmoPickupLabel, Block3AmmoLocation, AmmoAsset, 8);
			!AmmoSlot.IsEmpty())
		{
			return AmmoSlot;
		}
		if (const FString TraumaSlot = GuardBlock3PickupSlot(World, Block3TraumaPickupLabel, Block3TraumaLocation, TraumaAsset, 1);
			!TraumaSlot.IsEmpty())
		{
			return TraumaSlot;
		}
		if (!LoadClass<AActor>(nullptr, Block3PickupClassPath))
		{
			return TEXT("AProjectOrganoidItemPickup class is not loaded.");
		}

		Before->SetStringField(TEXT("destination_package"), AdminPackage);
		Before->SetStringField(TEXT("item_package"), Block3TraumaItemPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("trauma_already_present"), TraumaAsset != nullptr);
		Before->SetNumberField(TEXT("existing_ammo_pickup"), FindByExactLabel(World, Block3AmmoPickupLabel).Num());
		Before->SetNumberField(TEXT("existing_trauma_pickup"), FindByExactLabel(World, Block3TraumaPickupLabel).Num());
		Proposed->SetStringField(TEXT("ammo_label"), Block3AmmoPickupLabel);
		Proposed->SetStringField(TEXT("trauma_label"), Block3TraumaPickupLabel);
		Proposed->SetStringField(TEXT("ammo_item_path"), Block3PistolAmmoObjectPath);
		Proposed->SetStringField(TEXT("trauma_item_path"), Block3TraumaItemObjectPath);
		Proposed->SetArrayField(TEXT("ammo_location"), Vec(Block3AmmoLocation));
		Proposed->SetArrayField(TEXT("trauma_location"), Vec(Block3TraumaLocation));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Create DA_Item_TraumaStabilizer if missing and spawn Block 3 pickups into Admin. Does not save."));
		return TEXT("");
	}

	AActor* SpawnBlock3Pickup(
		UWorld* World,
		ULevel* TargetLevel,
		const TCHAR* Label,
		const FVector& Location,
		UObject* ItemAsset,
		int32 Quantity,
		FString& OutError)
	{
		OutError.Reset();
		UClass* PickupClass = LoadClass<AActor>(nullptr, Block3PickupClassPath);
		if (!PickupClass)
		{
			OutError = TEXT("AProjectOrganoidItemPickup class vanished.");
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AActor* Pickup = World->SpawnActor<AActor>(PickupClass, Location, Block3PickupRotation, Params);
		if (!Pickup)
		{
			OutError = FString::Printf(TEXT("SpawnActor failed for %s."), Label);
			return nullptr;
		}
		Pickup->SetActorLabel(Label, true);
		Pickup->SetActorScale3D(Block3PickupScale);
		if (const FString AssignError = AssignPickupItemData(Pickup, ItemAsset); !AssignError.IsEmpty())
		{
			Pickup->Destroy();
			OutError = AssignError;
			return nullptr;
		}
		if (const FString QtyError = SetNamedPropertyFromNumber(Pickup, TEXT("Quantity"), static_cast<double>(Quantity)); !QtyError.IsEmpty())
		{
			Pickup->Destroy();
			OutError = QtyError;
			return nullptr;
		}
		if (const FString DestroyError = SetNamedPropertyFromBool(Pickup, TEXT("bDestroyOnPickup"), true); !DestroyError.IsEmpty())
		{
			Pickup->Destroy();
			OutError = DestroyError;
			return nullptr;
		}
		Pickup->MarkPackageDirty();
		if (const FString AfterError = Block3PickupMismatchReason(Pickup, ItemAsset, Location, Quantity); !AfterError.IsEmpty())
		{
			Pickup->Destroy();
			OutError = AfterError;
			return nullptr;
		}
		return Pickup;
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminBlock3Resources(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_admin_block3_resources must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnAdminBlock3Resources(Change.Args, Before, Proposed);
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

		UObject* AmmoAsset = LoadBlock3PistolAmmo();
		if (!AmmoAsset)
		{
			return FailAudit(TEXT("item_missing"), TEXT("DA_Item_PistolAmmo vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UObject* TraumaAsset = LoadBlock3TraumaItem();
		bool bCreatedItem = false;
		if (!TraumaAsset)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateTraumaStabilizer", "Create Trauma Stabilizer"));
			if (const FString CreateError = CreateBlock3TraumaItemAsset(TraumaAsset); !CreateError.IsEmpty())
			{
				return FailAudit(TEXT("item_create_failed"), FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError), MakeShared<FBridgeChange>(Change));
			}
			bCreatedItem = true;
		}
		if (const FString ItemError = Block3TraumaMismatchReason(TraumaAsset); !ItemError.IsEmpty())
		{
			return FailAudit(TEXT("item_invalid"), FString::Printf(TEXT("%s ZERO remaining writes."), *ItemError), MakeShared<FBridgeChange>(Change));
		}

		int32 Spawned = 0;
		int32 Already = 0;
		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "Block3Resources", "Block 3 resource pickups"));

		TArray<AActor*> AmmoOwned = FindOwnedByExactLabel(World, Block3AmmoPickupLabel, AdminPackage);
		AActor* AmmoPickup = nullptr;
		if (AmmoOwned.Num() == 1 && Block3PickupMismatchReason(AmmoOwned[0], AmmoAsset, Block3AmmoLocation, 8).IsEmpty())
		{
			AmmoPickup = AmmoOwned[0];
			++Already;
		}
		else
		{
			FString SpawnError;
			AmmoPickup = SpawnBlock3Pickup(World, TargetLevel, Block3AmmoPickupLabel, Block3AmmoLocation, AmmoAsset, 8, SpawnError);
			if (!AmmoPickup)
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("Ammo %s ZERO remaining writes."), *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++Spawned;
		}

		TArray<AActor*> TraumaOwned = FindOwnedByExactLabel(World, Block3TraumaPickupLabel, AdminPackage);
		AActor* TraumaPickup = nullptr;
		if (TraumaOwned.Num() == 1 && Block3PickupMismatchReason(TraumaOwned[0], TraumaAsset, Block3TraumaLocation, 1).IsEmpty())
		{
			TraumaPickup = TraumaOwned[0];
			++Already;
		}
		else
		{
			FString SpawnError;
			TraumaPickup = SpawnBlock3Pickup(World, TargetLevel, Block3TraumaPickupLabel, Block3TraumaLocation, TraumaAsset, 1, SpawnError);
			if (!TraumaPickup)
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("Trauma %s ZERO remaining writes."), *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++Spawned;
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("ammo_label"), Block3AmmoPickupLabel);
		Change.After->SetStringField(TEXT("trauma_label"), Block3TraumaPickupLabel);
		Change.After->SetStringField(TEXT("ammo_item_path"), AmmoAsset->GetPathName());
		Change.After->SetStringField(TEXT("trauma_item_path"), TraumaAsset ? TraumaAsset->GetPathName() : FString());
		Change.After->SetArrayField(TEXT("ammo_location"), Vec(AmmoPickup->GetActorLocation()));
		Change.After->SetArrayField(TEXT("trauma_location"), Vec(TraumaPickup->GetActorLocation()));
		Change.After->SetBoolField(TEXT("item_created"), bCreatedItem);
		Change.After->SetNumberField(TEXT("spawned"), Spawned);
		Change.After->SetNumberField(TEXT("already_present"), Already);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
