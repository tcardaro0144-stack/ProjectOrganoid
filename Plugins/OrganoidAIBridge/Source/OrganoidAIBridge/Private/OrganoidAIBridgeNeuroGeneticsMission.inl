	// Fixed NeuroGenetics mission DataAsset — create_neurogenetics_mission / neurogenetics_mission_v1.
	// Preview is mutation-free. Apply creates/configures only the exact asset; never saves; never repairs mismatches.
	const TCHAR* NeuroGeneticsMissionSpec = TEXT("neurogenetics_mission_v1");
	const TCHAR* NeuroGeneticsMissionAction = TEXT("create_neurogenetics_mission");
	const TCHAR* NeuroGeneticsMissionPackage = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	const TCHAR* NeuroGeneticsMissionObjectPath =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	const TCHAR* NeuroGeneticsMissionAssetName = TEXT("DA_Mission_NeuroGenetics");
	const TCHAR* NeuroGeneticsMissionClassPath =
		TEXT("/Script/ProjectOrganoid.ProjectOrganoidObjectiveDataAsset");
	const TCHAR* NeuroGeneticsMissionClassName = TEXT("ProjectOrganoidObjectiveDataAsset");
	const TCHAR* NeuroGeneticsMissionId = TEXT("Mission_NeuroGenetics");
	const TCHAR* NeuroGeneticsMissionTitle = TEXT("NeuroGenetics");
	const TCHAR* NeuroGeneticsMissionDescription =
		TEXT("Isolate the unstable research load before reconnecting the primary feed.");
	const TCHAR* NeuroGeneticsObjectiveId = TEXT("Obj_IsolateNeuroResearchLoad");
	const TCHAR* NeuroGeneticsObjectiveTitle = TEXT("Isolate the NeuroGenetics research load");
	const TCHAR* NeuroGeneticsObjectiveDescription =
		TEXT("Find the emergency cutoff feeding the unstable research equipment.");
	const TCHAR* NeuroGeneticsObjectiveCategory = TEXT("Main");
	constexpr int32 NeuroGeneticsObjectiveTargetCount = 1;

	UObject* FindNeuroGeneticsMissionAssetExact()
	{
		if (UObject* Found = StaticFindObject(nullptr, nullptr, NeuroGeneticsMissionObjectPath))
		{
			return Found;
		}
		return StaticLoadObject(UObject::StaticClass(), nullptr, NeuroGeneticsMissionObjectPath);
	}

	bool NeuroGeneticsMissionClassMatches(UObject* Asset)
	{
		return Asset && Asset->GetClass() && Asset->GetClass()->GetPathName().Equals(NeuroGeneticsMissionClassPath);
	}

	TSharedRef<FJsonObject> NeuroGeneticsMissionProposedState(bool bAlreadyExact)
	{
		TSharedRef<FJsonObject> Task = MakeShared<FJsonObject>();
		Task->SetStringField(TEXT("objective_id"), NeuroGeneticsObjectiveId);
		Task->SetStringField(TEXT("title"), NeuroGeneticsObjectiveTitle);
		Task->SetStringField(TEXT("description"), NeuroGeneticsObjectiveDescription);
		Task->SetStringField(TEXT("category"), NeuroGeneticsObjectiveCategory);
		Task->SetNumberField(TEXT("target_count"), NeuroGeneticsObjectiveTargetCount);
		Task->SetBoolField(TEXT("b_auto_activate"), true);
		Task->SetArrayField(TEXT("prerequisite_objective_ids"), TArray<TSharedPtr<FJsonValue>>());
		Task->SetStringField(TEXT("state"), TEXT("Inactive"));
		Task->SetNumberField(TEXT("current_progress"), 0);
		Task->SetBoolField(TEXT("initially_incomplete"), true);

		TArray<TSharedPtr<FJsonValue>> Tasks;
		Tasks.Add(MakeShared<FJsonValueObject>(Task));

		TSharedRef<FJsonObject> Mission = MakeShared<FJsonObject>();
		Mission->SetStringField(TEXT("mission_id"), NeuroGeneticsMissionId);
		Mission->SetStringField(TEXT("mission_title"), NeuroGeneticsMissionTitle);
		Mission->SetStringField(TEXT("mission_description"), NeuroGeneticsMissionDescription);
		Mission->SetField(TEXT("next_mission_asset"), MakeShared<FJsonValueNull>());
		Mission->SetArrayField(TEXT("tasks"), Tasks);
		Mission->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		return Mission;
	}

	FString NeuroGeneticsMissionRejectClientOverrides(const TSharedPtr<FJsonObject>& Args)
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
			TEXT("copy"),
			TEXT("copy_from"),
			TEXT("source_asset"),
		};
		for (const TCHAR* Key : Rejected)
		{
			if (Args->HasField(Key))
			{
				return FString::Printf(
					TEXT("create_neurogenetics_mission rejects client-provided '%s'. Spec neurogenetics_mission_v1 is fixed."),
					Key);
			}
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroGeneticsMissionSpec);
		if (!Spec.Equals(NeuroGeneticsMissionSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neurogenetics_mission_v1.");
		}
		return TEXT("");
	}

	void* NeuroGeneticsMissionTaskElementPtr(UObject* Asset, int32 Index)
	{
		FProperty* TasksProp = FindInstanceProperty(Asset, TEXT("Tasks"));
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(TasksProp);
		if (!Asset || !ArrayProp)
		{
			return nullptr;
		}
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Asset));
		if (!Helper.IsValidIndex(Index))
		{
			return nullptr;
		}
		return Helper.GetRawPtr(Index);
	}

	void* NeuroGeneticsMissionObjectivePtr(void* TaskElem, FStructProperty* TaskStruct)
	{
		if (!TaskElem || !TaskStruct || !TaskStruct->Struct)
		{
			return nullptr;
		}
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!ObjectiveProp || !ObjectiveProp->Struct)
		{
			return nullptr;
		}
		return ObjectiveProp->ContainerPtrToValuePtr<void>(TaskElem);
	}

	FString NeuroGeneticsMissionReadName(void* Container, UStruct* Struct, const TCHAR* Field)
	{
		if (!Container || !Struct)
		{
			return FString();
		}
		if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Struct, Field))
		{
			return NameProp->GetPropertyValue_InContainer(Container).ToString();
		}
		return FString();
	}

	FString NeuroGeneticsMissionReadText(void* Container, UStruct* Struct, const TCHAR* Field)
	{
		if (!Container || !Struct)
		{
			return FString();
		}
		if (FTextProperty* TextProp = FindFProperty<FTextProperty>(Struct, Field))
		{
			return TextProp->GetPropertyValue_InContainer(Container).ToString();
		}
		return FString();
	}

	FString NeuroGeneticsMissionMismatchReason(UObject* Asset)
	{
		if (!Asset)
		{
			return TEXT("missing");
		}
		if (!NeuroGeneticsMissionClassMatches(Asset))
		{
			return FString::Printf(
				TEXT("class '%s' is not UProjectOrganoidObjectiveDataAsset"),
				*ClassName(Asset));
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
		if (Helper.Num() != 1)
		{
			return FString::Printf(TEXT("Tasks count=%d, expected exactly 1"), Helper.Num());
		}
		FStructProperty* TaskStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!TaskStruct || !TaskStruct->Struct)
		{
			return TEXT("Tasks element struct missing.");
		}
		void* TaskElem = Helper.GetRawPtr(0);
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Tasks[0].Objective missing.");
		}

		const FString LiveObjectiveId = NeuroGeneticsMissionReadName(Objective, ObjectiveProp->Struct, TEXT("ObjectiveId"));
		if (!LiveObjectiveId.Equals(NeuroGeneticsObjectiveId, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("ObjectiveId '%s' mismatch"), *LiveObjectiveId);
		}
		const FString LiveTitle = NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Title"));
		if (!LiveTitle.Equals(NeuroGeneticsObjectiveTitle, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Title '%s' mismatch"), *LiveTitle);
		}
		const FString LiveDesc = NeuroGeneticsMissionReadText(Objective, ObjectiveProp->Struct, TEXT("Description"));
		if (!LiveDesc.Equals(NeuroGeneticsObjectiveDescription, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Description '%s' mismatch"), *LiveDesc);
		}

		if (FEnumProperty* TypeProp = FindFProperty<FEnumProperty>(ObjectiveProp->Struct, TEXT("Type")))
		{
			UEnum* Enum = TypeProp->GetEnum();
			const int64 Value = TypeProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
				TypeProp->ContainerPtrToValuePtr<void>(Objective));
			const FString TypeName = Enum ? Enum->GetNameStringByValue(Value) : FString();
			if (!TypeName.Equals(TEXT("Main"), ESearchCase::IgnoreCase)
				&& !(Enum && Enum->GetDisplayNameTextByValue(Value).ToString().Contains(TEXT("Main"))))
			{
				return FString::Printf(TEXT("Category/Type '%s' is not Main"), *TypeName);
			}
		}
		else
		{
			return TEXT("Objective.Type enum missing.");
		}

		if (FEnumProperty* StateProp = FindFProperty<FEnumProperty>(ObjectiveProp->Struct, TEXT("State")))
		{
			UEnum* Enum = StateProp->GetEnum();
			const int64 Value = StateProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
				StateProp->ContainerPtrToValuePtr<void>(Objective));
			const FString StateName = Enum ? Enum->GetNameStringByValue(Value) : FString();
			if (!StateName.Equals(TEXT("Inactive"), ESearchCase::IgnoreCase))
			{
				return FString::Printf(TEXT("State '%s' is not Inactive (initially incomplete)"), *StateName);
			}
		}
		else
		{
			return TEXT("Objective.State enum missing.");
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
			return TEXT("Objective.CurrentProgress missing.");
		}

		if (FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress")))
		{
			if (TargetProp->GetPropertyValue_InContainer(Objective) != NeuroGeneticsObjectiveTargetCount)
			{
				return FString::Printf(
					TEXT("TargetProgress/TargetCount must be %d"),
					NeuroGeneticsObjectiveTargetCount);
			}
		}
		else
		{
			return TEXT("Objective.TargetProgress missing.");
		}

		if (FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds")))
		{
			FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
			if (PrereqHelper.Num() != 0)
			{
				return FString::Printf(TEXT("PrerequisiteObjectiveIds count=%d, expected 0"), PrereqHelper.Num());
			}
		}
		else
		{
			return TEXT("Objective.PrerequisiteObjectiveIds missing.");
		}

		if (FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate")))
		{
			if (!AutoProp->GetPropertyValue_InContainer(TaskElem))
			{
				return TEXT("bAutoActivate must be true.");
			}
		}
		else
		{
			return TEXT("Tasks[0].bAutoActivate missing.");
		}

		if (FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers")))
		{
			FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
			if (TriggerHelper.Num() != 0)
			{
				return FString::Printf(TEXT("EventTriggers count=%d, expected 0 for this mission"), TriggerHelper.Num());
			}
		}

		return TEXT("");
	}

	FString ApplyNeuroGeneticsMissionDefaults(UObject* Asset)
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
		Helper.Resize(1);
		void* TaskElem = Helper.GetRawPtr(0);
		void* Objective = NeuroGeneticsMissionObjectivePtr(TaskElem, TaskStruct);
		FStructProperty* ObjectiveProp = FindFProperty<FStructProperty>(TaskStruct->Struct, TEXT("Objective"));
		if (!Objective || !ObjectiveProp || !ObjectiveProp->Struct)
		{
			return TEXT("Tasks[0].Objective missing during apply.");
		}

		if (FNameProperty* IdProp = FindFProperty<FNameProperty>(ObjectiveProp->Struct, TEXT("ObjectiveId")))
		{
			IdProp->SetPropertyValue_InContainer(Objective, FName(NeuroGeneticsObjectiveId));
		}
		else
		{
			return TEXT("Objective.ObjectiveId missing during apply.");
		}
		if (FTextProperty* TitleProp = FindFProperty<FTextProperty>(ObjectiveProp->Struct, TEXT("Title")))
		{
			TitleProp->SetPropertyValue_InContainer(Objective, FText::FromString(NeuroGeneticsObjectiveTitle));
		}
		else
		{
			return TEXT("Objective.Title missing during apply.");
		}
		if (FTextProperty* DescProp = FindFProperty<FTextProperty>(ObjectiveProp->Struct, TEXT("Description")))
		{
			DescProp->SetPropertyValue_InContainer(Objective, FText::FromString(NeuroGeneticsObjectiveDescription));
		}
		else
		{
			return TEXT("Objective.Description missing during apply.");
		}

		if (FEnumProperty* TypeProp = FindFProperty<FEnumProperty>(ObjectiveProp->Struct, TEXT("Type")))
		{
			UEnum* Enum = TypeProp->GetEnum();
			if (!Enum)
			{
				return TEXT("Objective.Type enum unresolved.");
			}
			const int64 MainValue = Enum->GetValueByName(TEXT("Main"));
			if (MainValue == INDEX_NONE)
			{
				return TEXT("Objective.Type has no Main value.");
			}
			TypeProp->GetUnderlyingProperty()->SetIntPropertyValue(
				TypeProp->ContainerPtrToValuePtr<void>(Objective), MainValue);
		}
		else
		{
			return TEXT("Objective.Type missing during apply.");
		}

		if (FEnumProperty* StateProp = FindFProperty<FEnumProperty>(ObjectiveProp->Struct, TEXT("State")))
		{
			UEnum* Enum = StateProp->GetEnum();
			if (!Enum)
			{
				return TEXT("Objective.State enum unresolved.");
			}
			const int64 InactiveValue = Enum->GetValueByName(TEXT("Inactive"));
			if (InactiveValue == INDEX_NONE)
			{
				return TEXT("Objective.State has no Inactive value.");
			}
			StateProp->GetUnderlyingProperty()->SetIntPropertyValue(
				StateProp->ContainerPtrToValuePtr<void>(Objective), InactiveValue);
		}
		else
		{
			return TEXT("Objective.State missing during apply.");
		}

		if (FIntProperty* ProgressProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("CurrentProgress")))
		{
			ProgressProp->SetPropertyValue_InContainer(Objective, 0);
		}
		else
		{
			return TEXT("Objective.CurrentProgress missing during apply.");
		}
		if (FIntProperty* TargetProp = FindFProperty<FIntProperty>(ObjectiveProp->Struct, TEXT("TargetProgress")))
		{
			TargetProp->SetPropertyValue_InContainer(Objective, NeuroGeneticsObjectiveTargetCount);
		}
		else
		{
			return TEXT("Objective.TargetProgress missing during apply.");
		}

		if (FArrayProperty* PrereqProp = FindFProperty<FArrayProperty>(ObjectiveProp->Struct, TEXT("PrerequisiteObjectiveIds")))
		{
			FScriptArrayHelper PrereqHelper(PrereqProp, PrereqProp->ContainerPtrToValuePtr<void>(Objective));
			PrereqHelper.Resize(0);
		}
		else
		{
			return TEXT("Objective.PrerequisiteObjectiveIds missing during apply.");
		}

		if (FBoolProperty* AutoProp = FindFProperty<FBoolProperty>(TaskStruct->Struct, TEXT("bAutoActivate")))
		{
			AutoProp->SetPropertyValue_InContainer(TaskElem, true);
		}
		else
		{
			return TEXT("Tasks[0].bAutoActivate missing during apply.");
		}

		if (FArrayProperty* TriggersProp = FindFProperty<FArrayProperty>(TaskStruct->Struct, TEXT("EventTriggers")))
		{
			FScriptArrayHelper TriggerHelper(TriggersProp, TriggersProp->ContainerPtrToValuePtr<void>(TaskElem));
			TriggerHelper.Resize(0);
		}

		return TEXT("");
	}

	FString CleanupCreatedNeuroGeneticsMissionAsset(
		UObject* Asset,
		bool bPackageWasDirtyBefore,
		TArray<FString>& OutRestored)
	{
		if (!Asset)
		{
			return TEXT("");
		}
		UPackage* Package = Asset->GetOutermost();
		Asset->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
		const bool bRenamed = Asset->Rename(
			nullptr,
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_NonTransactional | REN_AllowPackageLinkerMismatch);
		if (!bRenamed)
		{
			return TEXT("Failed to relocate created mission asset to transient for cleanup.");
		}
		Asset->MarkAsGarbage();
		if (Package)
		{
			RestorePackageCleanIfWasClean(Package, bPackageWasDirtyBefore, OutRestored);
		}
		if (FindNeuroGeneticsMissionAssetExact() != nullptr)
		{
			return TEXT("Cleanup failed: exact mission object path is still resolvable.");
		}
		return TEXT("");
	}

	FString CreateNeuroGeneticsMissionAsset(UObject*& OutAsset, bool& bOutCreatedNow, bool& bOutPackageWasDirtyBefore)
	{
		OutAsset = nullptr;
		bOutCreatedNow = false;
		bOutPackageWasDirtyBefore = false;

		UClass* MissionClass = LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath);
		if (!MissionClass)
		{
			return TEXT("UProjectOrganoidObjectiveDataAsset class is not loaded.");
		}

		UPackage* ExistingPackage = FindPackage(nullptr, NeuroGeneticsMissionPackage);
		if (ExistingPackage)
		{
			bOutPackageWasDirtyBefore = ExistingPackage->IsDirty();
		}

		UPackage* Package = CreatePackage(NeuroGeneticsMissionPackage);
		if (!Package)
		{
			return TEXT("Failed to create DA_Mission_NeuroGenetics package.");
		}
		if (!ExistingPackage)
		{
			bOutPackageWasDirtyBefore = false;
		}

		UObject* Asset = NewObject<UObject>(
			Package, MissionClass, NeuroGeneticsMissionAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!Asset)
		{
			return TEXT("NewObject DA_Mission_NeuroGenetics returned null.");
		}
		bOutCreatedNow = true;

		if (const FString ApplyError = ApplyNeuroGeneticsMissionDefaults(Asset); !ApplyError.IsEmpty())
		{
			TArray<FString> Restored;
			CleanupCreatedNeuroGeneticsMissionAsset(Asset, bOutPackageWasDirtyBefore, Restored);
			return ApplyError;
		}

		FAssetRegistryModule::AssetCreated(Asset);
		Package->MarkPackageDirty();
		Asset->MarkPackageDirty();
		OutAsset = Asset;
		return TEXT("");
	}

	FString GuardNeuroGeneticsMissionEditorContext()
	{
		if (!GIsEditor || !GEditor)
		{
			return TEXT("Editor context required. create_neurogenetics_mission is an Unreal Editor write.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		return TEXT("");
	}

	FString PreflightCreateNeuroGeneticsMission(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (const FString ContextError = GuardNeuroGeneticsMissionEditorContext(); !ContextError.IsEmpty())
		{
			return ContextError;
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_neurogenetics_mission does not save or compile.");
		}
		if (const FString OverrideError = NeuroGeneticsMissionRejectClientOverrides(Args); !OverrideError.IsEmpty())
		{
			return OverrideError;
		}
		if (!IsInGameThread())
		{
			return TEXT("create_neurogenetics_mission preflight must run on the game thread.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		if (Dirty.Num() != 0)
		{
			return FString::Printf(
				TEXT("Packages dirty: %s. Require clean packages before create_neurogenetics_mission."),
				*FString::Join(Dirty, TEXT(",")));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		bool bAlreadyExact = false;
		if (Existing)
		{
			if (!NeuroGeneticsMissionClassMatches(Existing))
			{
				return FString::Printf(
					TEXT("Exact path occupied by wrong class '%s'. Fail closed — no automatic repair."),
					*ClassName(Existing));
			}
			const FString Mismatch = NeuroGeneticsMissionMismatchReason(Existing);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("DA_Mission_NeuroGenetics exists but mismatches: %s. Fail closed — no opportunistic repair."),
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<UObject>(nullptr, NeuroGeneticsMissionClassPath))
		{
			return TEXT("UProjectOrganoidObjectiveDataAsset class is not loaded.");
		}

		Before->SetStringField(TEXT("spec"), NeuroGeneticsMissionSpec);
		Before->SetStringField(TEXT("action"), NeuroGeneticsMissionAction);
		Before->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Before->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), true);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetBoolField(TEXT("asset_present"), Existing != nullptr);
		if (Existing)
		{
			Before->SetStringField(TEXT("class"), ClassName(Existing));
			Before->SetStringField(TEXT("path"), Existing->GetPathName());
		}

		Proposed->SetStringField(TEXT("spec"), NeuroGeneticsMissionSpec);
		Proposed->SetStringField(TEXT("action"), NeuroGeneticsMissionAction);
		Proposed->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Proposed->SetStringField(TEXT("package"), NeuroGeneticsMissionPackage);
		Proposed->SetStringField(TEXT("asset_name"), NeuroGeneticsMissionAssetName);
		Proposed->SetStringField(TEXT("class"), NeuroGeneticsMissionClassName);
		Proposed->SetObjectField(TEXT("mission"), NeuroGeneticsMissionProposedState(bAlreadyExact));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyExact
				? TEXT("DA_Mission_NeuroGenetics already matches neurogenetics_mission_v1. Execute is a clean no-op. Requires all packages clean. No save.")
				: TEXT("Create one UProjectOrganoidObjectiveDataAsset at /Game/Data/Missions/DA_Mission_NeuroGenetics with Mission_NeuroGenetics and Obj_IsolateNeuroResearchLoad. Does not save or compile."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroGeneticsMission(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("create_neurogenetics_mission must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroGeneticsMission(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Existing = FindNeuroGeneticsMissionAssetExact();
		if (Existing && NeuroGeneticsMissionMismatchReason(Existing).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact asset path found dirty packages: %s. Hard stop. Do not save."),
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
			Change.After->SetBoolField(TEXT("created"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("mission"), NeuroGeneticsMissionProposedState(true));
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DA_Mission_NeuroGenetics exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UObject* Created = nullptr;
		bool bCreatedNow = false;
		bool bPackageWasDirtyBefore = false;
		{
			const FScopedTransaction Transaction(
				NSLOCTEXT("OrganoidAIBridge", "CreateNeuroGeneticsMission", "Create NeuroGenetics Mission DataAsset"));
			if (const FString CreateError = CreateNeuroGeneticsMissionAsset(Created, bCreatedNow, bPackageWasDirtyBefore);
				!CreateError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				Change.After = MakeShared<FJsonObject>();
				Change.After->SetArrayField(
					TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
				return FailAudit(
					TEXT("create_failed"),
					FString::Printf(TEXT("%s ZERO remaining writes."), *CreateError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString VerifyError = NeuroGeneticsMissionMismatchReason(Created); !VerifyError.IsEmpty())
		{
			TArray<FString> Restored;
			const FString CleanupError =
				CleanupCreatedNeuroGeneticsMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("cleanup_ok"), CleanupError.IsEmpty());
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(
					TEXT("%s Created object removed. %s ZERO remaining writes."),
					*VerifyError,
					CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		const FString ExpectedDirty = NormalizePackage(NeuroGeneticsMissionPackage);
		if (DirtyAfter.Num() != 1 || !PackagesEqual(DirtyAfter[0], ExpectedDirty))
		{
			TArray<FString> Restored;
			const FString CleanupError =
				CleanupCreatedNeuroGeneticsMissionAsset(Created, bPackageWasDirtyBefore, Restored);
			Change.Status = TEXT("execute_failed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
			Change.After->SetArrayField(TEXT("restored_clean"), DirtyPackageJsonArray(Restored));
			Change.After->SetBoolField(TEXT("cleanup_ok"), CleanupError.IsEmpty());
			return FailAudit(
				TEXT("dirty_package_contract"),
				FString::Printf(
					TEXT("After create, dirty packages must be exactly [%s], got [%s]. Object removed. %s"),
					*ExpectedDirty,
					*FString::Join(DirtyAfter, TEXT(",")),
					CleanupError.IsEmpty() ? TEXT("Package state restored.") : *CleanupError),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetStringField(TEXT("object_path"), NeuroGeneticsMissionObjectPath);
		Change.After->SetStringField(TEXT("class"), ClassName(Created));
		Change.After->SetBoolField(TEXT("created"), bCreatedNow);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetObjectField(TEXT("mission"), NeuroGeneticsMissionProposedState(false));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
