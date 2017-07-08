// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SteamWorkshopToolkit.h"
#include "SteamWorkshopToolkitStyle.h"
#include "SteamWorkshopToolkitCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "OnlineSessionClient.h"
#include "Interfaces/IPluginManager.h"
#include "FileManager.h"
#include "FileHelper.h"
#include "SteamWorkshopMap.h"
#include "AssetRegistryModule.h"
//#include "UATHelperModule.h"
//#include "Editor/UATHelper/UATHelperModule.h"
//#include "Editor/UATHelper/Public/IUATHelperModule.h"
//#include "Misc/MonitoredProcess.h"
//#include "Framework/Notifications/NotificationManager.h"
//#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/Texture2D.h"
#include "Runtime/Engine/Classes/EditorFramework/AssetImportData.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"


static const FName SteamWorkshopToolkitTabName("SteamWorkshopToolkit");

#define LOCTEXT_NAMESPACE "FSteamWorkshopToolkitModule"

extern "C" void __cdecl SteamAPIDebugTextHook(int nSeverity, const char *pchDebugText)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *pchDebugText);

}

void FSteamWorkshopToolkitModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	AppId = 618500;

	SaveStringTextToFile(FString("."), FString("steam_appid.txt"), FString::FromInt(AppId), true);


	DelegateHandle_SteamCallbacks = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FSteamWorkshopToolkitModule::RunSteamCallbacks), 1.0f);
	//GEditor->GetTimerManager().Get().SetTimer(TimerHandle_SteamCallbacks*, &FSteamWorkshopToolkitModule::RunSteamCallbacks, 1.0f, true);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(FName("Steam"));
	if (OnlineSub) {
		OnlineSub->Init();
		UE_LOG(LogTemp, Log, TEXT("Steam inititialized"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Steam is invalid!"));
	}

	if (SteamAPI_RestartAppIfNecessary(AppId)) {
		UE_LOG(LogTemp, Log, TEXT("SteamAPI_RestartAppIfNecessary() -> true"));
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("SteamAPI_RestartAppIfNecessary() -> false"));
	}

	if (SteamUtils())
		SteamUtils()->SetWarningMessageHook(&SteamAPIDebugTextHook);
	else
		UE_LOG(LogTemp, Warning, TEXT("SteamUtils() is null!"));

	//if (SteamAPI_Init())
	//{
	//	//do my steam stuff
	//	const char* SteamName = SteamFriends()->GetPersonaName();
	//	UE_LOG(LogTemp, Log, TEXT("SteamName=%s"), SteamName);
	//}
	//else
	//{
	//	//push an error or something
	//	UE_LOG(LogTemp, Warning, TEXT("SteamAPI_Init() failed!"));
	//}

	//FString SteamworksPath = IPluginManager::Get().FindPlugin("OnlineSubsystemSteam")->GetBaseDir();
	//FPlatformProcess::GetDllHandle(*SteamworksPath);

	FSteamWorkshopToolkitStyle::Initialize();
	FSteamWorkshopToolkitStyle::ReloadTextures();

	FSteamWorkshopToolkitCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FSteamWorkshopToolkitCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FSteamWorkshopToolkitModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FSteamWorkshopToolkitModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FSteamWorkshopToolkitModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SteamWorkshopToolkitTabName, FOnSpawnTab::CreateRaw(this, &FSteamWorkshopToolkitModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FSteamWorkshopToolkitTabTitle", "SteamWorkshopToolkit"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FSteamWorkshopToolkitModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FSteamWorkshopToolkitStyle::Shutdown();

	FSteamWorkshopToolkitCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SteamWorkshopToolkitTabName);

	FTicker::GetCoreTicker().RemoveTicker(DelegateHandle_SteamCallbacks);
}

