// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "EnemyAIInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEnemyAIInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTER_API IEnemyAIInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetTarget(AActor* InCharacter, bool bIsNew);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StopSearch();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnElimmed();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool CanTarget();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnValidTarget(AActor* AttackerAI);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void PlayTauntSound();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	USceneComponent* OnGameEndWinner();
	
};
