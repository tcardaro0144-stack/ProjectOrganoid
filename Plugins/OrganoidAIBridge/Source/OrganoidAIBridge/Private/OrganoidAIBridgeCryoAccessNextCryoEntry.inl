// Soft-link Cryo Access → Cryo Entry. Does not save and does not change power.
	const TCHAR* CryoAccessNextCryoEntrySpec = TEXT("cryo_access_next_cryo_entry_v1");
	const TCHAR* CryoAccessNextCryoEntryAction = TEXT("set_cryo_access_next_cryo_entry");

	FString CryoAccessNextCryoEntryRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), CryoAccessNextCryoEntrySpec);
		if (!Spec.Equals(CryoAccessNextCryoEntrySpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), CryoAccessNextCryoEntrySpec);
		}
		return TEXT("");
	}

	FString ApplyCryoAccessNextCryoEntry(UObject* Asset)
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
		SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr(FSoftObjectPath(CryoEntryMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetCryoAccessNextCryoEntry(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_cryo_access_next_cryo_entry does not save.");
		}
		if (const FString OverrideError = CryoAccessNextCryoEntryRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* CryoAccess = FindCryoAccessMissionAssetExact();
		if (!CryoAccess)
		{
			return TEXT("DA_Mission_CryoAccess missing.");
		}
		if (const FString CryoMismatch = CryoAccessMissionMismatchReason(CryoAccess); !CryoMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoAccess mismatch: %s"), *CryoMismatch);
		}
		UObject* CryoEntry = FindCryoEntryMissionAssetExact();
		if (!CryoEntry)
		{
			return TEXT("DA_Mission_CryoEntry missing. Create it before setting NextMissionAsset.");
		}
		if (const FString EntryMismatch = CryoEntryMissionMismatchReason(CryoEntry); !EntryMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_CryoEntry mismatch: %s"), *EntryMismatch);
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoAccess);
		const bool bAlreadyExact = LiveNext.Equals(CryoEntryMissionObjectPath, ESearchCase::CaseSensitive);
		Before->SetStringField(TEXT("spec"), CryoAccessNextCryoEntrySpec);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), CryoAccessNextCryoEntrySpec);
		Proposed->SetStringField(TEXT("action"), CryoAccessNextCryoEntryAction);
		Proposed->SetStringField(TEXT("next_mission_asset"), CryoEntryMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetCryoAccessNextCryoEntry(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_cryo_access_next_cryo_entry must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetCryoAccessNextCryoEntry(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* CryoAccess = FindCryoAccessMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoAccess);
		if (LiveNext.Equals(CryoEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), CryoEntryMissionObjectPath);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoAccessNextCryoEntry", "Link Cryo Access to Cryo Entry"));
			if (const FString ApplyError = ApplyCryoAccessNextCryoEntry(CryoAccess); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			CryoAccess->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(CryoAccess);
		if (!AfterNext.Equals(CryoEntryMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Cryo Access NextMissionAsset did not become DA_Mission_CryoEntry."), MakeShared<FBridgeChange>(Change));
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
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
