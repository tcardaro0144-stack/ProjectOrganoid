	// Configure ResearchStation_NeuroGenetics campaign contract — configure_neuro_research_station_intro.
	const TCHAR* NeuroResearchStationIntroSpec = TEXT("neuro_research_station_intro_v1");
	const TCHAR* NeuroResearchStationIntroAction = TEXT("configure_neuro_research_station_intro");
	const TCHAR* NeuroResearchStationIntroLabel = TEXT("ResearchStation_NeuroGenetics");
	const TCHAR* NeuroResearchStationIntroPrompt = TEXT("Use Research Station");
	const TCHAR* NeuroResearchStationIntroAdaptationPath =
		TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");
	const TCHAR* NeuroResearchStationIntroSpeaker = TEXT("Nathan");
	const TCHAR* NeuroResearchStationIntroLine =
		TEXT("Neural Slow is mounted. Research Stations can swap unlocked adaptations without spending SOT.");
	constexpr double NeuroResearchStationIntroDuration = 7.0;
	const TCHAR* NeuroResearchStationIntroClassName = TEXT("ProjectOrganoidResearchStation");

	FString NeuroResearchStationIntro_ReadSoft(AActor* Actor, const TCHAR* PropertyName)
	{
		FProperty* Prop = FindInstanceProperty(Actor, PropertyName);
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Actor);
			return Soft.ToSoftObjectPath().IsValid() ? Soft.ToSoftObjectPath().ToString() : TEXT("");
		}
		return TEXT("");
	}

	FString NeuroResearchStationIntro_MismatchReason(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("ResearchStation_NeuroGenetics missing.");
		}
		if (!Actor->GetActorLabel().Equals(NeuroResearchStationIntroLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' is not ResearchStation_NeuroGenetics"), *Actor->GetActorLabel());
		}
		if (!Actor->GetClass() || !Actor->GetClass()->GetName().Equals(NeuroResearchStationIntroClassName))
		{
			return TEXT("class is not AProjectOrganoidResearchStation.");
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("InteractionPrompt"), NeuroResearchStationIntroPrompt);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("CampaignRequiredActiveObjectiveId"), NeuroResearchStationIntroObjectiveId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (!NeuroResearchStationIntro_ReadSoft(Actor, TEXT("CampaignUnlockAdaptation"))
				.Equals(NeuroResearchStationIntroAdaptationPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(
				TEXT("CampaignUnlockAdaptation expected %s got %s"),
				NeuroResearchStationIntroAdaptationPath,
				*NeuroResearchStationIntro_ReadSoft(Actor, TEXT("CampaignUnlockAdaptation")));
		}
		if (!NeuroResearchStationIntro_ReadSoft(Actor, TEXT("CampaignCreditAdaptation"))
				.Equals(NeuroResearchStationIntroAdaptationPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(
				TEXT("CampaignCreditAdaptation expected %s got %s"),
				NeuroResearchStationIntroAdaptationPath,
				*NeuroResearchStationIntro_ReadSoft(Actor, TEXT("CampaignCreditAdaptation")));
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("CampaignSuccessObjectiveEventId"), NeuroResearchStationIntroEventId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("CampaignReplayGuardObjectiveId"), NeuroResearchStationIntroObjectiveId);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("CampaignSuccessNotificationSpeaker"), NeuroResearchStationIntroSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("CampaignSuccessNotificationText"), NeuroResearchStationIntroLine);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckFloat(
				Actor, TEXT("CampaignSuccessNotificationDurationSeconds"), NeuroResearchStationIntroDuration);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString NeuroResearchStationIntro_RequireNeuroPackage(AActor* Actor)
	{
		if (!Actor || !Actor->GetLevel() || !Actor->GetLevel()->GetOutermost())
		{
			return TEXT("ResearchStation_NeuroGenetics level/package missing.");
		}
		const FString PackageName = Actor->GetLevel()->GetOutermost()->GetName();
		if (!PackagesEqual(PackageName, TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics")))
		{
			return FString::Printf(
				TEXT("ResearchStation_NeuroGenetics owning package '%s' is not SL_Epitope_NeuroGenetics."),
				*PackageName);
		}
		return TEXT("");
	}

	FString NeuroResearchStationIntro_WriteSoft(AActor* Actor, const TCHAR* PropertyName, const TCHAR* Path)
	{
		FProperty* Prop = FindInstanceProperty(Actor, PropertyName);
		FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Prop);
		if (!SoftProp)
		{
			return FString::Printf(TEXT("%s soft property missing."), PropertyName);
		}
		SoftProp->SetPropertyValue_InContainer(Actor, FSoftObjectPtr(FSoftObjectPath(Path)));
		return TEXT("");
	}

	FString PreflightConfigureNeuroResearchStationIntro(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("configure_neuro_research_station_intro must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before configuring ResearchStation_NeuroGenetics.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_neuro_research_station_intro does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroResearchStationIntroSpec);
		if (!Spec.Equals(NeuroResearchStationIntroSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroResearchStationIntroSpec);
		}
		static const TCHAR* Rejected[] = {
			TEXT("label"), TEXT("objective_id"), TEXT("event_id"), TEXT("adaptation"), TEXT("prompt"),
			TEXT("speaker"), TEXT("line"), TEXT("package"), TEXT("path")
		};
		if (Args.IsValid())
		{
			for (const TCHAR* Key : Rejected)
			{
				if (Args->HasField(Key))
				{
					return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
				}
			}
		}

		UObject* Intro = FindNeuroResearchStationIntroMissionAssetExact();
		if (!Intro || !NeuroResearchStationIntroMissionMismatchReason(Intro).IsEmpty())
		{
			return TEXT("DA_Mission_NeuroResearchStationIntro must exist and match locked contract before station configure.");
		}
		UObject* Targeting = FindNeuroTargetingWhyMissionAssetExact();
		if (!Targeting || !NeuroTargetingWhyMissionMismatchReason(Targeting).IsEmpty())
		{
			return TEXT("DA_Mission_NeuroTargetingWhy must exist and match locked contract before station configure.");
		}
		if (!NeuroTargetingWhyNextResearchStationReadNext(Targeting)
				.Equals(NeuroResearchStationIntroMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_NeuroTargetingWhy.NextMissionAsset must already point at DA_Mission_NeuroResearchStationIntro.");
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			return TEXT("Editor world missing.");
		}
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearchStationIntroLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), NeuroResearchStationIntroLabel, Matches.Num());
		}
		AActor* Station = Matches[0];
		if (const FString PackageError = NeuroResearchStationIntro_RequireNeuroPackage(Station); !PackageError.IsEmpty())
		{
			return PackageError;
		}
		if (!Station->GetClass() || !Station->GetClass()->GetName().Equals(NeuroResearchStationIntroClassName))
		{
			return TEXT("ResearchStation_NeuroGenetics class is not AProjectOrganoidResearchStation.");
		}
		if (const FString PromptError = NeuroPowerFailureDiscovery_CheckNameOrText(
				Station, TEXT("InteractionPrompt"), NeuroResearchStationIntroPrompt);
			!PromptError.IsEmpty())
		{
			return FString::Printf(TEXT("Prompt must remain unchanged. %s"), *PromptError);
		}

		const bool bAlreadyExact = NeuroResearchStationIntro_MismatchReason(Station).IsEmpty();
		Before->SetStringField(TEXT("spec"), NeuroResearchStationIntroSpec);
		Before->SetStringField(TEXT("action"), NeuroResearchStationIntroAction);
		Before->SetStringField(TEXT("label"), NeuroResearchStationIntroLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroResearchStationIntroSpec);
		Proposed->SetStringField(TEXT("action"), NeuroResearchStationIntroAction);
		Proposed->SetStringField(TEXT("label"), NeuroResearchStationIntroLabel);
		Proposed->SetStringField(TEXT("CampaignRequiredActiveObjectiveId"), NeuroResearchStationIntroObjectiveId);
		Proposed->SetStringField(TEXT("CampaignUnlockAdaptation"), NeuroResearchStationIntroAdaptationPath);
		Proposed->SetStringField(TEXT("CampaignCreditAdaptation"), NeuroResearchStationIntroAdaptationPath);
		Proposed->SetStringField(TEXT("CampaignSuccessObjectiveEventId"), NeuroResearchStationIntroEventId);
		Proposed->SetStringField(TEXT("CampaignReplayGuardObjectiveId"), NeuroResearchStationIntroObjectiveId);
		Proposed->SetStringField(TEXT("CampaignSuccessNotificationSpeaker"), NeuroResearchStationIntroSpeaker);
		Proposed->SetStringField(TEXT("CampaignSuccessNotificationText"), NeuroResearchStationIntroLine);
		Proposed->SetNumberField(TEXT("CampaignSuccessNotificationDurationSeconds"), NeuroResearchStationIntroDuration);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroResearchStationIntro(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("configure_neuro_research_station_intro must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroResearchStationIntro(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearchStationIntroLabel);
		if (Matches.Num() != 1)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("not_found"),
				TEXT("ResearchStation_NeuroGenetics vanished. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}
		AActor* Station = Matches[0];

		if (NeuroResearchStationIntro_MismatchReason(Station).IsEmpty())
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), NeuroResearchStationIntroLabel);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"NeuroResearchStationIntro",
				"Configure NeuroGenetics Research Station intro contract"));

			FString WriteError = SetNamedPropertyFromString(
				Station, TEXT("CampaignRequiredActiveObjectiveId"), NeuroResearchStationIntroObjectiveId);
			if (WriteError.IsEmpty())
			{
				WriteError = NeuroResearchStationIntro_WriteSoft(
					Station, TEXT("CampaignUnlockAdaptation"), NeuroResearchStationIntroAdaptationPath);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = NeuroResearchStationIntro_WriteSoft(
					Station, TEXT("CampaignCreditAdaptation"), NeuroResearchStationIntroAdaptationPath);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(
					Station, TEXT("CampaignSuccessObjectiveEventId"), NeuroResearchStationIntroEventId);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(
					Station, TEXT("CampaignReplayGuardObjectiveId"), NeuroResearchStationIntroObjectiveId);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(
					Station, TEXT("CampaignSuccessNotificationSpeaker"), NeuroResearchStationIntroSpeaker);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromString(
					Station, TEXT("CampaignSuccessNotificationText"), NeuroResearchStationIntroLine);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = SetNamedPropertyFromNumber(
					Station, TEXT("CampaignSuccessNotificationDurationSeconds"), NeuroResearchStationIntroDuration);
			}
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}

			Station->MarkPackageDirty();
		}

		if (const FString VerifyError = NeuroResearchStationIntro_MismatchReason(Station); !VerifyError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-configure verify failed for ResearchStation_NeuroGenetics: %s"), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetStringField(TEXT("label"), NeuroResearchStationIntroLabel);
		Change.After->SetStringField(TEXT("CampaignRequiredActiveObjectiveId"), NeuroResearchStationIntroObjectiveId);
		Change.After->SetStringField(TEXT("CampaignSuccessObjectiveEventId"), NeuroResearchStationIntroEventId);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
