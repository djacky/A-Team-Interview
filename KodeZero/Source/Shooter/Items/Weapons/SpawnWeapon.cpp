// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnWeapon.h"
#include "Weapon.h"
#include "Item.h"
#include "BoostItem.h"
#include "Ammo.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Engine/TargetPoint.h"


// Sets default values
ASpawnWeapon::ASpawnWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ASpawnWeapon::BeginPlay()
{
	Super::BeginPlay();
    if (HasAuthority())
    {
        // SpawnWeapons();
    }
}

void ASpawnWeapon::CalcProbabilityArrays(float RarityFactor, int32 ArrayNum, TArray<int32>& ProbabilityArray)
{
    float Exp = 2.71828f, ExpSum = 0, ExpFactor = 0, Ke = 0;
    TArray<float> ExpFactorArray;
    for (int32 i = 0; i < ArrayNum; i++)
    {
        ExpSum += FMath::Pow(Exp, -ExpFactor);
        ExpFactorArray.Add(-ExpFactor);
        ExpFactor += RarityFactor; // Main factor used to determine rarity
    }
    Ke = 1 / ExpSum;

    TArray<int32> TempArray;
    // Create array that determines how probable it is to spawn a good/bad weapon.
    // i = 0 is worst weapon
    for (int32 i = 0; i < ArrayNum; i++)
    {
        TempArray.Init(i, FGenericPlatformMath::RoundToInt32(1000 * Ke * FMath::Pow(Exp, ExpFactorArray[i])));
        ProbabilityArray += TempArray;
    }
    ShuffleArray(ProbabilityArray);
}

void ASpawnWeapon::InitializeMaps()
{

    // Set rarity factors (higher number = more rare)
    float WeaponRarityFactor = 0.15, DamageRarityFactor = 0.4, BoostRarityFactor = 0.05, AmmoRarityFactor = 0.05;

    WeaponIndexArray = {EWeaponType::EWT_Pistol, EWeaponType::EWT_AR, EWeaponType::EWT_SubmachineGun,
                    EWeaponType::EWT_Shotgun, EWeaponType::EWT_Sniper, EWeaponType::EWT_GrenadeLauncher,
                    EWeaponType::EWT_GravCannon};

    //WeaponIndexArray = {EWeaponType::EWT_SubmachineGun};

    RarityIndexArray = {EItemRarity::EIR_Damaged, EItemRarity::EIR_Common, EItemRarity::EIR_Uncommon,
                        EItemRarity::EIR_Rare, EItemRarity::EIR_Legendary};

    // Initialize the boost items
    BoostIndexArray = {EBoostType::EBT_ShieldV2, EBoostType::EBT_HealthV2, EBoostType::EBT_ShieldV1,
                    EBoostType::EBT_HealthV1, EBoostType::EBT_Grapple, EBoostType::EBT_SuperPunch, EBoostType::EBT_SuperJump, EBoostType::EBT_Teleport,
                    EBoostType::EBT_Fly, EBoostType::EBT_Copy, EBoostType::EBT_Slow, EBoostType::EBT_Protect, EBoostType::EBT_Ghost};

	// Initialize the ammo
    AmmoIndexArray = {EAmmoType::EAT_9mm, EAmmoType::EAT_45mm, EAmmoType::EAT_AR,
                    EAmmoType::EAT_Shells, EAmmoType::EAT_GrenadeRounds, EAmmoType::EAT_HelicopterMissiles,
                    EAmmoType::EAT_GravCharges};

    CalcProbabilityArrays(WeaponRarityFactor, WeaponIndexArray.Num(), WeaponProbabliltyArray);
    CalcProbabilityArrays(DamageRarityFactor, RarityIndexArray.Num(), RarityProbabliltyArray);
    CalcProbabilityArrays(BoostRarityFactor, BoostIndexArray.Num(), BoostProbabliltyArray);
    CalcProbabilityArrays(AmmoRarityFactor, AmmoIndexArray.Num(), AmmoProbabliltyArray);
}

void ASpawnWeapon::ShuffleArray(TArray<int32>& myArray)
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

// Called every frame
void ASpawnWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASpawnWeapon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
}

