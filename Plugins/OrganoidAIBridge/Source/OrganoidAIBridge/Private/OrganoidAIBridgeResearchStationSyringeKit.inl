// Add the third Research Station contract for the syringe kit. Does not move, save, or change power.
	const TCHAR* ResearchStationSyringeKitSpec = TEXT("research_station_syringe_kit_v1");
	const TCHAR* ResearchStationSyringeKitAction = TEXT("configure_research_station_syringe_kit");
	const TCHAR* ResearchStationSyringeKitPrompt = TEXT("Recover Syringe Kit");
	const TCHAR* ResearchStationSyringeKitSpeaker = TEXT("Nathan");
	const TCHAR* ResearchStationSyringeKitLine = TEXT("More syringes. Each one targets a different system. Movement, vision... Epitope was building a toolkit.");
	constexpr double ResearchStationSyringeKitDuration = 7.0;

	bool ResearchStationSyringeKitExact(AActor* Actor)
	{
		if (!Actor || !ResearchStationConfigureExact(Actor))
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringeRequiredActiveObjectiveId"), SyringeKitObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringeSuccessObjectiveEventId"), SyringeKitEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringeReplayGuardObjectiveId"), SyringeKitObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringePrompt"), ResearchStationSyringeKitPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringeNotificationSpeaker"), ResearchStationSyringeKitSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SyringeNotificationText"), ResearchStationSyringeKitLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("SyringeNotificationDurationSeconds"), ResearchStationSyringeKitDuration).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), ResearchStationConfigurePrompt).IsEmpty();
	}

	FString PreflightConfigureResearchStationSyringeKit(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_research_station_syringe_kit does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ResearchStationSyringeKitSpec);
		if (!Spec.Equals(ResearchStationSyringeKitSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ResearchStationSyringeKitSpec);
		}
		UObject* SyringeKit = FindSyringeKitMissionAssetExact();
		if (!SyringeKit || !SyringeKitMissionMismatchReason(SyringeKit).IsEmpty())
		{
			return TEXT("DA_Mission_SyringeKit must exist and match.");
		}
		const FString ResearchNext = NeuroAdaptationConnectionNextRevelationReadNext(FindRespecBeatMissionAssetExact());
		if (!ResearchNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ResearchStation.NextMissionAsset must already be DA_Mission_SyringeKit.");
		}
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		AActor* Station = nullptr;
		if (const FString StationError = ResearchStationConfigureRequire(World, Station); !StationError.IsEmpty())
		{
			return StationError;
		}
		if (!ResearchStationConfigureExact(Station))
		{
			return TEXT("Intro and free-respec contracts must stay exact before the syringe-kit contract.");
		}
		const bool bAlreadyExact = ResearchStationSyringeKitExact(Station);
		Before->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
		Before->SetStringField(TEXT("location"), TEXT("800,-1600,-1100"));
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), ResearchStationSyringeKitAction);
		Proposed->SetStringField(TEXT("spec"), ResearchStationSyringeKitSpec);
		Proposed->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
		Proposed->SetStringField(TEXT("objective_id"), SyringeKitObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), SyringeKitEventId);
		Proposed->SetStringField(TEXT("prompt"), ResearchStationSyringeKitPrompt);
		Proposed->SetStringField(TEXT("line"), ResearchStationSyringeKitLine);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureResearchStationSyringeKit(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_research_station_syringe_kit must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureResearchStationSyringeKit(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		AActor* Station = nullptr;
		if (const FString StationError = ResearchStationConfigureRequire(World, Station); !StationError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), StationError, MakeShared<FBridgeChange>(Change));
		}
		const FVector LocationBefore = Station->GetActorLocation();
		if (ResearchStationSyringeKitExact(Station))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ConfigureResearchStationSyringeKit", "Configure Research Station syringe kit"));
			FString WriteError = SetNamedPropertyFromString(Station, TEXT("SyringeRequiredActiveObjectiveId"), SyringeKitObjectiveId);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromString(Station, TEXT("SyringeSuccessObjectiveEventId"), SyringeKitEventId);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromString(Station, TEXT("SyringeReplayGuardObjectiveId"), SyringeKitObjectiveId);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromString(Station, TEXT("SyringePrompt"), ResearchStationSyringeKitPrompt);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromString(Station, TEXT("SyringeNotificationSpeaker"), ResearchStationSyringeKitSpeaker);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromString(Station, TEXT("SyringeNotificationText"), ResearchStationSyringeKitLine);
			if (WriteError.IsEmpty()) WriteError = SetNamedPropertyFromNumber(Station, TEXT("SyringeNotificationDurationSeconds"), ResearchStationSyringeKitDuration);
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Station->MarkPackageDirty();
		}
		if (!Station->GetActorLocation().Equals(LocationBefore, 0.1f)
			|| !ResearchStationSyringeKitExact(Station)
			|| !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Syringe-kit contract did not verify, or the actor moved, or power changed."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
		Change.After->SetNumberField(TEXT("contracts"), 3);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
