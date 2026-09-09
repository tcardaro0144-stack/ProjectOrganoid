#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Tickable.h"
#include "OrganoidAIBridgePlaytestHost.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.generated.h"

class IOrganoidPlaytestCase;
class FOrganoidPlaytestLogSink;

UCLASS()
class UProjectOrganoidPlaytestEditorSubsystem : public UEditorSubsystem, public FTickableGameObject, public IOrganoidPlaytestHost
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return true; }
	virtual bool IsTickableWhenPaused() const override { return true; }

	virtual TSharedRef<FJsonObject> ListPlaytests() override { return ListPlaytestsJson(); }
	virtual TSharedRef<FJsonObject> RunPlaytest(const FString& TestId) override { return RequestRun(TestId); }
	virtual TSharedRef<FJsonObject> GetStatus(const FString& RunId) override { return GetStatusJson(RunId); }
	virtual TSharedRef<FJsonObject> GetResult(const FString& RunId) override { return GetResultJson(RunId); }
	virtual TSharedRef<FJsonObject> Stop(const FString& RunId) override { return RequestStop(RunId); }

	TSharedRef<FJsonObject> ListPlaytestsJson() const;
	TSharedRef<FJsonObject> RequestRun(const FString& TestId);
	TSharedRef<FJsonObject> GetStatusJson(const FString& RunId) const;
	TSharedRef<FJsonObject> GetResultJson(const FString& RunId) const;
	TSharedRef<FJsonObject> RequestStop(const FString& RunId);

	FOrganoidPlaytestRecord* GetActiveRecord() { return ActiveRecord.Get(); }
	FOrganoidPlaytestLogSink* GetLogSink() const { return LogSink; }

	void SetStage(const FString& Stage);
	void CompleteActive(EOrganoidPlaytestState State, const FString& Reason);
	bool RequestStartPie(const FString& MapPackage);
	void RequestEndPieIfStarted();
	bool DidStartPie() const { return bStartedPieOurselves; }

private:
	TSharedPtr<FOrganoidPlaytestRecord> ActiveRecord;
	TArray<TSharedPtr<FOrganoidPlaytestRecord>> History;
	TSharedPtr<IOrganoidPlaytestCase> ActiveCase;
	FOrganoidPlaytestLogSink* LogSink = nullptr;
	bool bStartedPieOurselves = false;
	bool bStopRequested = false;

	const FOrganoidPlaytestRecord* FindRecord(const FString& RunId) const;
	void AttachLogSink();
	void DetachLogSink();
	void ArchiveActive();
};
