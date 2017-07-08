// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"
#include "Input/Reply.h"
#include "Engine/EngineTypes.h"
#include "Editor.h"
#include "Editor/UATHelper/UATHelperModule.h"
#include "Editor/UATHelper/Public/IUATHelperModule.h"
#include "Misc/MonitoredProcess.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "steam/steam_api.h"

class FToolBarBuilder;
class FMenuBuilder;

/* Event Data
*****************************************************************************/

struct EventData
{
	FString EventName;
	bool bProjectHasCode;
	double StartTime;
	IUATHelperModule::UatTaskResultCallack ResultCallback;
};

/* FMainFrameActionCallbacks callbacks
*****************************************************************************/
class FMainFrameActionsNotificationTask
{
public:

	FMainFrameActionsNotificationTask(TWeakPtr<SNotificationItem> InNotificationItemPtr, SNotificationItem::ECompletionState InCompletionState, const FText& InText)
		: CompletionState(InCompletionState)
		, NotificationItemPtr(InNotificationItemPtr)
		, Text(InText)
	{ }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (NotificationItemPtr.IsValid())
		{
			if (CompletionState == SNotificationItem::CS_Fail)
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileFailed_Cue.CompileFailed_Cue"));
			}
			else
			{
				GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));
			}

			TSharedPtr<SNotificationItem> NotificationItem = NotificationItemPtr.Pin();
			NotificationItem->SetText(Text);
			NotificationItem->SetCompletionState(CompletionState);
			NotificationItem->ExpireAndFadeout();
		}
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMainFrameActionsNotificationTask, STATGROUP_TaskGraphTasks);
	}

private:

	SNotificationItem::ECompletionState CompletionState;
	TWeakPtr<SNotificationItem> NotificationItemPtr;
	FText Text;
};

class FSteamWorkshopToolkitModule : public IModuleInterface
{
public:

	// steam workshop stuff
	AppId_t AppId;

	///* Steam UGC details */
	//void OnUGCRequestUGCDetails( *pResult, bool bIOFailure);
	//CCallResult<UGameInstance, SteamUGCRequestUGCDetailsResult_t> m_callResultUGCRequestDetails;
	
	void OnMapCreated(CreateItemResult_t *pCallback, bool bIOFailure);
	CCallResult<FSteamWorkshopToolkitModule, CreateItemResult_t> m_callResultCreateItem;

	void OnMapUploaded(SubmitItemUpdateResult_t *pCallback, bool bIOFailure);
	CCallResult<FSteamWorkshopToolkitModule, SubmitItemUpdateResult_t> m_callResultUpdateItem;

	bool SaveStringTextToFile(FString SaveDirectory, FString FileName, FString SaveText, bool AllowOverWriting);

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
	USteamWorkshopMap* GetMap();
	FString GetUGCPath();

	FReply OnCreateMap();
	FReply OnCook();
	FReply OnPackage();
	FReply OnUpload();
	FReply OnOpenWorkshopPage();

	FDelegateHandle DelegateHandle_SteamCallbacks;
	bool RunSteamCallbacks(float DeltaTime);

	PublishedFileId_t FileId;

	void CookMap(USteamWorkshopMap* Map);
	void PackageMap(USteamWorkshopMap* Map);

	/** Used to callback into calling code when a UnrealPak task completes. First param is the result type, second param is the runtime in sec. */
	typedef TFunction<void(FString, double)> UnrealPakTaskResultCallack;

	void CreateUnrealPakTask(const FString& CommandLine, const FText& PlatformDisplayName, const FText& TaskName, const FText &TaskShortName, const FSlateBrush* TaskIcon, UnrealPakTaskResultCallack ResultCallback);

	UFUNCTION()
		static void HandleUnrealPakHyperlinkNavigate();
	UFUNCTION()
		static void HandleUnrealPakCancelButtonClicked(TSharedPtr<FMonitoredProcess> PackagerProcess);
	UFUNCTION()
		static void HandleUnrealPakCancelButtonClicked(TWeakPtr<FMonitoredProcess> PackagerProcessPtr);
	UFUNCTION()
		static void HandleUnrealPakProcessCanceled(TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event);
	UFUNCTION()
		static void HandleUnrealPakProcessCompleted(int32 ReturnCode, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName, EventData Event);
	UFUNCTION()
		static void HandleUnrealPakProcessOutput(FString Output, TWeakPtr<SNotificationItem> NotificationItemPtr, FText PlatformDisplayName, FText TaskName);

private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};
