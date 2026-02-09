// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomDebris.generated.h"

USTRUCT(BlueprintType)
struct FFireLocations
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FVector> FireLocationArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> FireIndicies;
};

UCLASS()
class SHOOTER_API ARandomDebris : public AActor
{
	GENERATED_BODY()
	
public:	
	ARandomDebris();

	void StartSpawnDebrisProcess(FVector2D ScaleArea, float RotationValue);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnBox", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MainSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnBox", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* SpawnBox;

	void SpawnDebris();

	UPROPERTY(EditAnywhere, Category = "SpawnBox");
	TSubclassOf<class ADebris> DebrisClass;

	int32 NumToSpawn = 0;

	UPROPERTY(VisibleAnywhere, Category = "SpawnBox")
	class UParticleSystemComponent* FireParticleComponent = nullptr;
	UPROPERTY(EditAnywhere, Category = "SpawnBox")
	TArray<UParticleSystem*> FireParticleArray;

	UPROPERTY(EditAnywhere, Category = "SpawnBox")
	USoundBase* FireSound;

	//TArray<FVector> StoreFireLocations;
	//UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_FireLocationArray, Category = "SpawnBox")
	//TArray<FVector> FireLocationArray;

	//UPROPERTY(VisibleAnywhere, Replicated, Category = "SpawnBox")
	//TArray<int32> FireIndicies;

	//UFUNCTION()
	//void OnRep_FireLocationArray();

	int8 NumFireToSpawn = 2;

	UPROPERTY(VisibleAnywhere)
	FFireLocations ServerFireLocations;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_FireLocations, Category = "SpawnBox")
	FFireLocations FireLocations;

	UFUNCTION()
	void OnRep_FireLocations();

};
