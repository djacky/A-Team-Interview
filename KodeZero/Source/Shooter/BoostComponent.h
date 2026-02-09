// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shooter/EnumTypes/BoostType.h"
#include "BoostComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTER_API UBoostComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBoostComponent();
	friend class AShooterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Heal(float HealAmount, float HealingTime);

	void AddShield(float ShieldAmount, float ShieldTime);
	void UseBoost(class ABoostItem* BoostItem);

	UPROPERTY()
	class UAudioComponent* ItemEffectSoundComponent;

	void RemoveItem(ABoostItem* BoostItem);

	void StartGrappleLocal(ABoostItem* BoostItem, FHitResult TraceHit);

	UFUNCTION(Server, Reliable)
	void ServerStartGrapple(ABoostItem* BoostItem, FHitResult TraceHit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartGrapple(ABoostItem* BoostItem, FHitResult TraceHit);

	UFUNCTION(Client, Reliable)
	void ClientInventoryAnim(int32 CurrentIndex, int32 NewIndex);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartBoost(FName ItemMontageSection, EBoostType InBoostType);

	void StartBoost();

	//UFUNCTION(Server, Reliable)
	void TakeItem(ABoostItem* BoostItem, bool bNewItem);

protected:
	virtual void BeginPlay() override;

	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);
	void SuperJump();

	void UpdateSlotIndex();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Boost, meta = (AllowPrivateAccess = "true"))
	class ABoostItem* BoostPickup;

	UFUNCTION(BlueprintCallable)
	void ItemMontageFinished();

	UFUNCTION(BlueprintCallable)
	void ItemEffect();

	UFUNCTION(BlueprintCallable)
	void ItemEffectSound();

	UFUNCTION(BlueprintCallable)
	void DetachItem();

	int32 CarriedItems;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Boost, meta = (AllowPrivateAccess = "true"))
	ABoostItem* EquippedBoost;

	void UpdateBoostAmount(ABoostItem* TakenBoost);

	UFUNCTION(Server, Reliable)
	void ServerStartBoost(FName ItemMontageSection, ABoostItem* BoostItem);

	UFUNCTION(Server, Reliable)
	void ServerStartSuperPunch(FName ItemMontageSection, ABoostItem* BoostItem);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartSuperPunch(FName ItemMontageSection, ABoostItem* BoostItem);

	UFUNCTION(Server, Reliable)
	void ServerStartSuperJump(FName ItemMontageSection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartSuperJump(FName ItemMontageSection);

	UFUNCTION()
	void UsingItemFinished(ABoostItem* BoostItem);

	UFUNCTION()
	void RunGrappleLocal(ABoostItem* BoostItem, FHitResult TraceHit);

	void SemiRandomTeleport();
	//void InitializeTeleportLocations();
	int32 GetTeleportLocationIndex(TArray<AActor*> TeleportLocations);

	FVector_NetQuantize GetTeleportLocation();

	void RunUseBoost(ABoostItem* BoostItem);

	UPROPERTY()
	class AShooterCharacter* Character;

private:


	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;

	bool bAddingShield = false;
	float AddShieldRate = 0.f;
	float ShieldAddAmount = 0.f;

	UPROPERTY(VisibleAnywhere, Category = Boost)
	UParticleSystemComponent* ItemEffectComponent;

	bool bNewBoostItem;

	UPROPERTY()
	FTimerHandle ItemTimer;

	//TArray<FVector> TeleportLocations;

	UPROPERTY(EditAnywhere, Category = Boost);
	TSubclassOf<class ATargetPoint> TeleportLocationClass;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};

class ItemAsync : public FNonAbandonableTask
{
public:
	ItemAsync(ABoostItem* BoostItem);
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(ItemAsync, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork(class UBoostComponent* BoostClass);
	UPROPERTY()
	ABoostItem* BoostItemAsync;
};