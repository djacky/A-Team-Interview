// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "Shooter/MissleSystem.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "ShooterPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Shooter/Drone/ItemContainer.h"
#include "Shooter/Drone/ContainerSplinePath.h"
#include "Components/SplineComponent.h"
//#include "GameFramework/GameplayMessageSubsystem.h"
#include "EngineUtils.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "GameMode/ShooterGameMode.h"
#include "ShooterCharacter.h"
#include "GameFramework/GameSession.h"
#include "Shooter/Drone/GroundContainer.h"
#include "Engine/TargetPoint.h"
#include "Helicopter.h"
#include "Shooter/Items/RandomDebris.h"
#include "Json.h"
#include "Shooter/AI/ShooterAI.h"
#include "Shooter/AI/BaseAI/AICharacterBase.h"
#include "Shooter/AI/ShooterAIController.h"
#include "Shooter/Misc/Requests/RequestsObject.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Item.h"
#include "BoostItem.h"
#include "Ammo.h"
#include "Shooter/Misc/Interfaces/GeneralInterface.h"
#include "Shooter/Misc/Interfaces/EnemyAIInterface.h"
#include "ShooterSpectatorPawn.h"
#include "Globals.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Misc/SecureHash.h"

extern ENGINE_API float GAverageFPS;

AShooterGameState::AShooterGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ServerFPS = 0.0f;

    //MissileTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MissileTimelineComponent"));
}

void AShooterGameState::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority())
    {        
        ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
        auto ShooterGM = GetWorld()->GetAuthGameMode<AShooterGameMode>();
        if (ShooterGM)
        {
            GameMaxPlayers = ShooterGM->MaxLevelPlayers;
            //bIsTeamMode = ShooterGM->bTeamMode;
        }
        if (IsPracticeMode())
        {
            AIPatrolPointsFixed = AIPatrolPointsFixedPractice;
            //UE_LOG(LogTemp, Warning, TEXT("GameState Practice Mode Enabled!"));
        }
        else
        {
            if (ShooterGI && ShooterGI->GameType != EGameModeType::EGMT_Lobby)
            {
                InitializeMissileHitLocations();
                InitializeTeleportLocations();
                #if !WITH_EDITOR
                    if (ShooterGM)
                    {
                        if (ShooterGM->GetNumMissilesToPrespawn() == 0)
                        {
                            ShooterGM->ActivateAWSSession();
                        }
                        else
                        {
                            MissleSpawnDelay = ((ShooterGM->MatchTime - 10.f) / ShooterGM->GetNumMissilesToPrespawn()) - FireMissileTime;
                            ShortenMapWithMissiles();
                        }
                        MultiplierEventTriggerTime = FMath::RandRange(0.25f * ShooterGM->MatchTime, 0.75f * ShooterGM->MatchTime);
                    }
                #endif
                SpawnDebris();
            }
        }
        AIPatrolPoints = AIPatrolPointsFixed;

        if (ShooterGI)
        {
            if (!ShooterGI->TargetGameSessionId.IsEmpty()) PartyId = FMD5::HashAnsiString(*ShooterGI->TargetGameSessionId);
            Handicap = ShooterGI->TournamentParams.handicap;
        }
        //if (ShooterGI && !IsPracticeMode()) StartServerFuncs(ShooterGI->bTeamMode);
        InitializeRandomAssetMaps();
        /*
        #if !WITH_EDITOR
            FTimerHandle CheckGameStateTimer;
            GetWorldTimerManager().SetTimer(CheckGameStateTimer, this, &AShooterGameState::GameStateExpiration, 30.f);
        #endif
        */
    }
}

bool AShooterGameState::IsPracticeMode()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI)
    {
        return ShooterGI->bPracticeMode;
    }
    return false;
}

void AShooterGameState::StartServerFuncs(bool bInTeamMode)
{
    if (!HasAuthority()) return;
    bIsTeamMode = bInTeamMode;
    GetContainerSplineActor();
    //SpawnGroundContainers(); //spawning them from the blueprint (due to level streaming)
    //SpawnHelicopters(); // spawning them from the blueprint (due to level streaming)

    MissileStart();
    DroneStart();

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI && ShooterGI->GameType != EGameModeType::EGMT_tournament && ShooterGI->GameType != EGameModeType::EGMT_tournamentTeam)
    {
        #if !WITH_EDITOR
            if (TargetIndex == 0)
            {
                FTimerHandle SpawnAITimer;
                GetWorldTimerManager().SetTimer(SpawnAITimer, this, &AShooterGameState::StartAISpawn, 70.f);
            }
        #endif
    }
    //if (TargetIndex == 0) StartAISpawn();
}

void AShooterGameState::SpawnDebris()
{
    FLevelDebris DebrisParams;
    /*
    // For TestMap
    DebrisParams.DebrisSpawnLocations = {
            {370.000000f,-3950.000000f,30.000000f}
        };
    DebrisParams.ScaleArray = {
            {35.f, 65.f}
        };

    DebrisParams.RotationArray = {
            0.f;
        };
    */

    DebrisParams.DebrisSpawnLocations = {
            {60870.0f,27830.0f,110.000000f},
            {4020.000000f,29070.000000f,110.000000f},
            {-57720.000000f,35990.000000f,100.000000f},
            {-16360.000000f,-22450.000000f,100.000000f},
            {-57750.000000f,-21890.000000f,110.000000f},
            {3870.000000f,25460.000000f,110.000000f},
            {-11500.000000f,-2350.000000f,110.000000f},
            {-47860.000000f,-17220.000000f,110.000000f},
            {16440.000000f,-19590.000000f,100.000000f},
            {16840.000000f,4710.000000f,110.000000f},
            {31460.000000f,10900.000000f,120.000000f},
            {-20580.000000f,17790.000000f,110.000000f},
            {-2560.000000f,21280.000000f,110.000000f},
            {-38610.000000f,38870.000000f,90.000000f}
        };
    DebrisParams.ScaleArray = {
            {35.f, 65.f},
            {45.f, 70.f},
            {55.f, 50.f},
            {60.f, 50.f},
            {65.f, 50.f},
            {65.f, 55.f},
            {80.f, 50.f},
            {35.f, 50.f},
            {70.f, 65.f},
            {60.f, 75.f},
            {55.f, 60.f},
            {50.f, 75.f},
            {70.f, 45.f},
            {85.f, 40.f}
        };

    DebrisParams.RotationArray = {
            0.f,
            0.f,
            0.f,
            20.f,
            -10.f,
            0.f,
            10.f,
            0.f,
            45.f,
            -7.f,
            0.f,
            0.f,
            0.f,
            -15.f
        };

        FStreamableManager& Streamable = UAssetManager::GetStreamableManager(); // Or create your own
        FSoftObjectPath DebrisClassPath = RandomDebrisClass.ToSoftObjectPath();

        if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
        {
            Streamable.RequestAsyncLoad(DebrisClassPath, FStreamableDelegate::CreateLambda([this, DebrisParams]()
            {
                if (UClass* LoadedClass = Cast<UClass>(RandomDebrisClass.Get()))
                {
                    for (int32 i = 0; i < DebrisParams.DebrisSpawnLocations.Num(); i++)
                    {
                        ARandomDebris* DebrisActor = GetWorld()->SpawnActor<ARandomDebris>(
                            LoadedClass,
                            DebrisParams.DebrisSpawnLocations[i],
                            FRotator(0.f, 0.f, 0.f)
                        );

                        if (DebrisActor)
                        {
                            DebrisActor->StartSpawnDebrisProcess(DebrisParams.ScaleArray[i], DebrisParams.RotationArray[i]);
                        }
                    }
                }
            }));
        }
    //bSpawnedDebris = true;
}

void AShooterGameState::SpawnGroundedItems()
{
    if (MapItemTargetPoints.Num() > 0)
    {
        ShuffleArray(MapItemTargetPoints);
        int32 NumWeapToSpawn = 0, NumBoostToSpawn = 50, NumAmmoToSpawn = 150;
        //int32 NumWeapToSpawn = 1, NumBoostToSpawn = 1, NumAmmoToSpawn = 1;
        if (NumWeapToSpawn + NumBoostToSpawn + NumAmmoToSpawn > MapItemTargetPoints.Num())
        {
            int32 NumToSpawn = FMath::FloorToInt32((MapItemTargetPoints.Num() - 1) / 3.f);
            NumWeapToSpawn = NumToSpawn;
            NumBoostToSpawn = NumToSpawn;
            NumAmmoToSpawn = NumToSpawn;
        }
        UWorld* World = GetWorld();
        if (World)
        {
            if (WeaponClass && NumWeapToSpawn > 0)
            {
                FRotator SpawnRotation{0.f, 0.f, 0.f};
                for (int32 i = 0; i < NumWeapToSpawn; i++)
                {
                    int32 RandomLocationIndex = FMath::RandRange(0, MapItemTargetPoints.Num() - 1);
                    SpawnRotation.Yaw = FMath::RandRange(0, 360);

                    AWeapon* Weapon = World->SpawnActor<AWeapon>(WeaponClass, MapItemTargetPoints[RandomLocationIndex]->GetActorLocation(), SpawnRotation);
                    if (Weapon)
                    {
                        Weapon->SetWeaponType(GetRandomWeaponType());
                        Weapon->UpdateWeapon();
                        Weapon->SetItemState(EItemState::EIS_Pickup);

                        AItem* Item = Cast<AItem>(Weapon);
                        if (Item)
                        {
                            Item->SetItemRarity(GetRandomRarityType());
                            Item->UpdateItem();
                        }
                    }
                    //ItemTargetPoints[RandomLocationIndex]->Destroy();
                    MapItemTargetPoints.RemoveAt(RandomLocationIndex);
                }
            }

            if (BoostClass && NumBoostToSpawn > 0)
            {
                FRotator BoostSpawnRotation{0.f, 0.f, 0.f};
                for (int32 i = 0; i < NumBoostToSpawn; i++)
                {
                    int32 RandomLocationIndex = FMath::RandRange(0, MapItemTargetPoints.Num() - 1);
                    BoostSpawnRotation.Yaw = FMath::RandRange(0, 360);

                    ABoostItem* BoostItem = World->SpawnActor<ABoostItem>(BoostClass, MapItemTargetPoints[RandomLocationIndex]->GetActorLocation(), BoostSpawnRotation);
                    if (BoostItem)
                    {
                        BoostItem->SetBoostType(GetRandomBoostType());
                        BoostItem->UpdateBoost();
                        BoostItem->SetItemState(EItemState::EIS_Pickup);
                    }
                    //ItemTargetPoints[RandomLocationIndex]->Destroy();
                    MapItemTargetPoints.RemoveAt(RandomLocationIndex);
                }
            }

            if (AmmoClass && NumAmmoToSpawn > 0)
            {
                FRotator AmmoSpawnRotation{0.f, 0.f, 0.f};
                for (int32 i = 0; i < NumAmmoToSpawn; i++)
                {
                    int32 RandomLocationIndex = FMath::RandRange(0, MapItemTargetPoints.Num() - 1);
                    AmmoSpawnRotation.Yaw = FMath::RandRange(0, 360);

                    AAmmo* Ammo = World->SpawnActor<AAmmo>(AmmoClass, MapItemTargetPoints[RandomLocationIndex]->GetActorLocation(), AmmoSpawnRotation);
                    if (Ammo)
                    {
                        Ammo->SetAmmoType(GetRandomAmmoType());
                        Ammo->UpdateAmmo();
                        Ammo->SetAmmoState(EItemState::EIS_Pickup);
                    }
                    //ItemTargetPoints[RandomLocationIndex]->Destroy();
                    MapItemTargetPoints.RemoveAt(RandomLocationIndex);
                }
            }
        }
    }
}

void AShooterGameState::SpawnItemsAfterAIKill(FTransform AITransform)
{
    int32 NumWeapToSpawn = 1, NumBoostToSpawn = 0, NumAmmoToSpawn = 2;
    UWorld* World = GetWorld();
    FVector SpawnLoc = AITransform.GetLocation();
    if (World)
    {
        if (WeaponClass && NumWeapToSpawn > 0)
        {
            FRotator SpawnRotation{0.f, 0.f, 0.f};
            for (int i = 0; i < NumWeapToSpawn; i++)
            {
                SpawnRotation.Yaw = FMath::RandRange(0, 360);
                AWeapon* Weapon = World->SpawnActor<AWeapon>(WeaponClass, SpawnLoc, SpawnRotation);
                if (Weapon)
                {
                    Weapon->SetWeaponType(GetRandomWeaponType());
                    Weapon->UpdateWeapon();
                    AItem* Item = Cast<AItem>(Weapon);
                    if (Item)
                    {
                        Item->SetItemRarity(GetRandomRarityType());
                        Item->UpdateItem();
                    }
                    Weapon->OnDropItem(nullptr, AITransform, true);
                }
            }
        }

        if (BoostClass && NumBoostToSpawn > 0)
        {
            FRotator BoostSpawnRotation{0.f, 0.f, 0.f};
            for (int i = 0; i < NumBoostToSpawn; i++)
            {
                BoostSpawnRotation.Yaw = FMath::RandRange(0, 360);
                ABoostItem* BoostItem = World->SpawnActor<ABoostItem>(BoostClass, SpawnLoc, BoostSpawnRotation);
                if (BoostItem)
                {
                    BoostItem->SetBoostType(GetRandomBoostType());
                    BoostItem->UpdateBoost();
                    BoostItem->OnDropItem(nullptr, AITransform, true);
                }
            }
        }

        if (AmmoClass && NumAmmoToSpawn > 0)
        {
            FRotator AmmoSpawnRotation{0.f, 0.f, 0.f};
            for (int i = 0; i < NumAmmoToSpawn; i++)
            {
                AmmoSpawnRotation.Yaw = FMath::RandRange(0, 360);
                AAmmo* Ammo = World->SpawnActor<AAmmo>(AmmoClass, SpawnLoc, AmmoSpawnRotation);
                if (Ammo)
                {
                    Ammo->SetAmmoType(GetRandomAmmoType());
                    Ammo->UpdateAmmo();
                    Ammo->OnDropItem(nullptr, AITransform, true);
                }
            }
        }
    }
}

void AShooterGameState::SpawnGroundContainers()
{
    if (GContainerTargetPointClass)
    {
        //UGameplayStatics::GetAllActorsOfClass(GetWorld(), GContainerTargetPointClass, GContainerTargetPoints); 
        ShuffleArray(GContainerTargetPoints);
        
        int32 NumContainerToSpawn = 500;
        if (NumContainerToSpawn > GContainerTargetPoints.Num()) NumContainerToSpawn = GContainerTargetPoints.Num() - 1;
        if (NumContainerToSpawn > 0 && GContainerClass)
        {
            AGroundContainer* GContainer = nullptr;
            for (int32 i = 0; i < NumContainerToSpawn; i++)
            {
                if (i == NumContainerToSpawn - 1)
                {
                    GContainer = GetWorld()->SpawnActor<AGroundContainer>(GContainerClass, GContainerTargetPoints[i]->GetTransform());
                }
                else
                {
                    GetWorld()->SpawnActor<AGroundContainer>(GContainerClass, GContainerTargetPoints[i]->GetTransform());
                }
            }
            // Destroy 1 container to load assets into memory, and prevent FPS drop when opening first container
            if (GContainer)
            {
                UGameplayStatics::ApplyDamage(
                    GContainer,
                    100,
                    nullptr,
                    this,
                    UDamageType::StaticClass()
                );
            }
        }
        //StartCheckContainersTimer();
    }
}

