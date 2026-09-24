	// Configure the existing neural-change evidence instrument's default-off follow-up hook.
	// Does not move the actor, change power, or save.
	const TCHAR* NeuroLiveAdaptationConnectionSpec = TEXT("neuro_live_adaptation_connection_v1");
	const TCHAR* NeuroLiveAdaptationConnectionAction = TEXT("configure_neuro_live_adaptation_connection");
	const TCHAR* NeuroLiveAdaptationConnectionLabel = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	const TCHAR* NeuroLiveAdaptationConnectionPackage = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	const FVector NeuroLiveAdaptationConnectionLocation(500.f, -2520.f, -1100.f);
	const TCHAR* NeuroLiveAdaptationConnectionRequired = TEXT("Obj_ConnectLiveAdaptation");
	const TCHAR* NeuroLiveAdaptationConnectionPrerequisite = TEXT("Obj_ApplyNeuralSlow");
	const TCHAR* NeuroLiveAdaptationConnectionEvent = TEXT("Event_LiveAdaptationConnected");
	const TCHAR* NeuroLiveAdaptationConnectionSpeaker = TEXT("Nathan");
	const TCHAR* NeuroLiveAdaptationConnectionLine =
		TEXT("The live Host slowed the same way these records describe. Epitope was adapting nervous systems, not only recording them.");
	constexpr double NeuroLiveAdaptationConnectionDuration = 7.0;
	const TCHAR* NeuroLiveAdaptationConnectionExamineObjective = TEXT("Obj_ExamineNeuralChangeEvidence");
	const TCHAR* NeuroLiveAdaptationConnectionExamineEvent = TEXT("Event_NeuralChangeEvidenceExamined");

	FString NeuroLiveAdaptationConnectionReadName(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return TEXT("");
		}
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			return NameProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		return TEXT("");
	}

	FString NeuroLiveAdaptationConnectionReadText(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return TEXT("");
		}
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			return TextProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		return TEXT("");
	}

	double NeuroLiveAdaptationConnectionReadNumber(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return -1.0;
		}
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			return FloatProp->GetPropertyValue_InContainer(Actor);
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			return DoubleProp->GetPropertyValue_InContainer(Actor);
		}
		return -1.0;
	}

	FString NeuroLiveAdaptationConnectionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("label"), TEXT("class"), TEXT("class_path"), TEXT("location"), TEXT("package"),
			TEXT("objective_id"), TEXT("event_id"), TEXT("speaker"), TEXT("response"), TEXT("duration"),
			TEXT("required_active"), TEXT("prerequisite")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroLiveAdaptationConnectionSpec);
		if (!Spec.Equals(NeuroLiveAdaptationConnectionSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroLiveAdaptationConnectionSpec);
		}
		return TEXT("");
	}

	AActor* FindNeuroLiveAdaptationConnectionInstrument()
	{
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return nullptr;
		}
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroLiveAdaptationConnectionLabel);
		return Matches.Num() == 1 ? Matches[0] : nullptr;
	}

	bool NeuroLiveAdaptationConnectionFollowupExact(AActor* Actor)
	{
		return Actor
			&& NeuroLiveAdaptationConnectionReadName(Actor, TEXT("FollowupRequiredActiveObjectiveId"))
				.Equals(NeuroLiveAdaptationConnectionRequired)
			&& NeuroLiveAdaptationConnectionReadName(Actor, TEXT("FollowupPrerequisiteCompletedObjectiveId"))
				.Equals(NeuroLiveAdaptationConnectionPrerequisite)
			&& NeuroLiveAdaptationConnectionReadName(Actor, TEXT("FollowupSuccessEventId"))
				.Equals(NeuroLiveAdaptationConnectionEvent)
			&& NeuroLiveAdaptationConnectionReadText(Actor, TEXT("FollowupSpeakerLabel"))
				.Equals(NeuroLiveAdaptationConnectionSpeaker)
			&& NeuroLiveAdaptationConnectionReadText(Actor, TEXT("FollowupResponseText"))
				.Equals(NeuroLiveAdaptationConnectionLine)
			&& FMath::IsNearlyEqual(
				NeuroLiveAdaptationConnectionReadNumber(Actor, TEXT("FollowupNotificationDurationSeconds")),
				NeuroLiveAdaptationConnectionDuration,
				0.01);
	}

	FString NeuroLiveAdaptationConnectionLegacyReason(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Instrument is null.");
		}
		if (!Actor->GetClass() || !Actor->GetClass()->GetPathName().Equals(NeuroChangeEvClassPath))
		{
			return TEXT("Class is not ProjectOrganoidInspectableInstrument.");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroLiveAdaptationConnectionPackage))
		{
			return FString::Printf(
				TEXT("Owner '%s' expected '%s'."),
				*ActorOwningPackage(Actor),
				NeuroLiveAdaptationConnectionPackage);
		}
		if (!LocationMatches(Actor->GetActorLocation(), NeuroLiveAdaptationConnectionLocation))
		{
			const FVector Loc = Actor->GetActorLocation();
			return FString::Printf(TEXT("Location (%.2f, %.2f, %.2f) expected (500, -2520, -1100)."), Loc.X, Loc.Y, Loc.Z);
		}
		if (!NeuroLiveAdaptationConnectionReadName(Actor, TEXT("RequiredActiveObjectiveId"))
				.Equals(NeuroLiveAdaptationConnectionExamineObjective)
			|| !NeuroLiveAdaptationConnectionReadName(Actor, TEXT("CompletedObjectiveIdForReplayGuard"))
				.Equals(NeuroLiveAdaptationConnectionExamineObjective)
			|| !NeuroLiveAdaptationConnectionReadName(Actor, TEXT("ObjectiveEventId"))
				.Equals(NeuroLiveAdaptationConnectionExamineEvent))
		{
			return TEXT("Primary examine contract changed. Fail closed.");
		}
		return TEXT("");
	}

	FString ApplyNeuroLiveAdaptationConnection(AActor* Actor)
	{
		if (const FString Error = SetNamedPropertyFromString(
				Actor, TEXT("FollowupRequiredActiveObjectiveId"), NeuroLiveAdaptationConnectionRequired);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Actor, TEXT("FollowupPrerequisiteCompletedObjectiveId"), NeuroLiveAdaptationConnectionPrerequisite);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Actor, TEXT("FollowupSuccessEventId"), NeuroLiveAdaptationConnectionEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Actor, TEXT("FollowupSpeakerLabel"), NeuroLiveAdaptationConnectionSpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(
				Actor, TEXT("FollowupResponseText"), NeuroLiveAdaptationConnectionLine);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(
				Actor, TEXT("FollowupNotificationDurationSeconds"), NeuroLiveAdaptationConnectionDuration);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString PreflightConfigureNeuroLiveAdaptationConnection(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("configure_neuro_live_adaptation_connection must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before configuring the instrument.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_neuro_live_adaptation_connection does not save.");
		}
		if (const FString OverrideError = NeuroLiveAdaptationConnectionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}

		AActor* Instrument = FindNeuroLiveAdaptationConnectionInstrument();
		if (!Instrument)
		{
			return TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics missing or not unique.");
		}
		if (const FString LegacyError = NeuroLiveAdaptationConnectionLegacyReason(Instrument); !LegacyError.IsEmpty())
		{
			return LegacyError;
		}

		const bool bAlreadyExact = NeuroLiveAdaptationConnectionFollowupExact(Instrument);
		Before->SetStringField(TEXT("spec"), NeuroLiveAdaptationConnectionSpec);
		Before->SetStringField(TEXT("action"), NeuroLiveAdaptationConnectionAction);
		Before->SetStringField(TEXT("label"), NeuroLiveAdaptationConnectionLabel);
		Before->SetStringField(TEXT("package"), NeuroLiveAdaptationConnectionPackage);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(
			TEXT("followup_event"),
			NeuroLiveAdaptationConnectionReadName(Instrument, TEXT("FollowupSuccessEventId")));

		Proposed->SetStringField(TEXT("spec"), NeuroLiveAdaptationConnectionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroLiveAdaptationConnectionAction);
		Proposed->SetStringField(TEXT("label"), NeuroLiveAdaptationConnectionLabel);
		Proposed->SetStringField(TEXT("package"), NeuroLiveAdaptationConnectionPackage);
		Proposed->SetStringField(TEXT("required_active"), NeuroLiveAdaptationConnectionRequired);
		Proposed->SetStringField(TEXT("prerequisite_completed"), NeuroLiveAdaptationConnectionPrerequisite);
		Proposed->SetStringField(TEXT("success_event"), NeuroLiveAdaptationConnectionEvent);
		Proposed->SetStringField(TEXT("speaker"), NeuroLiveAdaptationConnectionSpeaker);
		Proposed->SetNumberField(TEXT("duration_seconds"), NeuroLiveAdaptationConnectionDuration);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroLiveAdaptationConnection(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("configure_neuro_live_adaptation_connection must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroLiveAdaptationConnection(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		AActor* Instrument = FindNeuroLiveAdaptationConnectionInstrument();
		if (!Instrument)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Instrument missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (NeuroLiveAdaptationConnectionFollowupExact(Instrument))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetBoolField(TEXT("save_performed"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		const FVector LocationBefore = Instrument->GetActorLocation();
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"ConfigureNeuroLiveAdaptationConnection",
				"Configure live adaptation connection on the neural evidence instrument"));
			if (const FString ApplyError = ApplyNeuroLiveAdaptationConnection(Instrument); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Instrument->MarkPackageDirty();
		}

		if (!LocationMatches(Instrument->GetActorLocation(), LocationBefore)
			|| !NeuroLiveAdaptationConnectionFollowupExact(Instrument)
			|| !NeuroLiveAdaptationConnectionLegacyReason(Instrument).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				TEXT("Instrument follow-up verify failed or the actor moved."),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetStringField(TEXT("label"), NeuroLiveAdaptationConnectionLabel);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
