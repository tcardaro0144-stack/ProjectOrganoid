// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class AProjectOrganoidCharacter;

/** Block 2 reception/security evidence + objective events after existing terminal Interact. */
namespace ProjectOrganoidAdminTerminalInvestigation
{
	void NotifyActivated(AActor* Terminal, AProjectOrganoidCharacter* Interactor);
}