TSharedRef<SDockTab> FSteamWorkshopToolkitModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	//GetWorld()-> GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AProxyWarWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);

	if (USteamWorkshopMap* Map = GetMap()) {
		FileId = Map->UID;
	}

	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FSteamWorkshopToolkitModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("SteamWorkshopToolkit.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &FSteamWorkshopToolkitModule::OnCreateMap) //&FSteamWorkshopToolkitModule::CreateMapButtonClicked)
						.Text(FText::FromString("1) Create Map"))
						.HAlign(EHorizontalAlignment::HAlign_Left)
					]
				+ SVerticalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &FSteamWorkshopToolkitModule::OnCook)
						.Text(FText::FromString("2) Cook"))
						.HAlign(EHorizontalAlignment::HAlign_Left)
					]
				+ SVerticalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &FSteamWorkshopToolkitModule::OnPackage)
						.Text(FText::FromString("3) Package"))
						.HAlign(EHorizontalAlignment::HAlign_Left)
					]
				+ SVerticalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &FSteamWorkshopToolkitModule::OnUpload)
						.Text(FText::FromString("4) Upload"))
						.HAlign(EHorizontalAlignment::HAlign_Left)
					]
				+ SVerticalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &FSteamWorkshopToolkitModule::OnOpenWorkshopPage)
						.Text(FText::FromString("5) Open Workshop Page"))
						.HAlign(EHorizontalAlignment::HAlign_Left)
					]
			]
		];
}

bool FSteamWorkshopToolkitModule::RunSteamCallbacks(float DeltaTime) {
	SteamAPI_RunCallbacks();
	return true;
}

void FSteamWorkshopToolkitModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(SteamWorkshopToolkitTabName);
}

void FSteamWorkshopToolkitModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FSteamWorkshopToolkitCommands::Get().OpenPluginWindow);
}

void FSteamWorkshopToolkitModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FSteamWorkshopToolkitCommands::Get().OpenPluginWindow);
}

FReply FSteamWorkshopToolkitModule::OnCreateMap() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::CreateMapButtonClicked()"));

	if (GetMap()) {
		UE_LOG(LogTemp, Warning, TEXT("definition already exists"));
		return FReply::Handled();
	}

	if (SteamAPI_Init())
	{
		//do my steam stuff
		const char* SteamName = SteamFriends()->GetPersonaName();
		UE_LOG(LogTemp, Log, TEXT("SteamName=%s"), *FString(UTF8_TO_TCHAR(SteamName)));

		if (SteamUGC()) {
			SteamAPICall_t CreatedItem = SteamUGC()->CreateItem(AppId, EWorkshopFileType::k_EWorkshopFileTypeCommunity);
			//SteamCallItemCreated CreateItemResult_t
			m_callResultCreateItem.Set(CreatedItem, this, &FSteamWorkshopToolkitModule::OnMapCreated);


			//UE_LOG(LogTemp, Log, TEXT("CreatedItem=%d"), CreatedItem);

			//FString UID = TEXT("UGC") + FString::FromInt(CreatedItem);

			//FString PackageName = TEXT("/Game/CustomMaps/") + UID;
			//UPackage* Package = CreatePackage(NULL, *PackageName);

			//USteamWorkshopMap* NewAsset = new(Package, FName("definition"), RF_Public) USteamWorkshopMap(FObjectInitializer());

			//if (NewAsset != NULL)
			//{
			//	NewAsset->UID = UID;
			//	NewAsset->Name = FString("Untitled");
			//	NewAsset->Description = FString("Enter a map description here.");
			//	// Fill in the assets data here
			//}

			//FAssetRegistryModule::AssetCreated(NewAsset);
			//NewAsset->MarkPackageDirty();

		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("SteamUGC() is null!"));
		}
	}
	else
	{
		//push an error or something
		UE_LOG(LogTemp, Warning, TEXT("SteamAPI_Init() failed!"));
	}

	return FReply::Handled();
}

