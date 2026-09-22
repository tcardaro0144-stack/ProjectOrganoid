	// Configure PowerPanel_NeuroBackup for Beat 7 restore — configure_neuro_backup_power_restore.
	const TCHAR* NeuroBackupPowerRestoreSpec = TEXT("neuro_backup_power_restore_v1");
	const TCHAR* NeuroBackupPowerRestoreAction = TEXT("configure_neuro_backup_power_restore");
	const TCHAR* NeuroBackupPowerRestoreActivePrompt = TEXT("Restore NeuroGenetics power");
	const TCHAR* NeuroBackupPowerRestoreReviewPrompt = TEXT("Review backup power status");
	const TCHAR* NeuroBackupPowerRestoreRequiredObjective = TEXT("Obj_RestoreNeuroLabPower");
	const TCHAR* NeuroBackupPowerRestoreSpeaker = TEXT("Nathan");
	const TCHAR* NeuroBackupPowerRestoreLine =
		TEXT("NeuroGenetics is back online. Cryo is still dark, but I can work with this.");
	constexpr double NeuroBackupPowerRestoreDuration = 6.0;

	FString NeuroBackupPowerRestore_CheckFloat(AActor* Actor, const TCHAR* PropertyName, double Expected, double Tol = 0.01)
	{
		FProperty* Prop = FindInstanceProperty(Actor, PropertyName);
		if (!Prop)
		{
			return FString::Printf(TEXT("%s missing"), PropertyName);
		}
		FString Error;
		if (!PropertyMatchesJson(Actor, Prop, MakeShared<FJsonValueNumber>(Expected), Error))
		{
			// Float may serialize differently — read directly when numeric.
			if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
			{
				const float Live = FloatProp->GetPropertyValue_InContainer(Actor);
				if (!FMath::IsNearlyEqual(static_cast<double>(Live), Expected, Tol))
				{
					return FString::Printf(TEXT("%s expected %g got %g"), PropertyName, Expected, Live);
				}
				return TEXT("");
			}
			if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
			{
				const double Live = DoubleProp->GetPropertyValue_InContainer(Actor);
				if (!FMath::IsNearlyEqual(Live, Expected, Tol))
				{
					return FString::Printf(TEXT("%s expected %g got %g"), PropertyName, Expected, Live);
				}
				return TEXT("");
			}
			return FString::Printf(TEXT("%s: %s"), PropertyName, *Error);
		}
		return TEXT("");
	}

	bool NeuroBackupPowerRestore_IsFullyConfigured(AActor* Actor)
	{
		return NeuroPowerFailureDiscovery_PanelIdentity(Actor).IsEmpty()
			&& NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(Actor).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredActiveObjectiveId"), NeuroBackupPowerRestoreRequiredObjective).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ActiveRestorePrompt"), NeuroBackupPowerRestoreActivePrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ReviewPrompt"), NeuroBackupPowerRestoreReviewPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RestoreSuccessNotificationSpeaker"), NeuroBackupPowerRestoreSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RestoreSuccessNotificationText"), NeuroBackupPowerRestoreLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("RestoreSuccessNotificationDurationSeconds"), NeuroBackupPowerRestoreDuration).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SuccessObjectiveEventId"), NeuroPowerFailureRestoreEvent).IsEmpty();
	}

	FString PreflightConfigureNeuroBackupPowerRestore(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("configure_neuro_backup_power_restore must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before configuring the backup panel.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_neuro_backup_power_restore does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroBackupPowerRestoreSpec);
		if (!Spec.Equals(NeuroBackupPowerRestoreSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroBackupPowerRestoreSpec);
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			return TEXT("Editor world missing.");
		}
		if (const FString KeepError = NeuroPowerFailureDiscovery_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}
		if (const FString PowerError = NeuroPowerFailureDiscovery_RequirePowerSeed(World); !PowerError.IsEmpty())
		{
			return PowerError;
		}

		UObject* PowerMission = FindNeuroPowerRestoreMissionAssetExact();
		if (!PowerMission || !NeuroPowerRestoreMissionMismatchReason(PowerMission).IsEmpty())
		{
			return TEXT("DA_Mission_NeuroPowerRestore must exist and match locked contract before panel configure.");
		}
		UObject* NeuroMission = FindNeuroGeneticsMissionAssetExact();
		if (!NeuroMission
			|| !NeuroGeneticsNextPowerRestoreReadNext(NeuroMission).Equals(
				NeuroGeneticsNextPowerRestoreSoftPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_NeuroGenetics.NextMissionAsset must already point at DA_Mission_NeuroPowerRestore.");
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerFailurePanelLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), NeuroPowerFailurePanelLabel, Matches.Num());
		}
		AActor* Panel = Matches[0];
		if (const FString Identity = NeuroPowerFailureDiscovery_PanelIdentity(Panel); !Identity.IsEmpty())
		{
			return Identity;
		}
		if (const FString Wired = NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(Panel); !Wired.IsEmpty())
		{
			return Wired;
		}

		const bool bAlreadyConfigured = NeuroBackupPowerRestore_IsFullyConfigured(Panel);
		Before->SetStringField(TEXT("spec"), NeuroBackupPowerRestoreSpec);
		Before->SetStringField(TEXT("label"), NeuroPowerFailurePanelLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyConfigured);
		Before->SetStringField(TEXT("neuro_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2));
		Before->SetStringField(TEXT("cryo_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3));

		Proposed->SetStringField(TEXT("spec"), NeuroBackupPowerRestoreSpec);
		Proposed->SetStringField(TEXT("action"), NeuroBackupPowerRestoreAction);
		Proposed->SetStringField(TEXT("label"), NeuroPowerFailurePanelLabel);
		Proposed->SetStringField(TEXT("RequiredActiveObjectiveId"), NeuroBackupPowerRestoreRequiredObjective);
		Proposed->SetStringField(TEXT("ActiveRestorePrompt"), NeuroBackupPowerRestoreActivePrompt);
		Proposed->SetStringField(TEXT("ReviewPrompt"), NeuroBackupPowerRestoreReviewPrompt);
		Proposed->SetStringField(TEXT("RestoreSuccessNotificationSpeaker"), NeuroBackupPowerRestoreSpeaker);
		Proposed->SetStringField(TEXT("RestoreSuccessNotificationText"), NeuroBackupPowerRestoreLine);
		Proposed->SetNumberField(TEXT("RestoreSuccessNotificationDurationSeconds"), NeuroBackupPowerRestoreDuration);
		Proposed->SetStringField(TEXT("SuccessObjectiveEventId"), NeuroPowerFailureRestoreEvent);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyConfigured);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyConfigured);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroBackupPowerRestore(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("configure_neuro_backup_power_restore must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroBackupPowerRestore(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerFailurePanelLabel);
		AActor* Panel = Matches.Num() == 1 ? Matches[0] : nullptr;
		if (!Panel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("PowerPanel_NeuroBackup missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		if (NeuroBackupPowerRestore_IsFullyConfigured(Panel))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), NeuroPowerFailurePanelLabel);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(
				NSLOCTEXT("OrganoidAIBridge", "NeuroBackupPowerRestore", "Configure Neuro backup power restore panel"));

			auto ApplyString = [Panel](const TCHAR* Name, const TCHAR* Value) -> FString
			{
				return SetNamedPropertyFromString(Panel, Name, Value);
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

			FString WriteError;
			WriteError = ApplyString(TEXT("RequiredActiveObjectiveId"), NeuroBackupPowerRestoreRequiredObjective);
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("ActiveRestorePrompt"), NeuroBackupPowerRestoreActivePrompt);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("ReviewPrompt"), NeuroBackupPowerRestoreReviewPrompt);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("RestoreSuccessNotificationSpeaker"), NeuroBackupPowerRestoreSpeaker);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("RestoreSuccessNotificationText"), NeuroBackupPowerRestoreLine);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyFloat(TEXT("RestoreSuccessNotificationDurationSeconds"), NeuroBackupPowerRestoreDuration);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("SuccessObjectiveEventId"), NeuroPowerFailureRestoreEvent);
			}
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}

			Panel->MarkPackageDirty();
		}

		if (!NeuroBackupPowerRestore_IsFullyConfigured(Panel))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				TEXT("Post-configure verify failed for PowerPanel_NeuroBackup."),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = NeuroPowerFailureDiscovery_RequirePowerSeed(World); !PowerError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("power_seed"), PowerError, MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetStringField(TEXT("label"), NeuroPowerFailurePanelLabel);
		Change.After->SetStringField(TEXT("RequiredActiveObjectiveId"), NeuroBackupPowerRestoreRequiredObjective);
		Change.After->SetStringField(TEXT("ActiveRestorePrompt"), NeuroBackupPowerRestoreActivePrompt);
		Change.After->SetStringField(TEXT("ReviewPrompt"), NeuroBackupPowerRestoreReviewPrompt);
		Change.After->SetStringField(TEXT("neuro_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2));
		Change.After->SetStringField(TEXT("cryo_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
