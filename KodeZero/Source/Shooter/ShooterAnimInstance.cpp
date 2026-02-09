// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterAnimInstance.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"


void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
    if (ShooterCharacter == nullptr)
    {
        ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
    }
    if (ShooterCharacter == nullptr) return;

    bCrouching = ShooterCharacter->GetCharacterMovement()->IsCrouching(); // ShooterCharacter->GetCrouching();
    bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;
    bEquipping = ShooterCharacter->GetCombatState() == ECombatState::ECS_Equipping;
    bUnequippedState = ShooterCharacter->GetUnEquippedState();
    bHitting = ShooterCharacter->GetComboPlaying();
    bHittingAir = ShooterCharacter->GetHittingInAir();
    bPlayerElim = ShooterCharacter->GetPlayerEliminated();
    EquippedWeapon = ShooterCharacter->GetEquippedWeapon();
    EquippedItem = ShooterCharacter->GetEquippedItem();
    if (EquippedItem) EquippedItemTypeAnim = EquippedItem->GetItemType();
    ComboMontageSection = ShooterCharacter->GetComboMontageSectionPlaying();
    bShouldUseFABRIK = (ShooterCharacter->GetCombatState() == ECombatState::ECS_Unoccupied || 
                        ShooterCharacter->GetCombatState() == ECombatState::ECS_FireTimerInProgress); // ||
                        // ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading);
    // Get the lateral speed of the character from velocity
    FVector Velocity{ ShooterCharacter->GetVelocity() };
    Velocity.Z = 0.f;
    Speed = Velocity.Size();
    Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, ShooterCharacter->GetActorRotation());

    // Is the character in the air?
    //bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling() || ShooterCharacter->GetStartFlying();
    bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();
    bFlying = ShooterCharacter->GetStartFlying() || ShooterCharacter->GetHoverState() == EHoverState::EHS_HoverStart || 
        ShooterCharacter->GetHoverState() == EHoverState::EHS_HoverRush || ShooterCharacter->GetIsGrappling();
    HoverState = ShooterCharacter->GetHoverState();
    bGrappling = ShooterCharacter->GetStartGrapplePull();
    bGrappleItemEquipped = ShooterCharacter->GetGrappleItemEquipped();
    bHandShield = ShooterCharacter->GetHandShield();
    if (!bIsInAir)
    {
        bPlayerWon = ShooterCharacter->GetPlayerWon();   
    }

    // Is the character accelerating?
    if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
    {
        bIsAccelerating = true;
    }
    else
    {
        bIsAccelerating = false;
    }

        // FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
    FRotator AimRotation = FRotator(ShooterCharacter->CurrentInterpolatedRotation.Pitch, ShooterCharacter->CurrentInterpolatedRotation.Yaw, 0.0f);
    //FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);
    //UE_LOG(LogTemp, Log, TEXT("AnimBlueprint AimRotation: Pitch=%f, Yaw=%f, Roll=%f"), AimRotation.Pitch, AimRotation.Yaw, AimRotation.Roll);
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
    //FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation: %f"), MovementRotation.Yaw);
    MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
    
    if (ShooterCharacter->GetVelocity().Size() > 0.f)
    {
        LastMovementOffsetYaw = MovementOffsetYaw;
    }

    bAiming = ShooterCharacter->GetAiming();
    
    if (GEngine)
    {
        //GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, MovementRotationMessage);
    }

    if (bReloading)
    {
        OffsetState = EOffsetState::EOS_Reloading;
    }
    else if (bAiming)
    {
        OffsetState = EOffsetState::EOS_Aiming;
    }
    else if (bIsInAir)
    {
        OffsetState = EOffsetState::EOS_InAir;
    }
    else
    {
        OffsetState = EOffsetState::EOS_Hip;
    }
    // Check if ShooterCharacter has a valid EquippedWeapon
    if (ShooterCharacter->GetEquippedWeapon())
    {
        EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
        
        /*
        switch (EquippedWeaponType)
        {
        case EWeaponType::EWT_CyberPistol:
        case EWeaponType::EWT_Pistol:
            bUseCustomFire = bAiming && ShooterCharacter->GetIsFiring();
            break;
        
        default:
            bUseCustomFire = false;
            break;
        }
        */
    }

    TurnInPlace();
    Lean(DeltaTime);
    
    bool bFABRIKCond = EquippedWeapon && EquippedWeapon->GetItemMesh() && ShooterCharacter->GetMesh(); 
    if (bFABRIKCond)
    {
        if (!EquippedWeapon->bIsAnNFT && EquippedWeapon->GetItemMesh())
        {
            LeftHandTransform = EquippedWeapon->GetItemMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        }
        else if (EquippedWeapon->bIsAnNFT && EquippedWeapon->GetNFTMesh())
        {
            // *** Need to fix this for new NFT
            LeftHandTransform = EquippedWeapon->GetNFTMesh()->GetComponentTransform();
        }
        
        FVector OutPosition;
        FRotator OutRotation;
        // Get the position of the LeftHandSocket relative to the right hand
        ShooterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
        LeftHandTransform.SetLocation(OutPosition);
        LeftHandTransform.SetRotation(FQuat(OutRotation));
        /*
		if (ShooterCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetItemMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - ShooterCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);
		}
        */
    }
    FlyingVars(DeltaTime);
    //if (GEngine) GEngine->AddOnScreenDebugMessage(2, -1, FColor::Red, FString::Printf(TEXT("Is Flying?: %i"), bFlying));
}

