// Soft-link Cryo Evidence → Compute Entry. Does not save and does not change power.
	const TCHAR* CryoEvidenceNextComputeEntrySpec = TEXT("cryo_evidence_next_compute_entry_v1");
	const TCHAR* CryoEvidenceNextComputeEntryAction = TEXT("set_cryo_evidence_next_compute_entry");

	FString PreflightSetCryoEvidenceNextComputeEntry(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_cryo_evidence_next_compute_entry does not save.");
		}
		UObject* Evidence = FindCryoEvidenceMissionAssetExact();
		if (!Evidence)
		{
			return TEXT("DA_Mission_CryoEvidence missing.");
		}
		if (const FString EvidenceMismatch = CryoEvidenceMissionMismatchReason(Evidence); !EvidenceMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoEvidence mismatch: %s"), *EvidenceMismatch);
		}
		UObject* CryoEntry = FindCryoEntryMissionAssetExact();
		const FString EntryNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoEntry);
		if (!EntryNext.Equals(CryoEvidenceMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_CryoEntry.NextMissionAsset must already be DA_Mission_CryoEvidence.");
		}
		UObject* ComputeEntry = FindComputeEntryMissionAssetExact();
		if (!ComputeEntry)
		{
			return TEXT("DA_Mission_ComputeEntry missing. Create it before setting NextMissionAsset.");
		}
		if (const FString ComputeMismatch = ComputeEntryMissionMismatchReason(ComputeEntry); !ComputeMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeEntry mismatch: %s"), *ComputeMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Evidence);
		const bool bAlreadyExact = LiveNext.Equals(ComputeEntryMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), CryoEvidenceNextComputeEntryAction);
		Proposed->SetStringField(TEXT("spec"), CryoEvidenceNextComputeEntrySpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), ComputeEntryMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetCryoEvidenceNextComputeEntry(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_cryo_evidence_next_compute_entry must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetCryoEvidenceNextComputeEntry(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Evidence = FindCryoEvidenceMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Evidence);
		if (LiveNext.Equals(ComputeEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), ComputeEntryMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoEvidenceNextCompute", "Link Cryo Evidence to Compute Entry"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Evidence, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(Evidence, FSoftObjectPtr(FSoftObjectPath(ComputeEntryMissionObjectPath)));
			Evidence->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(Evidence);
		if (!AfterNext.Equals(ComputeEntryMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Cryo Evidence NextMissionAsset did not become DA_Mission_ComputeEntry."), MakeShared<FBridgeChange>(Change));
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
