// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "ProjectileGrenade.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileGrenade : public AProjectile, public IWeaponHitInterface
{
	GENERATED_BODY()
public:
	AProjectileGrenade();
	virtual void Destroyed() override;

	EWeaponType GetWeaponType_Implementation() override;
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);
private:

	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
