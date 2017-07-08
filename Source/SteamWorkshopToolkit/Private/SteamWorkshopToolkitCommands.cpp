// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SteamWorkshopToolkitCommands.h"

#define LOCTEXT_NAMESPACE "FSteamWorkshopToolkitModule"

void FSteamWorkshopToolkitCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "SteamWorkshopToolkit", "Bring up SteamWorkshopToolkit window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
