	const TCHAR* NeuroCh3DataPadClassPath = TEXT("/Script/ProjectOrganoid.ProjectOrganoidDataPad");

	struct FNeuroCh3MeshSpec
	{
		const TCHAR* Label;
		FVector Location;
		FRotator Rotation;
		FVector Scale;
		ENeuroArrivalMesh Mesh;
	};

	struct FNeuroCh3PadSpec
	{
		const TCHAR* Label;
		FVector Location;
		const TCHAR* EntryId;
		const TCHAR* Title;
		const TCHAR* Body;
		const TCHAR* Author;
	};

	const FNeuroCh3MeshSpec NeuroCh3Meshes[] = {
		{TEXT("Neuro_Ch3_ContainmentUnit"), FVector(350.f, -1680.f, -1140.f), FRotator::ZeroRotator, FVector(1.40f, 1.20f, 1.60f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_ContainmentHatch"), FVector(430.f, -1660.f, -1100.f), FRotator(0.f, 0.f, -35.f), FVector(1.20f, 0.08f, 1.00f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_Stripe_1"), FVector(350.f, -1580.f, -1196.f), FRotator::ZeroRotator, FVector(2.40f, 0.18f, 0.04f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_Stripe_2"), FVector(350.f, -1780.f, -1196.f), FRotator::ZeroRotator, FVector(2.40f, 0.18f, 0.04f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_Stripe_3"), FVector(250.f, -1680.f, -1196.f), FRotator(0.f, 90.f, 0.f), FVector(2.00f, 0.18f, 0.04f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_LockdownSign"), FVector(220.f, -1560.f, -1080.f), FRotator::ZeroRotator, FVector(0.06f, 0.70f, 0.55f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_Glass_1"), FVector(400.f, -1720.f, -1188.f), FRotator::ZeroRotator, FVector(0.35f, 0.28f, 0.03f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_Glass_2"), FVector(310.f, -1640.f, -1185.f), FRotator(0.f, 25.f, 0.f), FVector(0.28f, 0.22f, 0.03f), ENeuroArrivalMesh::Cube},
		{TEXT("Neuro_Ch3_EmergencyKit"), FVector(180.f, -1720.f, -1175.f), FRotator::ZeroRotator, FVector(0.35f, 0.28f, 0.22f), ENeuroArrivalMesh::Cube},
	};

	const FNeuroCh3PadSpec NeuroCh3Pads[] = {
		{
			TEXT("DataPad_NeuroContainment"),
			FVector(380.f, -1580.f, -1110.f),
			TEXT("Pad_Neuro_ContainmentAnomaly"),
			TEXT("Containment anomaly noted"),
			TEXT("Containment variance is visible in this lab. Breach state is not explained. Cause unknown."),
			TEXT("Unknown")
		},
		{
			TEXT("DataPad_NeuroResearchFailure"),
			FVector(280.f, -1760.f, -1110.f),
			TEXT("Pad_Neuro_ContainmentVariance"),
			TEXT("Research log: containment variance"),
			TEXT("Research notes incomplete. Containment did not hold as recorded. No mechanism identified."),
			TEXT("Unknown")
		},
	};

	void* NeuroCh3LogEntryPtr(AActor* Actor)
	{
		FProperty* Property = FindInstanceProperty(Actor, TEXT("LogEntry"));
		const FStructProperty* Struct = CastField<FStructProperty>(Property);
		if (!Actor || !Struct)
		{
			return nullptr;
		}
		return Struct->ContainerPtrToValuePtr<void>(Actor);
	}

	FString NeuroCh3ReadLogField(AActor* Actor, const TCHAR* Field)
	{
		void* Entry = NeuroCh3LogEntryPtr(Actor);
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

	FString NeuroCh3WriteLogField(AActor* Actor, const TCHAR* Field, const FString& Value, bool bName)
	{
		void* Entry = NeuroCh3LogEntryPtr(Actor);
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

	FString NeuroCh3PadMismatch(AActor* Actor, const FNeuroCh3PadSpec& Spec)
	{
		if (!Actor)
		{
			return TEXT("missing");
		}
		if (!PackagesEqual(ActorOwningPackage(Actor), NeuroPackage))
		{
			return FString::Printf(TEXT("owner '%s' is not NeuroGenetics"), *ActorOwningPackage(Actor));
		}
		if (!ClassName(Actor).Contains(TEXT("ProjectOrganoidDataPad")))
		{
			return FString::Printf(TEXT("class '%s' is not ProjectOrganoidDataPad"), *ClassName(Actor));
		}
		if (const FString Xform = TransformMismatch(Actor, Spec.Location, FRotator::ZeroRotator, FVector::OneVector); !Xform.IsEmpty())
		{
			return Xform;
		}
		if (const FString Broadcast = CheckBoolProperty(Actor, TEXT("bBroadcastGenericDataPadEvent"), false); !Broadcast.IsEmpty())
		{
			return Broadcast;
		}
		if (!NeuroCh3ReadLogField(Actor, TEXT("EntryId")).Equals(Spec.EntryId, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("EntryId '%s' expected '%s'"), *NeuroCh3ReadLogField(Actor, TEXT("EntryId")), Spec.EntryId);
		}
		if (!NeuroCh3ReadLogField(Actor, TEXT("Title")).Equals(Spec.Title, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Title '%s' expected '%s'"), *NeuroCh3ReadLogField(Actor, TEXT("Title")), Spec.Title);
		}
		if (!NeuroCh3ReadLogField(Actor, TEXT("Body")).Equals(Spec.Body, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Body mismatch."));
		}
		if (!NeuroCh3ReadLogField(Actor, TEXT("Author")).Equals(Spec.Author, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("Author '%s' expected '%s'"), *NeuroCh3ReadLogField(Actor, TEXT("Author")), Spec.Author);
		}
		return TEXT("");
	}

	FString ApplyNeuroCh3PadFields(AActor* Actor, const FNeuroCh3PadSpec& Spec)
	{
		if (const FString Broadcast = SetNamedPropertyFromBool(Actor, TEXT("bBroadcastGenericDataPadEvent"), false); !Broadcast.IsEmpty())
		{
			return Broadcast;
		}
		if (FProperty* Objective = FindInstanceProperty(Actor, TEXT("ObjectiveEventId")))
		{
			if (FNameProperty* NameProp = CastField<FNameProperty>(Objective))
			{
				NameProp->SetPropertyValue_InContainer(Actor, NAME_None);
			}
		}
		if (const FString Error = NeuroCh3WriteLogField(Actor, TEXT("EntryId"), Spec.EntryId, true); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroCh3WriteLogField(Actor, TEXT("Title"), Spec.Title, false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroCh3WriteLogField(Actor, TEXT("Body"), Spec.Body, false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroCh3WriteLogField(Actor, TEXT("Author"), Spec.Author, false); !Error.IsEmpty())
		{
			return Error;
		}
		if (const FString Error = NeuroCh3WriteLogField(Actor, TEXT("Category"), TEXT("Facility"), true); !Error.IsEmpty())
		{
			return Error;
		}
		return TEXT("");
	}

	FString GuardNeuroCh3KeepList(UWorld* World)
	{
		if (const FString Error = GuardNeuroArrivalKeepList(World); !Error.IsEmpty())
		{
			return Error;
		}
		for (const FNeuroArrivalPropSpec& Spec : NeuroArrivalProps)
		{
			TArray<AActor*> Matches = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage);
			if (Matches.Num() != 1)
			{
				return FString::Printf(TEXT("Keep-list %s Neuro count=%d expected=1."), Spec.Label, Matches.Num());
			}
			if (const FString Mismatch = NeuroArrivalPropMismatch(Matches[0], Spec); !Mismatch.IsEmpty())
			{
				return FString::Printf(TEXT("Keep-list %s: %s"), Spec.Label, *Mismatch);
			}
		}
		for (const FNeuroArrivalStaleSpec& Spec : NeuroArrivalStale)
		{
			const int32 Count = FindOwnedByExactLabel(World, Spec.Label, NeuroPackage).Num();
			if (Count != 0)
			{
				return FString::Printf(TEXT("%s must stay removed (count=%d)."), Spec.Label, Count);
			}
		}
		return TEXT("");
	}

	FString PreflightSpawnNeuroCh3ContainmentEvidence(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. spawn_neuro_ch3_containment_evidence does not save or compile.");
		}

		UWorld* World = GetEditorWorld();
		if (const FString KeepError = GuardNeuroCh3KeepList(World); !KeepError.IsEmpty())
		{
			return KeepError;
		}

		int32 MeshesPresent = 0;
		for (const FNeuroCh3MeshSpec& Spec : NeuroCh3Meshes)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() > 1)
			{
				return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), Spec.Label, Matches.Num());
			}
			if (Matches.Num() == 1)
			{
				FNeuroArrivalPropSpec AsArrival;
				AsArrival.Label = Spec.Label;
				AsArrival.Location = Spec.Location;
				AsArrival.Rotation = Spec.Rotation;
				AsArrival.Scale = Spec.Scale;
				AsArrival.Mesh = Spec.Mesh;
				if (const FString Mismatch = NeuroArrivalPropMismatch(Matches[0], AsArrival); !Mismatch.IsEmpty())
				{
					return FString::Printf(TEXT("Mismatched %s: %s. Abort."), Spec.Label, *Mismatch);
				}
				++MeshesPresent;
			}
		}

		int32 PadsPresent = 0;
		for (const FNeuroCh3PadSpec& Spec : NeuroCh3Pads)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() > 1)
			{
				return FString::Printf(TEXT("%s count=%d. Abort rather than stack."), Spec.Label, Matches.Num());
			}
			if (Matches.Num() == 1)
			{
				if (const FString Mismatch = NeuroCh3PadMismatch(Matches[0], Spec); !Mismatch.IsEmpty())
				{
					return FString::Printf(TEXT("Mismatched %s: %s. Abort."), Spec.Label, *Mismatch);
				}
				++PadsPresent;
			}
		}

		if (PadsPresent < UE_ARRAY_COUNT(NeuroCh3Pads))
		{
			if (!LoadClass<AActor>(nullptr, NeuroCh3DataPadClassPath))
			{
				return TEXT("AProjectOrganoidDataPad class is not loaded.");
			}
		}

		Before->SetStringField(TEXT("destination_package"), NeuroPackage);
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetNumberField(TEXT("meshes_present"), MeshesPresent);
		Before->SetNumberField(TEXT("pads_present"), PadsPresent);
		Proposed->SetNumberField(TEXT("mesh_count"), UE_ARRAY_COUNT(NeuroCh3Meshes));
		Proposed->SetNumberField(TEXT("pad_count"), UE_ARRAY_COUNT(NeuroCh3Pads));
		Proposed->SetBoolField(TEXT("idempotent"), true);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Spawn SE-pocket Ch #3 blockout (open containment unit, stripes, sign, glass) and two DataPads. bBroadcastGenericDataPadEvent=false. No power change. Does not save."));
		return TEXT("");
	}

	AActor* SpawnNeuroCh3Pad(UWorld* World, ULevel* TargetLevel, const FNeuroCh3PadSpec& Spec, FString& OutError)
	{
		OutError.Reset();
		UClass* PadClass = LoadClass<AActor>(nullptr, NeuroCh3DataPadClassPath);
		if (!PadClass)
		{
			OutError = TEXT("AProjectOrganoidDataPad class vanished.");
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.OverrideLevel = TargetLevel;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags = RF_Transactional;
		AActor* Actor = World->SpawnActor<AActor>(PadClass, Spec.Location, FRotator::ZeroRotator, Params);
		if (!Actor)
		{
			OutError = FString::Printf(TEXT("SpawnActor failed for %s."), Spec.Label);
			return nullptr;
		}
		Actor->SetActorLabel(Spec.Label, true);
		Actor->SetActorScale3D(FVector::OneVector);
		if (const FString FieldError = ApplyNeuroCh3PadFields(Actor, Spec); !FieldError.IsEmpty())
		{
			Actor->Destroy();
			OutError = FieldError;
			return nullptr;
		}
		Actor->MarkPackageDirty();
		if (const FString AfterError = NeuroCh3PadMismatch(Actor, Spec); !AfterError.IsEmpty())
		{
			Actor->Destroy();
			OutError = AfterError;
			return nullptr;
		}
		return Actor;
	}

	TSharedRef<FJsonObject> ExecuteSpawnNeuroCh3ContainmentEvidence(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("spawn_neuro_ch3_containment_evidence must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSpawnNeuroCh3ContainmentEvidence(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}

		UWorld* World = GetEditorWorld();
		ULevel* TargetLevel = World ? FindLoadedLevelByPackage(World, NeuroPackage) : nullptr;
		if (!World || !TargetLevel)
		{
			return FailAudit(TEXT("not_found"), TEXT("World or Neuro level vanished. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}

		int32 SpawnedMeshes = 0;
		int32 SpawnedPads = 0;
		int32 Already = 0;
		const FScopedTransaction Transaction(NSLOCTEXT("OrganoidAIBridge", "NeuroCh3Evidence", "Neuro Ch3 containment evidence"));

		for (const FNeuroCh3MeshSpec& Spec : NeuroCh3Meshes)
		{
			FNeuroArrivalPropSpec AsArrival;
			AsArrival.Label = Spec.Label;
			AsArrival.Location = Spec.Location;
			AsArrival.Rotation = Spec.Rotation;
			AsArrival.Scale = Spec.Scale;
			AsArrival.Mesh = Spec.Mesh;
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && NeuroArrivalPropMismatch(Matches[0], AsArrival).IsEmpty())
			{
				++Already;
				continue;
			}
			FString SpawnError;
			if (!SpawnNeuroArrivalProp(World, TargetLevel, AsArrival, SpawnError))
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("%s %s ZERO remaining writes."), Spec.Label, *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++SpawnedMeshes;
		}

		for (const FNeuroCh3PadSpec& Spec : NeuroCh3Pads)
		{
			TArray<AActor*> Matches = FindByExactLabel(World, Spec.Label);
			if (Matches.Num() == 1 && NeuroCh3PadMismatch(Matches[0], Spec).IsEmpty())
			{
				++Already;
				continue;
			}
			FString SpawnError;
			if (!SpawnNeuroCh3Pad(World, TargetLevel, Spec, SpawnError))
			{
				return FailAudit(TEXT("spawn_failed"), FString::Printf(TEXT("%s %s ZERO remaining writes."), Spec.Label, *SpawnError), MakeShared<FBridgeChange>(Change));
			}
			++SpawnedPads;
		}

		if (const FString KeepError = GuardNeuroCh3KeepList(World); !KeepError.IsEmpty())
		{
			return FailAudit(TEXT("keep_list_failed"), KeepError, MakeShared<FBridgeChange>(Change));
		}

		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetNumberField(TEXT("spawned_meshes"), SpawnedMeshes);
		Change.After->SetNumberField(TEXT("spawned_pads"), SpawnedPads);
		Change.After->SetNumberField(TEXT("already_present"), Already);
		Change.After->SetBoolField(TEXT("save_performed"), false);
		Change.After->SetBoolField(TEXT("power_changed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}
