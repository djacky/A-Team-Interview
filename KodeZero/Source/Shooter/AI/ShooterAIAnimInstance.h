// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterAnimInstance.h"
#include "ShooterAIAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UShooterAIAnimInstance : public UShooterAnimInstance
{
	GENERATED_BODY()

public:

	virtual void UpdateAnimationProperties(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class AShooterAI* ShooterAICharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	bool bIsTargetValid;
	
};
