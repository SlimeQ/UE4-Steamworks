// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

using System.IO;

public class SteamWorkshopToolkit : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

    public SteamWorkshopToolkit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
				"SteamWorkshopToolkit/Public",
                "OnlineSubsystemSteam/Public",
                "Steamworks/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"SteamWorkshopToolkit/Private",
                "OnlineSubsystemSteam/Private",
                "Steamworks/Private"
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "OnlineSubsystem",
                "OnlineSubsystemUtils",
                //"Steamworks",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "UATHelper",
                //"OnlineSubsystemSteam",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        LoadSteamworks(Target);
    }

    public bool LoadSteamworks(ReadOnlyTargetRules Target)
    {
        bool is64bit = (Target.Platform == UnrealTargetPlatform.Win64);
        bool isLibrarySupported = false;


        if ((Target.Platform == UnrealTargetPlatform.Linux))
        {
            isLibrarySupported = true;

            string PlatformString = "linux64";
            string SteamApiLib = Path.Combine(ThirdPartyPath, "sdk", "redistributable_bin", PlatformString, "libsteam_api.so");

            PublicAdditionalLibraries.Add(SteamApiLib);
            RuntimeDependencies.Add(new RuntimeDependency(SteamApiLib));
        }
        else
        if ((Target.Platform == UnrealTargetPlatform.Win64))
        {
            isLibrarySupported = true;


            string PlatformString = is64bit ? "win64" : "win32";
            string LibrariesPath = Path.Combine(ThirdPartyPath, "sdk", "public", "steam", "lib", PlatformString);

            string EncryptedAppTicketLib = is64bit ? "sdkencryptedappticket64.lib" : "sdkencryptedappticket.lib";

            //Console.WriteLine("Thirdparty path " + LibrariesPath);

            string SteamApiLib = Path.Combine(ThirdPartyPath, "sdk", "redistributable_bin");

            if (is64bit)
            {
                SteamApiLib = Path.Combine(SteamApiLib, "win64", "steam_api64.lib");
            }
            else
            {
                SteamApiLib = Path.Combine(SteamApiLib, "steam_api.lib");
            }

            PublicAdditionalLibraries.Add(Path.Combine(LibrariesPath, EncryptedAppTicketLib));
            PublicAdditionalLibraries.Add(SteamApiLib);
        }

        if (isLibrarySupported)
        {
            PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "sdk", "public"));
        }

        Definitions.Add(string.Format("WITH_CUSTOM_STEAMWORKS={0}", isLibrarySupported ? 1 : 0));

        return isLibrarySupported;
    }
}
