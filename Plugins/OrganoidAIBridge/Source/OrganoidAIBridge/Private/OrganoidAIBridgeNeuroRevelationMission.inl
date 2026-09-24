// Fixed Neuro revelation mission — create_neuro_revelation_mission / neuro_revelation_mission_v1.
// Preview is mutation-free. Apply creates only the exact asset; never saves; never repairs mismatches.
	const TCHAR* NeuroRevelationMissionSpec = TEXT("neuro_revelation_mission_v1");
	const TCHAR* NeuroRevelationMissionAction = TEXT("create_neuro_revelation_mission");
	const TCHAR* NeuroRevelationMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation");
	const TCHAR* NeuroRevelationMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation.DA_Mission_NeuroRevelation");
	const TCHAR* NeuroRevelationMissionAssetName = TEXT("DA_Mission_NeuroRevelation");
	const TCHAR* NeuroRevelationMissionId = TEXT("Mission_NeuroRevelation");
	const TCHAR* NeuroRevelationMissionTitle = TEXT("Read the Neural Pattern");
	const TCHAR* NeuroRevelationMissionDescription =
		TEXT("The mapping and signature data now show a systematic reorganization of nervous systems tied to the research conducted in this wing.");
	const TCHAR* NeuroRevelationObjectiveId = TEXT("Obj_ReachNeuroRevelation");
	const TCHAR* NeuroRevelationObjectiveTitle = TEXT("Reach Neuro Revelation");
	const TCHAR* NeuroRevelationObjectiveDescription =
		TEXT("Review the neural signature observation node to confirm the pattern.");
	const TCHAR* NeuroRevelationEventId = TEXT("Event_NeuroRevelationReached");

	const FVector NeuroRevelationNodeLocation(500.f, -2100.f, -1100.f);
	const FVector NeuroRevelationTerminalLocation(300.f, -600.f, -1100.f);
	const FVector NeuroRevelationEvidenceLocation(500.f, -2520.f, -1100.f);

	TSharedRef<FJsonObject> NeuroRevelationMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), NeuroRevelationObjectiveId);
		Task->SetStringField(TEXT("title"), NeuroRevelationObjectiveTitle);
		Task->SetStringField(TEXT("description"), NeuroRevelationObjectiveDescription);
		Task->SetStringField(TEXT("category"), TEXT("Main"));
		Task->SetNumberField(TEXT("target_count"), 1);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("complete_event"), NeuroRevelationEventId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroRevelationMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroRevelationMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroRevelationMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString NeuroRevelationMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroRevelationMissionSpec);
		if (!Spec.Equals(NeuroRevelationMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroRevelationMissionSpec);
		}
		return TEXT("");
	}

	FString NeuroRevelationRequireCleanLoadedPackage(const TCHAR* PackageName)
	{
		UPackage* Package = FindPackage(nullptr, PackageName);
		if (!Package)
		{
			return FString::Printf(TEXT("%s must be loaded. ZERO writes."), PackageName);
		}
		if (Package->IsDirty())
		{
			return FString::Printf(TEXT("%s is dirty. ZERO writes."), PackageName);
		}
		return TEXT("");
	}

	FString NeuroRevelationRequireKeepActor(UWorld* World, const TCHAR* Label, const FVector& Expected)
	{
		if (!World)
		{
			return TEXT("Editor world missing.");
		}
		const TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1 || !Matches[0])
		{
			return FString::Printf(TEXT("Keep-list actor '%s' must be unique. count=%d. ZERO writes."), Label, Matches.Num());
		}
		if (!LocationMatches(Matches[0]->GetActorLocation(), Expected))
		{
			const FVector Loc = Matches[0]->GetActorLocation();
			return FString::Printf(
				TEXT("Keep-list actor '%s' location (%.2f, %.2f, %.2f) moved. ZERO writes."),
				Label, Loc.X, Loc.Y, Loc.Z);
		}
		return TEXT("");
	}

	FString NeuroRevelationRequireEditorStable()
	{
		if (!IsInGameThread())
		{
			return TEXT("Neuro revelation writes must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before a Neuro revelation write.");
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
		if (const FString Node = NeuroRevelationRequireKeepActor(
				World, TEXT("NeuralSignatureObservationNode_NeuroGenetics"), NeuroRevelationNodeLocation);
			!Node.IsEmpty())
		{
			return Node;
		}
		if (const FString Terminal = NeuroRevelationRequireKeepActor(
				World, TEXT("NeuralMappingTerminal_NeuroGenetics"), NeuroRevelationTerminalLocation);
			!Terminal.IsEmpty())
		{
			return Terminal;
		}
		if (const FString Evidence = NeuroRevelationRequireKeepActor(
				World, TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics"), NeuroRevelationEvidenceLocation);
			!Evidence.IsEmpty())
		{
			return Evidence;
		}

		// Campaign contract is Neuro Online / Cryo Blackout. The editor subsystem seed is Emergency
		// until the backup panel runs in play. This write must not change either sector.
		const FString NeuroPower = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2);
		const FString CryoPower = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3);
		if (!CryoPower.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Cryo power must be Blackout (live '%s'). ZERO writes."), *CryoPower);
		}
		if (NeuroPower.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase) || NeuroPower.IsEmpty())
		{
			return FString::Printf(TEXT("NeuroGenetics power must stay Online or the editor Emergency seed (live '%s'). ZERO writes."), *NeuroPower);
		}
		return TEXT("");
	}

	UObject* FindNeuroRevelationMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NeuroRevelationMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, NeuroRevelationMissionObjectPath);
	}

	FString NeuroRevelationMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(NeuroRevelationMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroRevelationMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(Asset, DescProp, MakeShared<FJsonValueString>(NeuroRevelationMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}

		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			if (Soft.ToSoftObjectPath().IsValid())
			{
				return FString::Printf(
					TEXT("NextMissionAsset must be null, got '%s'"),
					*Soft.ToSoftObjectPath().ToString());
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
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroRevelationObjectiveId,
				NeuroRevelationObjectiveTitle,
				NeuroRevelationObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroRevelationEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroRevelationMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroRevelationMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroRevelationMissionTitle); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), NeuroRevelationMissionDescription); !Error.IsEmpty())
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
		Events.Add(FName(NeuroRevelationEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroRevelationObjectiveId,
				NeuroRevelationObjectiveTitle,
				NeuroRevelationObjectiveDescription,
				true,
				NoPrereqs,
				Events);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString CleanupCreatedNeuroRevelationMissionAsset(UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateNeuroRevelationMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;
		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}
		UPackage* ExistingPackage = FindPackage(nullptr, NeuroRevelationMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();
		UPackage* Package = CreatePackage(NeuroRevelationMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), NeuroRevelationMissionPackage);
		}
		UObject* Asset = NewObject<UObject>(
			Package,
			MissionClass,
			NeuroRevelationMissionAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_NeuroRevelation.");
		}
		if (const FString ApplyError = ApplyNeuroRevelationMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroRevelationMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}
		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString PreflightCreateNeuroRevelationMission(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = NeuroRevelationRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_neuro_revelation_mission does not save.");
		}
		if (const FString OverrideError = NeuroRevelationMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Existing = FindNeuroRevelationMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = NeuroRevelationMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Mission_NeuroRevelation exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return FString::Printf(TEXT("Mission class '%s' unresolved."), NeuroGeneticsMissionClassPath);
		}

		Before->SetStringField(TEXT("spec"), NeuroRevelationMissionSpec);
		Before->SetStringField(TEXT("action"), NeuroRevelationMissionAction);
		Before->SetStringField(TEXT("object_path"), NeuroRevelationMissionObjectPath);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), NeuroRevelationMissionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroRevelationMissionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroRevelationMissionObjectPath);
		Proposed->SetObjectField(TEXT("mission"), NeuroRevelationMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroRevelationMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_neuro_revelation_mission must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroRevelationMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroRevelationMissionAssetExact();
		if (Existing && NeuroRevelationMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroRevelationMissionObjectPath);
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("mismatch"), TEXT("DA_Mission_NeuroRevelation exists but is not exact. Fail closed. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"CreateNeuroRevelationMission",
				"Create Neuro Revelation Mission DataAsset"));
			if (const FString CreateError = CreateNeuroRevelationMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore); !CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError), MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString VerifyError = NeuroRevelationMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroRevelationMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError), MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroRevelationMissionPackage);
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
			CleanupCreatedNeuroRevelationMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("dirty_package_contract"),
				FString::Printf(
					TEXT("After create, dirty must include [%s] with no new unexpected dirties. got [%s] unexpected_new [%s]."),
					*ExpectedDirty,
					*FString::Join(DirtyAfter, TEXT(",")),
					*FString::Join(UnexpectedNew, TEXT(","))),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetStringField(TEXT("object_path"), NeuroRevelationMissionObjectPath);
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
