// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameSettings.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UGameSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MatchTime;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bSpawnAIs;
	
	
};
