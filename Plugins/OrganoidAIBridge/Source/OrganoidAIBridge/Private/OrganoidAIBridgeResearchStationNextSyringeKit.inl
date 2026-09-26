// Soft-link Research Station → Syringe Kit. Does not save and does not change power.
	const TCHAR* ResearchStationNextSyringeKitSpec = TEXT("research_station_next_syringe_kit_v1");
	const TCHAR* ResearchStationNextSyringeKitAction = TEXT("set_research_station_next_syringe_kit");

	FString PreflightSetResearchStationNextSyringeKit(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_research_station_next_syringe_kit does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ResearchStationNextSyringeKitSpec);
		if (!Spec.Equals(ResearchStationNextSyringeKitSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ResearchStationNextSyringeKitSpec);
		}
		const FString ConclusionNext = NeuroAdaptationConnectionNextRevelationReadNext(FindTheConclusionMissionAssetExact());
		if (!ConclusionNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_TheConclusion.NextMissionAsset must stay DA_Mission_ResearchStation.");
		}
		UObject* SyringeKit = FindSyringeKitMissionAssetExact();
		if (!SyringeKit)
		{
			return TEXT("DA_Mission_SyringeKit missing. Create it before setting NextMissionAsset.");
		}
		if (const FString SyringeMismatch = SyringeKitMissionMismatchReason(SyringeKit); !SyringeMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_SyringeKit mismatch: %s"), *SyringeMismatch);
		}
		UObject* ResearchStation = FindRespecBeatMissionAssetExact();
		if (!ResearchStation)
		{
			return TEXT("DA_Mission_ResearchStation missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(ResearchStation);
		const bool bAlreadyExact = LiveNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString ResearchMismatch = RespecBeatMissionMismatchReason(ResearchStation); !ResearchMismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_ResearchStation mismatch: %s"), *ResearchMismatch);
			}
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), ResearchStationNextSyringeKitAction);
		Proposed->SetStringField(TEXT("spec"), ResearchStationNextSyringeKitSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), SyringeKitMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetResearchStationNextSyringeKit(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_research_station_next_syringe_kit must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetResearchStationNextSyringeKit(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* ResearchStation = FindRespecBeatMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(ResearchStation);
		if (LiveNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), SyringeKitMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ResearchStationNextSyringeKit", "Link Research Station to the Syringe Kit"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(ResearchStation, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(ResearchStation, FSoftObjectPtr(FSoftObjectPath(SyringeKitMissionObjectPath)));
			ResearchStation->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(ResearchStation);
		if (!AfterNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Research Station NextMissionAsset did not become DA_Mission_SyringeKit, or power changed."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetBoolField(TEXT("linked"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
