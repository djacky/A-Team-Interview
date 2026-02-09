// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Shooter/EnumTypes/BoostType.h"
#include "Engine/DataTable.h"
#include "BoostItem.generated.h"

USTRUCT(BlueprintType)
struct FBoostDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemMontageSection = FName("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ItemMontageEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundBase* ItemMontageSoundEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName HandSocketName = FName("");;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* PickupSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* EquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* ItemMesh = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ItemScale = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* AmmoIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* InventoryIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* BoostIconBackground = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* MaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaterialIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimBP;

};

UENUM(BlueprintType)
enum class EGrappleState : uint8
{
	EGS_Start UMETA(DisplayName = "Start"),
	EGS_Hit UMETA(DisplayName = "Hit"),
	EGS_Pull UMETA(DisplayName = "Pull"),
	EGS_Release UMETA(DisplayName = "Release"),

	EGS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class SHOOTER_API ABoostItem : public AItem
{
	GENERATED_BODY()

public: 
	ABoostItem();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void OnEquipped() override;

	void RotateMeshEffect(float DeltaTime);

	virtual void OnRep_ItemState() override;

	void EndGrappleSystem();
	//UPROPERTY(VisibleAnywhere)
	//class UProjectileMovementComponent* ProjectileMovementComponent;

	virtual void Destroyed() override;

	void SetHUDBoost(int32 InBoost);

	//UFUNCTION(Server, Reliable)
	void GrappleOnHit();

	//virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void StopFalling() override;
private:

	// The type of boost item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_BoostType, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	EBoostType BoostType = EBoostType::EBT_MAX;

	UFUNCTION()
	void OnRep_BoostType();

	// FName for the reload montage section
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	FName ItemMontageSection = TEXT("Use Health");

	int32 PreviousMaterialIndex;

	// Icon Background in the inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	UTexture2D* BoostIconBackground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	FVector ItemScale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	FName HandSocketName;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_BoostItemAmount, BlueprintReadOnly, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	int32 BoostItemAmount = 1;

	UFUNCTION()
	void OnRep_BoostItemAmount();

	// Healing properties
	UPROPERTY(VisibleAnywhere)
	float AmountToHealV1 = 100.f;

	UPROPERTY(VisibleAnywhere)
	float HealingTimeV1 = 15.f;

	UPROPERTY(VisibleAnywhere)
	float AmountToHealV2 = 25.f;

	UPROPERTY(VisibleAnywhere)
	float HealingTimeV2 = 1.5f;

	// Shield properties
	UPROPERTY(VisibleAnywhere)
	float AmountToReplenishV1 = 100.f;

	UPROPERTY(VisibleAnywhere)
	float ReplenishTimeV1 = 15.f;

	UPROPERTY(VisibleAnywhere)
	float AmountToReplenishV2 = 25.f;

	UPROPERTY(VisibleAnywhere)
	float ReplenishTimeV2 = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ItemMontageEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	USoundBase* ItemMontageSoundEffect;

	UFUNCTION(Client, Reliable)
	void ClientUpdateBoost(int32 ServerBoost);

	UFUNCTION(Client, Reliable)
	void ClientAddBoost(int32 BoostToAdd, class AShooterCharacter* InShooterCharacter);

	float InitialSpeed = 0.f;

	UPROPERTY()
	AShooterCharacter* ShooterCharacter;

	UPROPERTY(VisibleAnywhere, Category = "Boost Properties")
	UParticleSystemComponent* GrappleEffectComponent;

	UPROPERTY(EditAnywhere, Category = "Boost Properties")
	UParticleSystem* GrappleEffect;

	UPROPERTY(EditAnywhere, Category = "Boost Properties")
	UParticleSystem* EndGrappleEffect;

	UPROPERTY(EditAnywhere, Category = "Boost Properties")
	USoundBase* EndGrappleSound;

public:
	void ThrowItem(bool bUnequipped = false, FTransform CharacterTransform = FTransform(), float ThrowImpulse = 10000.f, bool bRandomDirection = false);
	FORCEINLINE EBoostType GetBoostType() const { return BoostType; }
	FORCEINLINE void SetBoostType(EBoostType BType) { BoostType = BType; }
	FORCEINLINE FName GetItemMontageSection() const { return ItemMontageSection; }
	FORCEINLINE UParticleSystem* GetItemMontageEffect() const { return ItemMontageEffect; }
	FORCEINLINE USoundBase* GetItemMontageSoundEffect() const { return ItemMontageSoundEffect; }
	//FORCEINLINE int32 GetBoostItemAmount() const { return BoostItemAmount; }
	virtual int32 GetBoostItemAmount() const override { return BoostItemAmount; }
	FORCEINLINE void SetBoostItemAmount(int32 BAmount) { BoostItemAmount = BAmount; }

	FORCEINLINE float GetAmountToHealV1() const { return AmountToHealV1; }
	FORCEINLINE float GetHealingTimeV1() const { return HealingTimeV1; }
	FORCEINLINE float GetAmountToHealV2() const { return AmountToHealV2; }
	FORCEINLINE float GetHealingTimeV2() const { return HealingTimeV2; }

	FORCEINLINE float GetAmountToReplenishV1() const { return AmountToReplenishV1; }
	FORCEINLINE float GetReplenishTimeV1() const { return ReplenishTimeV1; }
	FORCEINLINE float GetAmountToReplenishV2() const { return AmountToReplenishV2; }
	FORCEINLINE float GetReplenishTimeV2() const { return ReplenishTimeV2; }
	
	FORCEINLINE FVector GetItemScale() const { return ItemScale; }
	FORCEINLINE FName GetHandSocketName() const { return HandSocketName; }

	void SpendBoost();
	void AddBoost(int32 BoostToAdd, AShooterCharacter* ShooterOwnerCharacter);
	void UpdateBoost();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_GrappleState, Category = "Boost Properties", meta = (AllowPrivateAccess = "true"))
	EGrappleState GrappleState = EGrappleState::EGS_MAX;

	UFUNCTION()
	void OnRep_GrappleState();

	virtual void OnItemPickedUp(AShooterCharacter* InShooter) override;
	virtual void OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance) override;
	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed) override;
	virtual bool IsAnItem() override;
};
