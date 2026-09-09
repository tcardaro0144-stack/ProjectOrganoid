	const TCHAR* ResearchWingPickupLabel = TEXT("Pickup_ResearchWingKeycard");
	const TCHAR* ResearchWingPickupClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidItemPickup");
	const TCHAR* ResearchWingItemClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidItemData");
	const TCHAR* ResearchWingItemPackage = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard");
	const TCHAR* ResearchWingItemObjectPath = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard.DA_Item_ResearchWingKeycard");
	const FVector ResearchWingPickupLocation(2580.f, -560.f, 80.f);
	const FRotator ResearchWingPickupRotation(0.f, 0.f, 0.f);
	const FVector ResearchWingPickupScale(1.f, 1.f, 1.f);

	UObject* LoadResearchWingItemAsset()
	{
		return StaticLoadObject(UObject::StaticClass(), nullptr, ResearchWingItemObjectPath);
	}

	FString SetNamedPropertyFromString(UObject* Object, const TCHAR* PropertyName, const FString& Value)
	{
		FProperty* Property = FindInstanceProperty(Object, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		FString Error;
		if (!SetPropertyFromJson(Object, Property, MakeShared<FJsonValueString>(Value), Error))
		{
			return Error;
		}
		return TEXT("");
	}

	FString SetNamedPropertyFromNumber(UObject* Object, const TCHAR* PropertyName, double Value)
	{
		FProperty* Property = FindInstanceProperty(Object, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		FString Error;
		if (!SetPropertyFromJson(Object, Property, MakeShared<FJsonValueNumber>(Value), Error))
		{
			return Error;
		}
		return TEXT("");
	}

	FString SetNamedPropertyFromBool(UObject* Object, const TCHAR* PropertyName, bool bValue)
	{
		FProperty* Property = FindInstanceProperty(Object, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		FString Error;
		if (!SetPropertyFromJson(Object, Property, MakeShared<FJsonValueBoolean>(bValue), Error))
		{
			return Error;
		}
		return TEXT("");
	}

	FString ApplyResearchWingItemDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Item asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("ItemName"), TEXT("Research Wing Keycard")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("ItemType"), TEXT("KeyItem")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("SecurityTier"), TEXT("Level2_Lab")); !Error.IsEmpty())
		{
			return Error;
		}
		FString Error = SetNamedPropertyFromBool(Asset, TEXT("bBroadcastGenericKeycardObjectiveEvent"), false);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = SetNamedPropertyFromNumber(Asset, TEXT("GridWidth"), 1.0);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = SetNamedPropertyFromNumber(Asset, TEXT("GridHeight"), 1.0);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = SetNamedPropertyFromNumber(Asset, TEXT("ItemWeight"), 0.2);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = SetNamedPropertyFromBool(Asset, TEXT("bCanStack"), false);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString ResearchWingItemMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Contains(TEXT("ProjectOrganoidItemData")))
		{
			return FString::Printf(TEXT("class '%s' is not UProjectOrganoidItemData"), *ClassName(Asset));
		}
		if (!PackagesEqual(Asset->GetOutermost() ? Asset->GetOutermost()->GetName() : FString(), ResearchWingItemPackage))
		{
			return FString::Printf(TEXT("package '%s' is not the Research Wing item DA"),
				Asset->GetOutermost() ? *Asset->GetOutermost()->GetName() : TEXT("none"));
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

		if (const FString Error = MatchString(TEXT("ItemName"), TEXT("Research Wing Keycard")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchString(TEXT("ItemType"), TEXT("KeyItem")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchString(TEXT("SecurityTier"), TEXT("Level2_Lab")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = MatchBool(TEXT("bBroadcastGenericKeycardObjectiveEvent"), false); !Error.IsEmpty())
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
		return TEXT("");
	}

	FString AssignPickupItemData(AActor* Pickup, UObject* ItemAsset)
	{
		if (!Pickup || !ItemAsset)
		{
			return TEXT("Pickup or item asset is null.");
		}
		FProperty* Property = FindInstanceProperty(Pickup, TEXT("ItemData"));
		FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property);
		if (!ObjectProp)
		{
			return TEXT("ItemData object property missing on pickup.");
		}
		if (ObjectProp->PropertyClass && !ItemAsset->IsA(ObjectProp->PropertyClass))
		{
			return TEXT("Item asset is not compatible with ItemData.");
		}
		ObjectProp->SetObjectPropertyValue_InContainer(Pickup, ItemAsset);
		return TEXT("");
	}

	UObject* ReadPickupItemData(AActor* Pickup)
	{
		if (!Pickup)
		{
			return nullptr;
		}
		FProperty* Property = FindInstanceProperty(Pickup, TEXT("ItemData"));
		if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
		{
			return ObjectProp->GetObjectPropertyValue_InContainer(Pickup);
		}
		return nullptr;
	}

	FString ResearchWingPickupMismatchReason(AActor* Actor, UObject* ExpectedItem)
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
		if (const FString Xform = TransformMismatch(Actor, ResearchWingPickupLocation, ResearchWingPickupRotation, ResearchWingPickupScale);
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
			return FString::Printf(TEXT("ItemData '%s' is not DA_Item_ResearchWingKeycard"), *LiveItem->GetPathName());
		}
		if (const FString ItemError = ResearchWingItemMismatchReason(LiveItem); !ItemError.IsEmpty())
		{
			return FString::Printf(TEXT("ItemData %s"), *ItemError);
		}
		FProperty* QtyProp = FindInstanceProperty(Actor, TEXT("Quantity"));
		FString QtyError;
		if (!PropertyMatchesJson(Actor, QtyProp, MakeShared<FJsonValueNumber>(1.0), QtyError))
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

	FString CreateResearchWingItemAsset(UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* ItemClass = LoadClass<UObject>(nullptr, ResearchWingItemClassPath);
		if (!ItemClass)
		{
			return TEXT("UProjectOrganoidItemData class is not loaded.");
		}
		UPackage* Package = CreatePackage(ResearchWingItemPackage);
		if (!Package)
		{
			return TEXT("Failed to create DA_Item_ResearchWingKeycard package.");
		}
		UObject* Asset = NewObject<UObject>(
			Package, ItemClass, TEXT("DA_Item_ResearchWingKeycard"), RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject DA_Item_ResearchWingKeycard returned null.");
		}
		if (const FString ApplyError = ApplyResearchWingItemDefaults(Asset); !ApplyError.IsEmpty())
		{
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		Asset->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightSpawnAdminResearchWingKeycard(
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
			return TEXT("save/compile must be false. spawn_admin_research_wing_keycard does not save or compile.");
		}

		const FString Label = GetString(Args, TEXT("label"), ResearchWingPickupLabel);
		if (!Label.Equals(ResearchWingPickupLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("label must be Pickup_ResearchWingKeycard.");
		}

		FVector Location = ResearchWingPickupLocation;
		if (GetVector(Args, TEXT("location"), Location) && !LocationMatches(Location, ResearchWingPickupLocation))
		{
			return TEXT("location must be (2580, -560, 80).");
		}

		FVector RotationVec = FVector(0.f, 0.f, 0.f);
		if (GetVector(Args, TEXT("rotation"), RotationVec)
			&& !LocationMatches(RotationVec, FVector(0.f, 0.f, 0.f)))
		{
			return TEXT("rotation must be (0, 0, 0).");
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

		TArray<AActor*> AdminCards = FindByExactLabel(World, TEXT("Pickup_AdminKeycard"));
		if (AdminCards.Num() != 1)
		{
			return FString::Printf(TEXT("Pickup_AdminKeycard count=%d expected=1. Abort."), AdminCards.Num());
		}
		if (FVector::Dist(AdminCards[0]->GetActorLocation(), ResearchWingPickupLocation) < 200.f)
		{
			return TEXT("Approved location overlaps Pickup_AdminKeycard. Abort.");
		}

		TArray<AActor*> Existing = FindByExactLabel(World, Label);
		TArray<AActor*> Owned = FindOwnedByExactLabel(World, Label, AdminPackage);
		if (Existing.Num() > 1 || Owned.Num() > 1)
		{
			return FString::Printf(TEXT("Pickup_ResearchWingKeycard count=%d. Abort rather than stack."), Existing.Num());
		}
		if (Existing.Num() == 1 && Owned.Num() == 0)
		{
			return TEXT("Pickup_ResearchWingKeycard exists outside Admin. Abort.");
		}

		UObject* ItemAsset = LoadResearchWingItemAsset();
		if (ItemAsset)
		{
			if (const FString ItemError = ResearchWingItemMismatchReason(ItemAsset); !ItemError.IsEmpty())
			{
				return FString::Printf(TEXT("Existing DA_Item_ResearchWingKeycard mismatch: %s. Abort."), *ItemError);
			}
		}
		else if (!LoadClass<UObject>(nullptr, ResearchWingItemClassPath))
		{
			return TEXT("UProjectOrganoidItemData class is not loaded.");
		}

		if (Owned.Num() == 1)
		{
			if (const FString Mismatch = ResearchWingPickupMismatchReason(Owned[0], ItemAsset); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Mismatched Pickup_ResearchWingKeycard: %s. Abort."), *Mismatch);
			}
		}
		else if (!LoadClass<AActor>(nullptr, ResearchWingPickupClassPath))
		{
			return TEXT("AProjectOrganoidItemPickup class is not loaded.");
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetStringField(TEXT("item_package"), ResearchWingItemPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("existing_pickup_count"), Existing.Num());
		Before->SetBoolField(TEXT("item_already_present"), ItemAsset != nullptr);
		Before->SetBoolField(TEXT("pickup_already_present"), Owned.Num() == 1);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetStringField(TEXT("class"), TEXT("ProjectOrganoidItemPickup"));
		Proposed->SetStringField(TEXT("item_path"), ResearchWingItemObjectPath);
		Proposed->SetArrayField(TEXT("location"), Vec(ResearchWingPickupLocation));
		Proposed->SetArrayField(TEXT("rotation"), Vec(FVector(0.f, 0.f, 0.f)));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Create DA_Item_ResearchWingKeycard if missing and spawn Pickup_ResearchWingKeycard into Admin. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminResearchWingKeycard(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_admin_research_wing_keycard must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnAdminResearchWingKeycard(Change.Args, Before, Proposed);
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

		UObject* ItemAsset = LoadResearchWingItemAsset();
		bool bCreatedItem = false;
		if (!ItemAsset)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateResearchWingKeycard", "Create Research Wing Keycard"));
			if (const FString CreateError = CreateResearchWingItemAsset(ItemAsset); !CreateError.IsEmpty())
			{
				return FailAudit(TEXT("item_create_failed"), FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError), MakeShared<FBridgeChange>(Change));
			}
			bCreatedItem = true;
		}

		if (const FString ItemError = ResearchWingItemMismatchReason(ItemAsset); !ItemError.IsEmpty())
		{
			return FailAudit(TEXT("item_invalid"), FString::Printf(TEXT("%s ZERO remaining writes."), *ItemError), MakeShared<FBridgeChange>(Change));
		}

		TArray<AActor*> Owned = FindOwnedByExactLabel(World, ResearchWingPickupLabel, AdminPackage);
		AActor* Pickup = nullptr;
		bool bSpawnedNow = false;
		if (Owned.Num() == 1 && ResearchWingPickupMismatchReason(Owned[0], ItemAsset).IsEmpty())
		{
			Pickup = Owned[0];
		}
		else
		{
			UClass* PickupClass = LoadClass<AActor>(nullptr, ResearchWingPickupClassPath);
			if (!PickupClass)
			{
				return FailAudit(TEXT("class_missing"), TEXT("AProjectOrganoidItemPickup class vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}

			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnResearchWingKeycard", "Spawn Research Wing Keycard"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Pickup = World->SpawnActor<AActor>(PickupClass, ResearchWingPickupLocation, ResearchWingPickupRotation, Params);
			if (!Pickup)
			{
				return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor AProjectOrganoidItemPickup returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
			}
			Pickup->SetActorLabel(ResearchWingPickupLabel, true);
			Pickup->SetActorScale3D(ResearchWingPickupScale);
			if (const FString AssignError = AssignPickupItemData(Pickup, ItemAsset); !AssignError.IsEmpty())
			{
				Pickup->Destroy();
				return FailAudit(TEXT("assign_failed"), FString::Printf(TEXT("%s Actor destroyed. ZERO remaining writes."), *AssignError), MakeShared<FBridgeChange>(Change));
			}
			if (const FString QtyError = SetNamedPropertyFromNumber(Pickup, TEXT("Quantity"), 1.0); !QtyError.IsEmpty())
			{
				Pickup->Destroy();
				return FailAudit(TEXT("assign_failed"), FString::Printf(TEXT("%s Actor destroyed. ZERO remaining writes."), *QtyError), MakeShared<FBridgeChange>(Change));
			}
			if (const FString DestroyError = SetNamedPropertyFromBool(Pickup, TEXT("bDestroyOnPickup"), true); !DestroyError.IsEmpty())
			{
				Pickup->Destroy();
				return FailAudit(TEXT("assign_failed"), FString::Printf(TEXT("%s Actor destroyed. ZERO remaining writes."), *DestroyError), MakeShared<FBridgeChange>(Change));
			}
			Pickup->MarkPackageDirty();
			bSpawnedNow = true;

			if (const FString AfterError = ResearchWingPickupMismatchReason(Pickup, ItemAsset); !AfterError.IsEmpty())
			{
				Pickup->Destroy();
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
		Change.After->SetStringField(TEXT("label"), ResearchWingPickupLabel);
		Change.After->SetStringField(TEXT("class"), ClassName(Pickup));
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Pickup));
		Change.After->SetStringField(TEXT("item_path"), ItemAsset ? ItemAsset->GetPathName() : FString());
		Change.After->SetArrayField(TEXT("location"), Vec(Pickup->GetActorLocation()));
		Change.After->SetBoolField(TEXT("item_created"), bCreatedItem);
		Change.After->SetBoolField(TEXT("already_present"), !bSpawnedNow);
		Change.After->SetBoolField(TEXT("spawned"), bSpawnedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
