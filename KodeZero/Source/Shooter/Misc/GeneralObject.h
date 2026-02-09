// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GeneralObject.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UGeneralObject : public UObject
{
	GENERATED_BODY()

public:

	bool ReadJsonFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutJsonObject);
	
};