void AShooterGameState::StartCheckContainersTimer()
{
    FTimerHandle CheckContainerTimer;
    GetWorldTimerManager().SetTimer(CheckContainerTimer, this, &AShooterGameState::CheckContainersAndSpawn, 10.f);
}

void AShooterGameState::CheckContainersAndSpawn()
{
    if (GContainerClass && GContainerTargetPoints.Num() > 0)
    {
        for (auto TargetPoint : GContainerTargetPoints)
        {
            const FVector Start{ TargetPoint->GetActorLocation()  + FVector(0.f, 0.f, 500.f)};
            const FVector End{ TargetPoint->GetActorLocation() };
            FHitResult OutHitResult;
            GetWorld()->LineTraceSingleByChannel(
                OutHitResult,
                Start,
                End,
                ECollisionChannel::ECC_Visibility);

            if (OutHitResult.bBlockingHit && Cast<AGroundContainer>(OutHitResult.GetActor()))
            {
                // Do nothing because a container already exists there.
            }
            else
            {
                GetWorld()->SpawnActor<AGroundContainer>(GContainerClass, TargetPoint->GetTransform());
                StartCheckContainersTimer();
                return;
            }
        }
        StartCheckContainersTimer();
    }
}

void AShooterGameState::CheckPracticeContainers()
{
    if (IsPracticeMode() && GetWorld() && GContainerClass)
    {
        TArray<AActor*> GroundContainers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), GContainerClass, GroundContainers);
        if (GroundContainers.Num() == 0)
        {
            SpawnGroundContainers();
        }
    }
}

void AShooterGameState::SpawnHelicopters()
{
    if (HelicopterTargetPointClass)
    {
        //UGameplayStatics::GetAllActorsOfClass(GetWorld(), HelicopterTargetPointClass, HelicopterTargetPoints); 
        ShuffleArray(HelicopterTargetPoints);

        int32 NumHelicopterToSpawn = 7;
        if (NumHelicopterToSpawn > HelicopterTargetPoints.Num()) NumHelicopterToSpawn = HelicopterTargetPoints.Num();
        if (NumHelicopterToSpawn > 0 && GetWorld() && HelicopterClass)
        {
            for (int32 i = 0; i < NumHelicopterToSpawn; i++)
            {
                if (HelicopterTargetPoints[i])
                {
                    AHelicopter* SpawnedHelicopter = GetWorld()->SpawnActor<AHelicopter>(HelicopterClass, HelicopterTargetPoints[i]->GetTransform());
                    SpawnedHelicopter->SetOwner(nullptr);
                }
            }
        }
    }
}

EEnemyType AShooterGameState::GetRandomEnemyType()
{
    int32 RandomIndex = FMath::RandRange(0, static_cast<int32>(EEnemyType::EET_MAX) - 1);
    return static_cast<EEnemyType>(RandomIndex);
}

void AShooterGameState::SpawnAIShooters(AShooterPlayerController* InController)
{    
    UWorld* World = GetWorld();
    if (World && ShooterAITargetPointClass)
    {
        if (IsPracticeMode())
        {
            if (AIShooterArray.Num() >= 10) return;
        }

        if (ShooterAIClass && EnemyAIClass && ShooterAIControllerClass && ShooterAITargetPoints.Num() > 0 && ShooterAITargetPoints.IsValidIndex(TargetIndex) && ShooterAITargetPoints[TargetIndex])
        {
            auto AIController = World->SpawnActor<AShooterAIController>(ShooterAIControllerClass, ShooterAITargetPoints[TargetIndex]->GetTransform());
            ACharacter* AICharacter = nullptr;
            
            EEnemyType AIType = GetRandomEnemyType();
            //EEnemyType AIType = EEnemyType::EET_Howitzer;
            if (!AICharacterMap[AIType]) return;

            AICharacter = World->SpawnActor<ACharacter>(AICharacterMap[AIType], ShooterAITargetPoints[TargetIndex]->GetTransform());
            
            //TargetIndex = FMath::RandRange(0, ShooterAITargetPoints.Num() - 1);
            //auto AIController = Cast<AShooterAIController>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ShooterAIControllerClass, ShooterAITargetPoints[i]->GetTransform()));

            if (AIController && AICharacter)
            {
                if (auto ShooterPS = Cast<AShooterPlayerState>(AIController->PlayerState)) ShooterPS->SetAIProperties(AIType);
                AIController->Possess(AICharacter);
                AIShooterArray.Add(AICharacter);
                if (IsPracticeMode() && InController)
                {
                    TargetIndex = FMath::RandRange(0, ShooterAITargetPoints.Num() - 1);
                    InController->UpdateAICount(AIShooterArray.Num());
                }
                else
                {
                    ++TargetIndex;
                    if (TargetIndex >= NumAIToSpawn) return;
                    FTimerDelegate SpawnAIDel;
                    FTimerHandle SpawnAITimer;
                    SpawnAIDel.BindUFunction(this, FName("SpawnAIShooters"), nullptr);
                    World->GetTimerManager().SetTimer(SpawnAITimer, SpawnAIDel, 1.f, false);
                }
            }
        }
    }
}

void AShooterGameState::StartAISpawn()
{
    if (NumAIToSpawn > ShooterAITargetPoints.Num()) NumAIToSpawn = ShooterAITargetPoints.Num() - 1;
    SpawnAIShooters(nullptr);
}

void AShooterGameState::SpawnNewAI(AController* InController, const EEnemyType& AIType)
{
    bool bSpawnCond = ShooterControllerArray.Num() + AIShooterArray.Num() < 25;

    //if (MissileStartLocation.Num() <= 3) return;
    UWorld* World = GetWorld();
    if (World && InController && ShooterAITargetPointClass && ShooterAITargetPoints.Num() > 0 && bSpawnCond && AICharacterMap.Contains(AIType))
    {
        ShuffleArray(ShooterAITargetPoints);
        //auto AIController = GetWorld()->SpawnActor<AShooterAIController>(ShooterAIControllerClass, ShooterAITargetPoints[0]->GetTransform());
        if (!ShooterAITargetPoints[0] || !AICharacterMap[AIType]) return;
        ACharacter* AICharacter = World->SpawnActor<ACharacter>(AICharacterMap[AIType], ShooterAITargetPoints[0]->GetTransform());
        if (AICharacter)
        {
            InController->Possess(AICharacter);
            AIShooterArray.Add(AICharacter);
            /*
            if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(AICharacter))
            {
                EnemyInterface->Execute_SetTarget(AICharacter, nullptr, false);
            }
            */
        }
    } 
}

void AShooterGameState::StopAISearch()
{
    if (AIShooterArray.Num() > 0)
    {
        for (auto AIShooter : AIShooterArray)
        {
            if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(AIShooter))
            {
                EnemyInterface->Execute_StopSearch(AIShooter);
            }
        }
    }
}

void AShooterGameState::DestroyAIs()
{
	if (AIShooterArray.Num() > 0)
	{
		if (AIShooterArray[0] == nullptr) return;
        if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(AIShooterArray[0]))
        {
            EnemyInterface->Execute_OnElimmed(AIShooterArray[0]);
        }
		FTimerHandle ElimAITimer;
    	GetWorldTimerManager().SetTimer(ElimAITimer, this, &AShooterGameState::DestroyAIs, 0.1f);
	}
}

void AShooterGameState::CheckWinnerAfterAIKillTimer()
{
    if (IsPracticeMode()) return;
    FTimerHandle AIKillTimer;
    GetWorldTimerManager().SetTimer(AIKillTimer, this, &AShooterGameState::CheckWinnerAfterAIKill, 0.5f);
}

void AShooterGameState::CheckWinnerAfterAIKill()
{
    if (AIShooterArray.Num() == 0)
    {
        PlayersStillAlive = GetPlayersStillAlive();
        //CheckWinners();
    }
}

void AShooterGameState::ShuffleArray(TArray<AActor*>& myArray)
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

void AShooterGameState::ShuffleIntArray(TArray<int32>& myArray)
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

bool AShooterGameState::PracticeModeEndMatch()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI && ShooterGI->bPracticeMode && !ShooterGI->GameAccessCheck.bIsFullVersion && 
        ShooterControllerArray.IsValidIndex(0) && ShooterControllerArray[0] &&
        ShooterPlayerArray.IsValidIndex(0) && ShooterPlayerArray[0])
    {
        ShooterControllerArray[0]->bMatchEnded = true;
        ShooterControllerArray[0]->DestroyReturnToMainMenu();
        if (auto ShooterChar = Cast<AShooterCharacter>(ShooterControllerArray[0]->GetPawn()))
        {
            if (!ShooterChar->GetPlayerEliminated())
            {
                ShooterChar->StopMatch();
            }
        }
        ShooterControllerArray[0]->ShowReturnToMainMenu(false, ShooterPlayerArray[0]->PlayerGameStats, 0, 0);
        return true;
    }
    else if (ShooterGI && ShooterGI->bPracticeMode && ShooterGI->GameAccessCheck.bIsFullVersion)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void AShooterGameState::SetEndMatchMetrics()
{
    if (ShooterGI)
    {
        for (auto ShooterPS : ShooterPlayerArray)
        {
            if (ShooterPS)
            {
                UpdateStats(ShooterPS);
                switch (ShooterGI->GameType)
                {
                case EGameModeType::EGMT_freeSolo:
                    ShooterPS->AddToXPMetric(EProgressMetric::EPM_SoloMatchesPlayed, 1);
                    break;
                case EGameModeType::EGMT_freeTeam:
                    ShooterPS->AddToXPMetric(EProgressMetric::EPM_TeamMatchesPlayed, 1);
                    break;
                case EGameModeType::EGMT_tournament:
                case EGameModeType::EGMT_tournamentTeam:
                    ShooterPS->AddToXPMetric(EProgressMetric::EPM_StakingMatchesPlayed, 1);
                    break;
                
                default:
                    ShooterPS->AddToXPMetric(EProgressMetric::EPM_SoloMatchesPlayed, 1);
                    break;
                }
                
                if (ShooterPS->PlayerGameStats.KDR_raw >= 2.f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_KdRatioAtLeast2, 1);
                if (ShooterPS->PlayerGameStats.KDR_raw >= 3.f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_KdRatioAtLeast3, 1);
                if (ShooterPS->PlayerGameStats.KDR_raw >= 4.f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_KdRatioAtLeast4, 1);
                if (ShooterPS->PlayerGameStats.DTR_raw >= 0.35f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_DamageEfficiency35Percent, 1);
                if (ShooterPS->PlayerGameStats.DTR_raw >= 0.50f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_DamageEfficiency50Percent, 1);
                if (ShooterPS->PlayerGameStats.DTR_raw >= 0.75f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_DamageEfficiency75Percent, 1);
                if (ShooterPS->PlayerGameStats.DamageDealt >= 1000.f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_Deal1000Damage, 1);
                if (ShooterPS->PlayerGameStats.Accuracy >= 0.3f) ShooterPS->AddToXPMetric(EProgressMetric::EPM_AccuracyAtLeast30Percent, 1);
                if (!ShooterPS->bHitByMapMissile) ShooterPS->AddToXPMetric(EProgressMetric::EPM_NoDamageFromMapMissile, 1);
                if (ShooterPS->HandShieldHits > 0) ShooterPS->AddToXPMetric(EProgressMetric::EPM_BlockShotsWithShield, ShooterPS->HandShieldHits);
                if (ShooterPS->PlayedEmotes > 0) ShooterPS->AddToXPMetric(EProgressMetric::EPM_PlayFullEmote, ShooterPS->PlayedEmotes);
                if (ShooterPS->NumGrapples > 0) ShooterPS->AddToXPMetric(EProgressMetric::EPM_GrappleSwings, ShooterPS->NumGrapples);
                if (ShooterPlayerArray.Num() >= 3 && ShooterPS->PlayerGameStats.MatchRank <= 3) ShooterPS->AddToXPMetric(EProgressMetric::EPM_FinishTopThree, 1);
                if (ShooterPlayerArray.Num() >= 10 && ShooterPS->PlayerGameStats.MatchRank <= 10) ShooterPS->AddToXPMetric(EProgressMetric::EPM_FinishTopTen, 1);
                if (ShooterPlayerArray.Num() >= 3 && ShooterPS->PlayerGameStats.MatchRank == 1) ShooterPS->AddToXPMetric(EProgressMetric::EPM_FinishFirstPlace, 1);
            }
        }
    }
}

void AShooterGameState::EndMatch()
{
    if (!HasAuthority()) return;
    
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI == nullptr) return;
    if (!CheckIsValidGameMode()) return;

    #if !WITH_EDITOR
        QuitServer(true, 90.f, false);
    #endif
    UE_LOG(LogTemp, Warning, TEXT("Match has ended!"));
    GameMatchState = EGameMatchState::EMS_Stop;
    StopAISearch();

    HTTPRequestObj = NewObject<URequestsObject>(this);
    if (HTTPRequestObj) HTTPRequestObj->OnRequestResponseDelegate.AddDynamic(this, &AShooterGameState::PostResponseReceived);

    TArray<FPlayerGameStats> PlayerStatsArray = GetRankedPlayerScores();
    SetEndMatchMetrics();
    ShooterGI->bTeamMode ? TeamEndMatch(PlayerStatsArray) : SoloEndMatch(PlayerStatsArray);

    //PostPlayerStats();
    #if !WITH_EDITOR
        PostPlayerStats();
    #endif
}

void AShooterGameState::DelayedTournamentCheck(const TArray<FPlayerGameStats> &PlayerStatsArray)
{
    UWorld* World = GetWorld();
    if (World)
    {
        FTimerDelegate TournamentCheckDel;
        FTimerHandle TournamentCheckTimer;
        TournamentCheckDel.BindUFunction(this, FName("TournamentCheck"), PlayerStatsArray);
        World->GetTimerManager().SetTimer(TournamentCheckTimer, TournamentCheckDel, 2.f, false);
    }
}

void AShooterGameState::TournamentCheck(const TArray<FPlayerGameStats> &PlayerStatsArray)
{
    if (HTTPRequestObj && ShooterGI && (ShooterGI->GameType == EGameModeType::EGMT_tournament || ShooterGI->GameType == EGameModeType::EGMT_tournamentTeam))
    {
        FKeyValue Params;
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        for (auto Stat : PlayerStatsArray)
        {
            TArray<float> StatArray = {Stat.Score, Stat.KDR_raw, Stat.DTR_raw, float(Stat.TeamId)};
            TArray<TSharedPtr<FJsonValue>> JsonArray;
            for (float StatVal : StatArray)
            {
                JsonArray.Add(MakeShared<FJsonValueNumber>(StatVal));
            }
            //if (!Stat.PlayerId.IsEmpty()) JsonObject->SetNumberField(Stat.PlayerId, Stat.Score);
            if (!Stat.PlayerId.IsEmpty()) JsonObject->SetArrayField(Stat.PlayerId, JsonArray);
        }
        JsonObject->SetStringField(TEXT("tournamentId"), ShooterGI->TournamentParams._id);
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Sending tournament into for tournament ID: %s"), *ShooterGI->TournamentParams._id));
        Params.JsonArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));

        FString Data = ShooterGI->FormHMACData(Params.JsonArray[0]);
        const FString HMACSignature = IGeneralInterface::Execute_GenerateHMAC(
            UGameplayStatics::GetGameInstance(this),
            Data,
            !ShooterGI->ServerKeys.PostRequestKey.IsEmpty() ? ShooterGI->ServerKeys.PostRequestKey : "1");

        Params.AddHeader("auth", HMACSignature);
        HTTPRequestObj->MakeRequest(Params, "tournament/setScores", ERequestType::ERT_POST, EURLType::EUT_Main);
    }
}

