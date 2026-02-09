// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Shooter/EnumTypes/MovementModifiers.h"
#include "ShooterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UShooterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
    UShooterMovementComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(Transient)
    EMovementModifiers WantsState;

    UPROPERTY(Transient)
    mutable class AShooterCharacter* CachedOwner = nullptr;

    virtual float GetMaxSpeed() const override;
    virtual float GetMaxAcceleration() const override;
    virtual float GetMaxBrakingDeceleration() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

    friend class FSavedMove_Shooter;
};

class FSavedMove_Shooter : public FSavedMove_Character
{
public:
    typedef FSavedMove_Character Super;

    uint8 WantsState;

    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
    virtual void SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
};

class FNetworkPredictionData_Client_Shooter : public FNetworkPredictionData_Client_Character
{
public:
    typedef FNetworkPredictionData_Client_Character Super;

    FNetworkPredictionData_Client_Shooter(const UCharacterMovementComponent& ClientMovement);

    virtual FSavedMovePtr AllocateNewMove() override;
	
	
};
