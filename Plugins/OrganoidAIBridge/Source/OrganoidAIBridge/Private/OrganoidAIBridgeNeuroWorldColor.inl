// Beat 27 NeuroGenetics world color: five zone materials, two Blueprints under
// /Game/Blueprints/Neuro/, props, and material paint on Neuro blockout meshes.
// Does not move ResearchStation_NeuroGenetics or Reactor actors. Does not save;
// save_neuro_world_color persists Neuro + the seven packages only.
	const TCHAR* NeuroWorldColorSpec = TEXT("neuro_world_color_v1");
	const TCHAR* NeuroWorldColorAction = TEXT("create_neuro_world_color");
	const TCHAR* NeuroWorldColorSaveSpec = TEXT("neuro_world_color_save_v1");
	const TCHAR* NeuroWorldColorSaveAction = TEXT("save_neuro_world_color");
	const TCHAR* NeuroWorldColorFolder = TEXT("/Game/Materials/Neuro");
	const TCHAR* NeuroWorldColorBpFolder = TEXT("/Game/Blueprints/Neuro");
	const TCHAR* NeuroWorldColorLightBpPackage = TEXT("/Game/Blueprints/Neuro/BP_NeuroLightController");
	const TCHAR* NeuroWorldColorLightBpPath = TEXT("/Game/Blueprints/Neuro/BP_NeuroLightController.BP_NeuroLightController");
	const TCHAR* NeuroWorldColorAudioBpPackage = TEXT("/Game/Blueprints/Neuro/BP_NeuroAudioZone");
	const TCHAR* NeuroWorldColorAudioBpPath = TEXT("/Game/Blueprints/Neuro/BP_NeuroAudioZone.BP_NeuroAudioZone");
	const TCHAR* NeuroWorldColorLightLabel = TEXT("Neuro_WorldColor_LightController");
	// ResearchStation_NeuroGenetics is at (800,-1600,-1100). Place the world-color
	// light controller at that same authored station location.
	const FVector NeuroWorldColorLightLocation(800.f, -1600.f, -1100.f);
	const FRotator NeuroWorldColorLightRotation(0.f, 0.f, 0.f);
	const FVector NeuroWorldColorLightScale(1.f, 1.f, 1.f);
	const TCHAR* NeuroWorldColorCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroWorldColorPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* NeuroWorldColorAlarmPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* NeuroWorldColorAmbienceNative = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone");
	const TCHAR* LegacyNeuroLightControllerLabel = TEXT("__NO_LEGACY_NEURO_LIGHT__");

	enum class ENeuroWorldColorZone : int32
	{
		Public = 0,
		Restricted,
		Executive,
		Operations,
		Service,
		Count
	};

	struct FNeuroWorldColorMatSpec
	{
		const TCHAR* Name;
		ENeuroWorldColorZone Zone;
		FLinearColor Color;
		float Roughness;
		float Metallic;
		float Emissive;
	};

	const FNeuroWorldColorMatSpec NeuroWorldColorMats[] = {
		{ TEXT("M_Neuro_Public_Research"), ENeuroWorldColorZone::Public,
			FLinearColor(0.92f, 0.94f, 0.96f), 0.28f, 0.02f, 0.04f },
		{ TEXT("M_Neuro_Restricted_Genetics"), ENeuroWorldColorZone::Restricted,
			FLinearColor(0.22f, 0.35f, 0.28f), 0.50f, 0.35f, 0.05f },
		{ TEXT("M_Neuro_Executive_Observation"), ENeuroWorldColorZone::Executive,
			FLinearColor(0.55f, 0.72f, 0.82f), 0.18f, 0.20f, 0.15f },
		{ TEXT("M_Neuro_Operations_Lab"), ENeuroWorldColorZone::Operations,
			FLinearColor(0.12f, 0.42f, 0.55f), 0.25f, 0.12f, 0.85f },
		{ TEXT("M_Neuro_Service_Industrial"), ENeuroWorldColorZone::Service,
			FLinearColor(0.42f, 0.36f, 0.28f), 0.90f, 0.10f, 0.0f },
	};

	struct FNeuroWorldColorPropSpec
	{
		const TCHAR* Label;
		ENeuroWorldColorZone Zone;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bPlane;
		bool bText;
		const TCHAR* Text;
	};

	const FNeuroWorldColorPropSpec NeuroWorldColorProps[] = {
		{ TEXT("Neuro_WC_Sign_Lab"), ENeuroWorldColorZone::Public,
			FVector(800.f, -1450.f, -980.f), FRotator(0.f, 0.f, 0.f), FVector(1.5f, 0.08f, 0.5f), false, true,
			TEXT("BSL-4 NEUROGENETICS — CLEAN LAB") },
		{ TEXT("Neuro_WC_Sign_Genetics"), ENeuroWorldColorZone::Restricted,
			FVector(950.f, -1750.f, -980.f), FRotator(0.f, -90.f, 0.f), FVector(1.3f, 0.08f, 0.45f), false, true,
			TEXT("RESTRICTED — ORGANOID MATRIX") },
		{ TEXT("Neuro_WC_Microscope_1"), ENeuroWorldColorZone::Operations,
			FVector(720.f, -1520.f, -1080.f), FRotator(0.f, 20.f, 0.f), FVector(0.35f, 0.35f, 0.7f), false, false, nullptr },
		{ TEXT("Neuro_WC_Microscope_2"), ENeuroWorldColorZone::Operations,
			FVector(880.f, -1520.f, -1080.f), FRotator(0.f, -15.f, 0.f), FVector(0.35f, 0.35f, 0.7f), false, false, nullptr },
		{ TEXT("Neuro_WC_Centrifuge_1"), ENeuroWorldColorZone::Operations,
			FVector(680.f, -1680.f, -1085.f), FRotator(0.f, 40.f, 0.f), FVector(0.45f, 0.45f, 0.55f), false, false, nullptr },
		{ TEXT("Neuro_WC_Centrifuge_2"), ENeuroWorldColorZone::Operations,
			FVector(920.f, -1680.f, -1085.f), FRotator(0.f, -30.f, 0.f), FVector(0.45f, 0.45f, 0.55f), false, false, nullptr },
		{ TEXT("Neuro_WC_SpecimenTray_1"), ENeuroWorldColorZone::Restricted,
			FVector(760.f, -1750.f, -1090.f), FRotator(0.f, 0.f, 0.f), FVector(0.7f, 0.4f, 0.08f), false, false, nullptr },
		{ TEXT("Neuro_WC_SpecimenTray_2"), ENeuroWorldColorZone::Restricted,
			FVector(840.f, -1750.f, -1090.f), FRotator(0.f, 10.f, 0.f), FVector(0.7f, 0.4f, 0.08f), false, false, nullptr },
		{ TEXT("Neuro_WC_SyringeKit_Display"), ENeuroWorldColorZone::Public,
			FVector(650.f, -1600.f, -1080.f), FRotator(0.f, 90.f, 0.f), FVector(0.5f, 0.25f, 0.2f), false, false, nullptr },
		{ TEXT("Neuro_WC_Biohazard_Decal"), ENeuroWorldColorZone::Restricted,
			FVector(800.f, -1600.f, -1195.f), FRotator(0.f, 0.f, 0.f), FVector(2.2f, 2.2f, 1.f), true, false, nullptr },
		{ TEXT("Neuro_WC_Observation_Glass"), ENeuroWorldColorZone::Executive,
			FVector(1100.f, -1600.f, -1000.f), FRotator(0.f, 90.f, 0.f), FVector(0.08f, 2.0f, 1.4f), false, false, nullptr },
		{ TEXT("Neuro_WC_Service_Pipe"), ENeuroWorldColorZone::Service,
			FVector(500.f, -1400.f, -1050.f), FRotator(0.f, 25.f, 0.f), FVector(2.0f, 0.16f, 0.16f), false, false, nullptr },
		{ TEXT("Neuro_WC_Brand_Organoid"), ENeuroWorldColorZone::Public,
			FVector(800.f, -1300.f, -950.f), FRotator(0.f, 0.f, 0.f), FVector(1.6f, 0.05f, 0.45f), false, true,
			TEXT("EPITOPE — Neural Change Evidence") },
	};

	FString NeuroWorldColorMatPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), NeuroWorldColorFolder, Name, Name);
	}

	FString NeuroWorldColorMatPackage(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s"), NeuroWorldColorFolder, Name);
	}

	const FNeuroWorldColorMatSpec* NeuroWorldColorFindMat(ENeuroWorldColorZone Zone)
	{
		for (const FNeuroWorldColorMatSpec& Spec : NeuroWorldColorMats)
		{
			if (Spec.Zone == Zone)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool NeuroWorldColorColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.05f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.05f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.05f);
	}

	bool NeuroWorldColorIsGray(const FLinearColor& Color)
	{
		const float Max = FMath::Max3(Color.R, Color.G, Color.B);
		const float Min = FMath::Min3(Color.R, Color.G, Color.B);
		return (Max - Min) < 0.08f && Max > 0.15f && Max < 0.85f;
	}

	FLinearColor NeuroWorldColorReadMaterialColor(const UMaterialInterface* Material)
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

	void NeuroWorldColorWireMaterial(UMaterial* Material, const FNeuroWorldColorMatSpec& Spec)
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

	bool NeuroWorldColorMaterialExact(const UMaterial* Material, const FNeuroWorldColorMatSpec& Spec)
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
		if (!ColorExpr || !NeuroWorldColorColorNear(ColorExpr->DefaultValue, Spec.Color))
		{
			return false;
		}
		return !NeuroWorldColorIsGray(ColorExpr->DefaultValue);
	}

	UMaterial* NeuroWorldColorLoadMat(const TCHAR* Name)
	{
		return LoadObject<UMaterial>(nullptr, *NeuroWorldColorMatPath(Name));
	}

	bool NeuroWorldColorEnsureMaterials(FString& OutError)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		for (const FNeuroWorldColorMatSpec& Spec : NeuroWorldColorMats)
		{
			UMaterial* Material = NeuroWorldColorLoadMat(Spec.Name);
			if (!Material)
			{
				UPackage* Package = CreatePackage(*NeuroWorldColorMatPackage(Spec.Name));
				Material = Cast<UMaterial>(Factory->FactoryCreateNew(
					UMaterial::StaticClass(), Package, Spec.Name, RF_Public | RF_Standalone, nullptr, GWarn));
				if (!Material)
				{
					OutError = FString::Printf(TEXT("Failed to create %s."), Spec.Name);
					return false;
				}
				FAssetRegistryModule::AssetCreated(Material);
			}
			NeuroWorldColorWireMaterial(Material, Spec);
			if (!NeuroWorldColorMaterialExact(Material, Spec))
			{
				OutError = FString::Printf(TEXT("%s is not a colored Admin material."), Spec.Name);
				return false;
			}
		}
		return true;
	}

	UBlueprint* NeuroWorldColorLoadLightBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, NeuroWorldColorLightBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, NeuroWorldColorLightBpPackage);
		}
		return Blueprint;
	}

	UBlueprint* NeuroWorldColorLoadAudioBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, NeuroWorldColorAudioBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, NeuroWorldColorAudioBpPackage);
		}
		return Blueprint;
	}

	bool NeuroWorldColorEnsureBlueprints(FString& OutError)
	{
		if (!NeuroWorldColorLoadLightBp())
		{
			UPackage* Package = CreatePackage(NeuroWorldColorLightBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				Package,
				TEXT("BP_NeuroLightController"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created || !Created->SimpleConstructionScript)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Neuro/BP_NeuroLightController.");
				return false;
			}
			USCS_Node* LightNode = Created->SimpleConstructionScript->CreateNode(
				UPointLightComponent::StaticClass(), TEXT("PublicZoneLight"));
			Created->SimpleConstructionScript->AddNode(LightNode);
			if (UPointLightComponent* Template = Cast<UPointLightComponent>(LightNode->ComponentTemplate))
			{
				Template->Intensity = 4500.f;
				Template->SetTemperature(6500.f);
				Template->bUseTemperature = true;
				Template->SetLightColor(FLinearColor(0.92f, 0.94f, 0.96f), true);
				Template->AttenuationRadius = 1000.f;
			}
			struct FNeuroZoneLightSpec
			{
				const TCHAR* Name;
				FLinearColor Color;
				float Intensity;
				float Temp;
			};
			const FNeuroZoneLightSpec ExtraLights[] = {
				{ TEXT("RestrictedZoneLight"), FLinearColor(0.22f, 0.35f, 0.28f), 3800.f, 4800.f },
				{ TEXT("ExecutiveZoneLight"), FLinearColor(0.55f, 0.72f, 0.82f), 4200.f, 7000.f },
				{ TEXT("OperationsZoneLight"), FLinearColor(0.12f, 0.42f, 0.55f), 5000.f, 5600.f },
				{ TEXT("ServiceZoneLight"), FLinearColor(0.42f, 0.36f, 0.28f), 2800.f, 3200.f },
			};
			for (const FNeuroZoneLightSpec& Spec : ExtraLights)
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

		if (!NeuroWorldColorLoadAudioBp())
		{
			UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, NeuroWorldColorAmbienceNative);
			if (!Native)
			{
				OutError = TEXT("ProjectOrganoidAmbienceZone is not loaded.");
				return false;
			}
			UPackage* Package = CreatePackage(NeuroWorldColorAudioBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				Native,
				Package,
				TEXT("BP_NeuroAudioZone"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Neuro/BP_NeuroAudioZone.");
				return false;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		UBlueprint* LightBp = NeuroWorldColorLoadLightBp();
		UBlueprint* AudioBp = NeuroWorldColorLoadAudioBp();
		if (!LightBp || !LightBp->GeneratedClass || LightBp->ParentClass != AActor::StaticClass())
		{
			OutError = TEXT("BP_NeuroLightController under /Game/Blueprints/Neuro/ is invalid.");
			return false;
		}
		UClass* AmbienceNative = StaticLoadClass(UObject::StaticClass(), nullptr, NeuroWorldColorAmbienceNative);
		if (!AudioBp || !AudioBp->GeneratedClass || !AmbienceNative || !AudioBp->GeneratedClass->IsChildOf(AmbienceNative))
		{
			OutError = TEXT("BP_NeuroAudioZone under /Game/Blueprints/Neuro/ must parent AmbienceZone.");
			return false;
		}
		return true;
	}

	ENeuroWorldColorZone NeuroWorldColorClassifyLocation(const FVector& Location)
	{
		const FVector Station(800.f, -1600.f, -1100.f);
		const FVector Delta = Location - Station;
		if (Delta.X > 250.f)
		{
			return ENeuroWorldColorZone::Executive;
		}
		if (Delta.X < -250.f || Delta.Y > 250.f)
		{
			return ENeuroWorldColorZone::Service;
		}
		if (Delta.Y < -80.f)
		{
			return ENeuroWorldColorZone::Restricted;
		}
		if (FMath::Abs(Delta.X) < 180.f && FMath::Abs(Delta.Y) < 180.f)
		{
			return ENeuroWorldColorZone::Operations;
		}
		return ENeuroWorldColorZone::Public;
	}

	ENeuroWorldColorZone NeuroWorldColorClassifyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return ENeuroWorldColorZone::Public;
		}
		const FString Label = ActorLabel(const_cast<AActor*>(Actor));
		if (Label.Contains(TEXT("Executive"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Observation"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Glass"), ESearchCase::IgnoreCase))
		{
			return ENeuroWorldColorZone::Executive;
		}
		if (Label.Contains(TEXT("Operations"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Microscope"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Centrifuge"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Bench"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("OpsLab"), ESearchCase::IgnoreCase))
		{
			return ENeuroWorldColorZone::Operations;
		}
		if (Label.Contains(TEXT("Service"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Transit"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pipe"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Corridor"), ESearchCase::IgnoreCase))
		{
			return ENeuroWorldColorZone::Service;
		}
		// Do not match bare "Genetics" — labels like NeuroGenetics_FloorPlate would all hit Restricted.
		if (Label.Contains(TEXT("Containment"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Restricted"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Biohazard"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Specimen"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("OrganoidMatrix"), ESearchCase::IgnoreCase))
		{
			return ENeuroWorldColorZone::Restricted;
		}
		return NeuroWorldColorClassifyLocation(Actor->GetActorLocation());
	}

	bool NeuroWorldColorIsProtectedLabel(const FString& Label)
	{
		static const TCHAR* Protected[] = {
			TEXT("ResearchStation_NeuroGenetics"),
			TEXT("Terminal_ControlSpine"),
			TEXT("Checkpoint_BasinRim"),
			TEXT("NavMeshBounds"),
			TEXT("RecastNavMesh"),
			TEXT("DataPad_"),
			TEXT("PowerPanel_"),
			TEXT("NeuralMapping"),
			TEXT("NeuralSignature"),
			TEXT("NeuralChange"),
			TEXT("Hazard_"),
			TEXT("CorridorTraps_"),
			TEXT("BP_Pursuer"),
			TEXT("BP_TransformedScientist"),
			TEXT("BP_NodeZeroCore"),
			TEXT("Reactor_"),
			TEXT("AdaptationSubject"),
			TEXT("Neuro_WorldColor_LightController"),
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

	bool NeuroWorldColorIsPaintableMesh(const AActor* Actor)
	{
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !PackagesEqual(ActorOwningPackage(const_cast<AStaticMeshActor*>(MeshActor)), NeuroPackage))
		{
			return false;
		}
		const FString Label = ActorLabel(const_cast<AStaticMeshActor*>(MeshActor));
		if (NeuroWorldColorIsProtectedLabel(Label) || Label.StartsWith(TEXT("Neuro_WC_"), ESearchCase::CaseSensitive))
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
			|| Lower.Contains(TEXT("neuro"));
		const FVector Loc = MeshActor->GetActorLocation();
		const FVector Scale = MeshActor->GetActorScale3D();
		const bool bCeilingShape = Loc.Z > -900.f && Scale.Z < 0.35f;
		const bool bFloorShape = Loc.Z < -1180.f && Scale.Z < 0.35f;
		UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
		const FLinearColor Slot0 = NeuroWorldColorReadMaterialColor(Comp ? Comp->GetMaterial(0) : nullptr);
		const bool bGraySlot = NeuroWorldColorIsGray(Slot0)
			|| (Comp && Comp->GetMaterial(0) && Comp->GetMaterial(0)->GetName().Contains(TEXT("WorldGridMaterial")));
		return bNameHit || bCeilingShape || bFloorShape || bGraySlot;
	}

	void NeuroWorldColorCollectDirty(TArray<FString>& OutContent, TArray<FString>& OutWorlds)
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

	TArray<FString> NeuroWorldColorAllowedPackages()
	{
		TArray<FString> Allowed;
		Allowed.Add(NeuroPackage);
		Allowed.Add(NeuroWorldColorLightBpPackage);
		Allowed.Add(NeuroWorldColorAudioBpPackage);
		for (const FNeuroWorldColorMatSpec& Spec : NeuroWorldColorMats)
		{
			Allowed.Add(NeuroWorldColorMatPackage(Spec.Name));
		}
		return Allowed;
	}

	FString NeuroWorldColorUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		NeuroWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = NeuroWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, NeuroPackage) && !WorldsBefore.Contains(Name))
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

	void NeuroWorldColorClearUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		const TArray<FString> Allowed = NeuroWorldColorAllowedPackages();
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

	FString NeuroWorldColorGuardProtected(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		TArray<AActor*> Stations = FindByExactLabel(World, TEXT("ResearchStation_NeuroGenetics"));
		if (Stations.Num() != 1)
		{
			return FString::Printf(TEXT("ResearchStation_NeuroGenetics count=%d expected=1"), Stations.Num());
		}
		if (!PackagesEqual(ActorOwningPackage(Stations[0]), NeuroPackage))
		{
			return TEXT("ResearchStation_NeuroGenetics is not on NeuroGenetics.");
		}
		if (!Stations[0]->GetActorLocation().Equals(FVector(800.f, -1600.f, -1100.f), 1.f))
		{
			return TEXT("ResearchStation_NeuroGenetics moved. Abort.");
		}
		for (const TCHAR* Label : { TEXT("Terminal_ControlSpine"), TEXT("Checkpoint_BasinRim"),
			TEXT("BP_Pursuer"), TEXT("BP_TransformedScientist"), TEXT("BP_NodeZeroCore") })
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Label);
			for (AActor* Actor : Matches)
			{
				if (Actor && PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
				{
					return FString::Printf(TEXT("%s must not live on NeuroGenetics for this pass."), Label);
				}
			}
		}
		return FString();
	}

	int32 NeuroWorldColorCountProps(UWorld* World)
	{
		int32 Count = 0;
		for (const FNeuroWorldColorPropSpec& Spec : NeuroWorldColorProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && PackagesEqual(ActorOwningPackage(Matches[0]), NeuroPackage))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 NeuroWorldColorPaintMeshes(UWorld* World, int32& OutCeilingPainted, int32& OutGrayRemaining)
	{
		OutCeilingPainted = 0;
		OutGrayRemaining = 0;
		int32 Painted = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!NeuroWorldColorIsPaintableMesh(MeshActor))
			{
				continue;
			}
			UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
			const ENeuroWorldColorZone Zone = NeuroWorldColorClassifyActor(MeshActor);
			const FNeuroWorldColorMatSpec* MatSpec = NeuroWorldColorFindMat(Zone);
			UMaterial* Material = MatSpec ? NeuroWorldColorLoadMat(MatSpec->Name) : nullptr;
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
				const FLinearColor CurrentColor = NeuroWorldColorReadMaterialColor(Current);
				if (Current && !NeuroWorldColorIsGray(CurrentColor) && Current->GetPathName().Contains(TEXT("/Game/Materials/Neuro/")))
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
				if (Label.Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -900.f)
				{
					++OutCeilingPainted;
				}
			}
			const FLinearColor After = NeuroWorldColorReadMaterialColor(Comp->GetMaterial(0));
			if (NeuroWorldColorIsGray(After))
			{
				++OutGrayRemaining;
			}
		}
		return Painted;
	}

	FString NeuroWorldColorApplyProp(AActor* Actor, const FNeuroWorldColorPropSpec& Spec)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Spec.bPlane ? NeuroWorldColorPlanePath : NeuroWorldColorCubePath);
		if (!Mesh)
		{
			return TEXT("Failed to load basic shape mesh.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		const FNeuroWorldColorMatSpec* MatSpec = NeuroWorldColorFindMat(Spec.Zone);
		UMaterial* Material = MatSpec ? NeuroWorldColorLoadMat(MatSpec->Name) : nullptr;
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

	TSharedRef<FJsonObject> CmdInspectNeuroWorldColor(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("neuro_package"), NeuroPackage);
		Data->SetBoolField(TEXT("neuro_loaded"), World && FindLoadedLevelByPackage(World, NeuroPackage) != nullptr);
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
		for (const FNeuroWorldColorMatSpec& Spec : NeuroWorldColorMats)
		{
			UMaterial* Material = NeuroWorldColorLoadMat(Spec.Name);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Spec.Name);
			Row->SetBoolField(TEXT("exists"), Material != nullptr);
			const bool bExact = NeuroWorldColorMaterialExact(Material, Spec);
			Row->SetBoolField(TEXT("exact"), bExact);
			if (bExact)
			{
				++MatExact;
			}
			Mats.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("materials"), Mats);
		Data->SetNumberField(TEXT("materials_exact"), MatExact);
		Data->SetBoolField(TEXT("light_bp"), NeuroWorldColorLoadLightBp() != nullptr);
		Data->SetBoolField(TEXT("audio_bp"), NeuroWorldColorLoadAudioBp() != nullptr);
		Data->SetNumberField(TEXT("prop_count"), World ? NeuroWorldColorCountProps(World) : 0);
		Data->SetNumberField(TEXT("prop_expected"), UE_ARRAY_COUNT(NeuroWorldColorProps));
		TArray<AActor*> LegacyLights = World ? FindByExactLabel(World, LegacyNeuroLightControllerLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("legacy_light_controller_count"), LegacyLights.Num());
		if (LegacyLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("legacy_light_controller_location"), Vec(LegacyLights[0]->GetActorLocation()));
		}
		TArray<AActor*> WorldColorLights = World ? FindByExactLabel(World, NeuroWorldColorLightLabel) : TArray<AActor*>();
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
				if (!NeuroWorldColorIsPaintableMesh(MeshActor))
				{
					continue;
				}
				++Paintable;
				UMaterialInterface* Mat = MeshActor->GetStaticMeshComponent()->GetMaterial(0);
				const FLinearColor Color = NeuroWorldColorReadMaterialColor(Mat);
				const bool bGray = NeuroWorldColorIsGray(Color);
				const FString Label = ActorLabel(MeshActor);
				const bool bCeiling = Label.ToLower().Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -900.f;
				if (bCeiling && bGray)
				{
					++GrayCeiling;
				}
				if (bCeiling && Mat && Mat->GetPathName().Contains(TEXT("/Game/Materials/Neuro/")))
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
		USoundWave* Alarm = LoadObject<USoundWave>(nullptr, NeuroWorldColorAlarmPath);
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
			TEXT("World-color light controller at ResearchStation_NeuroGenetics (800,-1600,-1100)."));
		return Ok(Data);
	}

	FString PreflightCreateNeuroWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(NeuroWorldColorSpec))
		{
			return TEXT("spec must be neuro_world_color_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_neuro_world_color does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, NeuroPackage))
		{
			return TEXT("SL_Epitope_NeuroGenetics must be loaded.");
		}
		const FString GuardError = NeuroWorldColorGuardProtected(World);
		if (!GuardError.IsEmpty())
		{
			return GuardError;
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		NeuroWorldColorCollectDirty(Content, Worlds);
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, NeuroPackage))
			{
				return FString::Printf(TEXT("Refusing create while non-Neuro world is dirty: %s"), *Name);
			}
		}
		Before->SetNumberField(TEXT("prop_count"), NeuroWorldColorCountProps(World));
		Before->SetNumberField(TEXT("materials_exact"), 0);
		Proposed->SetStringField(TEXT("result"), TEXT("Create Neuro world-color materials, Blueprints, props, and paint. Does not save."));
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetArrayField(TEXT("light_location"), Vec(NeuroWorldColorLightLocation));
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateNeuroWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_neuro_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNeuroWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		UWorld* World = GetEditorWorld();
		ULevel* NeuroLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		if (!World || !NeuroLevel)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("not_found"), TEXT("Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		NeuroWorldColorCollectDirty(ContentBefore, WorldsBefore);

		FString EnsureError;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateNeuroWorldColor", "Create Neuro World Color"));
			if (!NeuroWorldColorEnsureMaterials(EnsureError) || !NeuroWorldColorEnsureBlueprints(EnsureError))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), EnsureError, MakeShared<FBridgeChange>(Change));
			}

			UBlueprint* LightBp = NeuroWorldColorLoadLightBp();
			if (FindByExactLabel(World, NeuroWorldColorLightLabel).Num() == 0 && LightBp && LightBp->GeneratedClass)
			{
				FActorSpawnParameters Params;
				Params.OverrideLevel = NeuroLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Spawned = World->SpawnActor<AActor>(
					LightBp->GeneratedClass,
					NeuroWorldColorLightLocation,
					NeuroWorldColorLightRotation,
					Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), TEXT("Failed to spawn Neuro_WorldColor_LightController."), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(NeuroWorldColorLightScale);
#if WITH_EDITOR
				Spawned->SetActorLabel(NeuroWorldColorLightLabel);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Neuro_WorldColor")));
				Spawned->Tags.AddUnique(FName(TEXT("Neuro_WorldColor_Light")));
			}

			for (const FNeuroWorldColorPropSpec& Spec : NeuroWorldColorProps)
			{
				if (FindByExactLabel(World, Spec.Label).Num() != 0)
				{
					continue;
				}
				FActorSpawnParameters Params;
				Params.OverrideLevel = NeuroLevel;
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
				Spawned->Tags.AddUnique(FName(TEXT("Neuro_WorldColor")));
				const FString PropError = NeuroWorldColorApplyProp(Spawned, Spec);
				if (!PropError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("configure_failed"), PropError, MakeShared<FBridgeChange>(Change));
				}
			}

			int32 CeilingPainted = 0;
			int32 GrayRemaining = 0;
			const int32 Painted = NeuroWorldColorPaintMeshes(World, CeilingPainted, GrayRemaining);
			NeuroWorldColorClearUnexpectedDirty(ContentBefore, WorldsBefore);

			const FString DirtyError = NeuroWorldColorUnexpectedDirty(ContentBefore, WorldsBefore);
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
			Change.After->SetNumberField(TEXT("materials"), UE_ARRAY_COUNT(NeuroWorldColorMats));
			Change.After->SetNumberField(TEXT("blueprints"), 2);
			Change.After->SetNumberField(TEXT("props"), NeuroWorldColorCountProps(World));
			Change.After->SetNumberField(TEXT("meshes_painted"), Painted);
			Change.After->SetNumberField(TEXT("ceilings_painted"), CeilingPainted);
			Change.After->SetNumberField(TEXT("gray_remaining"), GrayRemaining);
			Change.After->SetArrayField(TEXT("light_location"), Vec(NeuroWorldColorLightLocation));
			Change.After->SetStringField(TEXT("light_label"), NeuroWorldColorLightLabel);
			Change.After->SetBoolField(TEXT("legacy_light_controller_kept"), FindByExactLabel(World, LegacyNeuroLightControllerLabel).Num() == 1);
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
			USoundWave* Alarm = LoadObject<USoundWave>(nullptr, NeuroWorldColorAlarmPath);
			Change.After->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
			Change.After->SetStringField(TEXT("note"),
				TEXT("Created /Game/Materials/Neuro (5) + /Game/Blueprints/Neuro (2). ResearchStation kept; S20/S22 Admin controllers untouched. Props>=10. No save."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
	}

	FString PreflightSaveNeuroWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(NeuroWorldColorSaveSpec))
		{
			return TEXT("spec must be neuro_world_color_save_v1.");
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
		if (!World || !FindLoadedLevelByPackage(World, NeuroPackage))
		{
			return TEXT("SL_Epitope_NeuroGenetics must be loaded.");
		}
		for (const FNeuroWorldColorMatSpec& Spec : NeuroWorldColorMats)
		{
			if (!NeuroWorldColorMaterialExact(NeuroWorldColorLoadMat(Spec.Name), Spec))
			{
				return FString::Printf(TEXT("%s is missing or gray. Refusing save."), Spec.Name);
			}
		}
		if (!NeuroWorldColorLoadLightBp() || !NeuroWorldColorLoadAudioBp())
		{
			return TEXT("World-color Blueprints are missing. Refusing save.");
		}
		if (NeuroWorldColorCountProps(World) < 10)
		{
			return TEXT("Fewer than 10 Neuro_WC_ props. Refusing save.");
		}
		if (FindByExactLabel(World, NeuroWorldColorLightLabel).Num() != 1)
		{
			return TEXT("Neuro_WorldColor_LightController count must be 1.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		NeuroWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = NeuroWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, NeuroPackage))
			{
				return FString::Printf(TEXT("Refusing save while non-Neuro world is dirty: %s"), *Name);
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
		Proposed->SetBoolField(TEXT("saves_neuro_map"), true);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteSaveNeuroWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_neuro_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveNeuroWorldColor(Change.Args, Before, Proposed);
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