void AShooterGameState::TeamTournamentCheck(const TArray<FTeamPlayerStates> &TeamStatsArray)
{
    // Am doing the calculations for this in the backend instead
    TArray<FPlayerGameStats> TeamPlayerStats;
    for (auto Team : TeamStatsArray)
    {
        FTeamPlayerStates SortedTeam = Team;
        SortedTeam.ScoreControllers.Sort([](const FPlayerControllerScores& A, const FPlayerControllerScores& B) {
            return A.PlayerScore > B.PlayerScore;
        });

        for (auto ShooterCon : SortedTeam.ScoreControllers)
        {
            FPlayerGameStats ShooterStat;
            ShooterStat.PlayerId = ShooterCon.PlayerId;
            ShooterStat.Score = ShooterCon.PlayerScore;
            TeamPlayerStats.Add(ShooterStat);
        }
    }
    TournamentCheck(TeamPlayerStats);
}

void AShooterGameState::SoloEndMatch(const TArray<FPlayerGameStats> &PlayerStatsArray)
{
    if (PlayerStatsArray.Num() > 0 && ShooterGI)
    {
        FString WinnerName = PlayerStatsArray[0].PlayerName;
        TArray<FString> WinnerNames; WinnerNames.Add(WinnerName);
        AShooterPlayerController* ShooterControllerWinner = nullptr;
        AShooterAIController* ShooterAIWinnerController = nullptr;
        USceneComponent* AICameraScene = nullptr;

        for (auto PS : PlayerArray)
        {
            if (PS && PS->GetPlayerName() == WinnerName)
            {
                //UE_LOG(LogTemp, Warning, TEXT("Winner Name = %s"), *WinnerName);
                ShooterControllerWinner = Cast<AShooterPlayerController>(PS->GetPlayerController());
                
                if (ShooterControllerWinner)
                {
                    ShooterControllerWinner->OnGameEndForWinner();
                    ShooterControllerWinner->ClientShowTopPlayers(PlayerStatsArray, true, WinnerNames, ShooterGI->GameType);
                    //ShooterControllerWinner->ClientShowPrizeAmount(67.12, 100.4897, TEXT("polygon"), TEXT("TourName"));
                }
                else
                {
                    // Condition if AI won the match
                    ShooterAIWinnerController = Cast<AShooterAIController>(PS->GetOwningController());
                    if (ShooterAIWinnerController)
                    {
                        AICameraScene = ShooterAIWinnerController->OnGameEndForWinner();
                    }
                }
                break;
            }
        }

        for (auto ShooterController : ShooterControllerArray)
        {
            if (ShooterController && ShooterControllerWinner)
            {
                if (ShooterControllerWinner != ShooterController)
                {
                    ShooterController->OnMatchStateSet(GameMatchState);
                    ShooterController->SetViewTargetWithBlend(ShooterControllerWinner->GetPawn(), 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                    // Spawn the Spectator Pawn *locally* on this client
                    //AShooterSpectatorPawn* PlayerSpectatorPawn = SpawnSpectator(ShooterControllerWinner);
                    //if (PlayerSpectatorPawn) ShooterController->SetViewTargetWithBlend(PlayerSpectatorPawn, 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                    ShooterController->ClientShowTopPlayers(PlayerStatsArray, false, WinnerNames, ShooterGI->GameType);
                }
            }
            else if (ShooterController && ShooterAIWinnerController)
            {
                ShooterController->OnMatchStateSet(GameMatchState);
                if (AICameraScene)
                {
                    ShooterController->ClientStartSetViewToAI(
                        AICameraScene->GetComponentLocation(),
                        AICameraScene->GetComponentRotation()
                    );
                }

                //ShooterController->SetViewTargetWithBlend(AICharacter, 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                ShooterController->ClientShowTopPlayers(PlayerStatsArray, false, WinnerNames, ShooterGI->GameType);
            }
        }
        DelayedTournamentCheck(PlayerStatsArray);
    }
}

void AShooterGameState::TeamEndMatch(const TArray<FPlayerGameStats> &PlayerStatsArray)
{
    if (PlayerStatsArray.Num() > 0 && ShooterGI)
    {
        ConstructTeamArray();
        for (FTeamPlayerStates &TeamInfo : TeamInfoArray)
        {       
            float TeamScore = 0;
            for (const FPlayerGameStats &PlayerStat : PlayerStatsArray)
            {
                if (PlayerStat.TeamId == TeamInfo.TeamID)
                {
                    TeamScore += PlayerStat.Score;
                    for (auto PC : TeamInfo.TeamControllerArray)
                    {
                        if (PC && PC->PlayerState->GetPlayerName() == PlayerStat.PlayerName)
                        {
                            TeamInfo.AddScoreController(PC, PlayerStat.Score, PlayerStat.PlayerId);
                        }
                    }
                }
            }
            TeamInfo.TeamAvgScore = TeamScore / TeamInfo.TeamArray.Num();
            UE_LOG(LogTemp, Warning, TEXT("Team Average Score = %f for team %i"), TeamInfo.TeamAvgScore, TeamInfo.TeamID);
        }

        TeamInfoArray.Sort([](const FTeamPlayerStates& A, const FTeamPlayerStates& B) {
            return A.TeamAvgScore > B.TeamAvgScore;
        });
        //TeamTournamentCheck(TeamInfoArray);

        AShooterPlayerController* TopWinnerController = nullptr;
        TArray<FString> WinnerNames;
        if (TeamInfoArray.Num() > 0)
        {
            FTeamPlayerStates WinnerTeam = TeamInfoArray[0];
            WinnerTeam.ScoreControllers.Sort([](const FPlayerControllerScores& A, const FPlayerControllerScores& B) {
                return A.PlayerScore > B.PlayerScore;
            });

            for (auto PS : WinnerTeam.TeamArray)
            {
                if (PS) WinnerNames.Add(PS->GetPlayerName());
            }

            TopWinnerController = WinnerTeam.ScoreControllers[0].PlayerController;
            //TopWinnerController = WinnerTeam.TeamControllerArray[FMath::RandRange(0, WinnerTeam.TeamControllerArray.Num() - 1)];
            for (auto WinnerPC : WinnerTeam.TeamControllerArray)
            {
                if (WinnerPC)
                {
                    if (WinnerPC == TopWinnerController)
                    {
                        WinnerPC->OnGameEndForWinner();
                    }
                    else
                    {
                        WinnerPC->OnMatchStateSet(GameMatchState);
                        if (TopWinnerController) WinnerPC->SetViewTargetWithBlend(TopWinnerController->GetPawn(), 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                        /*
                        if (TopWinnerController)
                        {
                            AShooterSpectatorPawn* PlayerSpectatorPawn = SpawnSpectator(TopWinnerController);
                            if (PlayerSpectatorPawn) WinnerPC->SetViewTargetWithBlend(PlayerSpectatorPawn, 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                        }
                        */
                    }
                    WinnerPC->ClientShowTopPlayers(PlayerStatsArray, true, WinnerNames, ShooterGI->GameType);
                }
            }
        }

        if (TeamInfoArray.Num() > 1)
        {
            for (int32 i = 1; i < TeamInfoArray.Num(); i++)
            {
                for (auto LoserController : TeamInfoArray[i].TeamControllerArray)
                {
                    if (LoserController)
                    {
                        LoserController->OnMatchStateSet(GameMatchState);
                        if (TopWinnerController) LoserController->SetViewTargetWithBlend(TopWinnerController->GetPawn(), 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                        /*
                        if (TopWinnerController)
                        {
                            AShooterSpectatorPawn* PlayerSpectatorPawn = SpawnSpectator(TopWinnerController);
                            if (PlayerSpectatorPawn) LoserController->SetViewTargetWithBlend(PlayerSpectatorPawn, 1.5f, EViewTargetBlendFunction::VTBlend_EaseIn);
                        }
                        */
                        LoserController->ClientShowTopPlayers(PlayerStatsArray, false, WinnerNames, ShooterGI->GameType);
                    }
                }
            }
        }
        DelayedTournamentCheck(PlayerStatsArray);
    }
}

AShooterSpectatorPawn* AShooterGameState::SpawnSpectator(AShooterPlayerController* WinnerController)
{
    if (ShooterSpectatorClass && GetWorld() && WinnerController && WinnerController->GetPawn())
    {
        AShooterSpectatorPawn* PlayerSpectatorPawn = GetWorld()->SpawnActor<AShooterSpectatorPawn>(
            AShooterSpectatorPawn::StaticClass(), 
            WinnerController->GetPawn()->GetActorLocation(), 
            FRotator::ZeroRotator
        );
    
        if (PlayerSpectatorPawn)
        {
            PlayerSpectatorPawn->SetTarget(WinnerController->GetPawn());
            return PlayerSpectatorPawn;
        }
    }
    return nullptr;
}

// Called every frame
void AShooterGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority)
	{
		ServerFPS = GAverageFPS;
	}
}

void AShooterGameState::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    //InitMaterialInstances();
}

void AShooterGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AShooterGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AShooterGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AShooterGameState::GameStateExpiration()
{
    SetWarningState();
    /*
    if (GameMatchState <= EGameMatchState::EMS_Idle && PlayerArray.Num() >= MinPlayersToStart)
    {
        SetSessionMaxPlayers();
        SetWarningState();
    }
    else if (GameMatchState <= EGameMatchState::EMS_Idle && PlayerArray.Num() < MinPlayersToStart)
    {
        //Notify players that not enough players to start. Abandon match?
    }
    */
}

void AShooterGameState::CheckStartMatch(AShooterPlayerState* ShooterPS)
{
	if (PlayerArray.Num() < 5)
	{
        auto ShooterController = Cast<AShooterPlayerController>(ShooterPS->GetPlayerController());
        if (ShooterController)
        {
            ShooterController->OnMatchStateSet(EGameMatchState::EMS_Idle);
        }
	}
	else
	{
        SetMaxPlayers(PlayerArray.Num());
        //SetSessionMaxPlayers();
        //SetMatchState(MatchState::EnteringMap);
        //SetWarningState();
	}
    // Testing with a delay here...
    FTimerHandle WarnTimer;
    GetWorldTimerManager().SetTimer(WarnTimer, this, &AShooterGameState::SetWarningState, 1.f);
}

void AShooterGameState::SetSessionMaxPlayers()
{
    auto ShooterGM = GetWorld()->GetAuthGameMode<AShooterGameMode>();
    if (ShooterGM)
    {
        UClass* gameSessionClass = ShooterGM->GetGameSessionClass();
        if (gameSessionClass)
        {
            AGameSession* gameSession = GetWorld()->SpawnActor<AGameSession>(gameSessionClass);
            gameSession->MaxPlayers = PlayerArray.Num();
        }
    }
}

void AShooterGameState::SetWarningState()
{
    if (GameMatchState >= EGameMatchState::EMS_Warn) return;
    UE_LOG(LogTemp, Warning, TEXT("Setting GameMatchState to Warning"));
    GameMatchState = EGameMatchState::EMS_Warn;
    FTimerHandle StartMatchTimer;
    #if WITH_EDITOR
        GetWorldTimerManager().SetTimer(StartMatchTimer, this, &AShooterGameState::StartBattle, 1.f);
    #else
        GetWorldTimerManager().SetTimer(StartMatchTimer, this, &AShooterGameState::StartBattle, 5.f);
    #endif
    
    for (int32 i = 0; i < PlayerArray.Num(); i++)
    {
        auto ShooterController = Cast<AShooterPlayerController>(PlayerArray[i]->GetPlayerController());
        if (ShooterController)
        {
            ShooterController->OnMatchStateSet(GameMatchState);
        }
    }
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI && ShooterGI->bTeamMode)
    {
        //ConstructTeamArray();
        //UE_LOG(LogTemp, Warning, TEXT("TeamInfoArray array Num = %i"), TeamInfoArray.Num());
    }
    auto ShooterGM = GetWorld()->GetAuthGameMode<AShooterGameMode>();
    if (ShooterGM) ShooterGM->bBattleStarted = true;
}

void AShooterGameState::ConstructTeamArray()
{
    for (auto ID : TeamIDArray)
    {
        if (ID < 1000) // over 1000 are AI's
        {
            FTeamPlayerStates TeamStruct;
            TeamStruct.TeamID = ID;
            for (auto ShooterPS : ShooterPlayerArray)
            {
                //auto ShooterPS = Cast<AShooterPlayerState>(PlayerArray[i]);
                if (ShooterPS && ShooterPS->PlayerGameStats.TeamId == ID)
                {
                    TeamStruct.TeamArray.Add(ShooterPS);
                    TeamStruct.TeamControllerArray.Add(Cast<AShooterPlayerController>(ShooterPS->GetPlayerController()));
                }
            }
            TeamInfoArray.Add(TeamStruct);
        }
    }
}

void AShooterGameState::SetVoiceVolume(AShooterPlayerState* NewShooterPS)
{
    for (auto PS : ShooterPlayerArray)
    {
        if (PS && PS != NewShooterPS) PS->ClientSetVoiceVolume(NewShooterPS);
    }
}

void AShooterGameState::StartBattle()
{
    GameMatchState = EGameMatchState::EMS_Start;
    for (int32 i = 0; i < PlayerArray.Num(); i++)
    {
        auto ShooterController = Cast<AShooterPlayerController>(PlayerArray[i]->GetPlayerController());
        if (ShooterController)
        {
            ShooterController->OnMatchStateSet(GameMatchState);
            auto ShooterChar = Cast<AShooterCharacter>(ShooterController->GetPawn());
            if (ShooterChar)
            {
                ShooterChar->bStartFlying = false;
                ShooterChar->bStopFlying = true;
                ShooterChar->OnRep_StartFlying();
            }
        }
    }
}

/*
void AShooterGameState::InitMaterialInstances()
{
    
    if (MinimapMaterialInstance && LargeMapMaterialInstance)
    {
        DynamicMinimapMaterialInstance = UMaterialInstanceDynamic::Create(MinimapMaterialInstance, this);
        DynamicMinimapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), 0.f);

        DynamicLargeMapMaterialInstance = UMaterialInstanceDynamic::Create(LargeMapMaterialInstance, this);
        DynamicLargeMapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), 0.f);
        
        InitializeDynamicMaterial(DynamicMinimapMaterialInstance, DynamicLargeMapMaterialInstance);
    }
}
*/

