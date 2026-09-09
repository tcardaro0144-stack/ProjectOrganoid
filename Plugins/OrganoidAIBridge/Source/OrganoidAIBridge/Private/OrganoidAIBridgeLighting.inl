	const TCHAR* LightControllerBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController");
	const TCHAR* LightControllerBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController");
	const TCHAR* LightControllerLabel = TEXT("Admin_LightController");
	const TCHAR* LightControllerCategory = TEXT("Admin|Lighting");
	const TCHAR* SetLightingZoneName = TEXT("SetLightingZone");
	const TCHAR* LightingSpec = TEXT("s20_admin_lighting_v1");
	const TCHAR* LightingLibraryPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminLightingLibrary");
	const TCHAR* RoomTriggerBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger");
	const TCHAR* RoomTriggerBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger");
	const TCHAR* RoomTriggerParentPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminRoomTrigger");
	const TCHAR* RequestAdminLightingZoneName = TEXT("RequestAdminLightingZone");
	const FVector LightControllerLocation(2280.f, 0.f, 250.f);
	const FRotator LightControllerRotation(0.f, 0.f, 0.f);
	const FVector LightControllerScale(1.f, 1.f, 1.f);
	const float LightingAuthoredIntensity = 5000.f;
	const float LightingInactiveMultiplier = 0.25f;

	struct FLightControllerVarSpec
	{
		const TCHAR* Name;
		const TCHAR* PinCategory;
		const TCHAR* DefaultValue;
	};

	const FLightControllerVarSpec LightControllerVarSpecs[] = {
		{TEXT("CurrentLightingZone"), TEXT("name"), TEXT("None")},
		{TEXT("InactiveMultiplier"), TEXT("real"), TEXT("0.250000")},
		{TEXT("AuthoredIntensity"), TEXT("real"), TEXT("5000.000000")},
	};

	struct FZoneLightSpec
	{
		const TCHAR* RoomID;
		const TCHAR* Label;
		FVector Location;
	};

	const FZoneLightSpec ZoneLightSpecs[] = {
		{TEXT("Vestibule"), TEXT("Admin_ZoneLight_Vestibule"), FVector(400.f, 0.f, 320.f)},
		{TEXT("Reception"), TEXT("Admin_ZoneLight_Reception"), FVector(1500.f, 0.f, 320.f)},
		{TEXT("Hub"), TEXT("Admin_ZoneLight_Hub"), FVector(2280.f, 0.f, 320.f)},
		{TEXT("Security"), TEXT("Admin_ZoneLight_Security"), FVector(2680.f, -400.f, 320.f)},
		{TEXT("Records"), TEXT("Admin_ZoneLight_Records"), FVector(2680.f, 400.f, 320.f)},
		{TEXT("Conference"), TEXT("Admin_ZoneLight_Conference"), FVector(3200.f, -1850.f, 320.f)},
		{TEXT("DirectorSuite"), TEXT("Admin_ZoneLight_DirectorSuite"), FVector(1950.f, -1850.f, 320.f)},
		{TEXT("Operations"), TEXT("Admin_ZoneLight_Operations"), FVector(3705.f, 0.f, 320.f)},
		{TEXT("Transit"), TEXT("Admin_ZoneLight_Transit"), FVector(5005.f, 0.f, 320.f)},
		{TEXT("ServiceCorridor"), TEXT("Admin_ZoneLight_ServiceCorridor"), FVector(3712.5f, -912.5f, 320.f)},
	};

	bool IsLightControllerPackage(const FString& Path)
	{
		const FString Normalized = NormalizePackage(Path);
		return PackagesEqual(Normalized, LightControllerBpPath) || PackagesEqual(Normalized, LightControllerBpPackage);
	}

	const FZoneLightSpec* FindZoneLightSpec(const FString& Label)
	{
		for (const FZoneLightSpec& Spec : ZoneLightSpecs)
		{
			if (Label.Equals(Spec.Label, ESearchCase::CaseSensitive))
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	FString RequireLightControllerBlueprint(const TSharedPtr<FJsonObject>& Args, UBlueprint*& OutBlueprint)
	{
		OutBlueprint = nullptr;
		const FString Path = GetString(Args, TEXT("path"), GetString(Args, TEXT("blueprint")));
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"), LightControllerBpPackage)));
		if (!IsLightControllerPackage(Path))
		{
			return TEXT("This action is allowlisted only for BP_AdminLightController.");
		}
		if (!PackagesEqual(RequiredPackage, LightControllerBpPackage))
		{
			return TEXT("required_package must be /Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController.");
		}
		if (Path.Contains(TEXT("BP_AdminAccessDoor")) || Path.Contains(TEXT("BP_AdminTerminal"))
			|| Path.Contains(TEXT("BP_AdminFacilityHologram")))
		{
			return TEXT("Refusing to mutate a non-light-controller Admin Blueprint.");
		}
		OutBlueprint = LoadBlueprintAsset(Path);
		if (!OutBlueprint)
		{
			return TEXT("BP_AdminLightController not found.");
		}
		if (!OutBlueprint->ParentClass || OutBlueprint->ParentClass != AActor::StaticClass())
		{
			return TEXT("BP_AdminLightController parent must remain Actor.");
		}
		return TEXT("");
	}

	UClass* LoadLightingLibraryClass()
	{
		if (UClass* Loaded = FindObject<UClass>(nullptr, LightingLibraryPath))
		{
			return Loaded;
		}
		return LoadObject<UClass>(nullptr, LightingLibraryPath);
	}

	UK2Node_VariableSet* SpawnSelfSet(UEdGraph* Graph, const FName VarName, int32 PosX, int32 PosY)
	{
		if (!Graph)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_VariableSet> Creator(*Graph);
		UK2Node_VariableSet* Node = Creator.CreateNode();
		Node->VariableReference.SetSelfMember(VarName);
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	UK2Node_FunctionResult* SpawnFunctionResult(UEdGraph* Graph, int32 PosX, int32 PosY)
	{
		if (!Graph)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_FunctionResult> Creator(*Graph);
		UK2Node_FunctionResult* Node = Creator.CreateNode();
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	FString PreflightAddLightControllerVariables(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save and compile must be false. add_blueprint_variable does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingSpec)
		{
			return TEXT("LightController variables require spec=s20_admin_lighting_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireLightControllerBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}

		TSet<FName> CurrentVars;
		FBlueprintEditorUtils::GetClassVariableList(Blueprint, CurrentVars, true);
		for (const FLightControllerVarSpec& VarSpec : LightControllerVarSpecs)
		{
			const FName VarFName(VarSpec.Name);
			if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarFName) != INDEX_NONE || CurrentVars.Contains(VarFName))
			{
				return FString::Printf(TEXT("Variable '%s' already exists."), VarSpec.Name);
			}
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));

		TArray<TSharedPtr<FJsonValue>> ProposedVars;
		for (const FLightControllerVarSpec& VarSpec : LightControllerVarSpecs)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("variable_name"), VarSpec.Name);
			Row->SetStringField(TEXT("pin_category"), VarSpec.PinCategory);
			Row->SetStringField(TEXT("default"), VarSpec.DefaultValue);
			Row->SetStringField(TEXT("category"), LightControllerCategory);
			Row->SetBoolField(TEXT("instance_editable"), true);
			ProposedVars.Add(MakeShared<FJsonValueObject>(Row));
		}
		Proposed->SetStringField(TEXT("spec"), LightingSpec);
		Proposed->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Proposed->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Proposed->SetArrayField(TEXT("variables"), ProposedVars);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Add LightController FName/float members only. Package not saved. Full compile not performed."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAddLightControllerVariables(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAddLightControllerVariables(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(GetString(Change.Args, TEXT("path"), GetString(Change.Args, TEXT("blueprint"))));
		if (!Blueprint)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		for (const FLightControllerVarSpec& VarSpec : LightControllerVarSpecs)
		{
			FEdGraphPinType PinType;
			if (FCString::Strcmp(VarSpec.PinCategory, TEXT("name")) == 0)
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
			}
			else
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
				PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
			}
			const FName VarFName(VarSpec.Name);
			if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarFName, PinType, VarSpec.DefaultValue))
			{
				return FailAudit(
					TEXT("add_failed"),
					FString::Printf(TEXT("AddMemberVariable rejected '%s'. ZERO further writes."), VarSpec.Name),
					MakeShared<FBridgeChange>(Change));
			}
			FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarFName, false);
			FBlueprintEditorUtils::SetBlueprintVariableCategory(
				Blueprint, VarFName, nullptr, FText::FromString(LightControllerCategory), /*bDontRecompile=*/true);
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		TSharedRef<FJsonObject> After = VariableListSnapshot(Blueprint);
		After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		After->SetBoolField(TEXT("save"), false);
		After->SetBoolField(TEXT("compile"), false);
		After->SetStringField(TEXT("note"), TEXT("LightController members added. Package not saved. Full compile not performed."));
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightAuthorLightControllerSetZone(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save and compile must be false. author_light_controller_set_zone does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingSpec)
		{
			return TEXT("author_light_controller_set_zone requires spec=s20_admin_lighting_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireLightControllerBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		for (const FLightControllerVarSpec& VarSpec : LightControllerVarSpecs)
		{
			if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(VarSpec.Name)) == INDEX_NONE)
			{
				return FString::Printf(TEXT("Missing LightController variable '%s'."), VarSpec.Name);
			}
		}
		if (!LoadLightingLibraryClass())
		{
			return TEXT("ProjectOrganoidAdminLightingLibrary is not loaded. Compile the game module first.");
		}
		for (UEdGraph* ExistingGraph : Blueprint->FunctionGraphs)
		{
			if (ExistingGraph && ExistingGraph->GetFName() == FName(SetLightingZoneName))
			{
				return TEXT("SetLightingZone already exists.");
			}
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));
		Proposed->SetStringField(TEXT("function"), SetLightingZoneName);
		Proposed->SetStringField(TEXT("spec"), LightingSpec);
		Proposed->SetStringField(TEXT("construction_script"), TEXT("ApplyAdminZoneLighting(CurrentLightingZone). No Tick. No EventGraph gameplay."));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAuthorLightControllerSetZone(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAuthorLightControllerSetZone(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(GetString(Change.Args, TEXT("path")));
		UClass* LibraryClass = LoadLightingLibraryClass();
		if (!Blueprint || !LibraryClass)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint or lighting library not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		UFunction* IsApprovedFn = LibraryClass->FindFunctionByName(TEXT("IsApprovedAdminLightingZone"));
		UFunction* ApplyFn = LibraryClass->FindFunctionByName(TEXT("ApplyAdminZoneLighting"));
		if (!IsApprovedFn || !ApplyFn)
		{
			return FailAudit(TEXT("not_found"), TEXT("Lighting library functions not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint, FName(SetLightingZoneName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		if (!Graph)
		{
			return FailAudit(TEXT("add_failed"), TEXT("CreateNewGraph failed. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		UClass* SignatureClass = Blueprint->GeneratedClass ? (UClass*)Blueprint->GeneratedClass : AActor::StaticClass();
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, Graph, true, SignatureClass);

		UK2Node_FunctionEntry* Entry = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			Entry = Cast<UK2Node_FunctionEntry>(Node);
			if (Entry)
			{
				break;
			}
		}
		const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
		if (!Entry || !Schema)
		{
			FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
			return FailAudit(TEXT("add_failed"), TEXT("Function entry or schema missing after graph create. Graph removed. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		Entry->SetExtraFlags(FUNC_BlueprintCallable | FUNC_Public);
		FEdGraphPinType NamePinType;
		NamePinType.PinCategory = UEdGraphSchema_K2::PC_Name;
		Entry->CreateUserDefinedPin(TEXT("ZoneName"), NamePinType, EGPD_Output);

		UK2Node_CallFunction* IsApproved = SpawnCallFunction(Graph, IsApprovedFn, 320, 80);
		UK2Node_IfThenElse* Branch = SpawnBranch(Graph, 620, 0);
		UK2Node_FunctionResult* EarlyReturn = SpawnFunctionResult(Graph, 980, 160);
		UK2Node_VariableSet* SetZone = SpawnSelfSet(Graph, FName(TEXT("CurrentLightingZone")), 980, -40);
		UK2Node_CallFunction* Apply = SpawnCallFunction(Graph, ApplyFn, 1320, -40);
		UK2Node_VariableGet* AuthoredGet = SpawnSelfGet(Graph, FName(TEXT("AuthoredIntensity")), 1040, 120);
		UK2Node_VariableGet* InactiveGet = SpawnSelfGet(Graph, FName(TEXT("InactiveMultiplier")), 1040, 220);
		UK2Node_FunctionResult* SuccessReturn = SpawnFunctionResult(Graph, 1680, -40);
		if (!IsApproved || !Branch || !EarlyReturn || !SetZone || !Apply || !AuthoredGet || !InactiveGet || !SuccessReturn)
		{
			FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
			return FailAudit(TEXT("add_failed"), TEXT("Failed to spawn SetLightingZone nodes. Graph removed. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		auto ConnectWorldContextIfVisible = [](const UEdGraphSchema_K2* InSchema, UEdGraphPin* SelfPin, UK2Node_CallFunction* Call, FString& OutError) -> bool
		{
			UEdGraphPin* WorldPin = FindPinByName(Call, TEXT("WorldContextObject"), EGPD_Input);
			if (!WorldPin || WorldPin->bHidden)
			{
				return true;
			}
			return ConnectSchemaPins(InSchema, SelfPin, WorldPin, OutError);
		};

		UEdGraphPin* ZoneOut = FindPinByName(Entry, TEXT("ZoneName"), EGPD_Output);
		FString WireError;
		if (!ConnectSchemaPins(Schema, FindExecPin(Entry, EGPD_Output), FindExecPin(Branch, EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, ZoneOut, FindPinByName(IsApproved, TEXT("ZoneName"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindFirstDataOutput(IsApproved), FindPinByName(Branch, UEdGraphSchema_K2::PN_Condition, EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindPinByName(Branch, UEdGraphSchema_K2::PN_Else, EGPD_Output), FindExecPin(EarlyReturn, EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindPinByName(Branch, UEdGraphSchema_K2::PN_Then, EGPD_Output), FindExecPin(SetZone, EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, ZoneOut, FindPinByName(SetZone, TEXT("CurrentLightingZone"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindExecPin(SetZone, EGPD_Output), FindExecPin(Apply, EGPD_Input), WireError)
			|| !ConnectWorldContextIfVisible(Schema, FindPinByName(Entry, UEdGraphSchema_K2::PN_Self, EGPD_Output), Apply, WireError)
			|| !ConnectSchemaPins(Schema, ZoneOut, FindPinByName(Apply, TEXT("CurrentZone"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindFirstDataOutput(AuthoredGet), FindPinByName(Apply, TEXT("AuthoredIntensity"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindFirstDataOutput(InactiveGet), FindPinByName(Apply, TEXT("InactiveMultiplier"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindExecPin(Apply, EGPD_Output), FindExecPin(SuccessReturn, EGPD_Input), WireError))
		{
			FBlueprintEditorUtils::RemoveGraph(Blueprint, Graph);
			return FailAudit(
				TEXT("connect_failed"),
				FString::Printf(TEXT("%s Graph removed. ZERO remaining writes."), *WireError),
				MakeShared<FBridgeChange>(Change));
		}

		UEdGraph* Construction = FBlueprintEditorUtils::FindUserConstructionScript(Blueprint);
		if (!Construction)
		{
			Construction = FBlueprintEditorUtils::CreateNewGraph(
				Blueprint, UEdGraphSchema_K2::FN_UserConstructionScript, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
			FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, Construction, false, AActor::StaticClass());
		}
		UK2Node_FunctionEntry* ConstructionEntry = nullptr;
		for (UEdGraphNode* Node : Construction->Nodes)
		{
			ConstructionEntry = Cast<UK2Node_FunctionEntry>(Node);
			if (ConstructionEntry)
			{
				break;
			}
		}
		const UEdGraphSchema_K2* ConstructionSchema = Cast<const UEdGraphSchema_K2>(Construction->GetSchema());
		UK2Node_CallFunction* ConstructionApply = SpawnCallFunction(Construction, ApplyFn, 400, 0);
		UK2Node_VariableGet* ConstructionZone = SpawnSelfGet(Construction, FName(TEXT("CurrentLightingZone")), 80, 80);
		UK2Node_VariableGet* ConstructionAuthored = SpawnSelfGet(Construction, FName(TEXT("AuthoredIntensity")), 80, 160);
		UK2Node_VariableGet* ConstructionInactive = SpawnSelfGet(Construction, FName(TEXT("InactiveMultiplier")), 80, 240);
		if (!ConstructionEntry || !ConstructionSchema || !ConstructionApply || !ConstructionZone || !ConstructionAuthored || !ConstructionInactive)
		{
			return FailAudit(TEXT("add_failed"), TEXT("Failed to wire Construction Script apply."), MakeShared<FBridgeChange>(Change));
		}
		UEdGraphPin* ConstructionThen = FindExecPin(ConstructionEntry, EGPD_Output);
		UEdGraphPin* PreviousTarget = (ConstructionThen && ConstructionThen->LinkedTo.Num() > 0) ? ConstructionThen->LinkedTo[0] : nullptr;
		if (ConstructionThen)
		{
			ConstructionThen->BreakAllPinLinks();
		}
		if (!ConnectSchemaPins(ConstructionSchema, ConstructionThen, FindExecPin(ConstructionApply, EGPD_Input), WireError)
			|| !ConnectWorldContextIfVisible(ConstructionSchema, FindPinByName(ConstructionEntry, UEdGraphSchema_K2::PN_Self, EGPD_Output), ConstructionApply, WireError)
			|| !ConnectSchemaPins(ConstructionSchema, FindFirstDataOutput(ConstructionZone), FindPinByName(ConstructionApply, TEXT("CurrentZone"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(ConstructionSchema, FindFirstDataOutput(ConstructionAuthored), FindPinByName(ConstructionApply, TEXT("AuthoredIntensity"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(ConstructionSchema, FindFirstDataOutput(ConstructionInactive), FindPinByName(ConstructionApply, TEXT("InactiveMultiplier"), EGPD_Input), WireError))
		{
			if (ConstructionThen)
			{
				ConstructionThen->BreakAllPinLinks();
				if (PreviousTarget)
				{
					ConnectSchemaPins(ConstructionSchema, ConstructionThen, PreviousTarget, WireError);
				}
			}
			Construction->RemoveNode(ConstructionApply);
			Construction->RemoveNode(ConstructionZone);
			Construction->RemoveNode(ConstructionAuthored);
			Construction->RemoveNode(ConstructionInactive);
			return FailAudit(
				TEXT("connect_failed"),
				FString::Printf(TEXT("%s Construction Script restored. ZERO remaining writes."), *WireError),
				MakeShared<FBridgeChange>(Change));
		}
		if (PreviousTarget)
		{
			ConnectSchemaPins(ConstructionSchema, FindExecPin(ConstructionApply, EGPD_Output), PreviousTarget, WireError);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("function"), SetLightingZoneName);
		Change.After->SetBoolField(TEXT("construction_script_wired"), true);
		Change.After->SetBoolField(TEXT("tick"), false);
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("SetLightingZone authored. Construction Script applies current zone. No Tick. Package not saved."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSpawnLightControllerActor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_blueprint_actor does not save or compile.");
		}

		const FString BlueprintPath = NormalizeBlueprintPath(
			GetString(Args, TEXT("blueprint"), GetString(Args, TEXT("class_path"), GetString(Args, TEXT("path")))));
		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("level_package"), GetString(Args, TEXT("required_package")))));
		const FString Label = GetString(Args, TEXT("label"), GetString(Args, TEXT("actor")));
		FVector Location = FVector::ZeroVector;
		FVector RotationVec = FVector::ZeroVector;
		FVector Scale = FVector::OneVector;
		if (!GetVector(Args, TEXT("location"), Location))
		{
			return TEXT("explicit location [x,y,z] is required.");
		}
		GetVector(Args, TEXT("rotation"), RotationVec);
		if (!GetVector(Args, TEXT("scale"), Scale))
		{
			Scale = FVector::OneVector;
		}
		if (!Label.Equals(LightControllerLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("LightController spawn label must be Admin_LightController.");
		}
		if (!IsLightControllerPackage(BlueprintPath))
		{
			return TEXT("LightController spawn class must be BP_AdminLightController.");
		}
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("LightController destination must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}
		if (!LocationMatches(Location, LightControllerLocation)
			|| !RotationMatches(FRotator(RotationVec.X, RotationVec.Y, RotationVec.Z), LightControllerRotation)
			|| !ScaleMatches(Scale, LightControllerScale))
		{
			return TEXT("Admin_LightController must use location (2280,0,250), rotation (0,0,0), scale (1,1,1).");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		ULevel* TargetLevel = FindLoadedLevelByPackage(World, Destination);
		if (!TargetLevel)
		{
			return TEXT("Target level is not loaded. ZERO writes.");
		}
		if (FindByExactLabel(World, Label).Num() != 0)
		{
			return TEXT("Admin_LightController already exists. Fail closed.");
		}

		FString GuardError = GuardExistingActor(World, ReceptionLabel, ReceptionLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door guard failed: %s"), *GuardError);
		}

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, *FString::Printf(TEXT("%s.%s"), *BlueprintPath, *FPaths::GetBaseFilename(BlueprintPath)));
		}
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return TEXT("BP_AdminLightController Blueprint or generated class not found.");
		}
		if (Blueprint->ParentClass != AActor::StaticClass())
		{
			return TEXT("BP_AdminLightController parent must remain Actor.");
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetArrayField(TEXT("location"), Vec(LightControllerLocation));
		Proposed->SetArrayField(TEXT("rotation"), Rot(LightControllerRotation));
		Proposed->SetArrayField(TEXT("scale"), Vec(LightControllerScale));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn Admin_LightController into loaded SL_Epitope_Admin. Does not save."));
		return TEXT("");
	}

	bool IsLightControllerSpawnArgs(const TSharedPtr<FJsonObject>& Args)
	{
		const FString BlueprintPath = NormalizeBlueprintPath(
			GetString(Args, TEXT("blueprint"), GetString(Args, TEXT("class_path"), GetString(Args, TEXT("path")))));
		return IsLightControllerPackage(BlueprintPath);
	}

	FString PreflightSpawnS20ZoneLight(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_s20_zone_light does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingSpec)
		{
			return TEXT("spawn_s20_zone_light requires spec=s20_admin_lighting_v1.");
		}

		const FString Label = GetString(Args, TEXT("label"), GetString(Args, TEXT("actor")));
		const FZoneLightSpec* Spec = FindZoneLightSpec(Label);
		if (!Spec)
		{
			return TEXT("label must be an approved Admin_ZoneLight_<RoomID>.");
		}
		FVector Location = FVector::ZeroVector;
		if (!GetVector(Args, TEXT("location"), Location))
		{
			return TEXT("explicit location [x,y,z] is required.");
		}
		if (!LocationMatches(Location, Spec->Location))
		{
			return FString::Printf(
				TEXT("%s must use location (%.1f, %.1f, %.1f)."),
				Spec->Label, Spec->Location.X, Spec->Location.Y, Spec->Location.Z);
		}
		const FString Destination = NormalizePackage(
			GetString(Args, TEXT("destination_package"), GetString(Args, TEXT("level_package"), GetString(Args, TEXT("required_package"), AdminPackage))));
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("Zone light destination must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, Destination))
		{
			return TEXT("Target level is not loaded. ZERO writes.");
		}
		if (FindByExactLabel(World, Label).Num() != 0)
		{
			return FString::Printf(TEXT("%s already exists. Fail closed."), Spec->Label);
		}

		FString GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door guard failed: %s"), *GuardError);
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetStringField(TEXT("label"), Spec->Label);
		Proposed->SetStringField(TEXT("room_id"), Spec->RoomID);
		Proposed->SetStringField(TEXT("class"), TEXT("PointLight"));
		Proposed->SetArrayField(TEXT("location"), Vec(Spec->Location));
		Proposed->SetNumberField(TEXT("intensity"), LightingAuthoredIntensity);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn one Admin_ZoneLight_* PointLight into SL_Epitope_Admin. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnS20ZoneLight(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_s20_zone_light must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnS20ZoneLight(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		const FString Label = GetString(Change.Args, TEXT("label"), GetString(Change.Args, TEXT("actor")));
		const FZoneLightSpec* Spec = FindZoneLightSpec(Label);
		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, AdminPackage) : nullptr;
		if (!World || !TargetLevel || !Spec)
		{
			return FailAudit(TEXT("not_found"), TEXT("World, Admin level, or zone spec vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnS20ZoneLight", "Spawn S20 Zone Light"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		const FRotator Rotation = FRotator::ZeroRotator;
		APointLight* Spawned = World->SpawnActor<APointLight>(Spec->Location, Rotation, Params);
		if (!Spawned)
		{
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor APointLight returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}
		Spawned->SetActorLabel(Spec->Label, true);
		Spawned->Tags.AddUnique(FName(TEXT("Admin_ZoneLight")));
		Spawned->Tags.AddUnique(FName(Spec->RoomID));
		Spawned->Tags.AddUnique(FName(Spec->Label));
		if (ULightComponent* Light = Spawned->GetLightComponent())
		{
			Light->SetMobility(EComponentMobility::Stationary);
			Light->SetIntensity(LightingAuthoredIntensity);
			Light->SetLightColor(FLinearColor::White);
			Light->SetCastShadows(false);
			if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light))
			{
				Point->SetAttenuationRadius(1500.f);
			}
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), Spec->Label);
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
		Change.After->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSetS20LightIntensity(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false))
		{
			return TEXT("save must be false. set_s20_light_intensity does not save maps.");
		}
		const FString Label = GetString(Args, TEXT("label"), GetString(Args, TEXT("actor")));
		const FZoneLightSpec* Spec = FindZoneLightSpec(Label);
		if (!Spec)
		{
			return TEXT("set_s20_light_intensity allows only Admin_ZoneLight_<RoomID> labels.");
		}
		if (!Args.IsValid() || !Args->HasField(TEXT("intensity")))
		{
			return TEXT("intensity is required.");
		}
		const double Intensity = GetNumber(Args, TEXT("intensity"), -1.0);
		if (Intensity < 0.0)
		{
			return TEXT("intensity must be >= 0.");
		}

		UWorld* World = GetEditorWorld();
		TArray<AActor*> Matches = FindByExactLabel(World, Label);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1"), *Label, Matches.Num());
		}
		APointLight* Point = Cast<APointLight>(Matches[0]);
		if (!Point)
		{
			return TEXT("Target is not a PointLight.");
		}
		if (!PackagesEqual(ActorOwningPackage(Point), AdminPackage))
		{
			return TEXT("Zone light must be owned by SL_Epitope_Admin.");
		}
		ULightComponent* Light = Point->GetLightComponent();
		if (!Light)
		{
			return TEXT("PointLight has no LightComponent.");
		}

		Before->SetStringField(TEXT("label"), Label);
		Before->SetStringField(TEXT("owning_package"), ActorOwningPackage(Point));
		Before->SetNumberField(TEXT("intensity"), Light->Intensity);
		Proposed->SetNumberField(TEXT("intensity"), Intensity);
		Proposed->SetBoolField(TEXT("save"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetS20LightIntensity(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetS20LightIntensity(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		const FString Label = GetString(Change.Args, TEXT("label"), GetString(Change.Args, TEXT("actor")));
		APointLight* Point = Cast<APointLight>(FindUniqueLabel(GetEditorWorld(), Label));
		ULightComponent* Light = Point ? Point->GetLightComponent() : nullptr;
		if (!Light)
		{
			return FailAudit(TEXT("not_found"), TEXT("Zone light vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		Light->SetIntensity(static_cast<float>(GetNumber(Change.Args, TEXT("intensity"), LightingAuthoredIntensity)));
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), Label);
		Change.After->SetNumberField(TEXT("intensity"), Light->Intensity);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	bool IsRoomTriggerPackage(const FString& Path)
	{
		const FString Normalized = NormalizePackage(Path);
		return PackagesEqual(Normalized, RoomTriggerBpPath) || PackagesEqual(Normalized, RoomTriggerBpPackage);
	}

	FString RequireRoomTriggerBlueprint(const TSharedPtr<FJsonObject>& Args, UBlueprint*& OutBlueprint)
	{
		OutBlueprint = nullptr;
		const FString Path = GetString(Args, TEXT("path"), GetString(Args, TEXT("blueprint")));
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"), RoomTriggerBpPackage)));
		if (!IsRoomTriggerPackage(Path))
		{
			return TEXT("This action is allowlisted only for BP_AdminRoomTrigger.");
		}
		if (!PackagesEqual(RequiredPackage, RoomTriggerBpPackage))
		{
			return TEXT("required_package must be /Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger.");
		}
		if (Path.Contains(TEXT("BP_AdminAccessDoor")) || Path.Contains(TEXT("BP_AdminTerminal"))
			|| Path.Contains(TEXT("BP_AdminFacilityHologram")) || Path.Contains(TEXT("BP_AdminSectorController"))
			|| Path.Contains(TEXT("BP_AdminLightController")))
		{
			return TEXT("Refusing to mutate a non-room-trigger Admin Blueprint.");
		}
		OutBlueprint = LoadBlueprintAsset(Path);
		if (!OutBlueprint)
		{
			return TEXT("BP_AdminRoomTrigger not found.");
		}
		const FString ParentPath = OutBlueprint->ParentClass ? OutBlueprint->ParentClass->GetPathName() : FString();
		if (!ParentPath.Equals(RoomTriggerParentPath, ESearchCase::IgnoreCase))
		{
			return TEXT("BP_AdminRoomTrigger parent must remain ProjectOrganoidAdminRoomTrigger.");
		}
		return TEXT("");
	}

	FName EventFunctionName(UK2Node_Event* Event)
	{
		return Event ? Event->GetFunctionName() : NAME_None;
	}

	TSharedRef<FJsonObject> SnapshotUbergraphEvents(UBlueprint* Blueprint)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Events;
		if (Blueprint)
		{
			for (UEdGraph* Page : Blueprint->UbergraphPages)
			{
				if (!Page)
				{
					continue;
				}
				for (UEdGraphNode* Node : Page->Nodes)
				{
					UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
					if (!Event)
					{
						continue;
					}
					TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
					Row->SetStringField(TEXT("name"), Event->GetName());
					Row->SetStringField(TEXT("function"), EventFunctionName(Event).ToString());
					Row->SetStringField(TEXT("title"), Event->GetNodeTitle(ENodeTitleType::ListView).ToString());
					Events.Add(MakeShared<FJsonValueObject>(Row));
				}
			}
		}
		Out->SetArrayField(TEXT("events"), Events);
		Out->SetNumberField(TEXT("event_count"), Events.Num());
		return Out;
	}

	TSharedRef<FJsonObject> SnapshotS20LightIntensities(UWorld* World)
	{
		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Lights;
		for (const FZoneLightSpec& Spec : ZoneLightSpecs)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Spec.Label);
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			Row->SetNumberField(TEXT("count"), Matches.Num());
			if (Matches.Num() == 1)
			{
				APointLight* Point = Cast<APointLight>(Matches[0]);
				Row->SetStringField(TEXT("class"), Point ? TEXT("PointLight") : ClassName(Matches[0]));
				Row->SetStringField(TEXT("owning_package"), ActorOwningPackage(Matches[0]));
				if (ULightComponent* Light = Point ? Point->GetLightComponent() : nullptr)
				{
					Row->SetNumberField(TEXT("intensity"), Light->Intensity);
				}
			}
			Lights.Add(MakeShared<FJsonValueObject>(Row));
		}
		Out->SetArrayField(TEXT("lights"), Lights);
		return Out;
	}

	FString ReadNameProperty(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return FString();
		}
		if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Actor->GetClass(), PropertyName))
		{
			return NameProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		return FString();
	}

	float ReadFloatProperty(AActor* Actor, const TCHAR* PropertyName, float DefaultValue)
	{
		if (!Actor)
		{
			return DefaultValue;
		}
		if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(PropertyName))
		{
			if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
			{
				return static_cast<float>(Num->GetFloatingPointPropertyValue(Num->ContainerPtrToValuePtr<void>(Actor)));
			}
		}
		return DefaultValue;
	}

	bool EventThenIsUnlinked(UK2Node_Event* Event)
	{
		UEdGraphPin* Then = FindExecPin(Event, EGPD_Output);
		return Then != nullptr && Then->LinkedTo.Num() == 0;
	}

	UK2Node_Event* FindUbergraphEventByFunction(UBlueprint* Blueprint, const FName FunctionName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}
		for (UEdGraph* Page : Blueprint->UbergraphPages)
		{
			if (!Page)
			{
				continue;
			}
			for (UEdGraphNode* Node : Page->Nodes)
			{
				UK2Node_Event* Event = Cast<UK2Node_Event>(Node);
				if (Event && EventFunctionName(Event) == FunctionName)
				{
					return Event;
				}
			}
		}
		return nullptr;
	}

	bool UbergraphCallsFunction(UBlueprint* Blueprint, const FName FunctionName)
	{
		if (!Blueprint)
		{
			return false;
		}
		for (UEdGraph* Page : Blueprint->UbergraphPages)
		{
			if (!Page)
			{
				continue;
			}
			for (UEdGraphNode* Node : Page->Nodes)
			{
				UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
				if (Call && Call->GetFunctionName() == FunctionName)
				{
					return true;
				}
			}
		}
		return false;
	}

	FString PreflightAuthorAdminRoomTriggerLightingHook(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save and compile must be false. author_admin_room_trigger_lighting_hook does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingSpec)
		{
			return TEXT("author_admin_room_trigger_lighting_hook requires spec=s20_admin_lighting_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireRoomTriggerBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		UClass* LibraryClass = LoadLightingLibraryClass();
		if (!LibraryClass || !LibraryClass->FindFunctionByName(RequestAdminLightingZoneName))
		{
			return TEXT("RequestAdminLightingZone is not loaded. Compile the game module first.");
		}
		if (UbergraphCallsFunction(Blueprint, FName(RequestAdminLightingZoneName)))
		{
			return TEXT("RequestAdminLightingZone is already present on BP_AdminRoomTrigger.");
		}
		if (UbergraphCallsFunction(Blueprint, FName(TEXT("SetCurrentRoom"))))
		{
			return TEXT("Refusing to author a graph that already calls SetCurrentRoom.");
		}
		if (FindUbergraphEventByFunction(Blueprint, FName(TEXT("OnTriggerBeginOverlap"))))
		{
			return TEXT("Refusing to author a Blueprint override of OnTriggerBeginOverlap.");
		}

		UK2Node_Event* BeginPlay = FindUbergraphEventByFunction(Blueprint, FName(TEXT("ReceiveBeginPlay")));
		if (!BeginPlay)
		{
			BeginPlay = FindUbergraphEventByFunction(Blueprint, FName(TEXT("BeginPlay")));
		}
		if (BeginPlay && !EventThenIsUnlinked(BeginPlay))
		{
			return TEXT("BP_AdminRoomTrigger BeginPlay is already wired. Refusing to author a lighting hook that could suppress native BeginPlay.");
		}

		UK2Node_Event* Overlap = FindUbergraphEventByFunction(Blueprint, FName(TEXT("ReceiveActorBeginOverlap")));
		if (!Overlap)
		{
			Overlap = FindUbergraphEventByFunction(Blueprint, FName(TEXT("ActorBeginOverlap")));
		}
		if (!Overlap)
		{
			return TEXT("ReceiveActorBeginOverlap stub is missing. Refusing to guess a new EventGraph event.");
		}
		if (!EventThenIsUnlinked(Overlap))
		{
			return TEXT("ReceiveActorBeginOverlap is already wired. Refusing to duplicate or replace it.");
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetObjectField(TEXT("ubergraph"), SnapshotUbergraphEvents(Blueprint));
		Before->SetBoolField(TEXT("begin_play_present_unwired"), BeginPlay != nullptr && EventThenIsUnlinked(BeginPlay));
		Before->SetBoolField(TEXT("actor_begin_overlap_present_unwired"), true);
		Proposed->SetStringField(TEXT("event"), TEXT("ReceiveActorBeginOverlap"));
		Proposed->SetStringField(TEXT("call"), RequestAdminLightingZoneName);
		Proposed->SetStringField(TEXT("spec"), LightingSpec);
		Proposed->SetStringField(
			TEXT("note"),
			TEXT("Wires the existing unwired ActorBeginOverlap stub to RequestAdminLightingZone(RoomID). Does not author BeginPlay. Does not call SetCurrentRoom. Does not modify SectorController."));
		Proposed->SetBoolField(TEXT("reuse_existing_overlap_event"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAuthorAdminRoomTriggerLightingHook(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAuthorAdminRoomTriggerLightingHook(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(GetString(Change.Args, TEXT("path")));
		UClass* LibraryClass = LoadLightingLibraryClass();
		UFunction* RequestFn = LibraryClass ? LibraryClass->FindFunctionByName(RequestAdminLightingZoneName) : nullptr;
		UK2Node_Event* EventNode = FindUbergraphEventByFunction(Blueprint, FName(TEXT("ReceiveActorBeginOverlap")));
		if (!EventNode)
		{
			EventNode = FindUbergraphEventByFunction(Blueprint, FName(TEXT("ActorBeginOverlap")));
		}
		if (!Blueprint || !RequestFn || !EventNode)
		{
			return FailAudit(TEXT("not_found"), TEXT("Room trigger overlap stub or RequestAdminLightingZone vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UEdGraph* EventGraph = EventNode->GetGraph();
		const UEdGraphSchema_K2* Schema = EventGraph ? Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema()) : nullptr;
		if (!EventGraph || !Schema)
		{
			return FailAudit(TEXT("add_failed"), TEXT("EventGraph or schema missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UK2Node_CallFunction* RequestCall = SpawnCallFunction(EventGraph, RequestFn, 420, 0);
		UK2Node_VariableGet* RoomGet = SpawnSelfGet(EventGraph, FName(TEXT("RoomID")), 80, 160);
		if (!RequestCall || !RoomGet)
		{
			if (RequestCall)
			{
				EventGraph->RemoveNode(RequestCall);
			}
			if (RoomGet)
			{
				EventGraph->RemoveNode(RoomGet);
			}
			return FailAudit(TEXT("add_failed"), TEXT("Failed to spawn lighting hook nodes. Existing overlap event left untouched. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		auto ConnectWorldContextIfVisible = [](const UEdGraphSchema_K2* InSchema, UEdGraphPin* SelfPin, UK2Node_CallFunction* Call, FString& OutError) -> bool
		{
			UEdGraphPin* WorldPin = FindPinByName(Call, TEXT("WorldContextObject"), EGPD_Input);
			if (!WorldPin || WorldPin->bHidden)
			{
				return true;
			}
			return ConnectSchemaPins(InSchema, SelfPin, WorldPin, OutError);
		};

		FString WireError;
		UEdGraphPin* OtherOut = FindPinByName(EventNode, TEXT("OtherActor"), EGPD_Output);
		if (!ConnectSchemaPins(Schema, FindExecPin(EventNode, EGPD_Output), FindExecPin(RequestCall, EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, OtherOut, FindPinByName(RequestCall, TEXT("OtherActor"), EGPD_Input), WireError)
			|| !ConnectSchemaPins(Schema, FindFirstDataOutput(RoomGet), FindPinByName(RequestCall, TEXT("ZoneName"), EGPD_Input), WireError)
			|| !ConnectWorldContextIfVisible(Schema, FindPinByName(EventNode, UEdGraphSchema_K2::PN_Self, EGPD_Output), RequestCall, WireError))
		{
			EventGraph->RemoveNode(RequestCall);
			EventGraph->RemoveNode(RoomGet);
			return FailAudit(
				TEXT("connect_failed"),
				FString::Printf(TEXT("%s Added nodes removed. Existing overlap event left unwired. ZERO remaining writes."), *WireError),
				MakeShared<FBridgeChange>(Change));
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("event"), TEXT("ReceiveActorBeginOverlap"));
		Change.After->SetStringField(TEXT("call"), RequestAdminLightingZoneName);
		Change.After->SetBoolField(TEXT("reused_existing_overlap_event"), true);
		Change.After->SetBoolField(TEXT("begin_play_authored"), false);
		Change.After->SetBoolField(TEXT("set_current_room_called"), false);
		Change.After->SetObjectField(TEXT("ubergraph"), SnapshotUbergraphEvents(Blueprint));
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetStringField(
			TEXT("note"),
			TEXT("Existing ActorBeginOverlap stub now calls RequestAdminLightingZone(RoomID). Native OnTriggerBeginOverlap/SetCurrentRoom unchanged. BeginPlay left unwired."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightInvokeS20SetLightingZone(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. invoke_s20_set_lighting_zone does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingSpec)
		{
			return TEXT("invoke_s20_set_lighting_zone requires spec=s20_admin_lighting_v1.");
		}
		const FString Label = GetString(Args, TEXT("label"), GetString(Args, TEXT("actor"), LightControllerLabel));
		if (!Label.Equals(LightControllerLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("invoke_s20_set_lighting_zone allows only Admin_LightController.");
		}
		const bool bReset = GetBool(Args, TEXT("reset"), false);
		const FString ZoneName = GetString(Args, TEXT("zone"), GetString(Args, TEXT("ZoneName")));
		if (!bReset && ZoneName.IsEmpty())
		{
			return TEXT("zone is required unless reset=true.");
		}

		UWorld* World = GetEditorWorld();
		TArray<AActor*> Matches = FindByExactLabel(World, LightControllerLabel);
		if (Matches.Num() != 1)
		{
			return FString::Printf(TEXT("Admin_LightController count=%d expected=1"), Matches.Num());
		}
		if (!PackagesEqual(ActorOwningPackage(Matches[0]), AdminPackage))
		{
			return TEXT("Admin_LightController must be owned by SL_Epitope_Admin.");
		}

		Before->SetStringField(TEXT("label"), LightControllerLabel);
		Before->SetStringField(TEXT("owning_package"), ActorOwningPackage(Matches[0]));
		Before->SetStringField(TEXT("CurrentLightingZone"), ReadNameProperty(Matches[0], TEXT("CurrentLightingZone")));
		Before->SetObjectField(TEXT("lights"), SnapshotS20LightIntensities(World));
		Proposed->SetBoolField(TEXT("reset"), bReset);
		Proposed->SetStringField(TEXT("zone"), ZoneName);
		Proposed->SetBoolField(TEXT("save"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteInvokeS20SetLightingZone(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightInvokeS20SetLightingZone(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		AActor* Controller = FindUniqueLabel(GetEditorWorld(), LightControllerLabel);
		if (!Controller)
		{
			return FailAudit(TEXT("not_found"), TEXT("Admin_LightController vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const bool bReset = GetBool(Change.Args, TEXT("reset"), false);
		const FString ZoneName = GetString(Change.Args, TEXT("zone"), GetString(Change.Args, TEXT("ZoneName")));
		if (bReset)
		{
			if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Controller->GetClass(), TEXT("CurrentLightingZone")))
			{
				NameProp->SetPropertyValue_InContainer(Controller, NAME_None);
			}
			UClass* LibraryClass = LoadLightingLibraryClass();
			UFunction* ApplyFn = LibraryClass ? LibraryClass->FindFunctionByName(TEXT("ApplyAdminZoneLighting")) : nullptr;
			UObject* LibraryCDO = LibraryClass ? LibraryClass->GetDefaultObject() : nullptr;
			if (!ApplyFn || !LibraryCDO)
			{
				return FailAudit(TEXT("not_found"), TEXT("ApplyAdminZoneLighting not loaded. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(ApplyFn->ParmsSize);
			if (FObjectProperty* WorldProp = FindFProperty<FObjectProperty>(ApplyFn, TEXT("WorldContextObject")))
			{
				WorldProp->SetObjectPropertyValue(WorldProp->ContainerPtrToValuePtr<void>(Parms.GetData()), Controller);
			}
			if (FNameProperty* ZoneProp = FindFProperty<FNameProperty>(ApplyFn, TEXT("CurrentZone")))
			{
				ZoneProp->SetPropertyValue(ZoneProp->ContainerPtrToValuePtr<void>(Parms.GetData()), NAME_None);
			}
			auto SetFloatParm = [&](const TCHAR* Name, float Value)
			{
				if (FNumericProperty* Num = CastField<FNumericProperty>(ApplyFn->FindPropertyByName(Name)))
				{
					Num->SetFloatingPointPropertyValue(Num->ContainerPtrToValuePtr<void>(Parms.GetData()), Value);
				}
			};
			SetFloatParm(TEXT("AuthoredIntensity"), ReadFloatProperty(Controller, TEXT("AuthoredIntensity"), LightingAuthoredIntensity));
			SetFloatParm(TEXT("InactiveMultiplier"), ReadFloatProperty(Controller, TEXT("InactiveMultiplier"), LightingInactiveMultiplier));
			LibraryCDO->ProcessEvent(ApplyFn, Parms.GetData());
		}
		else
		{
			UFunction* SetZone = Controller->FindFunction(FName(SetLightingZoneName));
			if (!SetZone)
			{
				return FailAudit(TEXT("not_found"), TEXT("SetLightingZone not found on Admin_LightController. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(SetZone->ParmsSize);
			bool bWroteZone = false;
			for (TFieldIterator<FProperty> It(SetZone); It; ++It)
			{
				FProperty* Property = *It;
				if (!Property || !Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
				{
					continue;
				}
				if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
				{
					NameProp->SetPropertyValue(NameProp->ContainerPtrToValuePtr<void>(Parms.GetData()), FName(*ZoneName));
					bWroteZone = true;
				}
			}
			if (!bWroteZone)
			{
				return FailAudit(TEXT("not_found"), TEXT("SetLightingZone has no FName input pin. ZERO writes."), MakeShared<FBridgeChange>(Change));
			}
			{
				FEditorScriptExecutionGuard ScriptGuard;
				Controller->ProcessEvent(SetZone, Parms.GetData());
			}
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("CurrentLightingZone"), ReadNameProperty(Controller, TEXT("CurrentLightingZone")));
		Change.After->SetNumberField(TEXT("InactiveMultiplier"), ReadFloatProperty(Controller, TEXT("InactiveMultiplier"), LightingInactiveMultiplier));
		Change.After->SetNumberField(TEXT("AuthoredIntensity"), ReadFloatProperty(Controller, TEXT("AuthoredIntensity"), LightingAuthoredIntensity));
		Change.After->SetObjectField(TEXT("lights"), SnapshotS20LightIntensities(GetEditorWorld()));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	const TCHAR* LightingListenerSpec = TEXT("s21_lighting_listener_v1");
	const TCHAR* LightingListenerComponentName = TEXT("FacilityStateListener");
	const TCHAR* LightingListenerClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminLightControllerFacilityStateListener");

	UClass* LoadLightControllerListenerClass()
	{
		if (UClass* Loaded = FindObject<UClass>(nullptr, LightingListenerClassPath))
		{
			return Loaded;
		}
		return LoadObject<UClass>(nullptr, LightingListenerClassPath);
	}

	FString SnapshotLightControllerScs(UBlueprint* Blueprint)
	{
		TArray<FString> Names;
		if (Blueprint && Blueprint->SimpleConstructionScript)
		{
			for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (Node)
				{
					Names.Add(Node->GetVariableName().ToString());
				}
			}
		}
		Names.Sort();
		return FString::Join(Names, TEXT(","));
	}

	FString PreflightAuthorLightControllerFacilityStateListener(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save and compile must be false. author_light_controller_facility_state_listener does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != LightingListenerSpec)
		{
			return TEXT("author_light_controller_facility_state_listener requires spec=s21_lighting_listener_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireLightControllerBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		if (!Blueprint->SimpleConstructionScript)
		{
			return TEXT("BP_AdminLightController has no SimpleConstructionScript.");
		}
		UClass* ListenerClass = LoadLightControllerListenerClass();
		if (!ListenerClass)
		{
			return TEXT("ProjectOrganoidAdminLightControllerFacilityStateListener is not loaded. Compile the game module first.");
		}
		if (!ListenerClass->IsChildOf(UActorComponent::StaticClass()) || ListenerClass->IsChildOf(USceneComponent::StaticClass()))
		{
			return TEXT("Listener class must be a non-scene UActorComponent.");
		}
		if (Blueprint->SimpleConstructionScript->FindSCSNode(FName(LightingListenerComponentName)))
		{
			return TEXT("FacilityStateListener already exists on BP_AdminLightController.");
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetStringField(TEXT("scs"), SnapshotLightControllerScs(Blueprint));
		Before->SetBoolField(TEXT("eventgraph_untouched"), true);
		Before->SetBoolField(TEXT("set_lighting_zone_untouched"), true);
		Proposed->SetStringField(TEXT("spec"), LightingListenerSpec);
		Proposed->SetStringField(TEXT("component"), LightingListenerComponentName);
		Proposed->SetStringField(TEXT("component_class"), LightingListenerClassPath);
		Proposed->SetStringField(
			TEXT("note"),
			TEXT("Adds native FacilityStateListener SCS component only. Does not author EventGraph, SetLightingZone, variables, or maps. Package not saved. Compile not performed."));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAuthorLightControllerFacilityStateListener(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAuthorLightControllerFacilityStateListener(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = nullptr;
		RequireLightControllerBlueprint(Change.Args, Blueprint);
		USimpleConstructionScript* SCS = Blueprint ? Blueprint->SimpleConstructionScript : nullptr;
		UClass* ListenerClass = LoadLightControllerListenerClass();
		if (!Blueprint || !SCS || !ListenerClass)
		{
			return FailAudit(TEXT("not_found"), TEXT("LightController Blueprint, SCS, or listener class vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (SCS->FindSCSNode(FName(LightingListenerComponentName)))
		{
			return FailAudit(TEXT("already_exists"), TEXT("FacilityStateListener already exists. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		Blueprint->Modify();
		SCS->Modify();
		USCS_Node* Node = SCS->CreateNode(ListenerClass, FName(LightingListenerComponentName));
		if (!Node || !Node->ComponentTemplate)
		{
			return FailAudit(TEXT("add_failed"), TEXT("Failed to create FacilityStateListener SCS node. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		SCS->AddNode(Node);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("component"), LightingListenerComponentName);
		Change.After->SetStringField(TEXT("component_class"), ListenerClass->GetPathName());
		Change.After->SetStringField(TEXT("scs"), SnapshotLightControllerScs(Blueprint));
		Change.After->SetBoolField(TEXT("eventgraph_authored"), false);
		Change.After->SetBoolField(TEXT("set_lighting_zone_modified"), false);
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
