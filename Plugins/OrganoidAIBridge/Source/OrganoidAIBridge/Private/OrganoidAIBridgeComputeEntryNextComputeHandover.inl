// Soft-link Compute Entry → Compute Handover. Does not save and does not change power.
	const TCHAR* ComputeEntryNextComputeHandoverSpec = TEXT("compute_entry_next_compute_handover_v1");
	const TCHAR* ComputeEntryNextComputeHandoverAction = TEXT("set_compute_entry_next_compute_handover");

	FString PreflightSetComputeEntryNextComputeHandover(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_compute_entry_next_compute_handover does not save.");
		}
		UObject* Evidence = FindCryoEvidenceMissionAssetExact();
		const FString EvidenceNext = NeuroAdaptationConnectionNextRevelationReadNext(Evidence);
		if (!EvidenceNext.Equals(ComputeEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_CryoEvidence.NextMissionAsset must already be DA_Mission_ComputeEntry.");
		}
		UObject* ComputeEntry = FindComputeEntryMissionAssetExact();
		if (!ComputeEntry)
		{
			return TEXT("DA_Mission_ComputeEntry missing.");
		}
		if (const FString EntryMismatch = ComputeEntryMissionMismatchReason(ComputeEntry); !EntryMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeEntry mismatch: %s"), *EntryMismatch);
		}
		UObject* Handover = FindComputeHandoverMissionAssetExact();
		if (!Handover)
		{
			return TEXT("DA_Mission_ComputeHandover missing. Create it before setting NextMissionAsset.");
		}
		if (const FString HandoverMismatch = ComputeHandoverMissionMismatchReason(Handover); !HandoverMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeHandover mismatch: %s"), *HandoverMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(ComputeEntry);
		const bool bAlreadyExact = LiveNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), ComputeEntryNextComputeHandoverAction);
		Proposed->SetStringField(TEXT("spec"), ComputeEntryNextComputeHandoverSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), ComputeHandoverMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetComputeEntryNextComputeHandover(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_compute_entry_next_compute_handover must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetComputeEntryNextComputeHandover(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* ComputeEntry = FindComputeEntryMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(ComputeEntry);
		if (LiveNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), ComputeHandoverMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ComputeEntryNextHandover", "Link Compute Entry to Compute Handover"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(ComputeEntry, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(ComputeEntry, FSoftObjectPtr(FSoftObjectPath(ComputeHandoverMissionObjectPath)));
			ComputeEntry->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(ComputeEntry);
		if (!AfterNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Compute Entry NextMissionAsset did not become DA_Mission_ComputeHandover."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetBoolField(TEXT("linked"), true);
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
