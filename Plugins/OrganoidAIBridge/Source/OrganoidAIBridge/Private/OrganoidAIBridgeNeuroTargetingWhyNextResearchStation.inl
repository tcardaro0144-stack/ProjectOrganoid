	// Soft-link NeuroTargetingWhy → Research Station intro — set_neuro_targeting_why_next_research_station.
	const TCHAR* NeuroTargetingWhyNextResearchStationSpec = TEXT("neuro_targeting_why_next_research_station_v1");
	const TCHAR* NeuroTargetingWhyNextResearchStationAction = TEXT("set_neuro_targeting_why_next_research_station");

	FString NeuroTargetingWhyNextResearchStationRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroTargetingWhyNextResearchStationSpec);
		if (!Spec.Equals(NeuroTargetingWhyNextResearchStationSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroTargetingWhyNextResearchStationSpec);
		}
		return TEXT("");
	}

	FString NeuroTargetingWhyNextResearchStationReadNext(UObject* Asset)
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

	FString ApplyNeuroTargetingWhyNextResearchStation(UObject* Asset)
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
			Asset, FSoftObjectPtr(FSoftObjectPath(NeuroResearchStationIntroMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroTargetingWhyNextResearchStation(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("set_neuro_targeting_why_next_research_station must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before setting NextMissionAsset.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_targeting_why_next_research_station does not save.");
		}
		if (const FString OverrideError = NeuroTargetingWhyNextResearchStationRejectClientOverrides(Args);
			!OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Targeting = FindNeuroTargetingWhyMissionAssetExact();
		if (!Targeting)
		{
			return TEXT("DA_Mission_NeuroTargetingWhy missing. Create it before setting NextMissionAsset.");
		}
		if (const FString TargetMismatch = NeuroTargetingWhyMissionMismatchReason(Targeting); !TargetMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroTargetingWhy mismatch: %s"), *TargetMismatch);
		}

		UObject* Intro = FindNeuroResearchStationIntroMissionAssetExact();
		if (!Intro)
		{
			return TEXT("DA_Mission_NeuroResearchStationIntro missing. Create it before setting NextMissionAsset.");
		}
		if (const FString IntroMismatch = NeuroResearchStationIntroMissionMismatchReason(Intro); !IntroMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroResearchStationIntro mismatch: %s"), *IntroMismatch);
		}

		const FString LiveNext = NeuroTargetingWhyNextResearchStationReadNext(Targeting);
		const bool bAlreadyExact = LiveNext.Equals(NeuroResearchStationIntroMissionObjectPath, ESearchCase::CaseSensitive);

		Before->SetStringField(TEXT("spec"), NeuroTargetingWhyNextResearchStationSpec);
		Before->SetStringField(TEXT("action"), NeuroTargetingWhyNextResearchStationAction);
		Before->SetStringField(TEXT("object_path"), NeuroTargetingWhyMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroTargetingWhyMissionPackage);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroTargetingWhyNextResearchStationSpec);
		Proposed->SetStringField(TEXT("action"), NeuroTargetingWhyNextResearchStationAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroTargetingWhyMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroTargetingWhyMissionPackage);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroResearchStationIntroMissionObjectPath);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroTargetingWhyNextResearchStation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("set_neuro_targeting_why_next_research_station must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroTargetingWhyNextResearchStation(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Targeting = FindNeuroTargetingWhyMissionAssetExact();
		if (!Targeting)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("DA_Mission_NeuroTargetingWhy missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		const FString LiveNext = NeuroTargetingWhyNextResearchStationReadNext(Targeting);
		if (LiveNext.Equals(NeuroResearchStationIntroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroResearchStationIntroMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SetNeuroTargetingWhyNextResearchStation",
				"Set TargetingWhy NextMissionAsset → Research Station Intro"));
			if (const FString ApplyError = ApplyNeuroTargetingWhyNextResearchStation(Targeting); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Targeting->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroTargetingWhyNextResearchStationReadNext(Targeting);
		if (!VerifyNext.Equals(NeuroResearchStationIntroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString TargetVerify = NeuroTargetingWhyMissionMismatchReason(Targeting); !TargetVerify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("TargetingWhy verify after next set failed: %s"), *TargetVerify),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("linked"));
		Change.After->SetStringField(TEXT("next_mission_asset"), NeuroResearchStationIntroMissionObjectPath);
		Change.After->SetBoolField(TEXT("mutated"), true);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
