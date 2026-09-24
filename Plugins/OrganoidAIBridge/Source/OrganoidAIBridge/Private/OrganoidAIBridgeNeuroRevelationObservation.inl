// Configure the observation node's existing default-off follow-up. IDs are not hard-coded in the instrument class.
// Does not move the actor, change power, unlock Cryo, or save.
	const TCHAR* NeuroRevelationObservationSpec = TEXT("neuro_revelation_observation_v1");
	const TCHAR* NeuroRevelationObservationAction = TEXT("configure_neuro_revelation_observation");
	const TCHAR* NeuroRevelationObservationLabel = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	const TCHAR* NeuroRevelationObservationSpeaker = TEXT("Nathan");
	const TCHAR* NeuroRevelationObservationLine =
		TEXT("These people are not simply infected; the research in this wing has been systematically reorganizing their nervous systems.");
	constexpr double NeuroRevelationObservationDuration = 7.0;

	FString NeuroRevelationObservationReadName(AActor* Actor, const TCHAR* PropertyName)
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

	FString NeuroRevelationObservationReadText(AActor* Actor, const TCHAR* PropertyName)
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

	double NeuroRevelationObservationReadNumber(AActor* Actor, const TCHAR* PropertyName)
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

	FString NeuroRevelationObservationRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), NeuroRevelationObservationSpec);
		if (!Spec.Equals(NeuroRevelationObservationSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroRevelationObservationSpec);
		}
		return TEXT("");
	}

	AActor* FindNeuroRevelationObservationNode()
	{
		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return nullptr;
		}
		const TArray<AActor*> Matches = FindByExactLabel(World, NeuroRevelationObservationLabel);
		return Matches.Num() == 1 ? Matches[0] : nullptr;
	}

	bool NeuroRevelationObservationFollowupExact(AActor* Actor)
	{
		return Actor
			&& NeuroRevelationObservationReadName(Actor, TEXT("FollowupRequiredActiveObjectiveId")).Equals(NeuroRevelationObjectiveId)
			&& NeuroRevelationObservationReadName(Actor, TEXT("FollowupPrerequisiteCompletedObjectiveId")).Equals(NeuroAdaptationConnectionObjectiveId)
			&& NeuroRevelationObservationReadName(Actor, TEXT("FollowupSuccessEventId")).Equals(NeuroRevelationEventId)
			&& NeuroRevelationObservationReadText(Actor, TEXT("FollowupSpeakerLabel")).Equals(NeuroRevelationObservationSpeaker)
			&& NeuroRevelationObservationReadText(Actor, TEXT("FollowupResponseText")).Equals(NeuroRevelationObservationLine)
			&& FMath::IsNearlyEqual(
				NeuroRevelationObservationReadNumber(Actor, TEXT("FollowupNotificationDurationSeconds")),
				NeuroRevelationObservationDuration,
				0.01);
	}

	FString NeuroRevelationObservationLegacyReason(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Observation node is null.");
		}
		if (!Actor->GetActorLabel().Equals(NeuroRevelationObservationLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("Actor label is not NeuralSignatureObservationNode_NeuroGenetics.");
		}
		if (!Actor->GetClass() || !Actor->GetClass()->GetPathName().Equals(NeuroSigObsClassPath))
		{
			return TEXT("Class is not ProjectOrganoidInspectableInstrument.");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("Owner '%s' expected NeuroGenetics."), *ActorOwningPackage(Actor));
		}
		if (!LocationMatches(Actor->GetActorLocation(), NeuroRevelationNodeLocation))
		{
			const FVector Loc = Actor->GetActorLocation();
			return FString::Printf(TEXT("Location (%.2f, %.2f, %.2f) expected (500, -2100, -1100)."), Loc.X, Loc.Y, Loc.Z);
		}
		if (!NeuroRevelationObservationReadName(Actor, TEXT("RequiredActiveObjectiveId")).Equals(NeuroSigObsRequiredActive)
			|| !NeuroRevelationObservationReadName(Actor, TEXT("CompletedObjectiveIdForReplayGuard")).Equals(NeuroSigObsReplayGuard)
			|| !NeuroRevelationObservationReadName(Actor, TEXT("ObjectiveEventId")).Equals(NeuroSigObsObjectiveEvent))
		{
			return TEXT("Primary follow-signature contract changed. Fail closed.");
		}
		return TEXT("");
	}

	FString ApplyNeuroRevelationObservation(AActor* Actor)
	{
		if (const FString Error = SetNamedPropertyFromString(Actor, TEXT("FollowupRequiredActiveObjectiveId"), NeuroRevelationObjectiveId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Actor, TEXT("FollowupPrerequisiteCompletedObjectiveId"), NeuroAdaptationConnectionObjectiveId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Actor, TEXT("FollowupSuccessEventId"), NeuroRevelationEventId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Actor, TEXT("FollowupSpeakerLabel"), NeuroRevelationObservationSpeaker); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Actor, TEXT("FollowupResponseText"), NeuroRevelationObservationLine); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromNumber(Actor, TEXT("FollowupNotificationDurationSeconds"), NeuroRevelationObservationDuration); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString PreflightConfigureNeuroRevelationObservation(
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
			return TEXT("save must be false. configure_neuro_revelation_observation does not save.");
		}
		if (const FString OverrideError = NeuroRevelationObservationRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		AActor* Node = FindNeuroRevelationObservationNode();
		if (!Node)
		{
			return TEXT("NeuralSignatureObservationNode_NeuroGenetics missing or not unique.");
		}
		if (const FString LegacyError = NeuroRevelationObservationLegacyReason(Node); !LegacyError.IsEmpty())
		{
			return LegacyError;
		}
		const bool bAlreadyExact = NeuroRevelationObservationFollowupExact(Node);
		Before->SetStringField(TEXT("spec"), NeuroRevelationObservationSpec);
		Before->SetStringField(TEXT("label"), NeuroRevelationObservationLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("spec"), NeuroRevelationObservationSpec);
		Proposed->SetStringField(TEXT("action"), NeuroRevelationObservationAction);
		Proposed->SetStringField(TEXT("label"), NeuroRevelationObservationLabel);
		Proposed->SetStringField(TEXT("required_active"), NeuroRevelationObjectiveId);
		Proposed->SetStringField(TEXT("prerequisite_completed"), NeuroAdaptationConnectionObjectiveId);
		Proposed->SetStringField(TEXT("success_event"), NeuroRevelationEventId);
		Proposed->SetNumberField(TEXT("duration_seconds"), NeuroRevelationObservationDuration);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroRevelationObservation(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_neuro_revelation_observation must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroRevelationObservation(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		AActor* Node = FindNeuroRevelationObservationNode();
		if (!Node)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Observation node missing at execute. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (NeuroRevelationObservationFollowupExact(Node))
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

		const FVector LocationBefore = Node->GetActorLocation();
		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"ConfigureNeuroRevelationObservation",
				"Configure Neuro revelation on the signature observation node"));
			if (const FString ApplyError = ApplyNeuroRevelationObservation(Node); !ApplyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("apply_failed"), ApplyError, MakeShared<FBridgeChange>(Change));
			}
			Node->MarkPackageDirty();
		}
		if (!LocationMatches(Node->GetActorLocation(), LocationBefore)
			|| !NeuroRevelationObservationFollowupExact(Node)
			|| !NeuroRevelationObservationLegacyReason(Node).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Observation follow-up verify failed or the actor moved."), MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetStringField(TEXT("label"), NeuroRevelationObservationLabel);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
