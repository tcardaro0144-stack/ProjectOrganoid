// Create the two syringe-kit adaptation data assets. Does not save and does not overwrite Neural Slow.
	struct FSyringeAdaptationSpec
	{
		const TCHAR* Spec = nullptr;
		const TCHAR* Action = nullptr;
		const TCHAR* Package = nullptr;
		const TCHAR* ObjectPath = nullptr;
		const TCHAR* AssetName = nullptr;
		const TCHAR* ClassPath = nullptr;
		const TCHAR* AdaptationId = nullptr;
		const TCHAR* Title = nullptr;
		const TCHAR* Description = nullptr;
		float SpeedMultiplier = 0.0f;
		bool bHasSpeed = false;
	};

	const FSyringeAdaptationSpec LocomotorDisruptSpec = {
		TEXT("locomotor_disrupt_adaptation_v1"),
		TEXT("create_locomotor_disrupt_adaptation"),
		TEXT("/Game/Data/Adaptations/DA_Adaptation_LocomotorDisrupt"),
		TEXT("/Game/Data/Adaptations/DA_Adaptation_LocomotorDisrupt.DA_Adaptation_LocomotorDisrupt"),
		TEXT("DA_Adaptation_LocomotorDisrupt"),
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt"),
		TEXT("Adaptation_LocomotorDisrupt"),
		TEXT("Locomotor Disrupt"),
		TEXT("Disrupts locomotor nerve clusters to impair movement"),
		0.5f,
		true
	};

	const FSyringeAdaptationSpec OpticalDisruptSpec = {
		TEXT("optical_disrupt_adaptation_v1"),
		TEXT("create_optical_disrupt_adaptation"),
		TEXT("/Game/Data/Adaptations/DA_Adaptation_OpticalDisrupt"),
		TEXT("/Game/Data/Adaptations/DA_Adaptation_OpticalDisrupt.DA_Adaptation_OpticalDisrupt"),
		TEXT("DA_Adaptation_OpticalDisrupt"),
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidBiologicalAdaptation_OpticalDisrupt"),
		TEXT("Adaptation_OpticalDisrupt"),
		TEXT("Optical Disrupt"),
		TEXT("Disrupts optical nodes to impair vision"),
		0.0f,
		false
	};

	const TCHAR* LocomotorDisruptPackage = LocomotorDisruptSpec.Package;
	const TCHAR* OpticalDisruptPackage = OpticalDisruptSpec.Package;

	FString SyringeAdaptationReadText(UObject* Asset, const TCHAR* PropertyName, FString& OutText)
	{
		FTextProperty* TextProp = CastField<FTextProperty>(FindInstanceProperty(Asset, PropertyName));
		if (!TextProp)
		{
			return FString::Printf(TEXT("%s text property missing."), PropertyName);
		}
		OutText = TextProp->GetPropertyValue_InContainer(Asset).ToString();
		return TEXT("");
	}

	FString SyringeAdaptationMismatchReason(UObject* Asset, const FSyringeAdaptationSpec& Spec)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(Spec.ClassPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Class is '%s'."), Asset->GetClass() ? *Asset->GetClass()->GetPathName() : TEXT("null"));
		}
		FNameProperty* IdName = CastField<FNameProperty>(FindInstanceProperty(Asset, TEXT("AdaptationId")));
		if (!IdName || !IdName->GetPropertyValue_InContainer(Asset).ToString().Equals(Spec.AdaptationId, ESearchCase::CaseSensitive))
		{
			return TEXT("AdaptationId mismatch.");
		}
		FString Title;
		FString Description;
		if (const FString TitleError = SyringeAdaptationReadText(Asset, TEXT("DisplayName"), Title); !TitleError.IsEmpty())
		{
			return TitleError;
		}
		if (!Title.Equals(Spec.Title, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("DisplayName is '%s'."), *Title);
		}
		if (const FString DescError = SyringeAdaptationReadText(Asset, TEXT("EffectDescription"), Description); !DescError.IsEmpty())
		{
			return DescError;
		}
		if (!Description.Equals(Spec.Description, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("EffectDescription is '%s'."), *Description);
		}
		float PECost = 0.0f;
		float Cooldown = 0.0f;
		float Range = 0.0f;
		float Duration = 0.0f;
		FString ReadError;
		if (!ReadNeuralSlowFloat(Asset, TEXT("PECost"), PECost, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("CooldownSeconds"), Cooldown, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("MaxTargetRange"), Range, ReadError)
			|| !ReadNeuralSlowFloat(Asset, TEXT("DurationSeconds"), Duration, ReadError))
		{
			return ReadError;
		}
		if (!NeuralSlowFloatMatches(PECost, 20.0f) || !NeuralSlowFloatMatches(Cooldown, 8.0f)
			|| !NeuralSlowFloatMatches(Range, 800.0f) || !NeuralSlowFloatMatches(Duration, 5.0f))
		{
			return TEXT("PE, cooldown, range, or duration mismatch.");
		}
		if (Spec.bHasSpeed)
		{
			float Speed = 0.0f;
			if (!ReadNeuralSlowFloat(Asset, TEXT("LocomotorSpeedMultiplier"), Speed, ReadError) || !NeuralSlowFloatMatches(Speed, Spec.SpeedMultiplier))
			{
				return TEXT("LocomotorSpeedMultiplier mismatch.");
			}
		}
		return TEXT("");
	}

	UObject* FindSyringeAdaptationExact(const FSyringeAdaptationSpec& Spec)
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

	FString CleanupCreatedSyringeAdaptation(UObject* Asset, bool bPackageWasDirtyBefore)
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

	FString CreateSyringeAdaptationAsset(const FSyringeAdaptationSpec& Spec, UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* AdaptationClass = LoadClass<UObject>(nullptr, Spec.ClassPath);
		if (!AdaptationClass)
		{
			return FString::Printf(TEXT("Failed to load class '%s'."), Spec.ClassPath);
		}
		if (FindPackage(nullptr, Spec.Package) || FPackageName::DoesPackageExist(Spec.Package))
		{
			return FString::Printf(TEXT("%s already exists. Fail closed — no overwrite."), Spec.AssetName);
		}
		UPackage* Package = CreatePackage(Spec.Package);
		UObject* Asset = Package
			? NewObject<UObject>(Package, AdaptationClass, FName(Spec.AssetName), RF_Public | RF_Standalone | RF_Transactional)
			: nullptr;
		if (!Asset)
		{
			return FString::Printf(TEXT("NewObject failed for %s."), Spec.AssetName);
		}
		if (const FString VerifyError = SyringeAdaptationMismatchReason(Asset, Spec); !VerifyError.IsEmpty())
		{
			CleanupCreatedSyringeAdaptation(Asset, false);
			return VerifyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightCreateSyringeAdaptation(
		const FSyringeAdaptationSpec& Spec,
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
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
		UObject* Existing = FindSyringeAdaptationExact(Spec);
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = SyringeAdaptationMismatchReason(Existing, Spec);
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
		Proposed->SetStringField(TEXT("adaptation_id"), Spec.AdaptationId);
		Proposed->SetStringField(TEXT("title"), Spec.Title);
		Proposed->SetStringField(TEXT("description"), Spec.Description);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateSyringeAdaptation(const FSyringeAdaptationSpec& Spec, FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), FString::Printf(TEXT("%s must run on the game thread. ZERO writes."), Spec.Action), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateSyringeAdaptation(Spec, Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindSyringeAdaptationExact(Spec);
		if (Existing && SyringeAdaptationMismatchReason(Existing, Spec).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateSyringeAdaptation", "Create syringe-kit adaptation"));
			if (const FString CreateError = CreateSyringeAdaptationAsset(Spec, Created); !CreateError.IsEmpty())
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
		if (!bExpectedDirty || UnexpectedNew.Num() != 0 || !Created || !SyringeAdaptationMismatchReason(Created, Spec).IsEmpty())
		{
			CleanupCreatedSyringeAdaptation(Created, false);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Syringe adaptation create did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), Spec.Package);
		Change.After->SetStringField(TEXT("adaptation_id"), Spec.AdaptationId);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightCreateLocomotorDisruptAdaptation(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateSyringeAdaptation(LocomotorDisruptSpec, Args, Before, Proposed);
	}

	TSharedRef<FJsonObject> ExecuteCreateLocomotorDisruptAdaptation(FBridgeChange& Change)
	{
		return ExecuteCreateSyringeAdaptation(LocomotorDisruptSpec, Change);
	}

	FString PreflightCreateOpticalDisruptAdaptation(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		return PreflightCreateSyringeAdaptation(OpticalDisruptSpec, Args, Before, Proposed);
	}

	TSharedRef<FJsonObject> ExecuteCreateOpticalDisruptAdaptation(FBridgeChange& Change)
	{
		return ExecuteCreateSyringeAdaptation(OpticalDisruptSpec, Change);
	}
