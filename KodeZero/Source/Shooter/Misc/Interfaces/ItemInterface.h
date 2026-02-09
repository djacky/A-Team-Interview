// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ItemInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UItemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTER_API IItemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual void OnItemPickedUp(class AShooterCharacter* InShooter);

	virtual void OnEquipItem(AShooterCharacter* InShooter, class UAnimInstance* InAnimInstance);

	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed);

	virtual bool IsAnItem();
};
