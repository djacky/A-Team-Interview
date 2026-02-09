// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterCharacter.h"
#include "Shooter/EnumTypes/CharacterState.h"
#include "ShooterCharacterAI.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FAIMeshProperties
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> AnimInstanceClass = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USkeletalMesh* MeshAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ECharacterProperty CharacterSelected = ECharacterProperty::ECP_MAX;
};


UCLASS()
class SHOOTER_API AShooterCharacterAI : public AShooterCharacter
{
	GENERATED_BODY()

public:
	AShooterCharacterAI(const FObjectInitializer& ObjectInitializer);
	FORCEINLINE void SetElimMontageAI(UAnimMontage* InMontage) { ElimMontageAI = InMontage; }

	void OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound) override;

	void OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound) override;

	void OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass()) override;

	void OnSpawn(AShooterCharacter* ShooterOwner);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_OnSpawn)
	FAIMeshProperties MeshProperties;

	UFUNCTION()
	void OnRep_OnSpawn();


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser) override;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimAI();

	void PlayElimMontageAI();

	UFUNCTION(BlueprintImplementableEvent)
	void TriggerAnim();

	virtual void CameraOffsetWithDistance() override;

private:

	UPROPERTY(EditAnywhere, Category = "AI")
	class USoundCue* ElimSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ElimMontageAI;
};
