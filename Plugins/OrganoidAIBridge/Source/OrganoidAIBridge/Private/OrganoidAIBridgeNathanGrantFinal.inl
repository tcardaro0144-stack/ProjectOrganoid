// Drake look for Nathan Grant. Duplicates SKM_NathanGrant into SKM_NathanGrant_Final and
// paints separate matte material slots. The tinted Manny mesh stays in place.
	const TCHAR* NathanFinalSpec = TEXT("nathan_grant_final_v1");
	const TCHAR* NathanFinalAction = TEXT("create_nathan_grant_final");
	const TCHAR* NathanFinalSaveSpec = TEXT("nathan_grant_final_save_v1");
	const TCHAR* NathanFinalSaveAction = TEXT("save_nathan_grant_final");
	const TCHAR* NathanFinalFolder = TEXT("/Game/Characters/Nathan");
	const TCHAR* NathanFinalSourcePath = TEXT("/Game/Characters/Nathan/SKM_NathanGrant.SKM_NathanGrant");
	const TCHAR* NathanFinalMeshPath = TEXT("/Game/Characters/Nathan/SKM_NathanGrant_Final.SKM_NathanGrant_Final");
	const TCHAR* NathanFinalMeshPackage = TEXT("/Game/Characters/Nathan/SKM_NathanGrant_Final");
	const TCHAR* NathanFinalParentPath = TEXT("/Game/Characters/Nathan/M_NathanGrant_Solid.M_NathanGrant_Solid");
	const TCHAR* NathanFinalParentPackage = TEXT("/Game/Characters/Nathan/M_NathanGrant_Solid");
	const int32 NathanFinalSlotCount = 13;

	struct FNathanFinalSlotSpec
	{
		const TCHAR* Slot;
		const TCHAR* Asset;
		FLinearColor Color;
		float Roughness;
		float Emissive;
	};

	const FNathanFinalSlotSpec NathanFinalSlots[] = {
		{ TEXT("Head"), TEXT("MI_NathanGrant_Head"), FLinearColor(0.73f, 0.54f, 0.42f), 0.58f, 0.0f },
		{ TEXT("Hair"), TEXT("MI_Hair_Brown"), FLinearColor(0.25f, 0.12f, 0.05f), 0.78f, 0.0f },
		{ TEXT("Stubble"), TEXT("MI_Stubble"), FLinearColor(0.42f, 0.30f, 0.22f), 0.84f, 0.0f },
		{ TEXT("Eyes"), TEXT("MI_Eyes_Blue"), FLinearColor(0.22f, 0.45f, 0.72f), 0.12f, 0.45f },
		{ TEXT("Jacket"), TEXT("MI_Jacket_Dark"), FLinearColor(0.045f, 0.048f, 0.052f), 0.78f, 0.0f },
		{ TEXT("Henley"), TEXT("MI_Henley_Charcoal"), FLinearColor(0.15f, 0.15f, 0.155f), 0.88f, 0.0f },
		{ TEXT("Arms"), TEXT("MI_Arms_Dark"), FLinearColor(0.07f, 0.075f, 0.08f), 0.74f, 0.0f },
		{ TEXT("Hands"), TEXT("MI_Hands_Skin"), FLinearColor(0.76f, 0.57f, 0.45f), 0.55f, 0.0f },
		{ TEXT("Cargo"), TEXT("MI_Cargo_Olive"), FLinearColor(0.34f, 0.36f, 0.20f), 0.82f, 0.0f },
		{ TEXT("Boots"), TEXT("MI_Boots"), FLinearColor(0.08f, 0.06f, 0.045f), 0.52f, 0.0f },
		{ TEXT("Holster"), TEXT("MI_Holster"), FLinearColor(0.24f, 0.14f, 0.07f), 0.62f, 0.0f },
		{ TEXT("Pack"), TEXT("MI_Pack"), FLinearColor(0.24f, 0.26f, 0.18f), 0.76f, 0.0f },
		{ TEXT("Badge"), TEXT("MI_Badge"), FLinearColor(0.90f, 0.88f, 0.75f), 0.40f, 0.05f },
	};

	enum class ENathanFinalSlot : int32
	{
		Head = 0,
		Hair,
		Stubble,
		Eyes,
		Jacket,
		Henley,
		Arms,
		Hands,
		Cargo,
		Boots,
		Holster,
		Pack,
		Badge
	};

	FString NathanFinalMiPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), NathanFinalFolder, AssetName, AssetName);
	}

	FString NathanFinalMiPackage(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s"), NathanFinalFolder, AssetName);
	}

	bool NathanFinalColorNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.04f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.04f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.04f);
	}

	FLinearColor NathanFinalReadColor(const UMaterialInstanceConstant* Material)
	{
		if (!Material)
		{
			return FLinearColor::Black;
		}
		for (const FVectorParameterValue& Value : Material->VectorParameterValues)
		{
			if (Value.ParameterInfo.Name == TEXT("Color"))
			{
				return Value.ParameterValue;
			}
		}
		return FLinearColor::Black;
	}

	USkeletalMesh* NathanFinalLoadSource()
	{
		return LoadObject<USkeletalMesh>(nullptr, NathanFinalSourcePath);
	}

	USkeletalMesh* NathanFinalLoadMesh()
	{
		return LoadObject<USkeletalMesh>(nullptr, NathanFinalMeshPath);
	}

	bool NathanFinalMeshExact(USkeletalMesh* Mesh)
	{
		if (!Mesh || !Mesh->GetSkeleton())
		{
			return false;
		}
		const TArray<FSkeletalMaterial>& Materials = Mesh->GetMaterials();
		if (Materials.Num() < NathanFinalSlotCount)
		{
			return false;
		}
		for (const FNathanFinalSlotSpec& Spec : NathanFinalSlots)
		{
			const UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *NathanFinalMiPath(Spec.Asset));
			if (!Instance || !NathanFinalColorNear(NathanFinalReadColor(Instance), Spec.Color))
			{
				return false;
			}
			bool bSlot = false;
			for (const FSkeletalMaterial& Slot : Materials)
			{
				if (Slot.MaterialSlotName == Spec.Slot && Slot.MaterialInterface == Instance)
				{
					bSlot = true;
					break;
				}
			}
			if (!bSlot)
			{
				return false;
			}
		}
		return true;
	}

	void NathanFinalWireParent(UMaterial* Material)
	{
		UMaterialEditorOnlyData* EditorOnly = Material->GetEditorOnlyData();
		if (!EditorOnly || EditorOnly->BaseColor.Expression)
		{
			return;
		}
		UMaterialExpressionVectorParameter* Color = NewObject<UMaterialExpressionVectorParameter>(Material);
		Color->ParameterName = TEXT("Color");
		Color->DefaultValue = FLinearColor::White;
		Color->MaterialExpressionEditorX = -420;
		Color->MaterialExpressionEditorY = 0;
		UMaterialExpressionScalarParameter* Roughness = NewObject<UMaterialExpressionScalarParameter>(Material);
		Roughness->ParameterName = TEXT("Roughness");
		Roughness->DefaultValue = 0.7f;
		Roughness->MaterialExpressionEditorX = -420;
		Roughness->MaterialExpressionEditorY = 180;
		UMaterialExpressionScalarParameter* Metallic = NewObject<UMaterialExpressionScalarParameter>(Material);
		Metallic->ParameterName = TEXT("Metallic");
		Metallic->DefaultValue = 0.0f;
		Metallic->MaterialExpressionEditorX = -420;
		Metallic->MaterialExpressionEditorY = 340;
		UMaterialExpressionScalarParameter* Emissive = NewObject<UMaterialExpressionScalarParameter>(Material);
		Emissive->ParameterName = TEXT("Emissive");
		Emissive->DefaultValue = 0.0f;
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
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->SetMaterialUsage(MATUSAGE_SkeletalMesh);
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	void NathanFinalApplyInstance(UMaterialInstanceConstant* Instance, const FNathanFinalSlotSpec& Spec)
	{
		Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Color")), Spec.Color);
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Roughness")), Spec.Roughness);
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Metallic")), 0.0f);
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Emissive")), Spec.Emissive);
		Instance->PostEditChange();
		Instance->MarkPackageDirty();
	}

	bool NathanFinalEnsureMaterials(IAssetTools& AssetTools, FString& OutError)
	{
		UMaterial* Parent = LoadObject<UMaterial>(nullptr, NathanFinalParentPath);
		if (!Parent)
		{
			UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
			Parent = Cast<UMaterial>(AssetTools.CreateAsset(TEXT("M_NathanGrant_Solid"), NathanFinalFolder, UMaterial::StaticClass(), Factory));
		}
		if (!Parent)
		{
			OutError = TEXT("M_NathanGrant_Solid was not created.");
			return false;
		}
		NathanFinalWireParent(Parent);
		if (!Parent->GetEditorOnlyData() || !Parent->GetEditorOnlyData()->BaseColor.Expression)
		{
			OutError = TEXT("M_NathanGrant_Solid has no base color.");
			return false;
		}
		for (const FNathanFinalSlotSpec& Spec : NathanFinalSlots)
		{
			UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *NathanFinalMiPath(Spec.Asset));
			if (!Instance)
			{
				UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
				Factory->InitialParent = Parent;
				Instance = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(Spec.Asset, NathanFinalFolder, UMaterialInstanceConstant::StaticClass(), Factory));
			}
			if (!Instance)
			{
				OutError = FString::Printf(TEXT("%s was not created."), Spec.Asset);
				return false;
			}
			if (Instance->Parent != Parent)
			{
				Instance->SetParentEditorOnly(Parent);
			}
			NathanFinalApplyInstance(Instance, Spec);
		}
		return true;
	}

	FString NathanFinalBoneToken(const FName& BoneName)
	{
		return BoneName.ToString().ToLower();
	}

	template<typename TWeights>
	int32 NathanFinalDominantBone(const TWeights& Weights)
	{
		float Best = -1.0f;
		int32 Index = INDEX_NONE;
		for (const UE::AnimationCore::FBoneWeight& Weight : Weights)
		{
			if (Weight.GetWeight() > Best)
			{
				Best = Weight.GetWeight();
				Index = Weight.GetBoneIndex();
			}
		}
		return Index;
	}

	ENathanFinalSlot NathanFinalSlotFromBone(const FString& Bone)
	{
		if (Bone.Contains(TEXT("foot")) || Bone.Contains(TEXT("ball")) || Bone.Contains(TEXT("ankle")))
		{
			return ENathanFinalSlot::Boots;
		}
		if (Bone.Contains(TEXT("hand")) || Bone.Contains(TEXT("thumb")) || Bone.Contains(TEXT("index"))
			|| Bone.Contains(TEXT("middle")) || Bone.Contains(TEXT("ring")) || Bone.Contains(TEXT("pinky"))
			|| Bone.Contains(TEXT("wrist")))
		{
			return ENathanFinalSlot::Hands;
		}
		if (Bone.Contains(TEXT("upperarm")) || Bone.Contains(TEXT("lowerarm")) || Bone.Contains(TEXT("forearm")))
		{
			return ENathanFinalSlot::Arms;
		}
		if (Bone.Contains(TEXT("thigh")) || Bone.Contains(TEXT("calf")))
		{
			return ENathanFinalSlot::Cargo;
		}
		if (Bone.Contains(TEXT("head")))
		{
			return ENathanFinalSlot::Head;
		}
		return ENathanFinalSlot::Jacket;
	}

	bool NathanFinalPartition(USkeletalMesh* Mesh, TArray<int32>& OutCounts, FString& OutMode, FString& OutError)
	{
		OutCounts.Init(0, NathanFinalSlotCount);
		if (!Mesh->HasMeshDescription(0))
		{
			OutError = TEXT("SKM_NathanGrant_Final has no LOD0 mesh description.");
			return false;
		}
		FMeshDescription* Desc = Mesh->GetMeshDescription(0);
		if (!Desc)
		{
			OutError = TEXT("LOD0 mesh description did not load.");
			return false;
		}
		FSkeletalMeshAttributes Attributes(*Desc);
		auto Skin = Attributes.GetVertexSkinWeights();
		auto BoneNames = Attributes.GetBoneNames();
		const int32 NumBones = BoneNames.GetNumElements();
		TArray<FString> Bones;
		Bones.Reserve(NumBones);
		for (int32 Index = 0; Index < NumBones; ++Index)
		{
			Bones.Add(NathanFinalBoneToken(BoneNames.Get(FBoneID(Index))));
		}
		const bool bHasNormals = Desc->VertexInstanceAttributes().HasAttribute(MeshAttribute::VertexInstance::Normal);
		TOptional<TVertexInstanceAttributesRef<FVector3f>> Normals;
		if (bHasNormals)
		{
			Normals.Emplace(Desc->VertexInstanceAttributes().GetAttributesRef<FVector3f>(MeshAttribute::VertexInstance::Normal));
		}

		struct FPolyInfo
		{
			FPolygonID Id;
			FVector3f Center = FVector3f::ZeroVector;
			FVector3f Normal = FVector3f::ZeroVector;
			int32 Bone = INDEX_NONE;
		};
		TArray<FPolyInfo> Polys;
		Polys.Reserve(Desc->Polygons().Num());
		bool bAnyHead = false;
		for (const FPolygonID PolyID : Desc->Polygons().GetElementIDs())
		{
			FPolyInfo Info;
			Info.Id = PolyID;
			const TArray<FVertexInstanceID> Instances = Desc->GetPolygonVertexInstances(PolyID);
			if (Instances.Num() == 0)
			{
				continue;
			}
			FVector3f NormalSum = FVector3f::ZeroVector;
			for (const FVertexInstanceID InstanceID : Instances)
			{
				const FVertexID VertexID = Desc->GetVertexInstanceVertex(InstanceID);
				Info.Center += Desc->GetVertexPosition(VertexID);
				if (Normals.IsSet())
				{
					NormalSum += Normals.GetValue().Get(InstanceID);
				}
			}
			Info.Center /= static_cast<float>(Instances.Num());
			Info.Normal = NormalSum.GetSafeNormal();
			const FVertexID FirstVertex = Desc->GetVertexInstanceVertex(Instances[0]);
			const int32 BoneIndex = NathanFinalDominantBone(Skin.Get(FirstVertex));
			Info.Bone = BoneIndex;
			if (Bones.IsValidIndex(BoneIndex) && Bones[BoneIndex].Contains(TEXT("head")))
			{
				bAnyHead = true;
			}
			Polys.Add(Info);
		}
		if (Polys.Num() == 0)
		{
			OutError = TEXT("LOD0 has no polygons.");
			return false;
		}

		FVector3f MinC(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector3f MaxC(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		FVector3f HeadMin(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector3f HeadMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		FVector3f Forward = FVector3f::ZeroVector;
		int32 HeadSamples = 0;
		for (const FPolyInfo& Poly : Polys)
		{
			MinC = MinC.ComponentMin(Poly.Center);
			MaxC = MaxC.ComponentMax(Poly.Center);
			const bool bHead = bAnyHead && Bones.IsValidIndex(Poly.Bone) && Bones[Poly.Bone].Contains(TEXT("head"));
			if (bHead)
			{
				HeadMin = HeadMin.ComponentMin(Poly.Center);
				HeadMax = HeadMax.ComponentMax(Poly.Center);
				Forward += Poly.Normal;
				++HeadSamples;
			}
		}
		Forward = Forward.GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			Forward = FVector3f(0.0f, 1.0f, 0.0f);
		}
		const FVector3f Right = FVector3f::CrossProduct(FVector3f::UpVector, Forward).GetSafeNormal();
		const FVector3f BoundsMin = bAnyHead ? HeadMin : MinC;
		const FVector3f BoundsMax = bAnyHead ? HeadMax : MaxC;
		const float HeadSpanZ = FMath::Max(BoundsMax.Z - BoundsMin.Z, 1.0f);
		const float BodySpanZ = FMath::Max(MaxC.Z - MinC.Z, 1.0f);

		TArray<ENathanFinalSlot> Assigned;
		Assigned.SetNum(Polys.Num());
		for (int32 Index = 0; Index < Polys.Num(); ++Index)
		{
			const FPolyInfo& Poly = Polys[Index];
			const FString Bone = Bones.IsValidIndex(Poly.Bone) ? Bones[Poly.Bone] : FString();
			ENathanFinalSlot Slot = bAnyHead ? NathanFinalSlotFromBone(Bone) : ENathanFinalSlot::Jacket;
			const float BodyT = (Poly.Center.Z - MinC.Z) / BodySpanZ;
			const float HeadT = (Poly.Center.Z - BoundsMin.Z) / HeadSpanZ;
			const float Facing = FVector3f::DotProduct(Poly.Normal, Forward);
			const float Lateral = FVector3f::DotProduct(Poly.Center - ((BoundsMin + BoundsMax) * 0.5f), Right);
			if (!bAnyHead)
			{
				if (BodyT > 0.90f)
				{
					Slot = ENathanFinalSlot::Hair;
				}
				else if (BodyT > 0.82f)
				{
					Slot = Facing > 0.2f && FMath::Abs(Lateral) > 2.0f ? ENathanFinalSlot::Eyes : ENathanFinalSlot::Head;
				}
				else if (BodyT > 0.76f)
				{
					Slot = Facing > 0.0f ? ENathanFinalSlot::Stubble : ENathanFinalSlot::Head;
				}
				else if (BodyT > 0.58f)
				{
					Slot = Facing < -0.25f ? ENathanFinalSlot::Pack : (Facing > 0.2f ? ENathanFinalSlot::Henley : ENathanFinalSlot::Jacket);
				}
				else if (BodyT > 0.42f)
				{
					Slot = ENathanFinalSlot::Jacket;
				}
				else if (BodyT > 0.16f)
				{
					Slot = Lateral > 8.0f ? ENathanFinalSlot::Holster : ENathanFinalSlot::Cargo;
				}
				else
				{
					Slot = ENathanFinalSlot::Boots;
				}
			}
			else if (Slot == ENathanFinalSlot::Head)
			{
				if (HeadT > 0.72f)
				{
					Slot = ENathanFinalSlot::Hair;
				}
				else if (HeadT > 0.42f && HeadT < 0.68f && Facing > 0.35f && FMath::Abs(Lateral) > 1.5f && FMath::Abs(Lateral) < 8.0f)
				{
					Slot = ENathanFinalSlot::Eyes;
				}
				else if (HeadT < 0.30f && Facing > -0.1f)
				{
					Slot = ENathanFinalSlot::Stubble;
				}
			}
			else if (Slot == ENathanFinalSlot::Jacket)
			{
				if (Facing < -0.30f && BodyT > 0.55f)
				{
					Slot = ENathanFinalSlot::Pack;
				}
				else if (Facing > 0.45f && FMath::Abs(Lateral) < 4.0f && BodyT > 0.58f && BodyT < 0.72f)
				{
					Slot = ENathanFinalSlot::Badge;
				}
				else if (Facing > 0.15f && BodyT > 0.52f)
				{
					Slot = ENathanFinalSlot::Henley;
				}
			}
			else if (Slot == ENathanFinalSlot::Cargo && Bone.Contains(TEXT("thigh_r")) && Lateral > 4.0f)
			{
				Slot = ENathanFinalSlot::Holster;
			}
			Assigned[Index] = Slot;
		}

		auto CountOf = [&Assigned](ENathanFinalSlot Slot)
		{
			int32 Count = 0;
			for (const ENathanFinalSlot Value : Assigned)
			{
				Count += Value == Slot ? 1 : 0;
			}
			return Count;
		};
		auto Steal = [&Assigned, &Polys](ENathanFinalSlot Dest, ENathanFinalSlot Source, const FVector3f& Target, int32 Need)
		{
			struct FRank
			{
				int32 Index;
				float Dist;
			};
			TArray<FRank> Ranks;
			for (int32 Index = 0; Index < Assigned.Num(); ++Index)
			{
				if (Assigned[Index] == Source)
				{
					Ranks.Add({ Index, FVector3f::DistSquared(Polys[Index].Center, Target) });
				}
			}
			Ranks.Sort([](const FRank& A, const FRank& B) { return A.Dist < B.Dist; });
			const int32 Give = FMath::Min(Need, Ranks.Num());
			for (int32 Index = 0; Index < Give; ++Index)
			{
				Assigned[Ranks[Index].Index] = Dest;
			}
		};
		const FVector3f HeadCenter = (BoundsMin + BoundsMax) * 0.5f;
		if (CountOf(ENathanFinalSlot::Hair) == 0)
		{
			Steal(ENathanFinalSlot::Hair, ENathanFinalSlot::Head, HeadCenter + FVector3f(0, 0, HeadSpanZ), 40);
		}
		if (CountOf(ENathanFinalSlot::Eyes) == 0)
		{
			Steal(ENathanFinalSlot::Eyes, ENathanFinalSlot::Head, HeadCenter + Forward * 6.0f + Right * 3.0f, 16);
			Steal(ENathanFinalSlot::Eyes, ENathanFinalSlot::Head, HeadCenter + Forward * 6.0f - Right * 3.0f, 16);
		}
		if (CountOf(ENathanFinalSlot::Stubble) == 0)
		{
			Steal(ENathanFinalSlot::Stubble, ENathanFinalSlot::Head, HeadCenter - FVector3f(0, 0, HeadSpanZ * 0.35f), 30);
		}
		if (CountOf(ENathanFinalSlot::Head) == 0)
		{
			Steal(ENathanFinalSlot::Head, ENathanFinalSlot::Hair, HeadCenter, 20);
		}
		if (CountOf(ENathanFinalSlot::Badge) == 0)
		{
			Steal(ENathanFinalSlot::Badge, ENathanFinalSlot::Henley, HeadCenter - FVector3f(0, 0, 18.0f) + Forward * 8.0f, 18);
			if (CountOf(ENathanFinalSlot::Badge) == 0)
			{
				Steal(ENathanFinalSlot::Badge, ENathanFinalSlot::Jacket, HeadCenter - FVector3f(0, 0, 22.0f) + Forward * 8.0f, 18);
			}
		}
		if (CountOf(ENathanFinalSlot::Henley) == 0)
		{
			Steal(ENathanFinalSlot::Henley, ENathanFinalSlot::Jacket, HeadCenter - FVector3f(0, 0, 24.0f), 40);
		}
		if (CountOf(ENathanFinalSlot::Pack) == 0)
		{
			Steal(ENathanFinalSlot::Pack, ENathanFinalSlot::Jacket, HeadCenter - Forward * 12.0f, 30);
		}
		if (CountOf(ENathanFinalSlot::Holster) == 0)
		{
			Steal(ENathanFinalSlot::Holster, ENathanFinalSlot::Cargo, MinC + FVector3f(0, 0, BodySpanZ * 0.35f) + Right * 10.0f, 24);
		}
		if (CountOf(ENathanFinalSlot::Arms) == 0)
		{
			Steal(ENathanFinalSlot::Arms, ENathanFinalSlot::Jacket, HeadCenter + Right * 16.0f, 30);
		}
		if (CountOf(ENathanFinalSlot::Hands) == 0)
		{
			Steal(ENathanFinalSlot::Hands, ENathanFinalSlot::Arms, HeadCenter + Right * 24.0f - FVector3f(0, 0, 10.0f), 20);
		}
		if (CountOf(ENathanFinalSlot::Cargo) == 0)
		{
			Steal(ENathanFinalSlot::Cargo, ENathanFinalSlot::Jacket, MinC + FVector3f(0, 0, BodySpanZ * 0.3f), 40);
		}
		if (CountOf(ENathanFinalSlot::Boots) == 0)
		{
			Steal(ENathanFinalSlot::Boots, ENathanFinalSlot::Cargo, MinC, 24);
		}

		for (int32 Slot = 0; Slot < NathanFinalSlotCount; ++Slot)
		{
			OutCounts[Slot] = CountOf(static_cast<ENathanFinalSlot>(Slot));
			if (OutCounts[Slot] <= 0)
			{
				OutError = FString::Printf(TEXT("Slot %s received no triangles."), NathanFinalSlots[Slot].Slot);
				return false;
			}
		}

		Mesh->ModifyMeshDescription(0);
		TArray<FPolygonGroupID> Groups;
		for (const FPolygonGroupID GroupID : Desc->PolygonGroups().GetElementIDs())
		{
			Groups.Add(GroupID);
		}
		Groups.Sort([](const FPolygonGroupID& A, const FPolygonGroupID& B) { return A.GetValue() < B.GetValue(); });
		while (Groups.Num() < NathanFinalSlotCount)
		{
			Groups.Add(Desc->CreatePolygonGroup());
		}
		const FName SlotAttr = MeshAttribute::PolygonGroup::ImportedMaterialSlotName;
		if (!Desc->PolygonGroupAttributes().HasAttribute(SlotAttr))
		{
			Desc->PolygonGroupAttributes().RegisterAttribute<FName>(SlotAttr, 1, NAME_None);
		}
		TPolygonGroupAttributesRef<FName> SlotNames = Desc->PolygonGroupAttributes().GetAttributesRef<FName>(SlotAttr);
		for (int32 Slot = 0; Slot < NathanFinalSlotCount; ++Slot)
		{
			SlotNames.Set(Groups[Slot], FName(NathanFinalSlots[Slot].Slot));
		}
		for (int32 Index = 0; Index < Polys.Num(); ++Index)
		{
			Desc->SetPolygonPolygonGroup(Polys[Index].Id, Groups[static_cast<int32>(Assigned[Index])]);
		}
		USkeletalMesh::FCommitMeshDescriptionParams CommitParams;
		CommitParams.bForceUpdate = true;
		if (!Mesh->CommitMeshDescription(0, CommitParams))
		{
			OutError = TEXT("CommitMeshDescription failed.");
			return false;
		}
		TArray<FSkeletalMaterial> Materials;
		Materials.SetNum(Groups.Num());
		for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
		{
			const int32 Slot = FMath::Min(GroupIndex, NathanFinalSlotCount - 1);
			UMaterialInterface* Interface = LoadObject<UMaterialInterface>(nullptr, *NathanFinalMiPath(NathanFinalSlots[Slot].Asset));
			Materials[GroupIndex].MaterialInterface = Interface;
			Materials[GroupIndex].MaterialSlotName = NathanFinalSlots[Slot].Slot;
		}
		Mesh->SetMaterials(Materials);
		Mesh->InvalidateDeriveDataCacheGUID();
		Mesh->Build();
		Mesh->SetMaterials(Materials);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		OutMode = bAnyHead ? TEXT("bones") : TEXT("height_fallback");
		return true;
	}

	void NathanFinalCollectDirty(TArray<FString>& Content, TArray<FString>& Worlds)
	{
		TArray<UPackage*> Packages;
		FEditorFileUtils::GetDirtyContentPackages(Packages);
		for (UPackage* Package : Packages)
		{
			if (Package)
			{
				Content.Add(Package->GetName());
			}
		}
		Packages.Reset();
		FEditorFileUtils::GetDirtyWorldPackages(Packages);
		for (UPackage* Package : Packages)
		{
			if (Package)
			{
				Worlds.Add(Package->GetName());
			}
		}
	}

	void NathanFinalClearNewWorldDirty(const TArray<FString>& WorldsBefore)
	{
		TArray<UPackage*> Packages;
		FEditorFileUtils::GetDirtyWorldPackages(Packages);
		for (UPackage* Package : Packages)
		{
			if (Package && !WorldsBefore.Contains(Package->GetName()))
			{
				Package->SetDirtyFlag(false);
			}
		}
	}

	FString NathanFinalUnexpectedDirty(const TArray<FString>& ContentBefore)
	{
		TArray<FString> Content;
		TArray<FString> Worlds;
		NathanFinalCollectDirty(Content, Worlds);
		if (Worlds.Num() > 0)
		{
			return FString::Printf(TEXT("World package became dirty: %s"), *FString::Join(Worlds, TEXT(", ")));
		}
		TArray<FString> Allowed;
		Allowed.Add(NathanFinalMeshPackage);
		Allowed.Add(NathanFinalParentPackage);
		for (const FNathanFinalSlotSpec& Spec : NathanFinalSlots)
		{
			Allowed.Add(NathanFinalMiPackage(Spec.Asset));
		}
		for (const FString& Name : Content)
		{
			if (!ContentBefore.Contains(Name) && !Allowed.Contains(Name))
			{
				return FString::Printf(TEXT("Unexpected dirty package %s"), *Name);
			}
		}
		return FString();
	}

	FString PreflightCreateNathanGrantFinal(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(NathanFinalSpec))
		{
			return TEXT("spec must be nathan_grant_final_v1.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save must be false. create_nathan_grant_final does not save.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this write.");
		}
		USkeletalMesh* Source = NathanFinalLoadSource();
		if (!Source || !Source->GetSkeleton() || !Source->HasMeshDescription(0))
		{
			return TEXT("SKM_NathanGrant is missing or has no mesh description.");
		}
		bool bTorso = false;
		bool bHead = false;
		for (const FSkeletalMaterial& Slot : Source->GetMaterials())
		{
			bTorso |= Slot.MaterialSlotName == TEXT("M_Torso");
			bHead |= Slot.MaterialSlotName == TEXT("M_HeadLegs");
		}
		if (!bTorso || !bHead || Source->GetMaterials().Num() < 2)
		{
			return TEXT("SKM_NathanGrant must keep M_Torso and M_HeadLegs.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		NathanFinalCollectDirty(Content, Worlds);
		if (Worlds.Num() > 0)
		{
			return TEXT("A world package is already dirty. Refusing the character write.");
		}
		USkeletalMesh* Mesh = NathanFinalLoadMesh();
		const bool bExact = NathanFinalMeshExact(Mesh);
		Before->SetBoolField(TEXT("source_exists"), true);
		Before->SetNumberField(TEXT("source_slots"), Source->GetMaterials().Num());
		Before->SetBoolField(TEXT("final_exists"), Mesh != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bExact);
		Before->SetStringField(TEXT("source_skeleton"), Source->GetSkeleton()->GetPathName());
		Proposed->SetStringField(TEXT("spec"), NathanFinalSpec);
		Proposed->SetStringField(TEXT("mesh"), NathanFinalMeshPath);
		Proposed->SetStringField(TEXT("parent"), NathanFinalParentPath);
		Proposed->SetNumberField(TEXT("slots"), NathanFinalSlotCount);
		Proposed->SetBoolField(TEXT("keeps_legacy_mesh"), true);
		Proposed->SetBoolField(TEXT("will_mutate"), !bExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves"), false);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteCreateNathanGrantFinal(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_nathan_grant_final must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNathanGrantFinal(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		USkeletalMesh* Source = NathanFinalLoadSource();
		const bool bSourceDirty = Source && Source->GetPackage() && Source->GetPackage()->IsDirty();
		TArray<FString> ContentBefore;
		TArray<FString> WorldsBefore;
		NathanFinalCollectDirty(ContentBefore, WorldsBefore);
		if (NathanFinalMeshExact(NathanFinalLoadMesh()))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("created"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetStringField(TEXT("mesh"), NathanFinalMeshPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		FString MaterialError;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		if (!NathanFinalEnsureMaterials(AssetToolsModule.Get(), MaterialError))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("write_failed"), MaterialError, MakeShared<FBridgeChange>(Change));
		}
		USkeletalMesh* Mesh = NathanFinalLoadMesh();
		if (!Mesh)
		{
			Mesh = Cast<USkeletalMesh>(AssetToolsModule.Get().DuplicateAsset(TEXT("SKM_NathanGrant_Final"), NathanFinalFolder, Source));
		}
		if (!Mesh)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("write_failed"), TEXT("SKM_NathanGrant_Final was not created."), MakeShared<FBridgeChange>(Change));
		}
		TArray<int32> Counts;
		FString Mode;
		FString PartitionError;
		if (!NathanFinalPartition(Mesh, Counts, Mode, PartitionError))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("write_failed"), PartitionError, MakeShared<FBridgeChange>(Change));
		}
		if (Source && Source->GetPackage() && !bSourceDirty)
		{
			Source->GetPackage()->SetDirtyFlag(false);
		}
		NathanFinalClearNewWorldDirty(WorldsBefore);
		const FString DirtyError = NathanFinalUnexpectedDirty(ContentBefore);
		Mesh = NathanFinalLoadMesh();
		if (!NathanFinalMeshExact(Mesh) || !DirtyError.IsEmpty())
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				DirtyError.IsEmpty() ? TEXT("SKM_NathanGrant_Final slots did not verify.") : DirtyError,
				MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("result"), TEXT("created"));
		Change.After->SetBoolField(TEXT("created"), true);
		Change.After->SetBoolField(TEXT("mutated"), true);
		Change.After->SetStringField(TEXT("mesh"), Mesh->GetPathName());
		Change.After->SetStringField(TEXT("skeleton"), Mesh->GetSkeleton()->GetPathName());
		Change.After->SetStringField(TEXT("classification"), Mode);
		Change.After->SetNumberField(TEXT("slots"), Mesh->GetMaterials().Num());
		Change.After->SetBoolField(TEXT("legacy_kept"), NathanFinalLoadSource() != nullptr);
		Change.After->SetBoolField(TEXT("saves"), false);
		TArray<TSharedPtr<FJsonValue>> SlotCounts;
		for (int32 Slot = 0; Slot < Counts.Num(); ++Slot)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("slot"), NathanFinalSlots[Slot].Slot);
			Row->SetNumberField(TEXT("triangles"), Counts[Slot]);
			SlotCounts.Add(MakeShared<FJsonValueObject>(Row));
		}
		Change.After->SetArrayField(TEXT("slot_counts"), SlotCounts);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSaveNathanGrantFinal(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(NathanFinalSaveSpec))
		{
			return TEXT("spec must be nathan_grant_final_save_v1.");
		}
		if (GetBool(Args, TEXT("save_all"), false))
		{
			return TEXT("save_all is forbidden.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before this save.");
		}
		if (!NathanFinalMeshExact(NathanFinalLoadMesh()))
		{
			return TEXT("SKM_NathanGrant_Final is not exact. Refusing save.");
		}
		TArray<FString> Content;
		TArray<FString> Worlds;
		NathanFinalCollectDirty(Content, Worlds);
		if (Worlds.Num() > 0)
		{
			return FString::Printf(TEXT("Refusing save while a world package is dirty: %s"), *FString::Join(Worlds, TEXT(", ")));
		}
		TArray<FString> Allowed;
		Allowed.Add(NathanFinalMeshPackage);
		Allowed.Add(NathanFinalParentPackage);
		for (const FNathanFinalSlotSpec& Spec : NathanFinalSlots)
		{
			Allowed.Add(NathanFinalMiPackage(Spec.Asset));
		}
		for (const FString& Name : Content)
		{
			if (!Allowed.Contains(Name))
			{
				return FString::Printf(TEXT("Refusing save while unexpected package is dirty: %s"), *Name);
			}
		}
		TArray<TSharedPtr<FJsonValue>> Packages;
		TArray<TSharedPtr<FJsonValue>> Dirty;
		for (const FString& Name : Allowed)
		{
			Packages.Add(MakeShared<FJsonValueString>(Name));
		}
		for (const FString& Name : Content)
		{
			Dirty.Add(MakeShared<FJsonValueString>(Name));
		}
		Before->SetArrayField(TEXT("dirty"), Dirty);
		Proposed->SetArrayField(TEXT("packages"), Packages);
		Proposed->SetBoolField(TEXT("saves_maps"), false);
		return FString();
	}

	TSharedRef<FJsonObject> ExecuteSaveNathanGrantFinal(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("save_nathan_grant_final must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSaveNathanGrantFinal(Change.Args, Before, Proposed);
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
			return FailAudit(TEXT("write_failed"), TEXT("No character packages to save."), MakeShared<FBridgeChange>(Change));
		}
		for (const TSharedPtr<FJsonValue>& Value : *ProposedPackages)
		{
			const FString Name = Value->AsString();
			if (IsMapPackageName(Name))
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("use_save_maps"), TEXT("Character save refused a map package."), MakeShared<FBridgeChange>(Change));
			}
			UPackage* Package = FindPackage(nullptr, *Name);
			if (!Package)
			{
				Package = LoadPackage(nullptr, *Name, LOAD_None);
			}
			if (!Package)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("not_found"), FString::Printf(TEXT("Package %s was not found."), *Name), MakeShared<FBridgeChange>(Change));
			}
			Packages.Add(Package);
		}
		const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, /*bOnlyDirty=*/true);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = bSaved;
		Change.Status = bSaved ? TEXT("executed") : TEXT("execute_save_failed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("saved"), bSaved);
		Change.After->SetNumberField(TEXT("packages"), Packages.Num());
		LogAudit(TEXT("execute"), Change);
		if (!bSaved)
		{
			return FailAudit(TEXT("save_failed"), TEXT("Character package save did not complete."), MakeShared<FBridgeChange>(Change));
		}
		return Ok(AuditBase(Change));
	}

	FString NathanFinalWriteThumbnail(USkeletalMesh* Mesh)
	{
		if (!Mesh)
		{
			return FString();
		}
		FObjectThumbnail Thumbnail;
		ThumbnailTools::RenderThumbnail(
			Mesh,
			1024,
			1024,
			ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush,
			nullptr,
			&Thumbnail);
		const TArray<uint8>& Image = Thumbnail.GetUncompressedImageData();
		if (Image.Num() == 0 || Thumbnail.GetImageWidth() <= 0 || Thumbnail.GetImageHeight() <= 0)
		{
			return FString();
		}
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (!Wrapper.IsValid() || !Wrapper->SetRaw(Image.GetData(), Image.Num(), Thumbnail.GetImageWidth(), Thumbnail.GetImageHeight(), ERGBFormat::BGRA, 8))
		{
			return FString();
		}
		const FString Path = FPaths::Combine(FPlatformMisc::GetEnvironmentVariable(TEXT("TEMP")), TEXT("b25_nathan_final.png"));
		if (!FFileHelper::SaveArrayToFile(Wrapper->GetCompressed(), *Path))
		{
			return FString();
		}
		return Path;
	}

	TSharedRef<FJsonObject> CmdInspectNathanGrantMesh(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		USkeletalMesh* Source = NathanFinalLoadSource();
		USkeletalMesh* Mesh = NathanFinalLoadMesh();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("legacy_found"), Source != nullptr);
		Data->SetNumberField(TEXT("legacy_slots"), Source ? Source->GetMaterials().Num() : 0);
		TArray<TSharedPtr<FJsonValue>> LegacySlots;
		if (Source)
		{
			for (const FSkeletalMaterial& Slot : Source->GetMaterials())
			{
				LegacySlots.Add(MakeShared<FJsonValueString>(Slot.MaterialSlotName.ToString()));
			}
		}
		Data->SetArrayField(TEXT("legacy_slot_names"), LegacySlots);
		Data->SetBoolField(TEXT("final_found"), Mesh != nullptr);
		Data->SetBoolField(TEXT("final_exact"), NathanFinalMeshExact(Mesh));
		Data->SetNumberField(TEXT("final_slots"), Mesh ? Mesh->GetMaterials().Num() : 0);
		TArray<TSharedPtr<FJsonValue>> FinalSlots;
		if (Mesh)
		{
			for (const FSkeletalMaterial& Slot : Mesh->GetMaterials())
			{
				TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("slot"), Slot.MaterialSlotName.ToString());
				Row->SetStringField(TEXT("material"), Slot.MaterialInterface ? Slot.MaterialInterface->GetPathName() : TEXT(""));
				FinalSlots.Add(MakeShared<FJsonValueObject>(Row));
			}
		}
		Data->SetArrayField(TEXT("final_materials"), FinalSlots);
		Data->SetStringField(TEXT("thumbnail"), NathanFinalWriteThumbnail(Mesh ? Mesh : Source));
		TArray<FString> Content;
		TArray<FString> Worlds;
		NathanFinalCollectDirty(Content, Worlds);
		Data->SetNumberField(TEXT("dirty_content"), Content.Num());
		Data->SetNumberField(TEXT("dirty_worlds"), Worlds.Num());
		return Ok(Data);
	}
