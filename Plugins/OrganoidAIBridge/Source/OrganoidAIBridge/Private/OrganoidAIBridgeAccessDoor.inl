	const TCHAR* AccessDoorListenerSpec = TEXT("s21_access_door_listener_v1");
	const TCHAR* AccessDoorListenerComponentName = TEXT("FacilityStateListener");
	const TCHAR* AccessDoorListenerClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAdminAccessDoorFacilityStateListener");

	UClass* LoadAccessDoorListenerClass()
	{
		if (UClass* Loaded = FindObject<UClass>(nullptr, AccessDoorListenerClassPath))
		{
			return Loaded;
		}
		return LoadObject<UClass>(nullptr, AccessDoorListenerClassPath);
	}

	FString RequireAccessDoorBlueprint(const TSharedPtr<FJsonObject>& Args, UBlueprint*& OutBlueprint)
	{
		OutBlueprint = nullptr;
		FString Path = GetString(Args, TEXT("path"), GetString(Args, TEXT("blueprint")));
		if (Path.IsEmpty())
		{
			Path = AccessDoorBpPath;
		}
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"), AccessDoorBpPackage)));
		if (!PackagesEqual(NormalizePackage(Path), AccessDoorBpPackage)
			&& !Path.Contains(TEXT("BP_AdminAccessDoor")))
		{
			return TEXT("author_access_door_facility_state_listener is allowlisted only for BP_AdminAccessDoor.");
		}
		if (!PackagesEqual(RequiredPackage, AccessDoorBpPackage))
		{
			return TEXT("required_package must be /Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.");
		}
		if (Path.Contains(TEXT("BP_AdminTerminal"))
			|| Path.Contains(TEXT("BP_AdminFacilityHologram"))
			|| Path.Contains(TEXT("BP_AdminLightController"))
			|| Path.Contains(TEXT("BP_AdminRoomTrigger")))
		{
			return TEXT("Refusing to mutate a non-Access-Door Admin Blueprint.");
		}
		OutBlueprint = LoadBlueprintAsset(Path.IsEmpty() ? FString(AccessDoorBpPath) : Path);
		if (!OutBlueprint)
		{
			return TEXT("BP_AdminAccessDoor not found.");
		}
		if (!OutBlueprint->ParentClass || OutBlueprint->ParentClass != AActor::StaticClass())
		{
			return TEXT("BP_AdminAccessDoor parent must remain Actor.");
		}
		return TEXT("");
	}

	FString SnapshotAccessDoorScs(UBlueprint* Blueprint)
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

	FString PreflightAuthorAccessDoorFacilityStateListener(
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
			return TEXT("save and compile must be false. author_access_door_facility_state_listener does not save or compile.");
		}
		if (GetString(Args, TEXT("spec")) != AccessDoorListenerSpec)
		{
			return TEXT("author_access_door_facility_state_listener requires spec=s21_access_door_listener_v1.");
		}

		UBlueprint* Blueprint = nullptr;
		const FString PathError = RequireAccessDoorBlueprint(Args, Blueprint);
		if (!PathError.IsEmpty())
		{
			return PathError;
		}
		if (!Blueprint->SimpleConstructionScript)
		{
			return TEXT("BP_AdminAccessDoor has no SimpleConstructionScript.");
		}
		UClass* ListenerClass = LoadAccessDoorListenerClass();
		if (!ListenerClass)
		{
			return TEXT("ProjectOrganoidAdminAccessDoorFacilityStateListener is not loaded. Compile the game module first.");
		}
		if (!ListenerClass->IsChildOf(UActorComponent::StaticClass()) || ListenerClass->IsChildOf(USceneComponent::StaticClass()))
		{
			return TEXT("Listener class must be a non-scene UActorComponent.");
		}
		if (Blueprint->SimpleConstructionScript->FindSCSNode(FName(AccessDoorListenerComponentName)))
		{
			return TEXT("FacilityStateListener already exists on BP_AdminAccessDoor.");
		}

		Before->SetStringField(TEXT("owning_package"), Blueprint->GetOutermost()->GetName());
		Before->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Before->SetStringField(TEXT("parent"), Blueprint->ParentClass ? Blueprint->ParentClass->GetPathName() : TEXT(""));
		Before->SetStringField(TEXT("scs"), SnapshotAccessDoorScs(Blueprint));
		Before->SetBoolField(TEXT("eventgraph_untouched"), true);
		Proposed->SetStringField(TEXT("spec"), AccessDoorListenerSpec);
		Proposed->SetStringField(TEXT("component"), AccessDoorListenerComponentName);
		Proposed->SetStringField(TEXT("component_class"), AccessDoorListenerClassPath);
		Proposed->SetStringField(
			TEXT("note"),
			TEXT("Adds native FacilityStateListener SCS component only. Does not author EventGraph, Timeline, variables, or meshes. Package not saved. Compile not performed."));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteAuthorAccessDoorFacilityStateListener(FBridgeChange& Change)
	{
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightAuthorAccessDoorFacilityStateListener(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UBlueprint* Blueprint = nullptr;
		RequireAccessDoorBlueprint(Change.Args, Blueprint);
		USimpleConstructionScript* SCS = Blueprint ? Blueprint->SimpleConstructionScript : nullptr;
		UClass* ListenerClass = LoadAccessDoorListenerClass();
		if (!Blueprint || !SCS || !ListenerClass)
		{
			return FailAudit(TEXT("not_found"), TEXT("Access Door Blueprint, SCS, or listener class vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		if (SCS->FindSCSNode(FName(AccessDoorListenerComponentName)))
		{
			return FailAudit(TEXT("already_exists"), TEXT("FacilityStateListener already exists. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		Blueprint->Modify();
		SCS->Modify();
		USCS_Node* Node = SCS->CreateNode(ListenerClass, FName(AccessDoorListenerComponentName));
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
		Change.After->SetStringField(TEXT("component"), AccessDoorListenerComponentName);
		Change.After->SetStringField(TEXT("component_class"), ListenerClass->GetPathName());
		Change.After->SetStringField(TEXT("scs"), SnapshotAccessDoorScs(Blueprint));
		Change.After->SetBoolField(TEXT("eventgraph_authored"), false);
		Change.After->SetBoolField(TEXT("timeline_modified"), false);
		Change.After->SetBoolField(TEXT("save"), false);
		Change.After->SetBoolField(TEXT("compile"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
