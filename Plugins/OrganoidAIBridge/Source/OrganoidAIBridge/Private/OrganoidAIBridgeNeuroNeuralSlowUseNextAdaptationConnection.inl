	// Soft-link Neural Slow use → live adaptation connection.
	const TCHAR* NeuroNeuralSlowUseNextAdaptationConnectionSpec =
		TEXT("neuro_neural_slow_use_next_adaptation_connection_v1");
	const TCHAR* NeuroNeuralSlowUseNextAdaptationConnectionAction =
		TEXT("set_neuro_neural_slow_use_next_adaptation_connection");

	FString NeuroNeuralSlowUseNextAdaptationConnectionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroNeuralSlowUseNextAdaptationConnectionSpec);
		if (!Spec.Equals(NeuroNeuralSlowUseNextAdaptationConnectionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroNeuralSlowUseNextAdaptationConnectionSpec);
		}
		return TEXT("");
	}

	FString NeuroNeuralSlowUseNextAdaptationConnectionReadNext(UObject* Asset)
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

	FString ApplyNeuroNeuralSlowUseNextAdaptationConnection(UObject* Asset)
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
			Asset, FSoftObjectPtr(FSoftObjectPath(NeuroAdaptationConnectionMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroNeuralSlowUseNextAdaptationConnection(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("set_neuro_neural_slow_use_next_adaptation_connection must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before setting NextMissionAsset.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_neural_slow_use_next_adaptation_connection does not save.");
		}
		if (const FString OverrideError = NeuroNeuralSlowUseNextAdaptationConnectionRejectClientOverrides(Args);
			!OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* UseMission = FindNeuroNeuralSlowUseMissionAssetExact();
		if (!UseMission)
		{
			return TEXT("DA_Mission_NeuroNeuralSlowUse missing.");
		}
		if (const FString UseMismatch = NeuroNeuralSlowUseMissionMismatchReason(UseMission); !UseMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroNeuralSlowUse mismatch: %s"), *UseMismatch);
		}

		UObject* Connection = FindNeuroAdaptationConnectionMissionAssetExact();
		if (!Connection)
		{
			return TEXT("DA_Mission_NeuroAdaptationConnection missing. Create it before setting NextMissionAsset.");
		}
		if (const FString ConnectionMismatch = NeuroAdaptationConnectionMissionMismatchReason(Connection);
			!ConnectionMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroAdaptationConnection mismatch: %s"), *ConnectionMismatch);
		}

		const FString LiveNext = NeuroNeuralSlowUseNextAdaptationConnectionReadNext(UseMission);
		const bool bAlreadyExact =
			LiveNext.Equals(NeuroAdaptationConnectionMissionObjectPath, ESearchCase::CaseSensitive);

		Before->SetStringField(TEXT("spec"), NeuroNeuralSlowUseNextAdaptationConnectionSpec);
		Before->SetStringField(TEXT("action"), NeuroNeuralSlowUseNextAdaptationConnectionAction);
		Before->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroNeuralSlowUseMissionPackage);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroNeuralSlowUseNextAdaptationConnectionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroNeuralSlowUseNextAdaptationConnectionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroNeuralSlowUseMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroNeuralSlowUseMissionPackage);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroAdaptationConnectionMissionObjectPath);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroNeuralSlowUseNextAdaptationConnection(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("set_neuro_neural_slow_use_next_adaptation_connection must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError =
			PreflightSetNeuroNeuralSlowUseNextAdaptationConnection(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* UseMission = FindNeuroNeuralSlowUseMissionAssetExact();
		if (!UseMission)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("Neural Slow use mission missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const FString LiveNext = NeuroNeuralSlowUseNextAdaptationConnectionReadNext(UseMission);
		if (LiveNext.Equals(NeuroAdaptationConnectionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroAdaptationConnectionMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SetNeuroNeuralSlowUseNextAdaptationConnection",
				"Set Neural Slow Use NextMissionAsset to Adaptation Connection"));
			if (const FString ApplyError = ApplyNeuroNeuralSlowUseNextAdaptationConnection(UseMission); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			UseMission->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroNeuralSlowUseNextAdaptationConnectionReadNext(UseMission);
		if (!VerifyNext.Equals(NeuroAdaptationConnectionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString UseVerify = NeuroNeuralSlowUseMissionMismatchReason(UseMission); !UseVerify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Use mission verify after next set failed: %s"), *UseVerify),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("next_mission_asset"), VerifyNext);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
