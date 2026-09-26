// Create DA_Mission_WeaponRoster. One Main objective, target 5, Advance by 1, Next null. Does not save.
	const TCHAR* WeaponRosterMissionSpec = TEXT("weapon_roster_mission_v1");
	const TCHAR* WeaponRosterMissionAction = TEXT("create_weapon_roster_mission");
	const TCHAR* WeaponRosterMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_WeaponRoster");
	const TCHAR* WeaponRosterMissionAssetName = TEXT("DA_Mission_WeaponRoster");
	const TCHAR* WeaponRosterMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_WeaponRoster.DA_Mission_WeaponRoster");
	const TCHAR* WeaponRosterMissionId = TEXT("Mission_WeaponRoster");
	const TCHAR* WeaponRosterMissionTitle = TEXT("Weapon Roster");
	const TCHAR* WeaponRosterMissionDescription = TEXT("The armory reveals the remaining weapons. Recover the full roster to expand tactical options.");
	const TCHAR* WeaponRosterObjectiveId = TEXT("Obj_RecoverWeaponRoster");
	const TCHAR* WeaponRosterObjectiveTitle = TEXT("Recover Weapon Roster");
	const TCHAR* WeaponRosterObjectiveDescription = TEXT("Recover the Bio-Stabilizer Pistol, Pulse Carbine, Cryo Injector, Denaturing Shotgun, and Incinerator Lance.");
	const TCHAR* WeaponRosterEventId = TEXT("Event_WeaponRosterRecovered");
	constexpr int32 WeaponRosterTargetCount = 5;

	UObject* FindWeaponRosterMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, WeaponRosterMissionObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(WeaponRosterMissionPackage))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, WeaponRosterMissionObjectPath);
		}
		return nullptr;
	}

	FString WeaponRosterPatchTaskCounting(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during weapon roster patch.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp)
		{
			return TEXT("TargetProgress missing during weapon roster patch.");
		}
		TargetProp->SetPropertyValue_InContainer(Objective, WeaponRosterTargetCount);
		FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers"));
		if (!TriggersProp)
		{
			return TEXT("EventTriggers missing during weapon roster patch.");
		}
		FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
		if (TriggerHelper.Num() != 1)
		{
			return TEXT("EventTriggers count is not 1 during weapon roster patch.");
		}
		FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
		void* TrigElem = TriggerHelper.GetRawPtr(0);
		FString EnumError;
		if (!TrigStruct || !NeuroGeneticsMissionWriteEnumByName(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Advance"), EnumError))
		{
			return EnumError.IsEmpty() ? TEXT("Event trigger missing.") : EnumError;
		}
		FIntProperty* DeltaProp = FindFProperty<FIntProperty>(TrigStruct->Struct, TEXT("ProgressDelta"));
		if (!DeltaProp)
		{
			return TEXT("ProgressDelta missing during weapon roster patch.");
		}
		DeltaProp->SetPropertyValue_InContainer(TrigElem, 1);
		return TEXT("");
	}

	FString WeaponRosterMissionMismatchReason(UObject* Asset)
	{
		if (!Asset || !Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FString IdError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(WeaponRosterMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(WeaponRosterMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(WeaponRosterMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Asset, TEXT("NextMissionAsset")));
		if (!SoftProp || SoftProp->GetPropertyValue_InContainer(Asset).ToSoftObjectPath().IsValid())
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
		FNameProperty* ObjectiveIdProp = ObjectiveProp ? FindFProperty<FNameProperty>(ObjectiveProp->Struct, TEXT("ObjectiveId")) : nullptr;
		if (!Objective || !ObjectiveIdProp || ObjectiveIdProp->GetPropertyValue_InContainer(Objective) != FName(WeaponRosterObjectiveId))
		{
			return TEXT("ObjectiveId must be Obj_RecoverWeaponRoster.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Main")))
		{
			return TEXT("Objective type must be Main.");
		}
		FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress"));
		if (!TargetProp || TargetProp->GetPropertyValue_InContainer(Objective) != WeaponRosterTargetCount)
		{
			return TEXT("TargetProgress must be 5.");
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
		if (!EventProp || EventProp->GetPropertyValue_InContainer(TrigElem) != FName(WeaponRosterEventId))
		{
			return TEXT("EventId must be Event_WeaponRosterRecovered.");
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

	FString ApplyWeaponRosterMissionDefaults(UObject* Asset)
	{
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), WeaponRosterMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), WeaponRosterMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), WeaponRosterMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(WeaponRosterEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, WeaponRosterObjectiveId, WeaponRosterObjectiveTitle,
				WeaponRosterObjectiveDescription, true, NoPrereqs, Events);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return WeaponRosterPatchTaskCounting(Helper.GetRawPtr(0), TaskStruct);
	}

	FString CleanupCreatedWeaponRosterMission(UObject* Asset, bool bPackageWasDirtyBefore)
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

	FString CreateWeaponRosterMissionAsset(UObject*& OutAsset)
	{
		OutAsset = nullptr;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return TEXT("Failed to load mission class.");
		}
		if (FindPackage(nullptr, WeaponRosterMissionPackage) || FPackageName::DoesPackageExist(WeaponRosterMissionPackage))
		{
			return TEXT("DA_Mission_WeaponRoster already exists. Fail closed — no overwrite.");
		}
		UPackage* Package = CreatePackage(WeaponRosterMissionPackage);
		UObject* Asset = Package ? NewObject<UObject>(Package, MissionClass, WeaponRosterMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_WeaponRoster.");
		}
		if (const FString ApplyError = ApplyWeaponRosterMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			CleanupCreatedWeaponRosterMission(Asset, false);
			return ApplyError;
		}
		if (const FString VerifyError = WeaponRosterMissionMismatchReason(Asset); !VerifyError.IsEmpty())
		{
			CleanupCreatedWeaponRosterMission(Asset, false);
			return VerifyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString PreflightCreateWeaponRosterMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_weapon_roster_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), WeaponRosterMissionSpec);
		if (!Spec.Equals(WeaponRosterMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), WeaponRosterMissionSpec);
		}
		for (const FWeaponRosterSpec& WeaponSpec : WeaponRosterSpecs)
		{
			UObject* Weapon = FindWeaponRosterAssetExact(WeaponSpec);
			if (!Weapon || !WeaponRosterMismatchReason(Weapon, WeaponSpec).IsEmpty())
			{
				return FString::Printf(TEXT("%s must exist and match before the weapon roster mission."), WeaponSpec.AssetName);
			}
		}
		UObject* Syringe = FindSyringeKitMissionAssetExact();
		if (const FString SyringeMismatch = SyringeKitMissionMismatchReason(Syringe); !SyringeMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_SyringeKit must still have Next null. %s"), *SyringeMismatch);
		}
		const FString ResearchNext = NeuroAdaptationConnectionNextRevelationReadNext(FindRespecBeatMissionAssetExact());
		if (!ResearchNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ResearchStation.NextMissionAsset must stay DA_Mission_SyringeKit.");
		}
		UObject* Existing = FindWeaponRosterMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = WeaponRosterMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_WeaponRoster mismatch: %s"), *Mismatch);
			}
			bAlreadyExact = true;
		}
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), WeaponRosterMissionAction);
		Proposed->SetStringField(TEXT("spec"), WeaponRosterMissionSpec);
		Proposed->SetStringField(TEXT("package"), WeaponRosterMissionPackage);
		Proposed->SetStringField(TEXT("mission_id"), WeaponRosterMissionId);
		Proposed->SetStringField(TEXT("title"), WeaponRosterMissionTitle);
		Proposed->SetStringField(TEXT("objective_id"), WeaponRosterObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), WeaponRosterEventId);
		Proposed->SetNumberField(TEXT("target"), WeaponRosterTargetCount);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateWeaponRosterMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_weapon_roster_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateWeaponRosterMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindWeaponRosterMissionAssetExact();
		if (Existing && WeaponRosterMissionMismatchReason(Existing).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateWeaponRosterMission", "Create Weapon Roster mission"));
			if (const FString CreateError = CreateWeaponRosterMissionAsset(Created); !CreateError.IsEmpty())
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
			if (PackagesEqual(Dirty, WeaponRosterMissionPackage))
			{
				bExpectedDirty = true;
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (!bExpectedDirty || UnexpectedNew.Num() != 0 || !Created || !WeaponRosterMissionMismatchReason(Created).IsEmpty())
		{
			CleanupCreatedWeaponRosterMission(Created, false);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Weapon Roster mission did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), WeaponRosterMissionPackage);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
