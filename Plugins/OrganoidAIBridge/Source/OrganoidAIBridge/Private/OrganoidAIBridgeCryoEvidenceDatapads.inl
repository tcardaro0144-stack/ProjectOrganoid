// Configure the three existing Cryo evidence datapads. Does not move them, rewrite their logs, save, or change power.
	const TCHAR* CryoEvidenceDatapadsSpec = TEXT("cryo_evidence_datapads_v1");
	const TCHAR* CryoEvidenceDatapadsAction = TEXT("configure_cryo_evidence_datapads");
	const TCHAR* CryoEvidenceDatapadPrompt = TEXT("Recover Cryo Evidence");
	const TCHAR* CryoEvidenceDatapadSpeaker = TEXT("Nathan");
	const TCHAR* CryoEvidenceDatapadLine = TEXT("Lot numbers, consent forms... These weren't specimens. They were staff. Authorization was filed before anyone died.");
	const double CryoEvidenceDatapadDuration = 7.0;
	const TCHAR* CryoEvidencePadLabels[] = {
		TEXT("DataPad_SpecimenManifest"),
		TEXT("DataPad_ConsentForms"),
		TEXT("DataPad_SterlingCryoNote")
	};
	const FVector CryoEvidencePadLocations[] = {
		FVector(-2330.f, -2150.f, -2310.f),
		FVector(-400.f, -1150.f, -2310.f),
		FVector(-2425.f, 1025.f, -2310.f)
	};

	bool CryoEvidenceDatapad_IsFullyConfigured(AActor* Actor)
	{
		return Actor
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("RequiredObjectiveIdForInteraction"), CryoEvidenceObjectiveId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ObjectiveEventId"), CryoEvidenceEventId).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), CryoEvidenceDatapadPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationSpeaker"), CryoEvidenceDatapadSpeaker).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("CompletionNotificationText"), CryoEvidenceDatapadLine).IsEmpty()
			&& NeuroBackupPowerRestore_CheckFloat(Actor, TEXT("CompletionNotificationDurationSeconds"), CryoEvidenceDatapadDuration).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bBroadcastGenericDataPadEvent"), false).IsEmpty();
	}

	FString CryoEvidenceFindPads(UWorld* World, AActor* OutPads[3])
	{
		for (int32 Index = 0; Index < 3; ++Index)
		{
			OutPads[Index] = nullptr;
			const TArray<AActor*> Matches = FindByExactLabel(World, CryoEvidencePadLabels[Index]);
			if (Matches.Num() != 1 || !Matches[0])
			{
				return FString::Printf(TEXT("%s must be unique. count=%d."), CryoEvidencePadLabels[Index], Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), CryoPackage))
			{
				return FString::Printf(TEXT("%s is not on SL_Epitope_Cryo."), CryoEvidencePadLabels[Index]);
			}
			if (!Matches[0]->GetActorLocation().Equals(CryoEvidencePadLocations[Index], 1.f))
			{
				return FString::Printf(TEXT("%s moved. ZERO writes."), CryoEvidencePadLabels[Index]);
			}
			if (!ClassName(Matches[0]).Contains(TEXT("ProjectOrganoidDataPad")))
			{
				return FString::Printf(TEXT("%s is not a data pad."), CryoEvidencePadLabels[Index]);
			}
			OutPads[Index] = Matches[0];
		}
		return TEXT("");
	}

	FString PreflightConfigureCryoEvidenceDatapads(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_cryo_evidence_datapads does not save.");
		}
		if (const FString EntryNext = NeuroAdaptationConnectionNextRevelationReadNext(FindCryoEntryMissionAssetExact());
			!EntryNext.Equals(CryoEvidenceMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_CryoEntry.NextMissionAsset must already be DA_Mission_CryoEvidence.");
		}
		if (const FString EvidenceMismatch = CryoEvidenceMissionMismatchReason(FindCryoEvidenceMissionAssetExact()); !EvidenceMismatch.IsEmpty())
		{
			return EvidenceMismatch;
		}
		AActor* Pads[3] = {};
		if (const FString PadError = CryoEvidenceFindPads(GetEditorWorld(), Pads); !PadError.IsEmpty())
		{
			return PadError;
		}
		bool bAlreadyExact = true;
		for (AActor* Pad : Pads)
		{
			bAlreadyExact = bAlreadyExact && CryoEvidenceDatapad_IsFullyConfigured(Pad);
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), CryoEvidenceDatapadsAction);
		Proposed->SetStringField(TEXT("required_objective"), CryoEvidenceObjectiveId);
		Proposed->SetStringField(TEXT("event"), CryoEvidenceEventId);
		Proposed->SetStringField(TEXT("prompt"), CryoEvidenceDatapadPrompt);
		Proposed->SetStringField(TEXT("line"), CryoEvidenceDatapadLine);
		Proposed->SetNumberField(TEXT("duration_seconds"), CryoEvidenceDatapadDuration);
		Proposed->SetNumberField(TEXT("actor_count"), 3);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves_actor"), false);
		Proposed->SetBoolField(TEXT("unlocks_cryo"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureCryoEvidenceDatapads(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_cryo_evidence_datapads must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureCryoEvidenceDatapads(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		AActor* Pads[3] = {};
		if (const FString PadError = CryoEvidenceFindPads(GetEditorWorld(), Pads); !PadError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), PadError, MakeShared<FBridgeChange>(Change));
		}
		bool bAlreadyExact = true;
		FVector Locations[3];
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Locations[Index] = Pads[Index]->GetActorLocation();
			bAlreadyExact = bAlreadyExact && CryoEvidenceDatapad_IsFullyConfigured(Pads[Index]);
		}
		if (!bAlreadyExact)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CryoEvidenceDatapads", "Configure Cryo evidence datapads"));
			for (AActor* Pad : Pads)
			{
				auto Apply = [Pad](const TCHAR* Name, const TCHAR* Value) { return SetNamedPropertyFromString(Pad, Name, Value); };
				FString WriteError = Apply(TEXT("RequiredObjectiveIdForInteraction"), CryoEvidenceObjectiveId);
				if (WriteError.IsEmpty()) WriteError = Apply(TEXT("ObjectiveEventId"), CryoEvidenceEventId);
				if (WriteError.IsEmpty()) WriteError = Apply(TEXT("InteractionPrompt"), CryoEvidenceDatapadPrompt);
				if (WriteError.IsEmpty()) WriteError = Apply(TEXT("CompletionNotificationSpeaker"), CryoEvidenceDatapadSpeaker);
				if (WriteError.IsEmpty()) WriteError = Apply(TEXT("CompletionNotificationText"), CryoEvidenceDatapadLine);
				if (WriteError.IsEmpty())
				{
					FProperty* Prop = FindInstanceProperty(Pad, TEXT("CompletionNotificationDurationSeconds"));
					FString Error;
					if (!Prop || !SetPropertyFromJson(Pad, Prop, MakeShared<FJsonValueNumber>(CryoEvidenceDatapadDuration), Error))
					{
						WriteError = Error.IsEmpty() ? TEXT("duration missing") : Error;
					}
				}
				if (WriteError.IsEmpty())
				{
					FProperty* Prop = FindInstanceProperty(Pad, TEXT("bBroadcastGenericDataPadEvent"));
					FString Error;
					if (!Prop || !SetPropertyFromJson(Pad, Prop, MakeShared<FJsonValueBoolean>(false), Error))
					{
						WriteError = Error.IsEmpty() ? TEXT("generic event missing") : Error;
					}
				}
				if (!WriteError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
				}
				Pad->MarkPackageDirty();
			}
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (!CryoEvidenceDatapad_IsFullyConfigured(Pads[Index]) || !Pads[Index]->GetActorLocation().Equals(Locations[Index], 0.1f))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("verify_failed"), TEXT("A Cryo evidence datapad was not configured in place."), MakeShared<FBridgeChange>(Change));
			}
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
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), !bAlreadyExact);
		Change.After->SetNumberField(TEXT("actor_count"), 3);
		Change.After->SetStringField(TEXT("cryo_power"), TEXT("Blackout"));
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
