// Soft-link Neuro Revelation → Cryo Access. Does not save and does not change power.
	const TCHAR* NeuroRevelationNextCryoAccessSpec = TEXT("neuro_revelation_next_cryo_access_v1");
	const TCHAR* NeuroRevelationNextCryoAccessAction = TEXT("set_neuro_revelation_next_cryo_access");

	FString NeuroRevelationNextCryoAccessRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroRevelationNextCryoAccessSpec);
		if (!Spec.Equals(NeuroRevelationNextCryoAccessSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroRevelationNextCryoAccessSpec);
		}
		return TEXT("");
	}

	FString ApplyNeuroRevelationNextCryoAccess(UObject* Asset)
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
		SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr(FSoftObjectPath(CryoAccessMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroRevelationNextCryoAccess(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_revelation_next_cryo_access does not save.");
		}
		if (const FString OverrideError = NeuroRevelationNextCryoAccessRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Revelation = FindNeuroRevelationMissionAssetExact();
		if (!Revelation)
		{
			return TEXT("DA_Mission_NeuroRevelation missing.");
		}
		if (const FString RevelationMismatch = NeuroRevelationMissionMismatchReason(Revelation); !RevelationMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroRevelation mismatch: %s"), *RevelationMismatch);
		}
		UObject* CryoAccess = FindCryoAccessMissionAssetExact();
		if (!CryoAccess)
		{
			return TEXT("DA_Mission_CryoAccess missing. Create it before setting NextMissionAsset.");
		}
		if (const FString CryoMismatch = CryoAccessMissionMismatchReason(CryoAccess); !CryoMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoAccess mismatch: %s"), *CryoMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Revelation);
		const bool bAlreadyExact = LiveNext.Equals(CryoAccessMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("spec"), NeuroRevelationNextCryoAccessSpec);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), NeuroRevelationNextCryoAccessSpec);
		Proposed->SetStringField(TEXT("action"), NeuroRevelationNextCryoAccessAction);
		Proposed->SetStringField(TEXT("next_mission_asset"), CryoAccessMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroRevelationNextCryoAccess(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_neuro_revelation_next_cryo_access must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroRevelationNextCryoAccess(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		UObject* Revelation = FindNeuroRevelationMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Revelation);
		if (LiveNext.Equals(CryoAccessMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), CryoAccessMissionObjectPath);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "RevelationNextCryoAccess", "Link Neuro Revelation to Cryo Access"));
			if (const FString ApplyError = ApplyNeuroRevelationNextCryoAccess(Revelation); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Revelation->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(Revelation);
		if (!AfterNext.Equals(CryoAccessMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Revelation NextMissionAsset did not become DA_Mission_CryoAccess."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("unlocks_cryo"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
