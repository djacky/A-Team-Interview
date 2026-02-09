// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/EnumTypes/HelicopterWeaponType.h"
#include "Engine/DataTable.h"
#include "Shooter/EnumTypes/AmmoType.h"
#include "HelicopterWeapon.generated.h"

USTRUCT(BlueprintType)
struct FHelicopterWeaponDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WeaponName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponDamageAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundCue* EquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* WeaponMesh = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IconBackground = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* InventoryIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairMiddle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AutoFireRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleFlash = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUsesAmmo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAmmoType AmmoType = EAmmoType::EAT_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WeaponAmmoCapacity = 0;

};

UCLASS()
class SHOOTER_API AHelicopterWeapon : public AActor
{
	GENERATED_BODY()
	
public:	

	AHelicopterWeapon();
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

protected:
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

private:	

	// Skeletal mesh for the weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_WeaponType, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EHelicopterWeaponType WeaponType = EHelicopterWeaponType::EHWT_MAX;

	UFUNCTION()
	void OnRep_WeaponType();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bAutomatic = false;

	// Inventory item icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UTexture2D* InventoryIcon;

	// Inventory item icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconBackground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* HitSound;

	// Ammo count for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType;

	int32 Sequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 WeaponAmmoCapacity;


public:

	FORCEINLINE EHelicopterWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE void SetWeaponType(EHelicopterWeaponType InType) { WeaponType = InType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE void SetAmmo(int32 InAmmo) { Ammo = InAmmo; }
	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }

	void UpdateWeapon();

	void FireProjectile(const FHitResult& WeaponTraceHit, class AShooterCharacter* AttackerCharacter, APawn* ControllingPawn);

	void SpendRound(AShooterCharacter* ShooterChar);

	void OnEquipped();

	// Slot in the inventory array
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 SlotIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* FireSound;

	// Rate of automatic gun fire
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float AutoFireRate;

	// Flash spawed at BarrelSocket
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* MuzzleFlash;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairMiddle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float WeaponDamageAmount;

	// Sound played when you equip a Item
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* EquipSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FString WeaponName = "Default";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bUsesAmmo = false;
};
