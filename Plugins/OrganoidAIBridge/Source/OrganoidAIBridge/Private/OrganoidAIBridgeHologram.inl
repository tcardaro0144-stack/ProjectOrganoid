	const TCHAR* HologramBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram");
	const TCHAR* HologramBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram");
	const TCHAR* HologramLabel = TEXT("Admin_FacilityHologram");
	const TCHAR* HologramParentName = TEXT("DefaultSceneRoot");
	const TCHAR* HologramBoolCategory = TEXT("Admin|Hologram");
	const TCHAR* HologramCubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* ApplyHologramStateName = TEXT("ApplyHologramState");
	const FVector HologramLocation(2400.f, 0.f, 0.f);
	const FRotator HologramRotation(0.f, 0.f, 0.f);
	const FVector HologramScale(1.f, 1.f, 1.f);
	const float HologramOnlineIntensity = 8000.f;
	const float HologramUnknownIntensity = 350.f;
	const float HologramLightRadius = 120.f;
	const FLinearColor HologramOnlineColor(0.15f, 1.0f, 0.75f, 1.0f);
	const FLinearColor HologramUnknownColor(0.12f, 0.14f, 0.18f, 1.0f);

	struct FHologramBoolSpec
	{
		const TCHAR* Name;
		bool bDefault;
	};

	const FHologramBoolSpec HologramBoolSpecs[] = {
		{TEXT("bAdminOnline"), true},
		{TEXT("bNeuroGeneticsOnline"), false},
		{TEXT("bCryoOnline"), false},
		{TEXT("bComputeOnline"), false},
		{TEXT("bReactorOnline"), false},
	};

	enum class EHologramCompKind : uint8
	{
		Mesh,
		Light
	};

	struct FHologramCompSpec
	{
		const TCHAR* Name;
		EHologramCompKind Kind;
		FVector RelativeLocation;
		FVector RelativeScale;
		bool bVisible;
		bool bHiddenInGame;
		float Intensity;
		FLinearColor Color;
	};

	const FHologramCompSpec HologramCompSpecs[] = {
		{TEXT("Pedestal"), EHologramCompKind::Mesh, FVector(0.f, 0.f, 20.f), FVector(0.7f, 0.7f, 0.4f), true, false, 0.f, FLinearColor::White},
		{TEXT("Column"), EHologramCompKind::Mesh, FVector(0.f, 0.f, 160.f), FVector(0.22f, 0.22f, 2.8f), true, false, 0.f, FLinearColor::White},
		{TEXT("Indicator_Admin_Online"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 250.f), FVector(0.12f, 0.12f, 0.12f), true, false, 0.f, FLinearColor::White},
		{TEXT("Indicator_Admin_Unknown"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 250.f), FVector(0.12f, 0.12f, 0.12f), false, true, 0.f, FLinearColor::White},
		{TEXT("Light_Admin"), EHologramCompKind::Light, FVector(18.f, 0.f, 250.f), FVector(1.f, 1.f, 1.f), true, false, HologramOnlineIntensity, HologramOnlineColor},
		{TEXT("Indicator_NeuroGenetics_Online"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 210.f), FVector(0.12f, 0.12f, 0.12f), false, true, 0.f, FLinearColor::White},
		{TEXT("Indicator_NeuroGenetics_Unknown"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 210.f), FVector(0.12f, 0.12f, 0.12f), true, false, 0.f, FLinearColor::White},
		{TEXT("Light_NeuroGenetics"), EHologramCompKind::Light, FVector(18.f, 0.f, 210.f), FVector(1.f, 1.f, 1.f), true, false, HologramUnknownIntensity, HologramUnknownColor},
		{TEXT("Indicator_Cryo_Online"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 170.f), FVector(0.12f, 0.12f, 0.12f), false, true, 0.f, FLinearColor::White},
		{TEXT("Indicator_Cryo_Unknown"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 170.f), FVector(0.12f, 0.12f, 0.12f), true, false, 0.f, FLinearColor::White},
		{TEXT("Light_Cryo"), EHologramCompKind::Light, FVector(18.f, 0.f, 170.f), FVector(1.f, 1.f, 1.f), true, false, HologramUnknownIntensity, HologramUnknownColor},
		{TEXT("Indicator_Compute_Online"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 130.f), FVector(0.12f, 0.12f, 0.12f), false, true, 0.f, FLinearColor::White},
		{TEXT("Indicator_Compute_Unknown"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 130.f), FVector(0.12f, 0.12f, 0.12f), true, false, 0.f, FLinearColor::White},
		{TEXT("Light_Compute"), EHologramCompKind::Light, FVector(18.f, 0.f, 130.f), FVector(1.f, 1.f, 1.f), true, false, HologramUnknownIntensity, HologramUnknownColor},
		{TEXT("Indicator_Reactor_Online"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 90.f), FVector(0.12f, 0.12f, 0.12f), false, true, 0.f, FLinearColor::White},
		{TEXT("Indicator_Reactor_Unknown"), EHologramCompKind::Mesh, FVector(18.f, 0.f, 90.f), FVector(0.12f, 0.12f, 0.12f), true, false, 0.f, FLinearColor::White},
		{TEXT("Light_Reactor"), EHologramCompKind::Light, FVector(18.f, 0.f, 90.f), FVector(1.f, 1.f, 1.f), true, false, HologramUnknownIntensity, HologramUnknownColor},
	};

	struct FHologramSectorSpec
	{
		const TCHAR* BoolName;
		const TCHAR* OnlineMesh;
		const TCHAR* UnknownMesh;
		const TCHAR* Light;
	};

	const FHologramSectorSpec HologramSectors[] = {
		{TEXT("bAdminOnline"), TEXT("Indicator_Admin_Online"), TEXT("Indicator_Admin_Unknown"), TEXT("Light_Admin")},
		{TEXT("bNeuroGeneticsOnline"), TEXT("Indicator_NeuroGenetics_Online"), TEXT("Indicator_NeuroGenetics_Unknown"), TEXT("Light_NeuroGenetics")},
		{TEXT("bCryoOnline"), TEXT("Indicator_Cryo_Online"), TEXT("Indicator_Cryo_Unknown"), TEXT("Light_Cryo")},
		{TEXT("bComputeOnline"), TEXT("Indicator_Compute_Online"), TEXT("Indicator_Compute_Unknown"), TEXT("Light_Compute")},
		{TEXT("bReactorOnline"), TEXT("Indicator_Reactor_Online"), TEXT("Indicator_Reactor_Unknown"), TEXT("Light_Reactor")},
	};

	bool IsHologramPackage(const FString& Path)
	{
		const FString Normalized = NormalizePackage(Path);
		return PackagesEqual(Normalized, HologramBpPath) || PackagesEqual(Normalized, HologramBpPackage);
	}

	bool FindHologramBoolSpec(const FString& Name, bool& OutDefault)
	{
		for (const FHologramBoolSpec& Spec : HologramBoolSpecs)
		{
			if (Name.Equals(Spec.Name, ESearchCase::CaseSensitive))
			{
				OutDefault = Spec.bDefault;
				return true;
			}
		}
		return false;
	}

	bool ParseExplicitBool(const TSharedPtr<FJsonObject>& Object, const FString& Field, bool& OutValue, bool& bHasValue)
	{
		bHasValue = false;
		if (!Object.IsValid() || !Object->HasField(Field))
		{
			return true;
		}
		const TSharedPtr<FJsonValue> Value = Object->TryGetField(Field);
		if (!Value.IsValid() || Value->IsNull())
		{
			return false;
		}
		if (Value->Type == EJson::Boolean)
		{
			OutValue = Value->AsBool();
			bHasValue = true;
			return true;
		}
		if (Value->Type == EJson::String)
		{
			const FString Token = Value->AsString();
			if (Token.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Token == TEXT("1"))
			{
				OutValue = true;
				bHasValue = true;
				return true;
			}
			if (Token.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Token == TEXT("0"))
			{
				OutValue = false;
				bHasValue = true;
				return true;
			}
		}
		return false;
	}

	FString LinearColorPinValue(const FLinearColor& Color)
	{
		return FString::Printf(
			TEXT("(R=%f,G=%f,B=%f,A=%f)"),
			Color.R, Color.G, Color.B, Color.A);
	}

	UEdGraphPin* FindExecPin(UEdGraphNode* Node, EEdGraphPinDirection Direction)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == Direction)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UEdGraphPin* FindPinByName(UEdGraphNode* Node, const FName PinName, EEdGraphPinDirection Direction)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction && Pin->PinName == PinName)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UEdGraphPin* FindFirstDataOutput(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	bool ConnectSchemaPins(const UEdGraphSchema_K2* Schema, UEdGraphPin* From, UEdGraphPin* To, FString& OutError)
	{
		if (!Schema || !From || !To)
		{
			OutError = TEXT("Pin connect failed: missing pin or schema.");
			return false;
		}
		const FPinConnectionResponse Response = Schema->CanCreateConnection(From, To);
		if (!Response.CanSafeConnect() && Response.Response != CONNECT_RESPONSE_BREAK_OTHERS_A && Response.Response != CONNECT_RESPONSE_BREAK_OTHERS_B
			&& Response.Response != CONNECT_RESPONSE_BREAK_OTHERS_AB)
		{
			OutError = FString::Printf(TEXT("Cannot connect %s to %s: %s"), *From->PinName.ToString(), *To->PinName.ToString(), *Response.Message.ToString());
			return false;
		}
		if (!Schema->TryCreateConnection(From, To))
		{
			OutError = FString::Printf(TEXT("TryCreateConnection failed for %s -> %s."), *From->PinName.ToString(), *To->PinName.ToString());
			return false;
		}
		return true;
	}

	FString RequireHologramBlueprint(const TSharedPtr<FJsonObject>& Args, UBlueprint*& OutBlueprint)
	{
		OutBlueprint = nullptr;
		const FString Path = GetString(Args, TEXT("path"), GetString(Args, TEXT("blueprint")));
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"), HologramBpPackage)));
		if (!IsHologramPackage(Path))
		{
			return TEXT("This action is allowlisted only for BP_AdminFacilityHologram.");
		}
		if (!PackagesEqual(RequiredPackage, HologramBpPackage))
		{
			return TEXT("required_package must be /Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram.");
		}
		OutBlueprint = LoadBlueprintAsset(Path);
		if (!OutBlueprint)
		{
			return TEXT("BP_AdminFacilityHologram not found.");
		}
		if (!OutBlueprint->ParentClass || OutBlueprint->ParentClass != AActor::StaticClass())
		{
			return TEXT("BP_AdminFacilityHologram parent must remain Actor.");
		}
		if (Path.Contains(TEXT("BP_AdminAccessDoor")) || Path.Contains(TEXT("BP_AdminTerminal")))
		{
			return TEXT("Refusing to mutate a non-hologram Admin Blueprint.");
		}
		return TEXT("");
	}

	FString PreflightAddHologramBoolVariables(
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

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireHologramBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}

		const FString Spec = GetString(Args, TEXT("spec"));
		const FString PinCategory = GetString(Args, TEXT("pin_category")).ToLower();
		const FString Category = GetString(Args, TEXT("category"), HologramBoolCategory);
		if (!Category.Equals(HologramBoolCategory, ESearchCase::CaseSensitive))
		{
			return TEXT("Hologram bool category must be Admin|Hologram.");
		}
		if (!PinCategory.IsEmpty() && PinCategory != TEXT("bool") && PinCategory != TEXT("boolean"))
		{
			return TEXT("Hologram variables must use pin_category=bool.");
		}

		TArray<TPair<FString, bool>> ToAdd;
		if (Spec == TEXT("s19_facility_hologram_bools_v1"))
		{
			for (const FHologramBoolSpec& BoolSpec : HologramBoolSpecs)
			{
				ToAdd.Add(TPair<FString, bool>(BoolSpec.Name, BoolSpec.bDefault));
			}
		}
		else
		{
			const FString VarName = GetString(Args, TEXT("variable_name"));
			bool bDefault = false;
			bool bHasDefault = false;
			if (!ParseExplicitBool(Args, TEXT("default"), bDefault, bHasDefault) && !ParseExplicitBool(Args, TEXT("default_value"), bDefault, bHasDefault))
			{
				return TEXT("explicit default true/false is required for bool variables.");
			}
			if (!bHasDefault)
			{
				bool bAlt = false;
				bool bHasAlt = false;
				ParseExplicitBool(Args, TEXT("default_value"), bAlt, bHasAlt);
				if (bHasAlt)
				{
					bDefault = bAlt;
					bHasDefault = true;
				}
			}
			if (!bHasDefault)
			{
				return TEXT("explicit default true/false is required for bool variables.");
			}
			bool bApprovedDefault = false;
			if (!FindHologramBoolSpec(VarName, bApprovedDefault))
			{
				return TEXT("Bool variable is not on the Section 19 hologram allowlist.");
			}
			if (bDefault != bApprovedDefault)
			{
				return FString::Printf(
					TEXT("Default for %s must be %s."),
					*VarName,
					bApprovedDefault ? TEXT("true") : TEXT("false"));
			}
			ToAdd.Add(TPair<FString, bool>(VarName, bDefault));
		}

		TSet<FName> CurrentVars;
		FBlueprintEditorUtils::GetClassVariableList(Blueprint, CurrentVars, true);
		for (const TPair<FString, bool>& Entry : ToAdd)
		{
			const FName VarFName(*Entry.Key);
			if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarFName) != INDEX_NONE || CurrentVars.Contains(VarFName))
			{
				return FString::Printf(TEXT("Variable '%s' already exists."), *Entry.Key);
			}
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));

		TArray<TSharedPtr<FJsonValue>> ProposedVars;
		for (const TPair<FString, bool>& Entry : ToAdd)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("variable_name"), Entry.Key);
			Row->SetStringField(TEXT("pin_category"), TEXT("bool"));
			Row->SetBoolField(TEXT("default"), Entry.Value);
			Row->SetStringField(TEXT("category"), HologramBoolCategory);
			Row->SetBoolField(TEXT("instance_editable"), true);
			ProposedVars.Add(MakeShared<FJsonValueObject>(Row));
		}
		Proposed->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Proposed->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Proposed->SetStringField(TEXT("pin_category"), TEXT("bool"));
		Proposed->SetArrayField(TEXT("variables"), ProposedVars);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Add hologram bool member variable(s) only. Package not saved. Full compile not performed."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAddHologramBoolVariables(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAddHologramBoolVariables(Change.Args, Before, Proposed);
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

		const TArray<TSharedPtr<FJsonValue>>* ProposedVars = nullptr;
		if (!Proposed->TryGetArrayField(TEXT("variables"), ProposedVars) || !ProposedVars)
		{
			return FailAudit(TEXT("preflight_failed"), TEXT("Proposed hologram variables missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		FEdGraphPinType PinType;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		for (const TSharedPtr<FJsonValue>& Value : *ProposedVars)
		{
			const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
			const FString VarName = GetString(Row, TEXT("variable_name"));
			const bool bDefault = GetBool(Row, TEXT("default"), false);
			const FName VarFName(*VarName);
			if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarFName, PinType, bDefault ? TEXT("true") : TEXT("false")))
			{
				return FailAudit(
					TEXT("add_failed"),
					FString::Printf(TEXT("AddMemberVariable rejected bool '%s'. ZERO further writes."), *VarName),
					MakeShared<FBridgeChange>(Change));
			}
			FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarFName, false);
			FBlueprintEditorUtils::SetBlueprintVariableCategory(
				Blueprint, VarFName, nullptr, FText::FromString(HologramBoolCategory), /*bDontRecompile=*/true);
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		TSharedRef<FJsonObject> After = VariableListSnapshot(Blueprint);
		After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		After->SetBoolField(TEXT("save"), false);
		After->SetBoolField(TEXT("compile"), false);
		After->SetStringField(TEXT("note"), TEXT("Hologram bool member(s) added. Package not saved. Full compile not performed."));
		Change.After = After;
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightAddScsComponent(
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
			return TEXT("save and compile must be false. add_scs_component does not save or compile.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireHologramBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		if (!Blueprint->SimpleConstructionScript)
		{
			return TEXT("BP_AdminFacilityHologram has no SimpleConstructionScript.");
		}
		USCS_Node* Root = Blueprint->SimpleConstructionScript->GetDefaultSceneRootNode();
		if (!Root || !Root->GetVariableName().ToString().Equals(HologramParentName, ESearchCase::CaseSensitive))
		{
			return TEXT("DefaultSceneRoot is required as the hologram SCS parent.");
		}

		const FString Spec = GetString(Args, TEXT("spec"));
		const FString Parent = GetString(Args, TEXT("parent"), GetString(Args, TEXT("attach_parent"), HologramParentName));
		if (!Parent.Equals(HologramParentName, ESearchCase::CaseSensitive))
		{
			return TEXT("Hologram SCS parent must be DefaultSceneRoot.");
		}
		if (Spec != TEXT("s19_facility_hologram_v1"))
		{
			return TEXT("add_scs_component currently requires spec=s19_facility_hologram_v1.");
		}

		for (const FHologramCompSpec& Comp : HologramCompSpecs)
		{
			if (Blueprint->SimpleConstructionScript->FindSCSNode(FName(Comp.Name)))
			{
				return FString::Printf(TEXT("SCS component '%s' already exists."), Comp.Name);
			}
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetStringField(TEXT("attach_parent"), HologramParentName);
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));
		Proposed->SetStringField(TEXT("spec"), TEXT("s19_facility_hologram_v1"));
		Proposed->SetNumberField(TEXT("component_count"), UE_ARRAY_COUNT(HologramCompSpecs));
		Proposed->SetStringField(TEXT("mesh"), HologramCubeMesh);
		Proposed->SetStringField(TEXT("collision"), TEXT("NoCollision"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Add approved hologram SCS meshes and lights. Package not saved. Compile not performed."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAddScsComponent(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAddScsComponent(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(GetString(Change.Args, TEXT("path")));
		USimpleConstructionScript* SCS = Blueprint ? Blueprint->SimpleConstructionScript : nullptr;
		USCS_Node* Root = SCS ? SCS->GetDefaultSceneRootNode() : nullptr;
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, HologramCubeMesh);
		if (!Blueprint || !SCS || !Root || !Cube)
		{
			return FailAudit(TEXT("not_found"), TEXT("Hologram Blueprint, SCS root, or Cube mesh missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		Blueprint->Modify();
		SCS->Modify();
		Root->Modify();

		for (const FHologramCompSpec& Comp : HologramCompSpecs)
		{
			UClass* CompClass = (Comp.Kind == EHologramCompKind::Light)
				? (UClass*)UPointLightComponent::StaticClass()
				: (UClass*)UStaticMeshComponent::StaticClass();
			USCS_Node* Node = SCS->CreateNode(CompClass, FName(Comp.Name));
			if (!Node || !Node->ComponentTemplate)
			{
				return FailAudit(
					TEXT("add_failed"),
					FString::Printf(TEXT("Failed to create SCS node '%s'. ZERO further writes."), Comp.Name),
					MakeShared<FBridgeChange>(Change));
			}
			if (USceneComponent* Scene = Cast<USceneComponent>(Node->ComponentTemplate))
			{
				Scene->SetRelativeLocation(Comp.RelativeLocation);
				Scene->SetRelativeRotation(FRotator::ZeroRotator);
				Scene->SetRelativeScale3D(Comp.RelativeScale);
				Scene->SetVisibility(Comp.bVisible, false);
				Scene->SetHiddenInGame(Comp.bHiddenInGame, false);
				Scene->SetMobility(EComponentMobility::Movable);
			}
			if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Node->ComponentTemplate))
			{
				Mesh->SetStaticMesh(Cube);
				Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
				Mesh->SetGenerateOverlapEvents(false);
				Mesh->SetCanEverAffectNavigation(false);
			}
			if (UPointLightComponent* Light = Cast<UPointLightComponent>(Node->ComponentTemplate))
			{
				Light->SetIntensity(Comp.Intensity);
				Light->SetLightColor(Comp.Color);
				Light->SetAttenuationRadius(HologramLightRadius);
				Light->SetCastShadows(false);
				Light->bUseInverseSquaredFalloff = true;
			}
			Root->AddChildNode(Node);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Change.After->SetNumberField(TEXT("component_count"), UE_ARRAY_COUNT(HologramCompSpecs));
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("Approved hologram SCS components added. Package not saved. Compile not performed."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	UK2Node_CallFunction* SpawnCallFunction(
		UEdGraph* Graph,
		UFunction* Function,
		int32 PosX,
		int32 PosY)
	{
		if (!Graph || !Function)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
		UK2Node_CallFunction* Node = Creator.CreateNode();
		Node->SetFromFunction(Function);
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	UK2Node_VariableGet* SpawnSelfGet(UEdGraph* Graph, const FName VarName, int32 PosX, int32 PosY)
	{
		if (!Graph)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
		UK2Node_VariableGet* Node = Creator.CreateNode();
		Node->VariableReference.SetSelfMember(VarName);
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	UK2Node_IfThenElse* SpawnBranch(UEdGraph* Graph, int32 PosX, int32 PosY)
	{
		if (!Graph)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_IfThenElse> Creator(*Graph);
		UK2Node_IfThenElse* Node = Creator.CreateNode();
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	UK2Node_CallFunction* SpawnSelfFunctionCall(UEdGraph* Graph, const FName FunctionName, int32 PosX, int32 PosY)
	{
		if (!Graph)
		{
			return nullptr;
		}
		FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
		UK2Node_CallFunction* Node = Creator.CreateNode();
		Node->FunctionReference.SetSelfMember(FunctionName);
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Creator.Finalize();
		return Node;
	}

	bool SetBoolPin(UEdGraphNode* Node, const FName PinName, bool bValue)
	{
		if (UEdGraphPin* Pin = FindPinByName(Node, PinName, EGPD_Input))
		{
			Pin->DefaultValue = bValue ? TEXT("true") : TEXT("false");
			return true;
		}
		return false;
	}

	bool SetFloatPin(UEdGraphNode* Node, const FName PinName, float Value)
	{
		if (UEdGraphPin* Pin = FindPinByName(Node, PinName, EGPD_Input))
		{
			Pin->DefaultValue = FString::SanitizeFloat(Value);
			return true;
		}
		return false;
	}

	bool SetColorPin(UEdGraphNode* Node, const FName PinName, const FLinearColor& Color)
	{
		if (UEdGraphPin* Pin = FindPinByName(Node, PinName, EGPD_Input))
		{
			Pin->DefaultValue = LinearColorPinValue(Color);
			return true;
		}
		return false;
	}

	FString WireVisibilityAndLight(
		const UEdGraphSchema_K2* Schema,
		UEdGraph* Graph,
		UEdGraphPin*& InOutExec,
		const FHologramSectorSpec& Sector,
		bool bOnline,
		int32 BaseX,
		int32 BaseY)
	{
		UFunction* SetVisibilityFn = USceneComponent::StaticClass()->FindFunctionByName(TEXT("SetVisibility"));
		UFunction* SetHiddenFn = USceneComponent::StaticClass()->FindFunctionByName(TEXT("SetHiddenInGame"));
		UFunction* SetIntensityFn = ULightComponent::StaticClass()->FindFunctionByName(TEXT("SetIntensity"));
		UFunction* SetColorFn = ULightComponent::StaticClass()->FindFunctionByName(TEXT("SetLightColor"));
		if (!SetVisibilityFn || !SetHiddenFn || !SetIntensityFn || !SetColorFn)
		{
			return TEXT("Engine light/scene functions not found.");
		}

		auto WireMesh = [&](const TCHAR* MeshName, bool bVisible, int32 X) -> FString
		{
			UK2Node_VariableGet* MeshGet = SpawnSelfGet(Graph, FName(MeshName), X - 280, BaseY);
			UK2Node_CallFunction* Vis = SpawnCallFunction(Graph, SetVisibilityFn, X, BaseY);
			UK2Node_CallFunction* Hidden = SpawnCallFunction(Graph, SetHiddenFn, X + 280, BaseY);
			if (!MeshGet || !Vis || !Hidden)
			{
				return TEXT("Failed to spawn mesh visibility nodes.");
			}
			SetBoolPin(Vis, TEXT("bNewVisibility"), bVisible);
			SetBoolPin(Vis, TEXT("bPropagateToChildren"), false);
			SetBoolPin(Hidden, TEXT("bNewHidden"), !bVisible);
			SetBoolPin(Hidden, TEXT("bPropagateToChildren"), false);
			FString Error;
			if (!ConnectSchemaPins(Schema, FindFirstDataOutput(MeshGet), FindPinByName(Vis, UEdGraphSchema_K2::PN_Self, EGPD_Input), Error)
				|| !ConnectSchemaPins(Schema, FindFirstDataOutput(MeshGet), FindPinByName(Hidden, UEdGraphSchema_K2::PN_Self, EGPD_Input), Error)
				|| !ConnectSchemaPins(Schema, InOutExec, FindExecPin(Vis, EGPD_Input), Error)
				|| !ConnectSchemaPins(Schema, FindExecPin(Vis, EGPD_Output), FindExecPin(Hidden, EGPD_Input), Error))
			{
				return Error;
			}
			InOutExec = FindExecPin(Hidden, EGPD_Output);
			return TEXT("");
		};

		FString Error = WireMesh(Sector.OnlineMesh, bOnline, BaseX);
		if (!Error.IsEmpty())
		{
			return Error;
		}
		Error = WireMesh(Sector.UnknownMesh, !bOnline, BaseX + 560);
		if (!Error.IsEmpty())
		{
			return Error;
		}

		UK2Node_VariableGet* LightGet = SpawnSelfGet(Graph, FName(Sector.Light), BaseX + 800, BaseY + 160);
		UK2Node_CallFunction* Intensity = SpawnCallFunction(Graph, SetIntensityFn, BaseX + 1080, BaseY + 160);
		UK2Node_CallFunction* Color = SpawnCallFunction(Graph, SetColorFn, BaseX + 1360, BaseY + 160);
		if (!LightGet || !Intensity || !Color)
		{
			return TEXT("Failed to spawn light nodes.");
		}
		SetFloatPin(Intensity, TEXT("NewIntensity"), bOnline ? HologramOnlineIntensity : HologramUnknownIntensity);
		SetColorPin(Color, TEXT("NewLightColor"), bOnline ? HologramOnlineColor : HologramUnknownColor);
		if (!ConnectSchemaPins(Schema, FindFirstDataOutput(LightGet), FindPinByName(Intensity, UEdGraphSchema_K2::PN_Self, EGPD_Input), Error)
			|| !ConnectSchemaPins(Schema, FindFirstDataOutput(LightGet), FindPinByName(Color, UEdGraphSchema_K2::PN_Self, EGPD_Input), Error)
			|| !ConnectSchemaPins(Schema, InOutExec, FindExecPin(Intensity, EGPD_Input), Error)
			|| !ConnectSchemaPins(Schema, FindExecPin(Intensity, EGPD_Output), FindExecPin(Color, EGPD_Input), Error))
		{
			return Error;
		}
		InOutExec = FindExecPin(Color, EGPD_Output);
		return TEXT("");
	}

	FString PreflightAuthorHologramApplyState(
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
			return TEXT("save and compile must be false. author_hologram_apply_state does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != TEXT("s19_facility_hologram_v1"))
		{
			return TEXT("author_hologram_apply_state requires spec=s19_facility_hologram_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireHologramBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		if (!Blueprint->SimpleConstructionScript)
		{
			return TEXT("Hologram SCS missing. Add components first.");
		}
		for (const FHologramBoolSpec& BoolSpec : HologramBoolSpecs)
		{
			if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, FName(BoolSpec.Name)) == INDEX_NONE)
			{
				return FString::Printf(TEXT("Missing hologram bool '%s'."), BoolSpec.Name);
			}
		}
		for (const FHologramCompSpec& Comp : HologramCompSpecs)
		{
			if (!Blueprint->SimpleConstructionScript->FindSCSNode(FName(Comp.Name)))
			{
				return FString::Printf(TEXT("Missing hologram SCS component '%s'."), Comp.Name);
			}
		}
		for (UEdGraph* ExistingGraph : Blueprint->FunctionGraphs)
		{
			if (ExistingGraph && ExistingGraph->GetFName() == FName(ApplyHologramStateName))
			{
				return TEXT("ApplyHologramState already exists.");
			}
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetObjectField(TEXT("members"), VariableListSnapshot(Blueprint));
		Proposed->SetStringField(TEXT("function"), ApplyHologramStateName);
		Proposed->SetStringField(TEXT("construction_script"), TEXT("Call ApplyHologramState. No Tick. No EventGraph gameplay."));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAuthorHologramApplyState(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAuthorHologramApplyState(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(GetString(Change.Args, TEXT("path")));
		if (!Blueprint)
		{
			return FailAudit(TEXT("not_found"), TEXT("Blueprint not found. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
			Blueprint, FName(ApplyHologramStateName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
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
			return FailAudit(TEXT("add_failed"), TEXT("Function entry or schema missing after graph create. ZERO further writes."), MakeShared<FBridgeChange>(Change));
		}

		FGraphNodeCreator<UK2Node_ExecutionSequence> SequenceCreator(*Graph);
		UK2Node_ExecutionSequence* Sequence = SequenceCreator.CreateNode();
		Sequence->NodePosX = 240;
		Sequence->NodePosY = 0;
		SequenceCreator.Finalize();
		for (int32 Extra = 0; Extra < 3; ++Extra)
		{
			Sequence->AddInputPin();
		}
		FString WireError;
		if (!ConnectSchemaPins(Schema, FindExecPin(Entry, EGPD_Output), FindExecPin(Sequence, EGPD_Input), WireError))
		{
			return FailAudit(TEXT("connect_failed"), WireError, MakeShared<FBridgeChange>(Change));
		}
		int32 RowY = 0;
		int32 SectorIndex = 0;
		for (const FHologramSectorSpec& Sector : HologramSectors)
		{
			UK2Node_VariableGet* BoolGet = SpawnSelfGet(Graph, FName(Sector.BoolName), 480, RowY);
			UK2Node_IfThenElse* Branch = SpawnBranch(Graph, 760, RowY);
			UEdGraphPin* SeqThen = Sequence->GetThenPinGivenIndex(SectorIndex);
			if (!BoolGet || !Branch || !SeqThen)
			{
				return FailAudit(TEXT("add_failed"), TEXT("Failed to spawn sector branch."), MakeShared<FBridgeChange>(Change));
			}
			if (!ConnectSchemaPins(Schema, SeqThen, FindExecPin(Branch, EGPD_Input), WireError)
				|| !ConnectSchemaPins(Schema, FindFirstDataOutput(BoolGet), FindPinByName(Branch, UEdGraphSchema_K2::PN_Condition, EGPD_Input), WireError))
			{
				return FailAudit(TEXT("connect_failed"), WireError, MakeShared<FBridgeChange>(Change));
			}
			UEdGraphPin* ThenExec = FindPinByName(Branch, UEdGraphSchema_K2::PN_Then, EGPD_Output);
			UEdGraphPin* ElseExec = FindPinByName(Branch, UEdGraphSchema_K2::PN_Else, EGPD_Output);
			WireError = WireVisibilityAndLight(Schema, Graph, ThenExec, Sector, true, 1100, RowY - 40);
			if (!WireError.IsEmpty())
			{
				return FailAudit(TEXT("connect_failed"), WireError, MakeShared<FBridgeChange>(Change));
			}
			WireError = WireVisibilityAndLight(Schema, Graph, ElseExec, Sector, false, 1100, RowY + 220);
			if (!WireError.IsEmpty())
			{
				return FailAudit(TEXT("connect_failed"), WireError, MakeShared<FBridgeChange>(Change));
			}
			RowY += 520;
			++SectorIndex;
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
		UK2Node_CallFunction* ApplyCall = SpawnSelfFunctionCall(Construction, FName(ApplyHologramStateName), 400, 0);
		const UEdGraphSchema_K2* ConstructionSchema = Cast<const UEdGraphSchema_K2>(Construction->GetSchema());
		if (!ConstructionEntry || !ApplyCall || !ConstructionSchema)
		{
			return FailAudit(TEXT("add_failed"), TEXT("Failed to wire Construction Script call."), MakeShared<FBridgeChange>(Change));
		}
		UEdGraphPin* ConstructionThen = FindExecPin(ConstructionEntry, EGPD_Output);
		UEdGraphPin* PreviousTarget = (ConstructionThen && ConstructionThen->LinkedTo.Num() > 0) ? ConstructionThen->LinkedTo[0] : nullptr;
		if (ConstructionThen)
		{
			ConstructionThen->BreakAllPinLinks();
		}
		if (!ConnectSchemaPins(ConstructionSchema, ConstructionThen, FindExecPin(ApplyCall, EGPD_Input), WireError))
		{
			return FailAudit(TEXT("connect_failed"), WireError, MakeShared<FBridgeChange>(Change));
		}
		if (PreviousTarget)
		{
			ConnectSchemaPins(ConstructionSchema, FindExecPin(ApplyCall, EGPD_Output), PreviousTarget, WireError);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("function"), ApplyHologramStateName);
		Change.After->SetBoolField(TEXT("construction_script_wired"), true);
		Change.After->SetBoolField(TEXT("tick"), false);
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		Change.After->SetStringField(TEXT("note"), TEXT("ApplyHologramState authored. Construction Script calls it. No Tick. Package not saved."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSpawnHologramActor(
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
		if (!Label.Equals(HologramLabel, ESearchCase::CaseSensitive))
		{
			return TEXT("Hologram spawn label must be Admin_FacilityHologram.");
		}
		if (!PackagesEqual(Destination, AdminPackage))
		{
			return TEXT("Hologram destination must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}
		if (!LocationMatches(Location, HologramLocation)
			|| !RotationMatches(FRotator(RotationVec.X, RotationVec.Y, RotationVec.Z), HologramRotation)
			|| !ScaleMatches(Scale, HologramScale))
		{
			return TEXT("Admin_FacilityHologram must use location (2400,0,0), rotation (0,0,0), scale (1,1,1).");
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
			return TEXT("Admin_FacilityHologram already exists. Fail closed.");
		}

		FString GuardError = GuardExistingActor(World, ReceptionLabel, ReceptionLocation, AdminPackage);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception guard failed: %s"), *GuardError);
		}
		GuardError = ReceptionConfigMismatch(FindByExactLabel(World, ReceptionLabel)[0]);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Reception config changed; refusing hologram spawn. %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, AccessDoorLabel, AccessDoorLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Access Door guard failed: %s"), *GuardError);
		}
		GuardError = GuardExistingActor(World, LegacySecurityLabel, LegacySecurityLocation, nullptr);
		if (!GuardError.IsEmpty())
		{
			return FString::Printf(TEXT("Legacy Terminal_AdminSecurity guard failed: %s"), *GuardError);
		}

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, *FString::Printf(TEXT("%s.%s"), *BlueprintPath, *FPaths::GetBaseFilename(BlueprintPath)));
		}
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return TEXT("BP_AdminFacilityHologram Blueprint or generated class not found.");
		}
		if (Blueprint->ParentClass != AActor::StaticClass())
		{
			return TEXT("BP_AdminFacilityHologram parent must remain Actor.");
		}

		Before->SetStringField(TEXT("destination_package"), Destination);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetStringField(TEXT("label"), Label);
		Proposed->SetArrayField(TEXT("location"), Vec(HologramLocation));
		Proposed->SetArrayField(TEXT("rotation"), Rot(HologramRotation));
		Proposed->SetArrayField(TEXT("scale"), Vec(HologramScale));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn Admin_FacilityHologram into loaded SL_Epitope_Admin. Does not save. Does not touch terminals or Access Door."));
		return TEXT("");
	}

	bool IsHologramSpawnArgs(const TSharedPtr<FJsonObject>& Args)
	{
		const FString BlueprintPath = NormalizeBlueprintPath(
			GetString(Args, TEXT("blueprint"), GetString(Args, TEXT("class_path"), GetString(Args, TEXT("path")))));
		return IsHologramPackage(BlueprintPath);
	}
