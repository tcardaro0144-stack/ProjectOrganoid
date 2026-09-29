// Sterling Escape cinematic + Reactor trigger. Does not save.
	const TCHAR* SterlingEscapeCinematicAction = TEXT("create_sterling_escape_cinematic");
	const TCHAR* SterlingEscapeCinematicSpec = TEXT("sterling_escape_cinematic_v1");
	const TCHAR* SterlingEscapeCinematicPackage = TEXT("/Game/Cinematics/BP_SterlingEscapeCinematic");
	const TCHAR* SterlingEscapeCinematicObjectPath = TEXT("/Game/Cinematics/BP_SterlingEscapeCinematic.BP_SterlingEscapeCinematic");
	const TCHAR* SterlingEscapeCinematicClassPath = TEXT("/Game/Cinematics/BP_SterlingEscapeCinematic.BP_SterlingEscapeCinematic_C");
	const TCHAR* SterlingEscapeCinematicNativePath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidSterlingEscapeCinematic");
	const TCHAR* SterlingEscapeTriggerAction = TEXT("create_sterling_escape_trigger_blueprint");
	const TCHAR* SterlingEscapeTriggerSpec = TEXT("sterling_escape_trigger_v1");
	const TCHAR* SterlingEscapeTriggerPackage = TEXT("/Game/Cinematics/BP_SterlingEscapeTrigger");
	const TCHAR* SterlingEscapeTriggerObjectPath = TEXT("/Game/Cinematics/BP_SterlingEscapeTrigger.BP_SterlingEscapeTrigger");
	const TCHAR* SterlingEscapeTriggerClassPath = TEXT("/Game/Cinematics/BP_SterlingEscapeTrigger.BP_SterlingEscapeTrigger_C");
	const TCHAR* SterlingEscapeTriggerNativePath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidSterlingEscapeTrigger");
	const TCHAR* SterlingEscapeSpawnAction = TEXT("spawn_reactor_sterling_escape");
	const TCHAR* SterlingEscapeSpawnSpec = TEXT("reactor_sterling_escape_v1");
	const TCHAR* SterlingEscapeCinematicLabel = TEXT("BP_SterlingEscapeCinematic");
	const TCHAR* SterlingEscapeTriggerLabel = TEXT("BP_SterlingEscapeTrigger");
	const FVector SterlingEscapeCinematicLocation(-200.f, 500.f, -4710.f);
	const FVector SterlingEscapeTriggerLocation(-200.f, 600.f, -4710.f);

	UBlueprint* FindSterlingEscapeCinematicExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, SterlingEscapeCinematicObjectPath))
		{
			return Cast<UBlueprint>(Found);
		}
		if (FPackageName::DoesPackageExist(SterlingEscapeCinematicPackage))
		{
			return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, SterlingEscapeCinematicObjectPath));
		}
		return nullptr;
	}

	UBlueprint* FindSterlingEscapeTriggerExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, SterlingEscapeTriggerObjectPath))
		{
			return Cast<UBlueprint>(Found);
		}
		if (FPackageName::DoesPackageExist(SterlingEscapeTriggerPackage))
		{
			return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, SterlingEscapeTriggerObjectPath));
		}
		return nullptr;
	}

	FString PreflightCreateSterlingEscapeCinematic(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_sterling_escape_cinematic does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SterlingEscapeCinematicSpec);
		if (!Spec.Equals(SterlingEscapeCinematicSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SterlingEscapeCinematicSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SterlingEscapeCinematicNativePath);
		if (!Native)
		{
			return TEXT("ProjectOrganoidSterlingEscapeCinematic native class missing.");
		}
		Before->SetBoolField(TEXT("exists"), FindSterlingEscapeCinematicExact() != nullptr);
		Proposed->SetStringField(TEXT("package"), SterlingEscapeCinematicPackage);
		Proposed->SetStringField(TEXT("parent"), SterlingEscapeCinematicNativePath);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateSterlingEscapeCinematic(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_sterling_escape_cinematic must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SterlingEscapeCinematicNativePath);
		UBlueprint* Existing = FindSterlingEscapeCinematicExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateSterlingEscapeCinematic", "Create BP_SterlingEscapeCinematic"));
			UPackage* Package = CreatePackage(SterlingEscapeCinematicPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_SterlingEscapeCinematic"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_SterlingEscapeCinematic."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetStringField(TEXT("package"), SterlingEscapeCinematicPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightCreateSterlingEscapeTriggerBlueprint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_sterling_escape_trigger_blueprint does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SterlingEscapeTriggerSpec);
		if (!Spec.Equals(SterlingEscapeTriggerSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SterlingEscapeTriggerSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SterlingEscapeTriggerNativePath);
		if (!Native)
		{
			return TEXT("ProjectOrganoidSterlingEscapeTrigger native class missing.");
		}
		Before->SetBoolField(TEXT("exists"), FindSterlingEscapeTriggerExact() != nullptr);
		Proposed->SetStringField(TEXT("package"), SterlingEscapeTriggerPackage);
		Proposed->SetStringField(TEXT("parent"), SterlingEscapeTriggerNativePath);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateSterlingEscapeTriggerBlueprint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_sterling_escape_trigger_blueprint must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, SterlingEscapeTriggerNativePath);
		UBlueprint* Existing = FindSterlingEscapeTriggerExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateSterlingEscapeTrigger", "Create BP_SterlingEscapeTrigger"));
			UPackage* Package = CreatePackage(SterlingEscapeTriggerPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_SterlingEscapeTrigger"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_SterlingEscapeTrigger."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetStringField(TEXT("package"), SterlingEscapeTriggerPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	bool SterlingEscapePlacementExact(UWorld* World, UClass* CinematicClass, UClass* TriggerClass)
	{
		const TArray<AActor*> Cinematics = FindByExactLabel(World, SterlingEscapeCinematicLabel);
		const TArray<AActor*> Triggers = FindByExactLabel(World, SterlingEscapeTriggerLabel);
		return Cinematics.Num() == 1 && Triggers.Num() == 1
			&& Cinematics[0]->GetClass() == CinematicClass
			&& Triggers[0]->GetClass() == TriggerClass
			&& Cinematics[0]->GetActorLocation().Equals(SterlingEscapeCinematicLocation, 1.f)
			&& Triggers[0]->GetActorLocation().Equals(SterlingEscapeTriggerLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Cinematics[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Triggers[0]), ReactorPackage);
	}

	FString PreflightSpawnReactorSterlingEscape(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_reactor_sterling_escape does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SterlingEscapeSpawnSpec);
		if (!Spec.Equals(SterlingEscapeSpawnSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SterlingEscapeSpawnSpec);
		}
		UWorld* World = GetEditorWorld();
		if (const FString Guard = NodeZeroGuardPlacedActors(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		if (FindByExactLabel(World, NodeZeroActorLabel).Num() != 1 || FindByExactLabel(World, NodeZeroTerminalLabel).Num() != 1)
		{
			return TEXT("BP_NodeZeroCore and Terminal_SterlingFinal must already exist in Reactor.");
		}
		UBlueprint* CinematicBP = FindSterlingEscapeCinematicExact();
		UBlueprint* TriggerBP = FindSterlingEscapeTriggerExact();
		if (!CinematicBP || !CinematicBP->GeneratedClass || !TriggerBP || !TriggerBP->GeneratedClass)
		{
			return TEXT("BP_SterlingEscapeCinematic and BP_SterlingEscapeTrigger must exist first.");
		}
		Before->SetBoolField(TEXT("already_placed"), SterlingEscapePlacementExact(World, CinematicBP->GeneratedClass, TriggerBP->GeneratedClass));
		Proposed->SetStringField(TEXT("cinematic_label"), SterlingEscapeCinematicLabel);
		Proposed->SetStringField(TEXT("trigger_label"), SterlingEscapeTriggerLabel);
		Proposed->SetStringField(TEXT("package"), ReactorPackage);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnReactorSterlingEscape(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_reactor_sterling_escape must run on the game thread."), MakeShared<FBridgeChange>(Change));
		}
		UWorld* World = GetEditorWorld();
		UBlueprint* CinematicBP = FindSterlingEscapeCinematicExact();
		UBlueprint* TriggerBP = FindSterlingEscapeTriggerExact();
		UClass* CinematicClass = CinematicBP ? CinematicBP->GeneratedClass.Get() : nullptr;
		UClass* TriggerClass = TriggerBP ? TriggerBP->GeneratedClass.Get() : nullptr;
		if (!World || !CinematicClass || !TriggerClass)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("missing"), TEXT("Sterling escape blueprints or world missing."), MakeShared<FBridgeChange>(Change));
		}
		if (SterlingEscapePlacementExact(World, CinematicClass, TriggerClass))
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
		AActor* Cinematic = nullptr;
		AActor* Trigger = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnSterlingEscape", "Place Sterling Escape in Reactor"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = ReactorLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Cinematic = World->SpawnActor<AActor>(CinematicClass, SterlingEscapeCinematicLocation, FRotator::ZeroRotator, Params);
			Trigger = World->SpawnActor<AActor>(TriggerClass, SterlingEscapeTriggerLocation, FRotator::ZeroRotator, Params);
			if (Cinematic)
			{
				Cinematic->SetActorLabel(SterlingEscapeCinematicLabel, true);
			}
			if (Trigger)
			{
				Trigger->SetActorLabel(SterlingEscapeTriggerLabel, true);
			}
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				if (Cinematic) ActorSub->DestroyActor(Cinematic);
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
		if (!SterlingEscapePlacementExact(World, CinematicClass, TriggerClass) || !bReactorDirty || UnexpectedNew.Num() != 0 || !NodeZeroGuardPlacedActors(World).IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Sterling Escape placement failed or guards moved."), MakeShared<FBridgeChange>(Change));
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
