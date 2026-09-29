// Beat 26 Admin world color: five zone materials, two Blueprints under
// /Game/Blueprints/Admin/, props, and material paint on Admin blockout meshes.
// Does not move mission actors. Does not retune S20 zone lights or S22 audio
// zones. Does not save; save_admin_world_color persists Admin + the seven packages.
	const TCHAR* AdminWorldColorSpec = TEXT("admin_world_color_v1");
	const TCHAR* AdminWorldColorAction = TEXT("create_admin_world_color");
	const TCHAR* AdminWorldColorSaveSpec = TEXT("admin_world_color_save_v1");
	const TCHAR* AdminWorldColorSaveAction = TEXT("save_admin_world_color");
	const TCHAR* AdminWorldColorFolder = TEXT("/Game/Materials/Admin");
	const TCHAR* AdminWorldColorBpFolder = TEXT("/Game/Blueprints/Admin");
	const TCHAR* AdminWorldColorLightBpPackage = TEXT("/Game/Blueprints/Admin/BP_AdminLightController");
	const TCHAR* AdminWorldColorLightBpPath = TEXT("/Game/Blueprints/Admin/BP_AdminLightController.BP_AdminLightController");
	const TCHAR* AdminWorldColorAudioBpPackage = TEXT("/Game/Blueprints/Admin/BP_AdminAudioZone");
	const TCHAR* AdminWorldColorAudioBpPath = TEXT("/Game/Blueprints/Admin/BP_AdminAudioZone.BP_AdminAudioZone");
	const TCHAR* AdminWorldColorLightLabel = TEXT("Admin_WorldColor_LightController");
	// Admin floor is Z≈110–320 (Reception at Z=110). User brief (0,0,-2310) is
	// below Admin geometry; place at Admin atrium/hub center instead.
	const FVector AdminWorldColorLightLocation(2280.f, 0.f, 280.f);
	const FRotator AdminWorldColorLightRotation(0.f, 0.f, 0.f);
	const FVector AdminWorldColorLightScale(1.f, 1.f, 1.f);
	const TCHAR* AdminWorldColorCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* AdminWorldColorPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* AdminWorldColorAlarmPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* AdminWorldColorAmbienceNative = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone");
	const TCHAR* LegacyLightControllerLabel = TEXT("Admin_LightController");

	enum class EAdminWorldColorZone : int32
	{
		Public = 0,
		Restricted,
		Executive,
		Operations,
		Service,
		Count
	};

	struct FAdminWorldColorMatSpec
	{
		const TCHAR* Name;
		EAdminWorldColorZone Zone;
		FLinearColor Color;
		float Roughness;
		float Metallic;
		float Emissive;
	};

	const FAdminWorldColorMatSpec AdminWorldColorMats[] = {
		{ TEXT("M_Admin_Public_Panel"), EAdminWorldColorZone::Public,
			FLinearColor(0.82f, 0.88f, 0.94f), 0.32f, 0.05f, 0.02f },
		{ TEXT("M_Admin_Restricted_Machinery"), EAdminWorldColorZone::Restricted,
			FLinearColor(0.18f, 0.24f, 0.32f), 0.55f, 0.45f, 0.0f },
		{ TEXT("M_Admin_Executive_Premium"), EAdminWorldColorZone::Executive,
			FLinearColor(0.74f, 0.58f, 0.40f), 0.42f, 0.08f, 0.0f },
		{ TEXT("M_Admin_Operations_Screen"), EAdminWorldColorZone::Operations,
			FLinearColor(0.05f, 0.28f, 0.48f), 0.22f, 0.15f, 1.1f },
		{ TEXT("M_Admin_Service_Industrial"), EAdminWorldColorZone::Service,
			FLinearColor(0.38f, 0.34f, 0.22f), 0.88f, 0.12f, 0.0f },
	};

	struct FAdminWorldColorPropSpec
	{
		const TCHAR* Label;
		EAdminWorldColorZone Zone;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bPlane;
		bool bText;
		const TCHAR* Text;
	};

	const FAdminWorldColorPropSpec AdminWorldColorProps[] = {
		{ TEXT("Admin_WC_Sign_Reception"), EAdminWorldColorZone::Public,
			FVector(1500.f, 420.f, 220.f), FRotator(0.f, -90.f, 0.f), FVector(1.4f, 0.08f, 0.55f), false, true,
			TEXT("EPITOPE ADMIN — PUBLIC LOBBY") },
		{ TEXT("Admin_WC_Sign_Security"), EAdminWorldColorZone::Restricted,
			FVector(2680.f, -620.f, 230.f), FRotator(0.f, 0.f, 0.f), FVector(1.2f, 0.08f, 0.45f), false, true,
			TEXT("RESTRICTED — CREDENTIALS REQUIRED") },
		{ TEXT("Admin_WC_Plant_Reception"), EAdminWorldColorZone::Public,
			FVector(1320.f, 260.f, 40.f), FRotator(0.f, 15.f, 0.f), FVector(0.35f, 0.35f, 0.9f), false, false, nullptr },
		{ TEXT("Admin_WC_Plant_Hub"), EAdminWorldColorZone::Public,
			FVector(2200.f, 280.f, 40.f), FRotator(0.f, -20.f, 0.f), FVector(0.4f, 0.4f, 1.05f), false, false, nullptr },
		{ TEXT("Admin_WC_Monitor_Ops1"), EAdminWorldColorZone::Operations,
			FVector(3650.f, -180.f, 180.f), FRotator(0.f, 90.f, 0.f), FVector(1.1f, 0.06f, 0.7f), false, false, nullptr },
		{ TEXT("Admin_WC_Monitor_Ops2"), EAdminWorldColorZone::Operations,
			FVector(3650.f, 180.f, 180.f), FRotator(0.f, 90.f, 0.f), FVector(1.1f, 0.06f, 0.7f), false, false, nullptr },
		{ TEXT("Admin_WC_Monitor_Security"), EAdminWorldColorZone::Restricted,
			FVector(2620.f, -320.f, 170.f), FRotator(0.f, 0.f, 0.f), FVector(0.9f, 0.05f, 0.55f), false, false, nullptr },
		{ TEXT("Admin_WC_Brand_Epitope"), EAdminWorldColorZone::Public,
			FVector(1450.f, 500.f, 260.f), FRotator(0.f, -90.f, 0.f), FVector(1.6f, 0.05f, 0.5f), false, true,
			TEXT("EPITOPE — Prepared Immunity.") },
		{ TEXT("Admin_WC_Brand_Visitor"), EAdminWorldColorZone::Public,
			FVector(480.f, 360.f, 210.f), FRotator(0.f, -90.f, 0.f), FVector(1.4f, 0.05f, 0.4f), false, true,
			TEXT("Visitor badge visible at all times.") },
		{ TEXT("Admin_WC_Decal_FloorHub"), EAdminWorldColorZone::Public,
			FVector(2280.f, 0.f, 2.f), FRotator(0.f, 0.f, 0.f), FVector(3.5f, 3.5f, 1.f), true, false, nullptr },
		{ TEXT("Admin_WC_Panel_Executive"), EAdminWorldColorZone::Executive,
			FVector(2600.f, -1850.f, 200.f), FRotator(0.f, 0.f, 0.f), FVector(1.8f, 0.08f, 1.2f), false, false, nullptr },
		{ TEXT("Admin_WC_Pipe_Service"), EAdminWorldColorZone::Service,
			FVector(3720.f, -900.f, 140.f), FRotator(0.f, 35.f, 0.f), FVector(2.4f, 0.18f, 0.18f), false, false, nullptr },
	};

	FString AdminWorldColorMatPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), AdminWorldColorFolder, Name, Name);
	}

	FString AdminWorldColorMatPackage(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s"), AdminWorldColorFolder, Name);
	}

	const FAdminWorldColorMatSpec* AdminWorldColorFindMat(EAdminWorldColorZone Zone)
	{
		for (const FAdminWorldColorMatSpec& Spec : AdminWorldColorMats)
		{
			if (Spec.Zone == Zone)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool AdminWorldColorColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.05f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.05f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.05f);
	}

	bool AdminWorldColorIsGray(const FLinearColor& Color)
	{
		const float Max = FMath::Max3(Color.R, Color.G, Color.B);
		const float Min = FMath::Min3(Color.R, Color.G, Color.B);
		return (Max - Min) < 0.08f && Max > 0.15f && Max < 0.85f;
	}

	FLinearColor AdminWorldColorReadMaterialColor(const UMaterialInterface* Material)
	{
		if (!Material)
		{
			return FLinearColor(0.5f, 0.5f, 0.5f);
		}
		if (const UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Material))
		{
			for (const FVectorParameterValue& Value : Instance->VectorParameterValues)
			{
				if (Value.ParameterInfo.Name == TEXT("Color") || Value.ParameterInfo.Name == TEXT("BaseColor"))
				{
					return Value.ParameterValue;
				}
			}
		}
		FLinearColor Out = FLinearColor::White;
		if (Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), Out)
			|| Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Out))
		{
			return Out;
		}
		return FLinearColor(0.5f, 0.5f, 0.5f);
	}

	void AdminWorldColorWireMaterial(UMaterial* Material, const FAdminWorldColorMatSpec& Spec)
	{
		UMaterialEditorOnlyData* EditorOnly = Material->GetEditorOnlyData();
		if (!EditorOnly)
		{
			return;
		}
		if (!EditorOnly->BaseColor.Expression)
		{
			UMaterialExpressionVectorParameter* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
			Color->ParameterName = TEXT("Color");
			Color->DefaultValue = Spec.Color;
			Color->MaterialExpressionEditorX = -420;
			Color->MaterialExpressionEditorY = 0;
			UMaterialExpressionScalarParameter* Roughness = NewObject<UMaterialExpressionScalarParameter>(Material);
			Roughness->ParameterName = TEXT("Roughness");
			Roughness->DefaultValue = Spec.Roughness;
			Roughness->MaterialExpressionEditorX = -420;
			Roughness->MaterialExpressionEditorY = 180;
			UMaterialExpressionScalarParameter* Metallic = NewObject<UMaterialExpressionScalarParameter>(Material);
			Metallic->ParameterName = TEXT("Metallic");
			Metallic->DefaultValue = Spec.Metallic;
			Metallic->MaterialExpressionEditorX = -420;
			Metallic->MaterialExpressionEditorY = 340;
			UMaterialExpressionScalarParameter* Emissive = NewObject<UMaterialExpressionScalarParameter>(Material);
			Emissive->ParameterName = TEXT("Emissive");
			Emissive->DefaultValue = Spec.Emissive;
			Emissive->MaterialExpressionEditorX = -420;
			Emissive->MaterialExpressionEditorY = 500;
			UMaterialExpressionMultiply* EmissiveColor = NewObject<UMaterialExpressionMultiply>(Material);
			EmissiveColor->A.Expression = Color;
			EmissiveColor->B.Expression = Emissive;
			EmissiveColor->MaterialExpressionEditorX = -160;
			EmissiveColor->MaterialExpressionEditorY = 500;
			EditorOnly->ExpressionCollection.AddExpression(Color);
			EditorOnly->ExpressionCollection.AddExpression(Roughness);
			EditorOnly->ExpressionCollection.AddExpression(Metallic);
			EditorOnly->ExpressionCollection.AddExpression(Emissive);
			EditorOnly->ExpressionCollection.AddExpression(EmissiveColor);
			EditorOnly->BaseColor.Expression = Color;
			EditorOnly->Roughness.Expression = Roughness;
			EditorOnly->Metallic.Expression = Metallic;
			EditorOnly->EmissiveColor.Expression = EmissiveColor;
		}
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	bool AdminWorldColorMaterialExact(const UMaterial* Material, const FAdminWorldColorMatSpec& Spec)
	{
		if (!Material)
		{
			return false;
		}
		const UMaterialEditorOnlyData* EditorOnly = Material->GetEditorOnlyData();
		if (!EditorOnly || !EditorOnly->BaseColor.Expression)
		{
			return false;
		}
		const UMaterialExpressionVectorParameter* ColorExpr =
			Cast<UMaterialExpressionVectorParameter>(EditorOnly->BaseColor.Expression);
		if (!ColorExpr || !AdminWorldColorColorNear(ColorExpr->DefaultValue, Spec.Color))
		{
			return false;
		}
		return !AdminWorldColorIsGray(ColorExpr->DefaultValue);
	}

	UMaterial* AdminWorldColorLoadMat(const TCHAR* Name)
	{
		return LoadObject<UMaterial>(nullptr, *AdminWorldColorMatPath(Name));
	}

	bool AdminWorldColorEnsureMaterials(FString& OutError)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		for (const FAdminWorldColorMatSpec& Spec : AdminWorldColorMats)
		{
			UMaterial* Material = AdminWorldColorLoadMat(Spec.Name);
			if (!Material)
			{
				UPackage* Package = CreatePackage(*AdminWorldColorMatPackage(Spec.Name));
				Material = Cast<UMaterial>(Factory->FactoryCreateNew(
					UMaterial::StaticClass(), Package, Spec.Name, RF_Public | RF_Standalone, nullptr, GWarn));
				if (!Material)
				{
					OutError = FString::Printf(TEXT("Failed to create %s."), Spec.Name);
					return false;
				}
				FAssetRegistryModule::AssetCreated(Material);
			}
			AdminWorldColorWireMaterial(Material, Spec);
			if (!AdminWorldColorMaterialExact(Material, Spec))
			{
				OutError = FString::Printf(TEXT("%s is not a colored Admin material."), Spec.Name);
				return false;
			}
		}
		return true;
	}

	UBlueprint* AdminWorldColorLoadLightBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, AdminWorldColorLightBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, AdminWorldColorLightBpPackage);
		}
		return Blueprint;
	}

	UBlueprint* AdminWorldColorLoadAudioBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, AdminWorldColorAudioBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, AdminWorldColorAudioBpPackage);
		}
		return Blueprint;
	}

	bool AdminWorldColorEnsureBlueprints(FString& OutError)
	{
		if (!AdminWorldColorLoadLightBp())
		{
			UPackage* Package = CreatePackage(AdminWorldColorLightBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				Package,
				TEXT("BP_AdminLightController"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created || !Created->SimpleConstructionScript)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Admin/BP_AdminLightController.");
				return false;
			}
			USCS_Node* LightNode = Created->SimpleConstructionScript->CreateNode(
				UPointLightComponent::StaticClass(), TEXT("PublicZoneLight"));
			Created->SimpleConstructionScript->AddNode(LightNode);
			if (UPointLightComponent* Template = Cast<UPointLightComponent>(LightNode->ComponentTemplate))
			{
				Template->Intensity = 5000.f;
				Template->SetTemperature(5000.f);
				Template->bUseTemperature = true;
				Template->SetLightColor(FLinearColor(1.f, 0.96f, 0.90f), true);
				Template->AttenuationRadius = 1200.f;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		if (!AdminWorldColorLoadAudioBp())
		{
			UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, AdminWorldColorAmbienceNative);
			if (!Native)
			{
				OutError = TEXT("ProjectOrganoidAmbienceZone is not loaded.");
				return false;
			}
			UPackage* Package = CreatePackage(AdminWorldColorAudioBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				Native,
				Package,
				TEXT("BP_AdminAudioZone"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Admin/BP_AdminAudioZone.");
				return false;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		UBlueprint* LightBp = AdminWorldColorLoadLightBp();
		UBlueprint* AudioBp = AdminWorldColorLoadAudioBp();
		if (!LightBp || !LightBp->GeneratedClass || LightBp->ParentClass != AActor::StaticClass())
		{
			OutError = TEXT("BP_AdminLightController under /Game/Blueprints/Admin/ is invalid.");
			return false;
		}
		UClass* AmbienceNative = StaticLoadClass(UObject::StaticClass(), nullptr, AdminWorldColorAmbienceNative);
		if (!AudioBp || !AudioBp->GeneratedClass || !AmbienceNative || !AudioBp->GeneratedClass->IsChildOf(AmbienceNative))
		{
			OutError = TEXT("BP_AdminAudioZone under /Game/Blueprints/Admin/ must parent AmbienceZone.");
			return false;
		}
		return true;
	}

	EAdminWorldColorZone AdminWorldColorClassifyLocation(const FVector& Location)
	{
		if (Location.Y < -1200.f)
		{
			return EAdminWorldColorZone::Executive;
		}
		if (Location.X > 4500.f || (Location.X > 3400.f && Location.Y < -400.f))
		{
			return EAdminWorldColorZone::Service;
		}
		if (Location.X > 3300.f)
		{
			return EAdminWorldColorZone::Operations;
		}
		if (Location.X > 2400.f)
		{
			return EAdminWorldColorZone::Restricted;
		}
		return EAdminWorldColorZone::Public;
	}

	EAdminWorldColorZone AdminWorldColorClassifyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return EAdminWorldColorZone::Public;
		}
		const FString Label = ActorLabel(const_cast<AActor*>(Actor));
		if (Label.Contains(TEXT("Executive"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Conference"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Director"), ESearchCase::IgnoreCase))
		{
			return EAdminWorldColorZone::Executive;
		}
		if (Label.Contains(TEXT("Operations"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Ops"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Screen"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Monitor"), ESearchCase::IgnoreCase))
		{
			return EAdminWorldColorZone::Operations;
		}
		if (Label.Contains(TEXT("Service"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Transit"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pipe"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Corridor"), ESearchCase::IgnoreCase))
		{
			return EAdminWorldColorZone::Service;
		}
		if (Label.Contains(TEXT("Security"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Records"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Restricted"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Machinery"), ESearchCase::IgnoreCase))
		{
			return EAdminWorldColorZone::Restricted;
		}
		return AdminWorldColorClassifyLocation(Actor->GetActorLocation());
	}

	bool AdminWorldColorIsProtectedLabel(const FString& Label)
	{
		static const TCHAR* Protected[] = {
			TEXT("Terminal_ControlSpine"),
			TEXT("Checkpoint_BasinRim"),
			TEXT("Admin_Terminal_"),
			TEXT("Admin_RoomTrigger_"),
			TEXT("Admin_ZoneLight_"),
			TEXT("Admin_LightController"),
			TEXT("Admin_AudioZone_"),
			TEXT("Admin_FacilityHologram"),
			TEXT("DoorLock_"),
			TEXT("BP_AdminAccessDoor"),
			TEXT("Gate_"),
			TEXT("NavMeshBounds"),
			TEXT("RecastNavMesh"),
			TEXT("Admin_Block2_"),
			TEXT("Admin_Brand_"),
		};
		for (const TCHAR* Prefix : Protected)
		{
			if (Label.Equals(Prefix, ESearchCase::CaseSensitive) || Label.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		return false;
	}

	bool AdminWorldColorIsPaintableMesh(const AActor* Actor)
	{
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !PackagesEqual(ActorOwningPackage(const_cast<AStaticMeshActor*>(MeshActor)), AdminPackage))
		{
			return false;
		}
		const FString Label = ActorLabel(const_cast<AStaticMeshActor*>(MeshActor));
		if (AdminWorldColorIsProtectedLabel(Label) || Label.StartsWith(TEXT("Admin_WC_"), ESearchCase::CaseSensitive))
		{
			return false;
		}
		if (!MeshActor->GetStaticMeshComponent() || !MeshActor->GetStaticMeshComponent()->GetStaticMesh())
		{
			return false;
		}
		const FString Lower = Label.ToLower();
		const bool bNameHit = Lower.Contains(TEXT("ceiling"))
			|| Lower.Contains(TEXT("floor"))
			|| Lower.Contains(TEXT("wall"))
			|| Lower.Contains(TEXT("tile"))
			|| Lower.Contains(TEXT("panel"))
			|| Lower.Contains(TEXT("block"))
			|| Lower.Contains(TEXT("slab"))
			|| Lower.Contains(TEXT("sm_"));
		const FVector Loc = MeshActor->GetActorLocation();
		const FVector Scale = MeshActor->GetActorScale3D();
		const bool bCeilingShape = Loc.Z > 250.f && Scale.Z < 0.35f;
		const bool bFloorShape = Loc.Z < 40.f && Scale.Z < 0.35f;
		return bNameHit || bCeilingShape || bFloorShape;
	}

	void AdminWorldColorCollectDirty(TArray<FString>& OutContent, TArray<FString>& OutWorlds)
	{
		OutContent.Reset();
		OutWorlds.Reset();
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			UPackage* Package = *It;
			if (!Package || !Package->IsDirty() || Package->HasAnyPackageFlags(PKG_PlayInEditor))
			{
				continue;
			}
			const FString Name = Package->GetName();
			if (Name.StartsWith(TEXT("/Engine")) || Name.StartsWith(TEXT("/Script")))
			{
				continue;
			}
			if (IsMapPackageName(Name))
			{
				OutWorlds.AddUnique(Name);
			}
			else
			{
				OutContent.AddUnique(Name);
			}
		}
	}

	TArray<FString> AdminWorldColorAllowedPackages()
	{
		TArray<FString> Allowed;
		Allowed.Add(AdminPackage);
		Allowed.Add(AdminWorldColorLightBpPackage);
		Allowed.Add(AdminWorldColorAudioBpPackage);
		for (const FAdminWorldColorMatSpec& Spec : AdminWorldColorMats)
		{
			Allowed.Add(AdminWorldColorMatPackage(Spec.Name));
		}
		return Allowed;
	}

	FString AdminWorldColorUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		AdminWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = AdminWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, AdminPackage) && !WorldsBefore.Contains(Name))
			{
				return FString::Printf(TEXT("Unexpected dirty world package: %s"), *Name);
			}
		}
		for (const FString& Name : Content)
		{
			if (!Allowed.Contains(Name) && !ContentBefore.Contains(Name))
			{
				return FString::Printf(TEXT("Unexpected dirty content package: %s"), *Name);
			}
		}
		return FString();
	}

	void AdminWorldColorClearUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		const TArray<FString> Allowed = AdminWorldColorAllowedPackages();
		for (TObjectIterator<UPackage> It; It; ++It)
		{
			UPackage* Package = *It;
			if (!Package || !Package->IsDirty())
			{
				continue;
			}
			const FString Name = Package->GetName();
			if (Allowed.Contains(Name) || ContentBefore.Contains(Name) || WorldsBefore.Contains(Name))
			{
				continue;
			}
			Package->SetDirtyFlag(false);
		}
	}

	FString AdminWorldColorGuardProtected(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		for (const TCHAR* Label : { ReceptionLabel, SecurityTerminalLabel, TEXT("Admin_FacilityHologram") })
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			if (Matches.Num() == 0)
			{
				continue;
			}
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d"), Label, Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), AdminPackage))
			{
				return FString::Printf(TEXT("%s is not on Admin."), Label);
			}
		}
		for (const TCHAR* Label : { TEXT("Terminal_ControlSpine"), TEXT("Checkpoint_BasinRim") })
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			for (AActor* Actor : Matches)
			{
				if (Actor && PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
				{
					return FString::Printf(TEXT("%s must not live on Admin for this pass."), Label);
				}
			}
		}
		return FString();
	}

	int32 AdminWorldColorCountProps(UWorld* World)
	{
		int32 Count = 0;
		for (const FAdminWorldColorPropSpec& Spec : AdminWorldColorProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && PackagesEqual(ActorOwningPackage(Matches[0]), AdminPackage))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 AdminWorldColorPaintMeshes(UWorld* World, int32& OutCeilingPainted, int32& OutGrayRemaining)
	{
		OutCeilingPainted = 0;
		OutGrayRemaining = 0;
		int32 Painted = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!AdminWorldColorIsPaintableMesh(MeshActor))
			{
				continue;
			}
			UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
			const EAdminWorldColorZone Zone = AdminWorldColorClassifyActor(MeshActor);
			const FAdminWorldColorMatSpec* MatSpec = AdminWorldColorFindMat(Zone);
			UMaterial* Material = MatSpec ? AdminWorldColorLoadMat(MatSpec->Name) : nullptr;
			if (!Comp || !Material)
			{
				continue;
			}
			const int32 SlotCount = FMath::Max(Comp->GetNumMaterials(), 1);
			bool bChanged = false;
			for (int32 Slot = 0; Slot < SlotCount; ++Slot)
			{
				UMaterialInterface* Current = Comp->GetMaterial(Slot);
				if (Current == Material)
				{
					continue;
				}
				const FLinearColor CurrentColor = AdminWorldColorReadMaterialColor(Current);
				if (Current && !AdminWorldColorIsGray(CurrentColor) && Current->GetPathName().Contains(TEXT("/Game/Materials/Admin/")))
				{
					continue;
				}
				Comp->SetMaterial(Slot, Material);
				bChanged = true;
			}
			if (bChanged)
			{
				MeshActor->MarkPackageDirty();
				++Painted;
				const FString Label = ActorLabel(MeshActor).ToLower();
				if (Label.Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > 250.f)
				{
					++OutCeilingPainted;
				}
			}
			const FLinearColor After = AdminWorldColorReadMaterialColor(Comp->GetMaterial(0));
			if (AdminWorldColorIsGray(After))
			{
				++OutGrayRemaining;
			}
		}
		return Painted;
	}

	FString AdminWorldColorApplyProp(AActor* Actor, const FAdminWorldColorPropSpec& Spec)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Spec.bPlane ? AdminWorldColorPlanePath : AdminWorldColorCubePath);
		if (!Mesh)
		{
			return TEXT("Failed to load basic shape mesh.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		const FAdminWorldColorMatSpec* MatSpec = AdminWorldColorFindMat(Spec.Zone);
		UMaterial* Material = MatSpec ? AdminWorldColorLoadMat(MatSpec->Name) : nullptr;
		if (Material)
		{
			MeshActor->GetStaticMeshComponent()->SetMaterial(0, Material);
		}
		Actor->SetActorEnableCollision(false);
		TArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive)
			{
				Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
		if (Spec.bText)
		{
			UTextRenderComponent* Text = Actor->FindComponentByClass<UTextRenderComponent>();
			if (!Text)
			{
				Text = NewObject<UTextRenderComponent>(Actor, TEXT("WorldColorText"));
				Text->SetupAttachment(MeshActor->GetStaticMeshComponent());
				Text->RegisterComponent();
				Actor->AddInstanceComponent(Text);
			}
			Text->SetText(FText::FromString(Spec.Text));
			Text->SetWorldSize(26.f);
			Text->SetRelativeLocation(FVector(0.f, -6.f, 0.f));
			Text->SetTextRenderColor(FColor(235, 240, 245));
			Text->SetHorizontalAlignment(EHTA_Center);
			Text->SetVerticalAlignment(EVRTA_TextCenter);
		}
		return FString();
	}

	TSharedRef<FJsonObject> CmdInspectAdminWorldColor(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("admin_package"), AdminPackage);
		Data->SetBoolField(TEXT("admin_loaded"), World && FindLoadedLevelByPackage(World, AdminPackage) != nullptr);
		int32 RecastNeedsRebuild = 0;
		int32 RecastCount = 0;
		if (World)
		{
			for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
			{
				++RecastCount;
				if (It->NeedsRebuild())
				{
					++RecastNeedsRebuild;
				}
			}
		}
		Data->SetNumberField(TEXT("recast_count"), RecastCount);
		Data->SetNumberField(TEXT("recast_needs_rebuild"), RecastNeedsRebuild);
		Data->SetBoolField(TEXT("navmesh_needs_rebuild"), RecastNeedsRebuild > 0);
		TArray<TSharedPtr<FJsonValue>> Mats;
		int32 MatExact = 0;
		for (const FAdminWorldColorMatSpec& Spec : AdminWorldColorMats)
		{
			UMaterial* Material = AdminWorldColorLoadMat(Spec.Name);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Spec.Name);
			Row->SetBoolField(TEXT("exists"), Material != nullptr);
			const bool bExact = AdminWorldColorMaterialExact(Material, Spec);
			Row->SetBoolField(TEXT("exact"), bExact);
			if (bExact)
			{
				++MatExact;
			}
			Mats.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("materials"), Mats);
		Data->SetNumberField(TEXT("materials_exact"), MatExact);
		Data->SetBoolField(TEXT("light_bp"), AdminWorldColorLoadLightBp() != nullptr);
		Data->SetBoolField(TEXT("audio_bp"), AdminWorldColorLoadAudioBp() != nullptr);
		Data->SetNumberField(TEXT("prop_count"), World ? AdminWorldColorCountProps(World) : 0);
		Data->SetNumberField(TEXT("prop_expected"), UE_ARRAY_COUNT(AdminWorldColorProps));
		TArray<AActor*> LegacyLights = World ? FindByExactLabel(World, LegacyLightControllerLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("legacy_light_controller_count"), LegacyLights.Num());
		if (LegacyLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("legacy_light_controller_location"), Vec(LegacyLights[0]->GetActorLocation()));
		}
		TArray<AActor*> WorldColorLights = World ? FindByExactLabel(World, AdminWorldColorLightLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("world_color_light_controller_count"), WorldColorLights.Num());
		if (WorldColorLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("world_color_light_controller_location"), Vec(WorldColorLights[0]->GetActorLocation()));
		}
		int32 GrayCeiling = 0;
		int32 PaintedCeiling = 0;
		int32 Paintable = 0;
		TArray<TSharedPtr<FJsonValue>> Samples;
		if (World)
		{
			for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
			{
				AStaticMeshActor* MeshActor = *It;
				if (!AdminWorldColorIsPaintableMesh(MeshActor))
				{
					continue;
				}
				++Paintable;
				UMaterialInterface* Mat = MeshActor->GetStaticMeshComponent()->GetMaterial(0);
				const FLinearColor Color = AdminWorldColorReadMaterialColor(Mat);
				const bool bGray = AdminWorldColorIsGray(Color);
				const FString Label = ActorLabel(MeshActor);
				const bool bCeiling = Label.ToLower().Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > 250.f;
				if (bCeiling && bGray)
				{
					++GrayCeiling;
				}
				if (bCeiling && Mat && Mat->GetPathName().Contains(TEXT("/Game/Materials/Admin/")))
				{
					++PaintedCeiling;
				}
				if (Samples.Num() < 24)
				{
					TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
					Row->SetStringField(TEXT("label"), Label);
					Row->SetBoolField(TEXT("gray"), bGray);
					Row->SetBoolField(TEXT("ceiling"), bCeiling);
					Row->SetStringField(TEXT("material"), Mat ? Mat->GetPathName() : FString());
					Row->SetArrayField(TEXT("location"), Vec(MeshActor->GetActorLocation()));
					Samples.Add(MakeShared<FJsonValueObject>(Row));
				}
			}
		}
		Data->SetNumberField(TEXT("paintable_meshes"), Paintable);
		Data->SetNumberField(TEXT("gray_ceiling_samples"), GrayCeiling);
		Data->SetNumberField(TEXT("painted_ceiling_samples"), PaintedCeiling);
		Data->SetArrayField(TEXT("mesh_samples"), Samples);
		USoundWave* Alarm = LoadObject<USoundWave>(nullptr, AdminWorldColorAlarmPath);
		Data->SetBoolField(TEXT("alarm_loaded"), Alarm != nullptr);
		Data->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
		int32 S22Zones = 0;
		if (World)
		{
			for (const TCHAR* Label : {
				TEXT("Admin_AudioZone_Public"), TEXT("Admin_AudioZone_Secure"),
				TEXT("Admin_AudioZone_Executive"), TEXT("Admin_AudioZone_Service") })
			{
				if (FindByExactLabel(World, Label).Num() == 1)
				{
					++S22Zones;
				}
			}
		}
		Data->SetNumberField(TEXT("s22_audio_zones"), S22Zones);
		Data->SetStringField(TEXT("note"),
			TEXT("World-color light controller uses Admin hub (2280,0,280). Brief (0,0,-2310) is below Admin floor."));
		return Ok(Data);
	}

	FString PreflightCreateAdminWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(AdminWorldColorSpec))
		{
			return TEXT("spec must be admin_world_color_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_admin_world_color does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded.");
		}
		const FString GuardError = AdminWorldColorGuardProtected(World);
		if (!GuardError.IsEmpty())
		{
			return GuardError;
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		AdminWorldColorCollectDirty(Content, Worlds);
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, AdminPackage))
			{
				return FString::Printf(TEXT("Refusing create while non-Admin world is dirty: %s"), *Name);
			}
		}
		Before->SetNumberField(TEXT("prop_count"), AdminWorldColorCountProps(World));
		Before->SetNumberField(TEXT("materials_exact"), 0);
		Proposed->SetStringField(TEXT("result"), TEXT("Create Admin world-color materials, Blueprints, props, and paint. Does not save."));
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetArrayField(TEXT("light_location"), Vec(AdminWorldColorLightLocation));
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateAdminWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_admin_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateAdminWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		UWorld* World = GetEditorWorld();
		ULevel* AdminLevel = World ? FindLoadedLevelByPackage(World, AdminPackage) : nullptr;
		if (!World || !AdminLevel)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("not_found"), TEXT("Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		AdminWorldColorCollectDirty(ContentBefore, WorldsBefore);

		FString EnsureError;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateAdminWorldColor", "Create Admin World Color"));
			if (!AdminWorldColorEnsureMaterials(EnsureError) || !AdminWorldColorEnsureBlueprints(EnsureError))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), EnsureError, MakeShared<FBridgeChange>(Change));
			}

			UBlueprint* LightBp = AdminWorldColorLoadLightBp();
			if (FindByExactLabel(World, AdminWorldColorLightLabel).Num() == 0 && LightBp && LightBp->GeneratedClass)
			{
				FActorSpawnParameters Params;
				Params.OverrideLevel = AdminLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Spawned = World->SpawnActor<AActor>(
					LightBp->GeneratedClass,
					AdminWorldColorLightLocation,
					AdminWorldColorLightRotation,
					Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), TEXT("Failed to spawn Admin_WorldColor_LightController."), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(AdminWorldColorLightScale);
#if WITH_EDITOR
				Spawned->SetActorLabel(AdminWorldColorLightLabel);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Admin_WorldColor")));
				Spawned->Tags.AddUnique(FName(TEXT("Admin_WorldColor_Light")));
			}

			for (const FAdminWorldColorPropSpec& Spec : AdminWorldColorProps)
			{
				if (FindByExactLabel(World, Spec.Label).Num() != 0)
				{
					continue;
				}
				FActorSpawnParameters Params;
				Params.OverrideLevel = AdminLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(Spec.Location, Spec.Rotation, Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("Failed to spawn %s."), Spec.Label), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(Spec.Scale);
#if WITH_EDITOR
				Spawned->SetActorLabel(Spec.Label);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Admin_WorldColor")));
				const FString PropError = AdminWorldColorApplyProp(Spawned, Spec);
				if (!PropError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("configure_failed"), PropError, MakeShared<FBridgeChange>(Change));
				}
			}

			int32 CeilingPainted = 0;
			int32 GrayRemaining = 0;
			const int32 Painted = AdminWorldColorPaintMeshes(World, CeilingPainted, GrayRemaining);
			AdminWorldColorClearUnexpectedDirty(ContentBefore, WorldsBefore);

			const FString DirtyError = AdminWorldColorUnexpectedDirty(ContentBefore, WorldsBefore);
			if (!DirtyError.IsEmpty())
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("dirty_failed"), DirtyError, MakeShared<FBridgeChange>(Change));
			}

			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("created"), true);
			Change.After->SetBoolField(TEXT("saves"), false);
			Change.After->SetNumberField(TEXT("materials"), UE_ARRAY_COUNT(AdminWorldColorMats));
			Change.After->SetNumberField(TEXT("blueprints"), 2);
			Change.After->SetNumberField(TEXT("props"), AdminWorldColorCountProps(World));
			Change.After->SetNumberField(TEXT("meshes_painted"), Painted);
			Change.After->SetNumberField(TEXT("ceilings_painted"), CeilingPainted);
			Change.After->SetNumberField(TEXT("gray_remaining"), GrayRemaining);
			Change.After->SetArrayField(TEXT("light_location"), Vec(AdminWorldColorLightLocation));
			Change.After->SetStringField(TEXT("light_label"), AdminWorldColorLightLabel);
			Change.After->SetBoolField(TEXT("legacy_light_controller_kept"), FindByExactLabel(World, LegacyLightControllerLabel).Num() == 1);
			int32 S22Count = 0;
			for (const TCHAR* Label : {
				TEXT("Admin_AudioZone_Public"), TEXT("Admin_AudioZone_Secure"),
				TEXT("Admin_AudioZone_Executive"), TEXT("Admin_AudioZone_Service") })
			{
				if (FindByExactLabel(World, Label).Num() == 1)
				{
					++S22Count;
				}
			}
			Change.After->SetNumberField(TEXT("s22_audio_zones"), S22Count);
			USoundWave* Alarm = LoadObject<USoundWave>(nullptr, AdminWorldColorAlarmPath);
			Change.After->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
			Change.After->SetStringField(TEXT("note"),
				TEXT("Created /Game/Materials/Admin (5) + /Game/Blueprints/Admin (2). S20/S22 controllers kept. Props>=10. No save."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
	}

	FString PreflightSaveAdminWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(AdminWorldColorSaveSpec))
		{
			return TEXT("spec must be admin_world_color_save_v1.");
		}
		if (GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save_all is forbidden.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this save.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin must be loaded.");
		}
		for (const FAdminWorldColorMatSpec& Spec : AdminWorldColorMats)
		{
			if (!AdminWorldColorMaterialExact(AdminWorldColorLoadMat(Spec.Name), Spec))
			{
				return FString::Printf(TEXT("%s is missing or gray. Refusing save."), Spec.Name);
			}
		}
		if (!AdminWorldColorLoadLightBp() || !AdminWorldColorLoadAudioBp())
		{
			return TEXT("World-color Blueprints are missing. Refusing save.");
		}
		if (AdminWorldColorCountProps(World) < 10)
		{
			return TEXT("Fewer than 10 Admin_WC_ props. Refusing save.");
		}
		if (FindByExactLabel(World, AdminWorldColorLightLabel).Num() != 1)
		{
			return TEXT("Admin_WorldColor_LightController count must be 1.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		AdminWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = AdminWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, AdminPackage))
			{
				return FString::Printf(TEXT("Refusing save while non-Admin world is dirty: %s"), *Name);
			}
		}
		for (const FString& Name : Content)
		{
			if (!Allowed.Contains(Name))
			{
				return FString::Printf(TEXT("Refusing save while unexpected package is dirty: %s"), *Name);
			}
		}
		TArray<TSharedPtr<FJsonValue>> Packages;
		for (const FString& Name : Allowed)
		{
			Packages.Add(MakeShared<FJsonValueString>(Name));
		}
		Before->SetNumberField(TEXT("dirty_content"), Content.Num());
		Before->SetNumberField(TEXT("dirty_worlds"), Worlds.Num());
		Proposed->SetArrayField(TEXT("packages"), Packages);
		Proposed->SetBoolField(TEXT("saves_admin_map"), true);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteSaveAdminWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_admin_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveAdminWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		TArray<UPackage*> Packages;
		const TArray<TSharedPtr<FJsonValue>>* ProposedPackages = nullptr;
		Proposed->TryGetArrayField(TEXT("packages"), ProposedPackages);
		if (!ProposedPackages)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("write_failed"), TEXT("No packages to save."), MakeShared<FBridgeChange>(Change));
		}
		for (const TSharedPtr<FJsonValue>& Value : *ProposedPackages)
		{
			const FString Name = Value->AsString();
			UPackage* Package = FindPackage(nullptr, *Name);
			if (!Package)
			{
				Package = LoadPackage(nullptr, *Name, LOAD_None);
			}
			if (!Package)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("not_found"), FString::Printf(TEXT("Package missing: %s"), *Name), MakeShared<FBridgeChange>(Change));
			}
			Packages.AddUnique(Package);
		}
		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
		if (!bSaved)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("save_failed"), TEXT("SavePackages returned false."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = true;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("saved"), true);
		Change.After->SetNumberField(TEXT("packages"), Packages.Num());
		TArray<TSharedPtr<FJsonValue>> SavedNames;
		for (UPackage* Package : Packages)
		{
			SavedNames.Add(MakeShared<FJsonValueString>(Package->GetName()));
		}
		Change.After->SetArrayField(TEXT("saved_packages"), SavedNames);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