void AShooterGameState::GetContainerSplineActor()
{
    for (TActorIterator<AContainerSplinePath> ActorIt(GetWorld()); ActorIt; ++ActorIt)
    {
        PathActor.Add(*ActorIt);
        //UE_LOG(LogTemp, Warning, TEXT("Set PathActor: %s"), *ActorIt->GetFullName());
    }
}

void AShooterGameState::DroneStart()
{
    FTimerHandle DroneTimer;
    GetWorldTimerManager().SetTimer(DroneTimer, this, &AShooterGameState::DroneTimer, 5.f);
    //UE_LOG(LogTemp, Warning, TEXT("DroneStart"));
}

void AShooterGameState::DroneTimer()
{
    if (DroneActor.IsEmpty())
    {
        if (ContainerClass)
        {
            UWorld* World = GetWorld();
            for (int i = 0; i < NumDronesToSpawn; i++)
            {
                if (!PathActor.IsValidIndex(i) || PathActor[i] == nullptr)
                {
                    DroneStart();
                    break;
                }
                //RandomDronePathIndex = FMath::RandRange(0, PathActor.Num() - 1);
                PathActor[i]->GenerateRandomKey();
                
                if (World)
                {
                    //UE_LOG(LogTemp, Warning, TEXT("ServerDroneTimer- spawn at RandomDronePathIndex = %i"), RandomDronePathIndex);
                    AItemContainer* SpawnedDrone = World->SpawnActor<AItemContainer>(
                        ContainerClass,
                        PathActor[i]->ContainerSpline->GetLocationAtSplineInputKey(PathActor[i]->RandomSplineIndex, ESplineCoordinateSpace::World),
                        PathActor[i]->ContainerSpline->GetRotationAtSplineInputKey(PathActor[i]->RandomSplineIndex, ESplineCoordinateSpace::World)
                    );
                    if (SpawnedDrone)
                    {
                        SpawnedDrone->SelectedPath = PathActor[i];
                        SpawnedDrone->StartProcess();
                        DroneActor.Add(SpawnedDrone);
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < NumDronesToSpawn; i++)
        {
            if (DroneActor.IsValidIndex(i) && DroneActor[i] == nullptr)
            {
                UWorld* World = GetWorld();
                if (!PathActor.IsValidIndex(i) || PathActor[i] == nullptr)
                {
                    DroneStart();
                    break;
                }
                PathActor[i]->GenerateRandomKey();
                
                if (World)
                {
                    //UE_LOG(LogTemp, Warning, TEXT("ServerDroneTimer- spawn at RandomDronePathIndex = %i"), RandomDronePathIndex);
                    AItemContainer* SpawnedDrone = World->SpawnActor<AItemContainer>(
                        ContainerClass,
                        PathActor[i]->ContainerSpline->GetLocationAtSplineInputKey(PathActor[i]->RandomSplineIndex, ESplineCoordinateSpace::World),
                        PathActor[i]->ContainerSpline->GetRotationAtSplineInputKey(PathActor[i]->RandomSplineIndex, ESplineCoordinateSpace::World)
                    );
                    if (SpawnedDrone)
                    {
                        SpawnedDrone->SelectedPath = PathActor[i];
                        SpawnedDrone->StartProcess();
                        DroneActor[i] = SpawnedDrone;
                    }
                }  
            }
        }
    }
}

void AShooterGameState::DroneDestroyed(AItemContainer* DestroyedDrone)
{
    for (auto Drone : DroneActor)
    {
        if (Drone == DestroyedDrone)
        {
            int32 DestroyedIndex = DroneActor.Find(DestroyedDrone);
            DroneActor[DestroyedIndex] = nullptr;
        }
    }
    FTimerHandle DroneTimer;
    GetWorldTimerManager().SetTimer(DroneTimer, this, &AShooterGameState::DroneTimer, 3.f);
}

void AShooterGameState::OnRep_DroneActor()
{
    DroneDelegate.Broadcast(DroneActor);
}

void AShooterGameState::ShortenMapWithMissiles()
{
    LocationIndex = GetMissileLocationIndex();
    MissileHitLocations.Add(MissileEndLocation[LocationIndex]);
    FVector SpawnLocation{ MissileStartLocation[LocationIndex] };
    // Spawn the missile close to the floor so that we reduce the time that ActivateAWSSession is called in GameMode
    SpawnLocation.Z = MissileEndLocation[LocationIndex].Z + 5000.f;
    FVector TargetLocation{ MissileEndLocation[LocationIndex] };
    FRotator TargetRotation = (TargetLocation - SpawnLocation).Rotation();
    UpdateAIPatrolPoints();
    SpawnMissileAsync(SpawnLocation, TargetRotation);
}

void AShooterGameState::MissileStart()
{
    FTimerHandle MissileWarningTimer;
    #if WITH_EDITOR
        GetWorldTimerManager().SetTimer(MissileWarningTimer, this, &AShooterGameState::MissileStartWarning, 5.f);
    #else
        GetWorldTimerManager().SetTimer(MissileWarningTimer, this, &AShooterGameState::MissileStartWarning, MissleSpawnDelay);

    #endif
    /*
        UWorld* World = GetWorld();
        if (!bMultiplierEventFired && World)
        {
            if (auto GM = World->GetAuthGameMode<AShooterGameMode>())
            {
                ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
                if (ShooterGI)
                {
                    switch (ShooterGI->GameType)
                    {
                    case EGameModeType::EGMT_tournament:
                    case EGameModeType::EGMT_tournamentTeam:
                            if (GM->GetCountdownTime() < MultiplierEventTriggerTime)
                            {
                                // Trigger multiplier event
                                GetWorldTimerManager().SetTimer(MissileWarningTimer, this, &AShooterGameState::StartMultiplierEvent, MissleSpawnDelay / 2.f);
                                
                                bMultiplierEventFired = true;
                            }
                        break;
                    
                    default:
                        break;
                    }
                }
            }
        }
    */
}

void AShooterGameState::MissileStartWarning()
{
    if (MissileStartLocation.Num() <= 0) return;

    //OnRep_MissileWarning();
    if (GameMatchState == EGameMatchState::EMS_Start)
    {
        bMissileWarning = true;
        //LocationIndex = FMath::RandRange(0, MissileStartLocation.Num() - 1);
        LocationIndex = GetMissileLocationIndex();
        MissileHitLocations.Add(MissileEndLocation[LocationIndex]);
        GetWorldTimerManager().SetTimer(MissleSpawnTimer, this, &AShooterGameState::FireMissle, FireMissileTime);
        StartMissileWarning();
    }
    else
    {
        MissileStart();
    }
}

int32 AShooterGameState::GetMissileLocationIndex()
{
    if (MissileStartLocation.Num() >= 9)
    {
        int8 MissileIndex = FMath::RandRange(0, NumMissileCorners - 1);
        NumMissileCorners -= 1;
        return MissileIndex;
    }
    else if (MissileStartLocation.Num() < 9 && MissileStartLocation.Num() >= 3)
    {
        int8 MissileIndex = FMath::RandRange(0, NumMissileSides - 1);
        NumMissileSides -= 1;
        return MissileIndex;
    }
    else
    {
        int8 MissileIndex = FMath::RandRange(0, NumMissileMiddle - 1);
        NumMissileMiddle -= 1;
        return MissileIndex;
    }
}

/*
void AShooterGameState::OnRep_MissileWarning()
{
    if (bMissileWarning)
    {
        StartMissileWarning();
    }
    else
    {
        MissileTimeline->Stop();
        if (DynamicMinimapMaterialInstance && DynamicLargeMapMaterialInstance)
        {
            DynamicMinimapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), 0.f);
            DynamicLargeMapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), 0.f);
        }
    }
}
*/

void AShooterGameState::FireMissle()
{
    if (!HasAuthority()) return;
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    bMissileWarning = false;
    //OnRep_MissileWarning();
    StopMissileWarning();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    //SpawnParams.Instigator = InstigatorPawn;
    //SetInstigator(InstigatorPawn);

    FVector SpawnLocation{ MissileStartLocation[LocationIndex] };
    FVector TargetLocation{ MissileEndLocation[LocationIndex] };
    FRotator TargetRotation = (TargetLocation - SpawnLocation).Rotation();
    UpdateAIPatrolPoints();
    //UE_LOG(LogTemp, Warning, TEXT("Missile Location = %s"), *MissileStartLocation[LocationIndex].ToString());
    //UE_LOG(LogTemp, Warning, TEXT("Missile Location Index = %i"), LocationIndex);

    SpawnMissileAsync(SpawnLocation, TargetRotation);
}

void AShooterGameState::SpawnMissileAsync(const FVector& SpawnLocation, const FRotator& TargetRotation)
{
    if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
    {
        FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
        Streamable.RequestAsyncLoad(
            MissleClass.ToSoftObjectPath(),
            FStreamableDelegate::CreateUObject(this, &AShooterGameState::OnMissileClassLoaded, SpawnLocation, TargetRotation)
        );
    }
    else
    {
        OnMissileClassLoaded(SpawnLocation, TargetRotation);
    }
}

void AShooterGameState::OnMissileClassLoaded(FVector SpawnLocation, FRotator TargetRotation)
{
    UClass* LoadedClass = MissleClass.Get();
    if (!LoadedClass) return;

    UWorld* World = GetWorld();
    if (!World) return;

    if (auto ShooterGM = World->GetAuthGameMode<AShooterGameMode>())
    {
        int32 NumMissilesToPrespawn = ShooterGM->GetNumMissilesToPrespawn();
        if (MissileHitLocations.Num() < NumMissilesToPrespawn)
        {
            FTimerHandle ShortMissileWarningTimer;
            GetWorldTimerManager().SetTimer(ShortMissileWarningTimer, this, &AShooterGameState::ShortenMapWithMissiles, 1.f);
        }

        AProjectile* ProjectileActor = World->SpawnActor<AProjectile>(
            LoadedClass,
            SpawnLocation,
            TargetRotation,
            FActorSpawnParameters()
        );

        MissileStartLocation.RemoveAt(LocationIndex);
        MissileEndLocation.RemoveAt(LocationIndex);

        //UE_LOG(LogTemp, Warning, TEXT("OnMissileClassLoaded: %i"), MissileHitLocations.Num());

        //if (MissileStartLocation.Num() == 6) StopBackfill();
        if (MissileHitLocations.Num() == NumMissilesToPrespawn) return;
        if (MissileHitLocations.Num() >= NumMissilesToPrespawn) MissileStart();
    }
}

void AShooterGameState::UpdateAIPatrolPoints()
{
    if (MissileEndLocation.Num() <= 1) return; // don't remove points when only 1 missile left to fire
    TArray<FVector> PointsToRemove;

    for (const FVector& PatrolPoint : AIPatrolPoints)
    {
        if (PatrolPoint.X >= MissileEndLocation[LocationIndex].X - (MissileBoxScale.X / 2) &&
            PatrolPoint.X <= MissileEndLocation[LocationIndex].X + (MissileBoxScale.X / 2) &&
            PatrolPoint.Y >= MissileEndLocation[LocationIndex].Y - (MissileBoxScale.Y / 2) &&
            PatrolPoint.Y <= MissileEndLocation[LocationIndex].Y + (MissileBoxScale.Y / 2))
        {
            PointsToRemove.Add(PatrolPoint);
        }
    }

    for (const FVector& Point : PointsToRemove)
    {
        if (AIPatrolPoints.Contains(Point)) AIPatrolPoints.Remove(Point);
    }
}

void AShooterGameState::InitializeTeleportLocations()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (!ShooterGI) return;
	if (!ShooterGI->bPracticeMode)
	{
		AllTeleportLocations = {
			{-14060.0,160.0,270.0},
			{-36280.0,-6000.0,270.0},
			{-40600.0,-18520.0,260.0},
			{-43580.0,-8850.0,3100.0},
			{-55990.0,-4890.0,460.0},
			{-53700.0,4730.0,860.0},
			{-64830.0,28500.0,430.0},
			{-49590.0,38130.0,260.0},
			{-42910.0,35620.0,470.0},
			{-28610.0,27240.0,940.0},
			{-8570.0,30010.0,3200.0},
			{2080.0,32890.0,270.0},
			{45980.0,24180.0,940.0},
			{47550.0,16750.0,3400.0},
			{66310.0,13620.0,270.0},
			{50320.0,3780.0,280.0},
			{44690.0,3510.0,280.0},
			{9360.0,4530.0,440.0},
			{5160.0,-1680.0,460.0},
			{59130.0,-32450.0,250.0},
			{53270.0,-24000.0,280.0},
			{45150.0,-30240.0,380.0}
			//{2820.0,34570.0,790.0},
			//{3900.0,35200.0,1280.0},
			//{4030.0,34730.0,290.0},
			//{-3180.0,30240.0,270.0},
			//{-3940.0,29080.0,770.0},
			//{-3900.0,29370.0,1380.0},
			//{-4480.0,28120.0,1880.0},
			//{-4590.0,29510.0,1880.0},
			//{-4540.0,28220.0,2980.0}
		};
	}
	else
	{
		AllTeleportLocations = {
			{-8500.0f,11210.0f,1210.0f},
			{-9850.0f,11880.0f,170.0f},
			{-14820.0f,8850.0f,1230.0f},
			{-7630.0f,4050.0f,1330.0f},
			{-6010.0f,8710.0f,1210.0f},
			{-13910.0f,5020.0f,1350.0f}
		};
	}
}

void AShooterGameState::InitializeMissileHitLocations()
{
    float StartZ = 75000.f;
    float EndZ = 0.f;
    //MissileStartLocation = { {-680.0f, 5650.0f, 10000.f} };
    //MissileEndLocation = { {-680.0f, 5650.0f, -100.f} };
    
    MissileStartLocation.SetNum(12);
    MissileEndLocation.SetNum(12);
    MissileStartLocation = { {61860.0f, 31760.0f, StartZ},
                            {61860.0f, -31820.0f, StartZ},
                            {-62250.0f, 31760.0f, StartZ},
                            {-62250.0f, -31820.0f, StartZ},

                            {20290.0f, 31760.0f, StartZ},
                            {-20880.0f, 31760.0f, StartZ},
                            {61860.0f, -100.f, StartZ},
                            {-62250.0f, -100.f, StartZ},
                            {20290.0f, -31820.0f, StartZ},
                            {-20880.0f, -31820.0f, StartZ},

                            {20290.0f, -100.f, StartZ},
                            {-20880.0f, -100.f, StartZ}
                            }; 
    MissileEndLocation = { {61860.0f, 31760.0f, EndZ},
                            {61860.0f, -31820.0f, EndZ},
                            {-62250.0f, 31760.0f, EndZ},
                            {-62250.0f, -31820.0f, EndZ},

                            {20290.0f, 31760.0f, EndZ},
                            {-20880.0f, 31760.0f, EndZ},
                            {61860.0f, -100.f, EndZ},
                            {-62250.0f, -100.f, EndZ},
                            {20290.0f, -31820.0f, EndZ},
                            {-20880.0f, -31820.0f, EndZ},

                            {20290.0f, -100.f, EndZ},
                            {-20880.0f, -100.f, EndZ}
                            };
    /*
    MissileStartLocation = { {61860.0f, 31760.0f, StartZ},
                            {20290.0f, 31760.0f, StartZ},
                            {-20880.0f, 31760.0f, StartZ},
                            {-62250.0f, 31760.0f, StartZ},
                            {61860.0f, -100.f, StartZ},
                            {20290.0f, -100.f, StartZ},
                            {-20880.0f, -100.f, StartZ},
                            {-62250.0f, -100.f, StartZ},
                            {61860.0f, -31820.0f, StartZ},
                            {20290.0f, -31820.0f, StartZ},
                            {-20880.0f, -31820.0f, StartZ},
                            {-62250.0f, -31820.0f, StartZ}
                            }; 
    MissileEndLocation = { {61860.0f, 31760.0f, EndZ},
                            {20290.0f, 31760.0f, EndZ},
                            {-20880.0f, 31760.0f, EndZ},
                            {-62250.0f, 31760.0f, EndZ},
                            {61860.0f, -100.f, EndZ},
                            {20290.0f, -100.f, EndZ},
                            {-20880.0f, -100.f, EndZ},
                            {-62250.0f, -100.f, EndZ},
                            {61860.0f, -31820.0f, EndZ},
                            {20290.0f, -31820.0f, EndZ},
                            {-20880.0f, -31820.0f, EndZ},
                            {-62250.0f, -31820.0f, EndZ}
                            };
    */
    
}

void AShooterGameState::StartMissileWarning()
{
	for (auto ShooterController : ShooterControllerArray)
    {
        if (ShooterController)
        {
            ShooterController->ClientBroadcastMissileWarning(MissileEndLocation[LocationIndex]);
        }
    }
    
    if (MissileEndLocation.Num() <= 1) return; // don't remove points when only 1 missile left to fire
    UWorld* World = GetWorld();
    if (World)
    {
        if (auto ShooterGM = World->GetAuthGameMode<AShooterGameMode>())
        {
            TArray<FVector>& SpawnLocations = ShooterGM->SpawnLocationPoints;
            FVector MissileLocation = MissileEndLocation[LocationIndex];
            SpawnLocations.RemoveAll([&](const FVector& SpawnLoc)
            {
                return (SpawnLoc.X >= MissileLocation.X - (MissileBoxScale.X / 2) &&
                        SpawnLoc.X <= MissileLocation.X + (MissileBoxScale.X / 2) &&
                        SpawnLoc.Y >= MissileLocation.Y - (MissileBoxScale.Y / 2) &&
                        SpawnLoc.Y <= MissileLocation.Y + (MissileBoxScale.Y / 2));
                // Note: Z check is skipped to simulate "ground projection"
            });
            AllTeleportLocations.RemoveAll([&](const FVector& TeleportLoc)
            {
                return (TeleportLoc.X >= MissileLocation.X - (MissileBoxScale.X / 2) &&
                        TeleportLoc.X <= MissileLocation.X + (MissileBoxScale.X / 2) &&
                        TeleportLoc.Y >= MissileLocation.Y - (MissileBoxScale.Y / 2) &&
                        TeleportLoc.Y <= MissileLocation.Y + (MissileBoxScale.Y / 2));
            });
        }
    }
}

void AShooterGameState::StopMissileWarning()
{
    for (auto ShooterController : ShooterControllerArray)
    {
        if (ShooterController)
        {
            ShooterController->ClientStopMissileWarning();
        }
    }
}

/*
void AShooterGameState::UpdateMissileWarning(float OpacityValue)
{
    
	if (DynamicMinimapMaterialInstance && DynamicLargeMapMaterialInstance)
	{
		DynamicMinimapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), OpacityValue);
        DynamicLargeMapMaterialInstance->SetScalarParameterValue(TEXT("MissileOpacity"), OpacityValue);
	}
}
*/

int32 AShooterGameState::GetPlayersStillAlive()
{
    int32 PlayersAlive = 0;

    for (auto ShooterPS : ShooterPlayerArray)
    {   
        if (ShooterPS && !ShooterPS->bPlayerIsDead && ShooterPS->GetPlayerController())
        {
            PlayersAlive += 1;
        }
        //UE_LOG(LogTemp, Warning, TEXT("Bool in Player State = %i"), ShooterPS->bPlayerIsDead);
    }
    return PlayersAlive;
}

void AShooterGameState::UpdateElimmedPlayerCount()
{
    if (HasAuthority())
    {
        PlayersStillAlive = GetPlayersStillAlive();

        for (auto ShooterController : ShooterControllerArray)
        {
            if (ShooterController)
            {
                ShooterController->ClientBroadcastPlayersAlive(GameMatchState, PlayersStillAlive, GameMaxPlayers);
            }
        }
    }
}

void AShooterGameState::CheckWinners()
{
    if (IsPracticeMode()) return;
    if (HasAuthority())
    {
        if (GameMatchState != EGameMatchState::EMS_Start) return;
        //if (PlayersAlive <= 1) QuitServer(true);
        ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
        if (ShooterGI && !ShooterGI->bTeamMode)
        {
            CheckForWinner(PlayersStillAlive);
        }
        else
        {
            CheckForWinningTeam();
        }
    }
}

void AShooterGameState::CheckForWinner(int32 PlayersAlive)
{
    if (PlayersAlive == 1)
    {
        APawn* WinnerPawn = nullptr;
        for (int32 i = 0; i < PlayerArray.Num(); i++)
        {
            auto ShooterPS = Cast<AShooterPlayerState>(PlayerArray[i]);
            
            if (ShooterPS && !ShooterPS->bPlayerIsDead)
            {
                WinnerPawn = ShooterPS->GetPawn();
                //ShooterPS->ClientPostWinner(ShooterPS->PlayerGameStats);
                break;
            }
            //UE_LOG(LogTemp, Warning, TEXT("Bool in Player State = %i"), ShooterPS->bPlayerIsDead);
        }
        if (WinnerPawn)
        {
            auto ShooterChar = Cast<AShooterCharacter>(WinnerPawn);
            auto HelicopterChar = Cast<AHelicopter>(WinnerPawn);
            if (ShooterChar)
            {
                ShooterChar->Winner();
                UE_LOG(LogTemp, Warning, TEXT("WinnerDeclared!"));
                //QuitServer(true, 20.f);
            }
            else if (HelicopterChar)
            {
                HelicopterChar->ServerStopPossess(true);
                //QuitServer(true, 20.f);
            }
            StopAISearch();
        }
    }
}

void AShooterGameState::CheckForWinningTeam()
{
    AShooterCharacter* ShooterChar = nullptr;
    AHelicopter* HelicopterChar = nullptr;
    int32 NumOfAliveTeams = 0;
    FTeamPlayerStates AliveTeam;
    TArray<FTeamPlayerStates> DeadTeams;

    for (auto Team : TeamInfoArray)
    {
        bool EntireTeamDead = true;
        for (auto ShooterPS : Team.TeamArray)
        {
            if (ShooterPS && !ShooterPS->bPlayerIsDead)
            {
                NumOfAliveTeams += 1;
                AliveTeam = Team;
                EntireTeamDead = false;
                break;
            }
        }
        if (EntireTeamDead)
        {
            DeadTeams.Add(Team);
        }
    }

    if (DeadTeams.Num() > 0)
    {
        for (auto DeadTeam : DeadTeams)
        {
            TeamInfoArray.Remove(DeadTeam);
        }
    }

    if (NumOfAliveTeams == 1)
    {
        for (auto ShooterPS : AliveTeam.TeamArray)
        {
            ShooterChar = Cast<AShooterCharacter>(ShooterPS->GetPawn());
            HelicopterChar = Cast<AHelicopter>(ShooterPS->GetPawn());
            if (ShooterChar)
            {
                ShooterChar->Winner();
                //QuitServer(true, 20.f);
            }
            else if (HelicopterChar)
            {
                HelicopterChar->ServerStopPossess(true);
                //QuitServer(true, 20.f);
            }
        }
        StopAISearch();
    }
}

TArray<AShooterPlayerState*> AShooterGameState::GetAliveTeamMembers(FGenericTeamId TeamID)
{
    TArray<AShooterPlayerState*> TeamMembers;
	for (auto ShooterPS : ShooterPlayerArray)
    {
        if (ShooterPS && !ShooterPS->bPlayerIsDead && TeamID == ShooterPS->GetGenericTeamId())
        {
            TeamMembers.Add(ShooterPS);
        }
    }
    return TeamMembers;
}

AShooterPlayerState* AShooterGameState::GetRandomAlivePlayer()
{
    for (auto ShooterPSLoc : ShooterPlayerArray)
    {
        if (ShooterPSLoc && !ShooterPSLoc->bPlayerIsDead)
        {
            return ShooterPSLoc;
        }
    }
    return nullptr;
}

void AShooterGameState::AddPlayerState(APlayerState* PlayerState)
{
    // Determine whether it should go in the active or inactive list
    if (!PlayerState->IsInactive())
    {
        // Make sure no duplicates
        PlayerArray.AddUnique(PlayerState);

        AShooterPlayerState* ShooterPS = CastChecked<AShooterPlayerState>(PlayerState);
        const int32 Idx = ShooterPlayerArray.AddUnique(ShooterPS);

        OnPlayerArrayAddedDelegate.Broadcast(Idx);
        BP_OnPlayerArrayAddedDelegate.Broadcast(Idx);
        UWorld* World = GetWorld();

        if (HasAuthority() && World)
        {
            FTimerDelegate NewPlayerStateDel;
            FTimerHandle NewPlayerStateTimer;
            NewPlayerStateDel.BindUFunction(this, FName("OnNewPlayerState"), ShooterPS, World);
            World->GetTimerManager().SetTimer(NewPlayerStateTimer, NewPlayerStateDel, 0.1f, false);
            //if (GEngine) GEngine->AddOnScreenDebugMessage(2, 20.f, FColor::Red, FString::Printf(TEXT("Adding Player State %i"), PlayerArray.Num()));
        }
        else if (!HasAuthority() && ShooterPS && ShooterPS->GetPlayerController())
        {
            /*
            UE_LOG(LogTemp, Warning, TEXT("ClientAddPlayerState"));
            FTimerDelegate ClientBeginProcessDel;
            FTimerHandle ClientBeginProcessTimer;
            ClientBeginProcessDel.BindUFunction(this, FName("ClientBeginProcess"), ShooterPS);
            World->GetTimerManager().SetTimer(ClientBeginProcessTimer, ClientBeginProcessDel, 0.25f, false);
            */
        }
    }
}

void AShooterGameState::OnNewPlayerState(AShooterPlayerState* ShooterPS, UWorld* World)
{
    if (ShooterPS == nullptr) return;

    AShooterPlayerController* NewShooterController = Cast<AShooterPlayerController>(ShooterPS->GetPlayerController());
    if (NewShooterController) ShooterControllerArray.AddUnique(NewShooterController);

    if (IsPracticeMode()) return;

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    ShooterPS->PlayerGameStats.StartTimeMinutes = World->GetTimeSeconds() / 60.f;
    if (!ShooterPS->IsABot() && World) //If is a real player
    {
        bool bSettingTeams = false; // set only to handle the cases when using ListenServer and Editor
        if (ShooterGI)
        {
            if (ShooterGI->GameType == EGameModeType::EGMT_Lobby && GetNetMode() == NM_ListenServer)
            {
                const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
                HostPlayerId = Ids.PlayerID;
                SetTeams(ShooterPS, TEXT(""));
                bSettingTeams = true;
            }
        }
        #if WITH_EDITOR
            if (ShooterGI && ShooterGI->GameType != EGameModeType::EGMT_Lobby) CheckStartMatch(ShooterPS);
            if (!bSettingTeams) SetTeams(ShooterPS, TEXT(""));
        #endif
        //NotifyPlayerJoined(ShooterPS, ShooterPS->GetPlayerName());
        UpdateElimmedPlayerCount();
        SetVoiceVolume(ShooterPS);
        ShooterPS->PlayerGameStats.PlayerName = ShooterPS->GetPlayerName();
        UE_LOG(LogTemp, Warning, TEXT("AddPlayerState: %s"), *ShooterPS->PlayerGameStats.PlayerName);
    }
    else //If is a AI player
    {
        ShooterPS->SetGenericTeamId(UKismetMathLibrary::Conv_IntToByte(++AITeamId));
        //TeamIDArray.AddUnique(UKismetMathLibrary::Conv_IntToByte(200));
        TeamIDArray.AddUnique(AITeamId);
        ShooterPS->PlayerGameStats.TeamId = AITeamId;
    }
}

void AShooterGameState::OnRep_ShooterControllerArray()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (!ShooterGI) return;

    if (!ShooterGI->TargetGameSessionId.IsEmpty())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            FString Secret = (ShooterGI->GameType == EGameModeType::EGMT_freeSolo || ShooterGI->GameType == EGameModeType::EGMT_freeTeam) ? 
                ShooterGI->TargetGameSessionId : TEXT("");
            if (auto LocalShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
            {
                FString State = ShooterGI->bTeamMode ? TEXT("Playing Team Mode") : TEXT("Playing Solo Mode");
                LocalShooterPC->UpdateDiscordPresence(State, ShooterControllerArray.Num(), Secret, PartyId);
            }
        }
    }
}

