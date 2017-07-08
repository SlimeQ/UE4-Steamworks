// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "SteamWorkshopToolkitStyle.h"

class FSteamWorkshopToolkitCommands : public TCommands<FSteamWorkshopToolkitCommands>
{
public:

	FSteamWorkshopToolkitCommands()
		: TCommands<FSteamWorkshopToolkitCommands>(TEXT("SteamWorkshopToolkit"), NSLOCTEXT("Contexts", "SteamWorkshopToolkit", "SteamWorkshopToolkit Plugin"), NAME_None, FSteamWorkshopToolkitStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};