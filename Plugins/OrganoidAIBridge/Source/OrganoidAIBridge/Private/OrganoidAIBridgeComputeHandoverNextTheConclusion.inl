// Soft-link Compute Handover → The Conclusion. Does not save and does not change power.
	const TCHAR* ComputeHandoverNextTheConclusionSpec = TEXT("compute_handover_next_the_conclusion_v1");
	const TCHAR* ComputeHandoverNextTheConclusionAction = TEXT("set_compute_handover_next_the_conclusion");

	FString PreflightSetComputeHandoverNextTheConclusion(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_compute_handover_next_the_conclusion does not save.");
		}
		const FString EntryNext = NeuroAdaptationConnectionNextRevelationReadNext(FindComputeEntryMissionAssetExact());
		if (!EntryNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ComputeEntry.NextMissionAsset must already be DA_Mission_ComputeHandover.");
		}
		UObject* Handover = FindComputeHandoverMissionAssetExact();
		if (!Handover)
		{
			return TEXT("DA_Mission_ComputeHandover missing.");
		}
		if (const FString HandoverMismatch = ComputeHandoverMissionMismatchReason(Handover); !HandoverMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeHandover mismatch: %s"), *HandoverMismatch);
		}
		UObject* Conclusion = FindTheConclusionMissionAssetExact();
		if (!Conclusion)
		{
			return TEXT("DA_Mission_TheConclusion missing. Create it before setting NextMissionAsset.");
		}
		if (const FString ConclusionMismatch = TheConclusionMissionMismatchReason(Conclusion); !ConclusionMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_TheConclusion mismatch: %s"), *ConclusionMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Handover);
		const bool bAlreadyExact = LiveNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), ComputeHandoverNextTheConclusionAction);
		Proposed->SetStringField(TEXT("spec"), ComputeHandoverNextTheConclusionSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), TheConclusionMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetComputeHandoverNextTheConclusion(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_compute_handover_next_the_conclusion must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetComputeHandoverNextTheConclusion(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Handover = FindComputeHandoverMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Handover);
		if (LiveNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), TheConclusionMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ComputeHandoverNextConclusion", "Link Compute Handover to The Conclusion"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Handover, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(Handover, FSoftObjectPtr(FSoftObjectPath(TheConclusionMissionObjectPath)));
			Handover->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(Handover);
		if (!AfterNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Compute Handover NextMissionAsset did not become DA_Mission_TheConclusion."), MakeShared<FBridgeChange>(Change));
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
