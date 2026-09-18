	// Temporary Neuro power-diagnostic DataPad — replaceable blockout kiosk (Engine cube), not final art.
	const TCHAR* NeuroPowerDiagnosisSpec = TEXT("neuro_power_diagnostic_v1");
	const TCHAR* NeuroPowerDiagnosisAction = TEXT("spawn_neuro_power_diagnostic");
	const TCHAR* NeuroPowerDiagnosisLabel = TEXT("DataPad_NeuroPowerDiagnostics");
	const TCHAR* NeuroPowerDiagnosisClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidDataPad");
	const TCHAR* NeuroPowerDiagnosisClassName = TEXT("ProjectOrganoidDataPad");
	const TCHAR* NeuroPowerDiagnosisCubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* NeuroPowerDiagnosisRequiredObjective = TEXT("Obj_InvestigateNeuroPowerFailure");
	const TCHAR* NeuroPowerDiagnosisEvent = TEXT("Event_NeuroPowerFailureDiagnosed");
	const TCHAR* NeuroPowerDiagnosisPrompt = TEXT("Inspect Feed Diagnostics");
	const TCHAR* NeuroPowerDiagnosisEntryId = TEXT("Log_NeuroPrimaryFeedDiagnostic");
	const TCHAR* NeuroPowerDiagnosisTitle = TEXT("NEUROGENETICS FEED DIAGNOSTIC");
	const TCHAR* NeuroPowerDiagnosisAuthor = TEXT("FACILITIES CONTROL");
	const TCHAR* NeuroPowerDiagnosisCategory = TEXT("Systems");
	const TCHAR* NeuroPowerDiagnosisBody =
		TEXT("PRIMARY FEED: ISOLATED\nEMERGENCY BACKUP: ACTIVE\nPRIMARY RECONNECT: INHIBITED\nFAULT HISTORY: REPEATING LOAD SPIKES — RESEARCH FLOOR");

	const FVector NeuroPowerDiagnosisLocation(-500.f, -2775.f, -1110.f);
	const FRotator NeuroPowerDiagnosisRotation(0.f, 180.f, 0.f);
	const FVector NeuroPowerDiagnosisScale = FVector::OneVector;
	const FVector NeuroPowerDiagnosisMeshRelLocation = FVector::ZeroVector;
	const FRotator NeuroPowerDiagnosisMeshRelRotation = FRotator::ZeroRotator;
	const FVector NeuroPowerDiagnosisMeshRelScale(0.8f, 0.2f, 1.6f);
	const float NeuroPowerDiagnosisInteractionRange = 200.f;
	const FVector NeuroPowerDiagnosisPanelLocation(-500.f, -2275.f, -1100.f);
	const float NeuroPowerDiagnosisPanelRange = 220.f;
	const float NeuroPowerDiagnosisExpectedPanelDistance = 500.1f;

	UStaticMeshComponent* NeuroPowerDiagnosis_FindPadMesh(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Actor->GetClass(), TEXT("PadMesh")))
		{
			if (UStaticMeshComponent* Named = Cast<UStaticMeshComponent>(Prop->GetObjectPropertyValue_InContainer(Actor)))
			{
				return Named;
			}
		}
		return Actor->FindComponentByClass<UStaticMeshComponent>();
	}

	void* NeuroPowerDiagnosis_LogEntryPtr(AActor* Actor)
	{
		FProperty* Property = FindInstanceProperty(Actor, TEXT("LogEntry"));
		const FStructProperty* Struct = CastField<FStructProperty>(Property);
		if (!Actor || !Struct)
		{
			return nullptr;
		}
		return Struct->ContainerPtrToValuePtr<void>(Actor);
	}

	FString NeuroPowerDiagnosis_ReadLogField(AActor* Actor, const TCHAR* Field)
	{
		void* Entry = NeuroPowerDiagnosis_LogEntryPtr(Actor);
		FProperty* Property = FindInstanceProperty(Actor, TEXT("LogEntry"));
		const FStructProperty* Struct = CastField<FStructProperty>(Property);
		if (!Entry || !Struct || !Struct->Struct)
		{
			return FString();
		}
		if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Struct->Struct, Field))
		{
			return NameProp->GetPropertyValue_InContainer(Entry).ToString();
		}
		if (FTextProperty* TextProp = FindFProperty<FTextProperty>(Struct->Struct, Field))
		{
			return TextProp->GetPropertyValue_InContainer(Entry).ToString();
		}
		return FString();
	}

	FString NeuroPowerDiagnosis_WriteLogField(AActor* Actor, const TCHAR* Field, const FString& Value, bool bName)
	{
		void* Entry = NeuroPowerDiagnosis_LogEntryPtr(Actor);
		FProperty* Property = FindInstanceProperty(Actor, TEXT("LogEntry"));
		const FStructProperty* Struct = CastField<FStructProperty>(Property);
		if (!Entry || !Struct || !Struct->Struct)
		{
			return TEXT("LogEntry struct missing.");
		}
		if (bName)
		{
			if (FNameProperty* NameProp = FindFProperty<FNameProperty>(Struct->Struct, Field))
			{
				NameProp->SetPropertyValue_InContainer(Entry, FName(*Value));
				return TEXT("");
			}
			return FString::Printf(TEXT("LogEntry.%s is not FName."), Field);
		}
		if (FTextProperty* TextProp = FindFProperty<FTextProperty>(Struct->Struct, Field))
		{
			TextProp->SetPropertyValue_InContainer(Entry, FText::FromString(Value));
			return TEXT("");
		}
		return FString::Printf(TEXT("LogEntry.%s is not FText."), Field);
	}

	FString NeuroPowerDiagnosis_SetNameProperty(AActor* Actor, const TCHAR* Name, const TCHAR* Value)
	{
		FProperty* Property = FindInstanceProperty(Actor, Name);
		if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			NameProp->SetPropertyValue_InContainer(Actor, FName(Value));
			return TEXT("");
		}
		return FString::Printf(TEXT("Property '%s' is not FName."), Name);
	}

	FString NeuroPowerDiagnosis_SetFloatProperty(AActor* Actor, const TCHAR* Name, float Value)
	{
		FProperty* Property = FindInstanceProperty(Actor, Name);
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			FloatProp->SetPropertyValue_InContainer(Actor, Value);
			return TEXT("");
		}
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			DoubleProp->SetPropertyValue_InContainer(Actor, static_cast<double>(Value));
			return TEXT("");
		}
		return FString::Printf(TEXT("Property '%s' is not float."), Name);
	}

	FString NeuroPowerDiagnosis_SetTextProperty(AActor* Actor, const TCHAR* Name, const TCHAR* Value)
	{
		FProperty* Property = FindInstanceProperty(Actor, Name);
		if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			TextProp->SetPropertyValue_InContainer(Actor, FText::FromString(Value));
			return TEXT("");
		}
		return FString::Printf(TEXT("Property '%s' is not FText."), Name);
	}

	FString NeuroPowerDiagnosis_CheckNameOrText(AActor* Actor, const TCHAR* PropertyName, const TCHAR* Expected)
	{
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		if (!Property)
		{
			return FString::Printf(TEXT("Missing property '%s'."), PropertyName);
		}
		FString Live;
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			Live = NameProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			Live = TextProp->GetPropertyValue_InContainer(Actor).ToString();
		}
		if (!Live.Equals(Expected, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("%s live '%s' expected '%s'"), PropertyName, *Live, Expected);
		}
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_CheckFloat(AActor* Actor, const TCHAR* PropertyName, float Expected)
	{
		FProperty* Property = FindInstanceProperty(Actor, PropertyName);
		float Live = 0.f;
		if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			Live = FloatProp->GetPropertyValue_InContainer(Actor);
		}
		else if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			Live = static_cast<float>(DoubleProp->GetPropertyValue_InContainer(Actor));
		}
		else
		{
			return FString::Printf(TEXT("Missing float '%s'."), PropertyName);
		}
		if (!FMath::IsNearlyEqual(Live, Expected, 0.01f))
		{
			return FString::Printf(TEXT("%s live %f expected %f"), PropertyName, Live, Expected);
		}
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_MeshMismatch(AActor* Actor)
	{
		UStaticMeshComponent* Mesh = NeuroPowerDiagnosis_FindPadMesh(Actor);
		if (!Mesh)
		{
			return TEXT("PadMesh component missing.");
		}
		const FString Path = Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName() : TEXT("");
		if (!Path.Contains(TEXT("/Engine/BasicShapes/Cube")))
		{
			return FString::Printf(TEXT("PadMesh asset '%s' expected Engine Cube blockout."), *Path);
		}
		if (!LocationMatches(Mesh->GetRelativeLocation(), NeuroPowerDiagnosisMeshRelLocation))
		{
			return TEXT("PadMesh relative location must be (0,0,0).");
		}
		if (!RotationMatches(Mesh->GetRelativeRotation(), NeuroPowerDiagnosisMeshRelRotation))
		{
			return TEXT("PadMesh relative rotation must be (0,0,0).");
		}
		if (!ScaleMatches(Mesh->GetRelativeScale3D(), NeuroPowerDiagnosisMeshRelScale))
		{
			return TEXT("PadMesh relative scale must be (0.8,0.2,1.6) temporary blockout.");
		}
		if (Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			return TEXT("PadMesh collision must be NoCollision.");
		}
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_ActorMismatch(AActor* Actor)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!ClassName(Actor).Equals(NeuroPowerDiagnosisClassName, ESearchCase::CaseSensitive)
			&& !ClassName(Actor).Contains(TEXT("ProjectOrganoidDataPad")))
		{
			return FString::Printf(TEXT("class '%s' expected ProjectOrganoidDataPad"), *ClassName(Actor));
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (const FString Xform = TransformMismatch(
				Actor, NeuroPowerDiagnosisLocation, NeuroPowerDiagnosisRotation, NeuroPowerDiagnosisScale);
			!Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckFloat(Actor, TEXT("InteractionRange"), NeuroPowerDiagnosisInteractionRange); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bIsInteractable"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bHasBeenRead"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = CheckBoolProperty(Actor, TEXT("bBroadcastGenericDataPadEvent"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("RequiredObjectiveIdForInteraction"), NeuroPowerDiagnosisRequiredObjective); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("ObjectiveEventId"), NeuroPowerDiagnosisEvent); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_CheckNameOrText(Actor, TEXT("InteractionPrompt"), NeuroPowerDiagnosisPrompt); !Error.IsEmpty())
		{
			return Error;
		}
		if (!NeuroPowerDiagnosis_ReadLogField(Actor, TEXT("EntryId")).Equals(NeuroPowerDiagnosisEntryId, ESearchCase::CaseSensitive))
		{
			return TEXT("LogEntry.EntryId mismatch.");
		}
		if (!NeuroPowerDiagnosis_ReadLogField(Actor, TEXT("Title")).Equals(NeuroPowerDiagnosisTitle, ESearchCase::CaseSensitive))
		{
			return TEXT("LogEntry.Title mismatch.");
		}
		if (!NeuroPowerDiagnosis_ReadLogField(Actor, TEXT("Author")).Equals(NeuroPowerDiagnosisAuthor, ESearchCase::CaseSensitive))
		{
			return TEXT("LogEntry.Author mismatch.");
		}
		if (!NeuroPowerDiagnosis_ReadLogField(Actor, TEXT("Category")).Equals(NeuroPowerDiagnosisCategory, ESearchCase::CaseSensitive))
		{
			return TEXT("LogEntry.Category must be exactly Systems.");
		}
		if (!NeuroPowerDiagnosis_ReadLogField(Actor, TEXT("Body")).Equals(NeuroPowerDiagnosisBody, ESearchCase::CaseSensitive))
		{
			return TEXT("LogEntry.Body mismatch.");
		}
		if (const FString MeshError = NeuroPowerDiagnosis_MeshMismatch(Actor); !MeshError.IsEmpty())
		{
			return MeshError;
		}
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_ApplyFields(AActor* Actor)
	{
		if (const FString Error = NeuroPowerDiagnosis_SetFloatProperty(Actor, TEXT("InteractionRange"), NeuroPowerDiagnosisInteractionRange); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Actor, TEXT("bIsInteractable"), true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Actor, TEXT("bHasBeenRead"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = SetNamedPropertyFromBool(Actor, TEXT("bBroadcastGenericDataPadEvent"), false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(Actor, TEXT("RequiredObjectiveIdForInteraction"), NeuroPowerDiagnosisRequiredObjective); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetNameProperty(Actor, TEXT("ObjectiveEventId"), NeuroPowerDiagnosisEvent); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_SetTextProperty(Actor, TEXT("InteractionPrompt"), NeuroPowerDiagnosisPrompt); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_WriteLogField(Actor, TEXT("EntryId"), NeuroPowerDiagnosisEntryId, true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_WriteLogField(Actor, TEXT("Title"), NeuroPowerDiagnosisTitle, false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_WriteLogField(Actor, TEXT("Author"), NeuroPowerDiagnosisAuthor, false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_WriteLogField(Actor, TEXT("Category"), NeuroPowerDiagnosisCategory, true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroPowerDiagnosis_WriteLogField(Actor, TEXT("Body"), NeuroPowerDiagnosisBody, false); !Error.IsEmpty())
		{
			return Error;
		}

		UStaticMeshComponent* Mesh = NeuroPowerDiagnosis_FindPadMesh(Actor);
		if (!Mesh)
		{
			return TEXT("PadMesh missing during apply.");
		}
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, NeuroPowerDiagnosisCubeMeshPath);
		if (!Cube)
		{
			return TEXT("Failed to load /Engine/BasicShapes/Cube.Cube temporary blockout mesh.");
		}
		Mesh->SetStaticMesh(Cube);
		Mesh->SetRelativeLocation(NeuroPowerDiagnosisMeshRelLocation);
		Mesh->SetRelativeRotation(NeuroPowerDiagnosisMeshRelRotation);
		Mesh->SetRelativeScale3D(NeuroPowerDiagnosisMeshRelScale);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_PlacementGeometry(UWorld* World)
	{
		TArray<AActor*> Panels = FindByExactLabel(World, TEXT("PowerPanel_NeuroBackup"));
		if (Panels.Num() != 1)
		{
			return FString::Printf(TEXT("PowerPanel_NeuroBackup count=%d expected=1."), Panels.Num());
		}
		AActor* Panel = Panels[0];
		if (const FString Identity = NeuroPowerFailureDiscovery_PanelIdentity(Panel); !Identity.IsEmpty())
		{
			return FString::Printf(TEXT("PowerPanel_NeuroBackup: %s"), *Identity);
		}
		const float Dist = FVector::Dist(Panel->GetActorLocation(), NeuroPowerDiagnosisLocation);
		if (!FMath::IsNearlyEqual(Dist, NeuroPowerDiagnosisExpectedPanelDistance, 0.15f))
		{
			return FString::Printf(
				TEXT("Panel-to-pad distance %.3f expected ~%.1f uu (N2S)."),
				Dist,
				NeuroPowerDiagnosisExpectedPanelDistance);
		}
		const float CombinedRadii = NeuroPowerDiagnosisPanelRange + NeuroPowerDiagnosisInteractionRange;
		if (CombinedRadii >= Dist)
		{
			return FString::Printf(
				TEXT("Interaction radii sum %.0f must stay < distance %.3f (disjoint)."),
				CombinedRadii,
				Dist);
		}

		// Floor/contact: pad must sit above NeuroGenetics floor plate Z and not overlap foreign labels at the target.
		TArray<AActor*> Floor = FindOwnedByExactLabel(World, TEXT("NeuroGenetics_FloorPlate"), NeuroPackage);
		if (Floor.Num() != 1)
		{
			return FString::Printf(TEXT("NeuroGenetics_FloorPlate count=%d expected=1."), Floor.Num());
		}
		if (NeuroPowerDiagnosisLocation.Z < Floor[0]->GetActorLocation().Z)
		{
			return TEXT("Diagnostic location is below NeuroGenetics_FloorPlate.");
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Other = *It;
			if (!Other || ActorLabel(Other).Equals(NeuroPowerDiagnosisLabel, ESearchCase::CaseSensitive))
			{
				continue;
			}
			if (FVector::Dist(Other->GetActorLocation(), NeuroPowerDiagnosisLocation) <= 5.f)
			{
				return FString::Printf(
					TEXT("Conflicting actor '%s' within 5uu of diagnostic target."),
					*ActorLabel(Other));
			}
		}
		return TEXT("");
	}

	FString NeuroPowerDiagnosis_KeepList(UWorld* World)
	{
		if (const FString Error = NeuroPowerFailureDiscovery_KeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = GuardExistingActor(
				World, TEXT("ResearchStation_NeuroGenetics"), FVector(800.f, -1600.f, -1100.f), NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list ResearchStation_NeuroGenetics: %s"), *Error);
		}
		if (const FString Error = GuardExistingActor(
				World, TEXT("Hazard_ScrubberLeak"), FVector(-1950.f, -1650.f, -1000.f), NeuroPackage);
			!Error.IsEmpty())
		{
			return FString::Printf(TEXT("Keep-list Hazard_ScrubberLeak: %s"), *Error);
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroPowerDiagnostic(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_power_diagnostic does not save or compile.");
		}
		const FString Spec = GetString(Args, TEXT("spec"), NeuroPowerDiagnosisSpec);
		if (!Spec.Equals(NeuroPowerDiagnosisSpec, ESearchCase::CaseSensitive))
		{
			return TEXT("spec must be neuro_power_diagnostic_v1.");
		}
		// Reject arbitrary client overrides — only fixed native constants are allowed.
		if (Args.IsValid())
		{
			static const TCHAR* ForbiddenKeys[] = {
				TEXT("label"), TEXT("class"), TEXT("class_path"), TEXT("location"), TEXT("rotation"), TEXT("scale"),
				TEXT("mesh"), TEXT("objective"), TEXT("event"), TEXT("prompt"), TEXT("entry_id"), TEXT("title"),
				TEXT("author"), TEXT("body"), TEXT("category"), TEXT("package"), TEXT("destination_package"),
				TEXT("interaction_range"), TEXT("power_state"), TEXT("restored_state")
			};
			for (const TCHAR* Key : ForbiddenKeys)
			{
				if (Args->HasField(Key))
				{
					return FString::Printf(
						TEXT("Arbitrary argument '%s' is refused. spawn_neuro_power_diagnostic uses fixed native constants only."),
						Key);
				}
			}
		}
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before preparing this write.");
		}
		if (!IsInGameThread())
		{
			return TEXT("spawn_neuro_power_diagnostic preflight must run on the game thread.");
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			return CleanError;
		}
		if (const FString KeepError = NeuroPowerDiagnosis_KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}
		if (const FString PlaceError = NeuroPowerDiagnosis_PlacementGeometry(World); !PlaceError.IsEmpty())
		{
			return PlaceError;
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerDiagnosisLabel);
		bool bAlreadyExact = false;
		if (Matches.Num() > 1)
		{
			return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), NeuroPowerDiagnosisLabel, Matches.Num());
		}
		if (Matches.Num() == 1)
		{
			const FString Mismatch = NeuroPowerDiagnosis_ActorMismatch(Matches[0]);
			if (!Mismatch.IsEmpty())
			{
				return FString::Printf(
					TEXT("%s exists but mismatches: %s. Fail closed — no opportunistic repair."),
					NeuroPowerDiagnosisLabel,
					*Mismatch);
			}
			bAlreadyExact = true;
		}
		else if (!LoadClass<AActor>(nullptr, NeuroPowerDiagnosisClassPath))
		{
			return TEXT("AProjectOrganoidDataPad class is not loaded.");
		}

		const TArray<FString> Dirty = CollectDirtyPackageNamesSorted();
		Before->SetStringField(TEXT("spec"), NeuroPowerDiagnosisSpec);
		Before->SetStringField(TEXT("action"), NeuroPowerDiagnosisAction);
		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Before->SetBoolField(TEXT("packages_clean"), Dirty.Num() == 0);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(Dirty));
		Before->SetStringField(TEXT("visual_note"), TEXT("Temporary Engine Cube blockout kiosk — not final art."));
		if (Matches.Num() == 1)
		{
			Before->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
		}

		Proposed->SetStringField(TEXT("spec"), NeuroPowerDiagnosisSpec);
		Proposed->SetStringField(TEXT("label"), NeuroPowerDiagnosisLabel);
		Proposed->SetStringField(TEXT("class"), NeuroPowerDiagnosisClassName);
		Proposed->SetStringField(TEXT("destination_package"), NeuroPackage);
		Proposed->SetArrayField(TEXT("location"), Vec(NeuroPowerDiagnosisLocation));
		Proposed->SetArrayField(TEXT("rotation"), Vec(FVector(0.f, 180.f, 0.f)));
		Proposed->SetArrayField(TEXT("scale"), Vec(NeuroPowerDiagnosisScale));
		Proposed->SetNumberField(TEXT("interaction_range"), NeuroPowerDiagnosisInteractionRange);
		Proposed->SetStringField(TEXT("required_objective"), NeuroPowerDiagnosisRequiredObjective);
		Proposed->SetStringField(TEXT("objective_event"), NeuroPowerDiagnosisEvent);
		Proposed->SetBoolField(TEXT("broadcast_generic"), false);
		Proposed->SetBoolField(TEXT("unread"), true);
		Proposed->SetStringField(TEXT("prompt"), NeuroPowerDiagnosisPrompt);
		Proposed->SetStringField(TEXT("entry_id"), NeuroPowerDiagnosisEntryId);
		Proposed->SetStringField(TEXT("title"), NeuroPowerDiagnosisTitle);
		Proposed->SetStringField(TEXT("author"), NeuroPowerDiagnosisAuthor);
		Proposed->SetStringField(TEXT("category"), NeuroPowerDiagnosisCategory);
		Proposed->SetStringField(TEXT("body"), NeuroPowerDiagnosisBody);
		Proposed->SetStringField(TEXT("mesh"), NeuroPowerDiagnosisCubeMeshPath);
		Proposed->SetArrayField(TEXT("mesh_rel_scale"), Vec(NeuroPowerDiagnosisMeshRelScale));
		Proposed->SetStringField(TEXT("mesh_collision"), TEXT("NoCollision"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetBoolField(TEXT("compile"), false);
		Proposed->SetBoolField(TEXT("invoke_interact"), false);
		Proposed->SetBoolField(TEXT("set_sector_power"), false);
		Proposed->SetBoolField(TEXT("already_exact"), bAlreadyExact);
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetStringField(
			TEXT("result"),
			bAlreadyExact
				? TEXT("DataPad_NeuroPowerDiagnostics already matches neuro_power_diagnostic_v1. Execute is a clean no-op. Requires all packages clean. Temporary Cube blockout — not final art.")
				: TEXT("Spawn one temporary blockout DataPad_NeuroPowerDiagnostics (Engine Cube kiosk) on Neuro. Configure gate/event/log/mesh. Does not interact, change power, fire events, save, or compile. Not final art."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroPowerDiagnostic(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(
				TEXT("wrong_thread"),
				TEXT("spawn_neuro_power_diagnostic must run on the game thread. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroPowerDiagnostic(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("preflight_failed"),
				FString::Printf(TEXT("ZERO writes. %s"), *PreflightError),
				MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = nullptr;
		if (const FString CleanError = GuardNeuroCh4PackagesLoadedAndClean(World); !CleanError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("packages_dirty"), FString::Printf(TEXT("ZERO writes. %s"), *CleanError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString KeepError = NeuroPowerDiagnosis_KeepList(World); !KeepError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("keep_list"), FString::Printf(TEXT("ZERO writes. %s"), *KeepError), MakeShared<FBridgeChange>(Change));
		}
		if (const FString PlaceError = NeuroPowerDiagnosis_PlacementGeometry(World); !PlaceError.IsEmpty())
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("placement"), FString::Printf(TEXT("ZERO writes. %s"), *PlaceError), MakeShared<FBridgeChange>(Change));
		}

		ULevel* TargetLevel = FindLoadedLevelByPackage(World, NeuroPackage);
		if (!TargetLevel)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("not_found"), TEXT("Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TArray<AActor*> Matches = FindByExactLabel(World, NeuroPowerDiagnosisLabel);
		if (Matches.Num() == 1 && NeuroPowerDiagnosis_ActorMismatch(Matches[0]).IsEmpty())
		{
			const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
			if (DirtyAfter.Num() != 0)
			{
				Change.Status = TEXT("execute_aborted_dirty_noop");
				return FailAudit(
					TEXT("packages_dirty"),
					FString::Printf(
						TEXT("No-op exact actor path found dirty packages: %s. Hard stop. Do not save."),
						*FString::Join(DirtyAfter, TEXT(","))),
					MakeShared<FBridgeChange>(Change));
			}
			Change.bExecuted = true;
			Change.ExecutedAt = NowIso();
			Change.bSavePerformed = false;
			Change.Status = TEXT("executed_noop");
			Change.After = MakeShared<FJsonObject>();
			Change.After->SetStringField(TEXT("result"), TEXT("already_exact_noop"));
			Change.After->SetStringField(TEXT("label"), NeuroPowerDiagnosisLabel);
			Change.After->SetBoolField(TEXT("spawned"), false);
			Change.After->SetBoolField(TEXT("save_performed"), false);
			Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Matches[0]));
			Change.After->SetStringField(TEXT("visual_note"), TEXT("Temporary Engine Cube blockout kiosk — not final art."));
			LogAudit(TEXT("execute"), Change);
			return Ok(AuditBase(Change));
		}
		if (Matches.Num() != 0)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(
				TEXT("mismatch"),
				TEXT("DataPad_NeuroPowerDiagnostics exists but is not exact. Fail closed. ZERO writes."),
				MakeShared<FBridgeChange>(Change));
		}

		UClass* PadClass = LoadClass<AActor>(nullptr, NeuroPowerDiagnosisClassPath);
		if (!PadClass)
		{
			Change.Status = TEXT("execute_aborted_preflight");
			return FailAudit(TEXT("class_missing"), TEXT("AProjectOrganoidDataPad vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		AActor* Spawned = nullptr;
		{
			const FScopedTransaction Transaction(
				NSLOCTEXT("OrganoidAIBridge", "SpawnNeuroPowerDiagnostic", "Spawn Neuro Power Diagnostic (temporary blockout)"));
			FActorSpawnParameters Params;
			Params.OverrideLevel = TargetLevel;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Params.ObjectFlags = RF_Transactional;
			Spawned = World->SpawnActor<AActor>(
				PadClass, NeuroPowerDiagnosisLocation, NeuroPowerDiagnosisRotation, Params);
			if (!Spawned)
			{
				Change.Status = TEXT("execute_failed");
				return FailAudit(TEXT("spawn_failed"), TEXT("SpawnActor returned null. ZERO remaining writes."), MakeShared<FBridgeChange>(Change));
			}
			Spawned->SetActorLabel(NeuroPowerDiagnosisLabel, true);
			Spawned->SetActorScale3D(NeuroPowerDiagnosisScale);
			if (const FString ApplyError = NeuroPowerDiagnosis_ApplyFields(Spawned); !ApplyError.IsEmpty())
			{
				Spawned->Destroy();
				Spawned = nullptr;
				Change.Status = TEXT("execute_failed_rolled_back");
				return FailAudit(
					TEXT("configure_failed"),
					FString::Printf(TEXT("%s. Spawned actor destroyed. Proposal terminated. Do not save. Do not retry same change."), *ApplyError),
					MakeShared<FBridgeChange>(Change));
			}
			Spawned->MarkPackageDirty();
			if (const FString AfterError = NeuroPowerDiagnosis_ActorMismatch(Spawned); !AfterError.IsEmpty())
			{
				Spawned->Destroy();
				Spawned = nullptr;
				Change.Status = TEXT("execute_failed_rolled_back");
				return FailAudit(
					TEXT("postcondition_failed"),
					FString::Printf(TEXT("%s. Spawned actor destroyed. Proposal terminated. Do not save."), *AfterError),
					MakeShared<FBridgeChange>(Change));
			}
		}

		if (const FString KeepError = NeuroPowerDiagnosis_KeepList(World); !KeepError.IsEmpty())
		{
			if (Spawned)
			{
				Spawned->Destroy();
			}
			Change.Status = TEXT("execute_failed_rolled_back");
			return FailAudit(
				TEXT("keep_list_failed"),
				FString::Printf(TEXT("%s. Spawned actor destroyed. Do not save."), *KeepError),
				MakeShared<FBridgeChange>(Change));
		}

		const TArray<FString> DirtyAfter = CollectDirtyPackageNamesSorted();
		TArray<FString> ExpectedDirty;
		ExpectedDirty.Add(NormalizePackage(NeuroPackage));
		if (DirtyAfter != ExpectedDirty)
		{
			if (Spawned)
			{
				Spawned->Destroy();
			}
			Change.Status = TEXT("execute_failed_rolled_back");
			return FailAudit(
				TEXT("unexpected_dirt"),
				FString::Printf(
					TEXT("Expected only Neuro dirty. Live dirty=[%s]. Spawned actor destroyed. Do not save."),
					*FString::Join(DirtyAfter, TEXT(","))),
				MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetStringField(TEXT("label"), NeuroPowerDiagnosisLabel);
		Change.After->SetStringField(TEXT("class"), ClassName(Spawned));
		Change.After->SetStringField(TEXT("owning_package"), ActorOwningPackage(Spawned));
		Change.After->SetArrayField(TEXT("location"), Vec(Spawned->GetActorLocation()));
		Change.After->SetBoolField(TEXT("spawned"), true);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("interacted"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(DirtyAfter));
		Change.After->SetObjectField(TEXT("actor"), ActorSnapshot(Spawned));
		Change.After->SetStringField(TEXT("category"), NeuroPowerDiagnosisCategory);
		Change.After->SetStringField(
			TEXT("visual_note"),
			TEXT("Temporary Engine Cube blockout kiosk — replaceable presentation, not final art."));
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
