// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemInterface.h"

void IItemInterface::OnItemPickedUp(AShooterCharacter* InShooter)
{
    // No default behavior
}

void IItemInterface::OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance)
{
    // No default behavior
}

void IItemInterface::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
    // No default behavior
}

bool IItemInterface::IsAnItem()
{
    return false;
}

