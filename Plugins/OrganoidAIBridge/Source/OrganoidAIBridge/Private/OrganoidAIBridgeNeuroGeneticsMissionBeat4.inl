	// Fixed NeuroGenetics mission Beat 4 expansion —
	// expand_neurogenetics_mission_beat4 / neurogenetics_mission_beat4_v1.
	// Transforms exact Beat 3 (neurogenetics_mission_beat3_v1) → exact Beat 4.
	// Preview is mutation-free. Apply modifies only the exact DataAsset; never saves; never repairs mismatches.
	// Reuses Beat 3 helpers already defined above; does not modify Beat 1–3 implementations.
	const TCHAR* NeuroGeneticsMissionBeat4Spec = TEXT("neurogenetics_mission_beat4_v1");
	const TCHAR* NeuroGeneticsMissionBeat4Action = TEXT("expand_neurogenetics_mission_beat4");
	const TCHAR* NeuroGeneticsTraceEventId = TEXT("Event_NeuralMappingSignalTraced");
	const TCHAR* NeuroGeneticsFollowObjectiveId = TEXT("Obj_FollowNeuralSignature");
	const TCHAR* NeuroGeneticsFollowObjectiveTitle = TEXT("Follow the neural signature");
	const TCHAR* NeuroGeneticsFollowObjectiveDescription =
		TEXT("Track the matching neural pattern deeper into the research wing.");

	TSharedRef<FJsonObject> NeuroGeneticsMissionBeat4ProposedTasks(bool bAlreadyExact)
	{
		TArray<FString> IsolateEvents;
		IsolateEvents.Add(NeuroGeneticsIsolateEventId);
		TArray<FString> TracePrereqs;
		TracePrereqs.Add(NeuroGeneticsObjectiveId);
		TArray<FString> TraceEvents;
		TraceEvents.Add(NeuroGeneticsTraceEventId);
		TArray<FString> FollowPrereqs;
		FollowPrereqs.Add(NeuroGeneticsTraceObjectiveId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsObjectiveId,
			NeuroGeneticsObjectiveTitle,
			NeuroGeneticsObjectiveDescription,
			true,
			TArray<FString>(),
			IsolateEvents)));
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsTraceObjectiveId,
			NeuroGeneticsTraceObjectiveTitle,
			NeuroGeneticsTraceObjectiveDescription,
			true,
			TracePrereqs,
			TraceEvents)));
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsFollowObjectiveId,
			NeuroGeneticsFollowObjectiveTitle,
			NeuroGeneticsFollowObjectiveDescription,
			true,
			FollowPrereqs,
			TArray<FString>())));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroGeneticsMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroGeneticsMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroGeneticsMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetNumberField(TEXT("task_count"), 3);
		Mission->SetStringField(TEXT("shape"), TEXT("beat4"));
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Mission->SetStringField(
			TEXT("runtime_task2"),
			TEXT("Locked/Inactive until Obj_IsolateNeuroResearchLoad completes, then Active via auto-unlock"));
		Mission->SetStringField(
			TEXT("runtime_task3"),
			TEXT("Locked/Inactive until Obj_TraceNeuralMappingSignal completes, then Active via auto-unlock"));
		return Mission;
	}

	FString NeuroGeneticsMissionBeat4RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("class"),
			TEXT("class_path"),
			TEXT("path"),
			TEXT("asset_path"),
			TEXT("object_path"),
			TEXT("package"),
			TEXT("mission_id"),
			TEXT("mission_title"),
			TEXT("mission_description"),
			TEXT("next_mission"),
			TEXT("next_mission_asset"),
			TEXT("tasks"),
			TEXT("objective_id"),
			TEXT("event_id"),
			TEXT("event_triggers"),
			TEXT("prerequisites"),
			TEXT("prerequisite_objective_ids"),
			TEXT("copy"),
			TEXT("copy_from"),
			TEXT("source_asset"),
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(
					TEXT("expand_neurogenetics_mission_beat4 rejects client-provided '%s'. Spec neurogenetics_mission_beat4_v1 is fixed."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroGeneticsMissionBeat4Spec);
		if (!Spec.Equals(NeuroGeneticsMissionBeat4Spec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neurogenetics_mission_beat4_v1.");
		}
		return TEXT("");
	}

	FString NeuroGeneticsMissionBeat4MismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!NeuroGeneticsMissionClassMatches(Asset))
		{
			return FString::Printf(TEXT("class '%s' is not UProjectOrganoidObjectiveDataAsset"), *ClassName(Asset));
		}
		if (!PackagesEqual(Asset->GetOutermost() ? Asset->GetOutermost()->GetName() : FString(), NeuroGeneticsMissionPackage))
		{
			return FString::Printf(
				TEXT("owning package '%s' is not %s"),
				Asset->GetOutermost() ? *Asset->GetOutermost()->GetName() : TEXT(""),
				NeuroGeneticsMissionPackage);
		}
		if (!Asset->GetName().Equals(NeuroGeneticsMissionAssetName, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("asset name '%s' is not DA_Mission_NeuroGenetics"), *Asset->GetName());
		}
		if (!Asset->GetPathName().Equals(NeuroGeneticsMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("object path '%s' is not exact"), *Asset->GetPathName());
		}

		FProperty* MissionIdProp = FindInstanceProperty(Asset, TEXT("MissionId"));
		FString MissionIdError;
		if (!PropertyMatchesJson(Asset, MissionIdProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionId), MissionIdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *MissionIdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(
				Asset, DescProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionDescription), DescError))
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
		if (Helper.Num() != 3)
		{
			return FString::Printf(TEXT("Tasks count=%d, expected exactly 3"), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}

		if (const FString Task0Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroGeneticsObjectiveId,
				NeuroGeneticsObjectiveTitle,
				NeuroGeneticsObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroGeneticsIsolateEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		if (const FString Task1Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(1),
				TaskStruct,
				NeuroGeneticsTraceObjectiveId,
				NeuroGeneticsTraceObjectiveTitle,
				NeuroGeneticsTraceObjectiveDescription,
				true,
				1,
				NeuroGeneticsObjectiveId,
				1,
				NeuroGeneticsTraceEventId);
			!Task1Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1]: %s"), *Task1Error);
		}
		if (const FString Task2Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(2),
				TaskStruct,
				NeuroGeneticsFollowObjectiveId,
				NeuroGeneticsFollowObjectiveTitle,
				NeuroGeneticsFollowObjectiveDescription,
				true,
				1,
				NeuroGeneticsTraceObjectiveId,
				0,
				nullptr);
			!Task2Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[2]: %s"), *Task2Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroGeneticsMissionBeat4(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroGeneticsMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroGeneticsMissionTitle);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), NeuroGeneticsMissionDescription);
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
		Helper.Resize(3);

		TArray<FName> NoPrereqs;
		TArray<FName> IsolateEvents;
		IsolateEvents.Add(FName(NeuroGeneticsIsolateEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroGeneticsObjectiveId,
				NeuroGeneticsObjectiveTitle,
				NeuroGeneticsObjectiveDescription,
				true,
				NoPrereqs,
				IsolateEvents);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}

		TArray<FName> TracePrereqs;
		TracePrereqs.Add(FName(NeuroGeneticsObjectiveId));
		TArray<FName> TraceEvents;
		TraceEvents.Add(FName(NeuroGeneticsTraceEventId));
		if (const FString Task1Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(1),
				TaskStruct,
				NeuroGeneticsTraceObjectiveId,
				NeuroGeneticsTraceObjectiveTitle,
				NeuroGeneticsTraceObjectiveDescription,
				true,
				TracePrereqs,
				TraceEvents);
			!Task1Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1] apply: %s"), *Task1Error);
		}

		TArray<FName> FollowPrereqs;
		FollowPrereqs.Add(FName(NeuroGeneticsTraceObjectiveId));
		TArray<FName> NoEvents;
		if (const FString Task2Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(2),
				TaskStruct,
				NeuroGeneticsFollowObjectiveId,
				NeuroGeneticsFollowObjectiveTitle,
				NeuroGeneticsFollowObjectiveDescription,
				true,
				FollowPrereqs,
				NoEvents);
			!Task2Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[2] apply: %s"), *Task2Error);
		}
		return TEXT("");
	}

	FString GuardNeuroGeneticsMissionBeat4EditorContext()
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. expand_neurogenetics_mission_beat4 is an Unreal Editor write.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		return TEXT("");
	}

	FString PreflightExpandNeuroGeneticsMissionBeat4(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroGeneticsMissionBeat4EditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. expand_neurogenetics_mission_beat4 does not save or compile.");
		}
		if (const FString OverrideError = NeuroGeneticsMissionBeat4RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (!IsInGameThread())
		{
			return TEXT("expand_neurogenetics_mission_beat4 preflight must run on the game thread.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Packages dirty: %s. Require clean packages before expand_neurogenetics_mission_beat4."),
				*FString::Join(Dirty, TEXT(",")));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (!Existing)
		{
			return TEXT("DA_Mission_NeuroGenetics is missing. Persist Beat 3 expand_neurogenetics_mission_beat3 + save first.");
		}
		if (!NeuroGeneticsMissionClassMatches(Existing))
		{
			return FString::Printf(
				TEXT("Exact path occupied by wrong class '%s'. Fail closed — no automatic repair."),
				*ClassName(Existing));
		}

		const FString Beat3Mismatch = NeuroGeneticsMissionBeat3MismatchReason(Existing);
		const FString Beat4Mismatch = NeuroGeneticsMissionBeat4MismatchReason(Existing);
		const bool bBeat3Exact = Beat3Mismatch.IsEmpty();
		const bool bBeat4Exact = Beat4Mismatch.IsEmpty();
		if (!bBeat3Exact && !bBeat4Exact)
		{
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is neither exact Beat 3 nor exact Beat 4. Beat3: %s | Beat4: %s. Fail closed — no opportunistic repair."),
				*Beat3Mismatch,
				*Beat4Mismatch);
		}

		Before->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat4Spec);
		Before->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat4Action);
		Before->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("packages_clean"), true);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("asset_present"), true);
		Before->SetStringField(TEXT("class"), ClassName(Existing));
		Before->SetBoolField(TEXT("beat3_exact"), bBeat3Exact);
		Before->SetBoolField(TEXT("beat4_exact"), bBeat4Exact);
		Before->SetObjectField(
			TEXT("mission_before"),
			bBeat4Exact ? NeuroGeneticsMissionBeat4ProposedTasks(true) : NeuroGeneticsMissionBeat3ProposedTasks(true));

		Proposed->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat4Spec);
		Proposed->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat4Action);
		Proposed->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroGeneticsMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat4ProposedTasks(bBeat4Exact));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bBeat4Exact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bBeat4Exact
				? TEXT("DA_Mission_NeuroGenetics already matches neurogenetics_mission_beat4_v1. Execute is a clean no-op. Requires all packages clean. No save.")
				: TEXT("Expand exact Beat 3 DA_Mission_NeuroGenetics to Beat 4: add Event_NeuralMappingSignalTraced on trace task and ordered Obj_FollowNeuralSignature. Does not save or compile."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteExpandNeuroGeneticsMissionBeat4(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("expand_neurogenetics_mission_beat4 must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightExpandNeuroGeneticsMissionBeat4(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (!Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("DA_Mission_NeuroGenetics missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		if (NeuroGeneticsMissionBeat4MismatchReason(Existing).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact Beat 4 found dirty packages: %s. Hard stop. Do not save."),
						*FString::Join(DirtyAfter, TEXT(","))),
					MakeShared<FBridgeChange>(Change));
			}
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
			Change.After->SetBoolField(TEXT("expanded"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroGeneticsMissionBeat4ProposedTasks(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		if (!NeuroGeneticsMissionBeat3MismatchReason(Existing).IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 3 at execute. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UPackage* Package = Existing->GetOutermost();
		const bool bPackageWasDirtyBefore = Package && Package->IsDirty();
		UObject* Snapshot = StaticDuplicateObject(
			Existing,
			GetTransientPackage(),
			*FString::Printf(TEXT("%s_Beat3Snapshot"), *Existing->GetName()));
		if (!Snapshot)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("snapshot_failed"),
				TEXT("Failed to snapshot Beat 3 mission for rollback. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"ExpandNeuroGeneticsMissionBeat4",
				"Expand NeuroGenetics Mission Beat 4"));
			if (const FString ApplyError = ApplyNeuroGeneticsMissionBeat4(Existing); !ApplyError.IsEmpty())
			{
				TArray<FString> Restored;
				const FString RestoreError =
					RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(
					TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
				Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
				return FailAudit(
					TEXT("apply_failed"),
					FString::Printf(
						TEXT("%s Rolled back to Beat 3 snapshot. %s ZERO remaining writes."),
						*ApplyError,
						RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
					MakeShared<FBridgeChange>(Change));
			}
			Existing->MarkPackageDirty();
			if (Package)
			{
				Package->MarkPackageDirty();
			}
		}

		if (const FString VerifyError = NeuroGeneticsMissionBeat4MismatchReason(Existing); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			const FString RestoreError =
				RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(
					TEXT("%s Rolled back to Beat 3 snapshot. %s ZERO remaining writes."),
					*VerifyError,
					RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroGeneticsMissionPackage);
		if (DirtyAfter.Num() != 1 || !PackagesEqual(DirtyAfter[0], ExpectedDirty))
		{
			TArray<FString> Restored;
			const FString RestoreError =
				RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
			return FailAudit(
				TEXT("dirty_package_contract"),
				FString::Printf(
					TEXT("After expand, dirty packages must be exactly [%s], got [%s]. Rolled back. %s"),
					*ExpectedDirty,
					*FString::Join(DirtyAfter, TEXT(",")),
					RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("expanded"));
		Change.After->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Existing));
		Change.After->SetBoolField(TEXT("expanded"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetObjectField(TEXT("mission_before"), NeuroGeneticsMissionBeat3ProposedTasks(true));
		Change.After->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat4ProposedTasks(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
