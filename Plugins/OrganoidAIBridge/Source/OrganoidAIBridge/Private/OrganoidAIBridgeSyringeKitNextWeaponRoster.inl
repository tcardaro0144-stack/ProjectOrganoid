// Soft-link Syringe Kit → Weapon Roster. Does not save and does not change power.
	const TCHAR* SyringeKitNextWeaponRosterSpec = TEXT("syringe_kit_next_weapon_roster_v1");
	const TCHAR* SyringeKitNextWeaponRosterAction = TEXT("set_syringe_kit_next_weapon_roster");

	FString PreflightSetSyringeKitNextWeaponRoster(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_syringe_kit_next_weapon_roster does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), SyringeKitNextWeaponRosterSpec);
		if (!Spec.Equals(SyringeKitNextWeaponRosterSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), SyringeKitNextWeaponRosterSpec);
		}
		UObject* WeaponRoster = FindWeaponRosterMissionAssetExact();
		if (!WeaponRoster || !WeaponRosterMissionMismatchReason(WeaponRoster).IsEmpty())
		{
			return TEXT("DA_Mission_WeaponRoster missing or not exact. Create it before setting NextMissionAsset.");
		}
		const FString ConclusionNext = NeuroAdaptationConnectionNextRevelationReadNext(FindTheConclusionMissionAssetExact());
		if (!ConclusionNext.Equals(RespecBeatMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_TheConclusion.NextMissionAsset must stay DA_Mission_ResearchStation.");
		}
		const FString ResearchNext = NeuroAdaptationConnectionNextRevelationReadNext(FindRespecBeatMissionAssetExact());
		if (!ResearchNext.Equals(SyringeKitMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ResearchStation.NextMissionAsset must stay DA_Mission_SyringeKit.");
		}
		UObject* Syringe = FindSyringeKitMissionAssetExact();
		if (!Syringe)
		{
			return TEXT("DA_Mission_SyringeKit missing.");
		}
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Syringe);
		const bool bAlreadyExact = LiveNext.Equals(WeaponRosterMissionObjectPath, ESearchCase::CaseSensitive);
		if (!bAlreadyExact)
		{
			if (const FString SyringeMismatch = SyringeKitMissionMismatchReason(Syringe); !SyringeMismatch.IsEmpty())
			{
				return FString::Printf(TEXT("DA_Mission_SyringeKit mismatch: %s"), *SyringeMismatch);
			}
		}
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), SyringeKitNextWeaponRosterAction);
		Proposed->SetStringField(TEXT("spec"), SyringeKitNextWeaponRosterSpec);
		Proposed->SetStringField(TEXT("next_mission_asset"), WeaponRosterMissionObjectPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetSyringeKitNextWeaponRoster(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_syringe_kit_next_weapon_roster must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetSyringeKitNextWeaponRoster(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UObject* Syringe = FindSyringeKitMissionAssetExact();
		const FString LiveNext = NeuroAdaptationConnectionNextRevelationReadNext(Syringe);
		if (LiveNext.Equals(WeaponRosterMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("linked"), true);
			Change.After->SetStringField(TEXT("next_mission_asset"), WeaponRosterMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SyringeKitNextWeaponRoster", "Link Syringe Kit to the Weapon Roster"));
			FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(FindInstanceProperty(Syringe, TEXT("NextMissionAsset")));
			if (!SoftProp)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("NextMissionAsset missing."), MakeShared<FBridgeChange>(Change));
			}
			SoftProp->SetPropertyValue_InContainer(Syringe, FSoftObjectPtr(FSoftObjectPath(WeaponRosterMissionObjectPath)));
			Syringe->MarkPackageDirty();
		}
		const FString AfterNext = NeuroAdaptationConnectionNextRevelationReadNext(Syringe);
		if (!AfterNext.Equals(WeaponRosterMissionObjectPath, ESearchCase::CaseSensitive) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Syringe Kit NextMissionAsset did not become DA_Mission_WeaponRoster, or power changed."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetBoolField(TEXT("linked"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("next_mission_asset"), AfterNext);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
