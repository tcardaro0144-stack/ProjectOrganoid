	// Fixed Neuro Power Restore mission — create_neuro_power_restore_mission / neuro_power_restore_mission_v1.
	const TCHAR* NeuroPowerRestoreMissionSpec = TEXT("neuro_power_restore_mission_v1");
	const TCHAR* NeuroPowerRestoreMissionAction = TEXT("create_neuro_power_restore_mission");
	const TCHAR* NeuroPowerRestoreMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore");
	const TCHAR* NeuroPowerRestoreMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore");
	const TCHAR* NeuroPowerRestoreMissionAssetName = TEXT("DA_Mission_NeuroPowerRestore");
	const TCHAR* NeuroPowerRestoreMissionId = TEXT("Mission_NeuroPowerRestore");
	const TCHAR* NeuroPowerRestoreMissionTitle = TEXT("Restore NeuroGenetics Power");
	const TCHAR* NeuroPowerRestoreMissionDescription =
		TEXT("Bring the NeuroGenetics research wing back online using its backup power panel.");
	const TCHAR* NeuroPowerRestoreObjectiveId = TEXT("Obj_RestoreNeuroLabPower");
	const TCHAR* NeuroPowerRestoreObjectiveTitle = TEXT("Restore NeuroGenetics power");
	const TCHAR* NeuroPowerRestoreObjectiveDescription =
		TEXT("Use the backup panel to bring the NeuroGenetics sector online.");
	const TCHAR* NeuroPowerRestoreEventId = TEXT("Event_NeuroPowerRestored");

	UObject* FindNeuroPowerRestoreMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NeuroPowerRestoreMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, NeuroPowerRestoreMissionObjectPath);
	}

	TSharedRef<FJsonObject> NeuroPowerRestoreMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), NeuroPowerRestoreObjectiveId);
		Task->SetStringField(TEXT("title"), NeuroPowerRestoreObjectiveTitle);
		Task->SetStringField(TEXT("description"), NeuroPowerRestoreObjectiveDescription);
		Task->SetStringField(TEXT("category"), TEXT("Main"));
		Task->SetNumberField(TEXT("target_count"), 1);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("complete_event"), NeuroPowerRestoreEventId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroPowerRestoreMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroPowerRestoreMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroPowerRestoreMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString NeuroPowerRestoreMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroPowerRestoreMissionSpec);
		if (!Spec.Equals(NeuroPowerRestoreMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroPowerRestoreMissionSpec);
		}
		return TEXT("");
	}

	FString NeuroPowerRestoreMissionMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!NeuroGeneticsMissionClassMatches(Asset))
		{
			return FString::Printf(TEXT("class '%s' is not UProjectOrganoidObjectiveDataAsset"), *ClassName(Asset));
		}
		if (!PackagesEqual(Asset->GetOutermost() ? Asset->GetOutermost()->GetName() : FString(), NeuroPowerRestoreMissionPackage))
		{
			return FString::Printf(
				TEXT("owning package '%s' is not %s"),
				Asset->GetOutermost() ? *Asset->GetOutermost()->GetName() : TEXT(""),
				NeuroPowerRestoreMissionPackage);
		}
		if (!Asset->GetName().Equals(NeuroPowerRestoreMissionAssetName, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("asset name '%s' is not DA_Mission_NeuroPowerRestore"), *Asset->GetName());
		}
		if (!Asset->GetPathName().Equals(NeuroPowerRestoreMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("object path '%s' is not exact"), *Asset->GetPathName());
		}

		FProperty* MissionIdProp = FindInstanceProperty(Asset, TEXT("MissionId"));
		FString MissionIdError;
		if (!PropertyMatchesJson(Asset, MissionIdProp, MakeShared<FJsonValueString>(NeuroPowerRestoreMissionId), MissionIdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *MissionIdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroPowerRestoreMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(
				Asset, DescProp, MakeShared<FJsonValueString>(NeuroPowerRestoreMissionDescription), DescError))
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
				NeuroPowerRestoreObjectiveId,
				NeuroPowerRestoreObjectiveTitle,
				NeuroPowerRestoreObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroPowerRestoreEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroPowerRestoreMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroPowerRestoreMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroPowerRestoreMissionTitle);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), NeuroPowerRestoreMissionDescription);
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
		Events.Add(FName(NeuroPowerRestoreEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroPowerRestoreObjectiveId,
				NeuroPowerRestoreObjectiveTitle,
				NeuroPowerRestoreObjectiveDescription,
				true,
				NoPrereqs,
				Events);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString CleanupCreatedNeuroPowerRestoreMissionAsset(
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

	FString CreateNeuroPowerRestoreMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;

		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}

		UPackage* ExistingPackage = FindPackage(nullptr, NeuroPowerRestoreMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();

		UPackage* Package = CreatePackage(NeuroPowerRestoreMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), NeuroPowerRestoreMissionPackage);
		}

		UObject* Asset = NewObject<UObject>(
			Package, MissionClass, NeuroPowerRestoreMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_NeuroPowerRestore.");
		}

		if (const FString ApplyError = ApplyNeuroPowerRestoreMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroPowerRestoreMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString GuardNeuroPowerRestoreMissionEditorContext()
	{
		if (!IsInGameThread())
		{
			return TEXT("create_neuro_power_restore_mission must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before creating the mission asset.");
		}
		return TEXT("");
	}

	FString PreflightCreateNeuroPowerRestoreMission(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroPowerRestoreMissionEditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_neuro_power_restore_mission does not save.");
		}
		if (const FString OverrideError = NeuroPowerRestoreMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Existing = FindNeuroPowerRestoreMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = NeuroPowerRestoreMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Mission_NeuroPowerRestore exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return FString::Printf(TEXT("Mission class '%s' unresolved."), NeuroGeneticsMissionClassPath);
		}

		Before->SetStringField(TEXT("spec"), NeuroPowerRestoreMissionSpec);
		Before->SetStringField(TEXT("action"), NeuroPowerRestoreMissionAction);
		Before->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroPowerRestoreMissionPackage);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroPowerRestoreMissionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroPowerRestoreMissionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroPowerRestoreMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroPowerRestoreMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission"), NeuroPowerRestoreMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroPowerRestoreMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("create_neuro_power_restore_mission must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroPowerRestoreMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroPowerRestoreMissionAssetExact();
		if (Existing && NeuroPowerRestoreMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPath);
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroPowerRestoreMissionProposedState(true));
			Change.After->SetArrayField(
				TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroPowerRestore exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(
				NSLOCTEXT("OrganoidAIBridge", "CreateNeuroPowerRestoreMission", "Create Neuro Power Restore Mission DataAsset"));
			if (const FString CreateError = CreateNeuroPowerRestoreMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore);
				!CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(
					TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				return FailAudit(
					TEXT("create_failed"),
					FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString VerifyError = NeuroPowerRestoreMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroPowerRestoreMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroPowerRestoreMissionPackage);
		bool bExpectedDirtyPresent = false;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, ExpectedDirty))
			{
				bExpectedDirtyPresent = true;
				break;
			}
		}
		// Allow pre-existing unrelated dirties; require the new mission package to be dirty.
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
			CleanupCreatedNeuroPowerRestoreMissionAsset(Created, bPackageWasDirtyBefore, Restored);
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
		Change.After->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Created));
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetObjectField(TEXT("mission"), NeuroPowerRestoreMissionProposedState(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
