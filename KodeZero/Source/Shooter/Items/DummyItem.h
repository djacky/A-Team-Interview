// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shooter/Item.h"
#include "DummyItem.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API ADummyItem : public AItem
{
	GENERATED_BODY()

protected:
	ADummyItem();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* MainComponent;

public:

	void UpdateDummyItem();
	virtual void OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance) override;

	virtual void SetItemProperties() override;
};