int32 AShooterGameState::SetNonLobbyTeam()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI == nullptr) return 0;
    if (ShooterGI->bTeamMode && TeamCount > MaxTeamMembers - 1)
    {
        ++TeamIdVal;
        TeamCount = 0;
    }
    else if (!ShooterGI->bTeamMode)
    {
        ++TeamIdVal;
    }
    ++TeamCount;
    return TeamIdVal;
}

void AShooterGameState::SetTeams(AShooterPlayerState* ShooterPS, const FString& GroupId)
{
    if (ShooterPS == nullptr) return;
    
    int32 TeamId;
    LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Team group ID %s for player %s"), *GroupId, *ShooterPS->GetPlayerName()));
    if (GroupId.IsEmpty())
    {
        TeamId = SetNonLobbyTeam();
    }
    else
    {
        if (LobbyToTeamMap.Contains(GroupId))
        {
            int32 ExistingTeamId = LobbyToTeamMap[GroupId];
            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Existing Lobby Team ID: %i"), ExistingTeamId));
            int8 Count = TeamMemberCount.Contains(ExistingTeamId) ? TeamMemberCount[ExistingTeamId] : 0;
            TeamId = Count < MaxTeamMembers ? ExistingTeamId : SetNonLobbyTeam();
        }
        else
        {
            TeamId = NextAvailableTeamId++;
            LobbyToTeamMap.Add(GroupId, TeamId);
            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Setting New Lobby Team ID: %i"), TeamId));
        }
    }
    FGenericTeamId TeamIdByte = UKismetMathLibrary::Conv_IntToByte(TeamId);
    ShooterPS->SetGenericTeamId(TeamIdByte);
    TeamIDArray.AddUnique(TeamId);
    ShooterPS->PlayerGameStats.TeamId = TeamId;
    TeamMemberCount.FindOrAdd(TeamId) += 1;
}

void AShooterGameState::ParsePlayerSessionData(AShooterPlayerState* ShooterPS, const FString& DataStr)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("groupId")))
        {
            SetTeams(ShooterPS, JsonObject->GetStringField(TEXT("groupId")));
        }
    }
}

void AShooterGameState::ClientBeginProcess(AShooterPlayerState* ClientPS)
{
    ClientPS->OnClientJoined();
}

