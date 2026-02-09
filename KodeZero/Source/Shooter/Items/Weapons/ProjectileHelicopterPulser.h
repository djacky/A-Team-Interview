// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Projectile.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "ProjectileHelicopterPulser.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileHelicopterPulser : public AProjectile, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	AProjectileHelicopterPulser();

	EWeaponType GetWeaponType_Implementation() override;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	float WeaponSpeed = 10000.f;

	void ShowHitNumber(class AHelicopter* HelicopterActor, float DamageAmount, FVector HitLocation, bool bHeadshot, bool bShield = false);
	float SetGunDamageAmount(class AShooterCharacter* DamagedCharacter, UPrimitiveComponent* HitComponent, class AHelicopter* OwnerHelicopter);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* BoxSceneComponent;

};
