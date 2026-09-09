// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminTerminalInvestigation.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	FName ReadNameProperty(AActor* Actor, const TCHAR* PropertyName)
	{
		if (!Actor)
		{
			return NAME_None;
		}
		const FNameProperty* NameProp = FindFProperty<FNameProperty>(Actor->GetClass(), PropertyName);
		if (!NameProp)
		{
			return NAME_None;
		}
		return NameProp->GetPropertyValue_InContainer(Actor);
	}

	int32 ReadTerminalTypeIndex(AActor* Actor)
	{
		if (!Actor)
		{
			return INDEX_NONE;
		}
		if (const FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Actor->GetClass(), TEXT("TerminalType")))
		{
			if (const FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
			{
				return static_cast<int32>(Underlying->GetSignedIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Actor)));
			}
		}
		if (const FByteProperty* ByteProp = FindFProperty<FByteProperty>(Actor->GetClass(), TEXT("TerminalType")))
		{
			return static_cast<int32>(ByteProp->GetPropertyValue_InContainer(Actor));
		}
		return INDEX_NONE;
	}
}

void ProjectOrganoidAdminTerminalInvestigation::NotifyActivated(
	AActor* Terminal,
	AProjectOrganoidCharacter* Interactor)
{
	if (!Terminal || !Interactor)
	{
		return;
	}

	const FName TerminalId = ReadNameProperty(Terminal, TEXT("TerminalID"));
	const int32 TypeIndex = ReadTerminalTypeIndex(Terminal);
	const bool bReception = TypeIndex == 0 || TerminalId == TEXT("Terminal_AdminReception");
	const bool bSecurity = TypeIndex == 1 || TerminalId == TEXT("Terminal_AdminSecurity");
	if (!bReception && !bSecurity)
	{
		return;
	}

	if (UProjectOrganoidLogComponent* Log = Interactor->GetLogComponent())
	{
		FProjectOrganoidLogEntry Entry;
		if (bReception)
		{
			Entry.EntryId = TEXT("Log_ReceptionVisitorQueue");
			Entry.Title = FText::FromString(TEXT("VISITOR QUEUE — TODAY"));
			Entry.Body = FText::FromString(TEXT("09:40  GRANT, N. — expected\nStatus: not checked in\nDesk: unattended"));
			Entry.Author = FText::FromString(TEXT("Reception"));
			Entry.Category = TEXT("Reception");
		}
		else
		{
			Entry.EntryId = TEXT("Log_SecurityPublicAccessWatch");
			Entry.Title = FText::FromString(TEXT("PUBLIC ACCESS WATCH"));
			Entry.Body = FText::FromString(
				TEXT("Post assignment: Security Desk 01\nLast verified badge cycle: incomplete\nIntake procedure: interrupted"));
			Entry.Author = FText::FromString(TEXT("Security"));
			Entry.Category = TEXT("Security");
		}
		Entry.bIsRead = true;
		Log->CollectLogEntry(Entry);
		Log->MarkEntryRead(Entry.EntryId);
	}

	if (UGameInstance* GI = Interactor->GetGameInstance())
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(bReception
				? FName(TEXT("Event_ReceptionTerminalUsed"))
				: FName(TEXT("Event_SecurityTerminalUsed")));
		}
	}
}
