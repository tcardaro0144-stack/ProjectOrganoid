// Configure Checkpoint_InterfaceChamber as the Compute entry interact. Does not move, save, or change power.
	const TCHAR* ComputeEntryCheckpointSpec = TEXT("compute_entry_checkpoint_v1");
	const TCHAR* ComputeEntryCheckpointAction = TEXT("configure_compute_entry_checkpoint");
	const TCHAR* ComputeEntryCheckpointLabel = TEXT("Checkpoint_InterfaceChamber");
	const FVector ComputeEntryCheckpointLocation(-2425.f, -1650.f, -3540.f);
	const TCHAR* ComputeEntryCheckpointPrompt = TEXT("Enter Compute");
	const TCHAR* ComputeEntryCheckpointSpeaker = TEXT("Nathan");
	const TCHAR* ComputeEntryCheckpointLine = TEXT("The compute substrate is still running. It's been running the whole lockdown.");
	const double ComputeEntryCheckpointDuration = 7.0;

	bool ComputeEntryCheckpoint_IsFullyConfigured(AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignRequiredActiveObjectiveId"), ComputeEntryObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignSuccessEventId"), ComputeEntryEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignEntryPrompt"), ComputeEntryCheckpointPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), ComputeEntryCheckpointPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignNotificationSpeaker"), ComputeEntryCheckpointSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignNotificationText"), ComputeEntryCheckpointLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CampaignNotificationDurationSeconds"), ComputeEntryCheckpointDuration).IsEmpty();
	}

	FString ComputeEntryCheckpointRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), ComputeEntryCheckpointSpec);
		if (!Spec.Equals(ComputeEntryCheckpointSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ComputeEntryCheckpointSpec);
		}
		return TEXT("");
	}

	FString PreflightConfigureComputeEntryCheckpoint(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_compute_entry_checkpoint does not save.");
		}
		if (const FString OverrideError = ComputeEntryCheckpointRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
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
		UObject* Evidence = FindCryoEvidenceMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Evidence);
		if (!LiveNext.Equals(ComputeEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_CryoEvidence.NextMissionAsset must already be DA_Mission_ComputeEntry.");
		}
		UWorld* World = GetEditorWorld();
		const TArray<AActor*> Checkpoints = FindByExactLabel(World, ComputeEntryCheckpointLabel);
		if (Checkpoints.Num() != 1 || !Checkpoints[0])
		{
			return FString::Printf(TEXT("%s must be unique. count=%d."), ComputeEntryCheckpointLabel, Checkpoints.Num());
		}
		AActor* Checkpoint = Checkpoints[0];
		if (!PackagesEqual(ActorOwningPackage(Checkpoint), ComputePackage))
		{
			return TEXT("Checkpoint_InterfaceChamber must live on SL_Epitope_Compute.");
		}
		if (!Checkpoint->GetActorLocation().Equals(ComputeEntryCheckpointLocation, 1.f))
		{
			return TEXT("Checkpoint_InterfaceChamber moved. ZERO writes.");
		}
		const bool bAlreadyExact = ComputeEntryCheckpoint_IsFullyConfigured(Checkpoint);
		Before->SetStringField(TEXT("spec"), ComputeEntryCheckpointSpec);
		Before->SetStringField(TEXT("label"), ComputeEntryCheckpointLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("location"), Checkpoint->GetActorLocation().ToString());
		Proposed->SetStringField(TEXT("spec"), ComputeEntryCheckpointSpec);
		Proposed->SetStringField(TEXT("action"), ComputeEntryCheckpointAction);
		Proposed->SetStringField(TEXT("label"), ComputeEntryCheckpointLabel);
		Proposed->SetStringField(TEXT("location"), TEXT("X=-2425.000 Y=-1650.000 Z=-3540.000"));
		Proposed->SetStringField(TEXT("sector"), TEXT("Compute"));
		Proposed->SetStringField(TEXT("restored_state"), TEXT("Online"));
		Proposed->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Proposed->SetStringField(TEXT("required_objective"), ComputeEntryObjectiveId);
		Proposed->SetStringField(TEXT("prompt"), ComputeEntryCheckpointPrompt);
		Proposed->SetStringField(TEXT("event"), ComputeEntryEventId);
		Proposed->SetStringField(TEXT("line"), ComputeEntryCheckpointLine);
		Proposed->SetNumberField(TEXT("duration_seconds"), ComputeEntryCheckpointDuration);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureComputeEntryCheckpoint(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_compute_entry_checkpoint must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureComputeEntryCheckpoint(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		const TArray<AActor*> Checkpoints = FindByExactLabel(World, ComputeEntryCheckpointLabel);
		AActor* Checkpoint = Checkpoints.Num() == 1 ? Checkpoints[0] : nullptr;
		if (!Checkpoint)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Checkpoint_InterfaceChamber missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const FVector LocationBefore = Checkpoint->GetActorLocation();
		if (ComputeEntryCheckpoint_IsFullyConfigured(Checkpoint))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetStringField(TEXT("compute_power"), TEXT("Online"));
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ComputeEntryCheckpoint", "Configure Compute entry checkpoint"));
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
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignRequiredActiveObjectiveId"), ComputeEntryObjectiveId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignSuccessEventId"), ComputeEntryEventId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignEntryPrompt"), ComputeEntryCheckpointPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("InteractionPrompt"), ComputeEntryCheckpointPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignNotificationSpeaker"), ComputeEntryCheckpointSpeaker);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("CampaignNotificationText"), ComputeEntryCheckpointLine);
			if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("CampaignNotificationDurationSeconds"), ComputeEntryCheckpointDuration);
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Checkpoint->MarkPackageDirty();
		}
		if (!ComputeEntryCheckpoint_IsFullyConfigured(Checkpoint) || !Checkpoint->GetActorLocation().Equals(LocationBefore, 0.1f) || !Checkpoint->GetActorLocation().Equals(ComputeEntryCheckpointLocation, 1.f))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Checkpoint_InterfaceChamber was not configured in place."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetStringField(TEXT("compute_power"), TEXT("Online"));
		Change.After->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
