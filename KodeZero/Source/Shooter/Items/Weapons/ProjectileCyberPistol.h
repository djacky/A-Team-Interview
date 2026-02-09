// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Projectile.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "ProjectileCyberPistol.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileCyberPistol : public AProjectile, public IWeaponHitInterface
{
	GENERATED_BODY()
	
	
public:
	AProjectileCyberPistol();
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	void PerformConeScanAndSetHomingTarget();

	EWeaponType GetWeaponType_Implementation() override;
	
};
