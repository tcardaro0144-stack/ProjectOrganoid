	// Fixed NeuroGenetics mission Beat 3 expansion —
	// expand_neurogenetics_mission_beat3 / neurogenetics_mission_beat3_v1.
	// Transforms exact Beat 2 (neurogenetics_mission_v1) → exact Beat 3.
	// Preview is mutation-free. Apply modifies only the exact DataAsset; never saves; never repairs mismatches.
	const TCHAR* NeuroGeneticsMissionBeat3Spec = TEXT("neurogenetics_mission_beat3_v1");
	const TCHAR* NeuroGeneticsMissionBeat3Action = TEXT("expand_neurogenetics_mission_beat3");
	const TCHAR* NeuroGeneticsTraceObjectiveId = TEXT("Obj_TraceNeuralMappingSignal");
	const TCHAR* NeuroGeneticsTraceObjectiveTitle = TEXT("Trace the neural mapping signal");
	const TCHAR* NeuroGeneticsTraceObjectiveDescription =
		TEXT("Use the research-floor systems to determine what the array was monitoring.");
	const TCHAR* NeuroGeneticsIsolateEventId = TEXT("Event_NeuroResearchLoadIsolated");

	TSharedRef<FJsonObject> NeuroGeneticsMissionBeat3TaskJson(
		const TCHAR* ObjectiveId,
		const TCHAR* Title,
		const TCHAR* Description,
		bool bAutoActivate,
		const TArray<FString>& Prerequisites,
		const TArray<FString>& EventIds)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), ObjectiveId);
		Task->SetStringField(TEXT("title"), Title);
		Task->SetStringField(TEXT("description"), Description);
		Task->SetStringField(TEXT("category"), NeuroGeneticsObjectiveCategory);
		Task->SetNumberField(TEXT("target_count"), NeuroGeneticsObjectiveTargetCount);
		Task->SetBoolField(TEXT("b_auto_activate"), bAutoActivate);
		TArray<TSharedPtr<FJsonValue>> PrereqJson;
		for (const FString& Id : Prerequisites)
		{
			PrereqJson.Add(MakeShared<FJsonValueString>(Id));
		}
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), PrereqJson);
		TArray<TSharedPtr<FJsonValue>> TriggerJson;
		for (const FString& EventId : EventIds)
		{
			TSharedRef<FJsonObject> Trig = MakeShared<FJsonObject>();
			Trig->SetStringField(TEXT("event_id"), EventId);
			Trig->SetStringField(TEXT("objective_id"), ObjectiveId);
			Trig->SetStringField(TEXT("action"), TEXT("Complete"));
			TriggerJson.Add(MakeShared<FJsonValueObject>(Trig));
		}
		Task->SetArrayField(TEXT("event_triggers"), TriggerJson);
		Task->SetStringField(TEXT("state"), TEXT("Inactive"));
		Task->SetNumberField(TEXT("current_progress"), 0);
		Task->SetBoolField(TEXT("initially_incomplete"), true);
		return Task;
	}

	TSharedRef<FJsonObject> NeuroGeneticsMissionBeat2ProposedTasks()
	{
		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsObjectiveId,
			NeuroGeneticsObjectiveTitle,
			NeuroGeneticsObjectiveDescription,
			true,
			TArray<FString>(),
			TArray<FString>())));
		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroGeneticsMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroGeneticsMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroGeneticsMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetNumberField(TEXT("task_count"), 1);
		Mission->SetStringField(TEXT("shape"), TEXT("beat2"));
		return Mission;
	}

	TSharedRef<FJsonObject> NeuroGeneticsMissionBeat3ProposedTasks(bool bAlreadyExact)
	{
		TArray<FString> IsolateEvents;
		IsolateEvents.Add(NeuroGeneticsIsolateEventId);
		TArray<FString> TracePrereqs;
		TracePrereqs.Add(NeuroGeneticsObjectiveId);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsObjectiveId,
			NeuroGeneticsObjectiveTitle,
			NeuroGeneticsObjectiveDescription,
			true,
			TArray<FString>(),
			IsolateEvents)));
		Tasks.Add(MakeShared<FJsonValueObject>(NeuroGeneticsMissionBeat3TaskJson(
			NeuroGeneticsTraceObjectiveId,
			NeuroGeneticsTraceObjectiveTitle,
			NeuroGeneticsTraceObjectiveDescription,
			true,
			TracePrereqs,
			TArray<FString>())));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroGeneticsMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroGeneticsMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroGeneticsMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetNumberField(TEXT("task_count"), 2);
		Mission->SetStringField(TEXT("shape"), TEXT("beat3"));
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Mission->SetStringField(
			TEXT("runtime_task2"),
			TEXT("Locked/Inactive until Obj_IsolateNeuroResearchLoad completes, then Active via auto-unlock"));
		return Mission;
	}

	FString NeuroGeneticsMissionBeat3RejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
	{
		if (!Args.IsValid())
		{
			return TEXT("");
		}
		static const TCHAR* Rejected[] = {
			TEXT("class"),
			TEXT("class_path"),
			TEXT("path"),
			TEXT("asset_path"),
			TEXT("object_path"),
			TEXT("package"),
			TEXT("mission_id"),
			TEXT("mission_title"),
			TEXT("mission_description"),
			TEXT("next_mission"),
			TEXT("next_mission_asset"),
			TEXT("tasks"),
			TEXT("objective_id"),
			TEXT("event_id"),
			TEXT("event_triggers"),
			TEXT("prerequisites"),
			TEXT("prerequisite_objective_ids"),
			TEXT("copy"),
			TEXT("copy_from"),
			TEXT("source_asset"),
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(
					TEXT("expand_neurogenetics_mission_beat3 rejects client-provided '%s'. Spec neurogenetics_mission_beat3_v1 is fixed."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroGeneticsMissionBeat3Spec);
		if (!Spec.Equals(NeuroGeneticsMissionBeat3Spec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neurogenetics_mission_beat3_v1.");
		}
		return TEXT("");
	}

	bool NeuroGeneticsMissionReadEnumNameEquals(
		void* Container,
		UStruct* Struct,
		const TCHAR* Field,
		const TCHAR* ExpectedName)
	{
		if (!Container || !Struct)
		{
			return false;
		}
		FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Struct, Field);
		if (!EnumProp || !EnumProp->GetEnum())
		{
			return false;
		}
		const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
			EnumProp->ContainerPtrToValuePtr<void>(Container));
		const FString Name = EnumProp->GetEnum()->GetNameStringByValue(Value);
		return Name.Equals(ExpectedName, ESearchCase::IgnoreCase);
	}

	bool NeuroGeneticsMissionWriteEnumByName(
		void* Container,
		UStruct* Struct,
		const TCHAR* Field,
		const TCHAR* ValueName,
		FString& OutError)
	{
		FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Struct, Field);
		if (!EnumProp || !EnumProp->GetEnum())
		{
			OutError = FString::Printf(TEXT("%s enum missing."), Field);
			return false;
		}
		const int64 Value = EnumProp->GetEnum()->GetValueByName(ValueName);
		if (Value == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("%s has no '%s' value."), Field, ValueName);
			return false;
		}
		EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
			EnumProp->ContainerPtrToValuePtr<void>(Container), Value);
		return true;
	}

	FString NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
		void* TaskElem,
		FStructProperty* TaskStruct,
		const TCHAR* ExpectedId,
		const TCHAR* ExpectedTitle,
		const TCHAR* ExpectedDescription,
		bool bExpectAutoActivate,
		int32 ExpectedPrereqCount,
		const TCHAR* ExpectedPrereq0,
		int32 ExpectedTriggerCount,
		const TCHAR* ExpectedEvent0)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing.");
		}

		const FString LiveId = NeuroGeneticsMissionReadName(Objective, ObjectiveProp->Struct, TEXT("ObjectiveId"));
		if (!LiveId.Equals(ExpectedId, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("ObjectiveId '%s' mismatch"), *LiveId);
		}
		const FString LiveTitle = NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Title"));
		if (!LiveTitle.Equals(ExpectedTitle, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Title '%s' mismatch"), *LiveTitle);
		}
		const FString LiveDesc = NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Description"));
		if (!LiveDesc.Equals(ExpectedDescription, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Description '%s' mismatch"), *LiveDesc);
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Main")))
		{
			return TEXT("Type is not Main.");
		}
		if (!NeuroGeneticsMissionReadEnumNameEquals(Objective, ObjectiveProp->Struct, TEXT("State"), TEXT("Inactive")))
		{
			return TEXT("State is not Inactive (initially incomplete).");
		}
		if (FIntProperty* ProgressProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("CurrentProgress")))
		{
			if (ProgressProp->GetPropertyValue_InContainer(Objective) != 0)
			{
				return TEXT("CurrentProgress must be 0.");
			}
		}
		else
		{
			return TEXT("CurrentProgress missing.");
		}
		if (FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress")))
		{
			if (TargetProp->GetPropertyValue_InContainer(Objective) != NeuroGeneticsObjectiveTargetCount)
			{
				return TEXT("TargetProgress must be 1.");
			}
		}
		else
		{
			return TEXT("TargetProgress missing.");
		}

		if (FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds")))
		{
			FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
			if (PrereqHelper.Num() != ExpectedPrereqCount)
			{
				return FString::Printf(
					TEXT("PrerequisiteObjectiveIds count=%d, expected %d"),
					PrereqHelper.Num(),
					ExpectedPrereqCount);
			}
			if (ExpectedPrereqCount == 1 && ExpectedPrereq0)
			{
				FNameProperty* NameInner = CastField<FNameProperty>(PrereqProp->Inner);
				if (!NameInner)
				{
					return TEXT("PrerequisiteObjectiveIds inner is not FName.");
				}
				const FName LivePrereq = NameInner->GetPropertyValue(PrereqHelper.GetRawPtr(0));
				if (!LivePrereq.ToString().Equals(ExpectedPrereq0, ESearchCase::CaseSensitive))
				{
					return FString::Printf(TEXT("Prerequisite[0] '%s' mismatch"), *LivePrereq.ToString());
				}
			}
		}
		else
		{
			return TEXT("PrerequisiteObjectiveIds missing.");
		}

		if (FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate")))
		{
			if (AutoProp->GetPropertyValue_InContainer(TaskElem) != bExpectAutoActivate)
			{
				return TEXT("bAutoActivate mismatch.");
			}
		}
		else
		{
			return TEXT("bAutoActivate missing.");
		}

		if (FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers")))
		{
			FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
			if (TriggerHelper.Num() != ExpectedTriggerCount)
			{
				return FString::Printf(
					TEXT("EventTriggers count=%d, expected %d"),
					TriggerHelper.Num(),
					ExpectedTriggerCount);
			}
			if (ExpectedTriggerCount == 1 && ExpectedEvent0)
			{
				FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
				if (!TrigStruct || !TrigStruct->Struct)
				{
					return TEXT("EventTriggers element struct missing.");
				}
				void* TrigElem = TriggerHelper.GetRawPtr(0);
				const FString LiveEvent = NeuroGeneticsMissionReadName(TrigElem, TrigStruct->Struct, TEXT("EventId"));
				if (!LiveEvent.Equals(ExpectedEvent0, ESearchCase::CaseSensitive))
				{
					return FString::Printf(TEXT("EventTriggers[0].EventId '%s' mismatch"), *LiveEvent);
				}
				const FString LiveObj = NeuroGeneticsMissionReadName(TrigElem, TrigStruct->Struct, TEXT("ObjectiveId"));
				if (!LiveObj.IsEmpty() && !LiveObj.Equals(ExpectedId, ESearchCase::CaseSensitive)
					&& !LiveObj.Equals(TEXT("None"), ESearchCase::IgnoreCase))
				{
					return FString::Printf(TEXT("EventTriggers[0].ObjectiveId '%s' mismatch"), *LiveObj);
				}
				if (!NeuroGeneticsMissionReadEnumNameEquals(TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Complete")))
				{
					return TEXT("EventTriggers[0].Action is not Complete.");
				}
			}
		}
		else if (ExpectedTriggerCount != 0)
		{
			return TEXT("EventTriggers property missing.");
		}

		return TEXT("");
	}

	FString NeuroGeneticsMissionBeat3MismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!NeuroGeneticsMissionClassMatches(Asset))
		{
			return FString::Printf(TEXT("class '%s' is not UProjectOrganoidObjectiveDataAsset"), *ClassName(Asset));
		}
		if (!PackagesEqual(Asset->GetOutermost() ? Asset->GetOutermost()->GetName() : FString(), NeuroGeneticsMissionPackage))
		{
			return FString::Printf(
				TEXT("owning package '%s' is not %s"),
				Asset->GetOutermost() ? *Asset->GetOutermost()->GetName() : TEXT(""),
				NeuroGeneticsMissionPackage);
		}
		if (!Asset->GetName().Equals(NeuroGeneticsMissionAssetName, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("asset name '%s' is not DA_Mission_NeuroGenetics"), *Asset->GetName());
		}
		if (!Asset->GetPathName().Equals(NeuroGeneticsMissionObjectPath, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("object path '%s' is not exact"), *Asset->GetPathName());
		}

		FProperty* MissionIdProp = FindInstanceProperty(Asset, TEXT("MissionId"));
		FString MissionIdError;
		if (!PropertyMatchesJson(Asset, MissionIdProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionId), MissionIdError))
		{
			return FString::Printf(TEXT("MissionId: %s"), *MissionIdError);
		}
		FProperty* TitleProp = FindInstanceProperty(Asset, TEXT("MissionTitle"));
		FString TitleError;
		if (!PropertyMatchesJson(Asset, TitleProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionTitle), TitleError))
		{
			return FString::Printf(TEXT("MissionTitle: %s"), *TitleError);
		}
		FProperty* DescProp = FindInstanceProperty(Asset, TEXT("MissionDescription"));
		FString DescError;
		if (!PropertyMatchesJson(
				Asset, DescProp, MakeShared<FJsonValueString>(NeuroGeneticsMissionDescription), DescError))
		{
			return FString::Printf(TEXT("MissionDescription: %s"), *DescError);
		}

		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			const FSoftObjectPtr Soft = SoftProp->GetPropertyValue_InContainer(Asset);
			if (Soft.ToSoftObjectPath().IsValid())
			{
				return FString::Printf(
					TEXT("NextMissionAsset must be null, got '%s'"),
					*Soft.ToSoftObjectPath().ToString());
			}
		}
		else
		{
			return TEXT("NextMissionAsset soft property missing.");
		}

		FProperty* TasksProp = FindInstanceProperty(Asset, TEXT("Tasks"));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(TasksProp);
		if (!ArrayProp)
		{
			return TEXT("Tasks array missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (Helper.Num() != 2)
		{
			return FString::Printf(TEXT("Tasks count=%d, expected exactly 2"), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}

		if (const FString Task0Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroGeneticsObjectiveId,
				NeuroGeneticsObjectiveTitle,
				NeuroGeneticsObjectiveDescription,
				true,
				0,
				nullptr,
				1,
				NeuroGeneticsIsolateEventId);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0]: %s"), *Task0Error);
		}
		if (const FString Task1Error = NeuroGeneticsMissionBeat3ReadTaskObjectiveMismatch(
				Helper.GetRawPtr(1),
				TaskStruct,
				NeuroGeneticsTraceObjectiveId,
				NeuroGeneticsTraceObjectiveTitle,
				NeuroGeneticsTraceObjectiveDescription,
				true,
				1,
				NeuroGeneticsObjectiveId,
				0,
				nullptr);
			!Task1Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1]: %s"), *Task1Error);
		}
		return TEXT("");
	}

	FString NeuroGeneticsMissionBeat3WriteTaskObjective(
		void* TaskElem,
		FStructProperty* TaskStruct,
		const TCHAR* ObjectiveId,
		const TCHAR* Title,
		const TCHAR* Description,
		bool bAutoActivate,
		const TArray<FName>& Prerequisites,
		const TArray<FName>& EventIds)
	{
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Objective missing during apply.");
		}

		if (FNameProperty* IdProp = FindFProperty<FNameProperty>(ObjectiveProp->Struct, TEXT("ObjectiveId")))
		{
			IdProp->SetPropertyValue_InContainer(Objective, FName(ObjectiveId));
		}
		else
		{
			return TEXT("ObjectiveId missing during apply.");
		}
		if (FTextProperty* TitleProp = FindFProperty<FTextProperty>(ObjectiveProp->Struct, TEXT("Title")))
		{
			TitleProp->SetPropertyValue_InContainer(Objective, FText::FromString(Title));
		}
		else
		{
			return TEXT("Title missing during apply.");
		}
		if (FTextProperty* DescProp = FindFProperty<FTextProperty>(ObjectiveProp->Struct, TEXT("Description")))
		{
			DescProp->SetPropertyValue_InContainer(Objective, FText::FromString(Description));
		}
		else
		{
			return TEXT("Description missing during apply.");
		}

		FString EnumError;
		if (!NeuroGeneticsMissionWriteEnumByName(Objective, ObjectiveProp->Struct, TEXT("Type"), TEXT("Main"), EnumError))
		{
			return EnumError;
		}
		if (!NeuroGeneticsMissionWriteEnumByName(Objective, ObjectiveProp->Struct, TEXT("State"), TEXT("Inactive"), EnumError))
		{
			return EnumError;
		}
		if (FIntProperty* ProgressProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("CurrentProgress")))
		{
			ProgressProp->SetPropertyValue_InContainer(Objective, 0);
		}
		else
		{
			return TEXT("CurrentProgress missing during apply.");
		}
		if (FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress")))
		{
			TargetProp->SetPropertyValue_InContainer(Objective, NeuroGeneticsObjectiveTargetCount);
		}
		else
		{
			return TEXT("TargetProgress missing during apply.");
		}

		if (FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds")))
		{
			FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
			PrereqHelper.Resize(Prerequisites.Num());
			FNameProperty* NameInner = CastField<FNameProperty>(PrereqProp->Inner);
			if (!NameInner)
			{
				return TEXT("PrerequisiteObjectiveIds inner is not FName.");
			}
			for (int32 Index = 0; Index < Prerequisites.Num(); ++Index)
			{
				NameInner->SetPropertyValue(PrereqHelper.GetRawPtr(Index), Prerequisites[Index]);
			}
		}
		else
		{
			return TEXT("PrerequisiteObjectiveIds missing during apply.");
		}

		if (FBoolProperty* AutoUnlock = FindFProperty<FBoolProperty>(ObjectiveProp->Struct, TEXT("bAutoUnlockWhenPrerequisitesMet")))
		{
			AutoUnlock->SetPropertyValue_InContainer(Objective, true);
		}
		if (FBoolProperty* ShowJournal = FindFProperty<FBoolProperty>(ObjectiveProp->Struct, TEXT("bShowInJournal")))
		{
			ShowJournal->SetPropertyValue_InContainer(Objective, true);
		}

		if (FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate")))
		{
			AutoProp->SetPropertyValue_InContainer(TaskElem, bAutoActivate);
		}
		else
		{
			return TEXT("bAutoActivate missing during apply.");
		}

		if (FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers")))
		{
			FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
			TriggerHelper.Resize(EventIds.Num());
			FStructProperty* TrigStruct = CastField<FStructProperty>(TriggersProp->Inner);
			if (!TrigStruct || !TrigStruct->Struct)
			{
				return TEXT("EventTriggers element struct missing during apply.");
			}
			for (int32 Index = 0; Index < EventIds.Num(); ++Index)
			{
				void* TrigElem = TriggerHelper.GetRawPtr(Index);
				if (FNameProperty* EventProp = FindFProperty<FNameProperty>(TrigStruct->Struct, TEXT("EventId")))
				{
					EventProp->SetPropertyValue_InContainer(TrigElem, EventIds[Index]);
				}
				else
				{
					return TEXT("EventTriggers.EventId missing during apply.");
				}
				if (FNameProperty* ObjProp = FindFProperty<FNameProperty>(TrigStruct->Struct, TEXT("ObjectiveId")))
				{
					ObjProp->SetPropertyValue_InContainer(TrigElem, FName(ObjectiveId));
				}
				if (!NeuroGeneticsMissionWriteEnumByName(
						TrigElem, TrigStruct->Struct, TEXT("Action"), TEXT("Complete"), EnumError))
				{
					return EnumError;
				}
			}
		}
		else if (EventIds.Num() > 0)
		{
			return TEXT("EventTriggers missing during apply.");
		}

		return TEXT("");
	}

	FString ApplyNeuroGeneticsMissionBeat3(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("Asset is null.");
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionId"), NeuroGeneticsMissionId); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromString(Asset, TEXT("MissionTitle"), NeuroGeneticsMissionTitle);
			!Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error =
				SetNamedPropertyFromString(Asset, TEXT("MissionDescription"), NeuroGeneticsMissionDescription);
			!Error.IsEmpty())
		{
			return Error;
		}

		FProperty* NextProp = FindInstanceProperty(Asset, TEXT("NextMissionAsset"));
		if (FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(NextProp))
		{
			SoftProp->SetPropertyValue_InContainer(Asset, FSoftObjectPtr());
		}
		else
		{
			return TEXT("NextMissionAsset soft property missing.");
		}

		FProperty* TasksProp = FindInstanceProperty(Asset, TEXT("Tasks"));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(TasksProp);
		if (!ArrayProp)
		{
			return TEXT("Tasks array missing.");
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		Helper.Resize(2);

		TArray<FName> NoPrereqs;
		TArray<FName> IsolateEvents;
		IsolateEvents.Add(FName(NeuroGeneticsIsolateEventId));
		if (const FString Task0Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(0),
				TaskStruct,
				NeuroGeneticsObjectiveId,
				NeuroGeneticsObjectiveTitle,
				NeuroGeneticsObjectiveDescription,
				true,
				NoPrereqs,
				IsolateEvents);
			!Task0Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[0] apply: %s"), *Task0Error);
		}

		TArray<FName> TracePrereqs;
		TracePrereqs.Add(FName(NeuroGeneticsObjectiveId));
		TArray<FName> NoEvents;
		if (const FString Task1Error = NeuroGeneticsMissionBeat3WriteTaskObjective(
				Helper.GetRawPtr(1),
				TaskStruct,
				NeuroGeneticsTraceObjectiveId,
				NeuroGeneticsTraceObjectiveTitle,
				NeuroGeneticsTraceObjectiveDescription,
				true,
				TracePrereqs,
				NoEvents);
			!Task1Error.IsEmpty())
		{
			return FString::Printf(TEXT("Tasks[1] apply: %s"), *Task1Error);
		}
		return TEXT("");
	}

	FString RestoreNeuroGeneticsMissionFromSnapshot(
		UObject* Asset,
		UObject* Snapshot,
		bool bPackageWasDirtyBefore,
		TArray<FString>& OutRestored)
	{
		if (!Asset || !Snapshot)
		{
			return TEXT("Snapshot restore missing asset or snapshot.");
		}
		UEngine::CopyPropertiesForUnrelatedObjects(Snapshot, Asset);
		if (UPackage* Package = Asset->GetOutermost())
		{
			RestorePackageCleanIfWasClean(Package, bPackageWasDirtyBefore, OutRestored);
		}
		return TEXT("");
	}

	FString GuardNeuroGeneticsMissionBeat3EditorContext()
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. expand_neurogenetics_mission_beat3 is an Unreal Editor write.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		return TEXT("");
	}

	FString PreflightExpandNeuroGeneticsMissionBeat3(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroGeneticsMissionBeat3EditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. expand_neurogenetics_mission_beat3 does not save or compile.");
		}
		if (const FString OverrideError = NeuroGeneticsMissionBeat3RejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (!IsInGameThread())
		{
			return TEXT("expand_neurogenetics_mission_beat3 preflight must run on the game thread.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Packages dirty: %s. Require clean packages before expand_neurogenetics_mission_beat3."),
				*FString::Join(Dirty, TEXT(",")));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (!Existing)
		{
			return TEXT("DA_Mission_NeuroGenetics is missing. Persist Beat 2 create_neurogenetics_mission first.");
		}
		if (!NeuroGeneticsMissionClassMatches(Existing))
		{
			return FString::Printf(
				TEXT("Exact path occupied by wrong class '%s'. Fail closed — no automatic repair."),
				*ClassName(Existing));
		}

		const FString Beat2Mismatch = NeuroGeneticsMissionMismatchReason(Existing);
		const FString Beat3Mismatch = NeuroGeneticsMissionBeat3MismatchReason(Existing);
		const bool bBeat2Exact = Beat2Mismatch.IsEmpty();
		const bool bBeat3Exact = Beat3Mismatch.IsEmpty();
		if (!bBeat2Exact && !bBeat3Exact)
		{
			return FString::Printf(
				TEXT("DA_Mission_NeuroGenetics is neither exact Beat 2 nor exact Beat 3. Beat2: %s | Beat3: %s. Fail closed — no opportunistic repair."),
				*Beat2Mismatch,
				*Beat3Mismatch);
		}

		Before->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat3Spec);
		Before->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat3Action);
		Before->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("packages_clean"), true);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("asset_present"), true);
		Before->SetStringField(TEXT("class"), ClassName(Existing));
		Before->SetBoolField(TEXT("beat2_exact"), bBeat2Exact);
		Before->SetBoolField(TEXT("beat3_exact"), bBeat3Exact);
		Before->SetObjectField(
			TEXT("mission_before"),
			bBeat3Exact ? NeuroGeneticsMissionBeat3ProposedTasks(true) : NeuroGeneticsMissionBeat2ProposedTasks());

		Proposed->SetStringField(TEXT("spec"), NeuroGeneticsMissionBeat3Spec);
		Proposed->SetStringField(TEXT("action"), NeuroGeneticsMissionBeat3Action);
		Proposed->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroGeneticsMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat3ProposedTasks(bBeat3Exact));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bBeat3Exact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bBeat3Exact
				? TEXT("DA_Mission_NeuroGenetics already matches neurogenetics_mission_beat3_v1. Execute is a clean no-op. Requires all packages clean. No save.")
				: TEXT("Expand exact Beat 2 DA_Mission_NeuroGenetics to Beat 3: add Event_NeuroResearchLoadIsolated on isolate task and ordered Obj_TraceNeuralMappingSignal. Does not save or compile."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteExpandNeuroGeneticsMissionBeat3(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("expand_neurogenetics_mission_beat3 must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightExpandNeuroGeneticsMissionBeat3(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (!Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("missing"),
				TEXT("DA_Mission_NeuroGenetics missing at execute. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		if (NeuroGeneticsMissionBeat3MismatchReason(Existing).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact Beat 3 found dirty packages: %s. Hard stop. Do not save."),
						*FString::Join(DirtyAfter, TEXT(","))),
					MakeShared<FBridgeChange>(Change));
			}
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
			Change.After->SetBoolField(TEXT("expanded"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroGeneticsMissionBeat3ProposedTasks(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		if (!NeuroGeneticsMissionMismatchReason(Existing).IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroGenetics is not exact Beat 2 at execute. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UPackage* Package = Existing->GetOutermost();
		const bool bPackageWasDirtyBefore = Package && Package->IsDirty();
		UObject* Snapshot = StaticDuplicateObject(
			Existing,
			GetTransientPackage(),
			*FString::Printf(TEXT("%s_Beat2Snapshot"), *Existing->GetName()));
		if (!Snapshot)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("snapshot_failed"),
				TEXT("Failed to snapshot Beat 2 mission for rollback. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT(
				"OrganoidAIBridge",
				"ExpandNeuroGeneticsMissionBeat3",
				"Expand NeuroGenetics Mission Beat 3"));
			if (const FString ApplyError = ApplyNeuroGeneticsMissionBeat3(Existing); !ApplyError.IsEmpty())
			{
				TArray<FString> Restored;
				const FString RestoreError =
					RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(
					TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
				Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
				return FailAudit(
					TEXT("apply_failed"),
					FString::Printf(
						TEXT("%s Rolled back to Beat 2 snapshot. %s ZERO remaining writes."),
						*ApplyError,
						RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
					MakeShared<FBridgeChange>(Change));
			}
			Existing->MarkPackageDirty();
			if (Package)
			{
				Package->MarkPackageDirty();
			}
		}

		if (const FString VerifyError = NeuroGeneticsMissionBeat3MismatchReason(Existing); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			const FString RestoreError =
				RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(
					TEXT("%s Rolled back to Beat 2 snapshot. %s ZERO remaining writes."),
					*VerifyError,
					RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroGeneticsMissionPackage);
		if (DirtyAfter.Num() != 1 || !PackagesEqual(DirtyAfter[0], ExpectedDirty))
		{
			TArray<FString> Restored;
			const FString RestoreError =
				RestoreNeuroGeneticsMissionFromSnapshot(Existing, Snapshot, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("rollback_ok"), RestoreError.IsEmpty());
			return FailAudit(
				TEXT("dirty_package_contract"),
				FString::Printf(
					TEXT("After expand, dirty packages must be exactly [%s], got [%s]. Rolled back. %s"),
					*ExpectedDirty,
					*FString::Join(DirtyAfter, TEXT(",")),
					RestoreError.IsEmpty() ? TEXT("Package state restored.") : *RestoreError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("expanded"));
		Change.After->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Existing));
		Change.After->SetBoolField(TEXT("expanded"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetObjectField(TEXT("mission_before"), NeuroGeneticsMissionBeat2ProposedTasks());
		Change.After->SetObjectField(TEXT("mission_after"), NeuroGeneticsMissionBeat3ProposedTasks(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
