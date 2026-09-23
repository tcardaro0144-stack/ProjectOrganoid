	// Soft-link NeuroPowerRestore → NeuroTargetingWhy — set_neuro_power_restore_next_targeting_why.
	const TCHAR* NeuroPowerRestoreNextTargetingWhySpec = TEXT("neuro_power_restore_next_targeting_why_v1");
	const TCHAR* NeuroPowerRestoreNextTargetingWhyAction = TEXT("set_neuro_power_restore_next_targeting_why");
	const TCHAR* NeuroPowerRestoreNextTargetingWhySoftPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroTargetingWhy.DA_Mission_NeuroTargetingWhy");
	const TCHAR* NeuroPowerRestoreMissionObjectPathForNext =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore");
	const TCHAR* NeuroPowerRestoreMissionPackageForNext = TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore");

	FString NeuroPowerRestoreNextTargetingWhyRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroPowerRestoreNextTargetingWhySpec);
		if (!Spec.Equals(NeuroPowerRestoreNextTargetingWhySpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroPowerRestoreNextTargetingWhySpec);
		}
		return TEXT("");
	}

	FString NeuroPowerRestoreNextTargetingWhyReadNext(UObject* Asset)
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

	FString ApplyNeuroPowerRestoreNextTargetingWhy(UObject* Asset)
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
			Asset, FSoftObjectPtr(FSoftObjectPath(NeuroPowerRestoreNextTargetingWhySoftPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroPowerRestoreNextTargetingWhy(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("set_neuro_power_restore_next_targeting_why must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before setting NextMissionAsset.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_power_restore_next_targeting_why does not save.");
		}
		if (const FString OverrideError = NeuroPowerRestoreNextTargetingWhyRejectClientOverrides(Args);
			!OverrideError.IsEmpty())
		{
			return OverrideError;
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

		UObject* TargetingWhy = FindNeuroTargetingWhyMissionAssetExact();
		if (!TargetingWhy)
		{
			return TEXT("DA_Mission_NeuroTargetingWhy missing. Create it before setting NextMissionAsset.");
		}
		if (const FString TargetMismatch = NeuroTargetingWhyMissionMismatchReason(TargetingWhy); !TargetMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroTargetingWhy mismatch: %s"), *TargetMismatch);
		}

		const FString LiveNext = NeuroPowerRestoreNextTargetingWhyReadNext(PowerRestore);
		const bool bAlreadyExact = LiveNext.Equals(NeuroPowerRestoreNextTargetingWhySoftPath, ESearchCase::CaseSensitive);

		Before->SetStringField(TEXT("spec"), NeuroPowerRestoreNextTargetingWhySpec);
		Before->SetStringField(TEXT("action"), NeuroPowerRestoreNextTargetingWhyAction);
		Before->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPathForNext);
		Before->SetStringField(TEXT("package"), NeuroPowerRestoreMissionPackageForNext);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroPowerRestoreNextTargetingWhySpec);
		Proposed->SetStringField(TEXT("action"), NeuroPowerRestoreNextTargetingWhyAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroPowerRestoreMissionObjectPathForNext);
		Proposed->SetStringField(TEXT("package"), NeuroPowerRestoreMissionPackageForNext);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroPowerRestoreNextTargetingWhySoftPath);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroPowerRestoreNextTargetingWhy(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("set_neuro_power_restore_next_targeting_why must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroPowerRestoreNextTargetingWhy(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* PowerRestore = FindNeuroPowerRestoreMissionAssetExact();
		if (!PowerRestore)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("DA_Mission_NeuroPowerRestore missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const FString LiveNext = NeuroPowerRestoreNextTargetingWhyReadNext(PowerRestore);
		if (LiveNext.Equals(NeuroPowerRestoreNextTargetingWhySoftPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroPowerRestoreNextTargetingWhySoftPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SetNeuroPowerRestoreNextTargetingWhy",
				"Set PowerRestore NextMissionAsset → TargetingWhy"));
			if (const FString ApplyError = ApplyNeuroPowerRestoreNextTargetingWhy(PowerRestore); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			PowerRestore->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroPowerRestoreNextTargetingWhyReadNext(PowerRestore);
		if (!VerifyNext.Equals(NeuroPowerRestoreNextTargetingWhySoftPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerVerify = NeuroPowerRestoreMissionMismatchReason(PowerRestore); !PowerVerify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("PowerRestore verify after next set failed: %s"), *PowerVerify),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetStringField(TEXT("next_mission_asset"), NeuroPowerRestoreNextTargetingWhySoftPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
