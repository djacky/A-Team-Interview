// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "GameplayTagContainer.h"
#include "WeaponHitInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UWeaponHitInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTER_API IWeaponHitInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnHitScanWeaponHit(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnShotgunWeaponHit(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnProjectileWeaponHit(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnHandCombatHit(AShooterCharacter* InstigatorShooter, float Damage, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnMissedShot(AActor* InstigatorShooter, const FVector& ImpactPoint);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnStunned(AActor* StunnerActor);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnSlowDown(bool bIsSlow, AShooterCharacter* InstigatorShooter);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnGravityProjectileHit(bool bIsPulling, AActor* ProjectileGravity);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool HasShield();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	EWeaponType GetWeaponType();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FGameplayTagContainer GetStateTags();
};
