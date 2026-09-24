// Configure PowerPanel_CryoBackup. Does not move the actor, save, or change live sector power.
	const TCHAR* CryoBackupPowerPanelSpec = TEXT("cryo_backup_power_panel_v1");
	const TCHAR* CryoBackupPowerPanelAction = TEXT("configure_cryo_backup_power_panel");
	const TCHAR* CryoBackupPowerPanelPrompt = TEXT("Engage Cryo Backup");
	const TCHAR* CryoBackupPowerPanelReviewPrompt = TEXT("Review Cryo power status");
	const TCHAR* CryoBackupPowerPanelSpeaker = TEXT("Nathan");
	const TCHAR* CryoBackupPowerPanelLine =
		TEXT("Cryo's backup came up. I can go in. I still don't know what they were keeping this cold.");
	const TCHAR* CryoBackupPowerPanelStatus = TEXT("CRYO SECTOR DARK — BACKUP NOT ENGAGED");
	const TCHAR* CryoBackupPowerPanelLogId = TEXT("Log_CryoBackupStatus");
	constexpr double CryoBackupPowerPanelDuration = 6.0;

	bool CryoBackupPowerPanel_IsFullyConfigured(AActor* Actor)
	{
		return Actor
			&& NeuroPowerFailureDiscovery_CheckEnum(Actor, TEXT("PowerSector"), TEXT("Cryo")).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckEnum(Actor, TEXT("RestoredState"), TEXT("Online")).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredActiveObjectiveId"), CryoAccessObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), CryoBackupPowerPanelPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ActiveRestorePrompt"), CryoBackupPowerPanelPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ReviewPrompt"), CryoBackupPowerPanelReviewPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SuccessObjectiveEventId"), CryoAccessEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RestoreSuccessNotificationSpeaker"), CryoBackupPowerPanelSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RestoreSuccessNotificationText"), CryoBackupPowerPanelLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("RestoreSuccessNotificationDurationSeconds"), CryoBackupPowerPanelDuration).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bDiscoverPowerFailureBeforeRestore"), true).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("DiscoveryObjectiveEventId"), TEXT("None")).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("FailureStatusReport"), CryoBackupPowerPanelStatus).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("FailureStatusLogEntryId"), CryoBackupPowerPanelLogId).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bHasBeenEngaged"), false).IsEmpty();
	}

	FString PreflightConfigureCryoBackupPowerPanel(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_cryo_backup_power_panel does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), CryoBackupPowerPanelSpec);
		if (!Spec.Equals(CryoBackupPowerPanelSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoBackupPowerPanelSpec);
		}
		UObject* CryoAccess = FindCryoAccessMissionAssetExact();
		if (!CryoAccess || !CryoAccessMissionMismatchReason(CryoAccess).IsEmpty())
		{
			return TEXT("DA_Mission_CryoAccess must exist and match the locked contract before panel configure.");
		}
		UObject* Revelation = FindNeuroRevelationMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Revelation);
		if (!Revelation || !LiveNext.Equals(CryoAccessMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_NeuroRevelation.NextMissionAsset must already point at DA_Mission_CryoAccess.");
		}
		UWorld* World = GetEditorWorld();
		TArray<AActor*> Panels = FindByExactLabel(World, CryoAccessPanelLabel);
		AActor* Panel = Panels.Num() == 1 ? Panels[0] : nullptr;
		if (!Panel)
		{
			return TEXT("PowerPanel_CryoBackup missing at preflight.");
		}
		const FVector Location = Panel->GetActorLocation();
		const bool bAlready = CryoBackupPowerPanel_IsFullyConfigured(Panel);
		Before->SetStringField(TEXT("spec"), CryoBackupPowerPanelSpec);
		Before->SetStringField(TEXT("label"), CryoAccessPanelLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlready);
		Before->SetArrayField(TEXT("location"), Vec(Location));
		Proposed->SetStringField(TEXT("spec"), CryoBackupPowerPanelSpec);
		Proposed->SetStringField(TEXT("action"), CryoBackupPowerPanelAction);
		Proposed->SetStringField(TEXT("label"), CryoAccessPanelLabel);
		Proposed->SetStringField(TEXT("PowerSector"), TEXT("Cryo"));
		Proposed->SetStringField(TEXT("RestoredState"), TEXT("Online"));
		Proposed->SetStringField(TEXT("RequiredActiveObjectiveId"), CryoAccessObjectiveId);
		Proposed->SetStringField(TEXT("InteractionPrompt"), CryoBackupPowerPanelPrompt);
		Proposed->SetStringField(TEXT("SuccessObjectiveEventId"), CryoAccessEventId);
		Proposed->SetStringField(TEXT("RestoreSuccessNotificationText"), CryoBackupPowerPanelLine);
		Proposed->SetNumberField(TEXT("RestoreSuccessNotificationDurationSeconds"), CryoBackupPowerPanelDuration);
		Proposed->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), true);
		Proposed->SetArrayField(TEXT("location"), Vec(Location));
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlready);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureCryoBackupPowerPanel(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_cryo_backup_power_panel must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureCryoBackupPowerPanel(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		UWorld* World = GetEditorWorld();
		TArray<AActor*> Panels = FindByExactLabel(World, CryoAccessPanelLabel);
		AActor* Panel = Panels.Num() == 1 ? Panels[0] : nullptr;
		if (!Panel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("PowerPanel_CryoBackup missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const FVector LocationBefore = Panel->GetActorLocation();
		if (CryoBackupPowerPanel_IsFullyConfigured(Panel))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), CryoAccessPanelLabel);
			Change.After->SetBoolField(TEXT("moves_actor"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoBackupPowerPanel", "Configure Cryo backup power panel"));
			auto ApplyString = [Panel](const TCHAR* Name, const TCHAR* Value) -> FString
			{
				return SetNamedPropertyFromString(Panel, Name, Value);
			};
			auto ApplyBool = [Panel](const TCHAR* Name, bool bValue) -> FString
			{
				FProperty* Prop = FindInstanceProperty(Panel, Name);
				if (!Prop)
				{
					return FString::Printf(TEXT("%s missing"), Name);
				}
				FString Error;
				if (!SetPropertyFromJson(Panel, Prop, MakeShared<FJsonValueBoolean>(bValue), Error))
				{
					return Error;
				}
				return TEXT("");
			};
			auto ApplyFloat = [Panel](const TCHAR* Name, double Value) -> FString
			{
				FProperty* Prop = FindInstanceProperty(Panel, Name);
				if (!Prop)
				{
					return FString::Printf(TEXT("%s missing"), Name);
				}
				FString Error;
				if (!SetPropertyFromJson(Panel, Prop, MakeShared<FJsonValueNumber>(Value), Error))
				{
					return Error;
				}
				return TEXT("");
			};
			FString WriteError = ApplyString(TEXT("PowerSector"), TEXT("Cryo"));
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("RestoredState"), TEXT("Online"));
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("RequiredActiveObjectiveId"), CryoAccessObjectiveId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("InteractionPrompt"), CryoBackupPowerPanelPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("ActiveRestorePrompt"), CryoBackupPowerPanelPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("ReviewPrompt"), CryoBackupPowerPanelReviewPrompt);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("SuccessObjectiveEventId"), CryoAccessEventId);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("RestoreSuccessNotificationSpeaker"), CryoBackupPowerPanelSpeaker);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("RestoreSuccessNotificationText"), CryoBackupPowerPanelLine);
			if (WriteError.IsEmpty()) WriteError = ApplyFloat(TEXT("RestoreSuccessNotificationDurationSeconds"), CryoBackupPowerPanelDuration);
			if (WriteError.IsEmpty()) WriteError = ApplyBool(TEXT("bDiscoverPowerFailureBeforeRestore"), true);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("DiscoveryObjectiveEventId"), TEXT("None"));
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("FailureStatusReport"), CryoBackupPowerPanelStatus);
			if (WriteError.IsEmpty()) WriteError = ApplyString(TEXT("FailureStatusLogEntryId"), CryoBackupPowerPanelLogId);
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Panel->MarkPackageDirty();
		}
		if (!Panel->GetActorLocation().Equals(LocationBefore, 0.1f))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("moved"), TEXT("PowerPanel_CryoBackup moved. ZERO further writes."), MakeShared<FBridgeChange>(Change));
		}
		if (!CryoBackupPowerPanel_IsFullyConfigured(Panel))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Post-configure verify failed for PowerPanel_CryoBackup."), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = CryoAccessRequirePowerContract(World); !PowerError.IsEmpty())
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
		Change.After->SetStringField(TEXT("label"), CryoAccessPanelLabel);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("unlocks_cryo"), false);
		Change.After->SetStringField(TEXT("cryo_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
