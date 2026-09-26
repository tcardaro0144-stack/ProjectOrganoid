// Configure the existing ResearchStation_NeuroGenetics respec presentation. Does not move, save, or change power.
	const TCHAR* ResearchStationConfigureSpec = TEXT("research_station_configure_v1");
	const TCHAR* ResearchStationConfigureAction = TEXT("configure_research_station");
	const TCHAR* ResearchStationConfigureLabel = TEXT("ResearchStation_NeuroGenetics");
	const FVector ResearchStationConfigureLocation(800.f, -1600.f, -1100.f);
	const TCHAR* ResearchStationConfigurePrompt = TEXT("Use Research Station");
	const TCHAR* ResearchStationConfigureSpeaker = TEXT("Nathan");
	const TCHAR* ResearchStationConfigureLine = TEXT("Free respec, no penalty. This is where I rethink the build. No currency, no shop — just reconfiguration.");
	constexpr double ResearchStationConfigureDuration = 7.0;

	bool ResearchStationConfigureExact(AActor* Actor)
	{
		if (!Actor || !Actor->GetActorLocation().Equals(ResearchStationConfigureLocation, 1.f))
		{
			return false;
		}
		if (!NeuroResearchStationIntro_MismatchReason(Actor).IsEmpty())
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RespecRequiredActiveObjectiveId"), RespecBeatObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RespecSuccessObjectiveEventId"), RespecBeatEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RespecReplayGuardObjectiveId"), RespecBeatObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RespecNotificationSpeaker"), ResearchStationConfigureSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RespecNotificationText"), ResearchStationConfigureLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("RespecNotificationDurationSeconds"), ResearchStationConfigureDuration).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), ResearchStationConfigurePrompt).IsEmpty();
	}

	FString ResearchStationConfigureRequire(UWorld* World, AActor*& OutActor)
	{
		OutActor = nullptr;
		const TArray<AActor*> Found = FindByExactLabel(World, ResearchStationConfigureLabel);
		if (Found.Num() != 1 || !Found[0])
		{
			return FString::Printf(TEXT("%s must be unique. count=%d."), ResearchStationConfigureLabel, Found.Num());
		}
		AActor* Actor = Found[0];
		if (!Actor->GetClass() || !Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidResearchStation")))
		{
			return TEXT("ResearchStation_NeuroGenetics class mismatch.");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return TEXT("ResearchStation_NeuroGenetics must live on SL_Epitope_NeuroGenetics.");
		}
		if (!Actor->GetActorLocation().Equals(ResearchStationConfigureLocation, 1.f))
		{
			return TEXT("ResearchStation_NeuroGenetics moved. ZERO writes.");
		}
		if (const FString IntroError = NeuroResearchStationIntro_MismatchReason(Actor); !IntroError.IsEmpty())
		{
			return FString::Printf(TEXT("Neural Slow intro contract must stay exact. %s"), *IntroError);
		}
		OutActor = Actor;
		return TEXT("");
	}

	FString PreflightConfigureResearchStation(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_research_station does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), ResearchStationConfigureSpec);
		if (!Spec.Equals(ResearchStationConfigureSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ResearchStationConfigureSpec);
		}
		UObject* ResearchStation = FindRespecBeatMissionAssetExact();
		if (!ResearchStation)
		{
			return TEXT("DA_Mission_ResearchStation missing.");
		}
		if (const FString ResearchMismatch = RespecBeatMissionMismatchReason(ResearchStation); !ResearchMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ResearchStation mismatch: %s"), *ResearchMismatch);
		}
		const FString ConclusionNext = NeuroAdaptationConnectionNextRevelationReadNext(FindTheConclusionMissionAssetExact());
		if (!ConclusionNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_TheConclusion.NextMissionAsset must already be DA_Mission_ResearchStation.");
		}
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		AActor* Station = nullptr;
		if (const FString StationError = ResearchStationConfigureRequire(World, Station); !StationError.IsEmpty())
		{
			return StationError;
		}
		const bool bAlreadyExact = ResearchStationConfigureExact(Station);
		Before->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
		Before->SetStringField(TEXT("package"), NeuroPackage);
		Before->SetStringField(TEXT("location"), TEXT("800,-1600,-1100"));
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), ResearchStationConfigureAction);
		Proposed->SetStringField(TEXT("spec"), ResearchStationConfigureSpec);
		Proposed->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
		Proposed->SetStringField(TEXT("objective_id"), RespecBeatObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), RespecBeatEventId);
		Proposed->SetStringField(TEXT("prompt"), ResearchStationConfigurePrompt);
		Proposed->SetStringField(TEXT("line"), ResearchStationConfigureLine);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves"), false);
		Proposed->SetBoolField(TEXT("unlocks"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureResearchStation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_research_station must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureResearchStation(Change.Args, Before, Proposed);
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
		if (ResearchStationConfigureExact(Station))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetStringField(TEXT("label"), ResearchStationConfigureLabel);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ConfigureResearchStationRespec", "Configure Research Station free respec presentation"));
			FString WriteError = SetNamedPropertyFromString(Station, TEXT("RespecRequiredActiveObjectiveId"), RespecBeatObjectiveId);
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(Station, TEXT("RespecSuccessObjectiveEventId"), RespecBeatEventId);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(Station, TEXT("RespecReplayGuardObjectiveId"), RespecBeatObjectiveId);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(Station, TEXT("RespecNotificationSpeaker"), ResearchStationConfigureSpeaker);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(Station, TEXT("RespecNotificationText"), ResearchStationConfigureLine);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromNumber(Station, TEXT("RespecNotificationDurationSeconds"), ResearchStationConfigureDuration);
			}
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Station->MarkPackageDirty();
		}
		if (!Station->GetActorLocation().Equals(LocationBefore, 0.1f) || !ResearchStationConfigureExact(Station) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Research Station respec contract did not verify, or the actor moved, or power changed."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetStringField(TEXT("package"), NeuroPackage);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves"), false);
		Change.After->SetBoolField(TEXT("unlocks"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