void ASpawnWeapon::SpawnWeapons()
{
    if (ItemTargetPointClass == nullptr) return;
    InitializeMaps();
    
    // Now TargetPoints are self-reported, this should help with WorldPartition
    // TArray<AActor*> ItemTargetPoints;
    // UGameplayStatics::GetAllActorsOfClass(GetWorld(), ItemTargetPointClass, ItemTargetPoints);
    UE_LOG(LogTemp, Warning, TEXT("Item Target Number = %i"), ItemTargetPoints.Num());

    int32 NumWeapToSpawn = 0, NumBoostToSpawn = 50, NumAmmoToSpawn = 100;
    //int32 NumWeapToSpawn = 1, NumBoostToSpawn = 1, NumAmmoToSpawn = 1;
    if (NumWeapToSpawn + NumBoostToSpawn + NumAmmoToSpawn > ItemTargetPoints.Num())
    {
        int32 NumToSpawn = FMath::FloorToInt32((ItemTargetPoints.Num() - 1) / 3.f);
        NumWeapToSpawn = NumToSpawn;
        NumBoostToSpawn = NumToSpawn;
        NumAmmoToSpawn = NumToSpawn;
    }
    //checkf(NumWeapToSpawn + NumBoostToSpawn + NumAmmoToSpawn < ItemTargetPoints.Num(), TEXT("Number of total items to spawn > Number of item target points in map. Decrease the number of items to spawn in the map."))

    UWorld* World = GetWorld();
    if (World)
    {
        if (WeaponClass && NumWeapToSpawn > 0)
        {
            FRotator SpawnRotation{0.f, 0.f, 0.f};
            for (int32 i = 0; i < NumWeapToSpawn; i++)
            {
                int32 RandomWeaponIndex = FMath::RandRange(0, WeaponProbabliltyArray.Num() - 1);
                int32 RandomRarityIndex = FMath::RandRange(0, RarityProbabliltyArray.Num() - 1);
                int32 RandomLocationIndex = FMath::RandRange(0, ItemTargetPoints.Num() - 1);
                SpawnRotation.Yaw = FMath::RandRange(0, 360);

                Weapon = World->SpawnActor<AWeapon>(WeaponClass, ItemTargetPoints[RandomLocationIndex]->GetActorLocation(), SpawnRotation);
                Weapon->SetWeaponType(WeaponIndexArray[WeaponProbabliltyArray[RandomWeaponIndex]]);
                Weapon->UpdateWeapon();
                Weapon->SetItemState(EItemState::EIS_Pickup);

                auto Item = Cast<AItem>(Weapon);
                if (Item)
                {
                    Item->SetItemRarity(RarityIndexArray[RarityProbabliltyArray[RandomRarityIndex]]);
                    Item->UpdateItem();
                }
                //ItemTargetPoints[RandomLocationIndex]->Destroy();
                ItemTargetPoints.RemoveAt(RandomLocationIndex);
            }
        }

        if (BoostClass && NumBoostToSpawn > 0)
        {
            FRotator BoostSpawnRotation{0.f, 0.f, 0.f};
            for (int32 i = 0; i < NumBoostToSpawn; i++)
            {
                int32 RandomBoostIndex = FMath::RandRange(0, BoostProbabliltyArray.Num() - 1);
                int32 RandomLocationIndex = FMath::RandRange(0, ItemTargetPoints.Num() - 1);
                BoostSpawnRotation.Yaw = FMath::RandRange(0, 360);

                BoostItem = World->SpawnActor<ABoostItem>(BoostClass, ItemTargetPoints[RandomLocationIndex]->GetActorLocation(), BoostSpawnRotation);
                BoostItem->SetBoostType(BoostIndexArray[BoostProbabliltyArray[RandomBoostIndex]]);
                BoostItem->UpdateBoost();
                BoostItem->SetItemState(EItemState::EIS_Pickup);

                //ItemTargetPoints[RandomLocationIndex]->Destroy();
                ItemTargetPoints.RemoveAt(RandomLocationIndex);
            }
        }

        if (AmmoClass && NumAmmoToSpawn > 0)
        {
            FRotator AmmoSpawnRotation{0.f, 0.f, 0.f};
            for (int32 i = 0; i < NumAmmoToSpawn; i++)
            {
                int32 RandomAmmoIndex = FMath::RandRange(0, AmmoProbabliltyArray.Num() - 1);
                int32 RandomLocationIndex = FMath::RandRange(0, ItemTargetPoints.Num() - 1);
                AmmoSpawnRotation.Yaw = FMath::RandRange(0, 360);

                Ammo = World->SpawnActor<AAmmo>(AmmoClass, ItemTargetPoints[RandomLocationIndex]->GetActorLocation(), AmmoSpawnRotation);
                Ammo->SetAmmoType(AmmoIndexArray[AmmoProbabliltyArray[RandomAmmoIndex]]);
                Ammo->UpdateAmmo();
                Ammo->SetAmmoState(EItemState::EIS_Pickup);

                //ItemTargetPoints[RandomLocationIndex]->Destroy();
                ItemTargetPoints.RemoveAt(RandomLocationIndex);
            }
        }

        /*
        if (ItemTargetPoints.Num() > 0)
        {
            for (int32 i = 0; i < ItemTargetPoints.Num(); i++)
            {
                ItemTargetPoints[i]->Destroy();
                ItemTargetPoints.RemoveAt(i);
            }
        }
        */
    }
}

// Replace this function with another function when actual game ends
// Should remove all weapon actors when a game is finished (because OnConstruction will spawn new weapons on a new game)
void ASpawnWeapon::PreExit()
{
    /*
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), WeaponClass, FoundActors);
	for (int32 i = 0; i < FoundActors.Num(); i++)
	{
		FoundActors[i]->Destroy();
	}
    */
}