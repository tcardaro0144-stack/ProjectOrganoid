// Create DA_Mission_TheConclusion. Does not save, move actors, or change power.
	const TCHAR* TheConclusionMissionSpec = TEXT("the_conclusion_mission_v1");
	const TCHAR* TheConclusionMissionAction = TEXT("create_the_conclusion_mission");
	const TCHAR* TheConclusionMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion");
	const TCHAR* TheConclusionMissionAssetName = TEXT("DA_Mission_TheConclusion");
	const TCHAR* TheConclusionMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion.DA_Mission_TheConclusion");
	const TCHAR* TheConclusionMissionId = TEXT("Mission_TheConclusion");
	const TCHAR* TheConclusionMissionTitle = TEXT("The Conclusion");
	const TCHAR* TheConclusionMissionDescription = TEXT("The incubator is awake. Reach the control spine overlooking the primary incubator.");
	const TCHAR* TheConclusionObjectiveId = TEXT("Obj_ReachControlSpine");
	const TCHAR* TheConclusionObjectiveTitle = TEXT("Reach Control Spine");
	const TCHAR* TheConclusionObjectiveDescription = TEXT("Reach the control spine.");
	const TCHAR* TheConclusionEventId = TEXT("Event_ReactorControlUsed");

	UObject* FindTheConclusionMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, TheConclusionMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, TheConclusionMissionObjectPath);
	}

	FString TheConclusionMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(TheConclusionMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(TheConclusionMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(Asset, DescProp, MakeShared<FJsonValueString>(TheConclusionMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			if (SoftProp->GetPropertyValue_InContainer(Asset).ToSoftObjectPath().IsValid())
			{
				return TEXT("NextMissionAsset must be null.");
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
				Helper.GetRawPtr(0), TaskStruct, TheConclusionObjectiveId, TheConclusionObjectiveTitle,
				TheConclusionObjectiveDescription, true, 0, nullptr, 1, TheConclusionEventId);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *TaskError);
		}
		return TEXT("");
	}

	FString TheConclusionMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), TheConclusionMissionSpec);
		if (!Spec.Equals(TheConclusionMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), TheConclusionMissionSpec);
		}
		return TEXT("");
	}

	FString ApplyTheConclusionMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), TheConclusionMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), TheConclusionMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), TheConclusionMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(TheConclusionEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, TheConclusionObjectiveId, TheConclusionObjectiveTitle,
				TheConclusionObjectiveDescription, true, NoPrereqs, Events);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *TaskError);
		}
		return TEXT("");
	}

	FString CleanupCreatedTheConclusionMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateTheConclusionMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}
		UPackage* ExistingPackage = FindPackage(nullptr, TheConclusionMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(TheConclusionMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), TheConclusionMissionPackage);
		}
		UObject* Asset = NewObject<UObject>(Package, MissionClass, TheConclusionMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_TheConclusion.");
		}
		if (const FString ApplyError = ApplyTheConclusionMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedTheConclusionMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateTheConclusionMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_the_conclusion_mission does not save.");
		}
		if (const FString OverrideError = TheConclusionMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		const FString EntryNext = NeuroAdaptationConnectionNextRevelationReadNext(FindComputeEntryMissionAssetExact());
		if (!EntryNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ComputeEntry.NextMissionAsset must already be DA_Mission_ComputeHandover.");
		}
		UObject* Handover = FindComputeHandoverMissionAssetExact();
		if (!Handover)
		{
			return TEXT("DA_Mission_ComputeHandover missing.");
		}
		if (const FString HandoverMismatch = ComputeHandoverMissionMismatchReason(Handover); !HandoverMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeHandover mismatch: %s"), *HandoverMismatch);
		}
		UObject* Existing = FindTheConclusionMissionAssetExact();
		bool bAlreadyExact = false;
		FString LiveMismatch;
		if (Existing)
		{
			LiveMismatch = TheConclusionMissionMismatchReason(Existing);
			bAlreadyExact = LiveMismatch.IsEmpty();
		}
		Before->SetStringField(TEXT("spec"), TheConclusionMissionSpec);
		Before->SetStringField(TEXT("object_path"), TheConclusionMissionObjectPath);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("mismatch"), LiveMismatch);
		Proposed->SetStringField(TEXT("spec"), TheConclusionMissionSpec);
		Proposed->SetStringField(TEXT("action"), TheConclusionMissionAction);
		Proposed->SetStringField(TEXT("object_path"), TheConclusionMissionObjectPath);
		Proposed->SetStringField(TEXT("mission_id"), TheConclusionMissionId);
		Proposed->SetStringField(TEXT("title"), TheConclusionMissionTitle);
		Proposed->SetStringField(TEXT("description"), TheConclusionMissionDescription);
		Proposed->SetStringField(TEXT("objective_id"), TheConclusionObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), TheConclusionEventId);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateTheConclusionMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_the_conclusion_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateTheConclusionMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindTheConclusionMissionAssetExact();
		if (Existing && TheConclusionMissionMismatchReason(Existing).IsEmpty())
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
		if (Existing)
		{
			{
				const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "RewriteTheConclusionMission", "Rewrite The Conclusion mission to the control spine contract"));
				if (const FString ApplyError = ApplyTheConclusionMissionDefaults(Existing); !ApplyError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("rewrite_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
				}
				Existing->MarkPackageDirty();
			}
			if (const FString VerifyError = TheConclusionMissionMismatchReason(Existing); !VerifyError.IsEmpty() || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("verify_failed"), VerifyError.IsEmpty() ? TEXT("Power changed during rewrite.") : VerifyError, MakeShared<FBridgeChange>(Change));
			}
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("rewritten"));
			Change.After->SetBoolField(TEXT("created"), true);
			Change.After->SetBoolField(TEXT("mutated"), true);
			Change.After->SetStringField(TEXT("object_path"), TheConclusionMissionObjectPath);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateTheConclusionMission", "Create The Conclusion Mission DataAsset"));
			if (const FString CreateError = CreateTheConclusionMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = TheConclusionMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedTheConclusionMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), VerifyError, MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		bool bExpectedDirtyPresent = false;
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, TheConclusionMissionPackage))
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
			CleanupCreatedTheConclusionMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("dirty_package_contract"), TEXT("After create, dirty must include only the Conclusion mission package."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(GetEditorWorld()); !PowerError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedTheConclusionMissionAsset(Created, bPackageWasDirtyBefore, Restored);
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
		Change.After->SetStringField(TEXT("object_path"), TheConclusionMissionObjectPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