void FSteamWorkshopToolkitModule::OnMapCreated(CreateItemResult_t *pCallback, bool bIOFailure) {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnMapCreated"));
	if (bIOFailure) {
		UE_LOG(LogTemp, Warning, TEXT("IOFailure in FSteamWorkshopToolkitModule::OnMapCreated"));
		return;
	}

	switch (pCallback->m_eResult) {
	case EResult::k_EResultOK:
		UE_LOG(LogTemp, Log, TEXT("Result: OK!"));
		break;
	case EResult::k_EResultInsufficientPrivilege:
		UE_LOG(LogTemp, Warning, TEXT("Result: Insufficient Privilege!"));
		break;
	case EResult::k_EResultTimeout:
		UE_LOG(LogTemp, Warning, TEXT("Result: Timeout!"));
		break;
	case EResult::k_EResultNotLoggedOn:
		UE_LOG(LogTemp, Warning, TEXT("Result: Not Logged On!"));
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Result: Unknown!"));
		break;
	}

	FileId = pCallback->m_nPublishedFileId;

	if (pCallback->m_bUserNeedsToAcceptWorkshopLegalAgreement) {
		UE_LOG(LogTemp, Warning, TEXT("User needs to accept workshop legal agreement!"));
		SteamFriends()->ActivateGameOverlayToWebPage(TCHAR_TO_ANSI(*(TEXT("steam://url/CommunityFilePage/") + FString::FromInt(pCallback->m_nPublishedFileId))));
	}

	UE_LOG(LogTemp, Log, TEXT("CreatedItem=%d"), pCallback->m_nPublishedFileId);

	FString UID = TEXT("UGC") + FString::FromInt(pCallback->m_nPublishedFileId);

	FString PackageName = FString("/Game/CustomMaps") / UID / FString("definition");
	UPackage* Package = CreatePackage(NULL, *PackageName);

	USteamWorkshopMap* NewAsset = new(Package, FName("definition"), RF_Public | RF_Standalone) USteamWorkshopMap(FObjectInitializer());

	if (NewAsset != NULL)
	{
		NewAsset->UID = pCallback->m_nPublishedFileId;
		NewAsset->Name = FString("Untitled");
		NewAsset->Description = FString("Enter a map description here.");
		// Fill in the assets data here
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	NewAsset->MarkPackageDirty();
}

void FSteamWorkshopToolkitModule::OnMapUploaded(SubmitItemUpdateResult_t *pCallback, bool bIOFailure) {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnMapUploaded"));
	if (bIOFailure) {
		UE_LOG(LogTemp, Warning, TEXT("IOFailure in FSteamWorkshopToolkitModule::OnMapUploaded"));
		return;
	}

	switch (pCallback->m_eResult) {
	case EResult::k_EResultOK:
		UE_LOG(LogTemp, Log, TEXT("Result: OK!"));
		break;
	case EResult::k_EResultInsufficientPrivilege:
		UE_LOG(LogTemp, Warning, TEXT("Result: Insufficient Privilege!"));
		break;
	case EResult::k_EResultTimeout:
		UE_LOG(LogTemp, Warning, TEXT("Result: Timeout!"));
		break;
	case EResult::k_EResultNotLoggedOn:
		UE_LOG(LogTemp, Warning, TEXT("Result: Not Logged On!"));
		break;
	case EResult::k_EResultFail:
		UE_LOG(LogTemp, Warning, TEXT("Result: Fail!"));
		break;
	case EResult::k_EResultInvalidParam:
		UE_LOG(LogTemp, Warning, TEXT("Result: Invalid Param!"));
		break;
	case EResult::k_EResultAccessDenied:
		UE_LOG(LogTemp, Warning, TEXT("Result: Access Denied!"));
		break;
	case EResult::k_EResultFileNotFound:
		UE_LOG(LogTemp, Warning, TEXT("Result: File Not Found!"));
		break;
	case EResult::k_EResultLockingFailed:
		UE_LOG(LogTemp, Warning, TEXT("Result: Locking Failed!"));
		break;
	case EResult::k_EResultLimitExceeded:
		UE_LOG(LogTemp, Warning, TEXT("Result: Limit Exceeded!"));
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("Result: Unknown!"));
		break;
	}

	if (pCallback->m_bUserNeedsToAcceptWorkshopLegalAgreement) {
		UE_LOG(LogTemp, Warning, TEXT("User needs to accept workshop legal agreement!"));
		SteamFriends()->ActivateGameOverlayToWebPage(TCHAR_TO_ANSI(*(TEXT("steam://url/CommunityFilePage/") + FString::FromInt(FileId))));
	}
}

