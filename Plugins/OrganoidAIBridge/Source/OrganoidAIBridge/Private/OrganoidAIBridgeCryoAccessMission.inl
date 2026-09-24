// Fixed Cryo access mission — create_cryo_access_mission / cryo_access_mission_v1.
// Preview is mutation-free. Apply creates only the exact asset; never saves; never changes power.
	const TCHAR* CryoAccessMissionSpec = TEXT("cryo_access_mission_v1");
	const TCHAR* CryoAccessMissionAction = TEXT("create_cryo_access_mission");
	const TCHAR* CryoAccessMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_CryoAccess");
	const TCHAR* CryoAccessMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_CryoAccess.DA_Mission_CryoAccess");
	const TCHAR* CryoAccessMissionAssetName = TEXT("DA_Mission_CryoAccess");
	const TCHAR* CryoAccessMissionId = TEXT("Mission_CryoAccess");
	const TCHAR* CryoAccessMissionTitle = TEXT("Open the Cryo Route");
	const TCHAR* CryoAccessMissionDescription =
		TEXT("Cryo is still on emergency backup. Restore its power to make the route legitimately available.");
	const TCHAR* CryoAccessObjectiveId = TEXT("Obj_RestoreCryoPower");
	const TCHAR* CryoAccessObjectiveTitle = TEXT("Restore Cryo Power");
	const TCHAR* CryoAccessObjectiveDescription = TEXT("Engage the Cryo backup power controls.");
	const TCHAR* CryoAccessEventId = TEXT("Event_CryoBackupEngaged");
	const TCHAR* CryoAccessPanelLabel = TEXT("PowerPanel_CryoBackup");
	const TCHAR* CryoAccessCheckpointLabel = TEXT("Checkpoint_FreightAirlock");
	const FVector CryoAccessCheckpointLocation(1950.f, 0.f, -2340.f);

	TSharedRef<FJsonObject> CryoAccessMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), CryoAccessObjectiveId);
		Task->SetStringField(TEXT("title"), CryoAccessObjectiveTitle);
		Task->SetStringField(TEXT("description"), CryoAccessObjectiveDescription);
		Task->SetStringField(TEXT("category"), TEXT("Main"));
		Task->SetNumberField(TEXT("target_count"), 1);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("complete_event"), CryoAccessEventId);
		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));
		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), CryoAccessMissionId);
		Mission->SetStringField(TEXT("mission_title"), CryoAccessMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), CryoAccessMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString CryoAccessMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), CryoAccessMissionSpec);
		if (!Spec.Equals(CryoAccessMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoAccessMissionSpec);
		}
		return TEXT("");
	}

	FString CryoAccessRequirePowerContract(UWorld* World)
	{
		const FString Facility = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 0);
		const FString Admin = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 1);
		const FString Neuro = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2);
		const FString Cryo = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3);
		const FString Compute = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 4);
		const FString Reactor = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 5);
		if (!Facility.Contains(TEXT("Online"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Facility power must be Online (live '%s'). ZERO writes."), *Facility);
		}
		if (!Admin.Contains(TEXT("Online"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Admin power must be Online (live '%s'). ZERO writes."), *Admin);
		}
		if (Neuro.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase) || Neuro.IsEmpty())
		{
			return FString::Printf(TEXT("NeuroGenetics power must stay Online or the editor Emergency seed (live '%s'). ZERO writes."), *Neuro);
		}
		if (!Cryo.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Cryo power must be Blackout (live '%s'). ZERO writes."), *Cryo);
		}
		if (!Compute.Contains(TEXT("Online"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Compute power must be Online (live '%s'). ZERO writes."), *Compute);
		}
		if (!Reactor.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Reactor power must be Emergency (live '%s'). ZERO writes."), *Reactor);
		}
		return TEXT("");
	}

	FString CryoAccessRequireEditorStable()
	{
		if (!IsInGameThread())
		{
			return TEXT("Cryo access writes must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before a Cryo access write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("Editor world missing.");
		}
		if (const FString Root = NeuroRevelationRequireCleanLoadedPackage(EpitopePackage); !Root.IsEmpty())
		{
			return Root;
		}
		if (const FString Admin = NeuroRevelationRequireCleanLoadedPackage(AdminPackage); !Admin.IsEmpty())
		{
			return Admin;
		}
		if (const FString Neuro = NeuroRevelationRequireCleanLoadedPackage(NeuroPackage); !Neuro.IsEmpty())
		{
			return Neuro;
		}
		if (const FString Cryo = NeuroRevelationRequireCleanLoadedPackage(CryoPackage); !Cryo.IsEmpty())
		{
			return Cryo;
		}
		const TArray<AActor*> Panels = FindByExactLabel(World, CryoAccessPanelLabel);
		if (Panels.Num() != 1 || !Panels[0])
		{
			return FString::Printf(TEXT("Keep-list actor '%s' must be unique. count=%d. ZERO writes."), CryoAccessPanelLabel, Panels.Num());
		}
		if (!ClassName(Panels[0]).Contains(TEXT("ProjectOrganoidPowerPanel")))
		{
			return FString::Printf(TEXT("%s class '%s' is not a power panel. ZERO writes."), CryoAccessPanelLabel, *ClassName(Panels[0]));
		}
		if (!PackagesEqual(ActorOwningPackage(Panels[0]), CryoPackage))
		{
			return FString::Printf(TEXT("%s owner '%s' is not Cryo. ZERO writes."), CryoAccessPanelLabel, *ActorOwningPackage(Panels[0]));
		}
		const TArray<AActor*> Checkpoints = FindByExactLabel(World, CryoAccessCheckpointLabel);
		if (Checkpoints.Num() != 1 || !Checkpoints[0])
		{
			return FString::Printf(TEXT("Keep-list actor '%s' must be unique. count=%d. ZERO writes."), CryoAccessCheckpointLabel, Checkpoints.Num());
		}
		if (!PackagesEqual(ActorOwningPackage(Checkpoints[0]), CryoPackage))
		{
			return FString::Printf(TEXT("%s owner '%s' is not Cryo. ZERO writes."), CryoAccessCheckpointLabel, *ActorOwningPackage(Checkpoints[0]));
		}
		if (!LocationMatches(Checkpoints[0]->GetActorLocation(), CryoAccessCheckpointLocation))
		{
			const FVector Loc = Checkpoints[0]->GetActorLocation();
			return FString::Printf(
				TEXT("Keep-list actor '%s' location (%.2f, %.2f, %.2f) is not the freight airlock. ZERO writes."),
				CryoAccessCheckpointLabel, Loc.X, Loc.Y, Loc.Z);
		}
		return CryoAccessRequirePowerContract(World);
	}

	UObject* FindCryoAccessMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, CryoAccessMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, CryoAccessMissionObjectPath);
	}

	FString CryoAccessMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(CryoAccessMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(CryoAccessMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(Asset, DescProp, MakeShared<FJsonValueString>(CryoAccessMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			if (Soft.ToSoftObjectPath().IsValid())
			{
				return FString::Printf(TEXT("NextMissionAsset must be null, got '%s'"), *Soft.ToSoftObjectPath().ToString());
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
				Helper.GetRawPtr(0), TaskStruct, CryoAccessObjectiveId, CryoAccessObjectiveTitle,
				CryoAccessObjectiveDescription, true, 0, nullptr, 1, CryoAccessEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString ApplyCryoAccessMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), CryoAccessMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), CryoAccessMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), CryoAccessMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(CryoAccessEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, CryoAccessObjectiveId, CryoAccessObjectiveTitle,
				CryoAccessObjectiveDescription, true, NoPrereqs, Events);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString CleanupCreatedCryoAccessMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateCryoAccessMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}
		UPackage* ExistingPackage = FindPackage(nullptr, CryoAccessMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(CryoAccessMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), CryoAccessMissionPackage);
		}
		UObject* Asset = NewObject<UObject>(Package, MissionClass, CryoAccessMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_CryoAccess.");
		}
		if (const FString ApplyError = ApplyCryoAccessMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoAccessMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateCryoAccessMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_cryo_access_mission does not save.");
		}
		if (const FString OverrideError = CryoAccessMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Existing = FindCryoAccessMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = CryoAccessMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_CryoAccess exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."), *Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return FString::Printf(TEXT("Mission class '%s' unresolved."), NeuroGeneticsMissionClassPath);
		}
		Before->SetStringField(TEXT("spec"), CryoAccessMissionSpec);
		Before->SetStringField(TEXT("action"), CryoAccessMissionAction);
		Before->SetStringField(TEXT("object_path"), CryoAccessMissionObjectPath);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), CryoAccessMissionSpec);
		Proposed->SetStringField(TEXT("action"), CryoAccessMissionAction);
		Proposed->SetStringField(TEXT("object_path"), CryoAccessMissionObjectPath);
		Proposed->SetObjectField(TEXT("mission"), CryoAccessMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateCryoAccessMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_cryo_access_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateCryoAccessMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		UObject* Existing = FindCryoAccessMissionAssetExact();
		if (Existing && CryoAccessMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), CryoAccessMissionObjectPath);
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("mismatch"), TEXT("DA_Mission_CryoAccess exists but is not exact. Fail closed. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateCryoAccessMission", "Create Cryo Access Mission DataAsset"));
			if (const FString CreateError = CreateCryoAccessMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError), MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = CryoAccessMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoAccessMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError), MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(CryoAccessMissionPackage);
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
			CleanupCreatedCryoAccessMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("dirty_package_contract"), TEXT("After create, dirty must include only the Cryo access mission package."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(GetEditorWorld()); !PowerError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedCryoAccessMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("power_changed"), PowerError, MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetStringField(TEXT("object_path"), CryoAccessMissionObjectPath);
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("unlocks_cryo"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
