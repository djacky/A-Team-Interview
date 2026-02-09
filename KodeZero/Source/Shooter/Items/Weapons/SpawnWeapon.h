// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "Shooter/EnumTypes/RarityType.h"
#include "Shooter/EnumTypes/BoostType.h"
#include "Shooter/EnumTypes/AmmoType.h"
#include "SpawnWeapon.generated.h"

UCLASS()
class SHOOTER_API ASpawnWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnWeapon();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ReportNewSpawnItemTargetPoint(ATargetPoint* InTargetPoint);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<AActor*> ItemTargetPoints;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PreExit();

	void InitializeMaps();

	UFUNCTION(BlueprintCallable)
	void SpawnWeapons();

	void ShuffleArray(TArray<int32>& myArray);
	void CalcProbabilityArrays(float RarityFactor, int32 ArrayNum, TArray<int32>& ProbabilityArray);

private:
	UPROPERTY(EditAnywhere);
	TSubclassOf<class ATargetPoint> ItemTargetPointClass;

	UPROPERTY(VisibleAnywhere)
	TArray<EBoostType> BoostIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EWeaponType> WeaponIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EItemRarity> RarityIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EAmmoType> AmmoIndexArray;

	UPROPERTY()
	TArray<int32> BoostProbabliltyArray;
	UPROPERTY()
	TArray<int32> WeaponProbabliltyArray;
	UPROPERTY()
	TArray<int32> RarityProbabliltyArray;
	UPROPERTY()
	TArray<int32> AmmoProbabliltyArray;

	UPROPERTY()
	TArray<FVector> RandomLocationSpawn;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AWeapon> WeaponClass;
	
	UPROPERTY()
	class AWeapon* Weapon;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABoostItem> BoostClass;
	
	UPROPERTY()
	class ABoostItem* BoostItem;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AAmmo> AmmoClass;
	
	UPROPERTY()
	class AAmmo* Ammo;

};