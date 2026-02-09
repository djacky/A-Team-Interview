// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAIAnimInstance.h"
#include "ShooterAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "Kismet/KismetMathLibrary.h"

void UShooterAIAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
    //Super::UpdateAnimationProperties(DeltaTime);
    if (ShooterAICharacter == nullptr)
    {
        ShooterAICharacter = Cast<AShooterAI>(TryGetPawnOwner());
    }
    if (ShooterAICharacter == nullptr) return;

    bIsTargetValid = ShooterAICharacter->GetTargetCharacter() != nullptr ? true : false; 

    bReloading = ShooterAICharacter->GetCombatState() == ECombatState::ECS_Reloading;
    bEquipping = ShooterAICharacter->GetCombatState() == ECombatState::ECS_Equipping;
    bUnequippedState = ShooterAICharacter->GetUnEquippedState();

    bPlayerElim = ShooterAICharacter->GetPlayerEliminated();
    EquippedWeapon = ShooterAICharacter->GetEquippedWeapon();
    EquippedItem = ShooterAICharacter->GetEquippedItem();
    if (EquippedItem) EquippedItemTypeAnim = EquippedItem->GetItemType();
    ComboMontageSection = ShooterAICharacter->GetComboMontageSectionPlaying();
    bShouldUseFABRIK = (ShooterAICharacter->GetCombatState() == ECombatState::ECS_Unoccupied || 
                        ShooterAICharacter->GetCombatState() == ECombatState::ECS_FireTimerInProgress); // ||
                        // ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading);
    // Get the lateral speed of the character from velocity
    FVector Velocity{ ShooterAICharacter->GetVelocity() };
    Velocity.Z = 0.f;
    Speed = Velocity.Size();

    // Is the character in the air?
    //bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling() || ShooterCharacter->GetStartFlying();
    bIsInAir = ShooterAICharacter->GetCharacterMovement()->IsFalling();
    bFlying = ShooterAICharacter->GetStartFlying() || ShooterAICharacter->GetHoverState() == EHoverState::EHS_HoverStart;
    bGrappleItemEquipped = ShooterAICharacter->GetGrappleItemEquipped();
    if (!bIsInAir)
    {
        bPlayerWon = ShooterAICharacter->GetPlayerWon();   
    }

    // Is the character accelerating?
    if (ShooterAICharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
    {
        bIsAccelerating = true;
    }
    else
    {
        bIsAccelerating = false;
    }

        // FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
    FRotator AimRotation = FRotator(ShooterAICharacter->ReplBaseAimRotationPitch, ShooterAICharacter->ReplBaseAimRotationYaw, 0.0f);
    //FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f | %f"), AimRotation.Pitch, AimRotation.Yaw);
    
    FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterAICharacter->GetVelocity());
    //FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation: %f"), MovementRotation.Yaw);
    MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
    
    if (ShooterAICharacter->GetVelocity().Size() > 0.f)
    {
        LastMovementOffsetYaw = MovementOffsetYaw;
    }
    bAiming = ShooterCharacter->GetAiming();

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
    if (ShooterAICharacter->GetEquippedWeapon())
    {
        EquippedWeaponType = ShooterAICharacter->GetEquippedWeapon()->GetWeaponType();
    }

    TurnInPlace();
    CharacterPitch = ShooterAICharacter->ReplBaseAimRotationPitch;
    Lean(DeltaTime);
    
    bool bFABRIKCond = EquippedWeapon && EquippedWeapon->GetItemMesh() && ShooterAICharacter->GetMesh(); 
    if (bFABRIKCond)
    {
        LeftHandTransform = EquippedWeapon->GetItemMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
        FVector OutPosition;
        FRotator OutRotation;
        // Get the position of the LeftHandSocket relative to the right hand
        ShooterAICharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
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
}