void AShooterGameState::NotifyPlayerJoined(APlayerState* NewPlayerState, FString NewPlayerName)
{
    for (int32 i = 0; i < PlayerArray.Num(); i++)
    {
        auto ShooterController = Cast<AShooterPlayerController>(PlayerArray[i]->GetPlayerController());
        if (ShooterController && PlayerArray[i] && PlayerArray[i] != NewPlayerState)
        {
            ShooterController->ClientNotifyNewPlayer(NewPlayerName);
        }
    }
}

void AShooterGameState::UpdateStats(AShooterPlayerState* InPlayerState)
{
    if (IsPracticeMode() || InPlayerState == nullptr || GameMatchState != EGameMatchState::EMS_Stop
        || InPlayerState->bIsAI || InPlayerState->IsABot()) return;
    //if (!InPlayerState->bOldStats) return; //Used when was getting old stats
    FPlayerStatistics Stats;
    //auto ShooterGI = Cast<UShooterGameInstance>(GetGameInstance());
    //if (ShooterGI == nullptr) return;
    float Multiplier = pow(10.0f, 3);

    //UE_LOG(LogTemp, Warning, TEXT("NumOfKills for player %s = %i"), *InPlayerState->PlayerGameStats.PlayerName, InPlayerState->PlayerGameStats.NumOfKills);
    Stats.PlayerId = InPlayerState->PlayerGameStats.PlayerId;
    Stats.PlayerName = InPlayerState->PlayerGameStats.PlayerName;
    Stats.NumKills = InPlayerState->PlayerGameStats.NumOfKills;
    Stats.NumDeaths = InPlayerState->PlayerGameStats.NumOfDeaths;
    Stats.NumGamesPlayed = 1;
    Stats.NumHits = FMath::RoundToFloat((InPlayerState->PlayerGameStats.ShotsHit / 1000.f) * Multiplier) / Multiplier;
    Stats.NumMiss = FMath::RoundToFloat(((InPlayerState->PlayerGameStats.ShotsFired - InPlayerState->PlayerGameStats.ShotsHit) / 1000.f) * Multiplier) / Multiplier;
    Stats.DamageDealt = FMath::RoundToFloat((InPlayerState->PlayerGameStats.DamageDealt / 1000.f) * Multiplier) / Multiplier;
    Stats.DamageTaken = FMath::RoundToFloat((InPlayerState->PlayerGameStats.DamageTaken / 1000.f) * Multiplier) / Multiplier;
    Stats.TotalMinutesPlayed = (InPlayerState->PlayerGameStats.StopTimeMinutes - InPlayerState->PlayerGameStats.StartTimeMinutes);
    Stats.Rating = 0; // Rating is calculated in site
    Stats.OwnerPlayerState = InPlayerState;

    AllPlayerStatsArray.Add(Stats);
}

bool AShooterGameState::CheckIsValidGameMode()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI)
    {
        UE_LOG(LogTemp, Display, TEXT("Game Type: %s"), *UEnum::GetValueAsString(ShooterGI->GameType));
        #if !WITH_EDITOR
            switch (ShooterGI->GameType)
            {
                case EGameModeType::EGMT_freeSolo:
                case EGameModeType::EGMT_freeTeam:
                case EGameModeType::EGMT_tournament:
                case EGameModeType::EGMT_tournamentTeam:
                    return true;
                
                case EGameModeType::EGMT_Lobby:
                default:
                    return false;
            }
        #else
            return true; // Always valid in editor
        #endif
    }
    return false;
}

void AShooterGameState::PostPlayerStats()
{
    if (!HasAuthority()) return;
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (ShooterGI && HTTPRequestObj && AllPlayerStatsArray.Num() > 0)
	{
        FKeyValue Params;
        for (auto Stat : AllPlayerStatsArray)
        {
            TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
            JsonObject->SetStringField("userId", Stat.PlayerId);
            JsonObject->SetStringField("userName", Stat.PlayerName);
            JsonObject->SetNumberField("numKills", Stat.NumKills);
            JsonObject->SetNumberField("numDeaths", Stat.NumDeaths);
            JsonObject->SetNumberField("numHit", Stat.NumHits);
            JsonObject->SetNumberField("numMiss", Stat.NumMiss);
            JsonObject->SetNumberField("numGamesPlayed", Stat.NumGamesPlayed);
            JsonObject->SetNumberField("totalDamageDealt", Stat.DamageDealt);
            JsonObject->SetNumberField("totalDamageTaken", Stat.DamageTaken);
            JsonObject->SetNumberField("totalMinutesPlayed", Stat.TotalMinutesPlayed);
            JsonObject->SetNumberField("rating", Stat.Rating);

            if (Stat.OwnerPlayerState)
            {
                if (TSharedPtr<FJsonObject> ProgressJson = Stat.OwnerPlayerState->SerializeProgressToJson())
                {
                    JsonObject->SetNumberField("currentLevel", ProgressJson->GetNumberField(TEXT("currentLevel")));
                    JsonObject->SetObjectField("metrics", ProgressJson->GetObjectField(TEXT("metrics")));
                }
            }
            else
            {
                JsonObject->SetNumberField("currentLevel", 0);
                JsonObject->SetObjectField("metrics", MakeShareable(new FJsonObject()));
            }

            Params.JsonArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));
        }

        FString Data = ShooterGI->FormHMACData(Params.JsonArray[0]);
        const FString HMACSignature = IGeneralInterface::Execute_GenerateHMAC(
            UGameplayStatics::GetGameInstance(this),
            Data,
            !ShooterGI->ServerKeys.PostRequestKey.IsEmpty() ? ShooterGI->ServerKeys.PostRequestKey : "1");

        //UE_LOG(LogTemp, Warning, TEXT("HMACSignature = %s"), *HMACSignature);
        //UE_LOG(LogTemp, Warning, TEXT("Body = %s"), *Data);
        //UE_LOG(LogTemp, Warning, TEXT("Key = %s"), *(!ShooterGI->ServerKeys.PostRequestKey.IsEmpty() ? ShooterGI->ServerKeys.PostRequestKey : "1"));
        Params.AddHeader("auth", HMACSignature);
        FString Type = (ShooterGI->GameType == EGameModeType::EGMT_tournament || ShooterGI->GameType == EGameModeType::EGMT_tournamentTeam) ? ShooterGI->TournamentParams.type : TEXT("free");
        FString EndPoint = FString::Printf(TEXT("stats/updateStats?type=%s&tourId=%s"), *Type, *ShooterGI->TournamentParams._id);
        HTTPRequestObj->MakeRequest(Params, EndPoint, ERequestType::ERT_POST, EURLType::EUT_Main);
        UE_LOG(LogTemp, Warning, TEXT("PostPlayerStats Done"));
	}
}

void AShooterGameState::PostResponseReceived(FString ResponseStr, FString ResponseURL)
{
    if (ResponseURL.Contains(TEXT("updateStats"), ESearchCase::CaseSensitive))
    {
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Stats Posted")));
        ParsePostedStats(ResponseStr);
    }
    if (ResponseURL.Contains(TEXT("tournament/setScores"), ESearchCase::IgnoreCase))
    {
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Received tournament backend response: %s"), *ResponseStr));
        ParseTournamentData(ResponseStr);
    }
    if (ResponseURL.Contains(TEXT("gamelift/createSession"), ESearchCase::IgnoreCase))
    {
        ParseLobbyMatch(ResponseStr);
    }
}

void AShooterGameState::ParsePostedStats(FString DataStr)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("result")))
        {
            TSharedPtr<FJsonObject> ResultData = JsonObject->GetObjectField(TEXT("result"));
            if (ResultData->HasField(TEXT("playerData")))
            {
                TSharedPtr<FJsonObject> PlayerData = ResultData->GetObjectField(TEXT("playerData"));
                for (auto PC : ShooterControllerArray)
                {
                    if (PC && !PC->PlayerId.IsEmpty() && PlayerData->HasField(PC->PlayerId))
                    {
                        TSharedPtr<FJsonObject> XPData = PlayerData->GetObjectField(PC->PlayerId);
                        if (XPData.IsValid())
                        {
                            TArray<TSharedPtr<FJsonValue>> UnlockArrayJson = XPData->GetArrayField(TEXT("newUnlocks"));
                            TArray<FName> UnlockArray;
                            for (TSharedPtr<FJsonValue> JsonVal : UnlockArrayJson)
                            {
                                if (JsonVal.IsValid())
                                {
                                    UnlockArray.Add(FName(*JsonVal->AsString()));
                                }
                            }

                            PC->UpdateProgressStats(
                                XPData->GetIntegerField(TEXT("newLevel")),
                                XPData->GetIntegerField(TEXT("newXp")),
                                UnlockArray
                            );
                        }

                    }
                }
            }
        }
    }
}

void AShooterGameState::ParseTournamentData(FString DataStr)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("result")))
        {
            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Tournament: Result valid")));
            TSharedPtr<FJsonObject> ResultData = JsonObject->GetObjectField(TEXT("result"));
            if (ResultData->HasField(TEXT("tourData")))
            {
                TSharedPtr<FJsonObject> TourData = ResultData->GetObjectField(TEXT("tourData"));
                if (TourData->HasField(TEXT("prizeInfo")))
                {
                    LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Tournament: PrizeInfo valid")));
                    TSharedPtr<FJsonObject> PrizeData = TourData->GetObjectField(TEXT("prizeInfo"));
                    for (auto PC: ShooterControllerArray)
                    {
                        if (PC && !PC->PlayerId.IsEmpty() && PrizeData->HasField(PC->PlayerId))
                        {
                            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Tournament: Got data for player: %s"), *PC->PlayerId));
                            TSharedPtr<FJsonObject> UserData = PrizeData->GetObjectField(PC->PlayerId);
                            PC->ClientShowPrizeAmount(
                                UserData->GetNumberField(TEXT("percent")),
                                UserData->GetNumberField(TEXT("totalStake")),
                                UserData->GetStringField(TEXT("network")),
                                UserData->GetStringField(TEXT("name"))
                            );
                        }
                    }
                }
            }
        }
    }
}

void AShooterGameState::RemovePlayerState(APlayerState* PlayerState)
{
    //@TODO: This isn't getting called right now (only the 'rich' AGameMode uses it, not AGameModeBase)
    // Need to at least comment the engine code, and possibly move things around

    //auto ShooterPS = Cast<AShooterPlayerState>(PlayerState);
    for (int32 i = 0; i < PlayerArray.Num(); i++)
    {
        if (PlayerArray[i] == PlayerState)
        {
            PlayerArray.RemoveAt(i, 1);

            AShooterPlayerState* ShooterPS = CastChecked<AShooterPlayerState>(PlayerState);

            ShooterPlayerArray.Remove(ShooterPS);

            OnPlayerArrayRemovedDelegate.Broadcast(ShooterPS);
            BP_OnPlayerArrayRemovedDelegate.Broadcast(ShooterPS);

            AShooterPlayerController* OutShooterController = Cast<AShooterPlayerController>(ShooterPS->GetPlayerController());
            if (OutShooterController && ShooterControllerArray.Contains(OutShooterController)) ShooterControllerArray.Remove(OutShooterController);

            if (HasAuthority() && ShooterPS)
            {
                UpdateElimmedPlayerCount();
                //CheckWinners();
            }
            return;
        }
    }
}

void AShooterGameState::KickPlayer(const FString& PlayerToKickId)
{
    if (!HasAuthority()) return;
    UWorld* World = GetWorld();
    if (World)
    {
        for (auto ShooterPC : ShooterControllerArray)
        {
            if (ShooterPC->PlayerId == PlayerToKickId)
            {
                World->GetAuthGameMode()->GameSession->KickPlayer(ShooterPC, FText::FromString(TEXT("You are not registered for this tournament")));
                return;
            }
        }
    }
}

/*
void AShooterGameState::MulticastMessageToClients_Implementation(const FShooterVerbMessage Message)
{
    if (GetNetMode() == NM_Client)
    {
        UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
    }
}

void AShooterGameState::MulticastReliableMessageToClients_Implementation(const FShooterVerbMessage Message)
{
    MulticastMessageToClients_Implementation(Message);
}
*/

void AShooterGameState::OnPlayerArrayAdded_Register(FShooterOnPlayerArrayAddedDelegate::FDelegate Delegate)
{
    if (!OnPlayerArrayAddedDelegate.IsBoundToObject(Delegate.GetUObject()))
    {
        OnPlayerArrayAddedDelegate.Add(Delegate);
    }
}

void AShooterGameState::OnPlayerArrayRemoved_Register(FShooterOnPlayerArrayRemovedDelegate::FDelegate Delegate)
{
    if (!OnPlayerArrayRemovedDelegate.IsBoundToObject(Delegate.GetUObject()))
    {
        OnPlayerArrayRemovedDelegate.Add(Delegate);
    }
}

