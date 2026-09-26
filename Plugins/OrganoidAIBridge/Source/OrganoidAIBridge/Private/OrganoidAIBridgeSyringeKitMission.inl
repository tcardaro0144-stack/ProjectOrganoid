// Create DA_Mission_SyringeKit. Target 2, Advance by 1, Next null. Does not save.
	const TCHAR* SyringeKitMissionSpec = TEXT("syringe_kit_mission_v1");
	const TCHAR* SyringeKitMissionAction = TEXT("create_syringe_kit_mission");
	const TCHAR* SyringeKitMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_SyringeKit");
	const TCHAR* SyringeKitMissionAssetName = TEXT("DA_Mission_SyringeKit");
	const TCHAR* SyringeKitMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_SyringeKit.DA_Mission_SyringeKit");
	const TCHAR* SyringeKitMissionId = TEXT("Mission_SyringeKit");
	const TCHAR* SyringeKitMissionTitle = TEXT("Syringe Kit");
	const TCHAR* SyringeKitMissionDescription = TEXT("The Research Station reveals the full Epitope syringe kit. Recover additional adaptations to expand tactical options.");
	const TCHAR* SyringeKitObjectiveId = TEXT("Obj_RecoverSyringeKit");
	const TCHAR* SyringeKitObjectiveTitle = TEXT("Recover Syringe Kit");
	const TCHAR* SyringeKitObjectiveDescription = TEXT("Recover the Locomotor Disrupt and Optical Disrupt syringes.");
	const TCHAR* SyringeKitEventId = TEXT("Event_SyringeKitRecovered");
	constexpr int32 SyringeKitTargetCount = 2;

	UObject* FindSyringeKitMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, SyringeKitMissionObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(SyringeKitMissionPackage))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, SyringeKitMissionObjectPath);
		}
		return nullptr;
	}

	FString SyringeKitPatchTaskCounting(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during syringe patch.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp)
		{
			return TEXT("TargetProgress missing during syringe patch.");
		}
		TargetProp->SetPropertyValue_InContainer(Objective, SyringeKitTargetCount);
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("EventTriggers missing during syringe patch.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers count is not 1 during syringe patch.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		if (!TrigStruct || !TrigStruct->Struct)
		{
			return TEXT("EventTriggers element missing during syringe patch.");
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
			return TEXT("ProgressDelta missing during syringe patch.");
		}
		return TEXT("");
	}

	FString SyringeKitMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(SyringeKitMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(SyringeKitMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(SyringeKitMissionDescription), DescError))
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
		if (Helper.Num() != 1)
		{
			return TEXT("Tasks count must be 1.");
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		void* Objective = NeuroGeneticsMissionObjectivePtr(Helper.GetRawPtr(0), TaskStruct);
		FStructProperty* ObjectiveProp = TaskStruct ? FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective")) : nullptr;
		if (!Objective || !ObjectiveProp)
		{
			return TEXT("Objective missing.");
		}
		FNameProperty* ObjectiveIdProp = FindFProperty<FNameProperty>(ObjectiveProp->Struct, TEXT("ObjectiveId"));
		if (!ObjectiveIdProp || ObjectiveIdProp->GetPropertyValue_InContainer(Objective) != FName(SyringeKitObjectiveId))
		{
			return TEXT("ObjectiveId must be Obj_RecoverSyringeKit.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Main")))
		{
			return TEXT("Objective type must be Main.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp || TargetProp->GetPropertyValue_InContainer(Objective) != SyringeKitTargetCount)
		{
			return TEXT("TargetProgress must be 2.");
		}
		FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds"));
		if (!PrereqProp)
		{
			return TEXT("PrerequisiteObjectiveIds missing.");
		}
		FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
		if (PrereqHelper.Num() != 0)
		{
			return TEXT("PrerequisiteObjectiveIds must be empty.");
		}
		FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate"));
		if (!AutoProp || !AutoProp->GetPropertyValue_InContainer(Helper.GetRawPtr(0)))
		{
			return TEXT("bAutoActivate must be true.");
		}
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("EventTriggers missing.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(Helper.GetRawPtr(0)));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers must contain one trigger.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		void* TrigElem = TriggerHelper.GetRawPtr(0);
		FNameProperty* EventProp = TrigStruct ? FindFProperty<FNameProperty>(TrigStruct->Struct, TEXT("EventId")) : nullptr;
		if (!EventProp || EventProp->GetPropertyValue_InContainer(TrigElem) != FName(SyringeKitEventId))
		{
			return TEXT("EventId must be Event_SyringeKitRecovered.");
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

	FString ApplySyringeKitMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), SyringeKitMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), SyringeKitMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), SyringeKitMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(SyringeKitEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, SyringeKitObjectiveId, SyringeKitObjectiveTitle,
				SyringeKitObjectiveDescription, true, NoPrereqs, Events);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return SyringeKitPatchTaskCounting(Helper.GetRawPtr(0), TaskStruct);
	}

	FString CleanupCreatedSyringeKitMission(UObject* Asset, bool bPackageWasDirtyBefore)
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

	FString CreateSyringeKitMissionAsset(UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return TEXT("Failed to load mission class.");
		}
		if (FindPackage(nullptr, SyringeKitMissionPackage) || FPackageName::DoesPackageExist(SyringeKitMissionPackage))
		{
			return TEXT("DA_Mission_SyringeKit already exists. Fail closed — no overwrite.");
		}
		UPackage* Package = CreatePackage(SyringeKitMissionPackage);
		UObject* Asset = Package ? NewObject<UObject>(Package, MissionClass, SyringeKitMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_SyringeKit.");
		}
		if (const FString ApplyError = ApplySyringeKitMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			CleanupCreatedSyringeKitMission(Asset, false);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightCreateSyringeKitMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_syringe_kit_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SyringeKitMissionSpec);
		if (!Spec.Equals(SyringeKitMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SyringeKitMissionSpec);
		}
		UObject* Locomotor = FindSyringeAdaptationExact(LocomotorDisruptSpec);
		UObject* Optical = FindSyringeAdaptationExact(OpticalDisruptSpec);
		if (!Locomotor || !SyringeAdaptationMismatchReason(Locomotor, LocomotorDisruptSpec).IsEmpty())
		{
			return TEXT("DA_Adaptation_LocomotorDisrupt must exist and match before the syringe-kit mission.");
		}
		if (!Optical || !SyringeAdaptationMismatchReason(Optical, OpticalDisruptSpec).IsEmpty())
		{
			return TEXT("DA_Adaptation_OpticalDisrupt must exist and match before the syringe-kit mission.");
		}
		UObject* ResearchStation = FindRespecBeatMissionAssetExact();
		if (!ResearchStation)
		{
			return TEXT("DA_Mission_ResearchStation missing.");
		}
		if (const FString ResearchMismatch = RespecBeatMissionMismatchReason(ResearchStation); !ResearchMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ResearchStation must still have Next null. %s"), *ResearchMismatch);
		}
		UObject* Existing = FindSyringeKitMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = SyringeKitMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_SyringeKit mismatch: %s"), *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), SyringeKitMissionAction);
		Proposed->SetStringField(TEXT("spec"), SyringeKitMissionSpec);
		Proposed->SetStringField(TEXT("package"), SyringeKitMissionPackage);
		Proposed->SetStringField(TEXT("mission_id"), SyringeKitMissionId);
		Proposed->SetStringField(TEXT("title"), SyringeKitMissionTitle);
		Proposed->SetStringField(TEXT("objective_id"), SyringeKitObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), SyringeKitEventId);
		Proposed->SetNumberField(TEXT("target"), SyringeKitTargetCount);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateSyringeKitMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_syringe_kit_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateSyringeKitMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindSyringeKitMissionAssetExact();
		if (Existing && SyringeKitMissionMismatchReason(Existing).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateSyringeKitMission", "Create Syringe Kit mission"));
			if (const FString CreateError = CreateSyringeKitMissionAsset(Created); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), CreateError, MakeShared<FBridgeChange>(Change));
			}
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		bool bExpectedDirty = false;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, SyringeKitMissionPackage))
			{
				bExpectedDirty = true;
				break;
			}
		}
		if (!bExpectedDirty || !Created || !SyringeKitMissionMismatchReason(Created).IsEmpty())
		{
			CleanupCreatedSyringeKitMission(Created, false);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Syringe Kit mission did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), SyringeKitMissionPackage);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
