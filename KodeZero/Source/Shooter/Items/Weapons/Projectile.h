// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "Shooter/Items/AllDamageTypes.h"
#include "Projectile.generated.h"

#define ECC_Projectile ECollisionChannel::ECC_GameTraceChannel8

UCLASS()
class SHOOTER_API AProjectile : public AActor, public IShooterInterface
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;
	void StartDestroyTimer();
	virtual void DestroyTimerFinished();
	void SpawnTrailSystem(const FVector& InScale = FVector(1.0f, 1.0f, 1.0f));
	void SpawnTracerSystem();
	virtual void ExplodeDamage(TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass());

	UFUNCTION()
	virtual void CheckIfNotDestroyed();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 37000;

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.f;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 500.f;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TrailSystem;

	UPROPERTY()
	class UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.f;

	float ImpactParticleScale = 1.f;

	UPROPERTY()
	FTimerHandle CheckProjectileTimer;

	bool bRegisteredHit = false;

	UPROPERTY(EditAnywhere)
	class UParticleSystem* Tracer;

	UPROPERTY(EditAnywhere)
	class UParticleSystemComponent* TracerComponent;

	//float BaseDamage = 1.f;

private:

	UPROPERTY()
	FTimerHandle DestroyTimer;


public:	
	class AShooterCharacter* GetShooterCharacter_Implementation();

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticlesLegacy;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;
	
	FORCEINLINE UStaticMeshComponent* GetProjectileMesh() const { return ProjectileMesh; }
	//FORCEINLINE void SetBaseDamage(float NewDamage) { BaseDamage = NewDamage; }

	void StartDestroyCheck();

};
