// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/EnumTypes/BoostType.h"
#include "SpawnBoostItem.generated.h"

UCLASS()
class SHOOTER_API ASpawnBoostItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnBoostItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	void InitializeMaps();
	void SpawnItems();
	void SetSpawnLocations();
	void ShuffleArray(TArray<int32>& myArray);

private:
	UPROPERTY(VisibleAnywhere)
	TMap<int32, EBoostType> BoostIndexMap;

	UPROPERTY()
	TArray<int32> BoostProbabliltyArray;
	UPROPERTY()
	TArray<FVector> RandomLocationSpawn;

public:	
	// Called every frame

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABoostItem> BoostClass;
	
	UPROPERTY()
	class ABoostItem* BoostItem;

};
