// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UShooterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTER_API IShooterInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnAmmoSphereOverlap(AAmmo* OverlappedAmmo);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnItemPickedUp(AActor* ItemActor, const FString& ItemType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnUpdateHUDBoostAmount(int32 BoostAmount, ABoostItem* BoostItem);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnFinishedReloading();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnShotgunShellReload();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	AShooterCharacter* GetShooterCharacter();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FVector GetPawnLocation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	float GetPawnRotationYaw();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnMatchEnded(AShooterPlayerController* ShooterController);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnEmoteFinished();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnRewardObtained(float CameraScaleVal);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnMissileHit(AActor* MissileActor, float DamageAmount);
};
