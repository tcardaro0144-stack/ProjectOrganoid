// Beat 28 Cryo world color: five zone materials, two Blueprints under
// /Game/Blueprints/Cryo/, props, and material paint on Cryo blockout meshes.
// Does not move Cryo evidence datapads or change power. Does not save;
// save_cryo_world_color persists Cryo + the seven packages only.

	const TCHAR* CryoWorldColorSpec = TEXT("cryo_world_color_v1");
	const TCHAR* CryoWorldColorAction = TEXT("create_cryo_world_color");
	const TCHAR* CryoWorldColorSaveSpec = TEXT("cryo_world_color_save_v1");
	const TCHAR* CryoWorldColorSaveAction = TEXT("save_cryo_world_color");
	const TCHAR* CryoWorldColorFolder = TEXT("/Game/Materials/Cryo");
	const TCHAR* CryoWorldColorBpFolder = TEXT("/Game/Blueprints/Cryo");
	const TCHAR* CryoWorldColorLightBpPackage = TEXT("/Game/Blueprints/Cryo/BP_CryoLightController");
	const TCHAR* CryoWorldColorLightBpPath = TEXT("/Game/Blueprints/Cryo/BP_CryoLightController.BP_CryoLightController");
	const TCHAR* CryoWorldColorAudioBpPackage = TEXT("/Game/Blueprints/Cryo/BP_CryoAudioZone");
	const TCHAR* CryoWorldColorAudioBpPath = TEXT("/Game/Blueprints/Cryo/BP_CryoAudioZone.BP_CryoAudioZone");
	const TCHAR* CryoWorldColorLightLabel = TEXT("Cryo_WorldColor_LightController");
	// Cryo floor is Z≈-2310. Place the world-color light controller at Cryo center (-1200,-800,-2310).
	const FVector CryoWorldColorLightLocation(-1200.f, -800.f, -2310.f);
	const FRotator CryoWorldColorLightRotation(0.f, 0.f, 0.f);
	const FVector CryoWorldColorLightScale(1.f, 1.f, 1.f);
	const TCHAR* CryoWorldColorCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CryoWorldColorPlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* CryoWorldColorAlarmPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* CryoWorldColorAmbienceNative = TEXT("/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone");
	const TCHAR* LegacyCryoLightControllerLabel = TEXT("__NO_LEGACY_CRYO_LIGHT__");

	enum class ECryoWorldColorZone : int32
	{
		Public = 0,
		Restricted,
		Executive,
		Operations,
		Service,
		Count
	};

	struct FCryoWorldColorMatSpec
	{
		const TCHAR* Name;
		ECryoWorldColorZone Zone;
		FLinearColor Color;
		float Roughness;
		float Metallic;
		float Emissive;
	};

	const FCryoWorldColorMatSpec CryoWorldColorMats[] = {
		{ TEXT("M_Cryo_Public_ColdLab"), ECryoWorldColorZone::Public,
			FLinearColor(0.55f, 0.72f, 0.88f), 0.30f, 0.05f, 0.06f },
		{ TEXT("M_Cryo_Restricted_Specimen"), ECryoWorldColorZone::Restricted,
			FLinearColor(0.42f, 0.48f, 0.55f), 0.40f, 0.55f, 0.02f },
		{ TEXT("M_Cryo_Executive_Observation"), ECryoWorldColorZone::Executive,
			FLinearColor(0.68f, 0.78f, 0.88f), 0.12f, 0.15f, 0.18f },
		{ TEXT("M_Cryo_Operations_CryoControl"), ECryoWorldColorZone::Operations,
			FLinearColor(0.06f, 0.32f, 0.52f), 0.22f, 0.18f, 1.0f },
		{ TEXT("M_Cryo_Service_Industrial"), ECryoWorldColorZone::Service,
			FLinearColor(0.58f, 0.62f, 0.68f), 0.92f, 0.08f, 0.0f },
	};

	struct FCryoWorldColorPropSpec
	{
		const TCHAR* Label;
		ECryoWorldColorZone Zone;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		bool bPlane;
		bool bText;
		const TCHAR* Text;
	};

	const FCryoWorldColorPropSpec CryoWorldColorProps[] = {
		{ TEXT("Cryo_WC_Sign_ColdLab"), ECryoWorldColorZone::Public,
			FVector(-1200.f, -650.f, -2180.f), FRotator(0.f, 0.f, 0.f), FVector(1.6f, 0.08f, 0.5f), false, true,
			TEXT("CRYO STORAGE — COLD LAB") },
		{ TEXT("Cryo_WC_Sign_Specimen"), ECryoWorldColorZone::Restricted,
			FVector(-2200.f, -2000.f, -2180.f), FRotator(0.f, 30.f, 0.f), FVector(1.4f, 0.08f, 0.45f), false, true,
			TEXT("RESTRICTED — SPECIMEN TANKS") },
		{ TEXT("Cryo_WC_Pod_1"), ECryoWorldColorZone::Restricted,
			FVector(-2450.f, -2000.f, -2280.f), FRotator(0.f, 20.f, 0.f), FVector(0.7f, 0.7f, 1.4f), false, false, nullptr },
		{ TEXT("Cryo_WC_Pod_2"), ECryoWorldColorZone::Restricted,
			FVector(-2100.f, -2300.f, -2280.f), FRotator(0.f, -15.f, 0.f), FVector(0.7f, 0.7f, 1.4f), false, false, nullptr },
		{ TEXT("Cryo_WC_Tank_1"), ECryoWorldColorZone::Restricted,
			FVector(-2500.f, 900.f, -2285.f), FRotator(0.f, 40.f, 0.f), FVector(0.55f, 0.55f, 1.1f), false, false, nullptr },
		{ TEXT("Cryo_WC_Tank_2"), ECryoWorldColorZone::Restricted,
			FVector(-2300.f, 1150.f, -2285.f), FRotator(0.f, -25.f, 0.f), FVector(0.55f, 0.55f, 1.1f), false, false, nullptr },
		{ TEXT("Cryo_WC_ConsentStack"), ECryoWorldColorZone::Operations,
			FVector(-520.f, -1100.f, -2290.f), FRotator(0.f, 10.f, 0.f), FVector(0.45f, 0.35f, 0.25f), false, false, nullptr },
		{ TEXT("Cryo_WC_ManifestDisplay"), ECryoWorldColorZone::Public,
			FVector(-2200.f, -2050.f, -2280.f), FRotator(0.f, 90.f, 0.f), FVector(0.5f, 0.08f, 0.4f), false, false, nullptr },
		{ TEXT("Cryo_WC_FrostDecal_1"), ECryoWorldColorZone::Service,
			FVector(-1200.f, -800.f, -2395.f), FRotator(0.f, 0.f, 0.f), FVector(3.0f, 3.0f, 1.f), true, false, nullptr },
		{ TEXT("Cryo_WC_FrostDecal_2"), ECryoWorldColorZone::Service,
			FVector(-2330.f, -2150.f, -2395.f), FRotator(0.f, 15.f, 0.f), FVector(2.0f, 2.0f, 1.f), true, false, nullptr },
		{ TEXT("Cryo_WC_Observation_Glass"), ECryoWorldColorZone::Executive,
			FVector(-900.f, -400.f, -2200.f), FRotator(0.f, 0.f, 0.f), FVector(0.08f, 2.2f, 1.5f), false, false, nullptr },
		{ TEXT("Cryo_WC_ControlPanel"), ECryoWorldColorZone::Operations,
			FVector(-300.f, -1300.f, -2250.f), FRotator(0.f, -90.f, 0.f), FVector(1.2f, 0.08f, 0.8f), false, false, nullptr },
		{ TEXT("Cryo_WC_Brand_Cryo"), ECryoWorldColorZone::Public,
			FVector(-1200.f, -500.f, -2150.f), FRotator(0.f, 0.f, 0.f), FVector(1.8f, 0.05f, 0.45f), false, true,
			TEXT("EPITOPE — What They Kept Cold") },
	};

	FString CryoWorldColorMatPath(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s.%s"), CryoWorldColorFolder, Name, Name);
	}

	FString CryoWorldColorMatPackage(const TCHAR* Name)
	{
		return FString::Printf(TEXT("%s/%s"), CryoWorldColorFolder, Name);
	}

	const FCryoWorldColorMatSpec* CryoWorldColorFindMat(ECryoWorldColorZone Zone)
	{
		for (const FCryoWorldColorMatSpec& Spec : CryoWorldColorMats)
		{
			if (Spec.Zone == Zone)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool CryoWorldColorColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.05f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.05f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.05f);
	}

	bool CryoWorldColorIsGray(const FLinearColor& Color)
	{
		const float Max = FMath::Max3(Color.R, Color.G, Color.B);
		const float Min = FMath::Min3(Color.R, Color.G, Color.B);
		return (Max - Min) < 0.08f && Max > 0.15f && Max < 0.85f;
	}

	FLinearColor CryoWorldColorReadMaterialColor(const UMaterialInterface* Material)
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

	void CryoWorldColorWireMaterial(UMaterial* Material, const FCryoWorldColorMatSpec& Spec)
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

	bool CryoWorldColorMaterialExact(const UMaterial* Material, const FCryoWorldColorMatSpec& Spec)
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
		if (!ColorExpr || !CryoWorldColorColorNear(ColorExpr->DefaultValue, Spec.Color))
		{
			return false;
		}
		return !CryoWorldColorIsGray(ColorExpr->DefaultValue);
	}

	UMaterial* CryoWorldColorLoadMat(const TCHAR* Name)
	{
		return LoadObject<UMaterial>(nullptr, *CryoWorldColorMatPath(Name));
	}

	bool CryoWorldColorEnsureMaterials(FString& OutError)
	{
		UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
		for (const FCryoWorldColorMatSpec& Spec : CryoWorldColorMats)
		{
			UMaterial* Material = CryoWorldColorLoadMat(Spec.Name);
			if (!Material)
			{
				UPackage* Package = CreatePackage(*CryoWorldColorMatPackage(Spec.Name));
				Material = Cast<UMaterial>(Factory->FactoryCreateNew(
					UMaterial::StaticClass(), Package, Spec.Name, RF_Public | RF_Standalone, nullptr, GWarn));
				if (!Material)
				{
					OutError = FString::Printf(TEXT("Failed to create %s."), Spec.Name);
					return false;
				}
				FAssetRegistryModule::AssetCreated(Material);
			}
			CryoWorldColorWireMaterial(Material, Spec);
			if (!CryoWorldColorMaterialExact(Material, Spec))
			{
				OutError = FString::Printf(TEXT("%s is not a colored Admin material."), Spec.Name);
				return false;
			}
		}
		return true;
	}

	UBlueprint* CryoWorldColorLoadLightBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, CryoWorldColorLightBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, CryoWorldColorLightBpPackage);
		}
		return Blueprint;
	}

	UBlueprint* CryoWorldColorLoadAudioBp()
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, CryoWorldColorAudioBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, CryoWorldColorAudioBpPackage);
		}
		return Blueprint;
	}

	bool CryoWorldColorEnsureBlueprints(FString& OutError)
	{
		if (!CryoWorldColorLoadLightBp())
		{
			UPackage* Package = CreatePackage(CryoWorldColorLightBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				Package,
				TEXT("BP_CryoLightController"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created || !Created->SimpleConstructionScript)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Cryo/BP_CryoLightController.");
				return false;
			}
			USCS_Node* LightNode = Created->SimpleConstructionScript->CreateNode(
				UPointLightComponent::StaticClass(), TEXT("PublicZoneLight"));
			Created->SimpleConstructionScript->AddNode(LightNode);
			if (UPointLightComponent* Template = Cast<UPointLightComponent>(LightNode->ComponentTemplate))
			{
				Template->Intensity = 4200.f;
				Template->SetTemperature(7000.f);
				Template->bUseTemperature = true;
				Template->SetLightColor(FLinearColor(0.55f, 0.72f, 0.88f), true);
				Template->AttenuationRadius = 1400.f;
			}
			struct FCryoZoneLightSpec
			{
				const TCHAR* Name;
				FLinearColor Color;
				float Intensity;
				float Temp;
			};
			const FCryoZoneLightSpec ExtraLights[] = {
				{ TEXT("RestrictedZoneLight"), FLinearColor(0.35f, 0.48f, 0.70f), 2200.f, 9000.f },
				{ TEXT("ExecutiveZoneLight"), FLinearColor(1.0f, 0.92f, 0.82f), 3600.f, 4200.f },
				{ TEXT("OperationsZoneLight"), FLinearColor(0.45f, 0.70f, 1.0f), 5500.f, 6500.f },
				{ TEXT("ServiceZoneLight"), FLinearColor(0.55f, 0.62f, 0.72f), 1800.f, 7500.f },
			};
			for (const FCryoZoneLightSpec& Spec : ExtraLights)
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

		if (!CryoWorldColorLoadAudioBp())
		{
			UClass* Native = StaticLoadClass(UObject::StaticClass(), nullptr, CryoWorldColorAmbienceNative);
			if (!Native)
			{
				OutError = TEXT("ProjectOrganoidAmbienceZone is not loaded.");
				return false;
			}
			UPackage* Package = CreatePackage(CryoWorldColorAudioBpPackage);
			UBlueprint* Created = FKismetEditorUtilities::CreateBlueprint(
				Native,
				Package,
				TEXT("BP_CryoAudioZone"),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			if (!Created)
			{
				OutError = TEXT("Failed to create /Game/Blueprints/Cryo/BP_CryoAudioZone.");
				return false;
			}
			FKismetEditorUtilities::CompileBlueprint(Created);
			FAssetRegistryModule::AssetCreated(Created);
			Package->MarkPackageDirty();
		}

		UBlueprint* LightBp = CryoWorldColorLoadLightBp();
		UBlueprint* AudioBp = CryoWorldColorLoadAudioBp();
		if (!LightBp || !LightBp->GeneratedClass || LightBp->ParentClass != AActor::StaticClass())
		{
			OutError = TEXT("BP_CryoLightController under /Game/Blueprints/Cryo/ is invalid.");
			return false;
		}
		UClass* AmbienceNative = StaticLoadClass(UObject::StaticClass(), nullptr, CryoWorldColorAmbienceNative);
		if (!AudioBp || !AudioBp->GeneratedClass || !AmbienceNative || !AudioBp->GeneratedClass->IsChildOf(AmbienceNative))
		{
			OutError = TEXT("BP_CryoAudioZone under /Game/Blueprints/Cryo/ must parent AmbienceZone.");
			return false;
		}
		return true;
	}

	ECryoWorldColorZone CryoWorldColorClassifyLocation(const FVector& Location)
	{
		const FVector Center(-1200.f, -800.f, -2310.f);
		const FVector Delta = Location - Center;
		// Near Sterling note / north-west tanks → Restricted specimen
		if (Location.X < -2000.f && Location.Y > 0.f)
		{
			return ECryoWorldColorZone::Restricted;
		}
		// Near SpecimenManifest south-west → Restricted
		if (Location.X < -1800.f && Location.Y < -1500.f)
		{
			return ECryoWorldColorZone::Restricted;
		}
		// Near ConsentForms east → Operations control
		if (Location.X > -700.f)
		{
			return ECryoWorldColorZone::Operations;
		}
		if (Delta.Y > 200.f)
		{
			return ECryoWorldColorZone::Executive;
		}
		if (Delta.Y < -200.f || Delta.X < -400.f)
		{
			return ECryoWorldColorZone::Service;
		}
		return ECryoWorldColorZone::Public;
	}

	ECryoWorldColorZone CryoWorldColorClassifyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return ECryoWorldColorZone::Public;
		}
		const FString Label = ActorLabel(const_cast<AActor*>(Actor));
		if (Label.Contains(TEXT("Executive"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Observation"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Glass"), ESearchCase::IgnoreCase))
		{
			return ECryoWorldColorZone::Executive;
		}
		if (Label.Contains(TEXT("Operations"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Control"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Consent"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("CryoControl"), ESearchCase::IgnoreCase))
		{
			return ECryoWorldColorZone::Operations;
		}
		if (Label.Contains(TEXT("Service"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Frost"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pipe"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Vent"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Corridor"), ESearchCase::IgnoreCase))
		{
			return ECryoWorldColorZone::Service;
		}
		if (Label.Contains(TEXT("Specimen"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Restricted"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Tank"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Pod"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Manifest"), ESearchCase::IgnoreCase)
			|| Label.Contains(TEXT("Sterling"), ESearchCase::IgnoreCase))
		{
			return ECryoWorldColorZone::Restricted;
		}
		return CryoWorldColorClassifyLocation(Actor->GetActorLocation());
	}

	bool CryoWorldColorIsProtectedLabel(const FString& Label)
	{
		static const TCHAR* Protected[] = {
			TEXT("DataPad_SpecimenManifest"),
			TEXT("DataPad_ConsentForms"),
			TEXT("DataPad_SterlingCryoNote"),
			TEXT("Terminal_ControlSpine"),
			TEXT("Checkpoint_BasinRim"),
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
			TEXT("Cryo_WorldColor_LightController"),
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

	bool CryoWorldColorIsPaintableMesh(const AActor* Actor)
	{
		const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !PackagesEqual(ActorOwningPackage(const_cast<AStaticMeshActor*>(MeshActor)), CryoPackage))
		{
			return false;
		}
		const FString Label = ActorLabel(const_cast<AStaticMeshActor*>(MeshActor));
		if (CryoWorldColorIsProtectedLabel(Label) || Label.StartsWith(TEXT("Cryo_WC_"), ESearchCase::CaseSensitive))
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
			|| Lower.Contains(TEXT("cryo"));
		const FVector Loc = MeshActor->GetActorLocation();
		const FVector Scale = MeshActor->GetActorScale3D();
		const bool bCeilingShape = Loc.Z > -2150.f && Scale.Z < 0.35f;
		const bool bFloorShape = Loc.Z < -2380.f && Scale.Z < 0.35f;
		UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
		const FLinearColor Slot0 = CryoWorldColorReadMaterialColor(Comp ? Comp->GetMaterial(0) : nullptr);
		const bool bGraySlot = CryoWorldColorIsGray(Slot0)
			|| (Comp && Comp->GetMaterial(0) && Comp->GetMaterial(0)->GetName().Contains(TEXT("WorldGridMaterial")));
		return bNameHit || bCeilingShape || bFloorShape || bGraySlot;
	}

	void CryoWorldColorCollectDirty(TArray<FString>& OutContent, TArray<FString>& OutWorlds)
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

	TArray<FString> CryoWorldColorAllowedPackages()
	{
		TArray<FString> Allowed;
		Allowed.Add(CryoPackage);
		Allowed.Add(CryoWorldColorLightBpPackage);
		Allowed.Add(CryoWorldColorAudioBpPackage);
		for (const FCryoWorldColorMatSpec& Spec : CryoWorldColorMats)
		{
			Allowed.Add(CryoWorldColorMatPackage(Spec.Name));
		}
		return Allowed;
	}

	FString CryoWorldColorUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		CryoWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = CryoWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, CryoPackage) && !WorldsBefore.Contains(Name))
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

	void CryoWorldColorClearUnexpectedDirty(const TArray<FString>& ContentBefore, const TArray<FString>& WorldsBefore)
	{
		const TArray<FString> Allowed = CryoWorldColorAllowedPackages();
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

	FString CryoWorldColorGuardProtected(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		struct FCryoPadGuard
		{
			const TCHAR* Label;
			FVector Location;
		};
		const FCryoPadGuard Pads[] = {
			{ TEXT("DataPad_SpecimenManifest"), FVector(-2330.f, -2150.f, -2310.f) },
			{ TEXT("DataPad_ConsentForms"), FVector(-400.f, -1150.f, -2310.f) },
			{ TEXT("DataPad_SterlingCryoNote"), FVector(-2425.f, 1025.f, -2310.f) },
		};
		for (const FCryoPadGuard& Pad : Pads)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Pad.Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1"), Pad.Label, Matches.Num());
			}
			if (!PackagesEqual(ActorOwningPackage(Matches[0]), CryoPackage))
			{
				return FString::Printf(TEXT("%s is not on Cryo."), Pad.Label);
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
				if (Actor && PackagesEqual(ActorOwningPackage(Actor), CryoPackage))
				{
					return FString::Printf(TEXT("%s must not live on Cryo for this pass."), Label);
				}
			}
		}
		return FString();
	}

	int32 CryoWorldColorCountProps(UWorld* World)
	{
		int32 Count = 0;
		for (const FCryoWorldColorPropSpec& Spec : CryoWorldColorProps)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && PackagesEqual(ActorOwningPackage(Matches[0]), CryoPackage))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CryoWorldColorPaintMeshes(UWorld* World, int32& OutCeilingPainted, int32& OutGrayRemaining)
	{
		OutCeilingPainted = 0;
		OutGrayRemaining = 0;
		int32 Painted = 0;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!CryoWorldColorIsPaintableMesh(MeshActor))
			{
				continue;
			}
			UStaticMeshComponent* Comp = MeshActor->GetStaticMeshComponent();
			const ECryoWorldColorZone Zone = CryoWorldColorClassifyActor(MeshActor);
			const FCryoWorldColorMatSpec* MatSpec = CryoWorldColorFindMat(Zone);
			UMaterial* Material = MatSpec ? CryoWorldColorLoadMat(MatSpec->Name) : nullptr;
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
				const FLinearColor CurrentColor = CryoWorldColorReadMaterialColor(Current);
				if (Current && !CryoWorldColorIsGray(CurrentColor) && Current->GetPathName().Contains(TEXT("/Game/Materials/Cryo/")))
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
				if (Label.Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -2150.f)
				{
					++OutCeilingPainted;
				}
			}
			const FLinearColor After = CryoWorldColorReadMaterialColor(Comp->GetMaterial(0));
			if (CryoWorldColorIsGray(After))
			{
				++OutGrayRemaining;
			}
		}
		return Painted;
	}

	FString CryoWorldColorApplyProp(AActor* Actor, const FCryoWorldColorPropSpec& Spec)
	{
		AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor);
		if (!MeshActor || !MeshActor->GetStaticMeshComponent())
		{
			return TEXT("StaticMeshActor missing.");
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Spec.bPlane ? CryoWorldColorPlanePath : CryoWorldColorCubePath);
		if (!Mesh)
		{
			return TEXT("Failed to load basic shape mesh.");
		}
		MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		const FCryoWorldColorMatSpec* MatSpec = CryoWorldColorFindMat(Spec.Zone);
		UMaterial* Material = MatSpec ? CryoWorldColorLoadMat(MatSpec->Name) : nullptr;
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

	TSharedRef<FJsonObject> CmdInspectCryoWorldColor(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("cryo_package"), CryoPackage);
		Data->SetBoolField(TEXT("cryo_loaded"), World && FindLoadedLevelByPackage(World, CryoPackage) != nullptr);
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
		for (const FCryoWorldColorMatSpec& Spec : CryoWorldColorMats)
		{
			UMaterial* Material = CryoWorldColorLoadMat(Spec.Name);
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("name"), Spec.Name);
			Row->SetBoolField(TEXT("exists"), Material != nullptr);
			const bool bExact = CryoWorldColorMaterialExact(Material, Spec);
			Row->SetBoolField(TEXT("exact"), bExact);
			if (bExact)
			{
				++MatExact;
			}
			Mats.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("materials"), Mats);
		Data->SetNumberField(TEXT("materials_exact"), MatExact);
		Data->SetBoolField(TEXT("light_bp"), CryoWorldColorLoadLightBp() != nullptr);
		Data->SetBoolField(TEXT("audio_bp"), CryoWorldColorLoadAudioBp() != nullptr);
		Data->SetNumberField(TEXT("prop_count"), World ? CryoWorldColorCountProps(World) : 0);
		Data->SetNumberField(TEXT("prop_expected"), UE_ARRAY_COUNT(CryoWorldColorProps));
		TArray<AActor*> LegacyLights = World ? FindByExactLabel(World, LegacyCryoLightControllerLabel) : TArray<AActor*>();
		Data->SetNumberField(TEXT("legacy_light_controller_count"), LegacyLights.Num());
		if (LegacyLights.Num() == 1)
		{
			Data->SetArrayField(TEXT("legacy_light_controller_location"), Vec(LegacyLights[0]->GetActorLocation()));
		}
		TArray<AActor*> WorldColorLights = World ? FindByExactLabel(World, CryoWorldColorLightLabel) : TArray<AActor*>();
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
				if (!CryoWorldColorIsPaintableMesh(MeshActor))
				{
					continue;
				}
				++Paintable;
				UMaterialInterface* Mat = MeshActor->GetStaticMeshComponent()->GetMaterial(0);
				const FLinearColor Color = CryoWorldColorReadMaterialColor(Mat);
				const bool bGray = CryoWorldColorIsGray(Color);
				const FString Label = ActorLabel(MeshActor);
				const bool bCeiling = Label.ToLower().Contains(TEXT("ceiling")) || MeshActor->GetActorLocation().Z > -2150.f;
				if (bCeiling && bGray)
				{
					++GrayCeiling;
				}
				if (bCeiling && Mat && Mat->GetPathName().Contains(TEXT("/Game/Materials/Cryo/")))
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
		USoundWave* Alarm = LoadObject<USoundWave>(nullptr, CryoWorldColorAlarmPath);
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
			TEXT("World-color light controller at Cryo center (-1200,-800,-2310). Datapads unmoved."));
		return Ok(Data);
	}

	FString PreflightCreateCryoWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(CryoWorldColorSpec))
		{
			return TEXT("spec must be cryo_world_color_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. create_cryo_world_color does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		UWorld* World = GetEditorWorld();
		if (!World || !FindLoadedLevelByPackage(World, CryoPackage))
		{
			return TEXT("SL_Epitope_Cryo must be loaded.");
		}
		const FString GuardError = CryoWorldColorGuardProtected(World);
		if (!GuardError.IsEmpty())
		{
			return GuardError;
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		CryoWorldColorCollectDirty(Content, Worlds);
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, CryoPackage))
			{
				return FString::Printf(TEXT("Refusing create while non-Cryo world is dirty: %s"), *Name);
			}
		}
		Before->SetNumberField(TEXT("prop_count"), CryoWorldColorCountProps(World));
		Before->SetNumberField(TEXT("materials_exact"), 0);
		Proposed->SetStringField(TEXT("result"), TEXT("Create Cryo world-color materials, Blueprints, props, and paint. Does not save."));
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetArrayField(TEXT("light_location"), Vec(CryoWorldColorLightLocation));
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateCryoWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_cryo_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateCryoWorldColor(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;

		UWorld* World = GetEditorWorld();
		ULevel* CryoLevel = World ? FindLoadedLevelByPackage(World, CryoPackage) : nullptr;
		if (!World || !CryoLevel)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("not_found"), TEXT("Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		CryoWorldColorCollectDirty(ContentBefore, WorldsBefore);

		FString EnsureError;
		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "CreateCryoWorldColor", "Create Cryo World Color"));
			if (!CryoWorldColorEnsureMaterials(EnsureError) || !CryoWorldColorEnsureBlueprints(EnsureError))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("create_failed"), EnsureError, MakeShared<FBridgeChange>(Change));
			}

			UBlueprint* LightBp = CryoWorldColorLoadLightBp();
			if (FindByExactLabel(World, CryoWorldColorLightLabel).Num() == 0 && LightBp && LightBp->GeneratedClass)
			{
				FActorSpawnParameters Params;
				Params.OverrideLevel = CryoLevel;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AActor* Spawned = World->SpawnActor<AActor>(
					LightBp->GeneratedClass,
					CryoWorldColorLightLocation,
					CryoWorldColorLightRotation,
					Params);
				if (!Spawned)
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("spawn_failed"), TEXT("Failed to spawn Cryo_WorldColor_LightController."), MakeShared<FBridgeChange>(Change));
				}
				Spawned->SetActorScale3D(CryoWorldColorLightScale);
