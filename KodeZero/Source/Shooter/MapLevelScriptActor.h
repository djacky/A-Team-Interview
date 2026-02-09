// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "MapLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AMapLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

public:
	AMapLevelScriptActor();
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	//UFUNCTION(BlueprintCallable, Category = LevelBlueprint)

	UFUNCTION(Server, Reliable)
	void ServerFireMissle();

	void MissleFireTimer();

    void InitializeMissileHitLocations();
	//void InitializeFireEffectLocations();
    //void SetupFireEffectArrays();

private:
	// Projectile missle class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AProjectile> MissleClass;

	// Store random missle hit location in array (so that missles don't hit the same place)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> RandomMissleHitLocations;

	// Store random integers in array (so that missles don't hit the same place)
	UPROPERTY()
	TArray<int32> RandomIntArray;

	UPROPERTY()
	FTimerHandle MissleSpawnTimer;

	// Time to wait until missle is launched
	float MissleSpawnDelay = 10.f;

	UPROPERTY()
	TArray<FVector> MissileStartLocation;
	UPROPERTY()
	TArray<FVector> MissileEndLocation;

	UPROPERTY()
	class AProjectile* ProjectileActor;

	UPROPERTY()
	class AMissleSystem* MissileActor;

	UPROPERTY(VisibleAnywhere)
	int32 LocationIndex;

	//TArray<FVector> FireLocation0;
	//TArray<FVector> FireLocation1;
	//TArray<FVector> FireLocation2;

	//TArray<FVector> SmokeLocation0;
	//TArray<FVector> SmokeLocation1;
	//TArray<FVector> SmokeLocation2;

public:
	//TArray<TArray<FVector>> ParticleSpawnArray;
	//TArray<TArray<FVector>> SmokeSpawnArray;
	//void UpdateEffectLocations();

	// Dynamic instance that we can change at runtime
	//UPROPERTY(VisibleAnywhere, Category = "Missle Properties")
	//UMaterialInstanceDynamic* DynamicMissileMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	//UPROPERTY(EditAnywhere, Category = "Missle Properties")
	//UMaterialInstance* MissileMaterialInstance;
	
};
