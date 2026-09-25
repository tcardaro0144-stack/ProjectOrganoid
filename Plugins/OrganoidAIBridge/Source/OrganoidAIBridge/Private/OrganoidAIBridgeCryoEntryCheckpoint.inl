// Configure Checkpoint_FreightAirlock as the Cryo entry interact. Does not move, save, or change power.
	const TCHAR* CryoEntryCheckpointSpec = TEXT("cryo_entry_checkpoint_v1");
	const TCHAR* CryoEntryCheckpointAction = TEXT("configure_cryo_entry_checkpoint");
	const TCHAR* CryoEntryCheckpointPrompt = TEXT("Enter Cryo");
	const TCHAR* CryoEntryCheckpointSpeaker = TEXT("Nathan");
	const TCHAR* CryoEntryCheckpointLine = TEXT("This isn't just storage. These were people. Or parts of people.");
	const double CryoEntryCheckpointDuration = 7.0;

	bool CryoEntryCheckpoint_IsFullyConfigured(AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignRequiredActiveObjectiveId"), CryoEntryObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignSuccessEventId"), CryoEntryEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignEntryPrompt"), CryoEntryCheckpointPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), CryoEntryCheckpointPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignNotificationSpeaker"), CryoEntryCheckpointSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignNotificationText"), CryoEntryCheckpointLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CampaignNotificationDurationSeconds"), CryoEntryCheckpointDuration).IsEmpty();
	}

	FString CryoEntryCheckpointRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("location"), TEXT("transform"), TEXT("label"), TEXT("package"), TEXT("power"),
			TEXT("prompt"), TEXT("line"), TEXT("actor")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), CryoEntryCheckpointSpec);
		if (!Spec.Equals(CryoEntryCheckpointSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoEntryCheckpointSpec);
		}
		return TEXT("");
	}

	FString PreflightConfigureCryoEntryCheckpoint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_cryo_entry_checkpoint does not save.");
		}
		if (const FString OverrideError = CryoEntryCheckpointRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
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
		UObject* CryoAccess = FindCryoAccessMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoAccess);
		if (!LiveNext.Equals(CryoEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_CryoAccess.NextMissionAsset must already be DA_Mission_CryoEntry.");
		}
		UWorld* World = GetEditorWorld();
		const TArray<AActor*> Checkpoints = FindByExactLabel(World, CryoAccessCheckpointLabel);
		if (Checkpoints.Num() != 1 || !Checkpoints[0])
		{
			return FString::Printf(TEXT("%s must be unique. count=%d."), CryoAccessCheckpointLabel, Checkpoints.Num());
		}
		AActor* Checkpoint = Checkpoints[0];
		if (!Checkpoint->GetActorLocation().Equals(CryoAccessCheckpointLocation, 1.f))
		{
			return TEXT("Checkpoint_FreightAirlock moved. ZERO writes.");
		}
		const bool bAlreadyExact = CryoEntryCheckpoint_IsFullyConfigured(Checkpoint);
		Before->SetStringField(TEXT("spec"), CryoEntryCheckpointSpec);
		Before->SetStringField(TEXT("label"), CryoAccessCheckpointLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("location"), Checkpoint->GetActorLocation().ToString());
		Proposed->SetStringField(TEXT("spec"), CryoEntryCheckpointSpec);
		Proposed->SetStringField(TEXT("action"), CryoEntryCheckpointAction);
		Proposed->SetStringField(TEXT("label"), CryoAccessCheckpointLabel);
		Proposed->SetStringField(TEXT("sector"), TEXT("Cryo"));
		Proposed->SetStringField(TEXT("restored_state"), TEXT("Online"));
		Proposed->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Proposed->SetStringField(TEXT("required_objective"), CryoEntryObjectiveId);
		Proposed->SetStringField(TEXT("prompt"), CryoEntryCheckpointPrompt);
		Proposed->SetStringField(TEXT("event"), CryoEntryEventId);
		Proposed->SetStringField(TEXT("line"), CryoEntryCheckpointLine);
		Proposed->SetNumberField(TEXT("duration_seconds"), CryoEntryCheckpointDuration);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureCryoEntryCheckpoint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_cryo_entry_checkpoint must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureCryoEntryCheckpoint(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		const TArray<AActor*> Checkpoints = FindByExactLabel(World, CryoAccessCheckpointLabel);
		AActor* Checkpoint = Checkpoints.Num() == 1 ? Checkpoints[0] : nullptr;
		if (!Checkpoint)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Checkpoint_FreightAirlock missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const FVector LocationBefore = Checkpoint->GetActorLocation();
		if (CryoEntryCheckpoint_IsFullyConfigured(Checkpoint))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoEntryCheckpoint", "Configure Cryo entry checkpoint"));
			auto ApplyString = [Checkpoint](const TCHAR* Name, const TCHAR* Value) -> FString
			{
				return SetNamedPropertyFromString(Checkpoint, Name, Value);
			};
			auto ApplyFloat = [Checkpoint](const TCHAR* Name, double Value) -> FString
			{
				FProperty* Prop = FindInstanceProperty(Checkpoint, Name);
				if (!Prop)
				{
					return FString::Printf(TEXT("%s missing"), Name);
				}
				FString Error;
				if (!SetPropertyFromJson(Checkpoint, Prop, MakeShared<FJsonValueNumber>(Value), Error))
				{
					return Error;
				}
				return TEXT("");
			};
			FString WriteError;
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignRequiredActiveObjectiveId"), CryoEntryObjectiveId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignSuccessEventId"), CryoEntryEventId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignEntryPrompt"), CryoEntryCheckpointPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("InteractionPrompt"), CryoEntryCheckpointPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignNotificationSpeaker"), CryoEntryCheckpointSpeaker);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignNotificationText"), CryoEntryCheckpointLine);
			if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("CampaignNotificationDurationSeconds"), CryoEntryCheckpointDuration);
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Checkpoint->MarkPackageDirty();
		}
		if (!CryoEntryCheckpoint_IsFullyConfigured(Checkpoint) || !Checkpoint->GetActorLocation().Equals(LocationBefore, 0.1f))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Checkpoint_FreightAirlock was not configured in place."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(GetEditorWorld()); !PowerError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("power_changed"), PowerError, MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("cryo_power"), TEXT("Blackout"));
		Change.After->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
