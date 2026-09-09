#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidPlaytestReport.h"

class IOrganoidPlaytestCase
{
public:
	virtual ~IOrganoidPlaytestCase() = default;
	virtual FString GetTestId() const = 0;
	virtual FString GetDisplayName() const = 0;
	virtual FString GetMapPackage() const = 0;
	virtual void Start(class UProjectOrganoidPlaytestEditorSubsystem& Owner) = 0;
	virtual void Tick(class UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) = 0;
	virtual void Abort(class UProjectOrganoidPlaytestEditorSubsystem& Owner) = 0;
};

struct FOrganoidPlaytestCatalogEntry
{
	FString TestId;
	FString DisplayName;
	FString MapPackage;
	TFunction<TSharedRef<IOrganoidPlaytestCase>()> Factory;
};

class FOrganoidPlaytestRegistry
{
public:
	static void Register(const FOrganoidPlaytestCatalogEntry& Entry);
	static const TArray<FOrganoidPlaytestCatalogEntry>& GetEntries();
	static const FOrganoidPlaytestCatalogEntry* Find(const FString& TestId);
	static TSharedPtr<IOrganoidPlaytestCase> Create(const FString& TestId);
};
