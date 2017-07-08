// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"

#include "SteamWorkshopMap.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class STEAMWORKSHOPTOOLKIT_API USteamWorkshopMap : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(VisibleAnywhere)
		int64 UID;
	UPROPERTY(Category = Map, BlueprintReadWrite, EditAnywhere)
		FString Name;
	UPROPERTY(Category = Map, BlueprintReadWrite, EditAnywhere)
		FString Description;
	UPROPERTY(Category = Map, BlueprintReadWrite, EditAnywhere)
		TAssetPtr<UWorld> Map;
	UPROPERTY(Category = Map, BlueprintReadWrite, EditAnywhere)
		UTexture2D* Thumbnail;
};
