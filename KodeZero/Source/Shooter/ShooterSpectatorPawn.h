// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "ShooterSpectatorPawn.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AShooterSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	AShooterSpectatorPawn();

	virtual void Tick(float DeltaTime) override;

	void SetTarget(AActor* NewTarget);

	UPROPERTY(Replicated)
	AActor* TargetActor = nullptr;

    UPROPERTY(EditAnywhere, Category = "Camera")
    FVector CameraOffset = FVector(-250.0f, 70.f, 100.0f); // Behind & above the winner

    UPROPERTY(EditAnywhere, Category = "Camera")
    float SmoothSpeed = 10.0f; // Adjust this for smoother movement

    UFUNCTION(Server, Reliable)
    void ServerSetTarget(AActor* NewTarget);
	
};