FReply FSteamWorkshopToolkitModule::OnCook() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnCook()"));

	USteamWorkshopMap* Map = GetMap();
	if (!Map) {
		UE_LOG(LogTemp, Warning, TEXT("Map definition not found!"));
		return FReply::Handled();
	}
	CookMap(Map);
	return FReply::Handled();
}

FReply FSteamWorkshopToolkitModule::OnPackage() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnPackage()"));

	USteamWorkshopMap* Map = GetMap();
	if (!Map) {
		UE_LOG(LogTemp, Warning, TEXT("Map definition not found!"));
		return FReply::Handled();
	}
	PackageMap(Map);
	return FReply::Handled();
}

USteamWorkshopMap* FSteamWorkshopToolkitModule::GetMap() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::GetMap()"));
	FString DefPath = FString("/Game") / GetUGCPath() / FString("definition");
	UE_LOG(LogTemp, Log, TEXT("MapDefinitionPath=%s"), *DefPath);
	return Cast<USteamWorkshopMap>(StaticLoadObject(USteamWorkshopMap::StaticClass(), NULL, *DefPath));
}

FString FSteamWorkshopToolkitModule::GetUGCPath() {
	FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir());
	UE_LOG(LogTemp, Log, TEXT("GetUGCPath() -> ContentDir=%s"), *ContentDir);
	TArray<FString> UGCDirectories;
	IFileManager::Get().FindFilesRecursive(UGCDirectories, *ContentDir, *FString("UGC*"), true, true);
	//IFileManager::Get().FindFiles(UGCDirectories, *FPaths::ConvertRelativePathToFull(FPaths::GameContentDir()), false, true);
	if (UGCDirectories.Num() == 0) {
		UE_LOG(LogTemp, Warning, TEXT("No UGC folders found!"));
		return FString("/");
	}

	if (UGCDirectories.Num() > 1) {
		UE_LOG(LogTemp, Warning, TEXT("Multiple UGC folders found!"));
	}

	FString FullUGCPath = UGCDirectories[0];
	FString UGCPath = FullUGCPath.Replace(*ContentDir, *FString(""), ESearchCase::IgnoreCase);

	return UGCPath;
}

FReply FSteamWorkshopToolkitModule::OnUpload() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnUpload()"));
	if (USteamWorkshopMap* Map = GetMap()) {
		if (SteamAPI_Init()) {
			FString CookedContentPath = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir()) / FString("Workshop");

			UE_LOG(LogTemp, Log, TEXT("ContentDir=%s"), *CookedContentPath);

			UGCUpdateHandle_t UGCUpdate = SteamUGC()->StartItemUpdate(AppId, FileId);
			SteamUGC()->SetItemTitle(UGCUpdate, TCHAR_TO_ANSI(*Map->Name));
			SteamUGC()->SetItemDescription(UGCUpdate, TCHAR_TO_ANSI(*Map->Description));
			SteamUGC()->SetItemContent(UGCUpdate, TCHAR_TO_ANSI(*CookedContentPath));

			FString ThumbnailPath;
			if (Map->Thumbnail && Map->Thumbnail->AssetImportData) {
				ThumbnailPath = Map->Thumbnail->AssetImportData->GetFirstFilename();
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("!Map->Thumbnail"));
			}
			UE_LOG(LogTemp, Log, TEXT("ThumbnailPath=%s"), *ThumbnailPath);
			SteamUGC()->SetItemPreview(UGCUpdate, TCHAR_TO_ANSI(*ThumbnailPath));

			SteamAPICall_t ItemUpdateCall = SteamUGC()->SubmitItemUpdate(UGCUpdate, TCHAR_TO_ANSI(*FString("")));
			m_callResultUpdateItem.Set(ItemUpdateCall, this, &FSteamWorkshopToolkitModule::OnMapUploaded);
			
		}
	}

	return FReply::Handled();
}

