#include "ProjectOrganoidPlaytestRegistry.h"

namespace
{
	void SortCatalog(TArray<FOrganoidPlaytestCatalogEntry>& Catalog);

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
			SortCatalog(Catalog);
			return;
		}
	}
	Catalog.Add(Entry);
	SortCatalog(Catalog);
}

namespace
{
	void SortCatalog(TArray<FOrganoidPlaytestCatalogEntry>& Catalog)
	{
		static const TCHAR* Order[] = {
			TEXT("AdminToNeuroTraversal_Functional"),
			TEXT("AmbienceLayerPlayback_Functional"),
			TEXT("AmmoReload_Functional"),
			TEXT("BeepClickInjection_Functional"),
			TEXT("BeepMixCapture_Functional"),
			TEXT("BeepMixCapture_CombatMute_Functional"),
			TEXT("BeepSourceIsolation_Functional"),
			TEXT("BiologicalAdaptation_Functional"),
			TEXT("CheckpointHealth_Functional"),
			TEXT("CryoAccess_Functional"),
			TEXT("DeconAudioIsolation_Functional"),
			TEXT("DoorLockPowerInteractable_Functional"),
			TEXT("HostCombatLoop_Functional"),
			TEXT("NeuroAccess_Functional"),
			TEXT("NeuroAdaptationConnection_Functional"),
			TEXT("NeuroExamineNeuralChangeEvidence_Functional"),
			TEXT("NeuroFollowNeuralSignature_Functional"),
			TEXT("NeuroMappingSignalTrace_Functional"),
			TEXT("NeuroNeuralSlowUse_Functional"),
			TEXT("NeuroPowerFailureDiagnosis_Functional"),
			TEXT("NeuroPowerFailureDiscovery_Functional"),
			TEXT("NeuroResearchFloorArray_Functional"),
			TEXT("NeuroResearchLoadCutoff_Functional"),
			TEXT("NeuroResearchStationIntro_Functional"),
			TEXT("NeuroResearchStationPlacement_Functional"),
			TEXT("NeuroRestoreLabPower_Functional"),
			TEXT("NeuroRevelation_Functional"),
			TEXT("NeuroTargetingWhy_Functional"),
			TEXT("OpeningBlock4_Functional"),
			TEXT("OpeningFoundation_Functional"),
			TEXT("OpeningInvestigation_Functional"),
			TEXT("OpeningResources_Functional"),
			TEXT("PETactical_Functional"),
			TEXT("ResearchStation_Functional"),
			TEXT("RoomEntryBeep_Functional"),
			TEXT("S17_AccessDoor_Functional"),
			TEXT("S18_ExecutiveTerminal_Functional"),
			TEXT("S18_OperationsTerminal_Functional"),
			TEXT("S18_ReceptionTerminal_Functional"),
			TEXT("S18_RecordsTerminal_Functional"),
			TEXT("S18_SecurityTerminal_Functional"),
			TEXT("S18_TransitTerminal_Functional"),
			TEXT("S19_FacilityHologram_Functional"),
			TEXT("S20_AdminLighting_Functional"),
			TEXT("S21_AccessDoorListener_Functional"),
			TEXT("S21_AdminFacilityState_Functional"),
			TEXT("S21_AdminLightingStateListener_Functional"),
			TEXT("S22_AdminAudioZones_Functional"),
			TEXT("CryoEntry_Functional"),
			TEXT("CryoEvidence_Functional"),
			TEXT("ComputeEntry_Functional"),
			TEXT("ComputeHandover_Functional"),
			TEXT("TheConclusion_Functional")
		};
		Catalog.StableSort([](const FOrganoidPlaytestCatalogEntry& A, const FOrganoidPlaytestCatalogEntry& B)
		{
			auto IndexOf = [](const FString& Id) -> int32
			{
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(Order); ++Index)
				{
					if (Id.Equals(Order[Index], ESearchCase::CaseSensitive))
					{
						return Index;
					}
				}
				return 1000;
			};
			return IndexOf(A.TestId) < IndexOf(B.TestId);
		});
	}
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
