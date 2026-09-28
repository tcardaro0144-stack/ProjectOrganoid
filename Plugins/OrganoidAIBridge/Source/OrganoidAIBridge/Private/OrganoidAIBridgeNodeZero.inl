// Node Zero. Mission, First Combat handoff, Blueprint, and Reactor placement. Does not save.
	const TCHAR* NodeZeroMissionSpec = TEXT("node_zero_mission_v1");
	const TCHAR* NodeZeroMissionAction = TEXT("create_node_zero_mission");
	const TCHAR* NodeZeroMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NodeZero");
	const TCHAR* NodeZeroMissionAssetName = TEXT("DA_Mission_NodeZero");
	const TCHAR* NodeZeroMissionObjectPath = TEXT("/Game/Data/Missions/DA_Mission_NodeZero.DA_Mission_NodeZero");
	const TCHAR* NodeZeroMissionId = TEXT("Mission_NodeZero");
	const TCHAR* NodeZeroMissionTitle = TEXT("Node Zero");
	const TCHAR* NodeZeroMissionDescription = TEXT("Sterling's confession points to the core. The Organoid's heart is still beating. Shut it down or take it with you.");
	const TCHAR* NodeZeroReachObjectiveId = TEXT("Obj_ReachNodeZero");
	const TCHAR* NodeZeroReachObjectiveTitle = TEXT("Reach Node Zero");
	const TCHAR* NodeZeroReachObjectiveDescription = TEXT("Shut down the Organoid's heart or take it with you.");
	const TCHAR* NodeZeroReachEventId = TEXT("Event_NodeZeroReached");
	const TCHAR* NodeZeroFateObjectiveId = TEXT("Obj_ChooseFate");
	const TCHAR* NodeZeroFateObjectiveTitle = TEXT("Choose a Fate");
	const TCHAR* NodeZeroFateObjectiveDescription = TEXT("Destroy the core or extract it.");
	const TCHAR* NodeZeroFateEventId = TEXT("Event_FateChosen");
	const TCHAR* NodeZeroNextAction = TEXT("set_first_combat_next_node_zero");
	const TCHAR* NodeZeroNextSpec = TEXT("first_combat_next_node_zero_v1");
	const TCHAR* NodeZeroBlueprintAction = TEXT("create_node_zero_blueprint");
	const TCHAR* NodeZeroBlueprintSpec = TEXT("node_zero_blueprint_v1");
	const TCHAR* NodeZeroBlueprintPackage = TEXT("/Game/AI/BP_NodeZeroCore");
	const TCHAR* NodeZeroBlueprintObjectPath = TEXT("/Game/AI/BP_NodeZeroCore.BP_NodeZeroCore");
	const TCHAR* NodeZeroBlueprintClassPath = TEXT("/Game/AI/BP_NodeZeroCore.BP_NodeZeroCore_C");
	const TCHAR* NodeZeroNativeClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidNodeZeroCore");
	const TCHAR* NodeZeroTerminalClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidTerminal");
	const TCHAR* NodeZeroSpawnAction = TEXT("spawn_reactor_node_zero");
	const TCHAR* NodeZeroSpawnSpec = TEXT("reactor_node_zero_v1");
	const TCHAR* NodeZeroActorLabel = TEXT("BP_NodeZeroCore");
	const TCHAR* NodeZeroTerminalLabel = TEXT("Terminal_SterlingFinal");
	const FVector NodeZeroActorLocation(-200.f, 0.f, -4710.f);
	const FVector NodeZeroTerminalLocation(-200.f, 400.f, -4710.f);

	UObject* FindNodeZeroMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NodeZeroMissionObjectPath))
		{
			return Found;
		}
		if (FPackageName::DoesPackageExist(NodeZeroMissionPackage))
		{
			return StaticLoadObject(UObject::StaticClass(), nullptr, NodeZeroMissionObjectPath);
		}
		return nullptr;
	}

	FString NodeZeroSideTaskRemainder(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Side")))
		{
			return TEXT("Type is not Side.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("State"), TEXT("Inactive")))
		{
			return TEXT("State is not Inactive (initially incomplete).");
		}
		if (FIntProperty* ProgressProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("CurrentProgress")))
		{
			if (ProgressProp->GetPropertyValue_InContainer(Objective) != 0)
			{
				return TEXT("CurrentProgress must be 0.");
			}
		}
		else
		{
			return TEXT("CurrentProgress missing.");
		}
		if (FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress")))
		{
			if (TargetProp->GetPropertyValue_InContainer(Objective) != 1)
			{
				return TEXT("TargetProgress must be 1.");
			}
		}
		else
		{
			return TEXT("TargetProgress missing.");
		}
		if (FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds")))
		{
			FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
			if (PrereqHelper.Num() != 0)
			{
				return TEXT("PrerequisiteObjectiveIds count must be 0.");
			}
		}
		else
		{
			return TEXT("PrerequisiteObjectiveIds missing.");
		}
		if (FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate")))
		{
			if (!AutoProp->GetPropertyValue_InContainer(TaskElem))
			{
				return TEXT("bAutoActivate mismatch.");
			}
		}
		else
		{
			return TEXT("bAutoActivate missing.");
		}
		if (FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers")))
		{
			FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
			if (TriggerHelper.Num() != 1)
			{
				return TEXT("EventTriggers count must be 1.");
			}
			FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
			void* TrigElem = TriggerHelper.GetRawPtr(0);
			if (!TrigStruct || !TrigStruct->Struct || !TrigElem)
			{
				return TEXT("EventTriggers element struct missing.");
			}
			const FString LiveEvent = NeuroGeneticsMissionReadName(TrigElem, TrigStruct->Struct, TEXT("EventId"));
			if (!LiveEvent.Equals(NodeZeroFateEventId, ESearchCase::CaseSensitive))
			{
				return FString::Printf(TEXT("EventTriggers[0].EventId '%s' mismatch"), *LiveEvent);
			}
			if (!NeuroGeneticsMissionReadEnumNameEquals(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Complete")))
			{
				return TEXT("EventTriggers[0].Action is not Complete.");
			}
		}
		else
		{
			return TEXT("EventTriggers property missing.");
		}
		return TEXT("");
	}

	FString NodeZeroMissionMismatchReason(UObject* Asset)
	{
		if (!Asset || !Asset->GetClass() || !Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath))
		{
			return TEXT("Class is not ProjectOrganoidObjectiveDataAsset.");
		}
		FString IdError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionId")), MakeShared<FJsonValueString>(NodeZeroMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FString TitleError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionTitle")), MakeShared<FJsonValueString>(NodeZeroMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FString DescError;
		if (!PropertyMatchesJson(Asset, FindInstanceProperty(Asset, TEXT("MissionDescription")), MakeShared<FJsonValueString>(NodeZeroMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Asset);
		if (!LiveNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("NextMissionAsset must be DA_Mission_TheConclusion.");
		}
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp ? ArrayProp->Inner : nullptr);
		if (!ArrayProp || !TaskStruct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (Helper.Num() != 2)
		{
			return TEXT("Tasks count must be 2.");
		}
		if (const FString TaskError = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0), TaskStruct, NodeZeroReachObjectiveId, NodeZeroReachObjectiveTitle,
				NodeZeroReachObjectiveDescription, true, 0, nullptr, 1, NodeZeroReachEventId);
			!TaskError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *TaskError);
		}
		const FString SideProbe = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
			Helper.GetRawPtr(1), TaskStruct, NodeZeroFateObjectiveId, NodeZeroFateObjectiveTitle,
			NodeZeroFateObjectiveDescription, true, 0, nullptr, 1, NodeZeroFateEventId);
		if (!SideProbe.Equals(TEXT("Type is not Main."), ESearchCase::CaseSensitive))
		{
			return SideProbe.IsEmpty() ? TEXT("Tasks[1] type must be Side.") : FString::Printf(TEXT("Tasks[1]: %s"), *SideProbe);
		}
		if (const FString SideError = NodeZeroSideTaskRemainder(Helper.GetRawPtr(1), TaskStruct); !SideError.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1]: %s"), *SideError);
		}
		return TEXT("");
	}

	FString NodeZeroPatchSideType(void* TaskElem, FStructProperty* TaskStruct)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during side patch.");
		}
		FString EnumError;
		if (!NeuroGeneticsMissionWriteEnumByName(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Side"), EnumError))
		{
			return EnumError;
		}
		return TEXT("");
	}

	FString ApplyNodeZeroMissionDefaults(UObject* Asset)
	{
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NodeZeroMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NodeZeroMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), NodeZeroMissionDescription); !Error.IsEmpty())
		{
			return Error;
		}
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Asset, TEXT("NextMissionAsset")));
		if (!SoftProp)
		{
			return TEXT("NextMissionAsset soft property missing.");
		}
		SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr(FSoftObjectPath(TheConclusionMissionObjectPath)));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(FindInstanceProperty(Asset, TEXT("Tasks")));
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp ? ArrayProp->Inner : nullptr);
		if (!ArrayProp || !TaskStruct)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(2);
		TArray<FName> NoPrereqs;
		TArray<FName> ReachEvents;
		ReachEvents.Add(FName(NodeZeroReachEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0), TaskStruct, NodeZeroReachObjectiveId, NodeZeroReachObjectiveTitle,
				NodeZeroReachObjectiveDescription, true, NoPrereqs, ReachEvents);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		TArray<FName> FateEvents;
		FateEvents.Add(FName(NodeZeroFateEventId));
		if (const FString TaskError = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(1), TaskStruct, NodeZeroFateObjectiveId, NodeZeroFateObjectiveTitle,
				NodeZeroFateObjectiveDescription, true, NoPrereqs, FateEvents);
			!TaskError.IsEmpty())
		{
			return TaskError;
		}
		return NodeZeroPatchSideType(Helper.GetRawPtr(1), TaskStruct);
	}

	FString NodeZeroRequireConclusionUnchanged()
	{
		const FString ConclusionNext = NeuroAdaptationConnectionNextRevelationReadNext(FindTheConclusionMissionAssetExact());
		if (!ConclusionNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_TheConclusion.NextMissionAsset must stay DA_Mission_ResearchStation.");
		}
		return TEXT("");
	}

	FString PreflightCreateNodeZeroMission(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_node_zero_mission does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NodeZeroMissionSpec);
		if (!Spec.Equals(NodeZeroMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NodeZeroMissionSpec);
		}
		if (const FString Conclusion = NodeZeroRequireConclusionUnchanged(); !Conclusion.IsEmpty())
		{
			return Conclusion;
		}
		UObject* FirstCombat = FindFirstCombatMissionAssetExact();
		if (!FirstCombat)
		{
			return TEXT("DA_Mission_FirstCombat missing.");
		}
		const FString FirstNext = NeuroAdaptationConnectionNextRevelationReadNext(FirstCombat);
		if (FirstNext.IsEmpty())
		{
			if (const FString Mismatch = FirstCombatMissionMismatchReason(FirstCombat); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_FirstCombat mismatch: %s"), *Mismatch);
			}
		}
		else if (!FirstNext.Equals(NodeZeroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_FirstCombat Next must be null or already DA_Mission_NodeZero.");
		}
		UObject* Existing = FindNodeZeroMissionAssetExact();
		const bool bAlreadyExact = Existing && NodeZeroMissionMismatchReason(Existing).IsEmpty();
		if (Existing && !bAlreadyExact)
		{
			return TEXT("DA_Mission_NodeZero exists but does not match the locked spec.");
		}
		Before->SetBoolField(TEXT("conclusion_next_research_station"), true);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), NodeZeroMissionAction);
		Proposed->SetStringField(TEXT("spec"), NodeZeroMissionSpec);
		Proposed->SetStringField(TEXT("package"), NodeZeroMissionPackage);
		Proposed->SetStringField(TEXT("mission_id"), NodeZeroMissionId);
		Proposed->SetStringField(TEXT("title"), NodeZeroMissionTitle);
		Proposed->SetStringField(TEXT("description"), NodeZeroMissionDescription);
		Proposed->SetStringField(TEXT("next_mission_asset"), TheConclusionMissionObjectPath);
		Proposed->SetStringField(TEXT("main_objective_id"), NodeZeroReachObjectiveId);
		Proposed->SetStringField(TEXT("side_objective_id"), NodeZeroFateObjectiveId);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNodeZeroMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_node_zero_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNodeZeroMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Existing = FindNodeZeroMissionAssetExact();
		if (Existing && NodeZeroMissionMismatchReason(Existing).IsEmpty())
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateNodeZeroMission", "Create Node Zero mission"));
			UPackage* Package = CreatePackage(NodeZeroMissionPackage);
			Created = Package ? NewObject<UObject>(Package, MissionClass, NodeZeroMissionAssetName, RF_Public | RF_Standalone | RF_Transactional) : nullptr;
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("NewObject failed for DA_Mission_NodeZero."), MakeShared<FBridgeChange>(Change));
			}
			if (const FString ApplyError = ApplyNodeZeroMissionDefaults(Created); !ApplyError.IsEmpty())
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
			if (PackagesEqual(Dirty, NodeZeroMissionPackage))
			{
				continue;
			}
			if (!DirtyBefore.ContainsByPredicate([&](const FString& Prior) { return PackagesEqual(Prior, Dirty); }))
			{
				UnexpectedNew.Add(Dirty);
			}
		}
		if (UnexpectedNew.Num() != 0 || !Created || !NodeZeroMissionMismatchReason(Created).IsEmpty() || !NodeZeroRequireConclusionUnchanged().IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Node Zero mission did not verify, or The Conclusion changed."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), NodeZeroMissionPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSetFirstCombatNextNodeZero(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_first_combat_next_node_zero does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NodeZeroNextSpec);
		if (!Spec.Equals(NodeZeroNextSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NodeZeroNextSpec);
		}
		UObject* NodeZero = FindNodeZeroMissionAssetExact();
		if (!NodeZero || !NodeZeroMissionMismatchReason(NodeZero).IsEmpty())
		{
			return TEXT("DA_Mission_NodeZero missing or not exact.");
		}
		if (const FString Conclusion = NodeZeroRequireConclusionUnchanged(); !Conclusion.IsEmpty())
		{
			return Conclusion;
		}
		UObject* FirstCombat = FindFirstCombatMissionAssetExact();
		if (!FirstCombat)
		{
			return TEXT("DA_Mission_FirstCombat missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(FirstCombat);
		const bool bAlreadyExact = LiveNext.Equals(NodeZeroMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString Mismatch = FirstCombatMissionMismatchReason(FirstCombat); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_FirstCombat mismatch: %s"), *Mismatch);
			}
		}
		UObject* PursuerIntro = FindPursuerIntroMissionAssetExact();
		const FString PursuerNext = NeuroAdaptationConnectionNextRevelationReadNext(PursuerIntro);
		if (!PursuerNext.Equals(FirstCombatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_PursuerIntro.NextMissionAsset must stay DA_Mission_FirstCombat.");
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), NodeZeroNextAction);
		Proposed->SetStringField(TEXT("spec"), NodeZeroNextSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), NodeZeroMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetFirstCombatNextNodeZero(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_first_combat_next_node_zero must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetFirstCombatNextNodeZero(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* FirstCombat = FindFirstCombatMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(FirstCombat);
		if (LiveNext.Equals(NodeZeroMissionObjectPath, ESearchCase::CaseSensitive))
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "FirstCombatNextNodeZero", "Link First Combat to Node Zero"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(FirstCombat, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(FirstCombat, FSoftObjectPtr(FSoftObjectPath(NodeZeroMissionObjectPath)));
			FirstCombat->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(FirstCombat);
		if (!AfterNext.Equals(NodeZeroMissionObjectPath, ESearchCase::CaseSensitive) || !NodeZeroRequireConclusionUnchanged().IsEmpty() || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("First Combat Next did not become DA_Mission_NodeZero, The Conclusion changed, or power changed."), MakeShared<FBridgeChange>(Change));
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

	UBlueprint* FindNodeZeroBlueprintExact()
	{
		return Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, NodeZeroBlueprintObjectPath));
	}

	FString PreflightCreateNodeZeroBlueprint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_node_zero_blueprint does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NodeZeroBlueprintSpec);
		if (!Spec.Equals(NodeZeroBlueprintSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NodeZeroBlueprintSpec);
		}
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, NodeZeroNativeClassPath);
		if (!Native)
		{
			return TEXT("AProjectOrganoidNodeZeroCore is not loaded. Rebuild the editor target first.");
		}
		UBlueprint* Existing = FindNodeZeroBlueprintExact();
		const bool bAlreadyExact = Existing && Existing->ParentClass == Native && Existing->GeneratedClass != nullptr;
		if (Existing && !bAlreadyExact)
		{
			return TEXT("BP_NodeZeroCore exists but is not a child of AProjectOrganoidNodeZeroCore.");
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), NodeZeroBlueprintAction);
		Proposed->SetStringField(TEXT("spec"), NodeZeroBlueprintSpec);
		Proposed->SetStringField(TEXT("package"), NodeZeroBlueprintPackage);
		Proposed->SetStringField(TEXT("parent"), NodeZeroNativeClassPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNodeZeroBlueprint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_node_zero_blueprint must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNodeZeroBlueprint(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, NodeZeroNativeClassPath);
		UBlueprint* Existing = FindNodeZeroBlueprintExact();
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
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateNodeZeroBlueprint", "Create BP_NodeZeroCore"));
			UPackage* Package = CreatePackage(NodeZeroBlueprintPackage);
			Created = FKismetEditorUtilities::CreateBlueprint(
				Native, Package, TEXT("BP_NodeZeroCore"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), TEXT("CreateBlueprint failed for BP_NodeZeroCore."), MakeShared<FBridgeChange>(Change));
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}
		if (!Created->GeneratedClass || Created->ParentClass != Native)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("BP_NodeZeroCore did not compile as a Node Zero child."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetStringField(TEXT("package"), NodeZeroBlueprintPackage);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString NodeZeroGuardPlacedActors(UWorld* World)
	{
		if (const FString Guard = PursuerGuardUnmoved(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		const TArray<AActor*> Pursuers = FindByExactLabel(World, PursuerActorLabel);
		const TArray<AActor*> PursuerTriggers = FindByExactLabel(World, PursuerTriggerLabel);
		const TArray<AActor*> Scientists = FindByExactLabel(World, FirstCombatActorLabel);
		const TArray<AActor*> CombatTriggers = FindByExactLabel(World, FirstCombatTriggerLabel);
		if (Pursuers.Num() != 1 || !Pursuers[0]->GetActorLocation().Equals(PursuerActorLocation, 1.f)
			|| PursuerTriggers.Num() != 1 || !PursuerTriggers[0]->GetActorLocation().Equals(PursuerTriggerLocation, 1.f))
		{
			return TEXT("BP_Pursuer and Reactor_PursuerTrigger must stay at their Beat 22 locations.");
		}
		if (Scientists.Num() != 1 || !Scientists[0]->GetActorLocation().Equals(FirstCombatActorLocation, 1.f)
			|| CombatTriggers.Num() != 1 || !CombatTriggers[0]->GetActorLocation().Equals(FirstCombatTriggerLocation, 1.f))
		{
			return TEXT("BP_TransformedScientist and Reactor_FirstCombatTrigger must stay at their Beat 23 locations.");
		}
		return TEXT("");
	}

	bool NodeZeroPlacementExact(UWorld* World, UClass* SpawnClass, UClass* TerminalClass)
	{
		const TArray<AActor*> Cores = FindByExactLabel(World, NodeZeroActorLabel);
		const TArray<AActor*> Terminals = FindByExactLabel(World, NodeZeroTerminalLabel);
		return Cores.Num() == 1 && Terminals.Num() == 1
			&& Cores[0]->GetClass() == SpawnClass
			&& Terminals[0]->GetClass() == TerminalClass
			&& Cores[0]->GetActorLocation().Equals(NodeZeroActorLocation, 1.f)
			&& Terminals[0]->GetActorLocation().Equals(NodeZeroTerminalLocation, 1.f)
			&& PackagesEqual(ActorOwningPackage(Cores[0]), ReactorPackage)
			&& PackagesEqual(ActorOwningPackage(Terminals[0]), ReactorPackage);
	}

	FString PreflightSpawnReactorNodeZero(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. spawn_reactor_node_zero does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NodeZeroSpawnSpec);
		if (!Spec.Equals(NodeZeroSpawnSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NodeZeroSpawnSpec);
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
		if (const FString Guard = NodeZeroGuardPlacedActors(World); !Guard.IsEmpty())
		{
			return Guard;
		}
		UClass* SpawnClass = LoadClass<AActor>(nullptr, NodeZeroBlueprintClassPath);
		UClass* TerminalClass = StaticLoadClass(AActor::StaticClass(), nullptr, NodeZeroTerminalClassPath);
		if (!SpawnClass || !TerminalClass)
		{
			return TEXT("BP_NodeZeroCore or AProjectOrganoidTerminal is not loaded.");
		}
		const TArray<AActor*> Cores = FindByExactLabel(World, NodeZeroActorLabel);
		const TArray<AActor*> Terminals = FindByExactLabel(World, NodeZeroTerminalLabel);
		const bool bAlreadyExact = NodeZeroPlacementExact(World, SpawnClass, TerminalClass);
		if (!bAlreadyExact)
		{
			if (Cores.Num() != 0 || Terminals.Num() != 0)
			{
				return TEXT("Node Zero actors exist but do not match the locked placement.");
			}
			if (!PursuerSpotOpen(World, NodeZeroActorLocation, NodeZeroActorLabel) || !PursuerSpotOpen(World, NodeZeroTerminalLocation, NodeZeroTerminalLabel))
			{
				return TEXT("Reactor basin near (-200, 0, -4710) is occupied. ZERO writes. Existing actors stay put.");
			}
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("core_location"), TEXT("-200,0,-4710"));
		Before->SetStringField(TEXT("terminal_location"), TEXT("-200,400,-4710"));
		Proposed->SetStringField(TEXT("action"), NodeZeroSpawnAction);
		Proposed->SetStringField(TEXT("spec"), NodeZeroSpawnSpec);
		Proposed->SetStringField(TEXT("package"), ReactorPackage);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("moves_existing_actors"), false);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnReactorNodeZero(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_reactor_node_zero must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnReactorNodeZero(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		UClass* SpawnClass = LoadClass<AActor>(nullptr, NodeZeroBlueprintClassPath);
		UClass* TerminalClass = StaticLoadClass(AActor::StaticClass(), nullptr, NodeZeroTerminalClassPath);
		if (NodeZeroPlacementExact(World, SpawnClass, TerminalClass))
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
		AActor* Core = nullptr;
		AActor* Terminal = nullptr;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnNodeZero", "Place Node Zero in the Reactor basin"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = ReactorLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Core = World->SpawnActor<AActor>(SpawnClass, NodeZeroActorLocation, FRotator::ZeroRotator, Params);
			Terminal = World->SpawnActor<AActor>(TerminalClass, NodeZeroTerminalLocation, FRotator::ZeroRotator, Params);
			if (Core)
			{
				Core->SetActorLabel(NodeZeroActorLabel, true);
			}
			if (Terminal)
			{
				Terminal->SetActorLabel(NodeZeroTerminalLabel, true);
			}
		}
		auto DestroySpawned = [&]()
		{
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				if (Core) ActorSub->DestroyActor(Core);
				if (Terminal) ActorSub->DestroyActor(Terminal);
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
		const bool bPlaced = NodeZeroPlacementExact(World, SpawnClass, TerminalClass);
		if (!bPlaced || !bReactorDirty || UnexpectedNew.Num() != 0 || !NodeZeroGuardPlacedActors(World).IsEmpty() || !CryoAccessRequirePowerContract(World).IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Node Zero placement failed, an unexpected package dirtied, or an existing actor moved."), MakeShared<FBridgeChange>(Change));
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
