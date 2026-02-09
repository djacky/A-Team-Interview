// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Shooter/StructTypes/WeaponPropertyStruct.h"
#include "Shooter/StructTypes/EnemyDataStruct.h"
#include "EnumTypes/AmmoType.h"
#include "ProjectileBullet.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileBullet : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileBullet();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetBulletProperties(const FWeaponDataStruct &InWeaponData, const EAmmoType &InAmmoType);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bullet, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BulletMesh;

	UFUNCTION(BlueprintCallable)
	void Initialize(const FEnemyAttackData& InProjectileData);

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Bullet, meta = (AllowPrivateAccess = "true"))
	FWeaponDataStruct WeaponData;

    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    FEnemyAttackData ProjectileData = FEnemyAttackData();

	virtual void ExplodeDamage(TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass()) override;

	void ApplyProjectileData();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyProjectileData(const FEnemyAttackData& InProjectileData);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnTimerExpired();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnHit();

	bool bClientOnHit = false;

	void SetVelocity();

	void OnRegisteredHit();

	virtual void Destroyed() override;
	virtual void CheckIfNotDestroyed() override;

	void LaunchTarget(float SphereRadius);

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;
};
