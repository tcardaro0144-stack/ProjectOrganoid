// Pursuer first intro. Mission, Weapon Roster handoff, Blueprint, and Reactor placement. Does not save.
	const TCHAR* PursuerIntroMissionSpec = TEXT("pursuer_intro_mission_v1");
	const TCHAR* PursuerIntroMissionAction = TEXT("create_pursuer_intro_mission");
	const TCHAR* PursuerIntroMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro");
	const TCHAR* PursuerIntroMissionAssetName = TEXT("DA_Mission_PursuerIntro");
	const TCHAR* PursuerIntroMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro.DA_Mission_PursuerIntro");
	const TCHAR* PursuerIntroMissionId = TEXT("Mission_PursuerIntro");
	const TCHAR* PursuerIntroMissionTitle = TEXT("Pursuer");
	const TCHAR* PursuerIntroMissionDescription = TEXT("A recurring presence appears behind the glass. Standing still to kill it with current resources is the wrong assumption.");
	const TCHAR* PursuerIntroObjectiveId = TEXT("Obj_EncounterPursuer");
	const TCHAR* PursuerIntroObjectiveTitle = TEXT("Encounter the Pursuer");
	const TCHAR* PursuerIntroObjectiveDescription = TEXT("Witness the pursuer behind the glass.");
	const TCHAR* PursuerIntroEventId = TEXT("Event_PursuerEncountered");
	const TCHAR* PursuerIntroNextAction = TEXT("set_weapon_roster_next_pursuer_intro");
	const TCHAR* PursuerIntroNextSpec = TEXT("weapon_roster_next_pursuer_intro_v1");
	const TCHAR* PursuerBlueprintAction = TEXT("create_pursuer_blueprint");
	const TCHAR* PursuerBlueprintSpec = TEXT("pursuer_blueprint_v1");
	const TCHAR* PursuerBlueprintPackage = TEXT("/Game/AI/BP_Pursuer");
	const TCHAR* PursuerBlueprintObjectPath = TEXT("/Game/AI/BP_Pursuer.BP_Pursuer");
	const TCHAR* PursuerBlueprintClassPath = TEXT("/Game/AI/BP_Pursuer.BP_Pursuer_C");
	const TCHAR* PursuerNativeClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidPursuer");
	const TCHAR* PursuerTriggerClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidPursuerTrigger");
	const TCHAR* PursuerSpawnAction = TEXT("spawn_reactor_pursuer_intro");
	const TCHAR* PursuerSpawnSpec = TEXT("reactor_pursuer_intro_v1");
	const TCHAR* PursuerActorLabel = TEXT("BP_Pursuer");
	const TCHAR* PursuerTriggerLabel = TEXT("Reactor_PursuerTrigger");
	const FVector PursuerActorLocation(-1800.f, 0.f, -4710.f);
	const FVector PursuerTriggerLocation(-1100.f, 0.f, -4710.f);
	const FVector PursuerTerminalGuard(-1950.f, -1650.f, -4700.f);
	const FVector PursuerCheckpointGuard(25.f, 0.f, -4740.f);

	UObject* FindPursuerIntroMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, PursuerIntroMissionObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(PursuerIntroMissionPackage))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, PursuerIntroMissionObjectPath);
		}
		return nullptr;
	}

	FString PursuerIntroMissionMismatchReason(UObject* Asset)
	{
		if (!Asset || !Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FString IdError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(PursuerIntroMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(PursuerIntroMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(PursuerIntroMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Asset, TEXT("NextMissionAsset")));
		if (!SoftProp || SoftProp->GetPropertyValue_InContainer(Asset).ToSoftObjectPath().IsValid())
		{
			return TEXT("NextMissionAsset must be null.");
		}
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp ? ArrayProp->Inner : nullptr);
		if (!ArrayProp || !TaskStruct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (Helper.Num() != 1)
		{
			return TEXT("Tasks count must be 1.");
		}
		if (const FString TaskError = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0), TaskStruct, PursuerIntroObjectiveId, PursuerIntroObjectiveTitle,
				PursuerIntroObjectiveDescription, true, 0, nullptr, 1, PursuerIntroEventId);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return TEXT("");
	}

	FString ApplyPursuerIntroMissionDefaults(UObject* Asset)
	{
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), PursuerIntroMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), PursuerIntroMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), PursuerIntroMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(PursuerIntroEventId));
		return NeuroGeneticsMissionBeat3WriteTaskObjective(
			Helper.GetRawPtr(0), TaskStruct, PursuerIntroObjectiveId, PursuerIntroObjectiveTitle,
			PursuerIntroObjectiveDescription, true, NoPrereqs, Events);
	}

	FString PreflightCreatePursuerIntroMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_pursuer_intro_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), PursuerIntroMissionSpec);
		if (!Spec.Equals(PursuerIntroMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), PursuerIntroMissionSpec);
		}
		UObject* WeaponRoster = FindWeaponRosterMissionAssetExact();
		if (!WeaponRoster || !WeaponRosterMissionMismatchReason(WeaponRoster).IsEmpty())
		{
			return TEXT("DA_Mission_WeaponRoster must exist with Next null before the pursuer intro.");
		}
		UObject* Existing = FindPursuerIntroMissionAssetExact();
		const bool bAlreadyExact = Existing && PursuerIntroMissionMismatchReason(Existing).IsEmpty();
		if (Existing && !bAlreadyExact)
		{
			return TEXT("DA_Mission_PursuerIntro exists but does not match the locked spec.");
		}
		Before->SetBoolField(TEXT("weapon_roster_next_null"), true);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), PursuerIntroMissionAction);
		Proposed->SetStringField(TEXT("spec"), PursuerIntroMissionSpec);
		Proposed->SetStringField(TEXT("package"), PursuerIntroMissionPackage);
		Proposed->SetStringField(TEXT("mission_id"), PursuerIntroMissionId);
		Proposed->SetStringField(TEXT("title"), PursuerIntroMissionTitle);
		Proposed->SetStringField(TEXT("description"), PursuerIntroMissionDescription);
		Proposed->SetStringField(TEXT("objective_id"), PursuerIntroObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), PursuerIntroEventId);
		Proposed->SetNumberField(TEXT("target"), 1);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreatePursuerIntroMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_pursuer_intro_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreatePursuerIntroMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindPursuerIntroMissionAssetExact();
		if (Existing && PursuerIntroMissionMismatchReason(Existing).IsEmpty())
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
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("create_failed"), TEXT("Failed to load mission class."), MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreatePursuerIntroMission", "Create Pursuer intro mission"));
			UPackage* Package = CreatePackage(PursuerIntroMissionPackage);
			Created = Package ? NewObject<UObject>(Package, MissionClass, PursuerIntroMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("NewObject failed for DA_Mission_PursuerIntro."), MakeShared<FBridgeChange>(Change));
			}
			if (const FString ApplyError = ApplyPursuerIntroMissionDefaults(Created); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		TArray<FString> UnexpectedNew;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, PursuerIntroMissionPackage))
			{
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (UnexpectedNew.Num() != 0 || !Created || !PursuerIntroMissionMismatchReason(Created).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Pursuer intro mission did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), PursuerIntroMissionPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSetWeaponRosterNextPursuerIntro(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_weapon_roster_next_pursuer_intro does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), PursuerIntroNextSpec);
		if (!Spec.Equals(PursuerIntroNextSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), PursuerIntroNextSpec);
		}
		UObject* Intro = FindPursuerIntroMissionAssetExact();
		if (!Intro || !PursuerIntroMissionMismatchReason(Intro).IsEmpty())
		{
			return TEXT("DA_Mission_PursuerIntro missing or not exact.");
		}
		UObject* WeaponRoster = FindWeaponRosterMissionAssetExact();
		if (!WeaponRoster)
		{
			return TEXT("DA_Mission_WeaponRoster missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(WeaponRoster);
		const bool bAlreadyExact = LiveNext.Equals(PursuerIntroMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString Mismatch = WeaponRosterMissionMismatchReason(WeaponRoster); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_WeaponRoster mismatch: %s"), *Mismatch);
			}
		}
		const FString SyringeNext = NeuroAdaptationConnectionNextRevelationReadNext(FindSyringeKitMissionAssetExact());
		if (!SyringeNext.Equals(WeaponRosterMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_SyringeKit.NextMissionAsset must stay DA_Mission_WeaponRoster.");
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), PursuerIntroNextAction);
		Proposed->SetStringField(TEXT("spec"), PursuerIntroNextSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), PursuerIntroMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetWeaponRosterNextPursuerIntro(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_weapon_roster_next_pursuer_intro must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetWeaponRosterNextPursuerIntro(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* WeaponRoster = FindWeaponRosterMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(WeaponRoster);
		if (LiveNext.Equals(PursuerIntroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "WeaponRosterNextPursuer", "Link Weapon Roster to the Pursuer intro"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(WeaponRoster, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(WeaponRoster, FSoftObjectPtr(FSoftObjectPath(PursuerIntroMissionObjectPath)));
			WeaponRoster->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(WeaponRoster);
		if (!AfterNext.Equals(PursuerIntroMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Weapon Roster Next did not become DA_Mission_PursuerIntro, or power changed."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("linked"), true);
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	UBlueprint* FindPursuerBlueprintExact()
	{
		return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, PursuerBlueprintObjectPath));
	}

	FString PreflightCreatePursuerBlueprint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_pursuer_blueprint does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), PursuerBlueprintSpec);
		if (!Spec.Equals(PursuerBlueprintSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), PursuerBlueprintSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, PursuerNativeClassPath);
		if (!Native)
		{
			return TEXT("AProjectOrganoidPursuer is not loaded. Rebuild the editor target first.");
		}
		UBlueprint* Existing = FindPursuerBlueprintExact();
		const bool bAlreadyExact = Existing && Existing->ParentClass == Native && Existing->GeneratedClass != nullptr;
		if (Existing && !bAlreadyExact)
		{
			return TEXT("BP_Pursuer exists but is not a child of AProjectOrganoidPursuer.");
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), PursuerBlueprintAction);
		Proposed->SetStringField(TEXT("spec"), PursuerBlueprintSpec);
		Proposed->SetStringField(TEXT("package"), PursuerBlueprintPackage);
		Proposed->SetStringField(TEXT("parent"), PursuerNativeClassPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreatePursuerBlueprint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_pursuer_blueprint must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreatePursuerBlueprint(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, PursuerNativeClassPath);
		UBlueprint* Existing = FindPursuerBlueprintExact();
		if (Existing && Existing->ParentClass == Native && Existing->GeneratedClass)
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
		UBlueprint* Created = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreatePursuerBlueprint", "Create BP_Pursuer"));
			UPackage* Package = CreatePackage(PursuerBlueprintPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_Pursuer"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_Pursuer."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		if (!Created->GeneratedClass || Created->ParentClass != Native)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("BP_Pursuer did not compile as a pursuer child."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), PursuerBlueprintPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PursuerGuardUnmoved(UWorld* World)
	{
		const TArray<AActor*> Terminal = FindByExactLabel(World, TEXT("Terminal_ControlSpine"));
		if (Terminal.Num() != 1 || !Terminal[0]->GetActorLocation().Equals(PursuerTerminalGuard, 1.f) || !PackagesEqual(ActorOwningPackage(Terminal[0]), ReactorPackage))
		{
			return TEXT("Terminal_ControlSpine must stay unique at (-1950, -1650, -4700) on SL_Epitope_Reactor.");
		}
		const TArray<AActor*> Checkpoint = FindByExactLabel(World, TEXT("Checkpoint_BasinRim"));
		if (Checkpoint.Num() != 1 || !Checkpoint[0]->GetActorLocation().Equals(PursuerCheckpointGuard, 1.f) || !PackagesEqual(ActorOwningPackage(Checkpoint[0]), ReactorPackage))
		{
			return TEXT("Checkpoint_BasinRim must stay unique at (25, 0, -4740) on SL_Epitope_Reactor.");
		}
		return TEXT("");
	}

	bool PursuerSpotOpen(UWorld* World, const FVector& Location, const TCHAR* AllowedLabel)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || Actor->GetActorLabel().Equals(AllowedLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (Actor->GetActorLocation().Equals(Location, 80.f))
			{
				return false;
			}
		}
		return true;
	}

	FString PreflightSpawnReactorPursuerIntro(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_reactor_pursuer_intro does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), PursuerSpawnSpec);
		if (!Spec.Equals(PursuerSpawnSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), PursuerSpawnSpec);
		}
		if (const FString Power = CryoAccessRequirePowerContract(GetEditorWorld()); !Power.IsEmpty())
		{
			return Power;
		}
		UWorld* World = GetEditorWorld();
		ULevel* ReactorLevel = World ? FindLoadedLevelByPackage(World, ReactorPackage) : nullptr;
		if (!World || !ReactorLevel)
		{
			return TEXT("SL_Epitope_Reactor is not loaded.");
		}
		if (const FString Guard = PursuerGuardUnmoved(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		UClass* SpawnClass = LoadClass<AActor>(nullptr, PursuerBlueprintClassPath);
		UClass* TriggerClass = StaticLoadClass(AActor::StaticClass(), nullptr, PursuerTriggerClassPath);
		if (!SpawnClass || !TriggerClass)
		{
			return TEXT("BP_Pursuer or the trigger class is not loaded.");
		}
		const TArray<AActor*> Pursuers = FindByExactLabel(World, PursuerActorLabel);
		const TArray<AActor*> Triggers = FindByExactLabel(World, PursuerTriggerLabel);
		const bool bAlreadyExact = Pursuers.Num() == 1 && Triggers.Num() == 1
			&& Pursuers[0]->GetClass() == SpawnClass
			&& Triggers[0]->GetClass() == TriggerClass
			&& Pursuers[0]->GetActorLocation().Equals(PursuerActorLocation, 1.f)
			&& Triggers[0]->GetActorLocation().Equals(PursuerTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Pursuers[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Triggers[0]), ReactorPackage);
		if (!bAlreadyExact)
		{
			if (Pursuers.Num() != 0 || Triggers.Num() != 0)
			{
				return TEXT("Pursuer actors exist but do not match the locked placement.");
			}
			if (!PursuerSpotOpen(World, PursuerActorLocation, PursuerActorLabel) || !PursuerSpotOpen(World, PursuerTriggerLocation, PursuerTriggerLabel))
			{
				return TEXT("Viewing area near (-1800, 0, -4710) is occupied. ZERO writes. Existing actors stay put.");
			}
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("pursuer_location"), TEXT("-1800,0,-4710"));
		Before->SetStringField(TEXT("trigger_location"), TEXT("-1100,0,-4710"));
		Proposed->SetStringField(TEXT("action"), PursuerSpawnAction);
		Proposed->SetStringField(TEXT("spec"), PursuerSpawnSpec);
		Proposed->SetStringField(TEXT("package"), ReactorPackage);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("moves_existing_actors"), false);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnReactorPursuerIntro(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_reactor_pursuer_intro must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnReactorPursuerIntro(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		UClass* SpawnClass = LoadClass<AActor>(nullptr, PursuerBlueprintClassPath);
		UClass* TriggerClass = StaticLoadClass(AActor::StaticClass(), nullptr, PursuerTriggerClassPath);
		const TArray<AActor*> ExistingPursuers = FindByExactLabel(World, PursuerActorLabel);
		if (ExistingPursuers.Num() == 1)
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("spawned"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		ULevel* ReactorLevel = FindLoadedLevelByPackage(World, ReactorPackage);
		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		AActor* Pursuer = nullptr;
		AActor* Trigger = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnPursuerIntro", "Place the Reactor pursuer intro"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = ReactorLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Pursuer = World->SpawnActor<AActor>(SpawnClass, PursuerActorLocation, FRotator::ZeroRotator, Params);
			Trigger = World->SpawnActor<AActor>(TriggerClass, PursuerTriggerLocation, FRotator::ZeroRotator, Params);
			if (Pursuer)
			{
				Pursuer->SetActorLabel(PursuerActorLabel, true);
			}
			if (Trigger)
			{
				Trigger->SetActorLabel(PursuerTriggerLabel, true);
			}
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				if (Pursuer) ActorSub->DestroyActor(Pursuer);
				if (Trigger) ActorSub->DestroyActor(Trigger);
			}
		};
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		TArray<FString> UnexpectedNew;
		bool bReactorDirty = false;
		for (const FString& Dirty : DirtyAfter)
		{
			if (PackagesEqual(Dirty, ReactorPackage))
			{
				bReactorDirty = true;
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		const bool bPlaced = Pursuer && Trigger
			&& Pursuer->GetActorLocation().Equals(PursuerActorLocation, 1.f)
			&& Trigger->GetActorLocation().Equals(PursuerTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Pursuer), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Trigger), ReactorPackage);
		if (!bPlaced || !bReactorDirty || UnexpectedNew.Num() != 0 || !PursuerGuardUnmoved(World).IsEmpty() || !CryoAccessRequirePowerContract(World).IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Pursuer placement failed, an unexpected package dirtied, or an existing actor moved."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetStringField(TEXT("package"), ReactorPackage);
		Change.After->SetArrayField(TEXT("unexpected_new_dirty"), DirtyPackageJsonArray(UnexpectedNew));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
