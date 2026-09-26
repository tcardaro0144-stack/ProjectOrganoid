// Duplicate the Manny skeleton mesh into SKM_NathanGrant and tint the existing slots.
// Head and legs stay one slot. Torso is the jacket. Does not save and does not move actors.
	const TCHAR* NathanLookSpec = TEXT("nathan_grant_look_v1");
	const TCHAR* NathanLookAction = TEXT("create_nathan_grant_look");
	const TCHAR* NathanFolder = TEXT("/Game/Characters/Nathan");
	const TCHAR* NathanMeshPath = TEXT("/Game/Characters/Nathan/SKM_NathanGrant.SKM_NathanGrant");
	const TCHAR* NathanTorsoPath = TEXT("/Game/Characters/Nathan/MI_NathanGrant_Torso.MI_NathanGrant_Torso");
	const TCHAR* NathanHeadPath = TEXT("/Game/Characters/Nathan/MI_NathanGrant_HeadLegs.MI_NathanGrant_HeadLegs");
	const TCHAR* MannyMeshPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const FLinearColor NathanTorsoTint(0.18f, 0.20f, 0.24f, 1.0f);
	const FLinearColor NathanHeadTint(1.0f, 0.84f, 0.70f, 1.0f);
	const FName NathanPaintTint(TEXT("Paint Tint"));

	bool NathanTintNear(const FLinearColor& A, const FLinearColor& B)
	{
		return FMath::IsNearlyEqual(A.R, B.R, 0.03f)
			&& FMath::IsNearlyEqual(A.G, B.G, 0.03f)
			&& FMath::IsNearlyEqual(A.B, B.B, 0.03f);
	}

	USkeletalMesh* NathanLoadMesh()
	{
		return LoadObject<USkeletalMesh>(nullptr, NathanMeshPath);
	}

	UMaterialInstanceConstant* NathanLoadTorso()
	{
		return LoadObject<UMaterialInstanceConstant>(nullptr, NathanTorsoPath);
	}

	UMaterialInstanceConstant* NathanLoadHead()
	{
		return LoadObject<UMaterialInstanceConstant>(nullptr, NathanHeadPath);
	}

	FLinearColor NathanReadTint(const UMaterialInstanceConstant* Material)
	{
		if (!Material)
		{
			return FLinearColor::White;
		}
		for (const FVectorParameterValue& Value : Material->VectorParameterValues)
		{
			if (Value.ParameterInfo.Name == NathanPaintTint)
			{
				return Value.ParameterValue;
			}
		}
		return FLinearColor::White;
	}

	bool NathanLookExact(USkeletalMesh* Mesh, UMaterialInstanceConstant* Torso, UMaterialInstanceConstant* Head)
	{
		if (!Mesh || !Torso || !Head || Mesh->GetSkeleton() == nullptr)
		{
			return false;
		}
		if (!NathanTintNear(NathanReadTint(Torso), NathanTorsoTint)
			|| !NathanTintNear(NathanReadTint(Head), NathanHeadTint))
		{
			return false;
		}
		bool bTorso = false;
		bool bHead = false;
		for (const FSkeletalMaterial& Slot : Mesh->GetMaterials())
		{
			if (Slot.MaterialSlotName == TEXT("M_Torso") && Slot.MaterialInterface == Torso)
			{
				bTorso = true;
			}
			if (Slot.MaterialSlotName == TEXT("M_HeadLegs") && Slot.MaterialInterface == Head)
			{
				bHead = true;
			}
		}
		return bTorso && bHead;
	}

	void NathanApplyTint(UMaterialInstanceConstant* Material, const FLinearColor& Tint)
	{
		Material->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(NathanPaintTint), Tint);
		Material->PostEditChange();
		Material->MarkPackageDirty();
	}

	FString PreflightCreateNathanGrantLook(const TSharedPtr<FJsonObject>& Args, TSharedRef<FJsonObject> Before, TSharedRef<FJsonObject> Proposed)
	{
		if (!GetString(Args, TEXT("spec")).Equals(NathanLookSpec))
		{
			return TEXT("spec must be nathan_grant_look_v1.");
		}
		USkeletalMesh* Source = LoadObject<USkeletalMesh>(nullptr, MannyMeshPath);
		if (!Source || !Source->GetSkeleton())
		{
			return TEXT("SKM_Manny_Simple is missing.");
		}
		bool bHasTorso = false;
		bool bHasHead = false;
		for (const FSkeletalMaterial& Slot : Source->GetMaterials())
		{
			bHasTorso |= Slot.MaterialSlotName == TEXT("M_Torso");
			bHasHead |= Slot.MaterialSlotName == TEXT("M_HeadLegs");
		}
		if (!bHasTorso || !bHasHead)
		{
			return TEXT("SKM_Manny_Simple is missing M_Torso or M_HeadLegs.");
		}
		USkeletalMesh* Mesh = NathanLoadMesh();
		UMaterialInstanceConstant* Torso = NathanLoadTorso();
		UMaterialInstanceConstant* Head = NathanLoadHead();
		const bool bExact = NathanLookExact(Mesh, Torso, Head);
		Before->SetBoolField(TEXT("exists"), Mesh != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bExact);
		Before->SetStringField(TEXT("source_mesh"), MannyMeshPath);
		Before->SetStringField(TEXT("skeleton"), Source->GetSkeleton()->GetPathName());
		Proposed->SetStringField(TEXT("spec"), NathanLookSpec);
		Proposed->SetStringField(TEXT("mesh"), NathanMeshPath);
		Proposed->SetStringField(TEXT("torso"), NathanTorsoPath);
		Proposed->SetStringField(TEXT("head_legs"), NathanHeadPath);
		Proposed->SetBoolField(TEXT("will_mutate"), !bExact);
		Proposed->SetBoolField(TEXT("saves"), false);
		Proposed->SetBoolField(TEXT("changes_power"), false);
		Proposed->SetBoolField(TEXT("moves"), false);
		return FString();
	}

	UMaterialInstanceConstant* NathanDuplicateMaterial(IAssetTools& AssetTools, UMaterialInterface* Source, const TCHAR* Name, const FLinearColor& Tint)
	{
		UMaterialInstanceConstant* Existing = LoadObject<UMaterialInstanceConstant>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), NathanFolder, Name, Name));
		if (!Existing)
		{
			UObject* Duplicated = AssetTools.DuplicateAsset(Name, NathanFolder, Source);
			Existing = Cast<UMaterialInstanceConstant>(Duplicated);
		}
		if (Existing)
		{
			NathanApplyTint(Existing, Tint);
		}
		return Existing;
	}

	TSharedRef<FJsonObject> ExecuteCreateNathanGrantLook(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("create_nathan_grant_look must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightCreateNathanGrantLook(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		Change.Before = Before;
		Change.Proposed = Proposed;
		USkeletalMesh* Mesh = NathanLoadMesh();
		UMaterialInstanceConstant* Torso = NathanLoadTorso();
		UMaterialInstanceConstant* Head = NathanLoadHead();
		if (NathanLookExact(Mesh, Torso, Head))
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("created"), true);
			Change.After->SetBoolField(TEXT("mutated"), false);
			Change.After->SetStringField(TEXT("mesh"), NathanMeshPath);
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}

		USkeletalMesh* Source = LoadObject<USkeletalMesh>(nullptr, MannyMeshPath);
		UMaterialInterface* SourceTorso = nullptr;
		UMaterialInterface* SourceHead = nullptr;
		if (Source)
		{
			for (const FSkeletalMaterial& Slot : Source->GetMaterials())
			{
				if (Slot.MaterialSlotName == TEXT("M_Torso"))
				{
					SourceTorso = Slot.MaterialInterface;
				}
				if (Slot.MaterialSlotName == TEXT("M_HeadLegs"))
				{
					SourceHead = Slot.MaterialInterface;
				}
			}
		}
		if (!Source || !SourceTorso || !SourceHead)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("missing"), TEXT("Manny mesh slots are missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		{
			const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "NathanGrantLook", "Create Nathan Grant look"));
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			IAssetTools& AssetTools = AssetToolsModule.Get();
			Torso = NathanDuplicateMaterial(AssetTools, SourceTorso, TEXT("MI_NathanGrant_Torso"), NathanTorsoTint);
			Head = NathanDuplicateMaterial(AssetTools, SourceHead, TEXT("MI_NathanGrant_HeadLegs"), NathanHeadTint);
			if (!Mesh)
			{
				Mesh = Cast<USkeletalMesh>(AssetTools.DuplicateAsset(TEXT("SKM_NathanGrant"), NathanFolder, Source));
			}
			if (!Mesh || !Torso || !Head)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("write_failed"), TEXT("Nathan Grant mesh or materials were not created."), MakeShared<FBridgeChange>(Change));
			}
			TArray<FSkeletalMaterial> Slots = Mesh->GetMaterials();
			for (FSkeletalMaterial& Slot : Slots)
			{
				if (Slot.MaterialSlotName == TEXT("M_Torso"))
				{
					Slot.MaterialInterface = Torso;
				}
				else if (Slot.MaterialSlotName == TEXT("M_HeadLegs"))
				{
					Slot.MaterialInterface = Head;
				}
			}
			Mesh->SetMaterials(Slots);
			Mesh->PostEditChange();
			Mesh->MarkPackageDirty();
		}

		Mesh = NathanLoadMesh();
		Torso = NathanLoadTorso();
		Head = NathanLoadHead();
		if (!NathanLookExact(Mesh, Torso, Head))
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("verify_failed"), TEXT("SKM_NathanGrant was not tinted on the Manny skeleton."), MakeShared<FBridgeChange>(Change));
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
		Change.After->SetBoolField(TEXT("saves"), false);
		Change.After->SetBoolField(TEXT("changes_power"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