void AShooterGameState::InitializeRandomAssetMaps()
{
    if (ShooterGI == nullptr) return;
    if (ShooterGI->TournamentParams.handicap == TEXT("OnlyGrenadeLaunch"))
    {
        WeaponIndexArray = {EWeaponType::EWT_GrenadeLauncher};
        AmmoIndexArray = {EAmmoType::EAT_GrenadeRounds, EAmmoType::EAT_HelicopterMissiles};
    }
    else if (ShooterGI->TournamentParams.handicap == TEXT("OnlyPistols"))
    {
        WeaponIndexArray = {EWeaponType::EWT_Pistol};
        AmmoIndexArray = {EAmmoType::EAT_9mm, EAmmoType::EAT_HelicopterMissiles};
    }
    else
    {
        WeaponIndexArray = {EWeaponType::EWT_Pistol, EWeaponType::EWT_AR, EWeaponType::EWT_SubmachineGun,
                        EWeaponType::EWT_Shotgun, EWeaponType::EWT_Sniper, EWeaponType::EWT_CyberPistol,
                        EWeaponType::EWT_GrenadeLauncher, EWeaponType::EWT_GravCannon};
        AmmoIndexArray = {EAmmoType::EAT_9mm, EAmmoType::EAT_45mm, EAmmoType::EAT_AR,
                        EAmmoType::EAT_Shells, EAmmoType::EAT_GrenadeRounds, EAmmoType::EAT_HelicopterMissiles,
                        EAmmoType::EAT_GravCharges};
    }

    float Exp = 2.71828f, ExpSum = 0, ExpFactor = 0, Ke = 0;
    TArray<float> ExpFactorArray;
    for (int32 i = 0; i < WeaponIndexArray.Num(); i++)
    {
        ExpSum += FMath::Pow(Exp, -ExpFactor);
        ExpFactorArray.Add(-ExpFactor);
        ExpFactor += 0.05; // Main factor used to determine rarity
    }
    Ke = 1 / ExpSum;

    TArray<int32> TempArray;
    // Create array that determines how probable it is to spawn a good/bad weapon.
    // i = 0 is worst weapon
    for (int32 i = 0; i < WeaponIndexArray.Num(); i++)
    {
        TempArray.Init(i, FGenericPlatformMath::RoundToInt32(1000 * Ke * FMath::Pow(Exp, ExpFactorArray[i])));
        WeaponProbabliltyArray += TempArray;
    }
    ShuffleIntArray(WeaponProbabliltyArray);

    RarityIndexArray = {EItemRarity::EIR_Damaged, EItemRarity::EIR_Common, EItemRarity::EIR_Uncommon,
                        EItemRarity::EIR_Rare, EItemRarity::EIR_Legendary};
    ExpSum = 0;
    ExpFactor = 0;
    TArray<float> ExpRarityFactorArray;
    for (int32 i = 0; i < RarityIndexArray.Num(); i++)
    {
        ExpSum += FMath::Pow(Exp, -ExpFactor);
        ExpRarityFactorArray.Add(-ExpFactor);
        ExpFactor += 0.05; // Main factor used to determine rarity
    }
    Ke = 1 / ExpSum;

    TArray<int32> RarityTempArray;
    for (int32 i = 0; i < RarityIndexArray.Num(); i++)
    {
        RarityTempArray.Init(i, FGenericPlatformMath::RoundToInt32(1000 * Ke * FMath::Pow(Exp, ExpRarityFactorArray[i])));
        RarityProbabliltyArray += RarityTempArray;
    }
    ShuffleIntArray(RarityProbabliltyArray);

    // Initialize the boost items
    BoostIndexArray = {EBoostType::EBT_ShieldV2, EBoostType::EBT_HealthV2, EBoostType::EBT_ShieldV1, EBoostType::EBT_HealthV1,
                    EBoostType::EBT_SuperPunch, EBoostType::EBT_SuperJump, EBoostType::EBT_Teleport,
                    EBoostType::EBT_Slow, EBoostType::EBT_Protect, EBoostType::EBT_Ghost, EBoostType::EBT_Copy};
    
    //auto ShooterGI = Cast<UShooterGameInstance>(GetGameInstance());
    //if (ShooterGI && !ShooterGI->bPracticeMode) BoostIndexArray.Add(EBoostType::EBT_Teleport);

    ExpSum = 0, ExpFactor = 0;
    TArray<float> ExpBoostFactorArray;
    for (int32 i = 0; i < BoostIndexArray.Num(); i++)
    {
        ExpSum += FMath::Pow(Exp, -ExpFactor);
        ExpBoostFactorArray.Add(-ExpFactor);
        ExpFactor += 0.05; // Main factor used to determine rarity
    }
    Ke = 1 / ExpSum;

    TArray<int32> BoostTempArray;
    // Create array that determines how probable it is to spawn a good/bad boost.
    // i = 0 is worst boost
    for (int32 i = 0; i < BoostIndexArray.Num(); i++)
    {
        BoostTempArray.Init(i, FGenericPlatformMath::RoundToInt32(1000 * Ke * FMath::Pow(Exp, ExpBoostFactorArray[i])));
        BoostProbabliltyArray += BoostTempArray;
    }
    ShuffleIntArray(BoostProbabliltyArray);
    bInitializedItemMaps = true;
}


EWeaponType AShooterGameState::GetRandomWeaponType()
{
    int32 RandomWeaponIndex = FMath::RandRange(0, WeaponProbabliltyArray.Num() - 1);
    return WeaponIndexArray[WeaponProbabliltyArray[RandomWeaponIndex]];
}

EItemRarity AShooterGameState::GetRandomRarityType()
{
    int32 RandomRarityIndex = FMath::RandRange(0, RarityProbabliltyArray.Num() - 1);
    return RarityIndexArray[RarityProbabliltyArray[RandomRarityIndex]];
}

EBoostType AShooterGameState::GetRandomBoostType()
{
    int32 RandomBoostIndex = FMath::RandRange(0, BoostProbabliltyArray.Num() - 1);
    return BoostIndexArray[BoostProbabliltyArray[RandomBoostIndex]];
}

EAmmoType AShooterGameState::GetRandomAmmoType()
{
    int32 RandomAmmoIndex = FMath::RandRange(0, AmmoIndexArray.Num() - 1);
    return AmmoIndexArray[RandomAmmoIndex];
}

void AShooterGameState::CollectAndPrepareStats(UWorld* World, FScoreCalcContext& Context)
{
    for (auto ShooterPS : ShooterPlayerArray)
    {
        if (!ShooterPS) continue;
        APlayerController* Controller = ShooterPS->GetPlayerController();

        // Ensure that player is still connected before processing stats
        UNetConnection* Conn = nullptr;
        if (Controller) Conn = Controller->GetNetConnection();
        if ((Conn && Conn->GetConnectionState() == EConnectionState::USOCK_Open) || ShooterPS->IsABot())
        {
            if (World)
                ShooterPS->PlayerGameStats.StopTimeMinutes = World->GetTimeSeconds() / 60.f;

            auto& Stats = ShooterPS->PlayerGameStats;
            Stats.KDR_raw = Stats.GetKDR(true);
            Stats.DTR_raw = Stats.GetDTR(true);
            Stats.Accuracy = Stats.GetAccuracy();

            float KDR = Stats.GetKDR(false);
            float DTR = Stats.GetDTR(false);
            float TimePlayed = Stats.GetTimeMinutesPlayed();

            Context.MaxKDR = FMath::Max(Context.MaxKDR, KDR);
            Context.MaxDTR = FMath::Max(Context.MaxDTR, DTR);
            Context.MaxTimePlayed = FMath::Max(Context.MaxTimePlayed, TimePlayed);
            Context.MaxDamageDealt = FMath::Max(Context.MaxDamageDealt, Stats.DamageDealt);

            Context.PlayerStatsArray.Add(&Stats);
        }
    }
}

void AShooterGameState::ComputeBaseScores(FScoreCalcContext& Context)
{
    for (auto* Stats : Context.PlayerStatsArray)
    {
        // A) Volume Score: Normalized damage dealt (rewards output)
        float VolumeScore = Context.MaxDamageDealt > 0.0f 
            ? Stats->DamageDealt / Context.MaxDamageDealt 
            : 0.0f;
        
        // B) Efficiency Score: Normalized damage efficiency (rewards smart play)
        float DTR = Stats->GetDTR(false);
        float EfficiencyScore = Context.MaxDTR > 0.0f 
            ? DTR / Context.MaxDTR 
            : 0.0f;
        
        // C) K/D Score: Normalized K/D ratio (rewards eliminations)
        float KDR = Stats->GetKDR(false);
        float KDRScore = Context.MaxKDR > 0.0f 
            ? FMath::Min(KDR / Context.MaxKDR, 1.0f) 
            : 0.0f;
        
        // D) Accuracy Score: Already 0-1 range
        float Accuracy = Stats->GetAccuracy();
        
        // Composite BaseScore with weights
        Stats->BaseScore = 
            0.35f * VolumeScore +
            0.30f * KDRScore +
            0.25f * EfficiencyScore +
            0.10f * Accuracy;
    }
    /*
    float KDRFactor = Context.MaxKDR == 0.f ? 0.f : 1.f / Context.MaxKDR;
    float DTRFactor = Context.MaxDTR == 0.f ? 0.f : 1.f / Context.MaxDTR;

    for (auto* Stats : Context.PlayerStatsArray)
    {
        float KDR_Norm = Stats->GetKDR(false) * KDRFactor;
        float DTR_Norm = Stats->GetDTR(false) * DTRFactor;
        float Accuracy = Stats->GetAccuracy();

        Stats->BaseScore = 0.5f * KDR_Norm + 0.35f * DTR_Norm + 0.15f * Accuracy;
    }
    */
}

void AShooterGameState::ComputeValorBoosts(FScoreCalcContext& Context)
{
    float MaxValor = 0.f;
    float MaxPenalty = 0.f;
    const float KillWeight = 150.f;  // Tune as needed; equivalent damage per kill

    // Initialize boosts and penalties to 0 for all players
    for (auto* Stats : Context.PlayerStatsArray)
    {
        Stats->ValorBoost = 0.f;
        Stats->ValorPenalty = 0.f;
    }

    // Single pass: Compute boosts for attackers and penalties for victims
    for (auto* Stats : Context.PlayerStatsArray)
    {
        float TotalUpsetKills = 0.f;  // **New: Track total upset kills instead of DamageDealt for averaging**

        for (const auto& Interaction : Stats->VictimInteractions)
        {
            AShooterPlayerState* VictimPS = Interaction.Victim;
            if (!VictimPS) continue;

            if (FPlayerGameStats* VictimStats = &VictimPS->PlayerGameStats)
            {
                if (VictimStats->BaseScore > Stats->BaseScore)
                {
                    float ScoreDiff = VictimStats->BaseScore - Stats->BaseScore;

                    // **Changed: Calculate upset impact only from kills**
                    float UpsetAmount = static_cast<float>(Interaction.Kills) * KillWeight * ScoreDiff;

                    if (UpsetAmount > 0.f)  // **Optional: Skip if no kills**
                    {
                        // Reward attacker
                        //UE_LOG(LogTemp, Warning, TEXT("Attacker Name = %s | Victim Name = %s | UpsetAmount = %f"), *Stats->PlayerName, *VictimStats->PlayerName, UpsetAmount);
                        Stats->ValorBoost += UpsetAmount;

                        // Penalize victim (use same amount; tune with a scalar if desired, e.g., 0.75f * UpsetAmount)
                        VictimStats->ValorPenalty += UpsetAmount;

                        TotalUpsetKills += static_cast<float>(Interaction.Kills);  // **New: Accumulate for averaging**
                    }
                }
            }
        }

        // **Changed: Average boost per upset kill (prevents volume farming on non-upsets; use TotalKills if preferred)**
        if (TotalUpsetKills > 0.f)
        {
            Stats->ValorBoost /= TotalUpsetKills;
            //UE_LOG(LogTemp, Warning, TEXT("Attacker Name = %s | Stats->ValorBoost %f"), *Stats->PlayerName, Stats->ValorBoost);
        }
    }

    for (auto* Stats : Context.PlayerStatsArray)
    {
        float TotalDeaths = static_cast<float>(Stats->NumOfDeaths);
        if (TotalDeaths > 0.f)
        {
            Stats->ValorPenalty /= TotalDeaths;
            //UE_LOG(LogTemp, Warning, TEXT("Attacker Name = %s | Stats->ValorPenalty %f"), *Stats->PlayerName, Stats->ValorPenalty);
        }
    }

    // Find maxes for normalization
    for (auto* Stats : Context.PlayerStatsArray)
    {
        MaxValor = FMath::Max(MaxValor, Stats->ValorBoost);
        MaxPenalty = FMath::Max(MaxPenalty, Stats->ValorPenalty);
    }

    // Normalize boosts
    if (MaxValor > 0.f)
    {
        for (auto* Stats : Context.PlayerStatsArray)
        {
            Stats->ValorNorm = Stats->ValorBoost / MaxValor;
        }
    }
    else
    {
        for (auto* Stats : Context.PlayerStatsArray)
        {
            Stats->ValorNorm = 0.f;
        }
    }

    // Normalize penalties
    if (MaxPenalty > 0.f)
    {
        for (auto* Stats : Context.PlayerStatsArray)
        {
            Stats->ValorPenaltyNorm = Stats->ValorPenalty / MaxPenalty;
        }
    }
    else
    {
        for (auto* Stats : Context.PlayerStatsArray)
        {
            Stats->ValorPenaltyNorm = 0.f;
        }
    }

    const float ValorWeight = 0.4f;
    const float PenaltyWeight = 0.15f;
    for (auto* Stats : Context.PlayerStatsArray)
    {
        Stats->NetValor = ValorWeight * Stats->ValorNorm - PenaltyWeight * Stats->ValorPenaltyNorm;
    }
}

void AShooterGameState::ComputeCombatFactors(FScoreCalcContext& Context)
{
    float MaxCombatFactor = 1.f;

    for (auto* Stats : Context.PlayerStatsArray)
    {
        float TimePlayed = Stats->GetTimeMinutesPlayed();
        float CombatRatio = (TimePlayed == 0.f) ? 0.f : FMath::Clamp(Stats->TimeInCombat / (TimePlayed * 60.f), 0.f, 1.f);
        float CombatFactor = 1.f + CombatRatio * Stats->BaseScore;

        MaxCombatFactor = FMath::Max(MaxCombatFactor, CombatFactor);
        Stats->CombatFactor = CombatFactor;
    }
    Context.MaxCombatFactor = MaxCombatFactor;
}

// This is used to rank player not based on individual raw metrics, but on graph node (PageRank method)
// Rewards who you beat
void AShooterGameState::ComputeGraphScores(FScoreCalcContext& Context)
{
    const float KillWeight = 150.f;  // Tune: Value of a kill in "damage units"
    const float Damping = 0.85f;     // Standard PageRank damping factor
    const int MaxIterations = 100;   // Enough for convergence in small graphs
    const float Tolerance = 1e-6f;   // Stop if changes < this
    const int NumPlayers = Context.PlayerStatsArray.Num();
    if (NumPlayers == 0) return;

    // Map players to indices (0 to NumPlayers-1)
    TMap<FPlayerGameStats*, int> PlayerToIndex;
    for (int i = 0; i < NumPlayers; ++i)
    {
        PlayerToIndex.Add(Context.PlayerStatsArray[i], i);
    }

    // Build adjacency matrix for REVERSE graph (victim -> killer, weight = damage + kills*weight)
    // adj[i][j] = weight from i (victim index) to j (killer index)
    TArray<TArray<float>> AdjReverse;
    AdjReverse.SetNum(NumPlayers);
    for (auto& Row : AdjReverse) Row.SetNumZeroed(NumPlayers);

    // Out-degrees for reverse graph (for normalization)
    TArray<float> OutDegreesReverse;
    OutDegreesReverse.SetNumZeroed(NumPlayers);

    for (auto* Stats : Context.PlayerStatsArray)
    {
        int KillerIdx = PlayerToIndex[Stats];
        for (const auto& Interaction : Stats->VictimInteractions)
        {
            AShooterPlayerState* VictimPS = Interaction.Victim;
            if (!VictimPS) continue;
            FPlayerGameStats* VictimStats = &VictimPS->PlayerGameStats;
            int VictimIdx = PlayerToIndex.FindRef(VictimStats);

            float Weight = Interaction.DamageDealt + static_cast<float>(Interaction.Kills) * KillWeight;
            if (Weight > 0.f)
            {
                // Reverse: victim -> killer
                AdjReverse[VictimIdx][KillerIdx] += Weight;
                OutDegreesReverse[VictimIdx] += Weight;
            }
        }
    }

    // Power iteration for PageRank on reverse graph
    TArray<float> Scores;
    Scores.SetNumZeroed(NumPlayers);
    for (auto& S : Scores) S = 1.f / NumPlayers;  // Initial uniform

    TArray<float> NewScores;
    NewScores.SetNumZeroed(NumPlayers);

    for (int Iter = 0; Iter < MaxIterations; ++Iter)
    {
        float MaxDelta = 0.f;
        float SumNew = 0.f;

        for (int i = 0; i < NumPlayers; ++i)
        {
            NewScores[i] = (1.f - Damping) / NumPlayers;  // Teleportation
            for (int j = 0; j < NumPlayers; ++j)
            {
                if (OutDegreesReverse[j] > 0.f)
                {
                    NewScores[i] += Damping * Scores[j] * (AdjReverse[j][i] / OutDegreesReverse[j]);
                }
            }
            MaxDelta = FMath::Max(MaxDelta, FMath::Abs(NewScores[i] - Scores[i]));
            SumNew += NewScores[i];
        }

        // Normalize to sum=1 (optional but stabilizes)
        if (SumNew > 0.f)
        {
            for (auto& NS : NewScores) NS /= SumNew;
        }

        Scores = NewScores;

        if (MaxDelta < Tolerance) break;  // Converged
    }

    // Post-process: Force 0 for players with no inflicted activity (damage dealt or kills)
    for (int i = 0; i < NumPlayers; ++i)
    {
        auto* Stats = Context.PlayerStatsArray[i];
        float TotalDealt = Stats->DamageDealt + static_cast<float>(Stats->NumOfKills) * KillWeight;  // Assume Stats->Kills exists
        if (TotalDealt == 0.f)
        {
            Scores[i] = 0.f;
        }
    }

    // Assign to stats (0-1, pre-normalization)
    for (int i = 0; i < NumPlayers; ++i)
    {
        Context.PlayerStatsArray[i]->GraphScore = Scores[i];
    }
}

