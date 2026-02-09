// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "MissleSystem.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AMissleSystem : public AProjectile
{
	GENERATED_BODY()

public:
	AMissleSystem();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

	// Apply damage to all characters in the DamageSphere
	void ApplyFireDamage();
	void OnHitDamage();

	// Call timer to apply damage to characters in the DamageSphere
	void DamageSphereEvent();
	void SpawnLastingEffects();
	void StopEffects();

	UFUNCTION()
	void OnDamageBoxOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDamageBoxEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OverlapBoxEffects(class AShooterCharacter* CharacterInBox);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastMissleHit();

	void DestroyActorsAfterHit();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDestroyOverlappedActor(AActor* ActorToDestroy);

	void RemoveShooterSpawnLocation();

private:
	// Area where players get damaged if entered
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	//class UBoxComponent* DamageBox;

	// Area where players get damaged if entered
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* DamageBoxMesh;

	UPROPERTY(EditAnywhere, Category = "Damage Area");
	TSubclassOf<class ATargetPoint> FireTargetPointClass;

	UPROPERTY(EditAnywhere, Category = "Damage Area");
	TSubclassOf<ATargetPoint> SmokeTargetPointClass;

	UPROPERTY(EditAnywhere, Category = "Damage Area");
	TSubclassOf<AShooterCharacter> ShooterCharacterClass;

	UPROPERTY(EditAnywhere, Category = "Damage Area");
	TSubclassOf<class AItem> ItemClass;

	UPROPERTY(EditAnywhere, Category = "Damage Area");
	TSubclassOf<class AGroundContainer> ContainerClass;

	// ShooterCharacter contained in the DamageSphere
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	//class AShooterCharacter* DamagingCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	TArray<AShooterCharacter*> CharacterArray;

	// Sound played when missle is spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	class USoundCue* SpawnSound;

	// Audio component that generates the continuous thrust sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* ThrustComponent;

	// Is the ShooterCharacter in the DamageSphere?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	bool bInDamageSphere = false;

	// Did the missle have a hit?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MissleHit, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	bool bMissleHit = false;

	UFUNCTION()
	void OnRep_MissleHit();

	// Time to wait until damage is applied
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	float DamageDelay = 1.f;

	// Amount of damage to be applied when in the DamageSphere
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage Area", meta = (AllowPrivateAccess = "true"))
	float DamageSphereAmount = 1.f;

	UPROPERTY()
	FTimerHandle DamageTimer;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	//class UNiagaraSystem* SmokeSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* ImpactEffect;

	UPROPERTY()
	class UNiagaraComponent* SmokeSystemComponent;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	//UParticleSystem* MissileFireEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	TArray<UNiagaraSystem*> SmokeSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	TArray<UParticleSystem*> MissileFireEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	USoundCue* OverlapBoxSound;

	UPROPERTY()
	class AMapLevelScriptActor* LevelScriptActor;

	UPROPERTY(VisibleAnywhere, Replicated)
	bool bOverlappingDamageBox = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Missile, meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* BoxMaterial;

	void SetCollisions();

	void SetHitProperties();

public:
	FORCEINLINE bool GetMissleHit() const { return bMissleHit; }
	//FORCEINLINE UBoxComponent* GetDamageBox() const { return DamageBox; }
	FORCEINLINE UStaticMeshComponent* GetDamageBox() const { return DamageBoxMesh; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Missile, meta = (AllowPrivateAccess = "true"))
	bool bMissileLaunched = false;

	void SetProjectileVelocity(const FVector& Direction, float Speed);

};
