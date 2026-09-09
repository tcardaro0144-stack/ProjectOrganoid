	const TCHAR* AudioZoneSpec = TEXT("s22_admin_audio_zones_v1");
	const TCHAR* AdminAudioZoneBpPath = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAudioZone");
	const TCHAR* AdminAudioZoneBpPackage = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAudioZone");
	const TCHAR* FacilityBedPath = TEXT("/Game/Audio/Ambient/SW_FacilityBed.SW_FacilityBed");
	const float AudioZoneDoorwayPadUu = 80.0f;
	const float AudioZoneExtentEps = 1.0f;
	const float AudioZoneOverlapThinAxisMax = 200.0f;

	struct FS22LegacyAmbienceSpec
	{
		const TCHAR* Label;
		const TCHAR* ZoneId;
		FVector Location;
	};

	const FS22LegacyAmbienceSpec LegacyAdminAmbienceSpecs[] = {
		{TEXT("Ambience_ReceptionAtrium"), TEXT("Admin_Atrium"), FVector(-950.f, 0.f, 200.f)},
		{TEXT("Ambience_HepaPlenum"), TEXT("Admin_Plenum"), FVector(-1950.f, 1650.f, 200.f)},
	};

	struct FS22AdminAudioZoneSpec
	{
		const TCHAR* Label;
		const TCHAR* ZoneId;
		const TCHAR* DisplayName;
		int32 Priority;
		const TCHAR* RoomTriggerLabels[3];
		int32 RoomTriggerCount;
	};

	const FS22AdminAudioZoneSpec AdminAudioZoneSpecs[] = {
		{TEXT("Admin_AudioZone_Public"), TEXT("Admin_Public"), TEXT("Admin Public"), 1,
			{TEXT("Admin_RoomTrigger_Vestibule"), TEXT("Admin_RoomTrigger_Reception"), TEXT("Admin_RoomTrigger_Hub")}, 3},
		{TEXT("Admin_AudioZone_Secure"), TEXT("Admin_Secure"), TEXT("Admin Secure"), 2,
			{TEXT("Admin_RoomTrigger_Security"), TEXT("Admin_RoomTrigger_Records"), nullptr}, 2},
		{TEXT("Admin_AudioZone_Executive"), TEXT("Admin_Executive"), TEXT("Admin Executive"), 2,
			{TEXT("Admin_RoomTrigger_Conference"), TEXT("Admin_RoomTrigger_DirectorSuite"), nullptr}, 2},
		{TEXT("Admin_AudioZone_Service"), TEXT("Admin_Service"), TEXT("Admin Service"), 1,
			{TEXT("Admin_RoomTrigger_Operations"), TEXT("Admin_RoomTrigger_Transit"), TEXT("Admin_RoomTrigger_ServiceCorridor")}, 3},
	};

	const FS22AdminAudioZoneSpec* FindAdminAudioZoneSpec(const FString& Label)
	{
		for (const FS22AdminAudioZoneSpec& Spec : AdminAudioZoneSpecs)
		{
			if (Label.Equals(Spec.Label, ESearchCase::CaseSensitive))
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	UBoxComponent* FindActorBox(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		TArray<UBoxComponent*> Boxes;
		Actor->GetComponents<UBoxComponent>(Boxes);
		for (UBoxComponent* Box : Boxes)
		{
			if (Box && Box->GetName().Equals(TEXT("TriggerBox"), ESearchCase::IgnoreCase))
			{
				return Box;
			}
		}
		if (UBoxComponent* Root = Cast<UBoxComponent>(Actor->GetRootComponent()))
		{
			return Root;
		}
		return Boxes.Num() > 0 ? Boxes[0] : nullptr;
	}

	bool ComputeS22AudioZoneBox(
		UWorld* World,
		const FS22AdminAudioZoneSpec& Spec,
		FVector& OutCenter,
		FVector& OutExtent,
		FString& OutError)
	{
		OutCenter = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		if (!World)
		{
			OutError = TEXT("No editor world.");
			return false;
		}

		FBox Union(ForceInit);
		bool bAny = false;
		for (int32 Index = 0; Index < Spec.RoomTriggerCount; ++Index)
		{
			const TCHAR* TriggerLabel = Spec.RoomTriggerLabels[Index];
			if (!TriggerLabel)
			{
				continue;
			}
			TArray<AActor*> Matches = FindByExactLabel(World, TriggerLabel);
			if (Matches.Num() != 1)
			{
				OutError = FString::Printf(TEXT("%s count=%d expected=1"), TriggerLabel, Matches.Num());
				return false;
			}
			AActor* Trigger = Matches[0];
			if (!PackagesEqual(ActorOwningPackage(Trigger), AdminPackage))
			{
				OutError = FString::Printf(TEXT("%s is not on SL_Epitope_Admin."), TriggerLabel);
				return false;
			}
			UBoxComponent* Box = FindActorBox(Trigger);
			if (!Box)
			{
				OutError = FString::Printf(TEXT("%s has no box component."), TriggerLabel);
				return false;
			}
			const FBox WorldBox = Box->Bounds.GetBox();
			if (!WorldBox.IsValid)
			{
				OutError = FString::Printf(TEXT("%s box bounds are invalid."), TriggerLabel);
				return false;
			}
			if (!bAny)
			{
				Union = WorldBox;
				bAny = true;
			}
			else
			{
				Union += WorldBox;
			}
		}
		if (!bAny || !Union.IsValid)
		{
			OutError = FString::Printf(TEXT("%s could not union live S15 trigger boxes."), Spec.Label);
			return false;
		}

		OutCenter = Union.GetCenter();
		OutExtent = Union.GetExtent();
		OutExtent.X += AudioZoneDoorwayPadUu;
		OutExtent.Y += AudioZoneDoorwayPadUu;
		if (OutExtent.X < 50.f || OutExtent.Y < 50.f || OutExtent.Z < 50.f)
		{
			OutError = FString::Printf(TEXT("%s computed extent is too small."), Spec.Label);
			return false;
		}
		return true;
	}

	FBox MakeCenterExtentBox(const FVector& Center, const FVector& Extent)
	{
		return FBox(Center - Extent, Center + Extent);
	}

	float SmallestOverlapAxis(const FBox& A, const FBox& B)
	{
		const FBox Overlap = A.Overlap(B);
		if (!Overlap.IsValid)
		{
			return 0.0f;
		}
		const FVector Size = Overlap.GetSize();
		return FMath::Min3(Size.X, Size.Y, Size.Z);
	}

	FString RequireS22AudioSpec(const TSharedPtr<FJsonObject>& Args)
	{
		if (GetString(Args, TEXT("spec")) != AudioZoneSpec)
		{
			return TEXT("This action requires spec=s22_admin_audio_zones_v1.");
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. Section 22 map writes do not save or compile.");
		}
		const FString RequiredPackage = NormalizePackage(
			GetString(Args, TEXT("required_package"), GetString(Args, TEXT("package"), GetString(Args, TEXT("destination_package"), AdminPackage))));
		if (!PackagesEqual(RequiredPackage, AdminPackage))
		{
			return TEXT("required_package must be /Game/Maps/Epitope/SL_Epitope_Admin.");
		}
		return TEXT("");
	}

	FString PreflightDeleteS22LegacyAdminAmbience(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		const FString SpecError = RequireS22AudioSpec(Args);
		if (!SpecError.IsEmpty())
		{
			return SpecError;
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin is not loaded. ZERO writes.");
		}

		TArray<TSharedPtr<FJsonValue>> BeforeActors;
		TArray<TSharedPtr<FJsonValue>> ProposedActors;
		for (const FS22LegacyAmbienceSpec& Spec : LegacyAdminAmbienceSpecs)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("%s count=%d expected=1"), Spec.Label, Matches.Num());
			}
			AActor* Actor = Matches[0];
			if (ClassName(Actor) != TEXT("ProjectOrganoidAmbienceZone"))
			{
				return FString::Printf(TEXT("%s class=%s expected ProjectOrganoidAmbienceZone"), Spec.Label, *ClassName(Actor));
			}
			if (!PackagesEqual(ActorOwningPackage(Actor), AdminPackage))
			{
				return FString::Printf(TEXT("%s owning package is not SL_Epitope_Admin."), Spec.Label);
			}
			if (!LocationMatches(Actor->GetActorLocation(), Spec.Location))
			{
				const FVector Loc = Actor->GetActorLocation();
				return FString::Printf(
					TEXT("%s location (%.2f, %.2f, %.2f) does not match (%.2f, %.2f, %.2f)"),
					Spec.Label, Loc.X, Loc.Y, Loc.Z, Spec.Location.X, Spec.Location.Y, Spec.Location.Z);
			}
			if (FNameProperty* ZoneIdProp = FindFProperty<FNameProperty>(Actor->GetClass(), TEXT("ZoneId")))
			{
				const FName ZoneId = ZoneIdProp->GetPropertyValue_InContainer(Actor);
				if (!ZoneId.IsEqual(FName(Spec.ZoneId)))
				{
					return FString::Printf(TEXT("%s ZoneId=%s expected %s"), Spec.Label, *ZoneId.ToString(), Spec.ZoneId);
				}
			}
			if (!AttachParentLabel(Actor).IsEmpty() || ChildLabels(Actor).Num() > 0)
			{
				return FString::Printf(TEXT("%s has attach parent or children. Refusing delete."), Spec.Label);
			}
			BeforeActors.Add(MakeShared<FJsonValueObject>(ActorSnapshot(Actor)));
			TSharedRef<FJsonObject> Gone = MakeShared<FJsonObject>();
			Gone->SetStringField(TEXT("label"), Spec.Label);
			Gone->SetStringField(TEXT("zone_id"), Spec.ZoneId);
			Gone->SetStringField(TEXT("state"), TEXT("deleted"));
			ProposedActors.Add(MakeShared<FJsonValueObject>(Gone));
		}

		Before->SetArrayField(TEXT("actors"), BeforeActors);
		Before->SetStringField(TEXT("owning_package"), AdminPackage);
		Before->SetStringField(TEXT("spec"), AudioZoneSpec);
		Proposed->SetArrayField(TEXT("actors"), ProposedActors);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Delete the two obsolete native Admin ambience actors only. Package not saved."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteDeleteS22LegacyAdminAmbience(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("delete_s22_legacy_admin_ambience must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightDeleteS22LegacyAdminAmbience(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;
		if (!World || !ActorSub)
		{
			return FailAudit(TEXT("no_subsystem"), TEXT("Editor world or EditorActorSubsystem unavailable. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<TSharedPtr<FJsonValue>> Deleted;
		for (const FS22LegacyAmbienceSpec& Spec : LegacyAdminAmbienceSpecs)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() != 1)
			{
				return FailAudit(TEXT("not_found"), FString::Printf(TEXT("%s vanished before destroy. ZERO remaining writes."), Spec.Label), MakeShared<FBridgeChange>(Change));
			}
			AActor* Actor = Matches[0];
			const FString Path = Actor->GetPathName();
			if (!ActorSub->DestroyActor(Actor))
			{
				return FailAudit(
					TEXT("destroy_failed"),
					FString::Printf(TEXT("destroy_actor failed for %s. Later targets not deleted. Package not saved."), Spec.Label),
					MakeShared<FBridgeChange>(Change));
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Spec.Label);
			Row->SetStringField(TEXT("path"), Path);
			Row->SetStringField(TEXT("state"), TEXT("deleted"));
			Deleted.Add(MakeShared<FJsonValueObject>(Row));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetArrayField(TEXT("deleted"), Deleted);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString PreflightSpawnS22AdminAudioZones(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		const FString SpecError = RequireS22AudioSpec(Args);
		if (!SpecError.IsEmpty())
		{
			return SpecError;
		}

		const FString BlueprintPath = NormalizePackage(
			GetString(Args, TEXT("blueprint"), GetString(Args, TEXT("path"), AdminAudioZoneBpPath)));
		if (!PackagesEqual(BlueprintPath, AdminAudioZoneBpPath) && !PackagesEqual(BlueprintPath, AdminAudioZoneBpPackage))
		{
			return TEXT("spawn_s22_admin_audio_zones is allowlisted only for BP_AdminAudioZone.");
		}

		UWorld* World = GetEditorWorld();
		if (!World)
		{
			return TEXT("No editor world.");
		}
		if (!FindLoadedLevelByPackage(World, AdminPackage))
		{
			return TEXT("SL_Epitope_Admin is not loaded. ZERO writes.");
		}

		UBlueprint* Blueprint = LoadBlueprintAsset(AdminAudioZoneBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadBlueprintAsset(FString::Printf(TEXT("%s.BP_AdminAudioZone"), AdminAudioZoneBpPath));
		}
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return TEXT("BP_AdminAudioZone or generated class not found.");
		}
		if (!Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()))
		{
			return TEXT("BP_AdminAudioZone generated class is not an Actor.");
		}
		if (Blueprint->ParentClass && Blueprint->ParentClass->GetName() != TEXT("ProjectOrganoidAmbienceZone"))
		{
			return TEXT("BP_AdminAudioZone parent must remain ProjectOrganoidAmbienceZone.");
		}

		for (const FS22LegacyAmbienceSpec& Legacy : LegacyAdminAmbienceSpecs)
		{
			if (FindByExactLabel(World, Legacy.Label).Num() != 0)
			{
				return FString::Printf(TEXT("%s still exists. Delete the obsolete Admin ambience actors first."), Legacy.Label);
			}
		}

		TArray<FBox> Boxes;
		TArray<TSharedPtr<FJsonValue>> ProposedZones;
		for (const FS22AdminAudioZoneSpec& Spec : AdminAudioZoneSpecs)
		{
			if (FindByExactLabel(World, Spec.Label).Num() != 0)
			{
				return FString::Printf(TEXT("%s already exists. Fail closed."), Spec.Label);
			}
			FVector Center = FVector::ZeroVector;
			FVector Extent = FVector::ZeroVector;
			FString BoxError;
			if (!ComputeS22AudioZoneBox(World, Spec, Center, Extent, BoxError))
			{
				return BoxError;
			}
			Boxes.Add(MakeCenterExtentBox(Center, Extent));
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), Spec.Label);
			Row->SetStringField(TEXT("zone_id"), Spec.ZoneId);
			Row->SetNumberField(TEXT("priority"), Spec.Priority);
			Row->SetArrayField(TEXT("location"), Vec(Center));
			Row->SetArrayField(TEXT("box_extent"), Vec(Extent));
			Row->SetStringField(TEXT("room_tone"), FacilityBedPath);
			ProposedZones.Add(MakeShared<FJsonValueObject>(Row));
		}

		for (int32 IndexA = 0; IndexA < Boxes.Num(); ++IndexA)
		{
			for (int32 IndexB = IndexA + 1; IndexB < Boxes.Num(); ++IndexB)
			{
				if (Boxes[IndexA].Intersect(Boxes[IndexB]))
				{
					const float Thin = SmallestOverlapAxis(Boxes[IndexA], Boxes[IndexB]);
					if (Thin > AudioZoneOverlapThinAxisMax)
					{
						return FString::Printf(
							TEXT("%s overlaps %s too deeply (thin axis %.1f). Refusing spawn."),
							AdminAudioZoneSpecs[IndexA].Label,
							AdminAudioZoneSpecs[IndexB].Label,
							Thin);
					}
				}
			}
		}

		Before->SetStringField(TEXT("destination_package"), AdminPackage);
		Before->SetStringField(TEXT("blueprint"), AdminAudioZoneBpPath);
		Before->SetStringField(TEXT("parent"), TEXT("ProjectOrganoidAmbienceZone"));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Proposed->SetStringField(TEXT("spec"), AudioZoneSpec);
		Proposed->SetArrayField(TEXT("zones"), ProposedZones);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("eventgraph_authored"), false);
		Proposed->SetStringField(TEXT("result"), TEXT("Spawn four BP_AdminAudioZone actors into SL_Epitope_Admin from live S15 trigger unions. Package not saved."));
		return TEXT("");
	}

	bool ConfigureSpawnedAdminAudioZone(AActor* Spawned, const FS22AdminAudioZoneSpec& Spec, const FVector& Extent)
	{
		if (!Spawned)
		{
			return false;
		}
		if (FNameProperty* ZoneIdProp = FindFProperty<FNameProperty>(Spawned->GetClass(), TEXT("ZoneId")))
		{
			ZoneIdProp->SetPropertyValue_InContainer(Spawned, FName(Spec.ZoneId));
		}
		if (FIntProperty* PriorityProp = FindFProperty<FIntProperty>(Spawned->GetClass(), TEXT("Priority")))
		{
			PriorityProp->SetPropertyValue_InContainer(Spawned, Spec.Priority);
		}
		if (FTextProperty* DisplayProp = FindFProperty<FTextProperty>(Spawned->GetClass(), TEXT("DisplayName")))
		{
			DisplayProp->SetPropertyValue_InContainer(Spawned, FText::FromString(Spec.DisplayName));
		}
		if (FSoftObjectProperty* ToneProp = FindFProperty<FSoftObjectProperty>(Spawned->GetClass(), TEXT("RoomToneSound")))
		{
			if (FSoftObjectPtr* TonePtr = ToneProp->ContainerPtrToValuePtr<FSoftObjectPtr>(Spawned))
			{
				*TonePtr = FSoftObjectPtr(FSoftObjectPath(FacilityBedPath));
			}
		}
		UBoxComponent* Box = FindActorBox(Spawned);
		if (!Box)
		{
			return false;
		}
		Box->SetBoxExtent(Extent, true);
		return true;
	}

	TSharedRef<FJsonObject> ExecuteSpawnS22AdminAudioZones(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_s22_admin_audio_zones must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnS22AdminAudioZones(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, AdminPackage) : nullptr;
		UBlueprint* Blueprint = LoadBlueprintAsset(AdminAudioZoneBpPath);
		if (!Blueprint)
		{
			Blueprint = LoadBlueprintAsset(FString::Printf(TEXT("%s.BP_AdminAudioZone"), AdminAudioZoneBpPath));
		}
		if (!World || !TargetLevel || !Blueprint || !Blueprint->GeneratedClass)
		{
			return FailAudit(TEXT("not_found"), TEXT("World, Admin level, or BP_AdminAudioZone vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		struct FPendingZone
		{
			const FS22AdminAudioZoneSpec* Spec;
			FVector Center;
			FVector Extent;
		};
		TArray<FPendingZone> Pending;
		for (const FS22AdminAudioZoneSpec& Spec : AdminAudioZoneSpecs)
		{
			FPendingZone Row;
			Row.Spec = &Spec;
			FString BoxError;
			if (!ComputeS22AudioZoneBox(World, Spec, Row.Center, Row.Extent, BoxError))
			{
				return FailAudit(TEXT("geometry_failed"), FString::Printf(TEXT("ZERO writes. %s"), *BoxError), MakeShared<FBridgeChange>(Change));
			}
			Pending.Add(Row);
		}

		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "SpawnS22AdminAudioZones", "Spawn S22 Admin Audio Zones"));
		TArray<TSharedPtr<FJsonValue>> SpawnedRows;
		for (const FPendingZone& Row : Pending)
		{
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			FVector Location = Row.Center;
			FRotator Rotation = FRotator::ZeroRotator;
			AActor* Spawned = World->SpawnActor(Blueprint->GeneratedClass, &Location, &Rotation, Params);
			if (!Spawned)
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("SpawnActor failed for %s. Package not saved."), Row.Spec->Label), MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(Row.Spec->Label, true);
			Spawned->Tags.AddUnique(FName(TEXT("Admin_AudioZone")));
			Spawned->Tags.AddUnique(FName(Row.Spec->ZoneId));
			Spawned->Tags.AddUnique(FName(Row.Spec->Label));
			if (!ConfigureSpawnedAdminAudioZone(Spawned, *Row.Spec, Row.Extent))
			{
				return FailAudit(TEXT("configure_failed"), FString::Printf(TEXT("Failed to configure %s after spawn. Package not saved."), Row.Spec->Label), MakeShared<FBridgeChange>(Change));
			}
			TSharedRef<FJsonObject> AfterRow = MakeShared<FJsonObject>();
			AfterRow->SetStringField(TEXT("label"), ActorLabel(Spawned));
			AfterRow->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
			AfterRow->SetStringField(TEXT("zone_id"), Row.Spec->ZoneId);
			AfterRow->SetNumberField(TEXT("priority"), Row.Spec->Priority);
			AfterRow->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
			AfterRow->SetArrayField(TEXT("box_extent"), Vec(Row.Extent));
			SpawnedRows.Add(MakeShared<FJsonValueObject>(AfterRow));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetArrayField(TEXT("zones"), SpawnedRows);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("eventgraph_authored"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
