// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "ProjectOrganoidDiagnosticsUploader.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidDiagnosticsArchiveReady, const FString&, ArchivePath, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidDiagnosticsUploadFinished, bool, bSuccess, int32, HttpStatus, const FString&, ResponseBody);

/**
 *  Crash / telemetry archive packer + remote diagnostics uploader.
 *  Bundles crash dumps and Saved/ProjectOrganoid/Telemetry logs for support upload.
 */
UCLASS(Config = Game)
class UProjectOrganoidDiagnosticsUploader : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Diagnostics")
	bool bAutoUploadOnCrash = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Diagnostics")
	bool bIncludeEngineLogs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Diagnostics")
	FString UploadEndpointUrl = TEXT("https://diagnostics.local/projectorganoid/upload");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Diagnostics", meta = (ClampMin = "1024"))
	int64 MaxArchiveBytes = 50 * 1024 * 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Diagnostics")
	bool bConsentToUpload = false;

	UPROPERTY(BlueprintAssignable, Category = "Diagnostics")
	FOnProjectOrganoidDiagnosticsArchiveReady OnDiagnosticsArchiveReady;

	UPROPERTY(BlueprintAssignable, Category = "Diagnostics")
	FOnProjectOrganoidDiagnosticsUploadFinished OnDiagnosticsUploadFinished;

	UFUNCTION(BlueprintCallable, Category = "Diagnostics")
	FString PackageDiagnosticsArchive(const FString& Reason = TEXT("Manual"));

	UFUNCTION(BlueprintCallable, Category = "Diagnostics")
	bool UploadDiagnosticsArchive(const FString& ArchiveDirectoryOrZip);

	UFUNCTION(BlueprintCallable, Category = "Diagnostics")
	bool PackageAndUpload(const FString& Reason = TEXT("Crash"));

	UFUNCTION(BlueprintPure, Category = "Diagnostics")
	FString GetDiagnosticsRoot() const;

protected:

	FDelegateHandle CrashReportHandle;

	UFUNCTION()
	void HandleCrashReportWritten(const FString& FilePath);

	void WriteManifest(const FString& PackageDir, const FString& Reason) const;
	bool CopyTelemetryFiles(const FString& PackageDir) const;
	bool CopyEngineLogs(const FString& PackageDir) const;
	bool TryCompressDirectory(const FString& PackageDir, FString& OutZipPath) const;
	void OnHttpRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
};
