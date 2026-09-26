// Credit the five existing Cryo and Compute datapads. Does not move them, rewrite their logs, save, or change power.
	const TCHAR* WeaponRosterDatapadsSpec = TEXT("weapon_roster_datapads_v1");
	const TCHAR* WeaponRosterDatapadsAction = TEXT("configure_weapon_roster_armory");
	const TCHAR* WeaponRosterPadLabels[] = {
		TEXT("DataPad_SpecimenManifest"),
		TEXT("DataPad_ConsentForms"),
		TEXT("DataPad_SterlingCryoNote"),
		TEXT("DataPad_AutonomousDecisionLog"),
		TEXT("DataPad_SterlingConfession")
	};
	const TCHAR* WeaponRosterPadPackages[] = {
		CryoPackage, CryoPackage, CryoPackage, ComputePackage, ComputePackage
	};
	const TCHAR* WeaponRosterPadWeaponIds[] = {
		TEXT("Weapon_BioStabilizerPistol"),
		TEXT("Weapon_PulseCarbine"),
		TEXT("Weapon_CryoInjector"),
		TEXT("Weapon_DenaturingShotgun"),
		TEXT("Weapon_IncineratorLance")
	};
	const FVector WeaponRosterPadLocations[] = {
		FVector(-2330.f, -2150.f, -2310.f),
		FVector(-400.f, -1150.f, -2310.f),
		FVector(-2425.f, 1025.f, -2310.f),
		FVector(0.f, -1650.f, -3510.f),
		FVector(-2425.f, -2275.f, -3510.f)
	};

	FString WeaponRosterReadNameOrText(AActor* Actor, const TCHAR* PropertyName)
	{
		if (FNameProperty* NameProp = CastField<FNameProperty>(FindInstanceProperty(Actor, PropertyName)))
		{
			return NameProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		if (FTextProperty* TextProp = CastField<FTextProperty>(FindInstanceProperty(Actor, PropertyName)))
		{
			return TextProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		return TEXT("");
	}

	bool WeaponRosterPadHasWeapon(AActor* Actor, const TCHAR* WeaponId)
	{
		return Actor && NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("WeaponRosterWeaponId"), WeaponId).IsEmpty();
	}

	FString WeaponRosterFindPads(UWorld* World, AActor* OutPads[5])
	{
		for (int32 Index = 0; Index < 5; ++Index)
		{
			OutPads[Index] = nullptr;
			const TArray<AActor*> Matches = FindByExactLabel(World, WeaponRosterPadLabels[Index]);
			if (Matches.Num() != 1 || !Matches[0])
			{
				return FString::Printf(TEXT("%s must be unique. count=%d."), WeaponRosterPadLabels[Index], Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), WeaponRosterPadPackages[Index]))
			{
				return FString::Printf(TEXT("%s is not on its sector map."), WeaponRosterPadLabels[Index]);
			}
			if (!Matches[0]->GetActorLocation().Equals(WeaponRosterPadLocations[Index], 1.f))
			{
				return FString::Printf(TEXT("%s moved. ZERO writes."), WeaponRosterPadLabels[Index]);
			}
			if (!ClassName(Matches[0]).Contains(TEXT("ProjectOrganoidDataPad")))
			{
				return FString::Printf(TEXT("%s is not a data pad."), WeaponRosterPadLabels[Index]);
			}
			OutPads[Index] = Matches[0];
		}
		return TEXT("");
	}

	FString PreflightConfigureWeaponRosterArmory(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (const FString Stable = CryoAccessRequireEditorStable(); !Stable.IsEmpty())
		{
			return Stable;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_weapon_roster_armory does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), WeaponRosterDatapadsSpec);
		if (!Spec.Equals(WeaponRosterDatapadsSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), WeaponRosterDatapadsSpec);
		}
		UObject* Mission = FindWeaponRosterMissionAssetExact();
		if (!Mission || !WeaponRosterMissionMismatchReason(Mission).IsEmpty())
		{
			return TEXT("DA_Mission_WeaponRoster must exist and stay Next null.");
		}
		const FString SyringeNext = NeuroAdaptationConnectionNextRevelationReadNext(FindSyringeKitMissionAssetExact());
		if (!SyringeNext.Equals(WeaponRosterMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_SyringeKit.NextMissionAsset must already be DA_Mission_WeaponRoster.");
		}
		AActor* Pads[5] = {};
		if (const FString PadError = WeaponRosterFindPads(GetEditorWorld(), Pads); !PadError.IsEmpty())
		{
			return PadError;
		}
		if (!CryoEvidenceDatapad_IsFullyConfigured(Pads[0]) || !CryoEvidenceDatapad_IsFullyConfigured(Pads[1]) || !CryoEvidenceDatapad_IsFullyConfigured(Pads[2]))
		{
			return TEXT("Cryo evidence datapads must stay fully configured.");
		}
		if (!ComputeHandoverDatapadConfigured(Pads[4]))
		{
			return TEXT("DataPad_SterlingConfession must stay fully configured.");
		}
		bool bAlreadyExact = true;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			bAlreadyExact = bAlreadyExact && WeaponRosterPadHasWeapon(Pads[Index], WeaponRosterPadWeaponIds[Index]);
		}
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetStringField(TEXT("action"), WeaponRosterDatapadsAction);
		Proposed->SetStringField(TEXT("spec"), WeaponRosterDatapadsSpec);
		Proposed->SetStringField(TEXT("package"), CryoPackage);
		Proposed->SetStringField(TEXT("objective_id"), WeaponRosterObjectiveId);
		Proposed->SetStringField(TEXT("event_id"), WeaponRosterEventId);
		Proposed->SetStringField(TEXT("prompt"), TEXT("Recover Weapon Roster"));
		Proposed->SetNumberField(TEXT("target"), 5);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureWeaponRosterArmory(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_weapon_roster_armory must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureWeaponRosterArmory(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		AActor* Pads[5] = {};
		if (const FString PadError = WeaponRosterFindPads(GetEditorWorld(), Pads); !PadError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), PadError, MakeShared<FBridgeChange>(Change));
		}
		FVector Locations[5];
		FString Prompts[5];
		FString Events[5];
		FString Required[5];
		bool bAlreadyExact = true;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Locations[Index] = Pads[Index]->GetActorLocation();
			Prompts[Index] = WeaponRosterReadNameOrText(Pads[Index], TEXT("InteractionPrompt"));
			Events[Index] = WeaponRosterReadNameOrText(Pads[Index], TEXT("ObjectiveEventId"));
			Required[Index] = WeaponRosterReadNameOrText(Pads[Index], TEXT("RequiredObjectiveIdForInteraction"));
			bAlreadyExact = bAlreadyExact && WeaponRosterPadHasWeapon(Pads[Index], WeaponRosterPadWeaponIds[Index]);
		}
		if (!bAlreadyExact)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "WeaponRosterDatapads", "Credit existing datapads for the weapon roster"));
			for (int32 Index = 0; Index < 5; ++Index)
			{
				if (const FString WriteError = SetNamedPropertyFromString(Pads[Index], TEXT("WeaponRosterWeaponId"), WeaponRosterPadWeaponIds[Index]); !WriteError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
				}
				Pads[Index]->MarkPackageDirty();
			}
		}
		for (int32 Index = 0; Index < 5; ++Index)
		{
			if (!WeaponRosterPadHasWeapon(Pads[Index], WeaponRosterPadWeaponIds[Index])
				|| !Pads[Index]->GetActorLocation().Equals(Locations[Index], 0.1f)
				|| !WeaponRosterReadNameOrText(Pads[Index], TEXT("InteractionPrompt")).Equals(Prompts[Index], ESearchCase::CaseSensitive)
				|| !WeaponRosterReadNameOrText(Pads[Index], TEXT("ObjectiveEventId")).Equals(Events[Index], ESearchCase::CaseSensitive)
				|| !WeaponRosterReadNameOrText(Pads[Index], TEXT("RequiredObjectiveIdForInteraction")).Equals(Required[Index], ESearchCase::CaseSensitive))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("verify_failed"), TEXT("A roster datapad moved or an existing field changed."), MakeShared<FBridgeChange>(Change));
			}
		}
		if (!CryoEvidenceDatapad_IsFullyConfigured(Pads[0]) || !CryoEvidenceDatapad_IsFullyConfigured(Pads[1]) || !CryoEvidenceDatapad_IsFullyConfigured(Pads[2])
			|| !ComputeHandoverDatapadConfigured(Pads[4]) || !CryoAccessRequirePowerContract(GetEditorWorld()).IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("Cryo evidence, Sterling's confession, or power did not stay exact."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("mutated"), !bAlreadyExact);
		Change.After->SetNumberField(TEXT("actor_count"), 5);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		Change.After->SetBoolField(TEXT("moves_actor"), false);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
