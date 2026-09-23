	// Configure Host_Neuro_Researcher for Beat 8 targeting lesson — configure_neuro_researcher_targeting_why.
	const TCHAR* NeuroResearcherTargetingWhySpec = TEXT("neuro_researcher_targeting_why_v1");
	const TCHAR* NeuroResearcherTargetingWhyAction = TEXT("configure_neuro_researcher_targeting_why");
	const TCHAR* NeuroResearcherTargetingWhyLabel = TEXT("Host_Neuro_Researcher");
	const TCHAR* NeuroResearcherTargetingWhyRequiredObjective = TEXT("Obj_ImpairHostLocomotorNerves");
	const TCHAR* NeuroResearcherTargetingWhyWeakPoint = TEXT("LocomotorNerves");
	const TCHAR* NeuroResearcherTargetingWhyEvent = TEXT("Event_NeuroLocomotorTargetDemonstrated");
	const TCHAR* NeuroResearcherTargetingWhySpeaker = TEXT("Nathan");
	const TCHAR* NeuroResearcherTargetingWhyLine =
		TEXT("That matches the evidence. Target the locomotor nerves, and the whole body slows with them.");
	constexpr double NeuroResearcherTargetingWhyDuration = 7.0;

	FString NeuroResearcherTargetingWhy_CheckFloat(AActor* Actor, const TCHAR* PropertyName, double Expected, double Tol = 0.01)
	{
		FProperty* Prop = FindInstanceProperty(Actor, PropertyName);
		if (!Prop)
		{
			return FString::Printf(TEXT("%s missing"), PropertyName);
		}
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
		{
			const float Live = FloatProp->GetPropertyValue_InContainer(Actor);
			if (!FMath::IsNearlyEqual(static_cast<double>(Live), Expected, Tol))
			{
				return FString::Printf(TEXT("%s expected %g got %g"), PropertyName, Expected, Live);
			}
			return TEXT("");
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
		{
			const double Live = DoubleProp->GetPropertyValue_InContainer(Actor);
			if (!FMath::IsNearlyEqual(Live, Expected, Tol))
			{
				return FString::Printf(TEXT("%s expected %g got %g"), PropertyName, Expected, Live);
			}
			return TEXT("");
		}
		FString Error;
		if (!PropertyMatchesJson(Actor, Prop, MakeShared<FJsonValueNumber>(Expected), Error))
		{
			return FString::Printf(TEXT("%s: %s"), PropertyName, *Error);
		}
		return TEXT("");
	}

	FString NeuroResearcherTargetingWhy_CheckBool(AActor* Actor, const TCHAR* PropertyName, bool Expected)
	{
		FProperty* Prop = FindInstanceProperty(Actor, PropertyName);
		if (!Prop)
		{
			return FString::Printf(TEXT("%s missing"), PropertyName);
		}
		FString Error;
		if (!PropertyMatchesJson(Actor, Prop, MakeShared<FJsonValueBoolean>(Expected), Error))
		{
			return FString::Printf(TEXT("%s: %s"), PropertyName, *Error);
		}
		return TEXT("");
	}

	FString NeuroResearcherTargetingWhy_MismatchReason(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Host_Neuro_Researcher missing.");
		}
		if (!Actor->GetActorLabel().Equals(NeuroResearcherTargetingWhyLabel, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("label '%s' is not Host_Neuro_Researcher"), *Actor->GetActorLabel());
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("RequiredActiveObjectiveId"), NeuroResearcherTargetingWhyRequiredObjective);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckEnum(
				Actor, TEXT("RequiredLessonWeakPoint"), NeuroResearcherTargetingWhyWeakPoint);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckBool(Actor, TEXT("bLessonRequiresTacticalMode"), true);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("LessonSuccessObjectiveEventId"), NeuroResearcherTargetingWhyEvent);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("LessonCompletedObjectiveReplayGuardId"), NeuroResearcherTargetingWhyRequiredObjective);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("LessonSuccessNotificationSpeaker"), NeuroResearcherTargetingWhySpeaker);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerFailureDiscovery_CheckNameOrText(
				Actor, TEXT("LessonSuccessNotificationText"), NeuroResearcherTargetingWhyLine);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckFloat(
				Actor, TEXT("LessonSuccessNotificationDurationSeconds"), NeuroResearcherTargetingWhyDuration);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckBool(Actor, TEXT("bRequiresEncounterActivation"), true);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroResearcherTargetingWhy_CheckBool(Actor, TEXT("bAllowPhaseShiftMutations"), false);
			!Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	bool NeuroResearcherTargetingWhy_IsFullyConfigured(AActor* Actor)
	{
		return NeuroResearcherTargetingWhy_MismatchReason(Actor).IsEmpty();
	}

	FString NeuroResearcherTargetingWhy_RequireNeuroPackage(AActor* Actor)
	{
		if (!Actor || !Actor->GetLevel() || !Actor->GetLevel()->GetOutermost())
		{
			return TEXT("Host_Neuro_Researcher level/package missing.");
		}
		const FString PackageName = Actor->GetLevel()->GetOutermost()->GetName();
		if (!PackagesEqual(PackageName, TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics")))
		{
			return FString::Printf(
				TEXT("Host_Neuro_Researcher owning package '%s' is not SL_Epitope_NeuroGenetics."),
				*PackageName);
		}
		return TEXT("");
	}

	FString PreflightConfigureNeuroResearcherTargetingWhy(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!IsInGameThread())
		{
			return TEXT("configure_neuro_researcher_targeting_why must run on the game thread.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before configuring Host_Neuro_Researcher.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. configure_neuro_researcher_targeting_why does not save.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroResearcherTargetingWhySpec);
		if (!Spec.Equals(NeuroResearcherTargetingWhySpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be '%s'."), NeuroResearcherTargetingWhySpec);
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			return TEXT("Editor world missing.");
		}
		if (const FString KeepError = NeuroPowerFailureDiscovery_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		UObject* TargetingMission = FindNeuroTargetingWhyMissionAssetExact();
		if (!TargetingMission || !NeuroTargetingWhyMissionMismatchReason(TargetingMission).IsEmpty())
		{
			return TEXT("DA_Mission_NeuroTargetingWhy must exist and match locked contract before Host configure.");
		}
		UObject* PowerMission = FindNeuroPowerRestoreMissionAssetExact();
		if (!PowerMission || !NeuroPowerRestoreMissionMismatchReason(PowerMission).IsEmpty())
		{
			return TEXT("DA_Mission_NeuroPowerRestore must exist and match locked contract before Host configure.");
		}
		const FString LiveNext = NeuroPowerRestoreNextTargetingWhyReadNext(PowerMission);
		if (!LiveNext.Equals(NeuroPowerRestoreNextTargetingWhySoftPath, ESearchCase::CaseSensitive))
		{
			return TEXT("DA_Mission_NeuroPowerRestore.NextMissionAsset must already point at DA_Mission_NeuroTargetingWhy.");
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearcherTargetingWhyLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), NeuroResearcherTargetingWhyLabel, Matches.Num());
		}
		AActor* Host = Matches[0];
		if (const FString PackageError = NeuroResearcherTargetingWhy_RequireNeuroPackage(Host); !PackageError.IsEmpty())
		{
			return PackageError;
		}

		const bool bAlreadyExact = NeuroResearcherTargetingWhy_IsFullyConfigured(Host);
		Before->SetStringField(TEXT("spec"), NeuroResearcherTargetingWhySpec);
		Before->SetStringField(TEXT("action"), NeuroResearcherTargetingWhyAction);
		Before->SetStringField(TEXT("label"), NeuroResearcherTargetingWhyLabel);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);

		Proposed->SetStringField(TEXT("spec"), NeuroResearcherTargetingWhySpec);
		Proposed->SetStringField(TEXT("action"), NeuroResearcherTargetingWhyAction);
		Proposed->SetStringField(TEXT("label"), NeuroResearcherTargetingWhyLabel);
		Proposed->SetStringField(TEXT("RequiredActiveObjectiveId"), NeuroResearcherTargetingWhyRequiredObjective);
		Proposed->SetStringField(TEXT("RequiredLessonWeakPoint"), NeuroResearcherTargetingWhyWeakPoint);
		Proposed->SetStringField(TEXT("LessonSuccessObjectiveEventId"), NeuroResearcherTargetingWhyEvent);
		Proposed->SetStringField(TEXT("LessonSuccessNotificationSpeaker"), NeuroResearcherTargetingWhySpeaker);
		Proposed->SetStringField(TEXT("LessonSuccessNotificationText"), NeuroResearcherTargetingWhyLine);
		Proposed->SetNumberField(TEXT("LessonSuccessNotificationDurationSeconds"), NeuroResearcherTargetingWhyDuration);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("will_mutate"), !bAlreadyExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteConfigureNeuroResearcherTargetingWhy(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("configure_neuro_researcher_targeting_why must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightConfigureNeuroResearcherTargetingWhy(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		TArray<AActor*> Matches = FindByExactLabel(World, NeuroResearcherTargetingWhyLabel);
		if (Matches.Num() != 1)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("not_found"),
				TEXT("Host_Neuro_Researcher vanished. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}
		AActor* Host = Matches[0];

		if (NeuroResearcherTargetingWhy_IsFullyConfigured(Host))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), NeuroResearcherTargetingWhyLabel);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"NeuroResearcherTargetingWhy",
				"Configure Neuro Researcher targeting-why lesson"));

			auto ApplyString = [Host](const TCHAR* Name, const TCHAR* Value) -> FString
			{
				return SetNamedPropertyFromString(Host, Name, Value);
			};
			auto ApplyFloat = [Host](const TCHAR* Name, double Value) -> FString
			{
				FProperty* Prop = FindInstanceProperty(Host, Name);
				if (!Prop)
				{
					return FString::Printf(TEXT("%s missing"), Name);
				}
				FString Error;
				if (!SetPropertyFromJson(Host, Prop, MakeShared<FJsonValueNumber>(Value), Error))
				{
					return Error;
				}
				return TEXT("");
			};
			auto ApplyBool = [Host](const TCHAR* Name, bool Value) -> FString
			{
				FProperty* Prop = FindInstanceProperty(Host, Name);
				if (!Prop)
				{
					return FString::Printf(TEXT("%s missing"), Name);
				}
				FString Error;
				if (!SetPropertyFromJson(Host, Prop, MakeShared<FJsonValueBoolean>(Value), Error))
				{
					return Error;
				}
				return TEXT("");
			};

			FString WriteError;
			WriteError = ApplyString(TEXT("RequiredActiveObjectiveId"), NeuroResearcherTargetingWhyRequiredObjective);
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("RequiredLessonWeakPoint"), NeuroResearcherTargetingWhyWeakPoint);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyBool(TEXT("bLessonRequiresTacticalMode"), true);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("LessonSuccessObjectiveEventId"), NeuroResearcherTargetingWhyEvent);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("LessonCompletedObjectiveReplayGuardId"), NeuroResearcherTargetingWhyRequiredObjective);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("LessonSuccessNotificationSpeaker"), NeuroResearcherTargetingWhySpeaker);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyString(TEXT("LessonSuccessNotificationText"), NeuroResearcherTargetingWhyLine);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyFloat(TEXT("LessonSuccessNotificationDurationSeconds"), NeuroResearcherTargetingWhyDuration);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyBool(TEXT("bRequiresEncounterActivation"), true);
			}
			if (WriteError.IsEmpty())
			{
				WriteError = ApplyBool(TEXT("bAllowPhaseShiftMutations"), false);
			}
			if (!WriteError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), WriteError, MakeShared<FBridgeChange>(Change));
			}

			Host->MarkPackageDirty();
		}

		if (const FString VerifyError = NeuroResearcherTargetingWhy_MismatchReason(Host); !VerifyError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("Post-configure verify failed for Host_Neuro_Researcher: %s"), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("configured"));
		Change.After->SetStringField(TEXT("label"), NeuroResearcherTargetingWhyLabel);
		Change.After->SetStringField(TEXT("RequiredActiveObjectiveId"), NeuroResearcherTargetingWhyRequiredObjective);
		Change.After->SetStringField(TEXT("RequiredLessonWeakPoint"), NeuroResearcherTargetingWhyWeakPoint);
		Change.After->SetStringField(TEXT("LessonSuccessObjectiveEventId"), NeuroResearcherTargetingWhyEvent);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
