// Soft-link Cryo Entry → Cryo Evidence. Does not save and does not change power.
	const TCHAR* CryoEntryNextCryoEvidenceSpec = TEXT("cryo_entry_next_cryo_evidence_v1");
	const TCHAR* CryoEntryNextCryoEvidenceAction = TEXT("set_cryo_entry_next_cryo_evidence");

	FString PreflightSetCryoEntryNextCryoEvidence(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_cryo_entry_next_cryo_evidence does not save.");
		}
		UObject* CryoEntry = FindCryoEntryMissionAssetExact();
		if (!CryoEntry)
		{
			return TEXT("DA_Mission_CryoEntry missing.");
		}
		if (const FString EntryMismatch = CryoEntryMissionMismatchReason(CryoEntry); !EntryMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoEntry mismatch: %s"), *EntryMismatch);
		}
		UObject* Evidence = FindCryoEvidenceMissionAssetExact();
		if (!Evidence)
		{
			return TEXT("DA_Mission_CryoEvidence missing. Create it before setting NextMissionAsset.");
		}
		if (const FString EvidenceMismatch = CryoEvidenceMissionMismatchReason(Evidence); !EvidenceMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoEvidence mismatch: %s"), *EvidenceMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoEntry);
		const bool bAlreadyExact = LiveNext.Equals(CryoEvidenceMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), CryoEntryNextCryoEvidenceAction);
		Proposed->SetStringField(TEXT("next_mission_asset"), CryoEvidenceMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetCryoEntryNextCryoEvidence(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_cryo_entry_next_cryo_evidence must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetCryoEntryNextCryoEvidence(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* CryoEntry = FindCryoEntryMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoEntry);
		if (LiveNext.Equals(CryoEvidenceMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("next_mission_asset"), CryoEvidenceMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoEntryNextEvidence", "Link Cryo Entry to Cryo Evidence"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(CryoEntry, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(CryoEntry, FSoftObjectPtr(FSoftObjectPath(CryoEvidenceMissionObjectPath)));
			CryoEntry->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoEntry);
		if (!AfterNext.Equals(CryoEvidenceMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Cryo Entry NextMissionAsset did not become DA_Mission_CryoEvidence."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
