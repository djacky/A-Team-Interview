// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Shooter/StructTypes/EnemyTypeStruct.h"
#include "Shooter/StructTypes/EnemyDataStruct.h"
#include "CharacterGlobalFuncs.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UCharacterGlobalFuncs : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

protected:
    static void ApplyDamage(AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS, float DamageAmount);
    static void HandleKill(AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS);
    static void SetAttackMethodMetric(AActor* DamageCauser, AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS, const UDamageType* DamageType);

public:

    UFUNCTION(BlueprintCallable, Category = "Global|Shooter")
    static void GetShooterReferences(
        AShooterPlayerController*& VictimController, AShooterPlayerController*& AttackerController,
        AShooterPlayerState*& VictimPS, AShooterPlayerState*& AttackerPS,
        AActor* VictimActor, AActor* AttackerActor, AController* InstigatorCon);

	UFUNCTION(BlueprintCallable, Category = "Global|Shooter")
	static void UpdateDamageStat(AActor* DamagedCharacter, AActor* DamageCauserCharacter, 
		AShooterPlayerState* DamagedCharPS, AShooterPlayerState* DamageCauserCharPS, float Damage, float Health, 
        const UDamageType* DamageType);

	static FEnemyData* GetEnemyDataRow(EEnemyType EnemyType);
	
};
