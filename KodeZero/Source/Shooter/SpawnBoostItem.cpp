// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnBoostItem.h"
#include "BoostItem.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASpawnBoostItem::ASpawnBoostItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ASpawnBoostItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASpawnBoostItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpawnBoostItem::InitializeMaps()
{
    BoostIndexMap.Add(0, EBoostType::EBT_ShieldV2);
    BoostIndexMap.Add(1, EBoostType::EBT_HealthV2);
    BoostIndexMap.Add(2, EBoostType::EBT_ShieldV1);
    BoostIndexMap.Add(3, EBoostType::EBT_HealthV1);
    BoostIndexMap.Add(4, EBoostType::EBT_SuperJump);
    BoostIndexMap.Add(5, EBoostType::EBT_Teleport);
	BoostIndexMap.Add(6, EBoostType::EBT_Slow);
	BoostIndexMap.Add(7, EBoostType::EBT_Protect);
	BoostIndexMap.Add(8, EBoostType::EBT_Ghost);

    float Exp = 2.71828f, ExpSum = 0, ExpFactor = 0, Ke = 0;
    TArray<float> ExpFactorArray;
    for (int32 i = 0; i < BoostIndexMap.Num(); i++)
    {
        ExpSum += FMath::Pow(Exp, -ExpFactor);
        ExpFactorArray.Add(-ExpFactor);
        ExpFactor += 0.05; // Main factor used to determine rarity
    }
    Ke = 1 / ExpSum;

    TArray<int32> TempArray;
    // Create array that determines how probable it is to spawn a good/bad boost.
    // i = 0 is worst boost
    for (int32 i = 0; i < BoostIndexMap.Num(); i++)
    {
        TempArray.Init(i, FGenericPlatformMath::RoundToInt32(1000 * Ke * FMath::Pow(Exp, ExpFactorArray[i])));
        BoostProbabliltyArray += TempArray;
    }
    ShuffleArray(BoostProbabliltyArray);
}

void ASpawnBoostItem::ShuffleArray(TArray<int32>& myArray)
{
    if (myArray.Num() > 0)
    {
        int32 LastIndex = myArray.Num() - 1;
        for (int32 i = 0; i <= LastIndex; ++i)
        {
            int32 Index = FMath::RandRange(i, LastIndex);
            if (i != Index)
            {
                myArray.Swap(i, Index);
            }
        }
    }
}

void ASpawnBoostItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
	SpawnItems();
}

void ASpawnBoostItem::SetSpawnLocations()
{
    RandomLocationSpawn.Add({-100.f, 600.f, 100.f});
    RandomLocationSpawn.Add({-250.f, 600.f, 100.f});
    RandomLocationSpawn.Add({-400.f, 600.f, 100.f});
    RandomLocationSpawn.Add({-550.f, 600.f, 100.f});
    RandomLocationSpawn.Add({-700.f, 600.f, 100.f});
}

void ASpawnBoostItem::SpawnItems()
{
    InitializeMaps();
    SetSpawnLocations();

    FRotator SpawnRotation{0.f, 0.f, 0.f};
    int32 NumBoostToSpawn = 2; //Number of Boost Items to spawn in the map

    for (int32 i = 0; i < NumBoostToSpawn; i++)
    {
        int32 RandomBoostIndex = FMath::RandRange(0, BoostProbabliltyArray.Num() - 1);
        int32 RandomLocationIndex = FMath::RandRange(0, RandomLocationSpawn.Num() - 1);
        float RandomSpawnYaw = FMath::RandRange(0, 360);
        SpawnRotation.Yaw = RandomSpawnYaw;

        BoostItem = GetWorld()->SpawnActor<ABoostItem>(BoostClass, RandomLocationSpawn[RandomLocationIndex], SpawnRotation);
        BoostItem->SetBoostType(BoostIndexMap[BoostProbabliltyArray[RandomBoostIndex]]);
        BoostItem->UpdateBoost();

        RandomLocationSpawn.RemoveAt(RandomLocationIndex);
    }
}


