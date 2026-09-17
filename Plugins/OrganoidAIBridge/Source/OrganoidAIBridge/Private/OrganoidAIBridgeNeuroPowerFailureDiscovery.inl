	const TCHAR* NeuroPowerFailureDiscoverySpec = TEXT("neuro_power_failure_discovery_v1");
	const TCHAR* NeuroPowerFailurePanelLabel = TEXT("PowerPanel_NeuroBackup");
	const TCHAR* NeuroPowerFailureClassName = TEXT("ProjectOrganoidPowerPanel");
	const TCHAR* NeuroPowerFailureRestoreEvent = TEXT("Event_NeuroPowerRestored");
	const TCHAR* NeuroPowerFailureDiscoveryEvent = TEXT("Event_NeuroPowerFailureDiscovered");
	const TCHAR* NeuroPowerFailureInspectPrompt = TEXT("Inspect Power Controls");
	const TCHAR* NeuroPowerFailureReviewPrompt = TEXT("Review Power Status");
	const TCHAR* NeuroPowerFailureStatusText = TEXT("PRIMARY FEED OFFLINE — EMERGENCY BACKUP ACTIVE");
	const FVector NeuroPowerFailurePanelLocation(-500.f, -2275.f, -1100.f);
	const FRotator NeuroPowerFailurePanelRotation = FRotator::ZeroRotator;
	const FVector NeuroPowerFailurePanelScale = FVector::OneVector;

	FString NeuroPowerFailureDiscovery_ReadSectorPowerState(UWorld* World, uint8 SectorValue)
	{
		if (!World)
		{
			return TEXT("");
		}
		UClass* PowerClass = StaticLoadClass(
			UWorldSubsystem::StaticClass(),
			nullptr,
			TEXT("/Script/ProjectOrganoid.ProjectOrganoidPowerSubsystem"));
		if (!PowerClass)
		{
			return TEXT("");
		}
		UWorldSubsystem* Power = World->GetSubsystemBase(PowerClass);
		if (!Power)
		{
			return TEXT("");
		}
		UFunction* Function = Power->FindFunction(FName(TEXT("GetSectorPowerState")));
		if (!Function)
		{
			return TEXT("");
		}
		TArray<uint8> Parms;
		Parms.AddZeroed(Function->ParmsSize);
		if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, TEXT("Sector")))
		{
			EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
				EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
				static_cast<int64>(SectorValue));
		}
		else if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(Function, TEXT("Sector")))
		{
			ByteProp->SetPropertyValue(ByteProp->ContainerPtrToValuePtr<void>(Parms.GetData()), SectorValue);
		}
		Power->ProcessEvent(Function, Parms.GetData());
		if (FProperty* ReturnProp = Function->GetReturnProperty())
		{
			void* ValuePtr = ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData());
			if (const FEnumProperty* RetEnum = CastField<FEnumProperty>(ReturnProp))
			{
				const int64 Value = RetEnum->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
				if (UEnum* Enum = RetEnum->GetEnum())
				{
					return Enum->GetNameStringByValue(Value);
				}
			}
			if (const FByteProperty* RetByte = CastField<FByteProperty>(ReturnProp))
			{
				if (UEnum* Enum = RetByte->Enum)
				{
					return Enum->GetNameStringByValue(RetByte->GetSignedIntPropertyValue(ValuePtr));
				}
			}
		}
		return TEXT("");
	}

	FString NeuroPowerFailureDiscovery_RequirePowerSeed(UWorld* World)
	{
		// NeuroGenetics=2, Cryo=3 (EProjectOrganoidPowerSector order).
		const FString Neuro = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2);
		const FString Cryo = NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3);
		if (!Neuro.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("NeuroGenetics power must be Emergency (live '%s'). ZERO writes."), *Neuro);
		}
		if (!Cryo.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase))
		{
			return FString::Printf(TEXT("Cryo power must be Blackout (live '%s'). ZERO writes."), *Cryo);
		}
		return TEXT("");
	}

	FString NeuroPowerFailureDiscovery_CheckNameOrText(AActor* Actor, const TCHAR* PropertyName, const TCHAR* Expected)
	{
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		const FString Live = [&]() -> FString
		{
			if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
			{
				return NameProp->GetPropertyValue_InContainer(Actor).ToString();
			}
			if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
			{
				return TextProp->GetPropertyValue_InContainer(Actor).ToString();
			}
			return TEXT("");
		}();
		if (!Live.Equals(Expected, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("%s live '%s' expected '%s'"), PropertyName, *Live, Expected);
		}
		return TEXT("");
	}

	FString NeuroPowerFailureDiscovery_CheckEnum(AActor* Actor, const TCHAR* PropertyName, const TCHAR* ExpectedInternal)
	{
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		FString Error;
		if (!PropertyMatchesJson(Actor, Property, MakeShared<FJsonValueString>(ExpectedInternal), Error))
		{
			return FString::Printf(TEXT("%s %s"), PropertyName, *Error);
		}
		return TEXT("");
	}

	FString NeuroPowerFailureDiscovery_PanelIdentity(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("panel is null");
		}
		if (!ClassName(Actor).Equals(NeuroPowerFailureClassName, ESearchCase::CaseSensitive)
			&& !ClassName(Actor).Contains(TEXT("ProjectOrganoidPowerPanel")))
		{
			return FString::Printf(TEXT("class '%s' expected ProjectOrganoidPowerPanel"), *ClassName(Actor));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(
			Actor, NeuroPowerFailurePanelLocation, NeuroPowerFailurePanelRotation, NeuroPowerFailurePanelScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		return TEXT("");
	}

	FString NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(AActor* Actor)
	{
		if (const FString Error = NeuroPowerFailureDiscovery_CheckEnum(Actor, TEXT("PowerSector"), TEXT("NeuroGenetics")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckEnum(Actor, TEXT("RestoredState"), TEXT("Online")); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("SuccessObjectiveEventId"), NeuroPowerFailureRestoreEvent); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bHasBeenEngaged"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bHasDiscoveredPowerFailure"), false); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	bool NeuroPowerFailureDiscovery_IsFullyConfigured(AActor* Actor)
	{
		return NeuroPowerFailureDiscovery_PanelIdentity(Actor).IsEmpty()
			&& NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(Actor).IsEmpty()
			&& CheckBoolProperty(Actor, TEXT("bDiscoverPowerFailureBeforeRestore"), true).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("InteractionPrompt"), NeuroPowerFailureInspectPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("ReviewPrompt"), NeuroPowerFailureReviewPrompt).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("DiscoveryObjectiveEventId"), NeuroPowerFailureDiscoveryEvent).IsEmpty()
			&& NeuroPowerFailureDiscovery_CheckNameOrText(Actor, TEXT("FailureStatusReport"), NeuroPowerFailureStatusText).IsEmpty();
	}

	FString NeuroPowerFailureDiscovery_KeepList(UWorld* World)
	{
		if (const FString Error = GuardNeuroCh4KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		struct FHostKeep
		{
			const TCHAR* Label;
			FVector Location;
		};
		const FHostKeep Hosts[] = {
			{TEXT("Host_Neuro_1"), FVector(-400.f, 1400.f, -1100.f)},
			{TEXT("Host_Neuro_2"), FVector(400.f, 2150.f, -1100.f)},
			{TEXT("Host_Neuro_3"), FVector(-1950.f, 1025.f, -1100.f)},
			{TEXT("Host_Neuro_Researcher"), FVector(-1200.f, 800.f, -1100.f)},
		};
		for (const FHostKeep& Host : Hosts)
		{
			if (const FString Error = GuardExistingActor(World, Host.Label, Host.Location, NeuroPackage); !Error.IsEmpty())
			{
				return FString::Printf(TEXT("Keep-list %s: %s"), Host.Label, *Error);
			}
		}
		return TEXT("");
	}

	FString PreflightConfigureNeuroPowerFailureDiscovery(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. configure_neuro_power_failure_discovery does not save or compile.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroPowerFailureDiscoverySpec);
		if (!Spec.Equals(NeuroPowerFailureDiscoverySpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_power_failure_discovery_v1.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("configure_neuro_power_failure_discovery preflight must run on the game thread.");
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			return CleanError;
		}
		if (const FString KeepError = NeuroPowerFailureDiscovery_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}
		if (const FString PowerError = NeuroPowerFailureDiscovery_RequirePowerSeed(World); !PowerError.IsEmpty())
		{
			return PowerError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerFailurePanelLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), NeuroPowerFailurePanelLabel, Matches.Num());
		}
		AActor* Panel = Matches[0];
		if (const FString Identity = NeuroPowerFailureDiscovery_PanelIdentity(Panel); !Identity.IsEmpty())
		{
			return Identity;
		}
		if (const FString Wired = NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(Panel); !Wired.IsEmpty())
		{
			return FString::Printf(TEXT("Panel restore-wired preconfig invalid: %s"), *Wired);
		}

		const bool bAlreadyConfigured = NeuroPowerFailureDiscovery_IsFullyConfigured(Panel);
		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroPowerFailureDiscoverySpec);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_configured"), bAlreadyConfigured);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetObjectField(TEXT("actor"), ActorSnapshot(Panel));
		Before->SetStringField(TEXT("neuro_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2));
		Before->SetStringField(TEXT("cryo_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3));
		Proposed->SetStringField(TEXT("label"), NeuroPowerFailurePanelLabel);
		Proposed->SetStringField(TEXT("spec"), NeuroPowerFailureDiscoverySpec);
		Proposed->SetBoolField(TEXT("bDiscoverPowerFailureBeforeRestore"), true);
		Proposed->SetStringField(TEXT("InteractionPrompt"), NeuroPowerFailureInspectPrompt);
		Proposed->SetStringField(TEXT("ReviewPrompt"), NeuroPowerFailureReviewPrompt);
		Proposed->SetStringField(TEXT("DiscoveryObjectiveEventId"), NeuroPowerFailureDiscoveryEvent);
		Proposed->SetStringField(TEXT("FailureStatusReport"), NeuroPowerFailureStatusText);
		Proposed->SetStringField(TEXT("PowerSector"), TEXT("NeuroGenetics"));
		Proposed->SetStringField(TEXT("RestoredState"), TEXT("Online"));
		Proposed->SetStringField(TEXT("SuccessObjectiveEventId"), NeuroPowerFailureRestoreEvent);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("invoke_interact"), false);
		Proposed->SetBoolField(TEXT("set_sector_power"), false);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyConfigured
				? TEXT("PowerPanel_NeuroBackup already matches neuro_power_failure_discovery_v1. Execute is a no-op. Requires all packages clean.")
				: TEXT("Configure PowerPanel_NeuroBackup discovery opt-in only. Does not interact, change power, fire events, save, or compile."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroPowerFailureDiscovery(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("configure_neuro_power_failure_discovery must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroPowerFailureDiscovery(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("packages_dirty"), FString::Printf(TEXT("ZERO writes. %s"), *CleanError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroPowerFailureDiscovery_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("keep_list"), FString::Printf(TEXT("ZERO writes. %s"), *KeepError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PowerError = NeuroPowerFailureDiscovery_RequirePowerSeed(World); !PowerError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("power_seed"), FString::Printf(TEXT("ZERO writes. %s"), *PowerError), MakeShared<FBridgeChange>(Change));
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerFailurePanelLabel);
		if (Matches.Num() != 1)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("PowerPanel_NeuroBackup vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		AActor* Panel = Matches[0];
		if (const FString Identity = NeuroPowerFailureDiscovery_PanelIdentity(Panel); !Identity.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("identity"), FString::Printf(TEXT("ZERO writes. %s"), *Identity), MakeShared<FBridgeChange>(Change));
		}

		if (NeuroPowerFailureDiscovery_IsFullyConfigured(Panel))
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.bExecuted = true;
				Change.ExecutedAt = NowIso();
				Change.bSavePerformed = false;
				Change.Status = TEXT("execute_postcondition_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetBoolField(TEXT("already_configured"), true);
				Change.After->SetBoolField(TEXT("noop"), true);
				Change.After->SetBoolField(TEXT("save_performed"), false);
				Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
				LogAudit(TEXT("execute"), Change);
				return FailAudit(
					TEXT("unexpected_dirty_packages"),
					FString::Printf(
						TEXT("No-op configure found dirty packages after zero mutation: %s. Hard stop. Do not save any map."),
						*FormatPackageList(DirtyAfter)),
					MakeShared<FBridgeChange>(Change));
			}

			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("already_configured"), true);
			Change.After->SetBoolField(TEXT("noop"), true);
			Change.After->SetBoolField(TEXT("mutation"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetBoolField(TEXT("packages_clean"), true);
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Panel));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		if (const FString Wired = NeuroPowerFailureDiscovery_RequireRestoreWiredPreconfig(Panel); !Wired.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preconfig"), FString::Printf(TEXT("ZERO writes. %s"), *Wired), MakeShared<FBridgeChange>(Change));
		}

		struct FRollbackValue
		{
			FString PropertyName;
			bool bIsBool = false;
			bool bBoolValue = false;
			FString StringValue;
		};
		TArray<FRollbackValue> Rollback;

		auto CaptureBool = [&](const TCHAR* Name) -> FString
		{
			FProperty* Property = FindInstanceProperty(Panel, Name);
			const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property);
			if (!BoolProp)
			{
				return FString::Printf(TEXT("Missing bool '%s' for rollback."), Name);
			}
			FRollbackValue Entry;
			Entry.PropertyName = Name;
			Entry.bIsBool = true;
			Entry.bBoolValue = BoolProp->GetPropertyValue_InContainer(Panel);
			Rollback.Add(Entry);
			return TEXT("");
		};
		auto CaptureString = [&](const TCHAR* Name) -> FString
		{
			FProperty* Property = FindInstanceProperty(Panel, Name);
			if (!Property)
			{
				return FString::Printf(TEXT("Missing property '%s' for rollback."), Name);
			}
			FRollbackValue Entry;
			Entry.PropertyName = Name;
			if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
			{
				Entry.StringValue = NameProp->GetPropertyValue_InContainer(Panel).ToString();
			}
			else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
			{
				Entry.StringValue = TextProp->GetPropertyValue_InContainer(Panel).ToString();
			}
			else
			{
				return FString::Printf(TEXT("Property '%s' is not name/text for rollback."), Name);
			}
			Rollback.Add(Entry);
			return TEXT("");
		};
		auto ApplyString = [&](const TCHAR* Name, const FString& Value) -> FString
		{
			if (const FString Cap = CaptureString(Name); !Cap.IsEmpty())
			{
				return Cap;
			}
			return SetNamedPropertyFromString(Panel, Name, Value);
		};
		auto ApplyBool = [&](const TCHAR* Name, bool bValue) -> FString
		{
			if (const FString Cap = CaptureBool(Name); !Cap.IsEmpty())
			{
				return Cap;
			}
			return SetNamedPropertyFromBool(Panel, Name, bValue);
		};
		auto RollbackAll = [&]()
		{
			for (int32 Index = Rollback.Num() - 1; Index >= 0; --Index)
			{
				const FRollbackValue& Entry = Rollback[Index];
				if (Entry.bIsBool)
				{
					SetNamedPropertyFromBool(Panel, *Entry.PropertyName, Entry.bBoolValue);
				}
				else
				{
					SetNamedPropertyFromString(Panel, *Entry.PropertyName, Entry.StringValue);
				}
			}
		};

		const FScopedTransaction Transaction(
			NSLOCTEXT("OrganoidAIBridge", "NeuroPowerFailureDiscovery", "Neuro power-failure discovery panel configure"));
		Panel->Modify();

		FString WriteError = ApplyBool(TEXT("bDiscoverPowerFailureBeforeRestore"), true);
		if (WriteError.IsEmpty())
		{
			WriteError = ApplyString(TEXT("InteractionPrompt"), NeuroPowerFailureInspectPrompt);
		}
		if (WriteError.IsEmpty())
		{
			WriteError = ApplyString(TEXT("ReviewPrompt"), NeuroPowerFailureReviewPrompt);
		}
		if (WriteError.IsEmpty())
		{
			WriteError = ApplyString(TEXT("DiscoveryObjectiveEventId"), NeuroPowerFailureDiscoveryEvent);
		}
		if (WriteError.IsEmpty())
		{
			WriteError = ApplyString(TEXT("FailureStatusReport"), NeuroPowerFailureStatusText);
		}
		if (!WriteError.IsEmpty())
		{
			RollbackAll();
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("write_failed"), FString::Printf(TEXT("%s Rolled back. Maps not saved."), *WriteError), MakeShared<FBridgeChange>(Change));
		}

		Panel->MarkPackageDirty();

		FString VerifyError;
		if (!NeuroPowerFailureDiscovery_IsFullyConfigured(Panel))
		{
			VerifyError = TEXT("Post-write discovery configuration mismatch.");
		}
		if (VerifyError.IsEmpty())
		{
			VerifyError = NeuroPowerFailureDiscovery_RequirePowerSeed(World);
		}
		if (VerifyError.IsEmpty())
		{
			VerifyError = NeuroPowerFailureDiscovery_KeepList(World);
		}
		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const bool bOnlyNeuroDirty = DirtyAfter.Num() == 1 && PackagesEqual(DirtyAfter[0], NeuroPackage);
		if (VerifyError.IsEmpty() && !bOnlyNeuroDirty)
		{
			VerifyError = FString::Printf(
				TEXT("Dirty packages after configure must be NeuroGenetics only. Dirty: %s"),
				*FormatPackageList(DirtyAfter));
		}
		if (!VerifyError.IsEmpty())
		{
			RollbackAll();
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("execute_postcondition_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("rolled_back"), true);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			Change.After->SetStringField(TEXT("verify_error"), VerifyError);
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(
					TEXT("%s Property changes rolled back. Hard stop. Do not save any map. Terminal — do not retry same change_id."),
					*VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("already_configured"), false);
		Change.After->SetBoolField(TEXT("configured"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		Change.After->SetBoolField(TEXT("interact_invoked"), false);
		Change.After->SetBoolField(TEXT("events_fired"), false);
		Change.After->SetStringField(TEXT("neuro_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 2));
		Change.After->SetStringField(TEXT("cryo_power"), NeuroPowerFailureDiscovery_ReadSectorPowerState(World, 3));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Panel));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
