// Configure Terminal_ControlSpine in place on SL_Epitope_Reactor. Does not move, save, or change power.
	const TCHAR* TheConclusionTerminalSpec = TEXT("reactor_control_spine_v1");
	const TCHAR* TheConclusionTerminalAction = TEXT("configure_reactor_control_spine");
	const TCHAR* TheConclusionTerminalLabel = TEXT("Terminal_ControlSpine");
	const FVector TheConclusionTerminalLocation(-1950.f, -1650.f, -4700.f);
	const TCHAR* TheConclusionTerminalPrompt = TEXT("Reach Control Spine");
	const TCHAR* TheConclusionSpeaker = TEXT("Nathan");
	const TCHAR* TheConclusionLine = TEXT("The incubator is awake. Every document leads here.");
	const double TheConclusionLineDuration = 7.0;

	bool TheConclusionSoftLinkEmpty(AActor* Actor, const TCHAR* PropertyName)
	{
		FSoftObjectProperty* Prop = CastField<FSoftObjectProperty>(FindInstanceProperty(Actor, PropertyName));
		return Prop && Prop->GetPropertyValue_InContainer(Actor).IsNull();
	}

	bool TheConclusionNameNone(AActor* Actor, const TCHAR* PropertyName)
	{
		FNameProperty* Prop = CastField<FNameProperty>(FindInstanceProperty(Actor, PropertyName));
		return Prop && Prop->GetPropertyValue_InContainer(Actor).IsNone();
	}

	bool TheConclusionTerminalConfigured(AActor* Actor)
	{
		if (!Actor || !Actor->GetClass() || !Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidTerminal")))
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredActiveObjectiveId"), TheConclusionObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SuccessObjectiveEventId"), TheConclusionEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), TheConclusionTerminalPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignHackPrompt"), TheConclusionTerminalPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationSpeaker"), TheConclusionSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationText"), TheConclusionLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CompletionNotificationDurationSeconds"), TheConclusionLineDuration).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bApplyPowerChangeOnSuccess"), false).IsEmpty()
			&& NeuroGeneticsMissionReadEnumNameEquals(Actor, Actor->GetClass(), TEXT("PowerSector"), TEXT("Reactor"))
			&& TheConclusionSoftLinkEmpty(Actor, TEXT("LinkedDoorLock"))
			&& TheConclusionSoftLinkEmpty(Actor, TEXT("LinkedSecurityGate"))
			&& TheConclusionNameNone(Actor, TEXT("LinkedSecurityGateId"));
	}

	FString TheConclusionTerminalRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("location"), TEXT("transform"), TEXT("label"), TEXT("package"), TEXT("power"),
			TEXT("prompt"), TEXT("line"), TEXT("actor")
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(TEXT("Client override '%s' rejected. Spec is locked."), Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), TheConclusionTerminalSpec);
		if (!Spec.Equals(TheConclusionTerminalSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), TheConclusionTerminalSpec);
		}
		return TEXT("");
	}

	FString TheConclusionRequireTerminal(UWorld* World, AActor*& OutActor)
	{
		OutActor = nullptr;
		const TArray<AActor*> Found = FindByExactLabel(World, TheConclusionTerminalLabel);
		if (Found.Num() != 1 || !Found[0])
		{
			return FString::Printf(TEXT("%s must be unique. count=%d."), TheConclusionTerminalLabel, Found.Num());
		}
		AActor* Actor = Found[0];
		if (!Actor->GetClass() || !Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidTerminal")))
		{
			return TEXT("Terminal_ControlSpine class mismatch.");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), ReactorPackage))
		{
			return TEXT("Terminal_ControlSpine must live on SL_Epitope_Reactor.");
		}
		if (!Actor->GetActorLocation().Equals(TheConclusionTerminalLocation, 1.f))
		{
			return TEXT("Terminal_ControlSpine moved. ZERO writes.");
		}
		if (!TheConclusionSoftLinkEmpty(Actor, TEXT("LinkedDoorLock")) || !TheConclusionSoftLinkEmpty(Actor, TEXT("LinkedSecurityGate")) || !TheConclusionNameNone(Actor, TEXT("LinkedSecurityGateId")))
		{
			return TEXT("Terminal_ControlSpine already unlocks a door or gate. ZERO writes.");
		}
		OutActor = Actor;
		return TEXT("");
	}

	FString PreflightConfigureReactorControlSpine(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_reactor_control_spine does not save.");
		}
		if (const FString OverrideError = TheConclusionTerminalRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Conclusion = FindTheConclusionMissionAssetExact();
		if (!Conclusion)
		{
			return TEXT("DA_Mission_TheConclusion missing.");
		}
		if (const FString ConclusionMismatch = TheConclusionMissionMismatchReason(Conclusion); !ConclusionMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_TheConclusion mismatch: %s"), *ConclusionMismatch);
		}
		const FString HandoverNext = NeuroAdaptationConnectionNextRevelationReadNext(FindComputeHandoverMissionAssetExact());
		if (!HandoverNext.Equals(TheConclusionMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ComputeHandover.NextMissionAsset must already be DA_Mission_TheConclusion.");
		}
		UWorld* World = GetEditorWorld();
		AActor* Terminal = nullptr;
		if (const FString Error = TheConclusionRequireTerminal(World, Terminal); !Error.IsEmpty())
		{
			return Error;
		}
		const bool bAlreadyExact = TheConclusionTerminalConfigured(Terminal);
		Before->SetStringField(TEXT("spec"), TheConclusionTerminalSpec);
		Before->SetStringField(TEXT("label"), TheConclusionTerminalLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("location"), Terminal->GetActorLocation().ToString());
		Proposed->SetStringField(TEXT("spec"), TheConclusionTerminalSpec);
		Proposed->SetStringField(TEXT("action"), TheConclusionTerminalAction);
		Proposed->SetStringField(TEXT("label"), TheConclusionTerminalLabel);
		Proposed->SetStringField(TEXT("sector"), TEXT("Reactor"));
		Proposed->SetStringField(TEXT("restored_state"), TEXT("Emergency"));
		Proposed->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Proposed->SetStringField(TEXT("required_objective"), TheConclusionObjectiveId);
		Proposed->SetStringField(TEXT("prompt"), TheConclusionTerminalPrompt);
		Proposed->SetStringField(TEXT("event"), TheConclusionEventId);
		Proposed->SetStringField(TEXT("line"), TheConclusionLine);
		Proposed->SetNumberField(TEXT("duration_seconds"), TheConclusionLineDuration);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		Proposed->SetBoolField(TEXT("unlocks"), false);
		return TEXT("");
	}

	FString ApplyTheConclusionTerminal(AActor* Terminal)
	{
		auto Apply = [Terminal](const TCHAR* Name, const TCHAR* Value) { return SetNamedPropertyFromString(Terminal, Name, Value); };
		FString Error = Apply(TEXT("RequiredActiveObjectiveId"), TheConclusionObjectiveId);
		if (Error.IsEmpty()) Error = Apply(TEXT("SuccessObjectiveEventId"), TheConclusionEventId);
		if (Error.IsEmpty()) Error = Apply(TEXT("InteractionPrompt"), TheConclusionTerminalPrompt);
		if (Error.IsEmpty()) Error = Apply(TEXT("CampaignHackPrompt"), TheConclusionTerminalPrompt);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationSpeaker"), TheConclusionSpeaker);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationText"), TheConclusionLine);
		if (Error.IsEmpty())
		{
			FProperty* Prop = FindInstanceProperty(Terminal, TEXT("CompletionNotificationDurationSeconds"));
			FString JsonError;
			if (!Prop || !SetPropertyFromJson(Terminal, Prop, MakeShared<FJsonValueNumber>(TheConclusionLineDuration), JsonError))
			{
				Error = JsonError.IsEmpty() ? TEXT("CompletionNotificationDurationSeconds missing") : JsonError;
			}
		}
		if (Error.IsEmpty())
		{
			FBoolProperty* Prop = CastField<FBoolProperty>(FindInstanceProperty(Terminal, TEXT("bApplyPowerChangeOnSuccess")));
			if (!Prop)
			{
				Error = TEXT("bApplyPowerChangeOnSuccess missing");
			}
			else
			{
				Prop->SetPropertyValue_InContainer(Terminal, false);
			}
		}
		if (Error.IsEmpty())
		{
			if (!NeuroGeneticsMissionWriteEnumByName(Terminal, Terminal->GetClass(), TEXT("PowerSector"), TEXT("Reactor"), Error))
			{
				return Error;
			}
			Error.Reset();
		}
		return Error;
	}

	TSharedRef<FJsonObject> ExecuteConfigureReactorControlSpine(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_reactor_control_spine must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureReactorControlSpine(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		AActor* Terminal = nullptr;
		if (const FString Error = TheConclusionRequireTerminal(World, Terminal); !Error.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), Error, MakeShared<FBridgeChange>(Change));
		}
		if (TheConclusionTerminalConfigured(Terminal))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetNumberField(TEXT("actor_count"), 1);
			Change.After->SetStringField(TEXT("reactor_power"), TEXT("Emergency"));
			Change.After->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
			Change.After->SetBoolField(TEXT("changes_power"), false);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		const FVector LocationBefore = Terminal->GetActorLocation();
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ReactorControlSpine", "Configure reactor control spine"));
			const FString WriteError = ApplyTheConclusionTerminal(Terminal);
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			Terminal->MarkPackageDirty();
		}
		if (!TheConclusionTerminalConfigured(Terminal) || !Terminal->GetActorLocation().Equals(LocationBefore, 0.1f) || !Terminal->GetActorLocation().Equals(TheConclusionTerminalLocation, 1.f) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Terminal_ControlSpine was not configured in place."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetNumberField(TEXT("actor_count"), 1);
		Change.After->SetStringField(TEXT("label"), TheConclusionTerminalLabel);
		Change.After->SetStringField(TEXT("reactor_power"), TEXT("Emergency"));
		Change.After->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("unlocks"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