void AShooterGameState::ApplyFinalScores(FScoreCalcContext& Context)
{
    float MaxCombatFactor = Context.MaxCombatFactor;
    float TimePlayedFactor = Context.MaxTimePlayed == 0.f ? 1.f : 1.f / Context.MaxTimePlayed;
    const float LowBound = 0.8f;  // Tune: e.g., 0.7 for stronger effects
    const float RewardCap = 1 - LowBound;

    // Find min/max NetValor
    float MinNet = MAX_FLT;
    float MaxNet = -MAX_FLT;
    for (auto* Stats : Context.PlayerStatsArray)
    {
        MinNet = FMath::Min(MinNet, Stats->NetValor);
        MaxNet = FMath::Max(MaxNet, Stats->NetValor);
    }

    for (auto* Stats : Context.PlayerStatsArray)
    {
        float TimePlayed_Norm = Context.bUseTimeFactor ? Stats->GetTimeMinutesPlayed() * TimePlayedFactor : 1.f;

        float ValorMultiplier = 1.f;  // Neutral baseline

        if (MaxNet > 0.f && Stats->NetValor > 0.f)
        {
            float ScaledBoost = Stats->NetValor / MaxNet;  // 0-1, 1=best positive
            ValorMultiplier = 1.f - RewardCap + RewardCap * ScaledBoost;  // E.g., [0.8, 1.0] for varying positives
        }
        else if (MinNet < 0.f && Stats->NetValor < 0.f)
        {
            float ScaledPenalty = FMath::Abs(Stats->NetValor) / FMath::Abs(MinNet);  // 0-1
            ValorMultiplier = 1.f - (1.f - LowBound) * ScaledPenalty;  // [LowBound, 1.0)
        }

        Stats->Score = 100.f * Stats->BaseScore * ValorMultiplier * (Stats->CombatFactor / MaxCombatFactor) * FMath::Sqrt(TimePlayed_Norm);
        //UE_LOG(LogTemp, Warning, TEXT("Player = %s | ValorMultiplier = %f | BaseScore = %f | Final Score = %f"), *Stats->PlayerName, ValorMultiplier, Stats->BaseScore, Stats->Score);
        // If using PageRank method
        //float NormGraph = Stats->GraphScore * GraphNormFactor;  // 0-1
        //Stats->Score = 100.f * NormGraph * (Stats->CombatFactor / MaxCombatFactor) * FMath::Sqrt(TimePlayed_Norm);
    }
}

TArray<FPlayerGameStats> AShooterGameState::GetSortedStatValues(FScoreCalcContext& Context)
{
    Context.PlayerStatsArray.Sort([](const FPlayerGameStats& A, const FPlayerGameStats& B) {
        return A.Score > B.Score;
    });
    
    // Assign ranks to the original structs via pointers
    for (int32 i = 0; i < Context.PlayerStatsArray.Num(); ++i)
    {
        if (Context.PlayerStatsArray[i])
        {
            Context.PlayerStatsArray[i]->MatchRank = i + 1;
        }
    }
    
    // Now create the value array from the sorted pointers
    TArray<FPlayerGameStats> PlayerStatsValueArray;
    PlayerStatsValueArray.Reserve(Context.PlayerStatsArray.Num());
    
    for (FPlayerGameStats* StatPtr : Context.PlayerStatsArray)
    {
        if (StatPtr)
        {
            PlayerStatsValueArray.Add(*StatPtr);
        }
    }
    
    return PlayerStatsValueArray;
}

TArray<FPlayerGameStats> AShooterGameState::GetRankedPlayerScores()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    UWorld* World = GetWorld();
    if (!ShooterGI || !World) TArray<FPlayerGameStats>();
    
    FScoreCalcContext Context;

    switch (ShooterGI->GameType)
    {
    case EGameModeType::EGMT_tournament:
    case EGameModeType::EGMT_tournamentTeam:
        Context.bUseTimeFactor = true;
        Context.bIncludeAIInScore = false;
        break;
    
    default:
        Context.bUseTimeFactor = false;
        break;
    }

    CollectAndPrepareStats(World, Context);
    ComputeBaseScores(Context);
    ComputeValorBoosts(Context);
    ComputeCombatFactors(Context);
    ApplyFinalScores(Context);
    return GetSortedStatValues(Context);
}

void AShooterGameState::GetPings()
{
    for (auto PS : ShooterPlayerArray)
    {
        if (PS)
        {
            PS->PlayerGameStats.PingMs = !PS->IsABot() ? 
                PS->GetCompressedPing() * 4 : FMath::RandRange(0.f, 75.f);
        }  
    }
}

void AShooterGameState::StartLobbyMatch(const FString& Region)
{
	UWorld* World = GetWorld();
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (HasAuthority() && World && ShooterGI)
	{
		bStartingMatchFromLobby = true;
		LobbyServerInfo.Region = Region;
		LobbyServerInfo.GameSessionId = TEXT("");
        switch (ShooterGI->SelectedGameType)
        {
        case EGameModeType::EGMT_freeTeam:
            LobbyServerInfo.setTeamGUID(ShooterGI->SelectedGameType);
            break;
        default:
            break;
        }
        LobbyServerInfo.GameType = ShooterGI->SelectedGameType;
		StartGettingServerInfoLobby();
        MulticastUpdateTravelStatus(TEXT("Searching"));
	}
}

void AShooterGameState::UpdateLobbyIdArray(bool bIsReady, const FString& ID)
{
	if (!bIsReady)
	{
        if (LobbyIdArray.Contains(ID))
        {
            LobbyIdArray.Remove(ID);
            OnRep_LobbyIdArray();
            if (LobbyIdArray.Num() == 0 && bStartingMatchFromLobby)
            {
                GetWorldTimerManager().ClearTimer(BackupLobbyTravelTimer);
                UE_LOG(LogTemp, Warning, TEXT("Start Server Timer For Travel"));
                FTimerHandle CheckTravelTimer;
                GetWorldTimerManager().SetTimer(CheckTravelTimer, this, &AShooterGameState::StartSendingServer, 2.f);
                //SendPlayerFromLobby();
            }
        }
	}
    else
    {
        LobbyIdArray.Add(ID);
        OnRep_LobbyIdArray();
    }
}

void AShooterGameState::OnRep_LobbyIdArray()
{
	UWorld* World = GetWorld();
	if (World)
	{
		auto LocalShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
		if (LocalShooterPC && LocalShooterPC->PauseMenuWidget && LobbyServerInfo.GameSessionId.IsEmpty())
		{
			LocalShooterPC->UpdateLobbyReady(LobbyIdArray);
		}
	}
}

void AShooterGameState::OnRep_LobbyServerInfo()
{
	UWorld* World = GetWorld();
	if (World && !LobbyServerInfo.GameSessionId.IsEmpty())
	{
		if (auto LocalShooterGS = Cast<AShooterGameState>(UGameplayStatics::GetGameState(World)))
		{
			const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
			if (LocalShooterGS->LobbyIdArray.Contains(Ids.PlayerID))
			{
                LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Sending player %s with %s / %s"), *Ids.PlayerID, *LobbyServerInfo.TeamGUID, *UEnum::GetValueAsString(LobbyServerInfo.GameType)));
				LocalShooterGS->StartGettingServerInfoLobby();
			}
		}
	}
}

void AShooterGameState::StartGettingServerInfoLobby()
{
	FTournamentParams TourParams;
	IGeneralInterface::Execute_UpdateTournamentParams(UGameplayStatics::GetGameInstance(this), TourParams);

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI == nullptr) return;
    if (ShooterGI->GameType != EGameModeType::EGMT_Lobby) return;
    const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
    HTTPRequestObj = NewObject<URequestsObject>(this);
    if (HTTPRequestObj)
    {
		if (!HTTPRequestObj->OnRequestResponseDelegate.IsBound())
		{
			HTTPRequestObj->OnRequestResponseDelegate.AddDynamic(this, &AShooterGameState::PostResponseReceived);
		}
        FKeyValue Params;
        switch (LobbyServerInfo.GameType)
        {
        case EGameModeType::EGMT_freeSolo:
            Params.SetStringField(TEXT("mode"), TEXT("solo"));
            break;
        case EGameModeType::EGMT_freeTeam:
            Params.SetStringField(TEXT("mode"), TEXT("team"));
            break;
        
        default:
            break;
        }

        UE_LOG(LogTemp, Warning, TEXT("StartGettingServerInfoLobby GameSessionId = %s"), *LobbyServerInfo.GameSessionId);
        Params.SetStringField(TEXT("type"), TEXT("free"));
        Params.SetStringField(TEXT("tournamentId"), TEXT(""));
        Params.SetStringField(TEXT("region"), LobbyServerInfo.Region);
        Params.SetStringField(TEXT("playerName"), Ids.LocalGameLiftID);
        Params.SetStringField(TEXT("gameSessionId"), LobbyServerInfo.GameSessionId);
        Params.SetStringField(TEXT("teamGUID"), LobbyServerInfo.TeamGUID);
        Params.SetIntegerField(TEXT("numLobbyPlayers"), LobbyIdArray.Num() + 1);
        Params.SetBoolField(TEXT("isFromLobby"), true);
        Params.AddHeader(TEXT("clientToken"), Ids.GameAccessJWT);
        HTTPRequestObj->MakeRequest(Params, TEXT("gamelift/createSession"), ERequestType::ERT_POST, EURLType::EUT_Main);
    }
}

void AShooterGameState::ParseLobbyMatch(FString DataStr)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (ShooterGI == nullptr) return;
    FLobbyInfo NewLobbyInfo;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("result")))
        {
            TSharedPtr<FJsonObject> ResultData = JsonObject->GetObjectField(TEXT("result"));
            if (ResultData->HasField(TEXT("serverInfo")))
            {
                TSharedPtr<FJsonObject> ServerData = ResultData->GetObjectField(TEXT("serverInfo"));
                if (ServerData->HasField(TEXT("status")))
                {
                    FString ServerStatus = ServerData->GetStringField(TEXT("status"));
					if (HasAuthority()) MulticastUpdateTravelStatus(ServerStatus);

                    if (ServerStatus.Equals("Searching") || ServerStatus.Equals("Placing") || ServerStatus.Equals("Queued"))
                    {
						FTimerHandle LobbyServerTimer;
						GetWorldTimerManager().SetTimer(LobbyServerTimer, this, &AShooterGameState::StartGettingServerInfoLobby, 3.f);
                    }
                    else if (ServerStatus.Equals("Completed"))
                    {
						FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
						ShooterGI->TargetGameSessionId = ServerData->GetStringField(TEXT("gameSessionId"));
						Ids.PlayerSessionID = ServerData->GetStringField(TEXT("playerSessionId"));
						IGeneralInterface::Execute_UpdatePlayerIds(UGameplayStatics::GetGameInstance(this), Ids);
						
                        if (HasAuthority())
                        {
                            FString TeamGUID = LobbyServerInfo.TeamGUID;
							LobbyServerInfo = FLobbyInfo();
                            NewLobbyInfo.GameType = ShooterGI->SelectedGameType;
                            NewLobbyInfo.IpAddress = ServerData->GetStringField(TEXT("ip"));
                            NewLobbyInfo.Port = ServerData->GetIntegerField(TEXT("port"));
                            NewLobbyInfo.GameSessionId = ServerData->GetStringField(TEXT("gameSessionId"));
                            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("SERVER TeamGUID = %s | GameSession Id = %s"), *TeamGUID, *NewLobbyInfo.GameSessionId));
                            NewLobbyInfo.Region = LobbyServerInfo.Region;
                            NewLobbyInfo.TeamGUID = TeamGUID;
                            LobbyServerInfo = NewLobbyInfo;
							// Backup timer in case the LobbyIdArray is not updated correctly
							GetWorldTimerManager().SetTimer(BackupLobbyTravelTimer, this, &AShooterGameState::SendPlayerFromLobby, 10.f);
                        }
                        else
                        {
							SendPlayerFromLobby();
                        }
                    }
                    else if (ServerStatus.Equals("Failed"))
                    {
						if (HasAuthority()) bStartingMatchFromLobby = false;
                    }
                }
                else
                {
                    if (HasAuthority())
                    {
                        bStartingMatchFromLobby = false;
                        MulticastUpdateTravelStatus(TEXT("Failed"));
                    }
                }
            }
        }
    }
}

void AShooterGameState::MulticastUpdateTravelStatus_Implementation(const FString& BackendStatus)
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (auto LocalShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
		{
			LocalShooterPC->UpdateLobbyStatus(BackendStatus);
		}
	}
}

void AShooterGameState::SendPlayerFromLobby()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (auto LocalShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
		{
			LocalShooterPC->SendPlayerFromLobby(LobbyServerInfo);
		}
	}
}

void AShooterGameState::StartSendingServer()
{
	UE_LOG(LogTemp, Warning, TEXT("StartSendingServer"));
	SendPlayerFromLobby();
}

void AShooterGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bMissileWarning);
    //DOREPLIFETIME(ThisClass, MissileTimeline);
    DOREPLIFETIME(ThisClass, LocationIndex);
    DOREPLIFETIME(ThisClass, MissileStartLocation);
    DOREPLIFETIME(ThisClass, MissileEndLocation);
    DOREPLIFETIME(ThisClass, MissileHitLocations);
    DOREPLIFETIME(ThisClass, ServerFPS);
    DOREPLIFETIME(ThisClass, GameMaxPlayers);
    DOREPLIFETIME(ThisClass, DroneActor);
    //DOREPLIFETIME(ThisClass, ShooterPlayerArray);
    //DOREPLIFETIME(ThisClass, PlayersStillAlive);
    DOREPLIFETIME(ThisClass, GameMatchState);
    //DOREPLIFETIME(ThisClass, bSpawnedDebris);
    DOREPLIFETIME(ThisClass, bIsTeamMode);
    //DOREPLIFETIME(ThisClass, PathActor);
	DOREPLIFETIME(ThisClass, LobbyIdArray);
	DOREPLIFETIME(ThisClass, LobbyServerInfo);
    DOREPLIFETIME(ThisClass, HostPlayerId);
    DOREPLIFETIME(ThisClass, ShooterControllerArray);
    DOREPLIFETIME(ThisClass, PartyId);
}