void FSteamWorkshopToolkitModule::CookMap(USteamWorkshopMap* Map)
{
	// RunUAT.bat -verbose BuildCookRun -project="C:\Users\quinc\Documents\Unreal Projects\ProxyWarToolkit\ProxyWarToolkit.uproject" -cookdir="C:\Users\quinc\Documents\Unreal Projects\ProxyWarToolkit\Content\CustomMaps\UGC965221773" -pak -stage -nocompile -cook
	// BuildCookRun -project="C:/Users/quinc/Documents/Unreal Projects/ProxyWarToolkit/ProxyWarToolkit.uproject" -noP4 -clientconfig=Development -serverconfig=Development -nocompile -nocompileeditor -installed -ue4exe=UE4Editor-Cmd.exe -utf8output -platform=Win64 -targetplatform=Win64 -cook -map=BlankStare2+BlankStare -unversionedcookedcontent -pak -SkipCookingEditorContent -compressed -stage -package
#if PLATFORM_WINDOWS
	FText PlatformName = LOCTEXT("PlatformName_Windows", "Windows");
#elif PLATFORM_MAC
	FText PlatformName = LOCTEXT("PlatformName_Mac", "Mac");
#elif PLATFORM_LINUX
	FText PlatformName = LOCTEXT("PlatformName_Linux", "Linux");
#else
	FText PlatformName = LOCTEXT("PlatformName_Desktop", "Desktop");
#endif

	FString MapPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir()) / GetUGCPath();
	UE_LOG(LogTemp, Log, TEXT("Cooking map at %s"), *MapPath);

	FString ReleaseVersion = TEXT("ProxyWarVersion0");	//@todo: need to get this out of an ini file

	FString CommandLine = FString::Printf(TEXT("BuildCookRun -project=\"%s\" -cook -nocompile"),
		*FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath())
	);

	FText CookingText = FText::Format(LOCTEXT("SteamWorkshopToolkit_PackagePluginTaskName", "Cooking {0}"), FText::FromString(Map->Name));

	FString FriendlyName = Map->Name; // Plugin->GetDescriptor().FriendlyName;
	IUATHelperModule::Get().CreateUatTask(CommandLine, PlatformName, CookingText,
		CookingText, nullptr, /*FOdinEditorStyle::Get().GetBrush(TEXT("OdinEditor.PackageGameModAction")),*/
		[ReleaseVersion, PlatformName, FriendlyName, this](FString TaskResult, double TimeSec)
	{
		UE_LOG(LogTemp, Log, TEXT("CookResult(%f): %s"), (float)TimeSec, *TaskResult);
	});
}


