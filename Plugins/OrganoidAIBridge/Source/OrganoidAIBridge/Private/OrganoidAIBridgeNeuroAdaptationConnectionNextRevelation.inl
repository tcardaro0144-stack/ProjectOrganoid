// Soft-link Adaptation Connection → Neuro Revelation. Does not save and does not change power.
	const TCHAR* NeuroAdaptationConnectionNextRevelationSpec = TEXT("neuro_adaptation_connection_next_revelation_v1");
	const TCHAR* NeuroAdaptationConnectionNextRevelationAction = TEXT("set_neuro_adaptation_connection_next_revelation");

	FString NeuroAdaptationConnectionNextRevelationRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroAdaptationConnectionNextRevelationSpec);
		if (!Spec.Equals(NeuroAdaptationConnectionNextRevelationSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroAdaptationConnectionNextRevelationSpec);
		}
		return TEXT("");
	}

	FString NeuroAdaptationConnectionNextRevelationReadNext(UObject* Asset)
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

	FString ApplyNeuroAdaptationConnectionNextRevelation(UObject* Asset)
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
		SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr(FSoftObjectPath(NeuroRevelationMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroAdaptationConnectionNextRevelation(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = NeuroRevelationRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_adaptation_connection_next_revelation does not save.");
		}
		if (const FString OverrideError = NeuroAdaptationConnectionNextRevelationRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Connection = FindNeuroAdaptationConnectionMissionAssetExact();
		if (!Connection)
		{
			return TEXT("DA_Mission_NeuroAdaptationConnection missing.");
		}
		if (const FString ConnectionMismatch = NeuroAdaptationConnectionMissionMismatchReason(Connection); !ConnectionMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroAdaptationConnection mismatch: %s"), *ConnectionMismatch);
		}
		UObject* Revelation = FindNeuroRevelationMissionAssetExact();
		if (!Revelation)
		{
			return TEXT("DA_Mission_NeuroRevelation missing. Create it before setting NextMissionAsset.");
		}
		if (const FString RevelationMismatch = NeuroRevelationMissionMismatchReason(Revelation); !RevelationMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroRevelation mismatch: %s"), *RevelationMismatch);
		}

		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Connection);
		const bool bAlreadyExact = LiveNext.Equals(NeuroRevelationMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("spec"), NeuroAdaptationConnectionNextRevelationSpec);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), NeuroAdaptationConnectionNextRevelationSpec);
		Proposed->SetStringField(TEXT("action"), NeuroAdaptationConnectionNextRevelationAction);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroRevelationMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroAdaptationConnectionNextRevelation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_neuro_adaptation_connection_next_revelation must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroAdaptationConnectionNextRevelation(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UObject* Connection = FindNeuroAdaptationConnectionMissionAssetExact();
		if (!Connection)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Adaptation connection mission missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Connection);
		if (LiveNext.Equals(NeuroRevelationMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroRevelationMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SetNeuroAdaptationConnectionNextRevelation",
				"Set Adaptation Connection NextMissionAsset to Neuro Revelation"));
			if (const FString ApplyError = ApplyNeuroAdaptationConnectionNextRevelation(Connection); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Connection->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroAdaptationConnectionNextRevelationReadNext(Connection);
		if (!VerifyNext.Equals(NeuroRevelationMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext), MakeShared<FBridgeChange>(Change));
		}
		if (const FString ConnectionVerify = NeuroAdaptationConnectionMissionMismatchReason(Connection); !ConnectionVerify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), FString::Printf(TEXT("Connection mission verify after next set failed: %s"), *ConnectionVerify), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
