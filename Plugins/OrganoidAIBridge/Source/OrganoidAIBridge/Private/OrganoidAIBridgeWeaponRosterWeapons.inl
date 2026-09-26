// Create the five roster weapon data assets. One class, distinct jobs. Does not save. Not the Lytic Cannon.
	struct FWeaponRosterSpec
	{
		const TCHAR* Spec = nullptr;
		const TCHAR* Action = nullptr;
		const TCHAR* Package = nullptr;
		const TCHAR* ObjectPath = nullptr;
		const TCHAR* AssetName = nullptr;
		const TCHAR* WeaponId = nullptr;
		const TCHAR* Title = nullptr;
		const TCHAR* Description = nullptr;
		const TCHAR* Role = nullptr;
		const TCHAR* Ammo = nullptr;
		const TCHAR* Effect = nullptr;
		float Damage = 0.0f;
		float Range = 0.0f;
		float FireRate = 0.0f;
		int32 Magazine = 0;
	};

	const TCHAR* WeaponRosterClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidWeaponData");

	const FWeaponRosterSpec WeaponRosterSpecs[] = {
		{
			TEXT("weapon_biostabilizer_pistol_v1"),
			TEXT("create_weapon_biostabilizer_pistol"),
			TEXT("/Game/Data/Weapons/DA_Weapon_BioStabilizerPistol"),
			TEXT("/Game/Data/Weapons/DA_Weapon_BioStabilizerPistol.DA_Weapon_BioStabilizerPistol"),
			TEXT("DA_Weapon_BioStabilizerPistol"),
			TEXT("Weapon_BioStabilizerPistol"),
			TEXT("Bio-Stabilizer Pistol"),
			TEXT("Precise sidearm that stabilizes small hosts and stuns"),
			TEXT("Sidearm, precise"),
			TEXT("Pistol"),
			TEXT("Stun"),
			8.0f, 4500.0f, 3.0f, 4
		},
		{
			TEXT("weapon_pulse_carbine_v1"),
			TEXT("create_weapon_pulse_carbine"),
			TEXT("/Game/Data/Weapons/DA_Weapon_PulseCarbine"),
			TEXT("/Game/Data/Weapons/DA_Weapon_PulseCarbine.DA_Weapon_PulseCarbine"),
			TEXT("DA_Weapon_PulseCarbine"),
			TEXT("Weapon_PulseCarbine"),
			TEXT("Pulse Carbine"),
			TEXT("Mid-range tactical carbine that targets biological weak points"),
			TEXT("Mid-range, reliable"),
			TEXT("Rifle"),
			TEXT("WeakPoint"),
			16.0f, 8000.0f, 6.0f, 8
		},
		{
			TEXT("weapon_cryo_injector_v1"),
			TEXT("create_weapon_cryo_injector"),
			TEXT("/Game/Data/Weapons/DA_Weapon_CryoInjector"),
			TEXT("/Game/Data/Weapons/DA_Weapon_CryoInjector.DA_Weapon_CryoInjector"),
			TEXT("DA_Weapon_CryoInjector"),
			TEXT("Weapon_CryoInjector"),
			TEXT("Cryo Injector"),
			TEXT("Applies cryo effect to slow/impair movement"),
			TEXT("Crowd control, slow"),
			TEXT("Special"),
			TEXT("CryoSlow"),
			4.0f, 3000.0f, 2.0f, 3
		},
		{
			TEXT("weapon_denaturing_shotgun_v1"),
			TEXT("create_weapon_denaturing_shotgun"),
			TEXT("/Game/Data/Weapons/DA_Weapon_DenaturingShotgun"),
			TEXT("/Game/Data/Weapons/DA_Weapon_DenaturingShotgun.DA_Weapon_DenaturingShotgun"),
			TEXT("DA_Weapon_DenaturingShotgun"),
			TEXT("Weapon_DenaturingShotgun"),
			TEXT("Denaturing Shotgun"),
			TEXT("Close-range spread that denatures tissue and staggers"),
			TEXT("Close-range, stagger"),
			TEXT("Shotgun"),
			TEXT("Stagger"),
			26.0f, 900.0f, 1.0f, 4
		},
		{
			TEXT("weapon_incinerator_lance_v1"),
			TEXT("create_weapon_incinerator_lance"),
			TEXT("/Game/Data/Weapons/DA_Weapon_IncineratorLance"),
			TEXT("/Game/Data/Weapons/DA_Weapon_IncineratorLance.DA_Weapon_IncineratorLance"),
			TEXT("DA_Weapon_IncineratorLance"),
			TEXT("Weapon_IncineratorLance"),
			TEXT("Incinerator Lance"),
			TEXT("Fire-based lance that burns tissue and area denial"),
			TEXT("Area denial, burn"),
			TEXT("Special"),
			TEXT("Burn"),
			14.0f, 2200.0f, 1.5f, 3
		}
	};

	const FWeaponRosterSpec* FindWeaponRosterSpec(const FString& Action)
	{
		for (const FWeaponRosterSpec& Spec : WeaponRosterSpecs)
		{
			if (Action.Equals(Spec.Action, ESearchCase::CaseSensitive))
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	UObject* FindWeaponRosterAssetExact(const FWeaponRosterSpec& Spec)
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, Spec.ObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(Spec.Package))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, Spec.ObjectPath);
		}
		return nullptr;
	}

	FString WeaponRosterWriteFloat(UObject* Asset, const TCHAR* Name, float Value)
	{
		FFloatProperty* Prop = CastField<FFloatProperty>(FindInstanceProperty(Asset, Name));
		if (!Prop)
		{
			return FString::Printf(TEXT("%s float missing."), Name);
		}
		Prop->SetPropertyValue_InContainer(Asset, Value);
		return TEXT("");
	}

	FString WeaponRosterWriteInt(UObject* Asset, const TCHAR* Name, int32 Value)
	{
		FIntProperty* Prop = CastField<FIntProperty>(FindInstanceProperty(Asset, Name));
		if (!Prop)
		{
			return FString::Printf(TEXT("%s int missing."), Name);
		}
		Prop->SetPropertyValue_InContainer(Asset, Value);
		return TEXT("");
	}

	FString WeaponRosterMismatchReason(UObject* Asset, const FWeaponRosterSpec& Spec)
	{
		if (!Asset || !Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(WeaponRosterClassPath, ESearchCase::CaseSensitive))
		{
			return TEXT("Class is not ProjectOrganoidWeaponData.");
		}
		FNameProperty* IdProp = CastField<FNameProperty>(FindInstanceProperty(Asset, TEXT("WeaponId")));
		if (!IdProp || !IdProp->GetPropertyValue_InContainer(Asset).ToString().Equals(Spec.WeaponId, ESearchCase::CaseSensitive))
		{
			return TEXT("WeaponId mismatch.");
		}
		FString Title;
		FString Description;
		FString Role;
		if (const FString TitleError = SyringeAdaptationReadText(Asset, TEXT("DisplayName"), Title); !TitleError.IsEmpty() || !Title.Equals(Spec.Title, ESearchCase::CaseSensitive))
		{
			return TitleError.IsEmpty() ? TEXT("DisplayName mismatch.") : TitleError;
		}
		if (const FString DescError = SyringeAdaptationReadText(Asset, TEXT("Description"), Description); !DescError.IsEmpty() || !Description.Equals(Spec.Description, ESearchCase::CaseSensitive))
		{
			return DescError.IsEmpty() ? TEXT("Description mismatch.") : DescError;
		}
		if (const FString RoleError = SyringeAdaptationReadText(Asset, TEXT("Role"), Role); !RoleError.IsEmpty() || !Role.Equals(Spec.Role, ESearchCase::CaseSensitive))
		{
			return RoleError.IsEmpty() ? TEXT("Role mismatch.") : RoleError;
		}
		float Damage = 0.0f;
		float Range = 0.0f;
		float FireRate = 0.0f;
		FString ReadError;
		if (!ReadNeuralSlowFloat(Asset, TEXT("Damage"), Damage, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("HitscanRange"), Range, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("FireRate"), FireRate, ReadError))
		{
			return ReadError;
		}
		if (!NeuralSlowFloatMatches(Damage, Spec.Damage) || !NeuralSlowFloatMatches(Range, Spec.Range) || !NeuralSlowFloatMatches(FireRate, Spec.FireRate))
		{
			return TEXT("Damage, range, or fire rate mismatch.");
		}
		FIntProperty* MagProp = CastField<FIntProperty>(FindInstanceProperty(Asset, TEXT("MagazineCapacity")));
		if (!MagProp || MagProp->GetPropertyValue_InContainer(Asset) != Spec.Magazine)
		{
			return TEXT("MagazineCapacity mismatch.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Asset, Asset->GetClass(), TEXT("AmmoType"), Spec.Ammo))
		{
			return TEXT("AmmoType mismatch.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Asset, Asset->GetClass(), TEXT("Effect"), Spec.Effect))
		{
			return TEXT("Effect mismatch.");
		}
		return TEXT("");
	}

	FString ApplyWeaponRosterDefaults(UObject* Asset, const FWeaponRosterSpec& Spec)
	{
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("WeaponId"), Spec.WeaponId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("DisplayName"), Spec.Title); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("Description"), Spec.Description); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("Role"), Spec.Role); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = WeaponRosterWriteFloat(Asset, TEXT("Damage"), Spec.Damage); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = WeaponRosterWriteFloat(Asset, TEXT("HitscanRange"), Spec.Range); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = WeaponRosterWriteFloat(Asset, TEXT("FireRate"), Spec.FireRate); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = WeaponRosterWriteInt(Asset, TEXT("MagazineCapacity"), Spec.Magazine); !Error.IsEmpty())
		{
			return Error;
		}
		FString EnumError;
		if (!NeuroGeneticsMissionWriteEnumByName(Asset, Asset->GetClass(), TEXT("AmmoType"), Spec.Ammo, EnumError))
		{
			return EnumError;
		}
		if (!NeuroGeneticsMissionWriteEnumByName(Asset, Asset->GetClass(), TEXT("Effect"), Spec.Effect, EnumError))
		{
			return EnumError;
		}
		return TEXT("");
	}

	FString CleanupCreatedWeaponRoster(UObject* Asset, bool bPackageWasDirtyBefore)
	{
		if (!Asset)
		{
			return TEXT("cleanup missing asset");
		}
		UPackage* Package = Asset->GetOutermost();
		Asset->ClearFlags(RF_Public | RF_Standalone);
		Asset->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
		Asset->MarkAsGarbage();
		if (Package && !bPackageWasDirtyBefore)
		{
			Package->SetDirtyFlag(false);
		}
		return TEXT("");
	}

	FString CreateWeaponRosterAsset(const FWeaponRosterSpec& Spec, UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* WeaponClass = LoadClass<UObject>(nullptr, WeaponRosterClassPath);
		if (!WeaponClass)
		{
			return TEXT("Failed to load ProjectOrganoidWeaponData.");
		}
		if (FindPackage(nullptr, Spec.Package) || FPackageName::DoesPackageExist(Spec.Package))
		{
			return FString::Printf(TEXT("%s already exists. Fail closed — no overwrite."), Spec.AssetName);
		}
		UPackage* Package = CreatePackage(Spec.Package);
		UObject* Asset = Package
			? NewObject<UObject>(Package, WeaponClass, FName(Spec.AssetName), RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (!Asset)
		{
			return FString::Printf(TEXT("NewObject failed for %s."), Spec.AssetName);
		}
		if (const FString ApplyError = ApplyWeaponRosterDefaults(Asset, Spec); !ApplyError.IsEmpty())
		{
			CleanupCreatedWeaponRoster(Asset, false);
			return ApplyError;
		}
		if (const FString VerifyError = WeaponRosterMismatchReason(Asset, Spec); !VerifyError.IsEmpty())
		{
			CleanupCreatedWeaponRoster(Asset, false);
			return VerifyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightCreateWeaponRoster(const FWeaponRosterSpec& Spec, const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return FString::Printf(TEXT("save must be false. %s does not save."), Spec.Action);
		}
		const FString SpecName = GetString(Args, TEXT("spec"), Spec.Spec);
		if (!SpecName.Equals(Spec.Spec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), Spec.Spec);
		}
		UObject* Existing = FindWeaponRosterAssetExact(Spec);
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = WeaponRosterMismatchReason(Existing, Spec);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("%s mismatch: %s"), Spec.AssetName, *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), Spec.Action);
		Proposed->SetStringField(TEXT("spec"), Spec.Spec);
		Proposed->SetStringField(TEXT("package"), Spec.Package);
		Proposed->SetStringField(TEXT("weapon_id"), Spec.WeaponId);
		Proposed->SetStringField(TEXT("title"), Spec.Title);
		Proposed->SetStringField(TEXT("description"), Spec.Description);
		Proposed->SetStringField(TEXT("role"), Spec.Role);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateWeaponRoster(const FWeaponRosterSpec& Spec, FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), FString::Printf(TEXT("%s must run on the game thread. ZERO writes."), Spec.Action), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateWeaponRoster(Spec, Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindWeaponRosterAssetExact(Spec);
		if (Existing && WeaponRosterMismatchReason(Existing, Spec).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("created"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateWeaponRoster", "Create roster weapon"));
			if (const FString CreateError = CreateWeaponRosterAsset(Spec, Created); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		bool bExpectedDirty = false;
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, Spec.Package))
			{
				bExpectedDirty = true;
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (!bExpectedDirty || UnexpectedNew.Num() != 0 || !Created || !WeaponRosterMismatchReason(Created, Spec).IsEmpty())
		{
			CleanupCreatedWeaponRoster(Created, false);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Roster weapon create did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), Spec.Package);
		Change.After->SetStringField(TEXT("weapon_id"), Spec.WeaponId);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightCreateWeaponBioStabilizerPistol(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateWeaponRoster(WeaponRosterSpecs[0], Args, Before, Proposed);
	}
	FString PreflightCreateWeaponPulseCarbine(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateWeaponRoster(WeaponRosterSpecs[1], Args, Before, Proposed);
	}
	FString PreflightCreateWeaponCryoInjector(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateWeaponRoster(WeaponRosterSpecs[2], Args, Before, Proposed);
	}
	FString PreflightCreateWeaponDenaturingShotgun(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateWeaponRoster(WeaponRosterSpecs[3], Args, Before, Proposed);
	}
	FString PreflightCreateWeaponIncineratorLance(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateWeaponRoster(WeaponRosterSpecs[4], Args, Before, Proposed);
	}
	TSharedRef<FJsonObject> ExecuteCreateWeaponBioStabilizerPistol(FBridgeChange& Change) { return ExecuteCreateWeaponRoster(WeaponRosterSpecs[0], Change); }
	TSharedRef<FJsonObject> ExecuteCreateWeaponPulseCarbine(FBridgeChange& Change) { return ExecuteCreateWeaponRoster(WeaponRosterSpecs[1], Change); }
	TSharedRef<FJsonObject> ExecuteCreateWeaponCryoInjector(FBridgeChange& Change) { return ExecuteCreateWeaponRoster(WeaponRosterSpecs[2], Change); }
	TSharedRef<FJsonObject> ExecuteCreateWeaponDenaturingShotgun(FBridgeChange& Change) { return ExecuteCreateWeaponRoster(WeaponRosterSpecs[3], Change); }
	TSharedRef<FJsonObject> ExecuteCreateWeaponIncineratorLance(FBridgeChange& Change) { return ExecuteCreateWeaponRoster(WeaponRosterSpecs[4], Change); }