void FSteamWorkshopToolkitModule::PackageMap(USteamWorkshopMap* Map) {
#if PLATFORM_WINDOWS
	FText PlatformName = LOCTEXT("PlatformName_Windows", "Windows");
#elif PLATFORM_MAC
	FText PlatformName = LOCTEXT("PlatformName_Mac", "Mac");
#elif PLATFORM_LINUX
	FText PlatformName = LOCTEXT("PlatformName_Linux", "Linux");
#else
	FText PlatformName = LOCTEXT("PlatformName_Desktop", "Desktop");
#endif

	// copy map definition to cooked path
	FString DefinitionPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir()) / GetUGCPath() / FString("definition.uasset");
	FString CookedPath = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir()) + FString("Cooked/WindowsNoEditor") / FApp::GetGameName() / FString("Content") / GetUGCPath();
	FString CookedDefPath = CookedPath + FString("/definition.uasset");


	UE_LOG(LogTemp, Log, TEXT("Copying %s to %s"), *DefinitionPath, *CookedDefPath);
	IFileManager::Get().Copy(*CookedDefPath, *DefinitionPath);

	// create paklist
	FString PakListString;
	TArray<FString> PakList;
	IFileManager::Get().FindFilesRecursive(PakList, *CookedPath, *FString("*"), true, false);

	for (int i = 0; i < PakList.Num(); i++) {
		PakListString += FString("\"") + PakList[i] + FString("\"\r\n");
	}

	SaveStringTextToFile(FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir()), FString("paklist.txt"), PakListString, true);


	// delete old paks
	FString WorkshopDirectory = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir()) / FString("Workshop");
	IFileManager::Get().DeleteDirectory(*WorkshopDirectory);

	// build pak using UnrealPak.exe
	FString PakPath = WorkshopDirectory / Map->Name + ".pak";
	UE_LOG(LogTemp, Log, TEXT("PakPath=%s"), *PakPath);
	FString PakListPath = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir()) / FString("paklist.txt");
	UE_LOG(LogTemp, Log, TEXT("PakListPath=%s"), *PakListPath);

	FString PakCommandLine = FString::Printf(TEXT("\"%s\" -create=\"%s\" -platform=Windows -UTF8Output -multiprocess -patchpaddingalign=2048"),
		*PakPath,
		*PakListPath
	);
	FText PackagingText = FText::Format(LOCTEXT("SteamWorkshopToolkit_PackagePluginTaskName", "Packaging {0}"), FText::FromString(Map->Name));
	CreateUnrealPakTask(PakCommandLine, PlatformName, PackagingText,
		PackagingText, nullptr,
		[PlatformName, Map, this](FString TaskResult, double TimeSec)
	{
		UE_LOG(LogTemp, Log, TEXT("PackagingResult(%f): %s"), (float)TimeSec, *TaskResult);
	});
}

