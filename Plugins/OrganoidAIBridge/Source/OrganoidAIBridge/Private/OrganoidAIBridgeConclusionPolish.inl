// Conclusion choice polish: credits, save cleanup, Reactor trigger, mission description. Does not save.
	const TCHAR* CreditsRollAction = TEXT("create_credits_roll");
	const TCHAR* CreditsRollSpec = TEXT("credits_roll_v1");
	const TCHAR* CreditsRollPackage = TEXT("/Game/Cinematics/BP_CreditsRoll");
	const TCHAR* CreditsRollObjectPath = TEXT("/Game/Cinematics/BP_CreditsRoll.BP_CreditsRoll");
	const TCHAR* CreditsRollNativePath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidCreditsRoll");
	const TCHAR* CreditsRollLabel = TEXT("BP_CreditsRoll");
	const FVector CreditsRollLocation(0.f, 900.f, -4710.f);

	const TCHAR* SaveCleanupAction = TEXT("create_save_cleanup");
	const TCHAR* SaveCleanupSpec = TEXT("save_cleanup_v1");
	const TCHAR* SaveCleanupPackage = TEXT("/Game/Save/BP_SaveCleanup");
	const TCHAR* SaveCleanupObjectPath = TEXT("/Game/Save/BP_SaveCleanup.BP_SaveCleanup");
	const TCHAR* SaveCleanupNativePath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidSaveCleanup");
	const TCHAR* SaveCleanupLabel = TEXT("BP_SaveCleanup");
	const FVector SaveCleanupLocation(0.f, 1000.f, -4710.f);

	const TCHAR* ConclusionTriggerAction = TEXT("create_conclusion_choice_trigger_blueprint");
	const TCHAR* ConclusionTriggerSpec = TEXT("conclusion_choice_trigger_v1");
	const TCHAR* ConclusionTriggerPackage = TEXT("/Game/Cinematics/BP_ConclusionChoiceTrigger");
	const TCHAR* ConclusionTriggerObjectPath = TEXT("/Game/Cinematics/BP_ConclusionChoiceTrigger.BP_ConclusionChoiceTrigger");
	const TCHAR* ConclusionTriggerNativePath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidConclusionChoiceTrigger");
	const TCHAR* ConclusionTriggerLabel = TEXT("BP_ConclusionChoiceTrigger");
	const FVector ConclusionTriggerLocation(0.f, 800.f, -4710.f);

	const TCHAR* ConclusionPolishSpawnAction = TEXT("spawn_reactor_conclusion_polish");
	const TCHAR* ConclusionPolishSpawnSpec = TEXT("reactor_conclusion_polish_v1");

	const TCHAR* PolishConclusionMissionAction = TEXT("polish_the_conclusion_mission");
	const TCHAR* PolishConclusionMissionSpec = TEXT("the_conclusion_mission_v2");
	const TCHAR* TheConclusionMissionDescriptionV2 = TEXT("The incubator is awake. Reach the control spine overlooking the primary incubator. Destroy collapses the facility and clears the run; Extract makes Nathan the carrier for the harder loop.");

	UBlueprint* FindCreditsRollExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, CreditsRollObjectPath))
		{
			return Cast<UBlueprint>(Found);
		}
		if (FPackageName::DoesPackageExist(CreditsRollPackage))
		{
			return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, CreditsRollObjectPath));
		}
		return nullptr;
	}

	UBlueprint* FindSaveCleanupExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, SaveCleanupObjectPath))
		{
			return Cast<UBlueprint>(Found);
		}
		if (FPackageName::DoesPackageExist(SaveCleanupPackage))
		{
			return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, SaveCleanupObjectPath));
		}
		return nullptr;
	}

	UBlueprint* FindConclusionTriggerExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, ConclusionTriggerObjectPath))
		{
			return Cast<UBlueprint>(Found);
		}
		if (FPackageName::DoesPackageExist(ConclusionTriggerPackage))
		{
			return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, ConclusionTriggerObjectPath));
		}
		return nullptr;
	}

	FString PreflightCreateCreditsRoll(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_credits_roll does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), CreditsRollSpec);
		if (!Spec.Equals(CreditsRollSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CreditsRollSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, CreditsRollNativePath);
		if (!Native)
		{
			return TEXT("ProjectOrganoidCreditsRoll native class missing.");
		}
		Before->SetBoolField(TEXT("exists"), FindCreditsRollExact() != nullptr);
		Proposed->SetStringField(TEXT("package"), CreditsRollPackage);
		Proposed->SetStringField(TEXT("parent"), CreditsRollNativePath);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateCreditsRoll(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_credits_roll must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, CreditsRollNativePath);
		UBlueprint* Existing = FindCreditsRollExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateCreditsRoll", "Create BP_CreditsRoll"));
			UPackage* Package = CreatePackage(CreditsRollPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_CreditsRoll"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_CreditsRoll."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), CreditsRollPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightCreateSaveCleanup(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_save_cleanup does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SaveCleanupSpec);
		if (!Spec.Equals(SaveCleanupSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SaveCleanupSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SaveCleanupNativePath);
		if (!Native)
		{
			return TEXT("ProjectOrganoidSaveCleanup native class missing.");
		}
		Before->SetBoolField(TEXT("exists"), FindSaveCleanupExact() != nullptr);
		Proposed->SetStringField(TEXT("package"), SaveCleanupPackage);
		Proposed->SetStringField(TEXT("parent"), SaveCleanupNativePath);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateSaveCleanup(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_save_cleanup must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SaveCleanupNativePath);
		UBlueprint* Existing = FindSaveCleanupExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateSaveCleanup", "Create BP_SaveCleanup"));
			UPackage* Package = CreatePackage(SaveCleanupPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_SaveCleanup"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_SaveCleanup."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), SaveCleanupPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightCreateConclusionTriggerBlueprint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_conclusion_choice_trigger_blueprint does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ConclusionTriggerSpec);
		if (!Spec.Equals(ConclusionTriggerSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ConclusionTriggerSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, ConclusionTriggerNativePath);
		if (!Native)
		{
			return TEXT("ProjectOrganoidConclusionChoiceTrigger native class missing.");
		}
		Before->SetBoolField(TEXT("exists"), FindConclusionTriggerExact() != nullptr);
		Proposed->SetStringField(TEXT("package"), ConclusionTriggerPackage);
		Proposed->SetStringField(TEXT("parent"), ConclusionTriggerNativePath);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateConclusionTriggerBlueprint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_conclusion_choice_trigger_blueprint must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, ConclusionTriggerNativePath);
		UBlueprint* Existing = FindConclusionTriggerExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateConclusionTrigger", "Create BP_ConclusionChoiceTrigger"));
			UPackage* Package = CreatePackage(ConclusionTriggerPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_ConclusionChoiceTrigger"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_ConclusionChoiceTrigger."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), ConclusionTriggerPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	bool ConclusionPolishPlacementExact(UWorld* World, UClass* CreditsClass, UClass* CleanupClass, UClass* TriggerClass)
	{
		const TArray<AActor*> Credits = FindByExactLabel(World, CreditsRollLabel);
		const TArray<AActor*> Cleanup = FindByExactLabel(World, SaveCleanupLabel);
		const TArray<AActor*> Triggers = FindByExactLabel(World, ConclusionTriggerLabel);
		return Credits.Num() == 1 && Cleanup.Num() == 1 && Triggers.Num() == 1
			&& Credits[0]->GetClass() == CreditsClass
			&& Cleanup[0]->GetClass() == CleanupClass
			&& Triggers[0]->GetClass() == TriggerClass
			&& Credits[0]->GetActorLocation().Equals(CreditsRollLocation, 1.f)
			&& Cleanup[0]->GetActorLocation().Equals(SaveCleanupLocation, 1.f)
			&& Triggers[0]->GetActorLocation().Equals(ConclusionTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Credits[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Cleanup[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Triggers[0]), ReactorPackage);
	}

	FString PreflightSpawnReactorConclusionPolish(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_reactor_conclusion_polish does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ConclusionPolishSpawnSpec);
		if (!Spec.Equals(ConclusionPolishSpawnSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ConclusionPolishSpawnSpec);
		}
		UWorld* World = GetEditorWorld();
		if (const FString Guard = NodeZeroGuardPlacedActors(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		if (FindByExactLabel(World, NodeZeroActorLabel).Num() != 1
			|| FindByExactLabel(World, NodeZeroTerminalLabel).Num() != 1
			|| FindByExactLabel(World, SterlingEscapeTriggerLabel).Num() != 1)
		{
			return TEXT("BP_NodeZeroCore, Terminal_SterlingFinal, and BP_SterlingEscapeTrigger must already exist in Reactor.");
		}
		UBlueprint* CreditsBP = FindCreditsRollExact();
		UBlueprint* CleanupBP = FindSaveCleanupExact();
		UBlueprint* TriggerBP = FindConclusionTriggerExact();
		if (!CreditsBP || !CreditsBP->GeneratedClass || !CleanupBP || !CleanupBP->GeneratedClass || !TriggerBP || !TriggerBP->GeneratedClass)
		{
			return TEXT("BP_CreditsRoll, BP_SaveCleanup, and BP_ConclusionChoiceTrigger must exist first.");
		}
		Before->SetBoolField(TEXT("already_placed"), ConclusionPolishPlacementExact(World, CreditsBP->GeneratedClass, CleanupBP->GeneratedClass, TriggerBP->GeneratedClass));
		Proposed->SetStringField(TEXT("trigger_label"), ConclusionTriggerLabel);
		Proposed->SetStringField(TEXT("package"), ReactorPackage);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnReactorConclusionPolish(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_reactor_conclusion_polish must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UWorld* World = GetEditorWorld();
		UBlueprint* CreditsBP = FindCreditsRollExact();
		UBlueprint* CleanupBP = FindSaveCleanupExact();
		UBlueprint* TriggerBP = FindConclusionTriggerExact();
		UClass* CreditsClass = CreditsBP ? CreditsBP->GeneratedClass.Get() : nullptr;
		UClass* CleanupClass = CleanupBP ? CleanupBP->GeneratedClass.Get() : nullptr;
		UClass* TriggerClass = TriggerBP ? TriggerBP->GeneratedClass.Get() : nullptr;
		if (!World || !CreditsClass || !CleanupClass || !TriggerClass)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("missing"), TEXT("Conclusion polish blueprints or world missing."), MakeShared<FBridgeChange>(Change));
		}
		if (ConclusionPolishPlacementExact(World, CreditsClass, CleanupClass, TriggerClass))
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
		AActor* Credits = nullptr;
		AActor* Cleanup = nullptr;
		AActor* Trigger = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnConclusionPolish", "Place Conclusion Polish in Reactor"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = ReactorLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Credits = World->SpawnActor<AActor>(CreditsClass, CreditsRollLocation, FRotator::ZeroRotator, Params);
			Cleanup = World->SpawnActor<AActor>(CleanupClass, SaveCleanupLocation, FRotator::ZeroRotator, Params);
			Trigger = World->SpawnActor<AActor>(TriggerClass, ConclusionTriggerLocation, FRotator::ZeroRotator, Params);
			if (Credits) { Credits->SetActorLabel(CreditsRollLabel, true); }
			if (Cleanup) { Cleanup->SetActorLabel(SaveCleanupLabel, true); }
			if (Trigger) { Trigger->SetActorLabel(ConclusionTriggerLabel, true); }
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				if (Credits) ActorSub->DestroyActor(Credits);
				if (Cleanup) ActorSub->DestroyActor(Cleanup);
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
		if (!ConclusionPolishPlacementExact(World, CreditsClass, CleanupClass, TriggerClass) || !bReactorDirty || UnexpectedNew.Num() != 0 || !NodeZeroGuardPlacedActors(World).IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Conclusion polish placement failed or guards moved."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetStringField(TEXT("package"), ReactorPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightPolishTheConclusionMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. polish_the_conclusion_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), PolishConclusionMissionSpec);
		if (!Spec.Equals(PolishConclusionMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), PolishConclusionMissionSpec);
		}
		UObject* Asset = FindTheConclusionMissionAssetExact();
		if (!Asset)
		{
			return TEXT("DA_Mission_TheConclusion is missing.");
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString CurrentDesc;
		if (FTextProperty* TextProp = CastField<FTextProperty>(DescProp))
		{
			CurrentDesc = TextProp->GetPropertyValue_InContainer(Asset).ToString();
		}
		Before->SetStringField(TEXT("description"), CurrentDesc);
		Before->SetBoolField(TEXT("already_polished"), CurrentDesc.Equals(TheConclusionMissionDescriptionV2, ESearchCase::CaseSensitive));
		Proposed->SetStringField(TEXT("package"), TheConclusionMissionPackage);
		Proposed->SetStringField(TEXT("description"), TheConclusionMissionDescriptionV2);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecutePolishTheConclusionMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("polish_the_conclusion_mission must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UObject* Asset = FindTheConclusionMissionAssetExact();
		if (!Asset)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("missing"), TEXT("DA_Mission_TheConclusion is missing."), MakeShared<FBridgeChange>(Change));
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString CurrentDesc;
		if (FTextProperty* TextProp = CastField<FTextProperty>(DescProp))
		{
			CurrentDesc = TextProp->GetPropertyValue_InContainer(Asset).ToString();
		}
		if (CurrentDesc.Equals(TheConclusionMissionDescriptionV2, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("polished"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "PolishConclusionMission", "Polish DA_Mission_TheConclusion"));
			if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), TheConclusionMissionDescriptionV2); !Error.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("set_failed"), Error, MakeShared<FBridgeChange>(Change));
			}
			Asset->MarkPackageDirty();
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("polished"), true);
		Change.After->SetStringField(TEXT("package"), TheConclusionMissionPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
