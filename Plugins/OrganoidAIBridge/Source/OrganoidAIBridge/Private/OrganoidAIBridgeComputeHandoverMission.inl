// Create DA_Mission_ComputeHandover. Does not save, move actors, or change power.
	const TCHAR* ComputeHandoverMissionSpec = TEXT("compute_handover_mission_v1");
	const TCHAR* ComputeHandoverMissionAction = TEXT("create_compute_handover_mission");
	const TCHAR* ComputeHandoverMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_ComputeHandover");
	const TCHAR* ComputeHandoverMissionAssetName = TEXT("DA_Mission_ComputeHandover");
	const TCHAR* ComputeHandoverMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_ComputeHandover.DA_Mission_ComputeHandover");
	const TCHAR* ComputeHandoverMissionId = TEXT("Mission_ComputeHandover");
	const TCHAR* ComputeHandoverMissionTitle = TEXT("The Handover");
	const TCHAR* ComputeHandoverMissionDescription = TEXT("The compute substrate has been running the lockdown. Wake the interface chamber and recover Sterling's confession.");
	const TCHAR* ComputeHandoverHackObjectiveId = TEXT("Obj_HackComputeCore");
	const TCHAR* ComputeHandoverHackObjectiveTitle = TEXT("Hack Compute Core");
	const TCHAR* ComputeHandoverHackObjectiveDescription = TEXT("Hack the compute core.");
	const TCHAR* ComputeHandoverHackEventId = TEXT("Event_ComputeCoreHacked");
	const TCHAR* ComputeHandoverConfessionObjectiveId = TEXT("Obj_ReadSterlingConfession");
	const TCHAR* ComputeHandoverConfessionObjectiveTitle = TEXT("Read Sterling's Confession");
	const TCHAR* ComputeHandoverConfessionObjectiveDescription = TEXT("Read Sterling's confession.");
	const TCHAR* ComputeHandoverConfessionEventId = TEXT("Event_SterlingConfessionRead");
	constexpr int32 ComputeHandoverHackTargetCount = 3;

	UObject* FindComputeHandoverMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, ComputeHandoverMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, ComputeHandoverMissionObjectPath);
	}

	FString ComputeHandoverPatchHackCounting(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during handover patch.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp)
		{
			return TEXT("TargetProgress missing during handover patch.");
		}
		TargetProp->SetPropertyValue_InContainer(Objective, ComputeHandoverHackTargetCount);
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("EventTriggers missing during handover patch.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers count is not 1 during handover patch.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		if (!TrigStruct || !TrigStruct->Struct)
		{
			return TEXT("EventTriggers element missing during handover patch.");
		}
		void* TrigElem = TriggerHelper.GetRawPtr(0);
		FString EnumError;
		if (!NeuroGeneticsMissionWriteEnumByName(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Advance"), EnumError))
		{
			return EnumError;
		}
		if (FIntProperty* DeltaProp = FindFProperty<FIntProperty>(TrigStruct->Struct, TEXT("ProgressDelta")))
		{
			DeltaProp->SetPropertyValue_InContainer(TrigElem, 1);
		}
		else
		{
			return TEXT("ProgressDelta missing during handover patch.");
		}
		return TEXT("");
	}

	FString ComputeHandoverReadHackTask(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing.");
		}
		if (!NeuroGeneticsMissionReadName(Objective, ObjectiveProp->Struct, TEXT("ObjectiveId")).Equals(ComputeHandoverHackObjectiveId))
		{
			return TEXT("Hack ObjectiveId mismatch.");
		}
		if (!NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Title")).Equals(ComputeHandoverHackObjectiveTitle))
		{
			return TEXT("Hack Title mismatch.");
		}
		if (!NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Description")).Equals(ComputeHandoverHackObjectiveDescription))
		{
			return TEXT("Hack Description mismatch.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Main")))
		{
			return TEXT("Hack Type is not Main.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("State"), TEXT("Inactive")))
		{
			return TEXT("Hack State is not Inactive.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp || TargetProp->GetPropertyValue_InContainer(Objective) != ComputeHandoverHackTargetCount)
		{
			return TEXT("Hack TargetProgress must be 3.");
		}
		FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds"));
		if (!PrereqProp)
		{
			return TEXT("Hack prerequisites missing.");
		}
		FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
		if (PrereqHelper.Num() != 0)
		{
			return TEXT("Hack objective must have no prerequisite.");
		}
		FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate"));
		if (!AutoProp || !AutoProp->GetPropertyValue_InContainer(TaskElem))
		{
			return TEXT("Hack objective must autoactivate.");
		}
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("Hack EventTriggers missing.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("Hack EventTriggers must contain one Advance trigger.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		void* TrigElem = TrigStruct ? TriggerHelper.GetRawPtr(0) : nullptr;
		FNameProperty* EventProp = TrigStruct ? FindFProperty<FNameProperty>(TrigStruct->Struct, TEXT("EventId")) : nullptr;
		if (!TrigElem || !EventProp || EventProp->GetPropertyValue_InContainer(TrigElem) != FName(ComputeHandoverHackEventId))
		{
			return TEXT("Hack EventId must be Event_ComputeCoreHacked.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Advance")))
		{
			return TEXT("Hack event action must be Advance.");
		}
		FIntProperty* DeltaProp = FindFProperty<FIntProperty>(TrigStruct->Struct, TEXT("ProgressDelta"));
		if (!DeltaProp || DeltaProp->GetPropertyValue_InContainer(TrigElem) != 1)
		{
			return TEXT("Hack ProgressDelta must be 1.");
		}
		return TEXT("");
	}

	FString ComputeHandoverMissionMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (!Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FString IdError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(ComputeHandoverMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(ComputeHandoverMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(ComputeHandoverMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Asset, TEXT("NextMissionAsset")));
		if (!SoftProp)
		{
			return TEXT("NextMissionAsset soft property missing.");
		}
		if (SoftProp->GetPropertyValue_InContainer(Asset).ToSoftObjectPath().IsValid())
		{
			return TEXT("NextMissionAsset must be null.");
		}
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
		if (!ArrayProp)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (Helper.Num() != 2)
		{
			return FString::Printf(TEXT("Tasks count %d, expected 2."), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}
		if (const FString HackError = ComputeHandoverReadHackTask(Helper.GetRawPtr(0), TaskStruct); !HackError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *HackError);
		}
		if (const FString ConfessionError = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(1), TaskStruct, ComputeHandoverConfessionObjectiveId, ComputeHandoverConfessionObjectiveTitle,
				ComputeHandoverConfessionObjectiveDescription, false, 1, ComputeHandoverHackObjectiveId, 1, ComputeHandoverConfessionEventId);
			!ConfessionError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1]: %s"), *ConfessionError);
		}
		return TEXT("");
	}

	FString ComputeHandoverMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), ComputeHandoverMissionSpec);
		if (!Spec.Equals(ComputeHandoverMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ComputeHandoverMissionSpec);
		}
		return TEXT("");
	}

	FString ApplyComputeHandoverMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), ComputeHandoverMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), ComputeHandoverMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), ComputeHandoverMissionDescription); !Error.IsEmpty())
		{
			return Error;
		}
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Asset, TEXT("NextMissionAsset")));
		if (!SoftProp)
		{
			return TEXT("NextMissionAsset soft property missing.");
		}
		SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr());
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp ? ArrayProp->Inner : nullptr);
		if (!ArrayProp || !TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(2);
		TArray<FName> NoPrereqs;
		TArray<FName> HackEvents;
		HackEvents.Add(FName(ComputeHandoverHackEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, ComputeHandoverHackObjectiveId, ComputeHandoverHackObjectiveTitle,
				ComputeHandoverHackObjectiveDescription, true, NoPrereqs, HackEvents);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *TaskError);
		}
		if (const FString PatchError = ComputeHandoverPatchHackCounting(Helper.GetRawPtr(0), TaskStruct); !PatchError.IsEmpty())
		{
			return PatchError;
		}
		TArray<FName> ConfessionPrereq;
		ConfessionPrereq.Add(FName(ComputeHandoverHackObjectiveId));
		TArray<FName> ConfessionEvents;
		ConfessionEvents.Add(FName(ComputeHandoverConfessionEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(1), TaskStruct, ComputeHandoverConfessionObjectiveId, ComputeHandoverConfessionObjectiveTitle,
				ComputeHandoverConfessionObjectiveDescription, false, ConfessionPrereq, ConfessionEvents);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1] apply: %s"), *TaskError);
		}
		return TEXT("");
	}

	FString CleanupCreatedComputeHandoverMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateComputeHandoverMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}
		UPackage* ExistingPackage = FindPackage(nullptr, ComputeHandoverMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(ComputeHandoverMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), ComputeHandoverMissionPackage);
		}
		UObject* Asset = NewObject<UObject>(Package, MissionClass, ComputeHandoverMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_ComputeHandover.");
		}
		if (const FString ApplyError = ApplyComputeHandoverMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedComputeHandoverMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateComputeHandoverMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_compute_handover_mission does not save.");
		}
		if (const FString OverrideError = ComputeHandoverMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* ComputeEntry = FindComputeEntryMissionAssetExact();
		if (!ComputeEntry)
		{
			return TEXT("DA_Mission_ComputeEntry missing.");
		}
		if (const FString EntryMismatch = ComputeEntryMissionMismatchReason(ComputeEntry); !EntryMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeEntry mismatch: %s"), *EntryMismatch);
		}
		UObject* Existing = FindComputeHandoverMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = ComputeHandoverMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_ComputeHandover exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."), *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetStringField(TEXT("spec"), ComputeHandoverMissionSpec);
		Before->SetStringField(TEXT("object_path"), ComputeHandoverMissionObjectPath);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), ComputeHandoverMissionSpec);
		Proposed->SetStringField(TEXT("action"), ComputeHandoverMissionAction);
		Proposed->SetStringField(TEXT("object_path"), ComputeHandoverMissionObjectPath);
		Proposed->SetStringField(TEXT("mission_id"), ComputeHandoverMissionId);
		Proposed->SetStringField(TEXT("title"), ComputeHandoverMissionTitle);
		Proposed->SetStringField(TEXT("description"), ComputeHandoverMissionDescription);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateComputeHandoverMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_compute_handover_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateComputeHandoverMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindComputeHandoverMissionAssetExact();
		if (Existing && ComputeHandoverMissionMismatchReason(Existing).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateComputeHandoverMission", "Create Compute Handover Mission DataAsset"));
			if (const FString CreateError = CreateComputeHandoverMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = ComputeHandoverMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedComputeHandoverMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), VerifyError, MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		bool bExpectedDirtyPresent = false;
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, ComputeHandoverMissionPackage))
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
			CleanupCreatedComputeHandoverMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("dirty_package_contract"), TEXT("After create, dirty must include only the Compute handover mission package."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(GetEditorWorld()); !PowerError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedComputeHandoverMissionAsset(Created, bPackageWasDirtyBefore, Restored);
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
		Change.After->SetStringField(TEXT("object_path"), ComputeHandoverMissionObjectPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
