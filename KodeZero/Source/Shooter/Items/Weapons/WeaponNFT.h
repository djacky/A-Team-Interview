// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Weapons/Weapon.h"
#include "Shooter/StructTypes/WeaponPropertyStruct.h"
#include "Components/TimelineComponent.h"
#include "WeaponNFT.generated.h"

/**
 * 
 */

UCLASS()
class SHOOTER_API AWeaponNFT : public AWeapon
{
	GENERATED_BODY()

protected:
	AWeaponNFT();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void SetItemProperties() override;

	virtual void UpdateWeapon() override;

	virtual UStaticMeshComponent* GetNFTMesh() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MainWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ClipMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USoundAttenuation> FireSoundAtt;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Weapon)
	UMaterialInstanceDynamic* DynamicDropMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Weapon)
	UMaterialInstance* DropMaterialInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TArray<UMaterialInterface*> AllMaterialsMain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	TArray<UMaterialInterface*> AllMaterialsClip;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	UTimelineComponent* DropEffectTimeline;
	UPROPERTY()
	FOnTimelineFloat DropEffectTrack;

	UPROPERTY(EditAnywhere, Category = Weapon)
	UCurveFloat* DropEffectCurve;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	class UWeaponPickupWidget* WeaponWidget;

	UPROPERTY(EditAnywhere, Category = Weapon)
	class USoundCue* SpawnSound;

	UPROPERTY(EditAnywhere, Category = Weapon)
	class UParticleSystem* SpawnEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	class UNiagaraSystem* DroppedEffectSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* DroppedEffectComponent;

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponData, Category = Weapon)
	FWeaponDataStruct WeaponData;

	UFUNCTION()
	void OnRep_WeaponData();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpawnEffects();

	void StartDropEffect(bool bIsDropped);

	UFUNCTION()
	void UpdateDropMaterial(float DropValue);

	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void ThrowWeapon(bool bUnequipped = false, FTransform CharacterTransform = FTransform(), float ThrowImpulse = 10000.f, bool bRandomDirection = false) override;
	virtual void FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter) override;
	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed) override;

	virtual void OnRep_ItemState() override;

	virtual void AttachClip(AShooterCharacter* ShooterChar, bool bAttach) override;

};
