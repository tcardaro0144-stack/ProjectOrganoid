// Beat 29 Compute world color: five zone materials, two Blueprints under
// /Game/Blueprints/Compute/, props, and material paint on Compute blockout meshes.
// Does not move Compute evidence datapads or change power. Does not save;
// save_compute_world_color persists Compute + the seven packages only.

	const TCHAR* ComputeWorldColorSpec = TEXT("compute_world_color_v1");
	const TCHAR* ComputeWorldColorAction = TEXT("create_compute_world_color");
	const TCHAR* ComputeWorldColorSaveSpec = TEXT("compute_world_color_save_v1");
	const TCHAR* ComputeWorldColorSaveAction = TEXT("save_compute_world_color");
	const TCHAR* ComputeWorldColorFolder = TEXT("/Game/Materials/Compute");
	const TCHAR* ComputeWorldColorBpFolder = TEXT("/Game/Blueprints/Compute");
	const TCHAR* ComputeWorldColorLightBpPackage = TEXT("/Game/Blueprints/Compute/BP_ComputeLightController");
	const TCHAR* ComputeWorldColorLightBpPath = TEXT("/Game/Blueprints/Compute/BP_ComputeLightController.BP_ComputeLightController");
	const TCHAR* ComputeWorldColorAudioBpPackage = TEXT("/Game/Blueprints/Compute/BP_ComputeAudioZone");
	const TCHAR* ComputeWorldColorAudioBpPath = TEXT("/Game/Blueprints/Compute/BP_ComputeAudioZone.BP_ComputeAudioZone");
	const TCHAR* ComputeWorldColorLightLabel = TEXT("Compute_WorldColor_LightController");
	// Compute floor is Z≈-3510. Place the world-color light controller near DataPad_AutonomousDecisionLog (0,-1650,-3510).
	const FVector ComputeWorldColorLightLocation(0.f, -1650.f, -3510.f);
	const FRotator ComputeWorldColorLightRotation(0.f, 0.f, 0.f);
	const FVector ComputeWorldColorLightScale(1.f, 1.f, 1.f);
	const TCHAR* ComputeWorldColorCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* ComputeWorldColorPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* ComputeWorldColorAlarmPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* ComputeWorldColorAmbienceNative = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone");
	const TCHAR* LegacyComputeLightControllerLabel = TEXT("__NO_LEGACY_COMPUTE_LIGHT__");

	enum class EComputeWorldColorZone : int32
	{
		Public = 0,
		Restricted,
		Executive,
		Operations,
		Service,
		Count
	};

	struct FComputeWorldColorMatSpec
	{
		const TCHAR* Name;
		EComputeWorldColorZone Zone;
		FLinearColor Color;
		float Roughness;
		float Metallic;
		float Emissive;
	};

	const FComputeWorldColorMatSpec ComputeWorldColorMats[] = {
		{ TEXT("M_Compute_Public_Server"), EComputeWorldColorZone::Public,
			FLinearColor(0.08f, 0.14f, 0.22f), 0.45f, 0.25f, 0.35f },
		{ TEXT("M_Compute_Restricted_Decision"), EComputeWorldColorZone::Restricted,
			FLinearColor(0.32f, 0.36f, 0.42f), 0.38f, 0.60f, 0.04f },
		{ TEXT("M_Compute_Executive_Observation"), EComputeWorldColorZone::Executive,
			FLinearColor(0.62f, 0.72f, 0.82f), 0.14f, 0.18f, 0.12f },
		{ TEXT("M_Compute_Operations_ComputeControl"), EComputeWorldColorZone::Operations,
			FLinearColor(0.05f, 0.40f, 0.35f), 0.20f, 0.15f, 1.15f },
		{ TEXT("M_Compute_Service_Industrial"), EComputeWorldColorZone::Service,
			FLinearColor(0.48f, 0.40f, 0.28f), 0.90f, 0.10f, 0.0f },
	};

	struct FComputeWorldColorPropSpec
	{
		const TCHAR* Label;
		EComputeWorldColorZone Zone;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bPlane;
		bool bText;
		const TCHAR* Text;
	};

	const FComputeWorldColorPropSpec ComputeWorldColorProps[] = {
		{ TEXT("Compute_WC_Sign_Server"), EComputeWorldColorZone::Public,
			FVector(0.f, -1500.f, -3380.f), FRotator(0.f, 0.f, 0.f), FVector(1.6f, 0.08f, 0.5f), false, true,
			TEXT("BIO-NEURAL COMPUTE — SERVER RACKS") },
		{ TEXT("Compute_WC_Sign_Decision"), EComputeWorldColorZone::Restricted,
			FVector(-200.f, -1800.f, -3380.f), FRotator(0.f, -90.f, 0.f), FVector(1.4f, 0.08f, 0.45f), false, true,
			TEXT("RESTRICTED — AUTONOMOUS DECISION LOG") },
		{ TEXT("Compute_WC_Rack_1"), EComputeWorldColorZone::Public,
			FVector(200.f, -1500.f, -3480.f), FRotator(0.f, 0.f, 0.f), FVector(0.6f, 0.4f, 1.6f), false, false, nullptr },
		{ TEXT("Compute_WC_Rack_2"), EComputeWorldColorZone::Public,
			FVector(-200.f, -1500.f, -3480.f), FRotator(0.f, 0.f, 0.f), FVector(0.6f, 0.4f, 1.6f), false, false, nullptr },
		{ TEXT("Compute_WC_Rack_3"), EComputeWorldColorZone::Public,
			FVector(350.f, -1750.f, -3480.f), FRotator(0.f, 15.f, 0.f), FVector(0.6f, 0.4f, 1.6f), false, false, nullptr },
		{ TEXT("Compute_WC_DecisionTerminal"), EComputeWorldColorZone::Restricted,
			FVector(120.f, -1750.f, -3490.f), FRotator(0.f, -20.f, 0.f), FVector(0.7f, 0.08f, 0.55f), false, false, nullptr },
		{ TEXT("Compute_WC_ConfessionDisplay"), EComputeWorldColorZone::Restricted,
			FVector(-2300.f, -2150.f, -3480.f), FRotator(0.f, 90.f, 0.f), FVector(0.55f, 0.08f, 0.45f), false, false, nullptr },
		{ TEXT("Compute_WC_CableTray_1"), EComputeWorldColorZone::Service,
			FVector(0.f, -1400.f, -3420.f), FRotator(0.f, 0.f, 0.f), FVector(2.5f, 0.2f, 0.12f), false, false, nullptr },
		{ TEXT("Compute_WC_CableTray_2"), EComputeWorldColorZone::Service,
			FVector(-2200.f, -2275.f, -3420.f), FRotator(0.f, 90.f, 0.f), FVector(2.0f, 0.2f, 0.12f), false, false, nullptr },
		{ TEXT("Compute_WC_FloorDecal"), EComputeWorldColorZone::Service,
			FVector(0.f, -1650.f, -3595.f), FRotator(0.f, 0.f, 0.f), FVector(2.5f, 2.5f, 1.f), true, false, nullptr },
		{ TEXT("Compute_WC_Observation_Glass"), EComputeWorldColorZone::Executive,
			FVector(400.f, -1650.f, -3400.f), FRotator(0.f, 90.f, 0.f), FVector(0.08f, 2.0f, 1.4f), false, false, nullptr },
		{ TEXT("Compute_WC_ControlPanel"), EComputeWorldColorZone::Operations,
			FVector(-100.f, -1900.f, -3450.f), FRotator(0.f, 0.f, 0.f), FVector(1.2f, 0.08f, 0.8f), false, false, nullptr },
		{ TEXT("Compute_WC_Brand_Compute"), EComputeWorldColorZone::Public,
			FVector(0.f, -1300.f, -3350.f), FRotator(0.f, 0.f, 0.f), FVector(1.8f, 0.05f, 0.45f), false, true,
			TEXT("EPITOPE — Lot Numbers / Substrate") },
	};

	FString ComputeWorldColorMatPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), ComputeWorldColorFolder, Name, Name);
	}

	FString ComputeWorldColorMatPackage(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s"), ComputeWorldColorFolder, Name);
	}

	const FComputeWorldColorMatSpec* ComputeWorldColorFindMat(EComputeWorldColorZone Zone)
	{
		for (const FComputeWorldColorMatSpec& Spec : ComputeWorldColorMats)
		{
			if (Spec.Zone == Zone)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool ComputeWorldColorColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.05f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.05f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.05f);
	}

	bool ComputeWorldColorIsGray(const FLinearColor& Color)
	{
		const float Max = FMath::Max3(Color.R, Color.G, Color.B);
		const float Min = FMath::Min3(Color.R, Color.G, Color.B);
		return (Max - Min) < 0.08f && Max > 0.15f && Max < 0.85f;
	}

	FLinearColor ComputeWorldColorReadMaterialColor(const UMaterialInterface* Material)
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

	void ComputeWorldColorWireMaterial(UMaterial* Material, const FComputeWorldColorMatSpec& Spec)
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

	bool ComputeWorldColorMaterialExact(const UMaterial* Material, const FComputeWorldColorMatSpec& Spec)
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
		if (!ColorExpr || !ComputeWorldColorColorNear(ColorExpr->DefaultValue, Spec.Color))
		{
			return false;
		}
		return !ComputeWorldColorIsGray(ColorExpr->DefaultValue);
	}

	UMaterial* ComputeWorldColorLoadMat(const TCHAR* Name)
	{
		return LoadObject<UMaterial>(nullptr, *ComputeWorldColorMatPath(Name));
	}

	bool ComputeWorldColorEnsureMaterials(FString& OutError)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		for (const FComputeWorldColorMatSpec& Spec : ComputeWorldColorMats)
		{
			UMaterial* Material = ComputeWorldColorLoadMat(Spec.Name);
			if (!Material)
			{
				UPackage* Package = CreatePackage(*ComputeWorldColorMatPackage(Spec.Name));
				Material = Cast<UMaterial>(Factory->FactoryCreateNew(
					UMaterial::StaticClass(), Package, Spec.Name, RF_Public | RF_Standalone, nullptr, GWarn));
				if (!Material)
				{
					OutError = FString::Printf(TEXT("Failed to create %s."), Spec.Name);
					return false;
				}
				FAssetRegistryModule::AssetCreated(Material);
			}
			ComputeWorldColorWireMaterial(Material, Spec);
			if (!ComputeWorldColorMaterialExact(Material, Spec))
			{
				OutError = FString::Printf(TEXT("%s is not a colored Compute material."), Spec.Name);
				return false;
			}
		}
		return true;
	}

	UBlueprint* ComputeWorldColorLoadLightBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ComputeWorldColorLightBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, ComputeWorldColorLightBpPackage);
		}
		return Blueprint;
	}

	UBlueprint* ComputeWorldColorLoadAudioBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, ComputeWorldColorAudioBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, ComputeWorldColorAudioBpPackage);
		}
		return Blueprint;
	}

	bool ComputeWorldColorEnsureBlueprints(FString& OutError)
	{
		if (!ComputeWorldColorLoadLightBp())
		{
			UPackage* Package = CreatePackage(ComputeWorldColorLightBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				Package,
				TEXT("BP_ComputeLightController"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created || !Created->SimpleConstructionScript)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Compute/BP_ComputeLightController.");
				return false;
			}
			USCS_Node* LightNode = Created->SimpleConstructionScript->CreateNode(
				UPointLightComponent::StaticClass(), TEXT("PublicZoneLight"));
			Created->SimpleConstructionScript->AddNode(LightNode);
			if (UPointLightComponent* Template = Cast<UPointLightComponent>(LightNode->ComponentTemplate))
			{
				Template->Intensity = 2800.f;
				Template->SetTemperature(6000.f);
				Template->bUseTemperature = true;
				Template->SetLightColor(FLinearColor(0.25f, 0.45f, 0.75f), true);
				Template->AttenuationRadius = 1200.f;
			}
			struct FComputeZoneLightSpec
			{
				const TCHAR* Name;
				FLinearColor Color;
				float Intensity;
				float Temp;
			};
			const FComputeZoneLightSpec ExtraLights[] = {
				{ TEXT("RestrictedZoneLight"), FLinearColor(0.30f, 0.38f, 0.55f), 1800.f, 7500.f },
				{ TEXT("ExecutiveZoneLight"), FLinearColor(1.0f, 0.90f, 0.78f), 3400.f, 4000.f },
				{ TEXT("OperationsZoneLight"), FLinearColor(0.35f, 0.85f, 0.70f), 5200.f, 6200.f },
				{ TEXT("ServiceZoneLight"), FLinearColor(0.50f, 0.48f, 0.42f), 1600.f, 3500.f },
			};
			for (const FComputeZoneLightSpec& Spec : ExtraLights)
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

		if (!ComputeWorldColorLoadAudioBp())
		{
			UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, ComputeWorldColorAmbienceNative);
			if (!Native)
			{
				OutError = TEXT("ProjectOrganoidAmbienceZone is not loaded.");
				return false;
			}
			UPackage* Package = CreatePackage(ComputeWorldColorAudioBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				Native,
				Package,
				TEXT("BP_ComputeAudioZone"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Compute/BP_ComputeAudioZone.");
				return false;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		UBlueprint* LightBp = ComputeWorldColorLoadLightBp();
		UBlueprint* AudioBp = ComputeWorldColorLoadAudioBp();
		if (!LightBp || !LightBp->GeneratedClass || LightBp->ParentClass != AActor::StaticClass())
		{
			OutError = TEXT("BP_ComputeLightController under /Game/Blueprints/Compute/ is invalid.");
			return false;
		}
		UClass* AmbienceNative = StaticLoadClass(UObject::StaticClass(), nullptr, ComputeWorldColorAmbienceNative);
		if (!AudioBp || !AudioBp->GeneratedClass || !AmbienceNative || !AudioBp->GeneratedClass->IsChildOf(AmbienceNative))
		{
			OutError = TEXT("BP_ComputeAudioZone under /Game/Blueprints/Compute/ must parent AmbienceZone.");
			return false;
		}
		return true;
	}

	EComputeWorldColorZone ComputeWorldColorClassifyLocation(const FVector& Location)
	{
		const FVector Decision(0.f, -1650.f, -3510.f);
		const FVector Delta = Location - Decision;
		// Near Sterling confession west → Restricted decision
		if (Location.X < -1800.f)
		{
			return EComputeWorldColorZone::Restricted;
		}
		// Near AutonomousDecisionLog core → Public server / Operations
		if (FMath::Abs(Delta.X) < 250.f && FMath::Abs(Delta.Y) < 250.f)
		{
			return EComputeWorldColorZone::Public;
		}
		if (Delta.Y < -200.f)
		{
			return EComputeWorldColorZone::Operations;
		}
		if (Delta.X > 250.f)
		{
			return EComputeWorldColorZone::Executive;
		}
		if (Delta.Y > 150.f)
		{
			return EComputeWorldColorZone::Service;
		}
		return EComputeWorldColorZone::Public;
	}

	EComputeWorldColorZone ComputeWorldColorClassifyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return EComputeWorldColorZone::Public;
		}
		const FString Label = ActorLabel(const_cast<AActor*>(Actor));
		if (Label.Contains(TEXT("Executive"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Observation"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Glass"), ESearchCase::IgnoreCase))
		{
			return EComputeWorldColorZone::Executive;
		}
		if (Label.Contains(TEXT("Operations"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Control"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("ComputeControl"), ESearchCase::IgnoreCase))
		{
			return EComputeWorldColorZone::Operations;
		}
		if (Label.Contains(TEXT("Service"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Cable"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Tray"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Corridor"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pipe"), ESearchCase::IgnoreCase))
		{
			return EComputeWorldColorZone::Service;
		}
		if (Label.Contains(TEXT("Decision"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Restricted"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Confession"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Sterling"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Autonomous"), ESearchCase::IgnoreCase))
		{
			return EComputeWorldColorZone::Restricted;
		}
		if (Label.Contains(TEXT("Server"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Rack"), ESearchCase::IgnoreCase))
		{
			return EComputeWorldColorZone::Public;
		}
		return ComputeWorldColorClassifyLocation(Actor->GetActorLocation());
	}

	bool ComputeWorldColorIsProtectedLabel(const FString& Label)
	{
		static const TCHAR* Protected[] = {
			TEXT("DataPad_AutonomousDecisionLog"),
			TEXT("DataPad_SterlingConfession"),
			TEXT("DataPad_SpecimenManifest"),
			TEXT("DataPad_ConsentForms"),
			TEXT("DataPad_SterlingCryoNote"),
			TEXT("Terminal_ControlSpine"),
			TEXT("Checkpoint_BasinRim"),
			TEXT("Checkpoint_InterfaceChamber"),
			TEXT("NavMeshBounds"),
			TEXT("RecastNavMesh"),
			TEXT("BackupPower"),
			TEXT("PowerPanel_"),
			TEXT("Hazard_"),
			TEXT("BP_Pursuer"),
			TEXT("BP_TransformedScientist"),
			TEXT("BP_NodeZeroCore"),
			TEXT("Reactor_"),
			TEXT("ResearchStation_"),
			TEXT("Compute_WorldColor_LightController"),
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

	bool ComputeWorldColorIsPaintableMesh(const AActor* Actor)
	{
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !PackagesEqual(ActorOwningPackage(const_cast<AStaticMeshActor*>(MeshActor)), ComputePackage))
		{
			return false;
		}
		const FString Label = ActorLabel(const_cast<AStaticMeshActor*>(MeshActor));
		if (ComputeWorldColorIsProtectedLabel(Label) || Label.StartsWith(TEXT("Compute_WC_"), ESearchCase::CaseSensitive))
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
			|| Lower.Contains(TEXT("compute"));
		const FVector Loc = MeshActor->GetActorLocation();
		const FVector Scale = MeshActor->GetActorScale3D();
		const bool bCeilingShape = Loc.Z > -3350.f && Scale.Z < 0.35f;
		const bool bFloorShape = Loc.Z < -3580.f && Scale.Z < 0.35f;
		UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
		const FLinearColor Slot0 = ComputeWorldColorReadMaterialColor(Comp ? Comp->GetMaterial(0) : nullptr);
		const bool bGraySlot = ComputeWorldColorIsGray(Slot0)
			|| (Comp && Comp->GetMaterial(0) && Comp->GetMaterial(0)->GetName().Contains(TEXT("WorldGridMaterial")));
		return bNameHit || bCeilingShape || bFloorShape || bGraySlot;
	}

	void ComputeWorldColorCollectDirty(TArray<FString>& OutContent, TArray<FString>& OutWorlds)
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

	TArray<FString> ComputeWorldColorAllowedPackages()
	{
		TArray<FString> Allowed;
		Allowed.Add(ComputePackage);
		Allowed.Add(ComputeWorldColorLightBpPackage);
		Allowed.Add(ComputeWorldColorAudioBpPackage);
		for (const FComputeWorldColorMatSpec& Spec : ComputeWorldColorMats)
		{
			Allowed.Add(ComputeWorldColorMatPackage(Spec.Name));
		}
		return Allowed;
	}

	FString ComputeWorldColorUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		ComputeWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = ComputeWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ComputePackage) && !WorldsBefore.Contains(Name))
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

	void ComputeWorldColorClearUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		const TArray<FString> Allowed = ComputeWorldColorAllowedPackages();
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

	FString ComputeWorldColorGuardProtected(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		struct FComputePadGuard
		{
			const TCHAR* Label;
			FVector Location;
		};
		const FComputePadGuard Pads[] = {
			{ TEXT("DataPad_AutonomousDecisionLog"), FVector(0.f, -1650.f, -3510.f) },
			{ TEXT("DataPad_SterlingConfession"), FVector(-2425.f, -2275.f, -3510.f) },
		};
		for (const FComputePadGuard& Pad : Pads)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Pad.Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1"), Pad.Label, Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), ComputePackage))
			{
				return FString::Printf(TEXT("%s is not on Compute."), Pad.Label);
			}
			if (!Matches[0]->GetActorLocation().Equals(Pad.Location, 1.f))
			{
				return FString::Printf(TEXT("%s moved. Abort."), Pad.Label);
			}
		}
		for (const TCHAR* Label : { TEXT("Terminal_ControlSpine"), TEXT("Checkpoint_BasinRim"),
			TEXT("BP_Pursuer"), TEXT("BP_TransformedScientist"), TEXT("BP_NodeZeroCore") })
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			for (AActor* Actor : Matches)
			{
				if (Actor && PackagesEqual(ActorOwningPackage(Actor), ComputePackage))
				{
					return FString::Printf(TEXT("%s must not live on Compute for this pass."), Label);
				}
			}
		}
		return FString();
	}

	int32 ComputeWorldColorCountProps(UWorld* World)
	{
		int32 Count = 0;
		for (const FComputeWorldColorPropSpec& Spec : ComputeWorldColorProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && PackagesEqual(ActorOwningPackage(Matches[0]), ComputePackage))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 ComputeWorldColorPaintMeshes(UWorld* World, int32& OutCeilingPainted, int32& OutGrayRemaining)
	{
		OutCeilingPainted = 0;
		OutGrayRemaining = 0;
		int32 Painted = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!ComputeWorldColorIsPaintableMesh(MeshActor))
			{
				continue;
			}
			UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
			const EComputeWorldColorZone Zone = ComputeWorldColorClassifyActor(MeshActor);
			const FComputeWorldColorMatSpec* MatSpec = ComputeWorldColorFindMat(Zone);
			UMaterial* Material = MatSpec ? ComputeWorldColorLoadMat(MatSpec->Name) : nullptr;
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
				const FLinearColor CurrentColor = ComputeWorldColorReadMaterialColor(Current);
				if (Current && !ComputeWorldColorIsGray(CurrentColor) && Current->GetPathName().Contains(TEXT("/Game/Materials/Compute/")))
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
				if (Label.Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -3350.f)
				{
					++OutCeilingPainted;
				}
			}
			const FLinearColor After = ComputeWorldColorReadMaterialColor(Comp->GetMaterial(0));
			if (ComputeWorldColorIsGray(After))
			{
				++OutGrayRemaining;
			}
		}
		return Painted;
	}

	FString ComputeWorldColorApplyProp(AActor* Actor, const FComputeWorldColorPropSpec& Spec)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Spec.bPlane ? ComputeWorldColorPlanePath : ComputeWorldColorCubePath);
		if (!Mesh)
		{
			return TEXT("Failed to load basic shape mesh.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		const FComputeWorldColorMatSpec* MatSpec = ComputeWorldColorFindMat(Spec.Zone);
		UMaterial* Material = MatSpec ? ComputeWorldColorLoadMat(MatSpec->Name) : nullptr;
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

	TSharedRef<FJsonObject> CmdInspectComputeWorldColor(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("compute_package"), ComputePackage);
		Data->SetBoolField(TEXT("compute_loaded"), World && FindLoadedLevelByPackage(World, ComputePackage) != nullptr);
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
		for (const FComputeWorldColorMatSpec& Spec : ComputeWorldColorMats)
		{
			UMaterial* Material = ComputeWorldColorLoadMat(Spec.Name);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Spec.Name);
			Row->SetBoolField(TEXT("exists"), Material != nullptr);
			const bool bExact = ComputeWorldColorMaterialExact(Material, Spec);
			Row->SetBoolField(TEXT("exact"), bExact);
			if (bExact)
			{
				++MatExact;
			}
			Mats.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("materials"), Mats);
		Data->SetNumberField(TEXT("materials_exact"), MatExact);
		Data->SetBoolField(TEXT("light_bp"), ComputeWorldColorLoadLightBp() != nullptr);
		Data->SetBoolField(TEXT("audio_bp"), ComputeWorldColorLoadAudioBp() != nullptr);
		Data->SetNumberField(TEXT("prop_count"), World ? ComputeWorldColorCountProps(World) : 0);
		Data->SetNumberField(TEXT("prop_expected"), UE_ARRAY_COUNT(ComputeWorldColorProps));
		TArray<AActor*> LegacyLights = World ? FindByExactLabel(World, LegacyComputeLightControllerLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("legacy_light_controller_count"), LegacyLights.Num());
		if (LegacyLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("legacy_light_controller_location"), Vec(LegacyLights[0]->GetActorLocation()));
		}
		TArray<AActor*> WorldColorLights = World ? FindByExactLabel(World, ComputeWorldColorLightLabel) : TArray<AActor*>();
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
				if (!ComputeWorldColorIsPaintableMesh(MeshActor))
				{
					continue;
				}
				++Paintable;
				UMaterialInterface* Mat = MeshActor->GetStaticMeshComponent()->GetMaterial(0);
				const FLinearColor Color = ComputeWorldColorReadMaterialColor(Mat);
				const bool bGray = ComputeWorldColorIsGray(Color);
				const FString Label = ActorLabel(MeshActor);
				const bool bCeiling = Label.ToLower().Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -3350.f;
				if (bCeiling && bGray)
				{
					++GrayCeiling;
				}
				if (bCeiling && Mat && Mat->GetPathName().Contains(TEXT("/Game/Materials/Compute/")))
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
		USoundWave* Alarm = LoadObject<USoundWave>(nullptr, ComputeWorldColorAlarmPath);
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
			TEXT("World-color light controller near DataPad_AutonomousDecisionLog (0,-1650,-3510). Datapads unmoved."));
		return Ok(Data);
	}

	FString PreflightCreateComputeWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(ComputeWorldColorSpec))
		{
			return TEXT("spec must be compute_world_color_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_compute_world_color does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, ComputePackage))
		{
			return TEXT("SL_Epitope_Compute must be loaded.");
		}
		const FString GuardError = ComputeWorldColorGuardProtected(World);
		if (!GuardError.IsEmpty())
		{
			return GuardError;
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		ComputeWorldColorCollectDirty(Content, Worlds);
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ComputePackage))
			{
				return FString::Printf(TEXT("Refusing create while non-Compute world is dirty: %s"), *Name);
			}
		}
		Before->SetNumberField(TEXT("prop_count"), ComputeWorldColorCountProps(World));
		Before->SetNumberField(TEXT("materials_exact"), 0);
		Proposed->SetStringField(TEXT("result"), TEXT("Create Compute world-color materials, Blueprints, props, and paint. Does not save."));
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetArrayField(TEXT("light_location"), Vec(ComputeWorldColorLightLocation));
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateComputeWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_compute_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateComputeWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		UWorld* World = GetEditorWorld();
		ULevel* ComputeLevel = World ? FindLoadedLevelByPackage(World, ComputePackage) : nullptr;
		if (!World || !ComputeLevel)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("not_found"), TEXT("Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		ComputeWorldColorCollectDirty(ContentBefore, WorldsBefore);

		FString EnsureError;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateComputeWorldColor", "Create Compute World Color"));
			if (!ComputeWorldColorEnsureMaterials(EnsureError) || !ComputeWorldColorEnsureBlueprints(EnsureError))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), EnsureError, MakeShared<FBridgeChange>(Change));
			}

			UBlueprint* LightBp = ComputeWorldColorLoadLightBp();
			if (FindByExactLabel(World, ComputeWorldColorLightLabel).Num() == 0 && LightBp && LightBp->GeneratedClass)
			{
				FActorSpawnParameters Params;
				Params.OverrideLevel = ComputeLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Spawned = World->SpawnActor<AActor>(
					LightBp->GeneratedClass,
					ComputeWorldColorLightLocation,
					ComputeWorldColorLightRotation,
					Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), TEXT("Failed to spawn Compute_WorldColor_LightController."), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(ComputeWorldColorLightScale);
#if WITH_EDITOR
				Spawned->SetActorLabel(ComputeWorldColorLightLabel);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Compute_WorldColor")));
				Spawned->Tags.AddUnique(FName(TEXT("Compute_WorldColor_Light")));
			}

			for (const FComputeWorldColorPropSpec& Spec : ComputeWorldColorProps)
			{
				if (FindByExactLabel(World, Spec.Label).Num() != 0)
				{
					continue;
				}
				FActorSpawnParameters Params;
				Params.OverrideLevel = ComputeLevel;
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
				Spawned->Tags.AddUnique(FName(TEXT("Compute_WorldColor")));
				const FString PropError = ComputeWorldColorApplyProp(Spawned, Spec);
				if (!PropError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("configure_failed"), PropError, MakeShared<FBridgeChange>(Change));
				}
			}

			int32 CeilingPainted = 0;
			int32 GrayRemaining = 0;
			const int32 Painted = ComputeWorldColorPaintMeshes(World, CeilingPainted, GrayRemaining);
			ComputeWorldColorClearUnexpectedDirty(ContentBefore, WorldsBefore);

			const FString DirtyError = ComputeWorldColorUnexpectedDirty(ContentBefore, WorldsBefore);
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
			Change.After->SetNumberField(TEXT("materials"), UE_ARRAY_COUNT(ComputeWorldColorMats));
			Change.After->SetNumberField(TEXT("blueprints"), 2);
			Change.After->SetNumberField(TEXT("props"), ComputeWorldColorCountProps(World));
			Change.After->SetNumberField(TEXT("meshes_painted"), Painted);
			Change.After->SetNumberField(TEXT("ceilings_painted"), CeilingPainted);
			Change.After->SetNumberField(TEXT("gray_remaining"), GrayRemaining);
			Change.After->SetArrayField(TEXT("light_location"), Vec(ComputeWorldColorLightLocation));
			Change.After->SetStringField(TEXT("light_label"), ComputeWorldColorLightLabel);
			Change.After->SetBoolField(TEXT("legacy_light_controller_kept"), FindByExactLabel(World, LegacyComputeLightControllerLabel).Num() == 1);
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
			USoundWave* Alarm = LoadObject<USoundWave>(nullptr, ComputeWorldColorAlarmPath);
			Change.After->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
			Change.After->SetStringField(TEXT("note"),
				TEXT("Created /Game/Materials/Compute (5) + /Game/Blueprints/Compute (2). Compute datapads kept; power preserved. Props>=10. No save."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
	}

	FString PreflightSaveComputeWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(ComputeWorldColorSaveSpec))
		{
			return TEXT("spec must be compute_world_color_save_v1.");
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
		if (!World || !FindLoadedLevelByPackage(World, ComputePackage))
		{
			return TEXT("SL_Epitope_Compute must be loaded.");
		}
		for (const FComputeWorldColorMatSpec& Spec : ComputeWorldColorMats)
		{
			if (!ComputeWorldColorMaterialExact(ComputeWorldColorLoadMat(Spec.Name), Spec))
			{
				return FString::Printf(TEXT("%s is missing or gray. Refusing save."), Spec.Name);
			}
		}
		if (!ComputeWorldColorLoadLightBp() || !ComputeWorldColorLoadAudioBp())
		{
			return TEXT("World-color Blueprints are missing. Refusing save.");
		}
		if (ComputeWorldColorCountProps(World) < 12)
		{
			return TEXT("Fewer than 12 Compute_WC_ props. Refusing save.");
		}
		if (FindByExactLabel(World, ComputeWorldColorLightLabel).Num() != 1)
		{
			return TEXT("Compute_WorldColor_LightController count must be 1.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		ComputeWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = ComputeWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, ComputePackage))
			{
				return FString::Printf(TEXT("Refusing save while non-Compute world is dirty: %s"), *Name);
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

	TSharedRef<FJsonObject> ExecuteSaveComputeWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_compute_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveComputeWorldColor(Change.Args, Before, Proposed);
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
