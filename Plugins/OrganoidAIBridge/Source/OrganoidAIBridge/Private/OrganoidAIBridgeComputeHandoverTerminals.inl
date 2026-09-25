// Configure the three Compute interface terminals and Sterling's confession pad in place.
	const TCHAR* ComputeHandoverActorsSpec = TEXT("compute_handover_terminals_and_datapad_v1");
	const TCHAR* ComputeHandoverActorsAction = TEXT("configure_compute_handover_terminals_and_datapad");
	const TCHAR* ComputeHandoverTerminalLabels[] = {
		TEXT("Terminal_CoreInterface_1"),
		TEXT("Terminal_CoreInterface_2"),
		TEXT("Terminal_CoreInterface_3")
	};
	const FVector ComputeHandoverTerminalLocations[] = {
		FVector(-1760.f, -2150.f, -3500.f),
		FVector(-1760.f, -1650.f, -3500.f),
		FVector(-1760.f, -1150.f, -3500.f)
	};
	const TCHAR* ComputeHandoverDatapadLabel = TEXT("DataPad_SterlingConfession");
	const FVector ComputeHandoverDatapadLocation(-2425.f, -2275.f, -3510.f);
	const TCHAR* ComputeHandoverTerminalPrompt = TEXT("Hack Compute Core");
	const TCHAR* ComputeHandoverDatapadPrompt = TEXT("Read Sterling's Confession");
	const TCHAR* ComputeHandoverSpeaker = TEXT("Nathan");
	const TCHAR* ComputeHandoverTerminalLine = TEXT("It's been running the lockdown the whole time. It didn't lose control.");
	const TCHAR* ComputeHandoverDatapadLine = TEXT("He didn't lose control. He handed it over.");
	const double ComputeHandoverLineDuration = 7.0;

	bool ComputeHandoverSoftLinkEmpty(AActor* Actor, const TCHAR* PropertyName)
	{
		FSoftObjectProperty* Prop = CastField<FSoftObjectProperty>(FindInstanceProperty(Actor, PropertyName));
		return Prop && Prop->GetPropertyValue_InContainer(Actor).IsNull();
	}

	bool ComputeHandoverTerminalConfigured(AActor* Actor)
	{
		if (!Actor || !Actor->GetClass() || !Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidTerminal")))
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredActiveObjectiveId"), ComputeHandoverHackObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SuccessObjectiveEventId"), ComputeHandoverHackEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), ComputeHandoverTerminalPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CampaignHackPrompt"), ComputeHandoverTerminalPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationSpeaker"), ComputeHandoverSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationText"), ComputeHandoverTerminalLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CompletionNotificationDurationSeconds"), ComputeHandoverLineDuration).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bApplyPowerChangeOnSuccess"), false).IsEmpty()
			&& NeuroGeneticsMissionReadEnumNameEquals(Actor, Actor->GetClass(), TEXT("PowerSector"), TEXT("Compute"))
			&& ComputeHandoverSoftLinkEmpty(Actor, TEXT("LinkedDoorLock"))
			&& ComputeHandoverSoftLinkEmpty(Actor, TEXT("LinkedSecurityGate"));
	}

	bool ComputeHandoverDatapadConfigured(AActor* Actor)
	{
		if (!Actor || !Actor->GetClass() || !Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidDataPad")))
		{
			return false;
		}
		return NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredObjectiveIdForInteraction"), ComputeHandoverConfessionObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ObjectiveEventId"), ComputeHandoverConfessionEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), ComputeHandoverDatapadPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationSpeaker"), ComputeHandoverSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationText"), ComputeHandoverDatapadLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CompletionNotificationDurationSeconds"), ComputeHandoverLineDuration).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bBroadcastGenericDataPadEvent"), false).IsEmpty();
	}

	FString ComputeHandoverActorsRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
		const FString Spec = GetString(Args, TEXT("spec"), ComputeHandoverActorsSpec);
		if (!Spec.Equals(ComputeHandoverActorsSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), ComputeHandoverActorsSpec);
		}
		return TEXT("");
	}

	FString ComputeHandoverRequireActor(UWorld* World, const TCHAR* Label, const FVector& Location, const TCHAR* ExpectedClass, AActor*& OutActor)
	{
		OutActor = nullptr;
		const TArray<AActor*> Found = FindByExactLabel(World, Label);
		if (Found.Num() != 1 || !Found[0])
		{
			return FString::Printf(TEXT("%s must be unique. count=%d."), Label, Found.Num());
		}
		AActor* Actor = Found[0];
		if (!Actor->GetClass() || !Actor->GetClass()->GetName().Equals(ExpectedClass))
		{
			return FString::Printf(TEXT("%s class mismatch."), Label);
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), ComputePackage))
		{
			return FString::Printf(TEXT("%s must live on SL_Epitope_Compute."), Label);
		}
		if (!Actor->GetActorLocation().Equals(Location, 1.f))
		{
			return FString::Printf(TEXT("%s moved. ZERO writes."), Label);
		}
		if (Actor->GetClass() && Actor->GetClass()->GetName().Equals(TEXT("ProjectOrganoidTerminal"))
			&& (!ComputeHandoverSoftLinkEmpty(Actor, TEXT("LinkedDoorLock")) || !ComputeHandoverSoftLinkEmpty(Actor, TEXT("LinkedSecurityGate"))))
		{
			return FString::Printf(TEXT("%s already unlocks a door or gate. ZERO writes."), Label);
		}
		OutActor = Actor;
		return TEXT("");
	}

	FString PreflightConfigureComputeHandoverActors(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_compute_handover_terminals_and_datapad does not save.");
		}
		if (const FString OverrideError = ComputeHandoverActorsRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		UObject* Handover = FindComputeHandoverMissionAssetExact();
		if (!Handover)
		{
			return TEXT("DA_Mission_ComputeHandover missing.");
		}
		if (const FString HandoverMismatch = ComputeHandoverMissionMismatchReason(Handover); !HandoverMismatch.IsEmpty())
		{
			return FString::Printf(TEXT("DA_Mission_ComputeHandover mismatch: %s"), *HandoverMismatch);
		}
		const FString EntryNext = NeuroAdaptationConnectionNextRevelationReadNext(FindComputeEntryMissionAssetExact());
		if (!EntryNext.Equals(ComputeHandoverMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_ComputeEntry.NextMissionAsset must already be DA_Mission_ComputeHandover.");
		}
		UWorld* World = GetEditorWorld();
		bool bAlreadyExact = true;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			AActor* Terminal = nullptr;
			if (const FString Error = ComputeHandoverRequireActor(World, ComputeHandoverTerminalLabels[Index], ComputeHandoverTerminalLocations[Index], TEXT("ProjectOrganoidTerminal"), Terminal); !Error.IsEmpty())
			{
				return Error;
			}
			bAlreadyExact = bAlreadyExact && ComputeHandoverTerminalConfigured(Terminal);
		}
		AActor* Pad = nullptr;
		if (const FString Error = ComputeHandoverRequireActor(World, ComputeHandoverDatapadLabel, ComputeHandoverDatapadLocation, TEXT("ProjectOrganoidDataPad"), Pad); !Error.IsEmpty())
		{
			return Error;
		}
		bAlreadyExact = bAlreadyExact && ComputeHandoverDatapadConfigured(Pad);
		Before->SetStringField(TEXT("spec"), ComputeHandoverActorsSpec);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetStringField(TEXT("terminals"), TEXT("Terminal_CoreInterface_1,Terminal_CoreInterface_2,Terminal_CoreInterface_3"));
		Before->SetStringField(TEXT("datapad"), ComputeHandoverDatapadLabel);
		Proposed->SetStringField(TEXT("spec"), ComputeHandoverActorsSpec);
		Proposed->SetStringField(TEXT("action"), ComputeHandoverActorsAction);
		Proposed->SetStringField(TEXT("sector"), TEXT("Compute"));
		Proposed->SetStringField(TEXT("restored_state"), TEXT("Online"));
		Proposed->SetStringField(TEXT("terminal_objective"), ComputeHandoverHackObjectiveId);
		Proposed->SetStringField(TEXT("terminal_prompt"), ComputeHandoverTerminalPrompt);
		Proposed->SetStringField(TEXT("terminal_event"), ComputeHandoverHackEventId);
		Proposed->SetStringField(TEXT("terminal_line"), ComputeHandoverTerminalLine);
		Proposed->SetStringField(TEXT("datapad_objective"), ComputeHandoverConfessionObjectiveId);
		Proposed->SetStringField(TEXT("datapad_prompt"), ComputeHandoverDatapadPrompt);
		Proposed->SetStringField(TEXT("datapad_event"), ComputeHandoverConfessionEventId);
		Proposed->SetStringField(TEXT("datapad_line"), ComputeHandoverDatapadLine);
		Proposed->SetNumberField(TEXT("duration_seconds"), ComputeHandoverLineDuration);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		Proposed->SetBoolField(TEXT("unlocks"), false);
		return TEXT("");
	}

	FString ApplyComputeHandoverTerminal(AActor* Terminal)
	{
		auto Apply = [Terminal](const TCHAR* Name, const TCHAR* Value) { return SetNamedPropertyFromString(Terminal, Name, Value); };
		FString Error = Apply(TEXT("RequiredActiveObjectiveId"), ComputeHandoverHackObjectiveId);
		if (Error.IsEmpty()) Error = Apply(TEXT("SuccessObjectiveEventId"), ComputeHandoverHackEventId);
		if (Error.IsEmpty()) Error = Apply(TEXT("InteractionPrompt"), ComputeHandoverTerminalPrompt);
		if (Error.IsEmpty()) Error = Apply(TEXT("CampaignHackPrompt"), ComputeHandoverTerminalPrompt);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationSpeaker"), ComputeHandoverSpeaker);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationText"), ComputeHandoverTerminalLine);
		if (Error.IsEmpty())
		{
			FProperty* Prop = FindInstanceProperty(Terminal, TEXT("CompletionNotificationDurationSeconds"));
			FString JsonError;
			if (!Prop || !SetPropertyFromJson(Terminal, Prop, MakeShared<FJsonValueNumber>(ComputeHandoverLineDuration), JsonError))
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
			if (!NeuroGeneticsMissionWriteEnumByName(Terminal, Terminal->GetClass(), TEXT("PowerSector"), TEXT("Compute"), Error))
			{
				return Error;
			}
			Error.Reset();
		}
		return Error;
	}

	FString ApplyComputeHandoverDatapad(AActor* Pad)
	{
		auto Apply = [Pad](const TCHAR* Name, const TCHAR* Value) { return SetNamedPropertyFromString(Pad, Name, Value); };
		FString Error = Apply(TEXT("RequiredObjectiveIdForInteraction"), ComputeHandoverConfessionObjectiveId);
		if (Error.IsEmpty()) Error = Apply(TEXT("ObjectiveEventId"), ComputeHandoverConfessionEventId);
		if (Error.IsEmpty()) Error = Apply(TEXT("InteractionPrompt"), ComputeHandoverDatapadPrompt);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationSpeaker"), ComputeHandoverSpeaker);
		if (Error.IsEmpty()) Error = Apply(TEXT("CompletionNotificationText"), ComputeHandoverDatapadLine);
		if (Error.IsEmpty())
		{
			FProperty* Prop = FindInstanceProperty(Pad, TEXT("CompletionNotificationDurationSeconds"));
			FString JsonError;
			if (!Prop || !SetPropertyFromJson(Pad, Prop, MakeShared<FJsonValueNumber>(ComputeHandoverLineDuration), JsonError))
			{
				Error = JsonError.IsEmpty() ? TEXT("CompletionNotificationDurationSeconds missing") : JsonError;
			}
		}
		if (Error.IsEmpty())
		{
			FBoolProperty* Prop = CastField<FBoolProperty>(FindInstanceProperty(Pad, TEXT("bBroadcastGenericDataPadEvent")));
			if (!Prop)
			{
				Error = TEXT("bBroadcastGenericDataPadEvent missing");
			}
			else
			{
				Prop->SetPropertyValue_InContainer(Pad, false);
			}
		}
		return Error;
	}

	TSharedRef<FJsonObject> ExecuteConfigureComputeHandoverActors(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_compute_handover_terminals_and_datapad must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureComputeHandoverActors(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		UWorld* World = GetEditorWorld();
		AActor* Terminals[3] = { nullptr, nullptr, nullptr };
		AActor* Pad = nullptr;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (const FString Error = ComputeHandoverRequireActor(World, ComputeHandoverTerminalLabels[Index], ComputeHandoverTerminalLocations[Index], TEXT("ProjectOrganoidTerminal"), Terminals[Index]); !Error.IsEmpty())
			{
				Change.Status = TEXT("execute_aborted_preflight");
				return FailAudit(TEXT("missing"), Error, MakeShared<FBridgeChange>(Change));
			}
		}
		if (const FString Error = ComputeHandoverRequireActor(World, ComputeHandoverDatapadLabel, ComputeHandoverDatapadLocation, TEXT("ProjectOrganoidDataPad"), Pad); !Error.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), Error, MakeShared<FBridgeChange>(Change));
		}
		const bool bAlready = ComputeHandoverTerminalConfigured(Terminals[0]) && ComputeHandoverTerminalConfigured(Terminals[1]) && ComputeHandoverTerminalConfigured(Terminals[2]) && ComputeHandoverDatapadConfigured(Pad);
		if (bAlready)
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("configured"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetNumberField(TEXT("actor_count"), 4);
			Change.After->SetStringField(TEXT("compute_power"), TEXT("Online"));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		const FVector LocationsBefore[] = {
			Terminals[0]->GetActorLocation(), Terminals[1]->GetActorLocation(), Terminals[2]->GetActorLocation(), Pad->GetActorLocation()
		};
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "ComputeHandoverActors", "Configure Compute handover terminals and datapad"));
			FString WriteError;
			for (int32 Index = 0; Index < 3 && WriteError.IsEmpty(); ++Index)
			{
				WriteError = ApplyComputeHandoverTerminal(Terminals[Index]);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyComputeHandoverDatapad(Pad);
			}
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}
			for (AActor* Terminal : Terminals)
			{
				Terminal->MarkPackageDirty();
			}
			Pad->MarkPackageDirty();
		}
		const bool bConfigured = ComputeHandoverTerminalConfigured(Terminals[0]) && ComputeHandoverTerminalConfigured(Terminals[1]) && ComputeHandoverTerminalConfigured(Terminals[2]) && ComputeHandoverDatapadConfigured(Pad);
		const bool bUnmoved = Terminals[0]->GetActorLocation().Equals(LocationsBefore[0], 0.1f)
			&& Terminals[1]->GetActorLocation().Equals(LocationsBefore[1], 0.1f)
			&& Terminals[2]->GetActorLocation().Equals(LocationsBefore[2], 0.1f)
			&& Pad->GetActorLocation().Equals(LocationsBefore[3], 0.1f);
		if (!bConfigured || !bUnmoved || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Compute handover actors were not configured in place."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetNumberField(TEXT("actor_count"), 4);
		Change.After->SetStringField(TEXT("compute_power"), TEXT("Online"));
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("unlocks"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
