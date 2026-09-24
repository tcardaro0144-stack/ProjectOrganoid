	// Fixed Neuro Research Station intro mission — create_neuro_neural_slow_use_mission.
	const TCHAR* NeuroNeuralSlowUseMissionSpec = TEXT("neuro_neural_slow_use_mission_v1");
	const TCHAR* NeuroNeuralSlowUseMissionAction = TEXT("create_neuro_neural_slow_use_mission");
	const TCHAR* NeuroNeuralSlowUseMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse");
	const TCHAR* NeuroNeuralSlowUseMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse.DA_Mission_NeuroNeuralSlowUse");
	const TCHAR* NeuroNeuralSlowUseMissionAssetName = TEXT("DA_Mission_NeuroNeuralSlowUse");
	const TCHAR* NeuroNeuralSlowUseMissionId = TEXT("Mission_NeuroNeuralSlowUse");
	const TCHAR* NeuroNeuralSlowUseMissionTitle = TEXT("Use Neural Slow");
	const TCHAR* NeuroNeuralSlowUseMissionDescription =
		TEXT("Slow the Host with the adaptation mounted at the Research Station.");
	const TCHAR* NeuroNeuralSlowUseObjectiveId = TEXT("Obj_ApplyNeuralSlow");
	const TCHAR* NeuroNeuralSlowUseObjectiveTitle = TEXT("Apply Neural Slow");
	const TCHAR* NeuroNeuralSlowUseObjectiveDescription =
		TEXT("Aim at the Host and activate Neural Slow.");
	const TCHAR* NeuroNeuralSlowUseEventId = TEXT("Event_NeuralSlowApplied");
	
	UObject* FindNeuroNeuralSlowUseMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NeuroNeuralSlowUseMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, NeuroNeuralSlowUseMissionObjectPath);
	}

	TSharedRef<FJsonObject> NeuroNeuralSlowUseMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), NeuroNeuralSlowUseObjectiveId);
		Task->SetStringField(TEXT("title"), NeuroNeuralSlowUseObjectiveTitle);
		Task->SetStringField(TEXT("description"), NeuroNeuralSlowUseObjectiveDescription);
		Task->SetStringField(TEXT("category"), TEXT("Main"));
		Task->SetNumberField(TEXT("target_count"), 1);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("complete_event"), NeuroNeuralSlowUseEventId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroNeuralSlowUseMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroNeuralSlowUseMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroNeuralSlowUseMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString NeuroNeuralSlowUseMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("class"), TEXT("class_path"), TEXT("path"), TEXT("asset_path"), TEXT("object_path"),
			TEXT("package"), TEXT("mission_id"), TEXT("mission_title"), TEXT("mission_description"),
			TEXT("next_mission"), TEXT("next_mission_asset"), TEXT("objective_id"), TEXT("tasks"),
			TEXT("event_id"), TEXT("title"), TEXT("description")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroNeuralSlowUseMissionSpec);
		if (!Spec.Equals(NeuroNeuralSlowUseMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroNeuralSlowUseMissionSpec);
		}
		return TEXT("");
	}

	FString NeuroNeuralSlowUseMissionMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FProperty* IdProp = FindInstanceProperty(Asset, TEXT("MissionId"));
		FString IdError;
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(NeuroNeuralSlowUseMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroNeuralSlowUseMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(
				Asset, DescProp, MakeShared<FJsonValueString>(NeuroNeuralSlowUseMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}

		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			if (Soft.ToSoftObjectPath().IsValid())
			{
				return FString::Printf(
					TEXT("NextMissionAsset must be null, got '%s'"),
					*Soft.ToSoftObjectPath().ToString());
			}
		}
		else
		{
			return TEXT("NextMissionAsset soft property missing.");
		}

		FProperty* TasksProp = FindInstanceProperty(Asset, TEXT("Tasks"));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(TasksProp);
		if (!ArrayProp)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (Helper.Num() != 1)
		{
			return FString::Printf(TEXT("Tasks count=%d, expected exactly 1"), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}

		if (const FString Task0Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroNeuralSlowUseObjectiveId,
				NeuroNeuralSlowUseObjectiveTitle,
				NeuroNeuralSlowUseObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroNeuralSlowUseEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroNeuralSlowUseMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroNeuralSlowUseMissionId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroNeuralSlowUseMissionTitle);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Asset, TEXT("MissionDescription"), NeuroNeuralSlowUseMissionDescription);
			!Error.IsEmpty())
		{
			return Error;
		}

		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr());
		}
		else
		{
			return TEXT("NextMissionAsset soft property missing.");
		}

		FProperty* TasksProp = FindInstanceProperty(Asset, TEXT("Tasks"));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(TasksProp);
		if (!ArrayProp)
		{
			return TEXT("Tasks array missing.");
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(1);

		TArray<FName> NoPrereqs;
		TArray<FName> Events;
		Events.Add(FName(NeuroNeuralSlowUseEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroNeuralSlowUseObjectiveId,
				NeuroNeuralSlowUseObjectiveTitle,
				NeuroNeuralSlowUseObjectiveDescription,
				true,
				NoPrereqs,
				Events);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString CleanupCreatedNeuroNeuralSlowUseMissionAsset(
		UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateNeuroNeuralSlowUseMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;

		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}

		UPackage* ExistingPackage = FindPackage(nullptr, NeuroNeuralSlowUseMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();

		UPackage* Package = CreatePackage(NeuroNeuralSlowUseMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), NeuroNeuralSlowUseMissionPackage);
		}

		UObject* Asset = NewObject<UObject>(
			Package,
			MissionClass,
			NeuroNeuralSlowUseMissionAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_NeuroNeuralSlowUse.");
		}

		if (const FString ApplyError = ApplyNeuroNeuralSlowUseMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroNeuralSlowUseMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString GuardNeuroNeuralSlowUseMissionEditorContext()
	{
		if (!IsInGameThread())
		{
			return TEXT("create_neuro_neural_slow_use_mission must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before creating the mission asset.");
		}
		return TEXT("");
	}

	FString PreflightCreateNeuroNeuralSlowUseMission(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroNeuralSlowUseMissionEditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_neuro_neural_slow_use_mission does not save.");
		}
		if (const FString OverrideError = NeuroNeuralSlowUseMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Existing = FindNeuroNeuralSlowUseMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = NeuroNeuralSlowUseMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Mission_NeuroNeuralSlowUse exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return FString::Printf(TEXT("Mission class '%s' unresolved."), NeuroGeneticsMissionClassPath);
		}

		Before->SetStringField(TEXT("spec"), NeuroNeuralSlowUseMissionSpec);
		Before->SetStringField(TEXT("action"), NeuroNeuralSlowUseMissionAction);
		Before->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroNeuralSlowUseMissionPackage);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroNeuralSlowUseMissionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroNeuralSlowUseMissionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroNeuralSlowUseMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroNeuralSlowUseMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission"), NeuroNeuralSlowUseMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroNeuralSlowUseMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("create_neuro_neural_slow_use_mission must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroNeuralSlowUseMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroNeuralSlowUseMissionAssetExact();
		if (Existing && NeuroNeuralSlowUseMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroNeuralSlowUseMissionProposedState(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroNeuralSlowUse exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"CreateNeuroNeuralSlowUseMission",
				"Create Neuro Neural Slow Use Mission DataAsset"));
			if (const FString CreateError =
					CreateNeuroNeuralSlowUseMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore);
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

		if (const FString VerifyError = NeuroNeuralSlowUseMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroNeuralSlowUseMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroNeuralSlowUseMissionPackage);
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
			CleanupCreatedNeuroNeuralSlowUseMissionAsset(Created, bPackageWasDirtyBefore, Restored);
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

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Created));
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetObjectField(TEXT("mission"), NeuroNeuralSlowUseMissionProposedState(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
