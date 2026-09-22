	// Fixed NeuroGenetics mission Beat 6 expansion —
	// expand_neurogenetics_mission_beat6 / neurogenetics_mission_beat6_v1.
	// Transforms exact Beat 5 (neurogenetics_mission_beat5_v1) → exact Beat 6.
	// Preview is mutation-free. Apply modifies only the exact DataAsset; never saves; never repairs mismatches.
	// Reuses Beat 3/5 helpers already defined above; does not modify Beat 1–5 implementations.
	const TCHAR* NeuroGeneticsMissionBeat6Spec = TEXT("neurogenetics_mission_beat6_v1");
	const TCHAR* NeuroGeneticsMissionBeat6Action = TEXT("expand_neurogenetics_mission_beat6");
	const TCHAR* NeuroGeneticsExamineEventId = TEXT("Event_NeuralChangeEvidenceExamined");

	TSharedRef<FJsonObject> NeuroGeneticsMissionBeat6ProposedTasks(bool bAlreadyExact)
	{
		TArray<FString> IsolateEvents;
		IsolateEvents.Add(NeuroGeneticsIsolateEventId);
		TArray<FString> TracePrereqs;
		TracePrereqs.Add(NeuroGeneticsObjectiveId);
		TArray<FString> TraceEvents;
		TraceEvents.Add(NeuroGeneticsTraceEventId);
		TArray<FString> FollowPrereqs;
		FollowPrereqs.Add(NeuroGeneticsTraceObjectiveId);
		TArray<FString> FollowEvents;
		FollowEvents.Add(NeuroGeneticsFollowEventId);
		TArray<FString> ExaminePrereqs;
		ExaminePrereqs.Add(NeuroGeneticsFollowObjectiveId);
		TArray<FString> ExamineEvents;
		ExamineEvents.Add(NeuroGeneticsExamineEventId);

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
			FollowEvents)));
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsExamineObjectiveId,
			NeuroGeneticsExamineObjectiveTitle,
			NeuroGeneticsExamineObjectiveDescription,
			true,
			ExaminePrereqs,
			ExamineEvents)));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroGeneticsMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroGeneticsMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroGeneticsMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetNumberField(TEXT("task_count"), 4);
		Mission->SetStringField(TEXT("shape"), TEXT("beat6"));
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Mission->SetStringField(
			TEXT("runtime_task3"),
			TEXT("Locked/Inactive until Obj_TraceNeuralMappingSignal completes, then Active via auto-unlock; completes on Event_NeuralSignatureFollowed"));
		Mission->SetStringField(
			TEXT("runtime_task4"),
			TEXT("Locked/Inactive until Obj_FollowNeuralSignature completes, then Active via auto-unlock; completes on Event_NeuralChangeEvidenceExamined"));
		return Mission;
	}

	FString NeuroGeneticsMissionBeat6RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
					TEXT("expand_neurogenetics_mission_beat6 rejects client-provided '%s'. Spec neurogenetics_mission_beat6_v1 is fixed."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroGeneticsMissionBeat6Spec);
		if (!Spec.Equals(NeuroGeneticsMissionBeat6Spec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neurogenetics_mission_beat6_v1.");
		}
		return TEXT("");
	}

	FString NeuroGeneticsMissionBeat6MismatchReason(UObject* Asset)
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
		if (Helper.Num() != 4)
		{
			return FString::Printf(TEXT("Tasks count=%d, expected exactly 4"), Helper.Num());
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
				1,
				NeuroGeneticsFollowEventId);
			!Task2Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[2]: %s"), *Task2Error);
		}
		if (const FString Task3Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(3),
				TaskStruct,
				NeuroGeneticsExamineObjectiveId,
				NeuroGeneticsExamineObjectiveTitle,
				NeuroGeneticsExamineObjectiveDescription,
				true,
				1,
				NeuroGeneticsFollowObjectiveId,
				1,
				NeuroGeneticsExamineEventId);
			!Task3Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[3]: %s"), *Task3Error);
		}
		return TEXT("");
	}

	FString ApplyNeuroGeneticsMissionBeat6(UObject* Asset)
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
		Helper.Resize(4);

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
		TArray<FName> FollowEvents;
		FollowEvents.Add(FName(NeuroGeneticsFollowEventId));
		if (const FString Task2Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(2),
				TaskStruct,
				NeuroGeneticsFollowObjectiveId,
				NeuroGeneticsFollowObjectiveTitle,
				NeuroGeneticsFollowObjectiveDescription,
				true,
				FollowPrereqs,
				FollowEvents);
			!Task2Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[2] apply: %s"), *Task2Error);
		}

		TArray<FName> ExaminePrereqs;
		ExaminePrereqs.Add(FName(NeuroGeneticsFollowObjectiveId));
		TArray<FName> ExamineEvents;
		ExamineEvents.Add(FName(NeuroGeneticsExamineEventId));
		if (const FString Task3Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(3),
				TaskStruct,
				NeuroGeneticsExamineObjectiveId,
				NeuroGeneticsExamineObjectiveTitle,
				NeuroGeneticsExamineObjectiveDescription,
				true,
				ExaminePrereqs,
				ExamineEvents);
			!Task3Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[3] apply: %s"), *Task3Error);
		}
		return TEXT("");
	}

	FString GuardNeuroGeneticsMissionBeat6EditorContext()
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. expand_neurogenetics_mission_beat6 is an Unreal Editor write.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		return TEXT("");
	}

	FString PreflightExpandNeuroGeneticsMissionBeat6(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroGeneticsMissionBeat6EditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. expand_neurogenetics_mission_beat6 does not save or compile.");
		}
		if (const FString OverrideError = NeuroGeneticsMissionBeat6RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (!IsInGameThread())
		{
			return TEXT("expand_neurogenetics_mission_beat6 preflight must run on the game thread.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Packages dirty: %s. Require clean packages before expand_neurogenetics_mission_beat6."),
				*FString::Join(Dirty, TEXT(",")));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (!Existing)
		{
			return TEXT("DA_Mission_NeuroGenetics is missing. Persist Beat 5 expand_neurogenetics_mission_beat5 + save first.");
		}
		if (!NeuroGeneticsMissionClassMatches(Existing))
		{
			return FString::Printf(
				TEXT("Exact path occupied by wrong class '%s'. Fail closed — no automatic repair."),
				*ClassName(Existing));
		}

		const FString Beat5Mismatch = NeuroGeneticsMissionBeat5MismatchReason(Existing);
		const FString Beat6Mismatch = NeuroGeneticsMissionBeat6MismatchReason(Existing);
		const bool bBeat5Exact = Beat5Mismatch.IsEmpty();
		const bool bBeat6Exact = Beat6Mismatch.IsEmpty();
		if (!bBeat5Exact && !bBeat6Exact)
		{
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is neither exact Beat 5 nor exact Beat 6. Beat5: %s | Beat6: %s. Fail closed — no opportunistic repair."),
				*Beat5Mismatch,
				*Beat6Mismatch);
		}

		Before->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat6Spec);
		Before->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat6Action);
		Before->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("packages_clean"), true);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("asset_present"), true);
		Before->SetStringField(TEXT("class"), ClassName(Existing));
		Before->SetBoolField(TEXT("beat5_exact"), bBeat5Exact);
		Before->SetBoolField(TEXT("beat6_exact"), bBeat6Exact);
		Before->SetObjectField(
			TEXT("mission_before"),
			bBeat6Exact ? NeuroGeneticsMissionBeat6ProposedTasks(true) : NeuroGeneticsMissionBeat5ProposedTasks(true));

		Proposed->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat6Spec);
		Proposed->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat6Action);
		Proposed->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroGeneticsMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat6ProposedTasks(bBeat6Exact));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bBeat6Exact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bBeat6Exact
				? TEXT("DA_Mission_NeuroGenetics already matches neurogenetics_mission_beat6_v1. Execute is a clean no-op. Requires all packages clean. No save.")
				: TEXT("Expand exact Beat 5 DA_Mission_NeuroGenetics to Beat 6: add Event_NeuralChangeEvidenceExamined Complete on examine task. Does not save or compile."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteExpandNeuroGeneticsMissionBeat6(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("expand_neurogenetics_mission_beat6 must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightExpandNeuroGeneticsMissionBeat6(Change.Args, Before, Proposed);
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

		if (NeuroGeneticsMissionBeat6MismatchReason(Existing).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact Beat 6 found dirty packages: %s. Hard stop. Do not save."),
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
			Change.After->SetObjectField(TEXT("mission"), NeuroGeneticsMissionBeat6ProposedTasks(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		if (!NeuroGeneticsMissionBeat5MismatchReason(Existing).IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 5 at execute. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UPackage* Package = Existing->GetOutermost();
		const bool bPackageWasDirtyBefore = Package && Package->IsDirty();
		UObject* Snapshot = StaticDuplicateObject(
			Existing,
			GetTransientPackage(),
			*FString::Printf(TEXT("%s_Beat5Snapshot"), *Existing->GetName()));
		if (!Snapshot)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("snapshot_failed"),
				TEXT("Failed to snapshot Beat 5 mission for rollback. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"ExpandNeuroGeneticsMissionBeat6",
				"Expand NeuroGenetics Mission Beat 6"));
			if (const FString ApplyError = ApplyNeuroGeneticsMissionBeat6(Existing); !ApplyError.IsEmpty())
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
						TEXT("%s Rolled back to Beat 5 snapshot. %s ZERO remaining writes."),
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

		if (const FString VerifyError = NeuroGeneticsMissionBeat6MismatchReason(Existing); !VerifyError.IsEmpty())
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
					TEXT("%s Rolled back to Beat 5 snapshot. %s ZERO remaining writes."),
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
		Change.After->SetObjectField(TEXT("mission_before"), NeuroGeneticsMissionBeat5ProposedTasks(true));
		Change.After->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat6ProposedTasks(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