void UShooterAnimInstance::FlyingVars(float DeltaTime)
{
    FVector UnRotateVector = UKismetMathLibrary::Quat_UnrotateVector(ShooterCharacter->GetActorRotation().Quaternion(), ShooterCharacter->GetVelocity());
    MaxFlySpeed = ShooterCharacter->GetCharacterMovement()->MaxFlySpeed;
    HoverSpeedX = UKismetMathLibrary::MapRangeClamped(UnRotateVector.X, -MaxFlySpeed, MaxFlySpeed, -1.f, 1.f);
    HoverSpeedY = UKismetMathLibrary::MapRangeClamped(UnRotateVector.Y, -MaxFlySpeed, MaxFlySpeed, -1.f, 1.f);
    if (HoverState == EHoverState::EHS_HoverRush && ShooterCharacter)
    {
        FVector Velocity = ShooterCharacter->GetVelocity();
        FVector NormVelocity = Velocity;
        UKismetMathLibrary::Vector_Normalize(NormVelocity, 0.0001f);
        HighSpeedVerticalSpeed = FMath::FInterpTo(HighSpeedVerticalSpeed, NormVelocity.Z, DeltaTime, 5.f);
        
        LastVelocityRotation = FRotator(
            UKismetMathLibrary::Conv_VectorToRotator(Velocity).Pitch,
            ShooterCharacter->GetActorRotation().Yaw,
            ShooterCharacter->GetActorRotation().Roll
        );

        FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(LastVelocityRotation, PreviousVelocityRotation);
        YawVelocityDifference = DeltaRot.Yaw / DeltaTime;
        PitchVelocityDifference = DeltaRot.Pitch / DeltaTime;
        PreviousVelocityRotation = LastVelocityRotation;
        RushLean.X = FMath::FInterpTo(RushLean.X, UKismetMathLibrary::MapRangeClamped(YawVelocityDifference, -180.f, 180.f, -1.f, 1.f), DeltaTime, 5.f);
        RushLean.Y = FMath::FInterpTo(RushLean.Y, UKismetMathLibrary::MapRangeClamped(PitchVelocityDifference, -180.f, 180.f, -1.f, 1.f), DeltaTime, 15.f);
        //UE_LOG(LogTemp, Warning, TEXT("High Speed V: %f | LeanX; %f | LeanY: %f"), HighSpeedVerticalSpeed, RushLean.X, RushLean.Y);
    }
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
    ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace()
{
    if (ShooterCharacter == nullptr) {return;}
    // CharacterPitch = ShooterCharacter->GetBaseAimRotation().Pitch; // Gets the pitch based on the aiming direction
    CharacterPitch = ShooterCharacter->CurrentInterpolatedRotation.Pitch; // Gets the pitch based on the aiming direction
    
    if (Speed > 0 || bIsInAir)
    {
        RootYawOffset = 0.f;
        TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
        TIPCharacterYawLastFrame = TIPCharacterYaw;
        RotationCurveLastFrame = 0.f;
        RotationCurve = 0.f;
        bStartedToMove = true;
    }
    else if (bStartedToMove)
    {
        TIPCharacterYawLastFrame = TIPCharacterYaw;
        TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
        
        const float TIPYawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;

        // RootYawOffset update and clapled to [-180, 180]
        RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

        // 1.0 if turning, 0.0 if not
        const float Turning = GetCurveValue(TEXT("Turning"));
        bCrouching == true || bEquipping == true ? RecoilWeight = 0.1f : bAiming == true ? RecoilWeight = 0.7f : RecoilWeight = 0.7f;

        if (Turning > 0)
        {
            RecoilWeight = 0.f;
            RotationCurveLastFrame = RotationCurve;
            RotationCurve = GetCurveValue(TEXT("Rotation"));
            const float DeltaRotation = RotationCurve - RotationCurveLastFrame;

            // If RootYawOffset > 0, --> we are turning left
            // If RootYawOffset < 0, --> we are turning right
            RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;
        
            const float ABSRootYawOffset = FMath::Abs(RootYawOffset); 
            if (ABSRootYawOffset > 90.f)
            {
                const float YawExcess = ABSRootYawOffset - 90.f;
                RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
            }
        }
    }
    else
    {
        RootYawOffset = 0.f;
    }
    //UE_LOG(LogTemp, Warning, TEXT("RootYawOffset = %f"), RootYawOffset);
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
    if (ShooterCharacter == nullptr) {return;}

    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = ShooterCharacter->GetActorRotation();

    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

    const float Target = Delta.Yaw / DeltaTime;
    const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f);
    YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

    //if (GEngine) GEngine->AddOnScreenDebugMessage(2, -1, FColor::Red, FString::Printf(TEXT("YawDelta: %f"), YawDelta));
}

