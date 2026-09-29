// Beat 30 Reactor world color: five zone materials, two Blueprints under
// /Game/Blueprints/Reactor/, props, and material paint on Reactor blockout meshes.
// Does not move Reactor encounter actors or change power. Does not save;
// save_reactor_world_color persists Reactor + the seven packages only.

	const TCHAR* ReactorWorldColorSpec = TEXT("reactor_world_color_v1");
	const TCHAR* ReactorWorldColorAction = TEXT("create_reactor_world_color");
	const TCHAR* ReactorWorldColorSaveSpec = TEXT("reactor_world_color_save_v1");
	const TCHAR* ReactorWorldColorSaveAction = TEXT("save_reactor_world_color");
	const TCHAR* ReactorWorldColorFolder = TEXT("/Game/Materials/Reactor");
	const TCHAR* ReactorWorldColorBpFolder = TEXT("/Game/Blueprints/Reactor");
	const TCHAR* ReactorWorldColorLightBpPackage = TEXT("/Game/Blueprints/Reactor/BP_ReactorLightController");
	const TCHAR* ReactorWorldColorLightBpPath = TEXT("/Game/Blueprints/Reactor/BP_ReactorLightController.BP_ReactorLightController");
	const TCHAR* ReactorWorldColorAudioBpPackage = TEXT("/Game/Blueprints/Reactor/BP_ReactorAudioZone");
	const TCHAR* ReactorWorldColorAudioBpPath = TEXT("/Game/Blueprints/Reactor/BP_ReactorAudioZone.BP_ReactorAudioZone");
	const TCHAR* ReactorWorldColorLightLabel = TEXT("Reactor_WorldColor_LightController");
	// Reactor floor is Z≈-4710. Place the world-color light controller near BP_NodeZeroCore (-200,0,-4710).
	const FVector ReactorWorldColorLightLocation(-200.f, 0.f, -4710.f);
	const FRotator ReactorWorldColorLightRotation(0.f, 0.f, 0.f);
	const FVector ReactorWorldColorLightScale(1.f, 1.f, 1.f);
	const TCHAR* ReactorWorldColorCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* ReactorWorldColorPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* ReactorWorldColorAlarmPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* ReactorWorldColorAmbienceNative = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone");
	const TCHAR* LegacyReactorLightControllerLabel = TEXT("__NO_LEGACY_REACTOR_LIGHT__");

	enum class EReactorWorldColorZone : int32
	{
		Public = 0,
		Restricted,
		Executive,
		Operations,
		Service,
		Count
	};

	struct FReactorWorldColorMatSpec
	{
		const TCHAR* Name;
		EReactorWorldColorZone Zone;
		FLinearColor Color;
		float Roughness;
		float Metallic;
		float Emissive;
	};

	const FReactorWorldColorMatSpec ReactorWorldColorMats[] = {
		{ TEXT("M_Reactor_Public_CoreGlow"), EReactorWorldColorZone::Public,
			FLinearColor(1.00f, 0.32f, 0.04f), 0.35f, 0.20f, 2.40f },
		{ TEXT("M_Reactor_Restricted_Hazard"), EReactorWorldColorZone::Restricted,
			FLinearColor(0.95f, 0.72f, 0.06f), 0.45f, 0.15f, 0.55f },
		{ TEXT("M_Reactor_Executive_Observation"), EReactorWorldColorZone::Executive,
			FLinearColor(0.68f, 0.76f, 0.84f), 0.12f, 0.22f, 0.10f },
		{ TEXT("M_Reactor_Operations_ReactorControl"), EReactorWorldColorZone::Operations,
			FLinearColor(0.12f, 0.42f, 0.48f), 0.22f, 0.18f, 1.20f },
		{ TEXT("M_Reactor_Service_Industrial"), EReactorWorldColorZone::Service,
			FLinearColor(0.48f, 0.40f, 0.28f), 0.90f, 0.10f, 0.0f },
	};

	struct FReactorWorldColorPropSpec
	{
		const TCHAR* Label;
		EReactorWorldColorZone Zone;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bPlane;
		bool bText;
		const TCHAR* Text;
	};

	const FReactorWorldColorPropSpec ReactorWorldColorProps[] = {
		{ TEXT("Reactor_WC_Sign_Core"), EReactorWorldColorZone::Public,
			FVector(-200.f, 200.f, -4580.f), FRotator(0.f, 0.f, 0.f), FVector(1.8f, 0.08f, 0.5f), false, true,
			TEXT("CORE REACTOR — PRIMARY INCUBATOR") },
		{ TEXT("Reactor_WC_Sign_Hazard"), EReactorWorldColorZone::Restricted,
			FVector(-900.f, 200.f, -4580.f), FRotator(0.f, -90.f, 0.f), FVector(1.5f, 0.08f, 0.45f), false, true,
			TEXT("HAZARD — RESTRICTED BASIN") },
		{ TEXT("Reactor_WC_Core_1"), EReactorWorldColorZone::Public,
			FVector(-50.f, -150.f, -4680.f), FRotator(0.f, 0.f, 0.f), FVector(0.9f, 0.9f, 1.4f), false, false, nullptr },
		{ TEXT("Reactor_WC_Core_2"), EReactorWorldColorZone::Public,
			FVector(-350.f, 150.f, -4680.f), FRotator(0.f, 25.f, 0.f), FVector(0.7f, 0.7f, 1.2f), false, false, nullptr },
		{ TEXT("Reactor_WC_Basin"), EReactorWorldColorZone::Public,
			FVector(50.f, 0.f, -4745.f), FRotator(0.f, 0.f, 0.f), FVector(3.5f, 3.5f, 0.15f), true, false, nullptr },
		{ TEXT("Reactor_WC_HazardDecal_1"), EReactorWorldColorZone::Restricted,
			FVector(-1100.f, 0.f, -4745.f), FRotator(0.f, 0.f, 0.f), FVector(2.0f, 2.0f, 1.f), true, false, nullptr },
		{ TEXT("Reactor_WC_HazardDecal_2"), EReactorWorldColorZone::Restricted,
			FVector(-500.f, 0.f, -4745.f), FRotator(0.f, 45.f, 0.f), FVector(1.8f, 1.8f, 1.f), true, false, nullptr },
		{ TEXT("Reactor_WC_WarningSign"), EReactorWorldColorZone::Restricted,
			FVector(-800.f, 600.f, -4600.f), FRotator(0.f, 180.f, 0.f), FVector(1.2f, 0.08f, 0.6f), false, true,
			TEXT("WARNING — TRANSFORMED PERSONNEL") },
		{ TEXT("Reactor_WC_Pipe_1"), EReactorWorldColorZone::Service,
			FVector(-600.f, -300.f, -4620.f), FRotator(0.f, 0.f, 0.f), FVector(3.0f, 0.18f, 0.18f), false, false, nullptr },
		{ TEXT("Reactor_WC_Pipe_2"), EReactorWorldColorZone::Service,
			FVector(100.f, 300.f, -4620.f), FRotator(0.f, 90.f, 0.f), FVector(2.5f, 0.18f, 0.18f), false, false, nullptr },
		{ TEXT("Reactor_WC_Observation_Glass"), EReactorWorldColorZone::Executive,
			FVector(-200.f, 550.f, -4600.f), FRotator(0.f, 0.f, 0.f), FVector(0.08f, 2.2f, 1.5f), false, false, nullptr },
		{ TEXT("Reactor_WC_ControlPanel"), EReactorWorldColorZone::Operations,
			FVector(150.f, 200.f, -4650.f), FRotator(0.f, -30.f, 0.f), FVector(1.3f, 0.08f, 0.9f), false, false, nullptr },
		{ TEXT("Reactor_WC_Brand_Reactor"), EReactorWorldColorZone::Public,
			FVector(-200.f, -350.f, -4550.f), FRotator(0.f, 0.f, 0.f), FVector(2.0f, 0.05f, 0.45f), false, true,
			TEXT("EPITOPE — CORE REACTOR / INCUBATOR") },
	};

	FString ReactorWorldColorMatPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), ReactorWorldColorFolder, Name, Name);
	}

	FString ReactorWorldColorMatPackage(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s"), ReactorWorldColorFolder, Name);
	}

	const FReactorWorldColorMatSpec* ReactorWorldColorFindMat(EReactorWorldColorZone Zone)
	{
		for (const FReactorWorldColorMatSpec& Spec : ReactorWorldColorMats)
		{
			if (Spec.Zone == Zone)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool ReactorWorldColorColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.05f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.05f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.05f);
	}

	bool ReactorWorldColorIsGray(const FLinearColor& Color)
	{
		const float Max = FMath::Max3(Color.R, Color.G, Color.B);
		const float Min = FMath::Min3(Color.R, Color.G, Color.B);
		return (Max - Min) < 0.08f && Max > 0.15f && Max < 0.85f;
	}

	FLinearColor ReactorWorldColorReadMaterialColor(const UMaterialInterface* Material)
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

	void ReactorWorldColorWireMaterial(UMaterial* Material, const FReactorWorldColorMatSpec& Spec)
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
		else if (UMaterialExpressionVectorParameter* ColorExpr =
			Cast<UMaterialExpressionVectorParameter>(EditorOnly->BaseColor.Expression))
		{
			ColorExpr->DefaultValue = Spec.Color;
			if (UMaterialExpressionScalarParameter* Roughness =
				Cast<UMaterialExpressionScalarParameter>(EditorOnly->Roughness.Expression))
			{
				Roughness->DefaultValue = Spec.Roughness;
			}
			if (UMaterialExpressionScalarParameter* Metallic =
				Cast<UMaterialExpressionScalarParameter>(EditorOnly->Metallic.Expression))
			{
				Metallic->DefaultValue = Spec.Metallic;
			}
		}
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	bool ReactorWorldColorMaterialExact(const UMaterial* Material, const FReactorWorldColorMatSpec& Spec)
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
		if (!ColorExpr || !ReactorWorldColorColorNear(ColorExpr->DefaultValue, Spec.Color))
		{
			return false;
		}
		return !ReactorWorldColorIsGray(ColorExpr->DefaultValue);
	}

	UMaterial* ReactorWorldColorLoadMat(const TCHAR* Name)
	{
		return LoadObject<UMaterial>(nullptr, *ReactorWorldColorMatPath(Name));
	}

	bool ReactorWorldColorEnsureMaterials(FString& OutError)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		for (const FReactorWorldColorMatSpec& Spec : ReactorWorldColorMats)
		{
			UMaterial* Material = ReactorWorldColorLoadMat(Spec.Name);
			if (!Material)
			{
				UPackage* Package = CreatePackage(*ReactorWorldColorMatPackage(Spec.Name));
				Material = Cast<UMaterial>(Factory->FactoryCreateNew(
					UMaterial::StaticClass(), Package, Spec.Name, RF_Public | RF_Standalone, nullptr, GWarn));
				if (!Material)
				{
					OutError = FString::Printf(TEXT("Failed to create %s."), Spec.Name);
					return false;
				}
				FAssetRegistryModule::AssetCreated(Material);
			}
			ReactorWorldColorWireMaterial(Material, Spec);
			if (!ReactorWorldColorMaterialExact(Material, Spec))
			{
				OutError = FString::Printf(TEXT("%s is not a colored Reactor material."), Spec.Name);
				return false;
			}
		}
		return true;
	}

	UBlueprint* ReactorWorldColorLoadLightBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ReactorWorldColorLightBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, ReactorWorldColorLightBpPackage);
		}
		return Blueprint;
	}

	UBlueprint* ReactorWorldColorLoadAudioBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ReactorWorldColorAudioBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, ReactorWorldColorAudioBpPackage);
		}
		return Blueprint;
	}

	bool ReactorWorldColorEnsureBlueprints(FString& OutError)
	{
		if (!ReactorWorldColorLoadLightBp())
		{
			UPackage* Package = CreatePackage(ReactorWorldColorLightBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				Package,
				TEXT("BP_ReactorLightController"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created || !Created->SimpleConstructionScript)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Reactor/BP_ReactorLightController.");
				return false;
			}
			USCS_Node* LightNode = Created->SimpleConstructionScript->CreateNode(
				UPointLightComponent::StaticClass(), TEXT("PublicZoneLight"));
			Created->SimpleConstructionScript->AddNode(LightNode);
			if (UPointLightComponent* Template = Cast<UPointLightComponent>(LightNode->ComponentTemplate))
			{
				Template->Intensity = 5200.f;
				Template->SetTemperature(2200.f);
				Template->bUseTemperature = true;
				Template->SetLightColor(FLinearColor(1.0f, 0.40f, 0.08f), true);
				Template->AttenuationRadius = 1400.f;
			}
			struct FReactorZoneLightSpec
			{
				const TCHAR* Name;
				FLinearColor Color;
				float Intensity;
				float Temp;
			};
			const FReactorZoneLightSpec ExtraLights[] = {
				{ TEXT("RestrictedZoneLight"), FLinearColor(1.0f, 0.85f, 0.10f), 2400.f, 4500.f },
				{ TEXT("ExecutiveZoneLight"), FLinearColor(1.0f, 0.90f, 0.78f), 3400.f, 4000.f },
				{ TEXT("OperationsZoneLight"), FLinearColor(0.40f, 0.80f, 0.90f), 5600.f, 6500.f },
				{ TEXT("ServiceZoneLight"), FLinearColor(0.50f, 0.48f, 0.42f), 1600.f, 3500.f },
			};
			for (const FReactorZoneLightSpec& Spec : ExtraLights)
			{
				USCS_Node* Node = Created->SimpleConstructionScript->CreateNode(
					UPointLightComponent::StaticClass(), Spec.Name);
				Created->SimpleConstructionScript->AddNode(Node);
				if (UPointLightComponent* Template = Cast<UPointLightComponent>(Node->ComponentTemplate))
				{
					Template->Intensity = Spec.Intensity;
					Template->SetTemperature(Spec.Temp);
					Template->bUseTemperature = true;
					Template->SetLightColor(Spec.Color, true);
					Template->AttenuationRadius = 900.f;
				}
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		if (!ReactorWorldColorLoadAudioBp())
		{
			UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, ReactorWorldColorAmbienceNative);
			if (!Native)
			{
				OutError = TEXT("ProjectOrganoidAmbienceZone is not loaded.");
				return false;
			}
			UPackage* Package = CreatePackage(ReactorWorldColorAudioBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				Native,
				Package,
				TEXT("BP_ReactorAudioZone"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Reactor/BP_ReactorAudioZone.");
				return false;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		UBlueprint* LightBp = ReactorWorldColorLoadLightBp();
		UBlueprint* AudioBp = ReactorWorldColorLoadAudioBp();
		if (!LightBp || !LightBp->GeneratedClass || LightBp->ParentClass != AActor::StaticClass())
		{
			OutError = TEXT("BP_ReactorLightController under /Game/Blueprints/Reactor/ is invalid.");
			return false;
		}
		UClass* AmbienceNative = StaticLoadClass(UObject::StaticClass(), nullptr, ReactorWorldColorAmbienceNative);
		if (!AudioBp || !AudioBp->GeneratedClass || !AmbienceNative || !AudioBp->GeneratedClass->IsChildOf(AmbienceNative))
		{
			OutError = TEXT("BP_ReactorAudioZone under /Game/Blueprints/Reactor/ must parent AmbienceZone.");
			return false;
		}
		return true;
	}

	EReactorWorldColorZone ReactorWorldColorClassifyLocation(const FVector& Location)
	{
		const FVector Core(-200.f, 0.f, -4710.f);
		const FVector Delta = Location - Core;
		// West toward Pursuer / FirstCombat triggers → Restricted hazard
		if (Location.X < -700.f)
		{
			return EReactorWorldColorZone::Restricted;
		}
		// Near NodeZero core → Public core glow
		if (FMath::Abs(Delta.X) < 300.f && FMath::Abs(Delta.Y) < 300.f)
		{
			return EReactorWorldColorZone::Public;
		}
		if (Delta.Y > 250.f)
		{
			return EReactorWorldColorZone::Executive;
		}
		if (Delta.X > 200.f)
		{
			return EReactorWorldColorZone::Operations;
		}
		if (Delta.Y < -200.f)
		{
			return EReactorWorldColorZone::Service;
		}
		return EReactorWorldColorZone::Public;
	}

	EReactorWorldColorZone ReactorWorldColorClassifyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return EReactorWorldColorZone::Public;
		}
		const FString Label = ActorLabel(const_cast<AActor*>(Actor));
		if (Label.Contains(TEXT("Executive"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Observation"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Glass"), ESearchCase::IgnoreCase))
		{
			return EReactorWorldColorZone::Executive;
		}
		if (Label.Contains(TEXT("Operations"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Control"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("ReactorControl"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Terminal_Control"), ESearchCase::IgnoreCase))
		{
			return EReactorWorldColorZone::Operations;
		}
		if (Label.Contains(TEXT("Service"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pipe"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Corridor"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Vent"), ESearchCase::IgnoreCase))
		{
			return EReactorWorldColorZone::Service;
		}
		if (Label.Contains(TEXT("Hazard"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Restricted"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Warning"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pursuer"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("FirstCombat"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Transformed"), ESearchCase::IgnoreCase))
		{
			return EReactorWorldColorZone::Restricted;
		}
		if (Label.Contains(TEXT("Core"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Basin"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("NodeZero"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Incubator"), ESearchCase::IgnoreCase))
		{
			return EReactorWorldColorZone::Public;
		}
		return ReactorWorldColorClassifyLocation(Actor->GetActorLocation());
	}

	bool ReactorWorldColorIsProtectedLabel(const FString& Label)
	{
		static const TCHAR* Protected[] = {
			TEXT("Terminal_ControlSpine"),
			TEXT("Terminal_SterlingFinal"),
			TEXT("Checkpoint_BasinRim"),
			TEXT("Checkpoint_InterfaceChamber"),
			TEXT("NavMeshBounds"),
			TEXT("RecastNavMesh"),
			TEXT("BackupPower"),
			TEXT("PowerPanel_"),
			TEXT("BP_Pursuer"),
			TEXT("BP_TransformedScientist"),
			TEXT("BP_NodeZeroCore"),
			TEXT("Reactor_PursuerTrigger"),
			TEXT("Reactor_FirstCombatTrigger"),
			TEXT("Reactor_WorldColor_LightController"),
			TEXT("ResearchStation_"),
			TEXT("DataPad_"),
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

	bool ReactorWorldColorIsPaintableMesh(const AActor* Actor)
	{
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !PackagesEqual(ActorOwningPackage(const_cast<AStaticMeshActor*>(MeshActor)), ReactorPackage))
		{
			return false;
		}
		const FString Label = ActorLabel(const_cast<AStaticMeshActor*>(MeshActor));
		if (ReactorWorldColorIsProtectedLabel(Label) || Label.StartsWith(TEXT("Reactor_WC_"), ESearchCase::CaseSensitive))
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
			|| Lower.Contains(TEXT("sm_"))
			|| Lower.Contains(TEXT("reactor"));
		const FVector Loc = MeshActor->GetActorLocation();
		const FVector Scale = MeshActor->GetActorScale3D();
		const bool bCeilingShape = Loc.Z > -4550.f && Scale.Z < 0.35f;
		const bool bFloorShape = Loc.Z < -4780.f && Scale.Z < 0.35f;
		UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
		const FLinearColor Slot0 = ReactorWorldColorReadMaterialColor(Comp ? Comp->GetMaterial(0) : nullptr);
		const bool bGraySlot = ReactorWorldColorIsGray(Slot0)
			|| (Comp && Comp->GetMaterial(0) && Comp->GetMaterial(0)->GetName().Contains(TEXT("WorldGridMaterial")));
		return bNameHit || bCeilingShape || bFloorShape || bGraySlot;
	}

	void ReactorWorldColorCollectDirty(TArray<FString>& OutContent, TArray<FString>& OutWorlds)
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

	TArray<FString> ReactorWorldColorAllowedPackages()
	{
		TArray<FString> Allowed;
		Allowed.Add(ReactorPackage);
		Allowed.Add(ReactorWorldColorLightBpPackage);
		Allowed.Add(ReactorWorldColorAudioBpPackage);
		for (const FReactorWorldColorMatSpec& Spec : ReactorWorldColorMats)
		{
			Allowed.Add(ReactorWorldColorMatPackage(Spec.Name));
		}
		return Allowed;
	}

	FString ReactorWorldColorUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		ReactorWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = ReactorWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ReactorPackage) && !WorldsBefore.Contains(Name))
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

	void ReactorWorldColorClearUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		const TArray<FString> Allowed = ReactorWorldColorAllowedPackages();
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

	FString ReactorWorldColorGuardProtected(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		struct FReactorActorGuard
		{
			const TCHAR* Label;
			FVector Location;
			bool bCheckLocation;
		};
		const FReactorActorGuard Guards[] = {
			{ TEXT("BP_Pursuer"), FVector(-1800.f, 0.f, -4710.f), true },
			{ TEXT("Reactor_PursuerTrigger"), FVector(-1100.f, 0.f, -4710.f), true },
			{ TEXT("BP_TransformedScientist"), FVector(-800.f, 800.f, -4710.f), true },
			{ TEXT("Reactor_FirstCombatTrigger"), FVector(-500.f, 0.f, -4710.f), true },
			{ TEXT("BP_NodeZeroCore"), FVector(-200.f, 0.f, -4710.f), true },
			{ TEXT("Terminal_SterlingFinal"), FVector(-200.f, 400.f, -4710.f), true },
			{ TEXT("Checkpoint_BasinRim"), FVector(25.f, 0.f, -4740.f), true },
			{ TEXT("Terminal_ControlSpine"), FVector(0.f, 0.f, 0.f), false },
		};
		for (const FReactorActorGuard& Guard : Guards)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Guard.Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1"), Guard.Label, Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), ReactorPackage))
			{
				return FString::Printf(TEXT("%s is not on Reactor."), Guard.Label);
			}
			if (Guard.bCheckLocation && !Matches[0]->GetActorLocation().Equals(Guard.Location, 1.f))
			{
				return FString::Printf(TEXT("%s moved. Abort."), Guard.Label);
			}
		}
		return FString();
	}

	int32 ReactorWorldColorCountProps(UWorld* World)
	{
		int32 Count = 0;
		for (const FReactorWorldColorPropSpec& Spec : ReactorWorldColorProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && PackagesEqual(ActorOwningPackage(Matches[0]), ReactorPackage))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 ReactorWorldColorPaintMeshes(UWorld* World, int32& OutCeilingPainted, int32& OutGrayRemaining)
	{
		OutCeilingPainted = 0;
		OutGrayRemaining = 0;
		int32 Painted = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!ReactorWorldColorIsPaintableMesh(MeshActor))
			{
				continue;
			}
			UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
			const EReactorWorldColorZone Zone = ReactorWorldColorClassifyActor(MeshActor);
			const FReactorWorldColorMatSpec* MatSpec = ReactorWorldColorFindMat(Zone);
			UMaterial* Material = MatSpec ? ReactorWorldColorLoadMat(MatSpec->Name) : nullptr;
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
				const FLinearColor CurrentColor = ReactorWorldColorReadMaterialColor(Current);
				if (Current && !ReactorWorldColorIsGray(CurrentColor) && Current->GetPathName().Contains(TEXT("/Game/Materials/Reactor/")))
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
				if (Label.Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -4550.f)
				{
					++OutCeilingPainted;
				}
			}
			const FLinearColor After = ReactorWorldColorReadMaterialColor(Comp->GetMaterial(0));
			if (ReactorWorldColorIsGray(After))
			{
				++OutGrayRemaining;
			}
		}
		return Painted;
	}

	FString ReactorWorldColorApplyProp(AActor* Actor, const FReactorWorldColorPropSpec& Spec)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Spec.bPlane ? ReactorWorldColorPlanePath : ReactorWorldColorCubePath);
		if (!Mesh)
		{
			return TEXT("Failed to load basic shape mesh.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		const FReactorWorldColorMatSpec* MatSpec = ReactorWorldColorFindMat(Spec.Zone);
		UMaterial* Material = MatSpec ? ReactorWorldColorLoadMat(MatSpec->Name) : nullptr;
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

	TSharedRef<FJsonObject> CmdInspectReactorWorldColor(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("reactor_package"), ReactorPackage);
		Data->SetBoolField(TEXT("reactor_loaded"), World && FindLoadedLevelByPackage(World, ReactorPackage) != nullptr);
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
		for (const FReactorWorldColorMatSpec& Spec : ReactorWorldColorMats)
		{
			UMaterial* Material = ReactorWorldColorLoadMat(Spec.Name);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Spec.Name);
			Row->SetBoolField(TEXT("exists"), Material != nullptr);
			const bool bExact = ReactorWorldColorMaterialExact(Material, Spec);
			Row->SetBoolField(TEXT("exact"), bExact);
			if (bExact)
			{
				++MatExact;
			}
			Mats.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("materials"), Mats);
		Data->SetNumberField(TEXT("materials_exact"), MatExact);
		Data->SetBoolField(TEXT("light_bp"), ReactorWorldColorLoadLightBp() != nullptr);
		Data->SetBoolField(TEXT("audio_bp"), ReactorWorldColorLoadAudioBp() != nullptr);
		Data->SetNumberField(TEXT("prop_count"), World ? ReactorWorldColorCountProps(World) : 0);
		Data->SetNumberField(TEXT("prop_expected"), UE_ARRAY_COUNT(ReactorWorldColorProps));
		TArray<AActor*> LegacyLights = World ? FindByExactLabel(World, LegacyReactorLightControllerLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("legacy_light_controller_count"), LegacyLights.Num());
		if (LegacyLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("legacy_light_controller_location"), Vec(LegacyLights[0]->GetActorLocation()));
		}
		TArray<AActor*> WorldColorLights = World ? FindByExactLabel(World, ReactorWorldColorLightLabel) : TArray<AActor*>();
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
				if (!ReactorWorldColorIsPaintableMesh(MeshActor))
				{
					continue;
				}
				++Paintable;
				UMaterialInterface* Mat = MeshActor->GetStaticMeshComponent()->GetMaterial(0);
				const FLinearColor Color = ReactorWorldColorReadMaterialColor(Mat);
				const bool bGray = ReactorWorldColorIsGray(Color);
				const FString Label = ActorLabel(MeshActor);
				const bool bCeiling = Label.ToLower().Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -3350.f;
				if (bCeiling && bGray)
				{
					++GrayCeiling;
				}
				if (bCeiling && Mat && Mat->GetPathName().Contains(TEXT("/Game/Materials/Reactor/")))
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
		USoundWave* Alarm = LoadObject<USoundWave>(nullptr, ReactorWorldColorAlarmPath);
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
			TEXT("World-color light controller near BP_NodeZeroCore (-200,0,-4710). Encounter actors unmoved."));
		return Ok(Data);
	}

	FString PreflightCreateReactorWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(ReactorWorldColorSpec))
		{
			return TEXT("spec must be reactor_world_color_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_reactor_world_color does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, ReactorPackage))
		{
			return TEXT("SL_Epitope_Reactor must be loaded.");
		}
		const FString GuardError = ReactorWorldColorGuardProtected(World);
		if (!GuardError.IsEmpty())
		{
			return GuardError;
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		ReactorWorldColorCollectDirty(Content, Worlds);
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ReactorPackage))
			{
				return FString::Printf(TEXT("Refusing create while non-Reactor world is dirty: %s"), *Name);
			}
		}
		Before->SetNumberField(TEXT("prop_count"), ReactorWorldColorCountProps(World));
		Before->SetNumberField(TEXT("materials_exact"), 0);
		Proposed->SetStringField(TEXT("result"), TEXT("Create Reactor world-color materials, Blueprints, props, and paint. Does not save."));
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetArrayField(TEXT("light_location"), Vec(ReactorWorldColorLightLocation));
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateReactorWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_reactor_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateReactorWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		UWorld* World = GetEditorWorld();
		ULevel* ReactorLevel = World ? FindLoadedLevelByPackage(World, ReactorPackage) : nullptr;
		if (!World || !ReactorLevel)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("not_found"), TEXT("Reactor level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		ReactorWorldColorCollectDirty(ContentBefore, WorldsBefore);

		FString EnsureError;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateReactorWorldColor", "Create Reactor World Color"));
			if (!ReactorWorldColorEnsureMaterials(EnsureError) || !ReactorWorldColorEnsureBlueprints(EnsureError))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), EnsureError, MakeShared<FBridgeChange>(Change));
			}

			UBlueprint* LightBp = ReactorWorldColorLoadLightBp();
			if (FindByExactLabel(World, ReactorWorldColorLightLabel).Num() == 0 && LightBp && LightBp->GeneratedClass)
			{
				FActorSpawnParameters Params;
				Params.OverrideLevel = ReactorLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Spawned = World->SpawnActor<AActor>(
					LightBp->GeneratedClass,
					ReactorWorldColorLightLocation,
					ReactorWorldColorLightRotation,
					Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), TEXT("Failed to spawn Reactor_WorldColor_LightController."), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(ReactorWorldColorLightScale);
#if WITH_EDITOR
				Spawned->SetActorLabel(ReactorWorldColorLightLabel);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Reactor_WorldColor")));
				Spawned->Tags.AddUnique(FName(TEXT("Reactor_WorldColor_Light")));
			}

			for (const FReactorWorldColorPropSpec& Spec : ReactorWorldColorProps)
			{
				if (FindByExactLabel(World, Spec.Label).Num() != 0)
				{
					continue;
				}
				FActorSpawnParameters Params;
				Params.OverrideLevel = ReactorLevel;
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
				Spawned->Tags.AddUnique(FName(TEXT("Reactor_WorldColor")));
				const FString PropError = ReactorWorldColorApplyProp(Spawned, Spec);
				if (!PropError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("configure_failed"), PropError, MakeShared<FBridgeChange>(Change));
				}
			}

			int32 CeilingPainted = 0;
			int32 GrayRemaining = 0;
			const int32 Painted = ReactorWorldColorPaintMeshes(World, CeilingPainted, GrayRemaining);
			ReactorWorldColorClearUnexpectedDirty(ContentBefore, WorldsBefore);

			const FString DirtyError = ReactorWorldColorUnexpectedDirty(ContentBefore, WorldsBefore);
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
			Change.After->SetNumberField(TEXT("materials"), UE_ARRAY_COUNT(ReactorWorldColorMats));
			Change.After->SetNumberField(TEXT("blueprints"), 2);
			Change.After->SetNumberField(TEXT("props"), ReactorWorldColorCountProps(World));
			Change.After->SetNumberField(TEXT("meshes_painted"), Painted);
			Change.After->SetNumberField(TEXT("ceilings_painted"), CeilingPainted);
			Change.After->SetNumberField(TEXT("gray_remaining"), GrayRemaining);
			Change.After->SetArrayField(TEXT("light_location"), Vec(ReactorWorldColorLightLocation));
			Change.After->SetStringField(TEXT("light_label"), ReactorWorldColorLightLabel);
			Change.After->SetBoolField(TEXT("legacy_light_controller_kept"), FindByExactLabel(World, LegacyReactorLightControllerLabel).Num() == 1);
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
			USoundWave* Alarm = LoadObject<USoundWave>(nullptr, ReactorWorldColorAlarmPath);
			Change.After->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
			Change.After->SetStringField(TEXT("note"),
				TEXT("Created /Game/Materials/Reactor (5) + /Game/Blueprints/Reactor (2). Reactor actors kept; power preserved. Props>=10. No save."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
	}

	FString PreflightSaveReactorWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(ReactorWorldColorSaveSpec))
		{
			return TEXT("spec must be reactor_world_color_save_v1.");
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
		if (!World || !FindLoadedLevelByPackage(World, ReactorPackage))
		{
			return TEXT("SL_Epitope_Reactor must be loaded.");
		}
		for (const FReactorWorldColorMatSpec& Spec : ReactorWorldColorMats)
		{
			if (!ReactorWorldColorMaterialExact(ReactorWorldColorLoadMat(Spec.Name), Spec))
			{
				return FString::Printf(TEXT("%s is missing or gray. Refusing save."), Spec.Name);
			}
		}
		if (!ReactorWorldColorLoadLightBp() || !ReactorWorldColorLoadAudioBp())
		{
			return TEXT("World-color Blueprints are missing. Refusing save.");
		}
		if (ReactorWorldColorCountProps(World) < 12)
		{
			return TEXT("Fewer than 12 Reactor_WC_ props. Refusing save.");
		}
		if (FindByExactLabel(World, ReactorWorldColorLightLabel).Num() != 1)
		{
			return TEXT("Reactor_WorldColor_LightController count must be 1.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		ReactorWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = ReactorWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ReactorPackage))
			{
				return FString::Printf(TEXT("Refusing save while non-Reactor world is dirty: %s"), *Name);
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
		Proposed->SetBoolField(TEXT("saves_compute_map"), true);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteSaveReactorWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_reactor_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveReactorWorldColor(Change.Args, Before, Proposed);
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
