// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Shooter/EnumTypes/WeaponType.h"
#include "CoreMinimal.h"
#include "Item.h"
#include "Shooter/EnumTypes/AmmoType.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "Engine/DataTable.h"
#include "Shooter/StructTypes/ADSSettings.h"
//#include "GameplayTagContainer.h"
//#include "NativeGameplayTags.h"
//#include "Shooter/Misc/NativeWeaponTags.h"
#include "Weapon.generated.h"

//UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_WeaponTypePistol)
//UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_WeaponTypeSubmachineGun)

USTRUCT(BlueprintType)
struct FWeaponDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutomatic = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WeaponAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagazineCapacity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponDamageAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundCue* PickupSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* EquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* ItemMesh = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* InventoryIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* AmmoIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* MaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaterialIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClipBoneName = FName("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ReloadMontageSection = FName("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairLeft = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairRight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairTop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairBottom = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairMiddle = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AutoFireRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleFlash = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* CharacterHitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneToHide = FName("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAnimationAsset* FireAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ImpactParticles = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FADSSettings ADSSettings = FADSSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AScopeAttachment> ScopeClass;
};

/**
 * 
 */
UCLASS()
class SHOOTER_API AWeapon : public AItem, public IShooterInterface, public IWeaponHitInterface
{
	GENERATED_BODY()
public: 
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	int32 ReplCachedAmmo = 0;

	EWeaponType GetWeaponType_Implementation() override;

protected:
	
	virtual void StopFalling() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	void FinishMovingSlide();
	void FinishMovingShotgunSlide();
	void UpdateSlideDisplacement();
	void UpdateShotgunSlideDisplacement();

	void HideBoneOfBelicaWeapons();

	// Set up and fire shotgun (with random scatter)
	
	void ShotGunDamage(TMap<AShooterCharacter*, float> HitMap, TMap<class AShooterCharacterAI*, float> HitAIChar, TMap<class AHelicopter*, float> HitMapHelicopter, TMap<class AItemContainer*, float> HitMapContainer, AShooterCharacter* AttackerCharacter);

	virtual void OnEquipped() override;

	void ScreenMessage(FString Mess);

	virtual void OnRep_ItemState() override;

	void SetHUDAmmo(int32 InAmmo);
protected:

	// Ammo count for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo = 20;

	// Maximum ammo that weapon can hold
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 MagazineCapacity = 30;

	// The type of weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_WeaponType, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EWeaponType WeaponType = EWeaponType::EWT_MAX;

	UFUNCTION()
	void OnRep_WeaponType();

	// The type of ammo for this weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	// FName for the reload montage section
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ReloadMontageSection = TEXT("Reload SMG");

	// True when moving the clip while reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bMovingClip;

	// Name for the clip bone
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ClipBoneName = TEXT("smg_clip");

	// Data table for weapon properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UDataTable* WeaponDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairBottom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairMiddle;

	// Flash spawed at BarrelSocket
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* MuzzleFlash;

	// Niagara flash spawed at BarrelSocket (used for NFT weapon)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	class UNiagaraSystem* MuzzleFlashNiagara;

	// Smoke trail for bullets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;

	// Particles spawned upon bullet impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	// Randomized gunshot sound cue
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* FireSound;
	
	int32 PreviousMaterialIndex;

	// Name of the bone to hide on the weapon mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName BoneToHide;

	// Amount that the pistol slide is moved when firing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float SlideDisplacement = 0.f;

	// Amount that the shotgun slide is moved when firing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float ShotgunSlideDisplacement = 0.f;

	// Curve for the slide displacement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* SlideDisplacementCurve;

	// Curve for the shotgun slide displacement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ShotgunSlideDisplacementCurve;

	// Timer handle for updating SlideDisplacement
	UPROPERTY()
	FTimerHandle SlideTimer;

	// Timer handle for updating ShotgunSlideDisplacement
	UPROPERTY()
	FTimerHandle ShotgunSlideTimer;

	// Time for displacing the slide during pistol fire
	float SlideDisplacementTime = 0.08;

	// Time for displacing the slide during shotgun fire
	float ShotgunSlideDisplacementTime = 0.85;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	USoundCue* CharacterHitSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties")
	USoundCue* ReloadSound;

	// Number of shotgun pellets
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;
	
	// True when moving the pistol slide
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	bool bMovingSlide = false;

	// True when moving the shotgun slide
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	bool bMovingShotgunSlide = false;

	// Max distance for the slide on the pistol
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float MaxSlideDisplacement = 4.f;

	// Max distance for the slide on the shotgun
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float MaxShotgunSlideDisplacement = 8.f;

	// Amount that the pistol is rotated when firing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float PistolRotation = 0.f;

	// Max rotation for pistol recoil
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float MaxPistolRotation = 20.f;

	// True for auto gunfire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bAutomatic = true;

	// Damage caused by weapon
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float WeaponDamageAmount = 20.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bUseFABRIK = true;

	virtual void OnRep_Owner() override;

public:
	// Adds an impulse to the weapon
	virtual void ThrowWeapon(bool bUnequipped = false, FTransform CharacterTransform = FTransform(), float ThrowImpulse = 10000.f, bool bRandomDirection = false);
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE void SetWeaponType(EWeaponType WType) { WeaponType = WType; }
	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }
	//FORCEINLINE int32 GetAmmo() const { return Ammo; }
	virtual int32 GetAmmo() const override { return Ammo; }
	FORCEINLINE int32 GetRepAmmo() const { return ReplCachedAmmo; }
	FORCEINLINE UAnimationAsset* GetFireAnimation() const { return FireAnimation; }
	FORCEINLINE int32 GetMagazineCapacity() const { return MagazineCapacity; }
	FORCEINLINE FName GetReloadMontageSection() const { return ReloadMontageSection; }
	FORCEINLINE FName GetClipBoneName() const { return ClipBoneName; }
	FORCEINLINE bool GetUseFABRIK() const { return bUseFABRIK; }
	FORCEINLINE UTexture2D* GetCrosshairLeft() const { return CrosshairLeft; }
	FORCEINLINE UTexture2D* GetCrosshairRight() const { return CrosshairRight; }
	FORCEINLINE UTexture2D* GetCrosshairTop() const { return CrosshairTop; }
	FORCEINLINE UTexture2D* GetCrosshairBottom() const { return CrosshairBottom; }
	FORCEINLINE UTexture2D* GetCrosshairMiddle() const { return CrosshairMiddle; }
	FORCEINLINE UParticleSystem* GetBeamParticles() const { return BeamParticles; }
	FORCEINLINE UParticleSystem* GetImpactParticles() const { return ImpactParticles; }
	FORCEINLINE USoundCue* GetHitSound() const { return HitSound; }
	FORCEINLINE USoundCue* GetCharacterHitSound() const { return CharacterHitSound; }
	FORCEINLINE USoundCue* GetReloadSound() const { return ReloadSound; }
	FORCEINLINE void SetWeaponDamageAmount(float InDamageAmount) { WeaponDamageAmount = InDamageAmount; }
	FORCEINLINE float GetWeaponDamageAmount() const { return WeaponDamageAmount; }
	
	FORCEINLINE float GetAutoFireRate() const { return AutoFireRate; }
	FORCEINLINE UParticleSystem* GetMuzzleFlash() const { return MuzzleFlash; }
	FORCEINLINE USoundCue* GetFireSound() const { return FireSound; }
	FORCEINLINE bool GetAutomatic() const { return bAutomatic; }

	FORCEINLINE void SetMovingClip(bool Move) { bMovingClip = Move; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AProjectile> ProjectileClass;

	void StartSlideTimer();
	void StartShotgunSlideTimer();
	float SetWeaponDamage(const FString& HitBone);

	void FireShotgun(AShooterCharacter* AttackerCharacter, const TArray<FHitResult>& WeaponTraceHits);
	void ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets);
	void ShotgunTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, AShooterCharacter* AttackerCharacter);

	virtual void FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter);

	virtual void UpdateWeapon();
	void AddAmmo(int32 AmmoToAdd);
	void SpendRound();
	
	// Rate of automatic gun fire (can make this 2 * ShootTimeDuration from ShooterCharacter)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float AutoFireRate;

	// Rate of automatic gun fire (can make this 2 * ShootTimeDuration from ShooterCharacter)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float FixedAutoFireRate;

	virtual void OnItemPickedUp(AShooterCharacter* InShooter) override;
	virtual void OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance) override;
	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed) override;
	virtual bool IsAnItem() override;
	AShooterCharacter* GetShooterCharacter_Implementation() override;

	float SetShotgunDamageAmount(AShooterCharacter* AttackerCharacter, AShooterCharacter* DamagedCharacter, const FHitResult& WeaponTraceHit);

	UPROPERTY()
	FTransform ClipTransformNFT; 
	virtual void AttachClip(AShooterCharacter* ShooterChar, bool bAttach);

	void OnNonZeroSequence(int32 ClientSequence);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FADSSettings ADSSettings = FADSSettings();

	void ActivateScope(bool bActivate);
	FTransform GetScopeLensTransform();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	class AScopeAttachment* ScopeAttachment = nullptr;

	float GetEffectiveADSMultiplier(const float InHipFOV) const;

protected:
	void ShotgunTraceHit_Effect(FVector InTraceStart, FVector BeamVal, UWorld* World);
	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo, int32 ServerWeaponSequence);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 ServerAmmo);

	void ResetSequence(AShooterCharacter* InShooter);

};
