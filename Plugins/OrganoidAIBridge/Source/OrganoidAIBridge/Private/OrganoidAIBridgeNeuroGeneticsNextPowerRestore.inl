	// Soft-link NeuroGenetics → NeuroPowerRestore — set_neurogenetics_next_mission_power_restore.
	const TCHAR* NeuroGeneticsNextPowerRestoreSpec = TEXT("neurogenetics_next_mission_power_restore_v1");
	const TCHAR* NeuroGeneticsNextPowerRestoreAction = TEXT("set_neurogenetics_next_mission_power_restore");
	const TCHAR* NeuroGeneticsNextPowerRestoreSoftPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore");

	FString NeuroGeneticsNextPowerRestoreRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("next_mission"), TEXT("next_mission_asset"), TEXT("object_path"), TEXT("package"),
			TEXT("mission_id"), TEXT("tasks"), TEXT("path")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroGeneticsNextPowerRestoreSpec);
		if (!Spec.Equals(NeuroGeneticsNextPowerRestoreSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroGeneticsNextPowerRestoreSpec);
		}
		return TEXT("");
	}

	FString NeuroGeneticsNextPowerRestoreReadNext(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("");
		}
		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			return Soft.ToSoftObjectPath().IsValid() ? Soft.ToSoftObjectPath().ToString() : TEXT("");
		}
		return TEXT("");
	}

	FString ApplyNeuroGeneticsNextPowerRestore(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp);
		if (!SoftProp)
		{
			return TEXT("NextMissionAsset soft property missing.");
		}
		SoftProp->SetPropertyValue_InContainer(
			Asset, FSoftObjectPtr(FSoftObjectPath(NeuroGeneticsNextPowerRestoreSoftPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroGeneticsNextMissionPowerRestore(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("set_neurogenetics_next_mission_power_restore must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before setting NextMissionAsset.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neurogenetics_next_mission_power_restore does not save.");
		}
		if (const FString OverrideError = NeuroGeneticsNextPowerRestoreRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Neuro = FindNeuroGeneticsMissionAssetExact();
		if (!Neuro)
		{
			return TEXT("DA_Mission_NeuroGenetics missing. Require Beat 6 persisted first.");
		}
		const FString Beat6Mismatch = NeuroGeneticsMissionBeat6MismatchReason(Neuro);
		if (!Beat6Mismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroGenetics is not exact Beat 6: %s"), *Beat6Mismatch);
		}

		UObject* PowerRestore = FindNeuroPowerRestoreMissionAssetExact();
		if (!PowerRestore)
		{
			return TEXT("DA_Mission_NeuroPowerRestore missing. Create it before setting NextMissionAsset.");
		}
		if (const FString PowerMismatch = NeuroPowerRestoreMissionMismatchReason(PowerRestore); !PowerMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroPowerRestore mismatch: %s"), *PowerMismatch);
		}

		const FString LiveNext = NeuroGeneticsNextPowerRestoreReadNext(Neuro);
		const bool bAlreadyExact = LiveNext.Equals(NeuroGeneticsNextPowerRestoreSoftPath, ESearchCase::CaseSensitive);

		Before->SetStringField(TEXT("spec"), NeuroGeneticsNextPowerRestoreSpec);
		Before->SetStringField(TEXT("action"), NeuroGeneticsNextPowerRestoreAction);
		Before->SetStringField(TEXT("neuro_object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroGeneticsNextPowerRestoreSpec);
		Proposed->SetStringField(TEXT("action"), NeuroGeneticsNextPowerRestoreAction);
		Proposed->SetStringField(TEXT("neuro_object_path"), NeuroGeneticsMissionObjectPath);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroGeneticsNextPowerRestoreSoftPath);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroGeneticsNextMissionPowerRestore(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("set_neurogenetics_next_mission_power_restore must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroGeneticsNextMissionPowerRestore(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Neuro = FindNeuroGeneticsMissionAssetExact();
		if (!Neuro)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("DA_Mission_NeuroGenetics missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const FString LiveNext = NeuroGeneticsNextPowerRestoreReadNext(Neuro);
		if (LiveNext.Equals(NeuroGeneticsNextPowerRestoreSoftPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroGeneticsNextPowerRestoreSoftPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(
				NSLOCTEXT("OrganoidAIBridge", "SetNeuroGeneticsNextPowerRestore", "Set NeuroGenetics NextMissionAsset → PowerRestore"));
			if (const FString ApplyError = ApplyNeuroGeneticsNextPowerRestore(Neuro); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Neuro->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroGeneticsNextPowerRestoreReadNext(Neuro);
		if (!VerifyNext.Equals(NeuroGeneticsNextPowerRestoreSoftPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString Beat6Verify = NeuroGeneticsMissionBeat6MismatchReason(Neuro); !Beat6Verify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Beat 6 verify after next set failed: %s"), *Beat6Verify),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetStringField(TEXT("next_mission_asset"), NeuroGeneticsNextPowerRestoreSoftPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
