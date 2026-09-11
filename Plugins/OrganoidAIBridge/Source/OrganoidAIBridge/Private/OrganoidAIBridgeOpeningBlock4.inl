	const TCHAR* Block4HostLabel = TEXT("Host_Admin_SecurityOfficer");
	const TCHAR* Block4SourceHostLabel = TEXT("Host_Neuro_1");
	const TCHAR* Block4HostBaseClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidHostBase");
	const TCHAR* Block4HostControllerClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidHostAIController");
	const TCHAR* Block4Spec = TEXT("opening_block4_security_officer_v1");
	const TCHAR* Block4AdminResidencyVolumeLabel = TEXT("StreamVolume_Region_Admin");
	const TCHAR* Block4AdminResidencyVolumeClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidStreamingVolume");
	const FVector Block4HostLocation(2820.f, -600.f, 100.f);
	const FRotator Block4HostRotation(0.f, 135.f, 0.f);
	const FVector Block4HostScale(1.f, 1.f, 1.f);

	TSharedRef<FJsonObject> Block4RequiredHostProperties()
	{
		TSharedRef<FJsonObject> Properties = MakeShared<FJsonObject>();
		Properties->SetNumberField(TEXT("MaxHealth"), 100.0);
		Properties->SetNumberField(TEXT("MeleeDamage"), 15.0);
		Properties->SetBoolField(TEXT("bRequiresEncounterActivation"), true);
		Properties->SetNumberField(TEXT("ProximityActivationRange"), 200.0);
		Properties->SetBoolField(TEXT("bAllowPhaseShiftMutations"), false);
		return Properties;
	}

	UClass* LoadBlock4Class(const TCHAR* Path)
	{
		return Path ? LoadObject<UClass>(nullptr, Path) : nullptr;
	}

	UClass* ReadOpeningBlock4ClassProperty(UObject* Object, const TCHAR* PropertyName)
	{
		const FClassProperty* ClassProperty = CastField<FClassProperty>(
			FindInstanceProperty(Object, PropertyName));
		return ClassProperty
			? Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Object))
			: nullptr;
	}

	int32 CountBlock4HostClassInPackage(UWorld* World, UClass* HostBaseClass, const TCHAR* PackageName)
	{
		int32 Count = 0;
		if (!World || !HostBaseClass || !PackageName)
		{
			return Count;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->IsA(HostBaseClass) && PackagesEqual(ActorOwningPackage(Actor), PackageName))
			{
				++Count;
			}
		}
		return Count;
	}

	FString Block4HostPropertyMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("Host actor is null.");
		}
		const TSharedRef<FJsonObject> Required = Block4RequiredHostProperties();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			FProperty* Property = FindInstanceProperty(Actor, Pair.Key);
			if (!Property)
			{
				return FString::Printf(TEXT("Required property '%s' is missing."), *Pair.Key);
			}
			FString VerifyError;
			if (!PropertyMatchesJson(Actor, Property, Pair.Value, VerifyError))
			{
				return VerifyError;
			}
		}
		return TEXT("");
	}

	FString Block4HostMismatch(AActor* Actor, UClass* ExpectedSpawnClass, UClass* HostBaseClass, UClass* ControllerClass)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ExpectedSpawnClass || !HostBaseClass || !ControllerClass)
		{
			return TEXT("required Host classes are unavailable");
		}
		if (Actor->GetClass() != ExpectedSpawnClass || !Actor->IsA(HostBaseClass))
		{
			return FString::Printf(
				TEXT("class '%s' expected exact source class '%s' derived from ProjectOrganoidHostBase"),
				*ClassName(Actor),
				*ExpectedSpawnClass->GetName());
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not Admin"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, Block4HostLocation, Block4HostRotation, Block4HostScale); !Xform.IsEmpty())
		{
			return Xform;
		}
		if (UClass* LiveControllerClass = ReadOpeningBlock4ClassProperty(Actor, TEXT("AIControllerClass")); LiveControllerClass != ControllerClass)
		{
			return FString::Printf(
				TEXT("AIControllerClass '%s' expected '%s'"),
				LiveControllerClass ? *LiveControllerClass->GetPathName() : TEXT("null"),
				*ControllerClass->GetPathName());
		}
		return Block4HostPropertyMismatch(Actor);
	}

	FString GuardOpeningBlock4Anchors(UWorld* World)
	{
		struct FAnchor
		{
			const TCHAR* Label;
			FVector Location;
		};
		const FAnchor Anchors[] = {
			{TEXT("Admin_Terminal_Security"), FVector(2680.f, -400.f, 110.f)},
			{TEXT("Admin_Block2_Security_Chair"), FVector(2680.f, -510.f, 50.f)},
			{TEXT("Pickup_ResearchWingKeycard"), FVector(2580.f, -560.f, 80.f)},
			{TEXT("Pickup_Block3_PistolAmmo"), FVector(2560.f, -340.f, 80.f)},
			{TEXT("Pickup_Block3_TraumaStabilizer"), FVector(2760.f, -300.f, 80.f)},
		};
		for (const FAnchor& Anchor : Anchors)
		{
			const FString Error = GuardExistingActor(World, Anchor.Label, Anchor.Location, AdminPackage);
			if (!Error.IsEmpty())
			{
				return FString::Printf(TEXT("Block 4 anchor %s failed: %s"), Anchor.Label, *Error);
			}
		}
		return TEXT("");
	}

	FString ResolveBlock4SourceClass(
		UWorld* World,
		UClass*& OutSourceClass,
		UClass*& OutHostBaseClass,
		UClass*& OutControllerClass,
		AActor*& OutSourceHost)
	{
		OutSourceClass = nullptr;
		OutHostBaseClass = LoadBlock4Class(Block4HostBaseClassPath);
		OutControllerClass = LoadBlock4Class(Block4HostControllerClassPath);
		OutSourceHost = nullptr;
		if (!World || !OutHostBaseClass || !OutControllerClass)
		{
			return TEXT("ProjectOrganoid Host base or Host AI controller class could not be loaded.");
		}

		TArray<AActor*> SourceMatches = FindByExactLabel(World, Block4SourceHostLabel);
		if (SourceMatches.Num() != 1)
		{
			return FString::Printf(TEXT("%s count=%d expected=1."), Block4SourceHostLabel, SourceMatches.Num());
		}
		OutSourceHost = SourceMatches[0];
		if (!PackagesEqual(ActorOwningPackage(OutSourceHost), NeuroPackage))
		{
			return FString::Printf(
				TEXT("%s owner '%s' expected NeuroGenetics."),
				Block4SourceHostLabel,
				*ActorOwningPackage(OutSourceHost));
		}
		if (!OutSourceHost->IsA(OutHostBaseClass))
		{
			return FString::Printf(TEXT("%s is not an AProjectOrganoidHostBase."), Block4SourceHostLabel);
		}
		OutSourceClass = OutSourceHost->GetClass();
		if (!OutSourceClass || OutSourceClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return TEXT("Source Host class is null or abstract.");
		}
		UObject* CDO = OutSourceClass->GetDefaultObject();
		if (!CDO || ReadOpeningBlock4ClassProperty(CDO, TEXT("AIControllerClass")) != OutControllerClass)
		{
			return TEXT("Source Host class does not use AProjectOrganoidHostAIController.");
		}

		const TSharedRef<FJsonObject> Required = Block4RequiredHostProperties();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			FProperty* Property = FindInstanceProperty(CDO, Pair.Key);
			if (!Property)
			{
				return FString::Printf(TEXT("Source Host class is missing required property '%s'."), *Pair.Key);
			}
			if (!(CastField<FBoolProperty>(Property) || CastField<FNumericProperty>(Property)))
			{
				return FString::Printf(TEXT("Required Host property '%s' is not bool/numeric."), *Pair.Key);
			}
		}
		return TEXT("");
	}

	/** Exact Admin region residency trigger only. Not a generic nonblocking/trigger allowlist. */
	bool IsExactAdminRegionResidencyOverlap(const FOverlapResult& Overlap)
	{
		if (Overlap.bBlockingHit)
		{
			return false;
		}
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor)
		{
			return false;
		}
		if (!ActorLabel(OverlapActor).Equals(Block4AdminResidencyVolumeLabel, ESearchCase::CaseSensitive))
		{
			return false;
		}
		if (!PackagesEqual(ActorOwningPackage(OverlapActor), EpitopePackage))
		{
			return false;
		}
		UClass* OverlapClass = OverlapActor->GetClass();
		if (!OverlapClass || !OverlapClass->GetPathName().Equals(Block4AdminResidencyVolumeClassPath, ESearchCase::CaseSensitive))
		{
			return false;
		}
		UPrimitiveComponent* Primitive = Overlap.GetComponent();
		UBoxComponent* Box = Cast<UBoxComponent>(Primitive);
		if (!Box
			|| Box->GetClass() != UBoxComponent::StaticClass()
			|| !Box->GetName().Equals(TEXT("TriggerVolume"), ESearchCase::CaseSensitive))
		{
			return false;
		}
		if (Box->GetCollisionEnabled() != ECollisionEnabled::QueryOnly)
		{
			return false;
		}
		if (Box->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Overlap)
		{
			return false;
		}
		if (Box->GetCollisionObjectType() != ECC_WorldDynamic)
		{
			return false;
		}
		if (!Box->GetGenerateOverlapEvents())
		{
			return false;
		}
		FProperty* RegionProp = FindInstanceProperty(OverlapActor, TEXT("RegionContextTag"));
		const FEnumProperty* EnumProp = CastField<FEnumProperty>(RegionProp);
		if (!EnumProp || !EnumProp->GetEnum())
		{
			return false;
		}
		const int64 RegionValue = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(
			EnumProp->ContainerPtrToValuePtr<void>(OverlapActor));
		const FString RegionInternal = EnumProp->GetEnum()->GetNameStringByValue(RegionValue);
		if (!RegionInternal.Equals(TEXT("SubLevel1_Admin"), ESearchCase::CaseSensitive))
		{
			return false;
		}
		return true;
	}

	FString PreflightSpawnAdminBlock4SecurityOfficer(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false)
			|| GetBool(Args, TEXT("save_all"), false)
			|| GetBool(Args, TEXT("save_dirty"), false)
			|| GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/save_all/save_dirty/compile must be false. Block 4 spawn does not save or compile.");
		}
		if (!GetBool(Args, TEXT("require_pie_stopped"), true))
		{
			return TEXT("require_pie_stopped must be true.");
		}
		if (GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("Block 4 spawn preflight must run on the game thread.");
		}
		if (!GetString(Args, TEXT("spec")).Equals(Block4Spec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be exactly '%s'."), Block4Spec);
		}
		const FString RequiredPackage = NormalizePackage(GetString(Args, TEXT("required_package")));
		if (!PackagesEqual(RequiredPackage, AdminPackage))
		{
			return TEXT("required_package must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}

		UWorld* World = nullptr;
		const FString WorldError = RequireEpitopeEditorWorld(World);
		if (!WorldError.IsEmpty())
		{
			return WorldError;
		}
		ULevel* AdminLevel = FindLoadedLevelByPackage(World, AdminPackage);
		if (!AdminLevel)
		{
			return TEXT("SL_Epitope_Admin must be loaded. ZERO writes.");
		}
		UPackage* EpitopeMapPackage = FindPackageByName(EpitopePackage);
		if (!EpitopeMapPackage || EpitopeMapPackage->IsDirty())
		{
			return TEXT("Lvl_Epitope must be loaded and clean. Refusing to mix Block 4 with unrelated persistent-map edits.");
		}
		UPackage* AdminMapPackage = FindPackageByName(AdminPackage);
		if (!AdminMapPackage)
		{
			return TEXT("SL_Epitope_Admin package is not loaded in memory.");
		}
		if (AdminMapPackage->IsDirty())
		{
			return TEXT("SL_Epitope_Admin is already dirty. Refusing to mix Block 4 with unrelated edits.");
		}
		if (const FString AnchorError = GuardOpeningBlock4Anchors(World); !AnchorError.IsEmpty())
		{
			return AnchorError;
		}

		FCollisionQueryParams CollisionParams(FName(TEXT("OpeningBlock4HostPreflight")), false);
		TArray<FOverlapResult> CapsuleOverlaps;
		World->OverlapMultiByProfile(
			CapsuleOverlaps,
			Block4HostLocation,
			Block4HostRotation.Quaternion(),
			FName(TEXT("Pawn")),
			FCollisionShape::MakeCapsule(42.0f, 96.0f),
			CollisionParams);
		for (const FOverlapResult& Overlap : CapsuleOverlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (!OverlapActor)
			{
				continue;
			}
			if (IsExactAdminRegionResidencyOverlap(Overlap))
			{
				continue;
			}
			const bool bAllowedAdminFloor =
				ActorLabel(OverlapActor).Equals(TEXT("Admin_FloorPlate"), ESearchCase::CaseSensitive)
				&& PackagesEqual(ActorOwningPackage(OverlapActor), AdminPackage);
			const bool bExistingAuthorizedLabel =
				ActorLabel(OverlapActor).Equals(Block4HostLabel, ESearchCase::CaseSensitive)
				&& PackagesEqual(ActorOwningPackage(OverlapActor), AdminPackage);
			if (!bAllowedAdminFloor && !bExistingAuthorizedLabel)
			{
				return FString::Printf(
					TEXT("Approved Host capsule unexpectedly overlaps '%s' in '%s'. ZERO writes."),
					*ActorLabel(OverlapActor),
					*ActorOwningPackage(OverlapActor));
			}
		}
		UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		FNavLocation ProjectedHostLocation;
		if (!NavSystem || !NavSystem->ProjectPointToNavigation(
			Block4HostLocation,
			ProjectedHostLocation,
			FVector(200.0f, 200.0f, 300.0f)))
		{
			return TEXT("Approved Host location is not projectable onto Admin navigation. ZERO writes.");
		}
		const float ProjectedXYDelta = FVector::Dist2D(Block4HostLocation, ProjectedHostLocation.Location);
		const float ProjectedZDelta = FMath::Abs(Block4HostLocation.Z - ProjectedHostLocation.Location.Z);
		if (ProjectedXYDelta > 75.0f || ProjectedZDelta > 150.0f)
		{
			return FString::Printf(
				TEXT("Nearest Admin navigation is too far from the approved Host location (xy=%.1f z=%.1f). ZERO writes."),
				ProjectedXYDelta,
				ProjectedZDelta);
		}

		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (const FString SourceError = ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost); !SourceError.IsEmpty())
		{
			return SourceError;
		}

		TArray<AActor*> LabelMatches = FindByExactLabel(World, Block4HostLabel);
		const int32 AdminHostCount = CountBlock4HostClassInPackage(World, HostBaseClass, AdminPackage);
		if (LabelMatches.Num() > 1 || AdminHostCount > 1)
		{
			return FString::Printf(
				TEXT("Block 4 uniqueness failed: label count=%d Admin Host count=%d expected at most 1."),
				LabelMatches.Num(),
				AdminHostCount);
		}
		if ((LabelMatches.Num() == 1) != (AdminHostCount == 1))
		{
			return FString::Printf(
				TEXT("Block 4 actor mismatch: label count=%d Admin Host count=%d. Refusing repair-by-spawn."),
				LabelMatches.Num(),
				AdminHostCount);
		}

		bool bAlreadyPresent = false;
		if (LabelMatches.Num() == 1)
		{
			if (const FString Mismatch = Block4HostMismatch(LabelMatches[0], SourceClass, HostBaseClass, ControllerClass); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Existing %s is mismatched: %s"), Block4HostLabel, *Mismatch);
			}
			bAlreadyPresent = true;
			Before->SetObjectField(TEXT("existing_actor"), ActorSnapshot(LabelMatches[0]));
		}

		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetStringField(TEXT("destination_package"), AdminPackage);
		Before->SetBoolField(TEXT("destination_loaded"), true);
		Before->SetBoolField(TEXT("destination_dirty"), false);
		Before->SetBoolField(TEXT("pie_running"), false);
		Before->SetBoolField(TEXT("on_game_thread"), true);
		Before->SetNumberField(TEXT("label_count"), LabelMatches.Num());
		Before->SetNumberField(TEXT("admin_host_count"), AdminHostCount);
		Before->SetBoolField(TEXT("already_present"), bAlreadyPresent);
		Before->SetObjectField(TEXT("source_host"), ActorSnapshot(SourceHost));
		Before->SetStringField(TEXT("source_class"), SourceClass->GetPathName());

		TArray<TSharedPtr<FJsonValue>> ProposedProps;
		const TSharedRef<FJsonObject> Required = Block4RequiredHostProperties();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("property"), Pair.Key);
			Row->SetStringField(TEXT("value"), JsonValueDebug(Pair.Value));
			ProposedProps.Add(MakeShared<FJsonValueObject>(Row));
		}
		Proposed->SetStringField(TEXT("spec"), Block4Spec);
		Proposed->SetStringField(TEXT("label"), Block4HostLabel);
		Proposed->SetStringField(TEXT("class"), SourceClass->GetPathName());
		Proposed->SetStringField(TEXT("destination_package"), AdminPackage);
		Proposed->SetArrayField(TEXT("location"), Vec(Block4HostLocation));
		Proposed->SetArrayField(TEXT("rotation"), Rot(Block4HostRotation));
		Proposed->SetArrayField(TEXT("scale"), Vec(Block4HostScale));
		Proposed->SetArrayField(TEXT("properties"), ProposedProps);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyPresent
				? TEXT("Authorized Block 4 Host already matches; execute is a no-op and does not save.")
				: TEXT("Spawn exactly one existing Host class as Host_Admin_SecurityOfficer in Admin with the authored activation and mutation configuration. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnAdminBlock4SecurityOfficer(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("Block 4 spawn must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> ReBefore = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> ReProposed = MakeShared<FJsonObject>();
		const FString Replay = PreflightSpawnAdminBlock4SecurityOfficer(Change.Args, ReBefore, ReProposed);
		if (!Replay.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("preflight_replay_failed"), FString::Printf(TEXT("ZERO writes. %s"), *Replay), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		UClass* SourceClass = nullptr;
		UClass* HostBaseClass = nullptr;
		UClass* ControllerClass = nullptr;
		AActor* SourceHost = nullptr;
		if (const FString SourceError = ResolveBlock4SourceClass(World, SourceClass, HostBaseClass, ControllerClass, SourceHost); !SourceError.IsEmpty())
		{
			return FailAudit(TEXT("source_changed"), FString::Printf(TEXT("ZERO writes. %s"), *SourceError), MakeShared<FBridgeChange>(Change));
		}
		(void)SourceHost;

		TArray<AActor*> Existing = FindByExactLabel(World, Block4HostLabel);
		if (Existing.Num() == 1)
		{
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetBoolField(TEXT("already_present"), true);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Existing[0]));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Existing.Num() != 0)
		{
			return FailAudit(TEXT("label_not_unique"), TEXT("Authorized label is not unique. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		ULevel* TargetLevel = FindLoadedLevelByPackage(World, AdminPackage);
		if (!World || !TargetLevel)
		{
			return FailAudit(TEXT("not_found"), TEXT("World or Admin level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "OpeningBlock4Host", "Opening Block 4 security officer"));
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		Params.bNoFail = false;
		AActor* Spawned = World->SpawnActor<AActor>(SourceClass, Block4HostLocation, Block4HostRotation, Params);
		if (!Spawned)
		{
			Change.Status = TEXT("execute_failed");
			return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
		}

		auto DestroySpawned = [&]()
		{
			if (!Spawned)
			{
				return;
			}
			if (UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr)
			{
				ActorSub->DestroyActor(Spawned);
			}
			else
			{
				World->DestroyActor(Spawned);
			}
			Spawned = nullptr;
		};

		Spawned->Modify();
		Spawned->SetActorLabel(Block4HostLabel, true);
		Spawned->SetActorScale3D(Block4HostScale);

		const TSharedRef<FJsonObject> Required = Block4RequiredHostProperties();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			FProperty* Property = FindInstanceProperty(Spawned, Pair.Key);
			FString SetError;
			if (!SetPropertyFromJson(Spawned, Property, Pair.Value, SetError)
				|| !PropertyMatchesJson(Spawned, Property, Pair.Value, SetError))
			{
				DestroySpawned();
				Change.Status = TEXT("execute_failed");
				return FailAudit(
					TEXT("config_failed"),
					FString::Printf(TEXT("%s Spawned actor destroyed. Admin not saved."), *SetError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		FString VerifyError = Block4HostMismatch(Spawned, SourceClass, HostBaseClass, ControllerClass);
		if (VerifyError.IsEmpty())
		{
			const int32 HostCount = CountBlock4HostClassInPackage(World, HostBaseClass, AdminPackage);
			const int32 LabelCount = FindByExactLabel(World, Block4HostLabel).Num();
			if (HostCount != 1 || LabelCount != 1)
			{
				VerifyError = FString::Printf(TEXT("post-spawn Host count=%d label count=%d expected 1/1"), HostCount, LabelCount);
			}
		}
		if (VerifyError.IsEmpty())
		{
			VerifyError = GuardOpeningBlock4Anchors(World);
		}
		if (!VerifyError.IsEmpty())
		{
			DestroySpawned();
			Change.Status = TEXT("execute_failed");
			return FailAudit(
				TEXT("verify_failed"),
				FString::Printf(TEXT("%s Spawned actor destroyed. Admin not saved."), *VerifyError),
				MakeShared<FBridgeChange>(Change));
		}

		Spawned->MarkPackageDirty();
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("already_present"), false);
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("compile_performed"), false);
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Spawned));
		TArray<TSharedPtr<FJsonValue>> AfterProps;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Required->Values)
		{
			AfterProps.Add(MakeShared<FJsonValueObject>(ReadPropertySnapshot(Spawned, FindInstanceProperty(Spawned, Pair.Key))));
		}
		Change.After->SetArrayField(TEXT("properties"), AfterProps);
		Change.After->SetNumberField(TEXT("admin_host_count"), CountBlock4HostClassInPackage(World, HostBaseClass, AdminPackage));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
