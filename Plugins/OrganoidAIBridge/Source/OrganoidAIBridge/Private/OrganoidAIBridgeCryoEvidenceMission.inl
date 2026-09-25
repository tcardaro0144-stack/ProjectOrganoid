// Create DA_Mission_CryoEvidence. Does not save, move actors, or change power.
	const TCHAR* CryoEvidenceMissionSpec = TEXT("cryo_evidence_mission_v1");
	const TCHAR* CryoEvidenceMissionAction = TEXT("create_cryo_evidence_mission");
	const TCHAR* CryoEvidenceMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_CryoEvidence");
	const TCHAR* CryoEvidenceMissionAssetName = TEXT("DA_Mission_CryoEvidence");
	const TCHAR* CryoEvidenceMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_CryoEvidence.DA_Mission_CryoEvidence");
	const TCHAR* CryoEvidenceMissionId = TEXT("Mission_CryoEvidence");
	const TCHAR* CryoEvidenceMissionTitle = TEXT("Lot Numbers");
	const TCHAR* CryoEvidenceMissionDescription = TEXT("The cryo manifests don't match the specimen logs. Recover the remaining facility documents downstairs.");
	const TCHAR* CryoEvidenceObjectiveId = TEXT("Obj_RecoverCryoEvidence");
	const TCHAR* CryoEvidenceObjectiveTitle = TEXT("Recover Cryo Evidence");
	const TCHAR* CryoEvidenceObjectiveDescription = TEXT("Recover the remaining Cryo documents.");
	const TCHAR* CryoEvidenceEventId = TEXT("Event_CryoEvidenceRecovered");
	constexpr int32 CryoEvidenceTargetCount = 3;

	UObject* FindCryoEvidenceMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, CryoEvidenceMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, CryoEvidenceMissionObjectPath);
	}

	FString CryoEvidencePatchTaskCounting(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during evidence patch.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp)
		{
			return TEXT("TargetProgress missing during evidence patch.");
		}
		TargetProp->SetPropertyValue_InContainer(Objective, CryoEvidenceTargetCount);
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("EventTriggers missing during evidence patch.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers count is not 1 during evidence patch.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		if (!TrigStruct || !TrigStruct->Struct)
		{
			return TEXT("EventTriggers element missing during evidence patch.");
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
			return TEXT("ProgressDelta missing during evidence patch.");
		}
		return TEXT("");
	}

	FString CryoEvidenceMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(CryoEvidenceMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(CryoEvidenceMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(CryoEvidenceMissionDescription), DescError))
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
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
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
		void* Objective = NeuroGeneticsMissionObjectivePtr(Helper.GetRawPtr(0), TaskStruct);
		FStructProperty* ObjectiveProp = TaskStruct ? FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective")) : nullptr;
		if (!Objective || !ObjectiveProp)
		{
			return TEXT("Objective missing.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp || TargetProp->GetPropertyValue_InContainer(Objective) != CryoEvidenceTargetCount)
		{
			return TEXT("TargetProgress must be 3.");
		}
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(0)));
		if (!TriggersProp || TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers must contain one Advance trigger.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		void* TrigElem = TriggerHelper.GetRawPtr(0);
		FNameProperty* EventProp = TrigStruct ? FindFProperty<FNameProperty>(TrigStruct->Struct, TEXT("EventId")) : nullptr;
		if (!EventProp || EventProp->GetPropertyValue_InContainer(TrigElem) != FName(CryoEvidenceEventId))
		{
			return TEXT("EventId must be Event_CryoEvidenceRecovered.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Advance")))
		{
			return TEXT("Event action must be Advance.");
		}
		FIntProperty* DeltaProp = FindFProperty<FIntProperty>(TrigStruct->Struct, TEXT("ProgressDelta"));
		if (!DeltaProp || DeltaProp->GetPropertyValue_InContainer(TrigElem) != 1)
		{
			return TEXT("ProgressDelta must be 1.");
		}
		return TEXT("");
	}

	FString CryoEvidenceMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), CryoEvidenceMissionSpec);
		if (!Spec.Equals(CryoEvidenceMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoEvidenceMissionSpec);
		}
		return TEXT("");
	}

	FString ApplyCryoEvidenceMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), CryoEvidenceMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), CryoEvidenceMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), CryoEvidenceMissionDescription); !Error.IsEmpty())
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
		if (!ArrayProp || !TaskStruct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(1);
		TArray<FName> NoPrereqs;
		TArray<FName> Events;
		Events.Add(FName(CryoEvidenceEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, CryoEvidenceObjectiveId, CryoEvidenceObjectiveTitle,
				CryoEvidenceObjectiveDescription, true, NoPrereqs, Events);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return CryoEvidencePatchTaskCounting(Helper.GetRawPtr(0), TaskStruct);
	}

	FString CleanupCreatedCryoEvidenceMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore)
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

	FString CreateCryoEvidenceMissionAsset(UObject*& OutAsset, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return TEXT("Failed to load mission class.");
		}
		UPackage* ExistingPackage = FindPackage(nullptr, CryoEvidenceMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(CryoEvidenceMissionPackage);
		UObject* Asset = Package ? NewObject<UObject>(Package, MissionClass, CryoEvidenceMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_CryoEvidence.");
		}
		if (const FString ApplyError = ApplyCryoEvidenceMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			CleanupCreatedCryoEvidenceMissionAsset(Asset, bOutPackageWasDirtyBefore);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightCreateCryoEvidenceMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_cryo_evidence_mission does not save.");
		}
		if (const FString OverrideError = CryoEvidenceMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Existing = FindCryoEvidenceMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = CryoEvidenceMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_CryoEvidence mismatch: %s"), *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), CryoEvidenceMissionAction);
		Proposed->SetStringField(TEXT("mission_id"), CryoEvidenceMissionId);
		Proposed->SetStringField(TEXT("title"), CryoEvidenceMissionTitle);
		Proposed->SetStringField(TEXT("objective_id"), CryoEvidenceObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), CryoEvidenceEventId);
		Proposed->SetNumberField(TEXT("target"), CryoEvidenceTargetCount);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateCryoEvidenceMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_cryo_evidence_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateCryoEvidenceMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindCryoEvidenceMissionAssetExact();
		if (Existing && CryoEvidenceMissionMismatchReason(Existing).IsEmpty())
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
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateCryoEvidenceMission", "Create Cryo Evidence Mission DataAsset"));
			if (const FString CreateError = CreateCryoEvidenceMissionAsset(Created, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = CryoEvidenceMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			CleanupCreatedCryoEvidenceMissionAsset(Created, bPackageWasDirtyBefore);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), VerifyError, MakeShared<FBridgeChange>(Change));
		}
		bool bExpectedDirtyPresent = false;
		int32 Unexpected = 0;
		for (const FString& Dirty : CollectDirtyPackageNamesSorted())
		{
			if (PackagesEqual(Dirty, CryoEvidenceMissionPackage))
			{
				bExpectedDirtyPresent = true;
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				++Unexpected;
			}
		}
		if (!bExpectedDirtyPresent || Unexpected != 0 || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			CleanupCreatedCryoEvidenceMissionAsset(Created, bPackageWasDirtyBefore);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Cryo evidence create did not leave only its package dirty."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
