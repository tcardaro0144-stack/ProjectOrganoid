// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  Soft paths must match Content Browser package paths exactly.
 *  On disk: Content/UI/Menus/WBP_MainMenu.uasset
 *  Browser: /Game/UI/Menus/WBP_MainMenu
 */
namespace ProjectOrganoidUIAssetPaths
{
	/** Widget Blueprint package (no _C). */
	inline constexpr const TCHAR* MainMenuWidgetAsset = TEXT("/Game/UI/Menus/WBP_MainMenu.WBP_MainMenu");

	/** Generated Blueprint class used by CreateWidget / LoadClass. */
	inline constexpr const TCHAR* MainMenuWidgetClass = TEXT("/Game/UI/Menus/WBP_MainMenu.WBP_MainMenu_C");

	/** FClassFinder / ConstructorHelpers path (engine appends _C). */
	inline constexpr const TCHAR* MainMenuWidgetFinder = TEXT("/Game/UI/Menus/WBP_MainMenu");

	inline constexpr const TCHAR* PauseMenuWidgetAsset = TEXT("/Game/UI/Menus/WBP_PauseMenu.WBP_PauseMenu");
	inline constexpr const TCHAR* PauseMenuWidgetClass = TEXT("/Game/UI/Menus/WBP_PauseMenu.WBP_PauseMenu_C");
	inline constexpr const TCHAR* PauseMenuWidgetFinder = TEXT("/Game/UI/Menus/WBP_PauseMenu");
}
