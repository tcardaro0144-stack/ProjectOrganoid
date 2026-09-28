	const TCHAR* AlarmPulseAction = TEXT("set_alarm_pulse_oneshot");
	const TCHAR* AlarmPulseSpec = TEXT("alarm_pulse_oneshot_v1");
	const TCHAR* AlarmPulsePackage = TEXT("/Game/Audio/Ambient/SW_AlarmPulse");
	const TCHAR* AlarmPulseObjectPath = TEXT("/Game/Audio/Ambient/SW_AlarmPulse.SW_AlarmPulse");
	const TCHAR* NavRebuildAction = TEXT("rebuild_epitope_navmesh");
	const TCHAR* NavRebuildSpec = TEXT("epitope_navmesh_rebuild_v1");
	constexpr float AlarmPulseExpectedHz = 880.0f;
	constexpr float AlarmPulseHzTolerance = 40.0f;
	constexpr float AlarmPulseMinDuration = 1.05f;
	constexpr float AlarmPulseMaxDuration = 1.35f;

	USoundWave* LoadAlarmPulseWave()
	{
		return LoadObject<USoundWave>(nullptr, AlarmPulseObjectPath);
	}

	bool EstimateAlarmPulseHz(USoundWave* Wave, float& OutHz, float& OutDuration, FString& OutError)
	{
		OutHz = 0.0f;
		OutDuration = 0.0f;
		if (!Wave)
		{
			OutError = TEXT("SW_AlarmPulse is not loaded.");
			return false;
		}
		TArray<uint8> Pcm;
		uint32 SampleRate = 0;
		uint16 Channels = 0;
		if (!Wave->GetImportedSoundWaveData(Pcm, SampleRate, Channels) || SampleRate < 8000 || Channels < 1)
		{
			OutError = TEXT("SW_AlarmPulse PCM could not be read.");
			return false;
		}
		const int32 BytesPerFrame = static_cast<int32>(sizeof(int16) * Channels);
		if (BytesPerFrame <= 0 || Pcm.Num() < BytesPerFrame)
		{
			OutError = TEXT("SW_AlarmPulse PCM is empty.");
			return false;
		}
		const int32 FrameCount = Pcm.Num() / BytesPerFrame;
		// GetDuration() returns the 10000s looping sentinel while bLooping is true.
		OutDuration = static_cast<float>(FrameCount) / static_cast<float>(SampleRate);
		const int32 Start = FMath::Min(FrameCount, static_cast<int32>(SampleRate * 0.02f));
		const int32 End = FMath::Min(FrameCount, static_cast<int32>(SampleRate * 0.16f));
		if (End - Start < static_cast<int32>(SampleRate * 0.08f))
		{
			OutError = TEXT("SW_AlarmPulse pulse window is too short to measure 880 Hz.");
			return false;
		}
		const int16* Samples = reinterpret_cast<const int16*>(Pcm.GetData());
		int32 Crossings = 0;
		int16 Previous = 0;
		bool bHavePrevious = false;
		for (int32 Frame = Start; Frame < End; ++Frame)
		{
			int32 Sum = 0;
			for (uint16 Channel = 0; Channel < Channels; ++Channel)
			{
				Sum += Samples[(Frame * Channels) + Channel];
			}
			const int16 Sample = static_cast<int16>(Sum / Channels);
			if (bHavePrevious && ((Previous < 0 && Sample >= 0) || (Previous >= 0 && Sample < 0)))
			{
				++Crossings;
			}
			Previous = Sample;
			bHavePrevious = true;
		}
		const float Seconds = static_cast<float>(End - Start) / static_cast<float>(SampleRate);
		OutHz = (static_cast<float>(Crossings) * 0.5f) / Seconds;
		return true;
	}

	FString AlarmPulseIdentityError(USoundWave* Wave, float& OutHz, float& OutDuration)
	{
		FString MeasureError;
		if (!EstimateAlarmPulseHz(Wave, OutHz, OutDuration, MeasureError))
		{
			return MeasureError;
		}
		if (OutDuration < AlarmPulseMinDuration || OutDuration > AlarmPulseMaxDuration)
		{
			return FString::Printf(TEXT("SW_AlarmPulse duration %.3f is not the 1.2s pulse."), OutDuration);
		}
		if (FMath::Abs(OutHz - AlarmPulseExpectedHz) > AlarmPulseHzTolerance)
		{
			return FString::Printf(TEXT("SW_AlarmPulse measured %.1f Hz, expected 880 Hz."), OutHz);
		}
		return TEXT("");
	}

	bool IsNavRebuildMapPackage(const FString& PackageName)
	{
		return PackagesEqual(PackageName, AdminPackage)
			|| PackagesEqual(PackageName, NeuroPackage)
			|| PackagesEqual(PackageName, CryoPackage)
			|| PackagesEqual(PackageName, ComputePackage)
			|| PackagesEqual(PackageName, ReactorPackage)
			|| PackagesEqual(PackageName, EpitopePackage);
	}

	TSharedRef<FJsonObject> AlarmPulseSnapshot(USoundWave* Wave, float Hz, float Duration)
	{
		TSharedRef<FJsonObject> Snap = MakeShared<FJsonObject>();
		Snap->SetStringField(TEXT("object"), AlarmPulseObjectPath);
		Snap->SetBoolField(TEXT("looping"), Wave && Wave->bLooping);
		Snap->SetNumberField(TEXT("duration"), Duration);
		Snap->SetNumberField(TEXT("hz"), Hz);
		Snap->SetBoolField(TEXT("dirty"), Wave && Wave->GetOutermost() && Wave->GetOutermost()->IsDirty());
		return Snap;
	}

	FString PreflightSetAlarmPulseOneshot(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. set_alarm_pulse_oneshot does not save.");
		}
		if (!GetString(Args, TEXT("spec")).Equals(AlarmPulseSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be exactly '%s'."), AlarmPulseSpec);
		}
		if (!IsInGameThread())
		{
			return TEXT("set_alarm_pulse_oneshot preflight must run on the game thread.");
		}
		USoundWave* Wave = LoadAlarmPulseWave();
		float Hz = 0.0f;
		float Duration = 0.0f;
		if (const FString Identity = AlarmPulseIdentityError(Wave, Hz, Duration); !Identity.IsEmpty())
		{
			return Identity;
		}
		if (!Wave->bLooping)
		{
			return TEXT("SW_AlarmPulse looping must still be true.");
		}
		Before->SetObjectField(TEXT("alarm_pulse"), AlarmPulseSnapshot(Wave, Hz, Duration));
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		Proposed->SetBoolField(TEXT("looping"), false);
		Proposed->SetBoolField(TEXT("one_shot"), true);
		Proposed->SetNumberField(TEXT("linger_seconds"), 8.0);
		Proposed->SetNumberField(TEXT("hz"), Hz);
		Proposed->SetNumberField(TEXT("duration"), Duration);
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Set SW_AlarmPulse bLooping false. PCM, duration, and 880 Hz stay. The 8s combat linger does not restart the one-shot. Does not save."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteSetAlarmPulseOneshot(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("set_alarm_pulse_oneshot must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightSetAlarmPulseOneshot(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		USoundWave* Wave = LoadAlarmPulseWave();
		float Hz = 0.0f;
		float Duration = 0.0f;
		if (const FString Identity = AlarmPulseIdentityError(Wave, Hz, Duration); !Identity.IsEmpty() || !Wave)
		{
			return FailAudit(TEXT("identity_failed"), Identity.IsEmpty() ? TEXT("SW_AlarmPulse vanished.") : Identity, MakeShared<FBridgeChange>(Change));
		}
		const bool bAlreadyOneShot = !Wave->bLooping;
		if (!bAlreadyOneShot)
		{
			Wave->Modify();
			Wave->bLooping = false;
			Wave->MarkPackageDirty();
		}
		if (Wave->bLooping)
		{
			return FailAudit(TEXT("looping_still_true"), TEXT("SW_AlarmPulse bLooping stayed true. Asset not saved."), MakeShared<FBridgeChange>(Change));
		}
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.Status = TEXT("executed");
		Change.After = MakeShared<FJsonObject>();
		Change.After->SetBoolField(TEXT("already_one_shot"), bAlreadyOneShot);
		Change.After->SetObjectField(TEXT("alarm_pulse"), AlarmPulseSnapshot(Wave, Hz, Duration));
		Change.After->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		Change.After->SetBoolField(TEXT("save_performed"), false);
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	FString RequirePolishLevelsLoaded(UWorld* World)
	{
		if (!World)
		{
			return TEXT("No editor world.");
		}
		const TCHAR* Required[] = { EpitopePackage, AdminPackage, NeuroPackage, CryoPackage, ComputePackage, ReactorPackage };
		for (const TCHAR* PackageName : Required)
		{
			if (!FindLoadedLevelByPackage(World, PackageName))
			{
				return FString::Printf(TEXT("%s is not loaded. ZERO writes."), PackageName);
			}
		}
		return TEXT("");
	}

	FString PreflightRebuildEpitopeNavMesh(
		const TSharedPtr<FJsonObject>& Args,
		TSharedRef<FJsonObject> Before,
		TSharedRef<FJsonObject> Proposed)
	{
		if (GetBool(Args, TEXT("require_pie_stopped"), true) && GetPieWorld())
		{
			return TEXT("PIE is running. Stop Play before rebuilding paths.");
		}
		if (GetBool(Args, TEXT("save"), false) || GetBool(Args, TEXT("save_all"), false) || GetBool(Args, TEXT("compile"), false))
		{
			return TEXT("save/compile must be false. rebuild_epitope_navmesh does not save.");
		}
		if (!GetString(Args, TEXT("spec")).Equals(NavRebuildSpec, ESearchCase::CaseSensitive))
		{
			return FString::Printf(TEXT("spec must be exactly '%s'."), NavRebuildSpec);
		}
		if (!IsInGameThread())
		{
			return TEXT("rebuild_epitope_navmesh preflight must run on the game thread.");
		}
		UWorld* World = nullptr;
		if (const FString WorldError = RequireEpitopeEditorWorld(World); !WorldError.IsEmpty())
		{
			return WorldError;
		}
		if (const FString LevelError = RequirePolishLevelsLoaded(World); !LevelError.IsEmpty())
		{
			return LevelError;
		}
		Before->SetStringField(TEXT("persistent_package"), NormalizePackage(WorldPackageName(World)));
		Before->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		Before->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		Proposed->SetStringField(TEXT("build"), TEXT("BuildAIPaths"));
		Proposed->SetBoolField(TEXT("save"), false);
		Proposed->SetStringField(
			TEXT("result"),
			TEXT("Build Paths on the loaded Epitope world. Saves nothing. Mission, weapon, and adaptation packages must stay clean."));
		return TEXT("");
	}

	TSharedRef<FJsonObject> ExecuteRebuildEpitopeNavMesh(FBridgeChange& Change)
	{
		if (!IsInGameThread())
		{
			return FailAudit(TEXT("wrong_thread"), TEXT("rebuild_epitope_navmesh must run on the game thread. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		TSharedRef<FJsonObject> Before = MakeShared<FJsonObject>();
		TSharedRef<FJsonObject> Proposed = MakeShared<FJsonObject>();
		const FString PreflightError = PreflightRebuildEpitopeNavMesh(Change.Args, Before, Proposed);
		if (!PreflightError.IsEmpty())
		{
			return FailAudit(TEXT("preflight_failed"), FString::Printf(TEXT("ZERO writes. %s"), *PreflightError), MakeShared<FBridgeChange>(Change));
		}
		UWorld* World = GetEditorWorld();
		UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
		if (!World || !NavSys)
		{
			return FailAudit(TEXT("nav_system_missing"), TEXT("Navigation system is missing. ZERO writes."), MakeShared<FBridgeChange>(Change));
		}
		const TArray<FString> PreDirty = CollectDirtyPackageNamesSorted();
		const bool bBuilt = FEditorBuildUtils::EditorBuild(World, FBuildOptions::BuildAIPaths, false);
		const TArray<FString> PostDirty = CollectDirtyPackageNamesSorted();
		TArray<FString> Unexpected;
		TArray<FString> NavDirty;
		for (const FString& Name : PostDirty)
		{
			if (PreDirty.Contains(Name))
			{
				continue;
			}
			if (IsNavRebuildMapPackage(Name))
			{
				NavDirty.Add(Name);
			}
			else
			{
				Unexpected.Add(Name);
			}
		}
		TArray<TSharedPtr<FJsonValue>> RecastJson;
		int32 RecastCount = 0;
		int32 NeedsRebuildCount = 0;
		for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
		{
			ARecastNavMesh* NavMesh = *It;
			if (!NavMesh)
			{
				continue;
			}
			++RecastCount;
			const bool bNeeds = NavMesh->NeedsRebuild();
			if (bNeeds)
			{
				++NeedsRebuildCount;
			}
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("label"), NavMesh->GetActorLabel());
			Row->SetStringField(TEXT("name"), NavMesh->GetName());
			Row->SetStringField(TEXT("package"), NormalizePackage(ActorOwningPackage(NavMesh)));
			Row->SetBoolField(TEXT("needs_rebuild"), bNeeds);
			RecastJson.Add(MakeShared<FJsonValueObject>(Row));
		}
		const bool bNavigationDirty = NavSys->IsNavigationDirty();
		TSharedRef<FJsonObject> After = MakeShared<FJsonObject>();
		After->SetBoolField(TEXT("build_returned"), bBuilt);
		After->SetBoolField(TEXT("navigation_dirty"), bNavigationDirty);
		After->SetNumberField(TEXT("recast_count"), RecastCount);
		After->SetNumberField(TEXT("needs_rebuild_count"), NeedsRebuildCount);
		After->SetArrayField(TEXT("recast"), RecastJson);
		After->SetArrayField(TEXT("pre_dirty"), DirtyPackageJsonArray(PreDirty));
		After->SetArrayField(TEXT("post_dirty"), DirtyPackageJsonArray(PostDirty));
		After->SetArrayField(TEXT("nav_dirty"), DirtyPackageJsonArray(NavDirty));
		After->SetArrayField(TEXT("unexpected_new_dirty"), DirtyPackageJsonArray(Unexpected));
		After->SetBoolField(TEXT("save_performed"), false);
		Change.bExecuted = true;
		Change.ExecutedAt = NowIso();
		Change.bSavePerformed = false;
		Change.After = After;
		if (!bBuilt || bNavigationDirty || NeedsRebuildCount > 0 || RecastCount < 1 || Unexpected.Num() > 0)
		{
			Change.Status = TEXT("execute_postcondition_failed");
			LogAudit(TEXT("execute"), Change);
			return FailAudit(
				TEXT("nav_rebuild_failed"),
				FString::Printf(
					TEXT("Build Paths did not finish clean. built=%s navigation_dirty=%s recast=%d needs_rebuild=%d unexpected=%s. Maps not saved."),
					bBuilt ? TEXT("true") : TEXT("false"),
					bNavigationDirty ? TEXT("true") : TEXT("false"),
					RecastCount,
					NeedsRebuildCount,
					*FormatPackageList(Unexpected)),
				MakeShared<FBridgeChange>(Change));
		}
		Change.Status = TEXT("executed");
		LogAudit(TEXT("execute"), Change);
		return Ok(AuditBase(Change));
	}

	TSharedRef<FJsonObject> CmdInspectAlarmPulse(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		USoundWave* Wave = LoadAlarmPulseWave();
		float Hz = 0.0f;
		float Duration = 0.0f;
		FString MeasureError;
		const bool bMeasured = Wave && EstimateAlarmPulseHz(Wave, Hz, Duration, MeasureError);
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("found"), Wave != nullptr);
		Data->SetStringField(TEXT("object"), AlarmPulseObjectPath);
		Data->SetBoolField(TEXT("looping"), Wave && Wave->bLooping);
		Data->SetNumberField(TEXT("duration"), Duration);
		Data->SetNumberField(TEXT("hz"), Hz);
		Data->SetBoolField(TEXT("hz_measured"), bMeasured);
		Data->SetStringField(TEXT("measure_error"), MeasureError);
		Data->SetBoolField(TEXT("hz_is_880"), bMeasured && FMath::Abs(Hz - AlarmPulseExpectedHz) <= AlarmPulseHzTolerance);
		return Ok(Data);
	}

	TSharedRef<FJsonObject> CmdInspectEpitopeNavMesh(const TSharedPtr<FJsonObject>& /*Args*/)
	{
		UWorld* World = GetEditorWorld();
		TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("has_editor_world"), World != nullptr);
		Data->SetStringField(TEXT("persistent_package"), World ? NormalizePackage(WorldPackageName(World)) : TEXT(""));
		Data->SetBoolField(TEXT("pie_running"), GetPieWorld() != nullptr);
		const TCHAR* Required[] = { EpitopePackage, AdminPackage, NeuroPackage, CryoPackage, ComputePackage, ReactorPackage };
		TArray<TSharedPtr<FJsonValue>> Levels;
		for (const TCHAR* PackageName : Required)
		{
			TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
			Row->SetStringField(TEXT("package"), PackageName);
			Row->SetBoolField(TEXT("loaded"), World && FindLoadedLevelByPackage(World, PackageName) != nullptr);
			Levels.Add(MakeShared<FJsonValueObject>(Row));
		}
		Data->SetArrayField(TEXT("levels"), Levels);
		TArray<TSharedPtr<FJsonValue>> RecastJson;
		if (World)
		{
			for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
			{
				ARecastNavMesh* NavMesh = *It;
				if (!NavMesh)
				{
					continue;
				}
				TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("label"), NavMesh->GetActorLabel());
				Row->SetStringField(TEXT("package"), NormalizePackage(ActorOwningPackage(NavMesh)));
				Row->SetBoolField(TEXT("needs_rebuild"), NavMesh->NeedsRebuild());
				RecastJson.Add(MakeShared<FJsonValueObject>(Row));
			}
			if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
			{
				Data->SetBoolField(TEXT("navigation_dirty"), NavSys->IsNavigationDirty());
			}
		}
		Data->SetArrayField(TEXT("recast"), RecastJson);
		Data->SetNumberField(TEXT("recast_count"), RecastJson.Num());
		Data->SetArrayField(TEXT("dirty_packages"), DirtyPackageJsonArray(CollectDirtyPackageNamesSorted()));
		return Ok(Data);
	}
