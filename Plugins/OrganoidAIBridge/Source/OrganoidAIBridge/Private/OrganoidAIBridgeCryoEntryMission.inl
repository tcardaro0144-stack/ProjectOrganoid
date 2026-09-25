// Create DA_Mission_CryoEntry. Does not save, move actors, or change power.
	const TCHAR* CryoEntryMissionSpec = TEXT("cryo_entry_mission_v1");
	const TCHAR* CryoEntryMissionAction = TEXT("create_cryo_entry_mission");
	const TCHAR* CryoEntryMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_CryoEntry");
	const TCHAR* CryoEntryMissionAssetName = TEXT("DA_Mission_CryoEntry");
	const TCHAR* CryoEntryMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_CryoEntry.DA_Mission_CryoEntry");
	const TCHAR* CryoEntryMissionId = TEXT("Mission_CryoEntry");
	const TCHAR* CryoEntryMissionTitle = TEXT("What They Kept Cold");
	const TCHAR* CryoEntryMissionDescription = TEXT("Cryo backup is online. Enter the wing and confirm what Epitope was preserving down here.");
	const TCHAR* CryoEntryObjectiveId = TEXT("Obj_EnterCryo");
	const TCHAR* CryoEntryObjectiveTitle = TEXT("Enter Cryo");
	const TCHAR* CryoEntryObjectiveDescription = TEXT("Enter the Cryo wing.");
	const TCHAR* CryoEntryEventId = TEXT("Event_CryoEntered");

	UObject* FindCryoEntryMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, CryoEntryMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, CryoEntryMissionObjectPath);
	}

	FString CryoEntryMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(CryoEntryMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(CryoEntryMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(Asset, DescProp, MakeShared<FJsonValueString>(CryoEntryMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			const FString NextPath = Soft.ToSoftObjectPath().ToString();
			const bool bNull = !Soft.ToSoftObjectPath().IsValid();
			const bool bEvidence = NextPath.Equals(TEXT("/Game/Data/Missions/DA_Mission_CryoEvidence.DA_Mission_CryoEvidence"), ESearchCase::CaseSensitive);
			if (!bNull && !bEvidence)
			{
				return FString::Printf(TEXT("NextMissionAsset must be null or DA_Mission_CryoEvidence, got '%s'"), *NextPath);
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
			return FString::Printf(TEXT("Tasks count %d, expected 1."), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}
		if (const FString TaskError = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0), TaskStruct, CryoEntryObjectiveId, CryoEntryObjectiveTitle,
				CryoEntryObjectiveDescription, true, 0, nullptr, 1, CryoEntryEventId);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *TaskError);
		}
		return TEXT("");
	}

	FString CryoEntryMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("description"), TEXT("title"), TEXT("package"), TEXT("mission_id"), TEXT("tasks"),
			TEXT("object_path"), TEXT("next_mission"), TEXT("next_mission_asset")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), CryoEntryMissionSpec);
		if (!Spec.Equals(CryoEntryMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoEntryMissionSpec);
		}
		return TEXT("");
	}

	FString ApplyCryoEntryMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), CryoEntryMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), CryoEntryMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), CryoEntryMissionDescription); !Error.IsEmpty())
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
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp ? ArrayProp->Inner : nullptr);
		if (!ArrayProp || !TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(1);
		TArray<FName> NoPrereqs;
		TArray<FName> Events;
		Events.Add(FName(CryoEntryEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, CryoEntryObjectiveId, CryoEntryObjectiveTitle,
				CryoEntryObjectiveDescription, true, NoPrereqs, Events);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *TaskError);
		}
		return TEXT("");
	}

	FString CleanupCreatedCryoEntryMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateCryoEntryMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}
		UPackage* ExistingPackage = FindPackage(nullptr, CryoEntryMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(CryoEntryMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), CryoEntryMissionPackage);
		}
		UObject* Asset = NewObject<UObject>(Package, MissionClass, CryoEntryMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_CryoEntry.");
		}
		if (const FString ApplyError = ApplyCryoEntryMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoEntryMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateCryoEntryMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_cryo_entry_mission does not save.");
		}
		if (const FString OverrideError = CryoEntryMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Existing = FindCryoEntryMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = CryoEntryMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_CryoEntry exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."), *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetStringField(TEXT("spec"), CryoEntryMissionSpec);
		Before->SetStringField(TEXT("object_path"), CryoEntryMissionObjectPath);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), CryoEntryMissionSpec);
		Proposed->SetStringField(TEXT("action"), CryoEntryMissionAction);
		Proposed->SetStringField(TEXT("object_path"), CryoEntryMissionObjectPath);
		Proposed->SetStringField(TEXT("mission_id"), CryoEntryMissionId);
		Proposed->SetStringField(TEXT("title"), CryoEntryMissionTitle);
		Proposed->SetStringField(TEXT("objective_id"), CryoEntryObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), CryoEntryEventId);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateCryoEntryMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_cryo_entry_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateCryoEntryMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindCryoEntryMissionAssetExact();
		if (Existing && CryoEntryMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateCryoEntryMission", "Create Cryo Entry Mission DataAsset"));
			if (const FString CreateError = CreateCryoEntryMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = CryoEntryMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoEntryMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), VerifyError, MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		bool bExpectedDirtyPresent = false;
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, CryoEntryMissionPackage))
			{
				bExpectedDirtyPresent = true;
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
			CleanupCreatedCryoEntryMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("dirty_package_contract"), TEXT("After create, dirty must include only the Cryo entry mission package."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(GetEditorWorld()); !PowerError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoEntryMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("power_changed"), PowerError, MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("object_path"), CryoEntryMissionObjectPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
