// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Projectile.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "ProjectileHelicopterMissile.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileHelicopterMissile : public AProjectile, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	AProjectileHelicopterMissile();

	EWeaponType GetWeaponType_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	void MissileExplodeDamage(class AHelicopter* DamagedHelicopter);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* BoxSceneComponent;

private:
	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;

	UPROPERTY()
	class UMissileMovementComponent* MissileMovementComponent;

};