#if WITH_EDITOR
				Spawned->SetActorLabel(CryoWorldColorLightLabel);
#endif
				Spawned->Tags.AddUnique(FName(TEXT("Cryo_WorldColor")));
				Spawned->Tags.AddUnique(FName(TEXT("Cryo_WorldColor_Light")));
			}

			for (const FCryoWorldColorPropSpec& Spec : CryoWorldColorProps)
			{
				if (FindByExactLabel(World, Spec.Label).Num() != 0)
				{
					continue;
				}
				FActorSpawnParameters Params;
				Params.OverrideLevel = CryoLevel;
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
				Spawned->Tags.AddUnique(FName(TEXT("Cryo_WorldColor")));
				const FString PropError = CryoWorldColorApplyProp(Spawned, Spec);
				if (!PropError.IsEmpty())
				{
					Change.Status = TEXT("execute_failed");
					return FailAudit(TEXT("configure_failed"), PropError, MakeShared<FBridgeChange>(Change));
				}
			}

			int32 CeilingPainted = 0;
			int32 GrayRemaining = 0;
			const int32 Painted = CryoWorldColorPaintMeshes(World, CeilingPainted, GrayRemaining);
			CryoWorldColorClearUnexpectedDirty(ContentBefore, WorldsBefore);

			const FString DirtyError = CryoWorldColorUnexpectedDirty(ContentBefore, WorldsBefore);
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
			Change.After->SetNumberField(TEXT("materials"), UE_ARRAY_COUNT(CryoWorldColorMats));
			Change.After->SetNumberField(TEXT("blueprints"), 2);
			Change.After->SetNumberField(TEXT("props"), CryoWorldColorCountProps(World));
			Change.After->SetNumberField(TEXT("meshes_painted"), Painted);
			Change.After->SetNumberField(TEXT("ceilings_painted"), CeilingPainted);
			Change.After->SetNumberField(TEXT("gray_remaining"), GrayRemaining);
			Change.After->SetArrayField(TEXT("light_location"), Vec(CryoWorldColorLightLocation));
			Change.After->SetStringField(TEXT("light_label"), CryoWorldColorLightLabel);
			Change.After->SetBoolField(TEXT("legacy_light_controller_kept"), FindByExactLabel(World, LegacyCryoLightControllerLabel).Num() == 1);
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
			USoundWave* Alarm = LoadObject<USoundWave>(nullptr, CryoWorldColorAlarmPath);
			Change.After->SetBoolField(TEXT("alarm_looping"), Alarm ? Alarm->bLooping : true);
			Change.After->SetStringField(TEXT("note"),
				TEXT("Created /Game/Materials/Cryo (5) + /Game/Blueprints/Cryo (2). Cryo datapads kept; power preserved. Props>=10. No save."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
	}

	FString PreflightSaveCryoWorldColor(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(CryoWorldColorSaveSpec))
		{
			return TEXT("spec must be cryo_world_color_save_v1.");
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
		if (!World || !FindLoadedLevelByPackage(World, CryoPackage))
		{
			return TEXT("SL_Epitope_Cryo must be loaded.");
		}
		for (const FCryoWorldColorMatSpec& Spec : CryoWorldColorMats)
		{
			if (!CryoWorldColorMaterialExact(CryoWorldColorLoadMat(Spec.Name), Spec))
			{
				return FString::Printf(TEXT("%s is missing or gray. Refusing save."), Spec.Name);
			}
		}
		if (!CryoWorldColorLoadLightBp() || !CryoWorldColorLoadAudioBp())
		{
			return TEXT("World-color Blueprints are missing. Refusing save.");
		}
		if (CryoWorldColorCountProps(World) < 10)
		{
			return TEXT("Fewer than 10 Cryo_WC_ props. Refusing save.");
		}
		if (FindByExactLabel(World, CryoWorldColorLightLabel).Num() != 1)
		{
			return TEXT("Cryo_WorldColor_LightController count must be 1.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		CryoWorldColorCollectDirty(Content, Worlds);
		const TArray<FString> Allowed = CryoWorldColorAllowedPackages();
		for (const FString& Name : Worlds)
		{
			if (!PackagesEqual(Name, CryoPackage))
			{
				return FString::Printf(TEXT("Refusing save while non-Cryo world is dirty: %s"), *Name);
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
		Proposed->SetBoolField(TEXT("saves_cryo_map"), true);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteSaveCryoWorldColor(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_cryo_world_color must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveCryoWorldColor(Change.Args, Before, Proposed);
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