void FSteamWorkshopToolkitModule::CreateUnrealPakTask(const FString& CommandLine, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon, UnrealPakTaskResultCallack ResultCallback)
{
	// make sure that the UAT batch file is in place
#if PLATFORM_WINDOWS
	FString UnrealPakName = TEXT("UnrealPak.exe");
	FString CmdExe = TEXT("cmd.exe");
#else
	UE_LOG(LogTemp, Warning, TEXT("Platform is not supported!!!"));
	return;
#endif

	FString UnrealPakPath = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Binaries/Win64") / UnrealPakName);
	// FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
	bool bHasCode = false; // GameProjectModule.Get().ProjectHasCodeFiles();

	if (!FPaths::FileExists(UnrealPakPath))
	{
	/*	FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("File"), FText::FromString(UnrealPakPath));
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("RequiredFileNotFoundMessage", "A required file could not be found:\n{File}"), Arguments));

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		FString EventName = (CommandLine.Contains(TEXT("-package")) ? TEXT("Editor.Package") : TEXT("Editor.Cook"));
		FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), bHasCode, EAnalyticsErrorCodes::UATNotFound, ParamArray);*/

		UE_LOG(LogTemp, Warning, TEXT("UnrealPak.exe not found!!!"));

		return;
	}

#if PLATFORM_WINDOWS
	FString FullCommandLine = FString::Printf(TEXT("/c \"\"%s\" %s\""), *UnrealPakPath, *CommandLine);
#else
	FString FullCommandLine = FString::Printf(TEXT("\"%s\" %s"), *UnrealPakPath, *CommandLine);
#endif

	TSharedPtr<FMonitoredProcess> UnrealPakProcess = MakeShareable(new FMonitoredProcess(CmdExe, FullCommandLine, true));

	// create notification item
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	FNotificationInfo Info(FText::Format(LOCTEXT("UnrealPakTaskInProgressNotification", "{TaskName} for {Platform}..."), Arguments));

	Info.Image = TaskIcon;
	Info.bFireAndForget = false;
	Info.ExpireDuration = 3.0f;
	Info.Hyperlink = FSimpleDelegate::CreateStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakHyperlinkNavigate);
	Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
	Info.ButtonDetails.Add(
		FNotificationButtonInfo(
			LOCTEXT("UnrealPakTaskCancel", "Cancel"),
			LOCTEXT("UnrealPakTaskCancelToolTip", "Cancels execution of this task."),
			FSimpleDelegate::CreateStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakCancelButtonClicked, UnrealPakProcess)
		)
	);

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

	if (!NotificationItem.IsValid())
	{
		return;
	}

	FString EventName = (CommandLine.Contains(TEXT("-package")) ? TEXT("Editor.Package") : TEXT("Editor.Cook"));
	//FEditorAnalytics::ReportEvent(EventName + TEXT(".Start"), PlatformDisplayName.ToString(), bHasCode);

	NotificationItem->SetCompletionState(SNotificationItem::CS_Pending);

	// launch the packager
	TWeakPtr<SNotificationItem> NotificationItemPtr(NotificationItem);

	EventData Data;
	Data.StartTime = FPlatformTime::Seconds();
	Data.EventName = EventName;
	Data.bProjectHasCode = bHasCode;
	Data.ResultCallback = ResultCallback;
	UnrealPakProcess->OnCanceled().BindStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakProcessCanceled, NotificationItemPtr, PlatformDisplayName, TaskShortName, Data);
	UnrealPakProcess->OnCompleted().BindStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakProcessCompleted, NotificationItemPtr, PlatformDisplayName, TaskShortName, Data);
	UnrealPakProcess->OnOutput().BindStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakProcessOutput, NotificationItemPtr, PlatformDisplayName, TaskShortName);

	TWeakPtr<FMonitoredProcess> UnrealPakProcessPtr(UnrealPakProcess);
	FEditorDelegates::OnShutdownPostPackagesSaved.Add(FSimpleDelegate::CreateStatic(&FSteamWorkshopToolkitModule::HandleUnrealPakCancelButtonClicked, UnrealPakProcessPtr));

	if (UnrealPakProcess->Launch())
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileStart_Cue.CompileStart_Cue"));
	}
	else
	{
		GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));

		NotificationItem->SetText(LOCTEXT("UnrealPakLaunchFailedNotification", "Failed to launch UnrealPak.exe!"));
		NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
		NotificationItem->ExpireAndFadeout();

		//TArray<FAnalyticsEventAttribute> ParamArray;
		//ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
		//FEditorAnalytics::ReportEvent(EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), bHasCode, EAnalyticsErrorCodes::UATLaunchFailure, ParamArray);
		if (ResultCallback)
		{
			ResultCallback(TEXT("FailedToStart"), 0.0f);
		}
	}
}


void FSteamWorkshopToolkitModule::HandleUnrealPakHyperlinkNavigate()
{
	FGlobalTabmanager::Get()->InvokeTab(FName("OutputLog"));
}


void FSteamWorkshopToolkitModule::HandleUnrealPakCancelButtonClicked(TSharedPtr<FMonitoredProcess> PackagerProcess)
{
	if (PackagerProcess.IsValid())
	{
		PackagerProcess->Cancel(true);
	}
}

void FSteamWorkshopToolkitModule::HandleUnrealPakCancelButtonClicked(TWeakPtr<FMonitoredProcess> PackagerProcessPtr)
{
	TSharedPtr<FMonitoredProcess> PackagerProcess = PackagerProcessPtr.Pin();
	if (PackagerProcess.IsValid())
	{
		PackagerProcess->Cancel(true);
	}
}

void FSteamWorkshopToolkitModule::HandleUnrealPakProcessCanceled(TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);

	TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
		NotificationItemPtr,
		SNotificationItem::CS_Fail,
		FText::Format(LOCTEXT("UatProcessFailedNotification", "{TaskName} canceled!"), Arguments)
	);

	//TArray<FAnalyticsEventAttribute> ParamArray;
	const double TimeSec = FPlatformTime::Seconds() - Event.StartTime;
	//ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), TimeSec));
	//FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Canceled"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ParamArray);
	if (Event.ResultCallback)
	{
		Event.ResultCallback(TEXT("Canceled"), TimeSec);
	}
	//	FMessageLog("PackagingResults").Warning(FText::Format(LOCTEXT("UatProcessCanceledMessageLog", "{TaskName} for {Platform} canceled by user"), Arguments));
}

