// Soft-link The Conclusion → Research Station. Does not save and does not change power.
	const TCHAR* TheConclusionNextResearchStationSpec = TEXT("the_conclusion_next_research_station_v1");
	const TCHAR* TheConclusionNextResearchStationAction = TEXT("set_the_conclusion_next_research_station");

	FString PreflightSetTheConclusionNextResearchStation(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_the_conclusion_next_research_station does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), TheConclusionNextResearchStationSpec);
		if (!Spec.Equals(TheConclusionNextResearchStationSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), TheConclusionNextResearchStationSpec);
		}
		const FString HandoverNext = NeuroAdaptationConnectionNextRevelationReadNext(FindComputeHandoverMissionAssetExact());
		if (!HandoverNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ComputeHandover.NextMissionAsset must already be DA_Mission_TheConclusion.");
		}
		UObject* ResearchStation = FindRespecBeatMissionAssetExact();
		if (!ResearchStation)
		{
			return TEXT("DA_Mission_ResearchStation missing. Create it before setting NextMissionAsset.");
		}
		if (const FString ResearchMismatch = RespecBeatMissionMismatchReason(ResearchStation); !ResearchMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ResearchStation mismatch: %s"), *ResearchMismatch);
		}
		UObject* Conclusion = FindTheConclusionMissionAssetExact();
		if (!Conclusion)
		{
			return TEXT("DA_Mission_TheConclusion missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Conclusion);
		const bool bAlreadyExact = LiveNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString ConclusionMismatch = TheConclusionMissionMismatchReason(Conclusion); !ConclusionMismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_TheConclusion mismatch: %s"), *ConclusionMismatch);
			}
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), TheConclusionNextResearchStationAction);
		Proposed->SetStringField(TEXT("spec"), TheConclusionNextResearchStationSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), RespecBeatMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetTheConclusionNextResearchStation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_the_conclusion_next_research_station must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetTheConclusionNextResearchStation(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Conclusion = FindTheConclusionMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Conclusion);
		if (LiveNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), RespecBeatMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ConclusionNextResearchStation", "Link The Conclusion to the Research Station"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Conclusion, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(Conclusion, FSoftObjectPtr(FSoftObjectPath(RespecBeatMissionObjectPath)));
			Conclusion->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(Conclusion);
		if (!AfterNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("The Conclusion NextMissionAsset did not become DA_Mission_ResearchStation."), MakeShared<FBridgeChange>(Change));
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
