// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "ShooterSpringArmComp.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UShooterSpringArmComp : public USpringArmComponent
{
	GENERATED_BODY()
	
public:
	/**
	 * If true, camera Z-Axis lags behind target position to smooth its movement.
	 * @see CameraLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bEnableCameraLagAxisX : 1;


	/**
	 * If true, camera Z-Axis lags behind target position to smooth its movement.
	 * @see CameraLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bEnableCameraLagAxisY : 1;


	/**
	 * If true, camera Z-Axis lags behind target position to smooth its movement.
	 * @see CameraLagSpeed
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Lag)
	uint32 bEnableCameraLagAxisZ : 1;

protected:
	/** Updates the desired arm location, calling BlendLocations to do the actual blending if a trace is done */
	void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;

};
