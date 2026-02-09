// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Projectile.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "ProjectileGravity.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AProjectileGravity : public AProjectile, public IWeaponHitInterface
{
	GENERATED_BODY()
	
public:
	AProjectileGravity();

	EWeaponType GetWeaponType_Implementation() override;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGravitySphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	// Called when end overlapping AreaSphere
	UFUNCTION()
	void OnGravitySphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gravity, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* GravitySphere;

	void SetShooterGravityPars(bool bSetPull);

	virtual void Destroyed() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnEffects(const FHitResult& Hit);

	void OnOverlappedTriggered(bool bSetPull, AActor* OverlappedActor);

private:

	bool bStartPull = false;

	UPROPERTY(EditAnywhere, Category = Gravity)
	TSubclassOf<class AShooterCharacter> ShooterCharacterClass;

	UPROPERTY()
	class UAudioComponent* StartPullSoundComponent;

	UPROPERTY(EditAnywhere, Category = Gravity)
	class USoundBase* StartPullSound;

	UPROPERTY(EditAnywhere, Category = Gravity)
	UParticleSystem* GravityEffect;

};
