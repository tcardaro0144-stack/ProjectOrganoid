	// Fixed Neural Slow data asset — create_neural_slow_adaptation_asset.
	const TCHAR* NeuralSlowAdaptationSpec = TEXT("neural_slow_adaptation_asset_v1");
	const TCHAR* NeuralSlowAdaptationAction = TEXT("create_neural_slow_adaptation_asset");
	const TCHAR* NeuralSlowAdaptationPackage = TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow");
	const TCHAR* NeuralSlowAdaptationObjectPath =
		TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");
	const TCHAR* NeuralSlowAdaptationAssetName = TEXT("DA_Adaptation_NeuralSlow");
	const TCHAR* NeuralSlowAdaptationClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidBiologicalAdaptation_NeuralSlow");
	const TCHAR* NeuralSlowAdaptationId = TEXT("NeuralSlow");
	constexpr float NeuralSlowAdaptationPECost = 20.0f;
	constexpr float NeuralSlowAdaptationCooldown = 8.0f;
	constexpr float NeuralSlowAdaptationRange = 800.0f;
	constexpr float NeuralSlowAdaptationDuration = 4.0f;
	constexpr float NeuralSlowAdaptationSpeedMultiplier = 0.6f;

	UClass* LoadNeuralSlowAdaptationClass()
	{
		return LoadClass<UObject>(nullptr, NeuralSlowAdaptationClassPath);
	}

	bool ReadNeuralSlowFloat(UObject* Asset, const TCHAR* PropertyName, float& OutValue, FString& OutError)
	{
		FProperty* Property = FindInstanceProperty(Asset, PropertyName);
		FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
		if (!Numeric || !Numeric->IsFloatingPoint())
		{
			OutError = FString::Printf(TEXT("%s float property missing."), PropertyName);
			return false;
		}
		OutValue = static_cast<float>(Numeric->GetFloatingPointPropertyValue(
			Numeric->ContainerPtrToValuePtr<void>(Asset)));
		return true;
	}

	bool NeuralSlowFloatMatches(float Value, float Expected)
	{
		return FMath::IsNearlyEqual(Value, Expected, KINDA_SMALL_NUMBER);
	}

	FString NeuralSlowAdaptationRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("class"), TEXT("class_path"), TEXT("asset_name"), TEXT("object_name"),
			TEXT("adaptation_id"), TEXT("pe_cost"), TEXT("cooldown"), TEXT("range"),
			TEXT("duration"), TEXT("movement_speed_multiplier"), TEXT("locomotor_speed_multiplier")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		auto AcceptLocked = [&](const TCHAR* Key, const TCHAR* Expected) -> FString
		{
			if (!Args->HasField(Key))
			{
				return TEXT("");
			}
			const FString Value = GetString(Args, Key);
			if (!Value.Equals(Expected, ESearchCase::CaseSensitive))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
			return TEXT("");
		};
		if (const FString TargetError = AcceptLocked(TEXT("target"), NeuralSlowAdaptationPackage); !TargetError.IsEmpty())
		{
			return TargetError;
		}
		if (const FString PackageError = AcceptLocked(TEXT("package"), NeuralSlowAdaptationPackage); !PackageError.IsEmpty())
		{
			return PackageError;
		}
		if (const FString RequiredError = AcceptLocked(TEXT("required_package"), NeuralSlowAdaptationPackage); !RequiredError.IsEmpty())
		{
			return RequiredError;
		}
		if (const FString ObjectError = AcceptLocked(TEXT("object_path"), NeuralSlowAdaptationObjectPath); !ObjectError.IsEmpty())
		{
			return ObjectError;
		}
		if (Args->HasField(TEXT("path")))
		{
			const FString Path = GetString(Args, TEXT("path"));
			if (!Path.Equals(NeuralSlowAdaptationPackage, ESearchCase::CaseSensitive)
				&& !Path.Equals(NeuralSlowAdaptationObjectPath, ESearchCase::CaseSensitive))
			{
				return TEXT("Client override 'path' rejected. Spec is locked.");
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuralSlowAdaptationSpec);
		if (!Spec.Equals(NeuralSlowAdaptationSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuralSlowAdaptationSpec);
		}
		return TEXT("");
	}

	void FillNeuralSlowAdaptationIdentity(
		TSharedRef<FJsonObject> Out,
		bool bExists,
		bool bAlreadyExact,
		bool bRedirector,
		const FString& ClassPath,
		const FString& AdaptationId,
		float PECost,
		float Cooldown,
		float Range,
		float Duration,
		float SpeedMultiplier)
	{
		Out->SetStringField(TEXT("spec"), NeuralSlowAdaptationSpec);
		Out->SetStringField(TEXT("action"), NeuralSlowAdaptationAction);
		Out->SetStringField(TEXT("package"), NeuralSlowAdaptationPackage);
		Out->SetStringField(TEXT("object_path"), NeuralSlowAdaptationObjectPath);
		Out->SetStringField(TEXT("asset_name"), NeuralSlowAdaptationAssetName);
		Out->SetBoolField(TEXT("exists"), bExists);
		Out->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Out->SetBoolField(TEXT("redirector"), bRedirector);
		Out->SetStringField(TEXT("class"), ClassPath);
		Out->SetStringField(TEXT("adaptation_id"), AdaptationId);
		Out->SetNumberField(TEXT("pe_cost"), PECost);
		Out->SetNumberField(TEXT("cooldown"), Cooldown);
		Out->SetNumberField(TEXT("range"), Range);
		Out->SetNumberField(TEXT("duration"), Duration);
		Out->SetNumberField(TEXT("movement_speed_multiplier"), SpeedMultiplier);
		Out->SetBoolField(TEXT("save_performed"), false);
	}

	FString NeuralSlowAdaptationMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (FAssetData::IsRedirector(Asset) || (Asset->GetClass() && Asset->GetClass()->GetName() == TEXT("ObjectRedirector")))
		{
			return TEXT("Object is a redirector.");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuralSlowAdaptationClassPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(
				TEXT("Class is '%s', expected '%s'."),
				Asset->GetClass() ? *Asset->GetClass()->GetPathName() : TEXT("null"),
				NeuralSlowAdaptationClassPath);
		}
		if (!Asset->GetName().Equals(NeuralSlowAdaptationAssetName, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Object name is '%s'."), *Asset->GetName());
		}
		FProperty* IdProperty = FindInstanceProperty(Asset, TEXT("AdaptationId"));
		const FNameProperty* IdName = CastField<FNameProperty>(IdProperty);
		if (!IdName)
		{
			return TEXT("AdaptationId name property missing.");
		}
		const FString AdaptationId = IdName->GetPropertyValue_InContainer(Asset).ToString();
		if (!AdaptationId.Equals(NeuralSlowAdaptationId, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("AdaptationId is '%s'."), *AdaptationId);
		}

		float PECost = 0.0f;
		float Cooldown = 0.0f;
		float Range = 0.0f;
		float Duration = 0.0f;
		float Speed = 0.0f;
		FString ReadError;
		if (!ReadNeuralSlowFloat(Asset, TEXT("PECost"), PECost, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("CooldownSeconds"), Cooldown, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("MaxTargetRange"), Range, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("DurationSeconds"), Duration, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("LocomotorSpeedMultiplier"), Speed, ReadError))
		{
			return ReadError;
		}
		if (!NeuralSlowFloatMatches(PECost, NeuralSlowAdaptationPECost)
			|| !NeuralSlowFloatMatches(Cooldown, NeuralSlowAdaptationCooldown)
			|| !NeuralSlowFloatMatches(Range, NeuralSlowAdaptationRange)
			|| !NeuralSlowFloatMatches(Duration, NeuralSlowAdaptationDuration)
			|| !NeuralSlowFloatMatches(Speed, NeuralSlowAdaptationSpeedMultiplier))
		{
			return FString::Printf(
				TEXT("Constructor defaults mismatch. PE=%s cooldown=%s range=%s duration=%s speed=%s."),
				*FString::SanitizeFloat(PECost),
				*FString::SanitizeFloat(Cooldown),
				*FString::SanitizeFloat(Range),
				*FString::SanitizeFloat(Duration),
				*FString::SanitizeFloat(Speed));
		}
		return TEXT("");
	}

	bool ReadNeuralSlowAdaptationLive(
		UObject* Asset,
		FString& OutClassPath,
		FString& OutAdaptationId,
		float& OutPECost,
		float& OutCooldown,
		float& OutRange,
		float& OutDuration,
		float& OutSpeed,
		bool& bOutRedirector)
	{
		bOutRedirector = Asset && (FAssetData::IsRedirector(Asset)
			|| (Asset->GetClass() && Asset->GetClass()->GetName() == TEXT("ObjectRedirector")));
		OutClassPath = Asset && Asset->GetClass() ? Asset->GetClass()->GetPathName() : TEXT("");
		OutAdaptationId = TEXT("");
		OutPECost = 0.0f;
		OutCooldown = 0.0f;
		OutRange = 0.0f;
		OutDuration = 0.0f;
		OutSpeed = 0.0f;
		if (!Asset || bOutRedirector)
		{
			return false;
		}
		if (const FNameProperty* IdName = CastField<FNameProperty>(FindInstanceProperty(Asset, TEXT("AdaptationId"))))
		{
			OutAdaptationId = IdName->GetPropertyValue_InContainer(Asset).ToString();
		}
		FString ReadError;
		ReadNeuralSlowFloat(Asset, TEXT("PECost"), OutPECost, ReadError);
		ReadNeuralSlowFloat(Asset, TEXT("CooldownSeconds"), OutCooldown, ReadError);
		ReadNeuralSlowFloat(Asset, TEXT("MaxTargetRange"), OutRange, ReadError);
		ReadNeuralSlowFloat(Asset, TEXT("DurationSeconds"), OutDuration, ReadError);
		ReadNeuralSlowFloat(Asset, TEXT("LocomotorSpeedMultiplier"), OutSpeed, ReadError);
		return true;
	}

	UObject* FindNeuralSlowAdaptationInMemory()
	{
		return StaticFindObject(nullptr, nullptr, NeuralSlowAdaptationObjectPath);
	}

	FString NeuralSlowAdaptationRegistryConflict()
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		const FAssetData ByPath = AssetRegistry.Get().GetAssetByObjectPath(FSoftObjectPath(NeuralSlowAdaptationObjectPath), true);
		if (ByPath.IsValid() && ByPath.IsRedirector())
		{
			return TEXT("Asset Registry has a redirector at the Neural Slow object path. Fail closed.");
		}
		TArray<FAssetData> PackageAssets;
		AssetRegistry.Get().GetAssetsByPackageName(FName(NeuralSlowAdaptationPackage), PackageAssets, true);
		for (const FAssetData& Asset : PackageAssets)
		{
			if (Asset.IsRedirector())
			{
				return TEXT("Asset Registry has a redirector in the Neural Slow package. Fail closed.");
			}
			if (!Asset.GetObjectPathString().Equals(NeuralSlowAdaptationObjectPath, ESearchCase::CaseSensitive))
			{
				return FString::Printf(
					TEXT("Asset Registry has a different object '%s' in the Neural Slow package. Fail closed."),
					*Asset.GetObjectPathString());
			}
		}
		if (ByPath.IsValid() && !ByPath.AssetClassPath.ToString().Equals(NeuralSlowAdaptationClassPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(
				TEXT("Asset Registry class is '%s'. Fail closed."),
				*ByPath.AssetClassPath.ToString());
		}
		return TEXT("");
	}

	FString GuardNeuralSlowAdaptationEditorContext()
	{
		if (!IsInGameThread())
		{
			return TEXT("create_neural_slow_adaptation_asset must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before creating the Neural Slow asset.");
		}
		return TEXT("");
	}

	FString CleanupCreatedNeuralSlowAdaptationAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
	{
		OutRestored.Reset();
		if (!Asset)
		{
			return TEXT("cleanup missing asset");
		}
		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return TEXT("cleanup missing package");
		}
		const FString PackageName = Package->GetName();
		Asset->ClearFlags(RF_Public | RF_Standalone);
		Asset->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
		Asset->MarkAsGarbage();
		if (!bPackageWasDirtyBefore)
		{
			Package->SetDirtyFlag(false);
		}
		OutRestored.Add(PackageName);
		return TEXT("");
	}

	FString CreateNeuralSlowAdaptationAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;

		UClass* AdaptationClass = LoadNeuralSlowAdaptationClass();
		if (!AdaptationClass)
		{
			return FString::Printf(TEXT("Failed to load class '%s'."), NeuralSlowAdaptationClassPath);
		}

		UPackage* ExistingPackage = FindPackage(nullptr, NeuralSlowAdaptationPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		if (ExistingPackage)
		{
			return TEXT("Neural Slow package is already in memory. Fail closed — no overwrite.");
		}
		FString ExistingFilename;
		if (FPackageName::DoesPackageExist(NeuralSlowAdaptationPackage, &ExistingFilename))
		{
			return FString::Printf(TEXT("Neural Slow package already exists on disk (%s). Fail closed — no overwrite."), *ExistingFilename);
		}

		UPackage* Package = CreatePackage(NeuralSlowAdaptationPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), NeuralSlowAdaptationPackage);
		}

		UObject* Asset = NewObject<UObject>(
			Package,
			AdaptationClass,
			NeuralSlowAdaptationAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Adaptation_NeuralSlow.");
		}

		if (const FString VerifyError = NeuralSlowAdaptationMismatchReason(Asset); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuralSlowAdaptationAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return VerifyError;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateNeuralSlowAdaptationAsset(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuralSlowAdaptationEditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_neural_slow_adaptation_asset does not save.");
		}
		if (const FString OverrideError = NeuralSlowAdaptationRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (const FString RegistryError = NeuralSlowAdaptationRegistryConflict(); !RegistryError.IsEmpty())
		{
			return RegistryError;
		}

		UObject* Existing = FindNeuralSlowAdaptationInMemory();
		if (!Existing)
		{
			FString ExistingFilename;
			if (FPackageName::DoesPackageExist(NeuralSlowAdaptationPackage, &ExistingFilename))
			{
				Existing = StaticLoadObject(UObject::StaticClass(), nullptr, NeuralSlowAdaptationObjectPath);
			}
			else if (FindPackage(nullptr, NeuralSlowAdaptationPackage))
			{
				return TEXT("Neural Slow package is in memory without the exact object. Fail closed.");
			}
		}

		bool bAlreadyExact = false;
		FString ClassPath;
		FString AdaptationId;
		float PECost = 0.0f;
		float Cooldown = 0.0f;
		float Range = 0.0f;
		float Duration = 0.0f;
		float Speed = 0.0f;
		bool bRedirector = false;
		if (Existing)
		{
			ReadNeuralSlowAdaptationLive(Existing, ClassPath, AdaptationId, PECost, Cooldown, Range, Duration, Speed, bRedirector);
			const FString Mismatch = NeuralSlowAdaptationMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Adaptation_NeuralSlow exists but mismatches locked contract: %s Fail closed — no overwrite."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadNeuralSlowAdaptationClass())
		{
			return FString::Printf(TEXT("Class '%s' unresolved."), NeuralSlowAdaptationClassPath);
		}

		FillNeuralSlowAdaptationIdentity(
			Before,
			Existing != nullptr,
			bAlreadyExact,
			bRedirector,
			bAlreadyExact ? ClassPath : TEXT(""),
			bAlreadyExact ? AdaptationId : TEXT(""),
			bAlreadyExact ? PECost : 0.0f,
			bAlreadyExact ? Cooldown : 0.0f,
			bAlreadyExact ? Range : 0.0f,
			bAlreadyExact ? Duration : 0.0f,
			bAlreadyExact ? Speed : 0.0f);
		Before->SetBoolField(TEXT("on_disk"), FPackageName::DoesPackageExist(NeuralSlowAdaptationPackage));
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));

		FillNeuralSlowAdaptationIdentity(
			Proposed,
			true,
			bAlreadyExact,
			false,
			NeuralSlowAdaptationClassPath,
			NeuralSlowAdaptationId,
			NeuralSlowAdaptationPECost,
			NeuralSlowAdaptationCooldown,
			NeuralSlowAdaptationRange,
			NeuralSlowAdaptationDuration,
			NeuralSlowAdaptationSpeedMultiplier);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetStringField(TEXT("result"), bAlreadyExact ? TEXT("already_exact_noop") : TEXT("created"));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuralSlowAdaptationAsset(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("create_neural_slow_adaptation_asset must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuralSlowAdaptationAsset(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuralSlowAdaptationInMemory();
		if (Existing && NeuralSlowAdaptationMismatchReason(Existing).IsEmpty())
		{
			FString ClassPath;
			FString AdaptationId;
			float PECost = 0.0f;
			float Cooldown = 0.0f;
			float Range = 0.0f;
			float Duration = 0.0f;
			float Speed = 0.0f;
			bool bRedirector = false;
			ReadNeuralSlowAdaptationLive(Existing, ClassPath, AdaptationId, PECost, Cooldown, Range, Duration, Speed, bRedirector);
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			FillNeuralSlowAdaptationIdentity(
				Change.After.ToSharedRef(), true, true, false, ClassPath, AdaptationId, PECost, Cooldown, Range, Duration, Speed);
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Adaptation_NeuralSlow exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"CreateNeuralSlowAdaptationAsset",
				"Create Neural Slow Adaptation DataAsset"));
			if (const FString CreateError = CreateNeuralSlowAdaptationAsset(Created, bCreatedNow, bPackageWasDirtyBefore);
				!CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				return FailAudit(
					TEXT("create_failed"),
					FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString VerifyError = NeuralSlowAdaptationMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuralSlowAdaptationAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuralSlowAdaptationPackage);
		bool bExpectedDirtyPresent = false;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, ExpectedDirty))
			{
				bExpectedDirtyPresent = true;
				break;
			}
		}
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, ExpectedDirty))
			{
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (!bExpectedDirtyPresent || UnexpectedNew.Num() != 0)
		{
			TArray<FString> Restored;
			CleanupCreatedNeuralSlowAdaptationAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			return FailAudit(
				TEXT("dirty_package_contract"),
				FString::Printf(
					TEXT("After create, dirty must include [%s] with no new unexpected dirties. got [%s] unexpected_new [%s]."),
					*ExpectedDirty,
					*FString::Join(DirtyAfter, TEXT(",")),
					*FString::Join(UnexpectedNew, TEXT(","))),
				MakeShared<FBridgeChange>(Change));
		}

		FString ClassPath;
		FString AdaptationId;
		float PECost = 0.0f;
		float Cooldown = 0.0f;
		float Range = 0.0f;
		float Duration = 0.0f;
		float Speed = 0.0f;
		bool bRedirector = false;
		ReadNeuralSlowAdaptationLive(Created, ClassPath, AdaptationId, PECost, Cooldown, Range, Duration, Speed, bRedirector);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		FillNeuralSlowAdaptationIdentity(
			Change.After.ToSharedRef(), true, false, false, ClassPath, AdaptationId, PECost, Cooldown, Range, Duration, Speed);
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
