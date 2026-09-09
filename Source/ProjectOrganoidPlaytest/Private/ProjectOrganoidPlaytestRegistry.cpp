#include "ProjectOrganoidPlaytestRegistry.h"

namespace
{
	TArray<FOrganoidPlaytestCatalogEntry>& Entries()
	{
		static TArray<FOrganoidPlaytestCatalogEntry> Catalog;
		return Catalog;
	}
}

void FOrganoidPlaytestRegistry::Register(const FOrganoidPlaytestCatalogEntry& Entry)
{
	TArray<FOrganoidPlaytestCatalogEntry>& Catalog = Entries();
	for (FOrganoidPlaytestCatalogEntry& Existing : Catalog)
	{
		if (Existing.TestId.Equals(Entry.TestId, ESearchCase::IgnoreCase))
		{
			Existing = Entry;
			return;
		}
	}
	Catalog.Add(Entry);
}

const TArray<FOrganoidPlaytestCatalogEntry>& FOrganoidPlaytestRegistry::GetEntries()
{
	return Entries();
}

const FOrganoidPlaytestCatalogEntry* FOrganoidPlaytestRegistry::Find(const FString& TestId)
{
	for (const FOrganoidPlaytestCatalogEntry& Entry : Entries())
	{
		if (Entry.TestId.Equals(TestId, ESearchCase::IgnoreCase))
		{
			return &Entry;
		}
	}
	return nullptr;
}

TSharedPtr<IOrganoidPlaytestCase> FOrganoidPlaytestRegistry::Create(const FString& TestId)
{
	const FOrganoidPlaytestCatalogEntry* Entry = Find(TestId);
	if (!Entry || !Entry->Factory)
	{
		return nullptr;
	}
	return Entry->Factory();
}
