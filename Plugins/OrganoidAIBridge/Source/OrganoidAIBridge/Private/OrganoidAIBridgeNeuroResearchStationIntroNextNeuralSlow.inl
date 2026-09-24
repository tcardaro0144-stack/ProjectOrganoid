	// Soft-link Research Station intro → Neural Slow use — set_neuro_research_station_intro_next_neural_slow.
	const TCHAR* NeuroResearchStationIntroNextNeuralSlowSpec = TEXT("neuro_research_station_intro_next_neural_slow_v1");
	const TCHAR* NeuroResearchStationIntroNextNeuralSlowAction = TEXT("set_neuro_research_station_intro_next_neural_slow");

	FString NeuroResearchStationIntroNextNeuralSlowRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroResearchStationIntroNextNeuralSlowSpec);
		if (!Spec.Equals(NeuroResearchStationIntroNextNeuralSlowSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroResearchStationIntroNextNeuralSlowSpec);
		}
		return TEXT("");
	}

	FString NeuroResearchStationIntroNextNeuralSlowReadNext(UObject* Asset)
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

	FString ApplyNeuroResearchStationIntroNextNeuralSlow(UObject* Asset)
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
			Asset, FSoftObjectPtr(FSoftObjectPath(NeuroNeuralSlowUseMissionObjectPath)));
		return TEXT("");
	}

	FString PreflightSetNeuroResearchStationIntroNextNeuralSlow(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("set_neuro_research_station_intro_next_neural_slow must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before setting NextMissionAsset.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. set_neuro_research_station_intro_next_neural_slow does not save.");
		}
		if (const FString OverrideError = NeuroResearchStationIntroNextNeuralSlowRejectClientOverrides(Args);
			!OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		UObject* Intro = FindNeuroResearchStationIntroMissionAssetExact();
		if (!Intro)
		{
			return TEXT("DA_Mission_NeuroResearchStationIntro missing.");
		}
		if (const FString IntroMismatch = NeuroResearchStationIntroMissionMismatchReason(Intro); !IntroMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroResearchStationIntro mismatch: %s"), *IntroMismatch);
		}

		UObject* UseMission = FindNeuroNeuralSlowUseMissionAssetExact();
		if (!UseMission)
		{
			return TEXT("DA_Mission_NeuroNeuralSlowUse missing. Create it before setting NextMissionAsset.");
		}
		if (const FString UseMismatch = NeuroNeuralSlowUseMissionMismatchReason(UseMission); !UseMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_NeuroNeuralSlowUse mismatch: %s"), *UseMismatch);
		}

		const FString LiveNext = NeuroResearchStationIntroNextNeuralSlowReadNext(Intro);
		const bool bAlreadyExact = LiveNext.Equals(NeuroNeuralSlowUseMissionObjectPath, ESearchCase::CaseSensitive);

		Before->SetStringField(TEXT("spec"), NeuroResearchStationIntroNextNeuralSlowSpec);
		Before->SetStringField(TEXT("action"), NeuroResearchStationIntroNextNeuralSlowAction);
		Before->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroResearchStationIntroMissionPackage);
		Before->SetStringField(TEXT("next_mission_asset"), LiveNext);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroResearchStationIntroNextNeuralSlowSpec);
		Proposed->SetStringField(TEXT("action"), NeuroResearchStationIntroNextNeuralSlowAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroResearchStationIntroMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroResearchStationIntroMissionPackage);
		Proposed->SetStringField(TEXT("next_mission_asset"), NeuroNeuralSlowUseMissionObjectPath);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetNeuroResearchStationIntroNextNeuralSlow(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("set_neuro_research_station_intro_next_neural_slow must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetNeuroResearchStationIntroNextNeuralSlow(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Intro = FindNeuroResearchStationIntroMissionAssetExact();
		if (!Intro)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Intro mission missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FString LiveNext = NeuroResearchStationIntroNextNeuralSlowReadNext(Intro);
		if (LiveNext.Equals(NeuroNeuralSlowUseMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("next_mission_asset"), NeuroNeuralSlowUseMissionObjectPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"SetNeuroResearchStationIntroNextNeuralSlow",
				"Set Research Station Intro NextMissionAsset to Neural Slow Use"));
			if (const FString ApplyError = ApplyNeuroResearchStationIntroNextNeuralSlow(Intro); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Intro->MarkPackageDirty();
		}

		const FString VerifyNext = NeuroResearchStationIntroNextNeuralSlowReadNext(Intro);
		if (!VerifyNext.Equals(NeuroNeuralSlowUseMissionObjectPath, ESearchCase::CaseSensitive))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-apply NextMissionAsset verify failed: '%s'"), *VerifyNext),
				MakeShared<FBridgeChange>(Change));
		}
		if (const FString IntroVerify = NeuroResearchStationIntroMissionMismatchReason(Intro); !IntroVerify.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Intro verify after next set failed: %s"), *IntroVerify),
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
