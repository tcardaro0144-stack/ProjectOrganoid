	// Fixed Neuro Research Station intro mission — create_neuro_research_station_intro_mission.
	const TCHAR* NeuroResearchStationIntroMissionSpec = TEXT("neuro_research_station_intro_mission_v1");
	const TCHAR* NeuroResearchStationIntroMissionAction = TEXT("create_neuro_research_station_intro_mission");
	const TCHAR* NeuroResearchStationIntroMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro");
	const TCHAR* NeuroResearchStationIntroMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro.DA_Mission_NeuroResearchStationIntro");
	const TCHAR* NeuroResearchStationIntroMissionAssetName = TEXT("DA_Mission_NeuroResearchStationIntro");
	const TCHAR* NeuroResearchStationIntroMissionId = TEXT("Mission_NeuroResearchStationIntro");
	const TCHAR* NeuroResearchStationIntroMissionTitle = TEXT("Use the Research Station");
	const TCHAR* NeuroResearchStationIntroMissionDescription =
		TEXT("Mount the neural adaptation at the NeuroGenetics Research Station.");
	const TCHAR* NeuroResearchStationIntroObjectiveId = TEXT("Obj_EquipNeuralSlow");
	const TCHAR* NeuroResearchStationIntroObjectiveTitle = TEXT("Equip Neural Slow");
	const TCHAR* NeuroResearchStationIntroObjectiveDescription =
		TEXT("Open the Research Station and equip Neural Slow.");
	const TCHAR* NeuroResearchStationIntroEventId = TEXT("Event_NeuralSlowEquipped");

	UObject* FindNeuroResearchStationIntroMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NeuroResearchStationIntroMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, NeuroResearchStationIntroMissionObjectPath);
	}

	TSharedRef<FJsonObject> NeuroResearchStationIntroMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), NeuroResearchStationIntroObjectiveId);
		Task->SetStringField(TEXT("title"), NeuroResearchStationIntroObjectiveTitle);
		Task->SetStringField(TEXT("description"), NeuroResearchStationIntroObjectiveDescription);
		Task->SetStringField(TEXT("category"), TEXT("Main"));
		Task->SetNumberField(TEXT("target_count"), 1);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("complete_event"), NeuroResearchStationIntroEventId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroResearchStationIntroMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroResearchStationIntroMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroResearchStationIntroMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString NeuroResearchStationIntroMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroResearchStationIntroMissionSpec);
		if (!Spec.Equals(NeuroResearchStationIntroMissionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroResearchStationIntroMissionSpec);
		}
		return TEXT("");
	}

	FString NeuroResearchStationIntroMissionMismatchReason(UObject* Asset)
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
		if (!PropertyMatchesJson(Asset, IdProp, MakeShared<FJsonValueString>(NeuroResearchStationIntroMissionId), IdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *IdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroResearchStationIntroMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(
				Asset, DescProp, MakeShared<FJsonValueString>(NeuroResearchStationIntroMissionDescription), DescError))
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
				NeuroResearchStationIntroObjectiveId,
				NeuroResearchStationIntroObjectiveTitle,
				NeuroResearchStationIntroObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroResearchStationIntroEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroResearchStationIntroMissionDefaults(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroResearchStationIntroMissionId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroResearchStationIntroMissionTitle);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Asset, TEXT("MissionDescription"), NeuroResearchStationIntroMissionDescription);
			!Error.IsEmpty())
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
		Events.Add(FName(NeuroResearchStationIntroEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroResearchStationIntroObjectiveId,
				NeuroResearchStationIntroObjectiveTitle,
				NeuroResearchStationIntroObjectiveDescription,
				true,
				NoPrereqs,
				Events);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}
		return TEXT("");
	}

	FString CleanupCreatedNeuroResearchStationIntroMissionAsset(
		UObject* Asset, bool bPackageWasDirtyBefore, TArray<FString>& OutRestored)
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

	FString CreateNeuroResearchStationIntroMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;

		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return FString::Printf(TEXT("Failed to load mission class '%s'."), NeuroGeneticsMissionClassPath);
		}

		UPackage* ExistingPackage = FindPackage(nullptr, NeuroResearchStationIntroMissionPackage);
		bOutPackageWasDirtyBefore = ExistingPackage && ExistingPackage->IsDirty();

		UPackage* Package = CreatePackage(NeuroResearchStationIntroMissionPackage);
		if (!Package)
		{
			return FString::Printf(TEXT("Failed to create package '%s'."), NeuroResearchStationIntroMissionPackage);
		}

		UObject* Asset = NewObject<UObject>(
			Package,
			MissionClass,
			NeuroResearchStationIntroMissionAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject failed for DA_Mission_NeuroResearchStationIntro.");
		}

		if (const FString ApplyError = ApplyNeuroResearchStationIntroMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroResearchStationIntroMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		OutAsset = Asset;
		bOutCreatedNow = true;
		return TEXT("");
	}

	FString GuardNeuroResearchStationIntroMissionEditorContext()
	{
		if (!IsInGameThread())
		{
			return TEXT("create_neuro_research_station_intro_mission must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before creating the mission asset.");
		}
		return TEXT("");
	}

	FString PreflightCreateNeuroResearchStationIntroMission(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroResearchStationIntroMissionEditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_neuro_research_station_intro_mission does not save.");
		}
		if (const FString OverrideError = NeuroResearchStationIntroMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Existing = FindNeuroResearchStationIntroMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			const FString Mismatch = NeuroResearchStationIntroMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Mission_NeuroResearchStationIntro exists but mismatches locked contract: %s. Fail closed — no opportunistic repair."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return FString::Printf(TEXT("Mission class '%s' unresolved."), NeuroGeneticsMissionClassPath);
		}

		Before->SetStringField(TEXT("spec"), NeuroResearchStationIntroMissionSpec);
		Before->SetStringField(TEXT("action"), NeuroResearchStationIntroMissionAction);
		Before->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroResearchStationIntroMissionPackage);
		Before->SetBoolField(TEXT("exists"), Existing != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroResearchStationIntroMissionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroResearchStationIntroMissionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroResearchStationIntroMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroResearchStationIntroMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission"), NeuroResearchStationIntroMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroResearchStationIntroMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("create_neuro_research_station_intro_mission must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroResearchStationIntroMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroResearchStationIntroMissionAssetExact();
		if (Existing && NeuroResearchStationIntroMissionMismatchReason(Existing).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroResearchStationIntroMissionProposedState(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroResearchStationIntro exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyBefore = CollectDirtyPackageNamesSorted();
		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"CreateNeuroResearchStationIntroMission",
				"Create Neuro Research Station Intro Mission DataAsset"));
			if (const FString CreateError =
					CreateNeuroResearchStationIntroMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore);
				!CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				return FailAudit(
					TEXT("create_failed"),
					FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString VerifyError = NeuroResearchStationIntroMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroResearchStationIntroMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Created object removed. ZERO remaining writes."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroResearchStationIntroMissionPackage);
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
			CleanupCreatedNeuroResearchStationIntroMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
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
		Change.After->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Created));
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetObjectField(TEXT("mission"), NeuroResearchStationIntroMissionProposedState(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