void FSteamWorkshopToolkitModule::HandleUnrealPakProcessCompleted(int32 ReturnCode, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Platform"), PlatformDisplayName);
	Arguments.Add(TEXT("TaskName"), TaskName);
	const double TimeSec = FPlatformTime::Seconds() - Event.StartTime;

	if (ReturnCode == 0)
	{
		TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Success,
			FText::Format(LOCTEXT("UnrealPakProcessSucceededNotification", "{TaskName} complete!"), Arguments)
		);

		//TArray<FAnalyticsEventAttribute> ParamArray;
		//ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), TimeSec));
		//FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Completed"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ParamArray);
		if (Event.ResultCallback)
		{
			Event.ResultCallback(TEXT("Completed"), TimeSec);
		}

		//		FMessageLog("PackagingResults").Info(FText::Format(LOCTEXT("UatProcessSuccessMessageLog", "{TaskName} for {Platform} completed successfully"), Arguments));
	}
	else
	{
		TGraphTask<FMainFrameActionsNotificationTask>::CreateTask().ConstructAndDispatchWhenReady(
			NotificationItemPtr,
			SNotificationItem::CS_Fail,
			FText::Format(LOCTEXT("PackagerFailedNotification", "{TaskName} failed!"), Arguments)
		);

		//TArray<FAnalyticsEventAttribute> ParamArray;
		//ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), TimeSec));
		//FEditorAnalytics::ReportEvent(Event.EventName + TEXT(".Failed"), PlatformDisplayName.ToString(), Event.bProjectHasCode, ReturnCode, ParamArray);
		if (Event.ResultCallback)
		{
			Event.ResultCallback(TEXT("Failed"), TimeSec);
		}
	}
}

void FSteamWorkshopToolkitModule::HandleUnrealPakProcessOutput(FString Output, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName)
{
	if (!Output.IsEmpty() && !Output.Equals("\r"))
	{
		UE_LOG(LogTemp, Log, TEXT("%s (%s): %s"), *TaskName.ToString(), *PlatformDisplayName.ToString(), *Output);

	}
}

FReply FSteamWorkshopToolkitModule::OnOpenWorkshopPage() {
	UE_LOG(LogTemp, Log, TEXT("FSteamWorkshopToolkitModule::OnOpenWorkshopPage()"));
	//UEditorEngine:: StartCookByTheBookInEditor()

	FPlatformProcess::LaunchURL(*(TEXT("https://steamcommunity.com/sharedfiles/filedetails/?id=") + FString::FromInt(FileId)), NULL, NULL);
	return FReply::Handled();
}

bool FSteamWorkshopToolkitModule::SaveStringTextToFile(
	FString SaveDirectory,
	FString FileName,
	FString SaveText,
	bool AllowOverWriting
) {

	//Dir Exists?
	if (!IFileManager::Get().DirectoryExists(*SaveDirectory))
	{
		//create directory if it not exist
		IFileManager::Get().MakeDirectory(*SaveDirectory);

		//still could not make directory?
		if (!IFileManager::Get().DirectoryExists(*SaveDirectory))
		{
			//Could not make the specified directory
			return false;
			//~~~~~~~~~~~~~~~~~~~~~~
		}
	}

	//get complete file path
	SaveDirectory += "\\";
	SaveDirectory += FileName;

	//No over-writing?
	if (!AllowOverWriting)
	{
		//Check if file exists already
		if (IFileManager::Get().GetFileAgeSeconds(*SaveDirectory) > 0)
		{
			//no overwriting
			return false;
		}
	}

	return FFileHelper::SaveStringToFile(SaveText, *SaveDirectory);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSteamWorkshopToolkitModule, SteamWorkshopToolkit)