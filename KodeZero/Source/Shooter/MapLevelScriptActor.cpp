// Fill out your copyright notice in the Description page of Project Settings.


#include "MapLevelScriptActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MissleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


AMapLevelScriptActor::AMapLevelScriptActor()
{
    PrimaryActorTick.bCanEverTick = false;

    //InitializeMissileHitLocations();

}

void AMapLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();
    //MissleFireTimer();
}

// Called every frame
void AMapLevelScriptActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMapLevelScriptActor::MissleFireTimer()
{
    if (HasAuthority())
    {
        GetWorldTimerManager().SetTimer(MissleSpawnTimer, this, &AMapLevelScriptActor::ServerFireMissle, MissleSpawnDelay);
    }
}

void AMapLevelScriptActor::ServerFireMissle_Implementation()
{
    if (MissileStartLocation.Num() <= 0) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    //SpawnParams.Instigator = InstigatorPawn;
    //SetInstigator(InstigatorPawn);
    LocationIndex = FMath::RandRange(0, MissileStartLocation.Num() - 1);

    FVector SpawnLocation{ MissileStartLocation[LocationIndex] };
    FVector TargetLocation{ MissileEndLocation[LocationIndex] };
    FRotator TargetRotation = (TargetLocation - SpawnLocation).Rotation();

    if (MissleClass)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            ProjectileActor = World->SpawnActor<AProjectile>(
                MissleClass,
                SpawnLocation,
                TargetRotation,
                SpawnParams
            );
            MissileActor = Cast<AMissleSystem>(ProjectileActor);
            MissileStartLocation.RemoveAt(LocationIndex);
            MissileEndLocation.RemoveAt(LocationIndex);
        }
    }

    //MissleSpawnDelay = 1000.f;
    MissleFireTimer();  
}

void AMapLevelScriptActor::InitializeMissileHitLocations()
{
    MissileStartLocation.SetNum(3);
    MissileEndLocation.SetNum(3);

    MissileStartLocation[0] = {-700.f, 120.f, 6520.f};
    MissileEndLocation[0] = {-200.f, 120.f, -20.f};

    MissileStartLocation[1] = {-700.f, 1060.f, 6520.f};
    MissileEndLocation[1] = {-200.f, 1060.f, -20.f};

    MissileStartLocation[2] = {-700.f, 2030.f, 6520.f};
    MissileEndLocation[2] = {-200.f, 2030.f, -20.f};
}

/*
void AMapLevelScriptActor::SetupFireEffectArrays()
{
    ParticleSpawnArray.SetNum(3);
    ParticleSpawnArray[0] = FireLocation0;
    ParticleSpawnArray[1] = FireLocation1;
    ParticleSpawnArray[2] = FireLocation2;

    SmokeSpawnArray.SetNum(3);
    SmokeSpawnArray[0] = SmokeLocation0;
    SmokeSpawnArray[1] = SmokeLocation1;
    SmokeSpawnArray[2] = SmokeLocation2;
}

void AMapLevelScriptActor::InitializeFireEffectLocations()
{
    // TArray<FVector> FireLocation0;
    FireLocation0 = { {520.f, -40.f, 130.f}, {520.f, -260.f, 130.f} };
    SmokeLocation0 = { {520.f, -40.f, 130.f}, {520.f, -260.f, 130.f} };

    FireLocation1 = { {520.f, 850.f, 100.f}, {520.f, 1010.f, 100.f} };
    SmokeLocation1 = { {520.f, 850.f, 100.f}, {520.f, 1010.f, 100.f} };

    FireLocation2 = { {520.f, 1760.f, 100.f}, {520.f, 2240.f, 100.f} };
    SmokeLocation2 = { {520.f, 1760.f, 100.f}, {520.f, 2240.f, 100.f} };

}

void AMapLevelScriptActor::UpdateEffectLocations()
{
    ParticleSpawnArray.RemoveAt(LocationIndex);
    SmokeSpawnArray.RemoveAt(LocationIndex);
    //MissileStartLocation.RemoveAt(LocationIndex);
    //MissileEndLocation.RemoveAt(LocationIndex);
}
*/