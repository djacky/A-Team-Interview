// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterMovementComponent.h"
#include "ShooterCharacter.h"  // Include your character header

UShooterMovementComponent::UShooterMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    WantsState = EMovementModifiers::EMM_None;
}

float UShooterMovementComponent::GetMaxSpeed() const
{
    if (!CachedOwner)
    {
        CachedOwner = Cast<AShooterCharacter>(GetPawnOwner());
    }
    if (!CachedOwner) return Super::GetMaxSpeed(); 

    if (CachedOwner->IsGrappling())
    {
        return 2.f * CachedOwner->GetBaseFlySpeed(); // Grapple speed overrides all
    }

    switch (WantsState)
    {
    case EMovementModifiers::EMM_Stop:
        return 0.f;
    case EMovementModifiers::EMM_DashGround:
        return 8.f * CachedOwner->GetBaseMovementSpeed();
    case EMovementModifiers::EMM_DashAir:
        return 3.f * CachedOwner->GetBaseMovementSpeed();
    case EMovementModifiers::EMM_ChargeAttack:
        return 5.f * CachedOwner->GetBaseMovementSpeed();
    case EMovementModifiers::EMM_Crouch:
        return CachedOwner->GetCrouchMovementSpeed();
    case EMovementModifiers::EMM_Normal:
        return CachedOwner->GetBaseMovementSpeed();
    case EMovementModifiers::EMM_AIPatrol:
        return 0.6f * CachedOwner->GetBaseMovementSpeed();
    
    default:
        break;
    }

    return Super::GetMaxSpeed();
}

float UShooterMovementComponent::GetMaxAcceleration() const
{
    if (!CachedOwner)
    {
        CachedOwner = Cast<AShooterCharacter>(GetPawnOwner());
    }
    if (!CachedOwner) return Super::GetMaxAcceleration(); 

    if (CachedOwner->IsGrappling())
    {
        return 2.f * CachedOwner->GetBaseAcceleration(); // Grapple accel overrides all
    }

    switch (WantsState)
    {
    case EMovementModifiers::EMM_ChargeAttack:
        return 6.f * CachedOwner->GetBaseAcceleration();
    case EMovementModifiers::EMM_DashGround:
    case EMovementModifiers::EMM_DashAir:
        return 1000.f * CachedOwner->GetBaseAcceleration();

    default:
        break;
    }

    return Super::GetMaxAcceleration();
}

float UShooterMovementComponent::GetMaxBrakingDeceleration() const
{
    if (!CachedOwner)
    {
        CachedOwner = Cast<AShooterCharacter>(GetPawnOwner());
    }
    if (!CachedOwner) return Super::GetMaxBrakingDeceleration();

    if (MovementMode == MOVE_Flying)
    {
        if (CachedOwner->IsGrappling())
        {
            return 10000.f; // Grapple braking overrides all
        }

        switch (WantsState)
        {
        case EMovementModifiers::EMM_GrappleStart:
            return 10000.f;  // High value for quick stopping near target
        // Add cases for other flying states if needed
        default:
            break;
        }
    }
    return Super::GetMaxBrakingDeceleration();
}

void UShooterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);

    // Decode state from bits 4-7 (custom flags)
    uint8 CompressedState = (Flags >> 4) & 0x0F;  // Extracts 4-bit value (0-15)
    WantsState = static_cast<EMovementModifiers>(CompressedState);
}

FNetworkPredictionData_Client* UShooterMovementComponent::GetPredictionData_Client() const
{
    check(PawnOwner != nullptr);

    // Gracefully handle if called on authority (e.g., listen server host)
    if (PawnOwner->GetLocalRole() == ROLE_Authority)
    {
        return nullptr;
    }

    if (!ClientPredictionData)
    {
        UShooterMovementComponent* MutableThis = const_cast<UShooterMovementComponent*>(this);
        if (MutableThis) MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Shooter(*this);
    }

    return ClientPredictionData;
}

void FSavedMove_Shooter::Clear()
{
    Super::Clear();
    WantsState = static_cast<uint8>(EMovementModifiers::EMM_None);
}

uint8 FSavedMove_Shooter::GetCompressedFlags() const
{
    uint8 Result = Super::GetCompressedFlags();

    // Compress state into bits 4-7
    uint8 CompressedState = static_cast<uint8>(static_cast<EMovementModifiers>(WantsState)) & 0x0F;
    Result |= (CompressedState << 4);  // Shifts into bits 4-7

    return Result;
}

bool FSavedMove_Shooter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
    FSavedMove_Shooter* NewShooterMove = static_cast<FSavedMove_Shooter*>(NewMove.Get());

    if (NewShooterMove && WantsState != NewShooterMove->WantsState)
    {
        return false;
    }

    return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_Shooter::SetMoveFor(ACharacter* InCharacter, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
    Super::SetMoveFor(InCharacter, InDeltaTime, NewAccel, ClientData);

    UShooterMovementComponent* CharMov = Cast<UShooterMovementComponent>(InCharacter->GetCharacterMovement());
    if (CharMov)
    {
        WantsState = static_cast<uint8>(CharMov->WantsState);
    }
}

FNetworkPredictionData_Client_Shooter::FNetworkPredictionData_Client_Shooter(const UCharacterMovementComponent& ClientMovement)
    : Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_Shooter::AllocateNewMove()
{
    return FSavedMovePtr(new FSavedMove_Shooter());
}
