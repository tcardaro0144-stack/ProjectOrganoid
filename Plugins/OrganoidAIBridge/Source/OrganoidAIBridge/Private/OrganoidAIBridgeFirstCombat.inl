// First combat. Mission, Pursuer handoff, Blueprint, and Reactor placement. Does not save.
	const TCHAR* FirstCombatMissionSpec = TEXT("first_combat_mission_v1");
	const TCHAR* FirstCombatMissionAction = TEXT("create_first_combat_mission");
	const TCHAR* FirstCombatMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_FirstCombat");
	const TCHAR* FirstCombatMissionAssetName = TEXT("DA_Mission_FirstCombat");
	const TCHAR* FirstCombatMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_FirstCombat.DA_Mission_FirstCombat");
	const TCHAR* FirstCombatMissionId = TEXT("Mission_FirstCombat");
	const TCHAR* FirstCombatMissionTitle = TEXT("First Combat");
	const TCHAR* FirstCombatMissionDescription = TEXT("A transformed scientist is still wearing the lab coat. It can be stopped. Lytic and the roster both work, and it leaves one Lytic charge.");
	const TCHAR* FirstCombatObjectiveId = TEXT("Obj_DefeatTransformed");
	const TCHAR* FirstCombatObjectiveTitle = TEXT("Defeat the Transformed Scientist");
	const TCHAR* FirstCombatObjectiveDescription = TEXT("Stop the transformed scientist.");
	const TCHAR* FirstCombatEventId = TEXT("Event_FirstCombatDefeated");
	const TCHAR* FirstCombatNextAction = TEXT("set_pursuer_intro_next_first_combat");
	const TCHAR* FirstCombatNextSpec = TEXT("pursuer_intro_next_first_combat_v1");
	const TCHAR* FirstCombatBlueprintAction = TEXT("create_transformed_scientist_blueprint");
	const TCHAR* FirstCombatBlueprintSpec = TEXT("transformed_scientist_blueprint_v1");
	const TCHAR* FirstCombatBlueprintPackage = TEXT("/Game/AI/BP_TransformedScientist");
	const TCHAR* FirstCombatBlueprintObjectPath = TEXT("/Game/AI/BP_TransformedScientist.BP_TransformedScientist");
	const TCHAR* FirstCombatBlueprintClassPath = TEXT("/Game/AI/BP_TransformedScientist.BP_TransformedScientist_C");
	const TCHAR* FirstCombatNativeClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidTransformedScientist");
	const TCHAR* FirstCombatTriggerClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidFirstCombatTrigger");
	const TCHAR* FirstCombatSpawnAction = TEXT("spawn_reactor_first_combat");
	const TCHAR* FirstCombatSpawnSpec = TEXT("reactor_first_combat_v1");
	const TCHAR* FirstCombatActorLabel = TEXT("BP_TransformedScientist");
	const TCHAR* FirstCombatTriggerLabel = TEXT("Reactor_FirstCombatTrigger");
	const FVector FirstCombatActorLocation(-800.f, 800.f, -4710.f);
	const FVector FirstCombatTriggerLocation(-500.f, 0.f, -4710.f);

	UObject* FindFirstCombatMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, FirstCombatMissionObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(FirstCombatMissionPackage))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, FirstCombatMissionObjectPath);
		}
		return nullptr;
	}

	FString FirstCombatMissionMismatchReason(UObject* Asset)
	{
		if (!Asset || !Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FString IdError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(FirstCombatMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(FirstCombatMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(FirstCombatMissionDescription), DescError))
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
				Helper.GetRawPtr(0), TaskStruct, FirstCombatObjectiveId, FirstCombatObjectiveTitle,
				FirstCombatObjectiveDescription, true, 0, nullptr, 1, FirstCombatEventId);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return TEXT("");
	}

	FString ApplyFirstCombatMissionDefaults(UObject* Asset)
	{
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), FirstCombatMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), FirstCombatMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), FirstCombatMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(FirstCombatEventId));
		return NeuroGeneticsMissionBeat3WriteTaskObjective(
			Helper.GetRawPtr(0), TaskStruct, FirstCombatObjectiveId, FirstCombatObjectiveTitle,
			FirstCombatObjectiveDescription, true, NoPrereqs, Events);
	}

	FString PreflightCreateFirstCombatMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_first_combat_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), FirstCombatMissionSpec);
		if (!Spec.Equals(FirstCombatMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), FirstCombatMissionSpec);
		}
		UObject* PursuerIntro = FindPursuerIntroMissionAssetExact();
		if (!PursuerIntro || !PursuerIntroMissionMismatchReason(PursuerIntro).IsEmpty())
		{
			return TEXT("DA_Mission_PursuerIntro must exist with Next null before first combat.");
		}
		UObject* Existing = FindFirstCombatMissionAssetExact();
		const bool bAlreadyExact = Existing && FirstCombatMissionMismatchReason(Existing).IsEmpty();
		if (Existing && !bAlreadyExact)
		{
			return TEXT("DA_Mission_FirstCombat exists but does not match the locked spec.");
		}
		Before->SetBoolField(TEXT("pursuer_intro_next_null"), true);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), FirstCombatMissionAction);
		Proposed->SetStringField(TEXT("spec"), FirstCombatMissionSpec);
		Proposed->SetStringField(TEXT("package"), FirstCombatMissionPackage);
		Proposed->SetStringField(TEXT("mission_id"), FirstCombatMissionId);
		Proposed->SetStringField(TEXT("title"), FirstCombatMissionTitle);
		Proposed->SetStringField(TEXT("description"), FirstCombatMissionDescription);
		Proposed->SetStringField(TEXT("objective_id"), FirstCombatObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), FirstCombatEventId);
		Proposed->SetNumberField(TEXT("target"), 1);
		Proposed->SetBoolField(TEXT("next_null"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateFirstCombatMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_first_combat_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateFirstCombatMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindFirstCombatMissionAssetExact();
		if (Existing && FirstCombatMissionMismatchReason(Existing).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateFirstCombatMission", "Create First Combat mission"));
			UPackage* Package = CreatePackage(FirstCombatMissionPackage);
			Created = Package ? NewObject<UObject>(Package, MissionClass, FirstCombatMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("NewObject failed for DA_Mission_FirstCombat."), MakeShared<FBridgeChange>(Change));
			}
			if (const FString ApplyError = ApplyFirstCombatMissionDefaults(Created); !ApplyError.IsEmpty())
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
			if (PackagesEqual(Dirty, FirstCombatMissionPackage))
			{
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (UnexpectedNew.Num() != 0 || !Created || !FirstCombatMissionMismatchReason(Created).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("First Combat mission did not verify."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), FirstCombatMissionPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSetPursuerIntroNextFirstCombat(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_pursuer_intro_next_first_combat does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), FirstCombatNextSpec);
		if (!Spec.Equals(FirstCombatNextSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), FirstCombatNextSpec);
		}
		UObject* FirstCombat = FindFirstCombatMissionAssetExact();
		if (!FirstCombat || !FirstCombatMissionMismatchReason(FirstCombat).IsEmpty())
		{
			return TEXT("DA_Mission_FirstCombat missing or not exact.");
		}
		UObject* PursuerIntro = FindPursuerIntroMissionAssetExact();
		if (!PursuerIntro)
		{
			return TEXT("DA_Mission_PursuerIntro missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(PursuerIntro);
		const bool bAlreadyExact = LiveNext.Equals(FirstCombatMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString Mismatch = PursuerIntroMissionMismatchReason(PursuerIntro); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_PursuerIntro mismatch: %s"), *Mismatch);
			}
		}
		UObject* WeaponRoster = FindWeaponRosterMissionAssetExact();
		const FString RosterNext = NeuroAdaptationConnectionNextRevelationReadNext(WeaponRoster);
		if (!RosterNext.Equals(PursuerIntroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_WeaponRoster.NextMissionAsset must stay DA_Mission_PursuerIntro.");
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), FirstCombatNextAction);
		Proposed->SetStringField(TEXT("spec"), FirstCombatNextSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), FirstCombatMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetPursuerIntroNextFirstCombat(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_pursuer_intro_next_first_combat must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetPursuerIntroNextFirstCombat(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* PursuerIntro = FindPursuerIntroMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(PursuerIntro);
		if (LiveNext.Equals(FirstCombatMissionObjectPath, ESearchCase::CaseSensitive))
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "PursuerNextFirstCombat", "Link Pursuer intro to First Combat"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(PursuerIntro, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(PursuerIntro, FSoftObjectPtr(FSoftObjectPath(FirstCombatMissionObjectPath)));
			PursuerIntro->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(PursuerIntro);
		if (!AfterNext.Equals(FirstCombatMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Pursuer intro Next did not become DA_Mission_FirstCombat, or power changed."), MakeShared<FBridgeChange>(Change));
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

	UBlueprint* FindTransformedScientistBlueprintExact()
	{
		return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, FirstCombatBlueprintObjectPath));
	}

	FString PreflightCreateTransformedScientistBlueprint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_transformed_scientist_blueprint does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), FirstCombatBlueprintSpec);
		if (!Spec.Equals(FirstCombatBlueprintSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), FirstCombatBlueprintSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, FirstCombatNativeClassPath);
		if (!Native)
		{
			return TEXT("AProjectOrganoidTransformedScientist is not loaded. Rebuild the editor target first.");
		}
		UBlueprint* Existing = FindTransformedScientistBlueprintExact();
		const bool bAlreadyExact = Existing && Existing->ParentClass == Native && Existing->GeneratedClass != nullptr;
		if (Existing && !bAlreadyExact)
		{
			return TEXT("BP_TransformedScientist exists but is not a child of AProjectOrganoidTransformedScientist.");
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), FirstCombatBlueprintAction);
		Proposed->SetStringField(TEXT("spec"), FirstCombatBlueprintSpec);
		Proposed->SetStringField(TEXT("package"), FirstCombatBlueprintPackage);
		Proposed->SetStringField(TEXT("parent"), FirstCombatNativeClassPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateTransformedScientistBlueprint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_transformed_scientist_blueprint must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateTransformedScientistBlueprint(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, FirstCombatNativeClassPath);
		UBlueprint* Existing = FindTransformedScientistBlueprintExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateTransformedScientistBlueprint", "Create BP_TransformedScientist"));
			UPackage* Package = CreatePackage(FirstCombatBlueprintPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_TransformedScientist"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_TransformedScientist."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		if (!Created->GeneratedClass || Created->ParentClass != Native)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("BP_TransformedScientist did not compile as a transformed-scientist child."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), FirstCombatBlueprintPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSpawnReactorFirstCombat(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_reactor_first_combat does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), FirstCombatSpawnSpec);
		if (!Spec.Equals(FirstCombatSpawnSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), FirstCombatSpawnSpec);
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
		const TArray<AActor*> Pursuers = FindByExactLabel(World, TEXT("BP_Pursuer"));
		const TArray<AActor*> PursuerTriggers = FindByExactLabel(World, TEXT("Reactor_PursuerTrigger"));
		if (Pursuers.Num() != 1 || !Pursuers[0]->GetActorLocation().Equals(FVector(-1800.f, 0.f, -4710.f), 1.f)
			|| PursuerTriggers.Num() != 1 || !PursuerTriggers[0]->GetActorLocation().Equals(FVector(-1100.f, 0.f, -4710.f), 1.f))
		{
			return TEXT("BP_Pursuer and Reactor_PursuerTrigger must stay at their Beat 22 locations.");
		}
		UClass* SpawnClass = LoadClass<AActor>(nullptr, FirstCombatBlueprintClassPath);
		UClass* TriggerClass = StaticLoadClass(AActor::StaticClass(), nullptr, FirstCombatTriggerClassPath);
		if (!SpawnClass || !TriggerClass)
		{
			return TEXT("BP_TransformedScientist or the trigger class is not loaded.");
		}
		const TArray<AActor*> Scientists = FindByExactLabel(World, FirstCombatActorLabel);
		const TArray<AActor*> Triggers = FindByExactLabel(World, FirstCombatTriggerLabel);
		const bool bAlreadyExact = Scientists.Num() == 1 && Triggers.Num() == 1
			&& Scientists[0]->GetClass() == SpawnClass
			&& Triggers[0]->GetClass() == TriggerClass
			&& Scientists[0]->GetActorLocation().Equals(FirstCombatActorLocation, 1.f)
			&& Triggers[0]->GetActorLocation().Equals(FirstCombatTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Scientists[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Triggers[0]), ReactorPackage);
		if (!bAlreadyExact)
		{
			if (Scientists.Num() != 0 || Triggers.Num() != 0)
			{
				return TEXT("First combat actors exist but do not match the locked placement.");
			}
			if (!PursuerSpotOpen(World, FirstCombatActorLocation, FirstCombatActorLabel) || !PursuerSpotOpen(World, FirstCombatTriggerLocation, FirstCombatTriggerLabel))
			{
				return TEXT("Reactor area near (-800, 800, -4710) is occupied. ZERO writes. Existing actors stay put.");
			}
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("scientist_location"), TEXT("-800,800,-4710"));
		Before->SetStringField(TEXT("trigger_location"), TEXT("-500,0,-4710"));
		Proposed->SetStringField(TEXT("action"), FirstCombatSpawnAction);
		Proposed->SetStringField(TEXT("spec"), FirstCombatSpawnSpec);
		Proposed->SetStringField(TEXT("package"), ReactorPackage);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("moves_existing_actors"), false);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnReactorFirstCombat(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_reactor_first_combat must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnReactorFirstCombat(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		UClass* SpawnClass = LoadClass<AActor>(nullptr, FirstCombatBlueprintClassPath);
		UClass* TriggerClass = StaticLoadClass(AActor::StaticClass(), nullptr, FirstCombatTriggerClassPath);
		const TArray<AActor*> Existing = FindByExactLabel(World, FirstCombatActorLabel);
		if (Existing.Num() == 1)
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
		AActor* Scientist = nullptr;
		AActor* Trigger = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnFirstCombat", "Place the Reactor first combat"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = ReactorLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Scientist = World->SpawnActor<AActor>(SpawnClass, FirstCombatActorLocation, FRotator::ZeroRotator, Params);
			Trigger = World->SpawnActor<AActor>(TriggerClass, FirstCombatTriggerLocation, FRotator::ZeroRotator, Params);
			if (Scientist)
			{
				Scientist->SetActorLabel(FirstCombatActorLabel, true);
			}
			if (Trigger)
			{
				Trigger->SetActorLabel(FirstCombatTriggerLabel, true);
			}
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				if (Scientist) ActorSub->DestroyActor(Scientist);
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
		const bool bPlaced = Scientist && Trigger
			&& Scientist->GetActorLocation().Equals(FirstCombatActorLocation, 1.f)
			&& Trigger->GetActorLocation().Equals(FirstCombatTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Scientist), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Trigger), ReactorPackage);
		if (!bPlaced || !bReactorDirty || UnexpectedNew.Num() != 0 || !PursuerGuardUnmoved(World).IsEmpty() || !CryoAccessRequirePowerContract(World).IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("First combat placement failed, an unexpected package dirtied, or an existing actor moved."), MakeShared<FBridgeChange>(Change));
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
