// Fill out your copyright notice in the Description page of Project Settings.

#include "MissleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "ShooterCharacter.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Shooter/GameMode/ShooterGameMode.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
//#include "Shooter/MapLevelScriptActor.h"
#include "Engine/TargetPoint.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "Helicopter.h"
#include "Item.h"
#include "Shooter/Drone/GroundContainer.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"

AMissleSystem::AMissleSystem()
{
    //PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
	SetReplicatingMovement(true);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Missle Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);

    /*
    DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
	DamageBox->SetupAttachment(GetRootComponent());
    DamageBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DamageBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    DamageBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    DamageBox->SetGenerateOverlapEvents(true);
    */

    DamageBoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DamageBoxMesh"));
	DamageBoxMesh->SetupAttachment(GetRootComponent());
    DamageBoxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DamageBoxMesh->SetGenerateOverlapEvents(true);
    
    ThrustComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ThrustComponent"));
    ThrustComponent->SetupAttachment(GetRootComponent());
}

void AMissleSystem::BeginPlay()
{
	Super::BeginPlay();
    //LevelScriptActor = Cast<AMapLevelScriptActor>(GetWorld()->GetLevelScriptActor());
    //if (LevelScriptActor == nullptr) return;
    
    /*
    DamageBox->SetWorldRotation({ 0.f, 0.f, 0.f});
    DamageBox->SetWorldScale3D({650.f, 500.f, 800.f});
    DamageBox->SetWorldLocation({DamageBox->GetComponentLocation().X, DamageBox->GetComponentLocation().Y, DamageBox->GetComponentLocation().Z + 400.f*50.f});
    */

    DamageBoxMesh->SetWorldRotation({ 0.f, 0.f, 0.f});
    DamageBoxMesh->SetWorldScale3D({416.f, 320.f, 560.f});
    DamageBoxMesh->SetWorldLocation({DamageBoxMesh->GetComponentLocation().X, DamageBoxMesh->GetComponentLocation().Y, DamageBoxMesh->GetComponentLocation().Z + (560.f/2.f)*50.f});
    DamageBoxMesh->OnComponentBeginOverlap.AddDynamic(this, &AMissleSystem::OnDamageBoxOverlap);
    DamageBoxMesh->OnComponentEndOverlap.AddDynamic(this, &AMissleSystem::OnDamageBoxEndOverlap);
    SetHitProperties();
    
	SpawnTrailSystem();
    UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
    bMissileLaunched = true;
}

void AMissleSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMissleSystem::SetProjectileVelocity(const FVector& Direction, float Speed)
{
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = Speed;
		ProjectileMovementComponent->MaxSpeed = Speed;
		ProjectileMovementComponent->Velocity = Speed * Direction;
	}
}

void AMissleSystem::SetCollisions()
{
    if (DamageBoxMesh)
    {
        DamageBoxMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
        DamageBoxMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
        DamageBoxMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    }
}

void AMissleSystem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (HasAuthority())
    {
        bMissleHit = true;
        OnRep_MissleHit();
        MulticastMissleHit();
        RemoveShooterSpawnLocation();
        DamageSphereEvent();
        //DamageBoxMesh->Bounds.GetBox().IsInside()
        //FTimerHandle DestroyActorsTimer;
        //GetWorldTimerManager().SetTimer(DestroyActorsTimer, this, &AMissleSystem::DestroyActorsAfterHit, 90.f);
        UE_LOG(LogTemp, Warning, TEXT("Missile On Hit"));
    }
}

void AMissleSystem::RemoveShooterSpawnLocation()
{
    AShooterGameMode* GM = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM && DamageBoxMesh)
    {
        GM->UpdateMissileHits();
        const TArray<FVector> SpawnLocations = GM->SpawnLocationPoints;
        for (auto ShooterSpawnLocation : SpawnLocations)
        {
            if (DamageBoxMesh->Bounds.GetBox().IsInside(ShooterSpawnLocation))
            {
                //UE_LOG(LogTemp, Warning, TEXT("Removing Spawn Location: %s"), *ShooterSpawnLocation.ToString());
                GM->SpawnLocationPoints.Remove(ShooterSpawnLocation);
            }
        }
    }
}

void AMissleSystem::MulticastMissleHit_Implementation()
{
    //if (ImpactParticles)
    //{
    //	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
    //}
    if (HasAuthority()) OnHitDamage();
    float NukeScale = 1.f;
    if (ImpactEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            ImpactEffect,
            GetRootComponent(),
            FName(),
            GetActorLocation(),
            {0.f, FMath::RandRange(0.f, 360.f), 0.f},
            {NukeScale, NukeScale, NukeScale},
            EAttachLocation::KeepWorldPosition,
            true,
            ENCPoolMethod::None
        );
    }

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
    }
}

void AMissleSystem::DestroyActorsAfterHit()
{
    DamageBoxMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
    TArray<AActor*> OverlappingItems;
    DamageBoxMesh->GetOverlappingActors(OverlappingItems);

    if (!bMissleHit) return;
    if (OverlappingItems.Num() > 0)
    {   
        for (int32 i = 0; i < OverlappingItems.Num(); i++)
        {
            FString OverlapActorName = OverlappingItems[i]->GetName();
            UE_LOG(LogTemp, Warning, TEXT("Overlapping with: %s"), *OverlapActorName);
            bool bActorsToDestroy = OverlapActorName.Contains("BaseWeapon") ||
                    OverlapActorName.Contains("Ammo") || OverlapActorName.Contains("BoostItem");
            if (bActorsToDestroy)
            {
                auto ItemToDestroy = Cast<AItem>(OverlappingItems[i]);
                if (ItemToDestroy && ItemToDestroy->GetItemState() == EItemState::EIS_Pickup)
                {
                    ItemToDestroy->Destroy();
                    //MulticastDestroyOverlappedActor(OverlappingActors[i]);
                }
            }
            else if (OverlapActorName.Contains("GroundContainer"))
            {

            }
        }
    }

    DamageBoxMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
    FTimerHandle DestroyActorsTimer;
    GetWorldTimerManager().SetTimer(DestroyActorsTimer, this, &AMissleSystem::DestroyActorsAfterHit, 60.f);
}

void AMissleSystem::MulticastDestroyOverlappedActor_Implementation(AActor* ActorToDestroy)
{
    //if (HasAuthority()) return;
    ActorToDestroy->Destroy();
}

void AMissleSystem::SetHitProperties()
{
    if (DamageBoxMesh && ThrustComponent && ProjectileMesh)
    {
        if (bMissleHit)
        {
            SetCollisions();
            ProjectileMesh->SetVisibility(false);
            ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            DamageBoxMesh->SetVisibility(true);
            ThrustComponent->Stop();
        }
        else
        {
            ProjectileMesh->SetVisibility(true);
            DamageBoxMesh->SetVisibility(false);
            ThrustComponent->Play();
        }
    }
}

void AMissleSystem::OnRep_MissleHit()
{
    SetHitProperties();
}

void AMissleSystem::OnDamageBoxOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
    if (bMissleHit)
    {
        bOverlappingDamageBox = true;
        auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
        if (ShooterCharacter)
        {
            ShooterCharacter->InMissileDamageBoxCount += 1;
            OverlapBoxEffects(ShooterCharacter);
        }
    }
}

void AMissleSystem::OnDamageBoxEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (bMissleHit)
    {
        bOverlappingDamageBox = false;
        auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
        if (ShooterCharacter)
        {
            ShooterCharacter->InMissileDamageBoxCount -= 1;
            if (ShooterCharacter->InMissileDamageBoxCount == 0)
            {
                OverlapBoxEffects(ShooterCharacter);
            }
        }
    }
}

void AMissleSystem::OverlapBoxEffects(AShooterCharacter* CharacterInBox)
{
    if (CharacterInBox && CharacterInBox->bIsAI) return;
    AShooterPlayerController* ShooterController = nullptr;
    if (CharacterInBox->OwnedPawn)
    {
        ShooterController = Cast<AShooterPlayerController>(CharacterInBox->OwnedPawn->Controller);
        if (!CharacterInBox->OwnedPawn->IsLocallyControlled()) return;
    }
    else
    {
        ShooterController = Cast<AShooterPlayerController>(CharacterInBox->Controller);
        if (!CharacterInBox->IsLocallyControlled()) return;
    }

    FVector ActorLoc;
    FRotator ActorRot;
    if (OverlapBoxSound && (CharacterInBox->InMissileDamageBoxCount == 0 || CharacterInBox->InMissileDamageBoxCount == 1))
    {
        if (CharacterInBox->OwnedPawn)
        {
            ActorLoc = CharacterInBox->OwnedPawn->GetActorLocation();
            ActorRot = CharacterInBox->OwnedPawn->GetActorRotation();
        }
        else
        {
            ActorLoc = CharacterInBox->GetActorLocation();
            ActorRot = CharacterInBox->GetActorRotation();
        }

        UGameplayStatics::SpawnSoundAtLocation(
            GetWorld(),
            OverlapBoxSound,
            ActorLoc,
            ActorRot,
            1.0f, // volume multiplier
            1.0f, // pitch multiplier
            0.0f, // start time
            nullptr, //USoundAttenuation
            nullptr,
            true
        );
    }

    if (CharacterInBox->InMissileDamageBoxCount > 0)
    {
        if (ShooterController && ShooterController->bMatchEnded) return;
        CharacterInBox->bInMissileDamageBox = true;
        if (CharacterInBox->OwnedPawn)
        {
            AHelicopter* OwnedHeli= Cast<AHelicopter>(CharacterInBox->OwnedPawn);
            if (OwnedHeli)
            {
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_ColorGain = true;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.ColorGain.Set(1.3f, 0.4f, 0.4f, 1.f);
                /*
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_ColorSaturation = true;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTemp = true;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTint = true;
                /OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_AutoExposureBias = true;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.ColorSaturation.Set(1.1f, 0.9f, 0.9f, 1.f);
                //OwnedHeli->GetFollowCamera()->PostProcessSettings.ColorSaturation = {4.f, 0.25f, 0.25f, 1.f};
                OwnedHeli->GetFollowCamera()->PostProcessSettings.WhiteTemp = 6700.f;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.WhiteTint = 0.1f;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.AutoExposureBias = -0.2f;
                */
            }
        }
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_ColorGain = true;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.ColorGain.Set(1.3f, 0.4f, 0.4f, 1.f);
        /*
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_ColorSaturation = true;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTemp = true;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTint = true;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_AutoExposureBias = true;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.ColorSaturation.Set(1.1f, 0.9f, 0.9f, 1.f);
        CharacterInBox->GetFollowCamera()->PostProcessSettings.WhiteTemp = 6700.f;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.WhiteTint = 0.1f;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.AutoExposureBias = -0.2f;
        */

        if (!CharacterInBox->GetMissileDamageEffectComp()->IsActive())
        {
            CharacterInBox->GetMissileDamageEffectComp()->Activate();
        }

        if (ShooterController) ShooterController->PlayMissileDamageBoxSound(true);
    }
    else
    {
        CharacterInBox->bInMissileDamageBox = false;
        if (CharacterInBox->OwnedPawn)
        {
            AHelicopter* OwnedHeli= Cast<AHelicopter>(CharacterInBox->OwnedPawn);
            if (OwnedHeli)
            {
                OwnedHeli->GetFollowCamera()->PostProcessSettings.ColorGain.Set(1.f, 1.f, 1.f, 1.f);
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_ColorGain = false;
                /*
                OwnedHeli->GetFollowCamera()->PostProcessSettings.ColorSaturation.Set(1.f, 1.f, 1.f, 1.f);
                OwnedHeli->GetFollowCamera()->PostProcessSettings.WhiteTemp = 6500.f;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.WhiteTint = 0.f;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_AutoExposureBias = false;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_ColorSaturation = false;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTemp = false;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTint = false;
                OwnedHeli->GetFollowCamera()->PostProcessSettings.AutoExposureBias = 0.0f;
                */
            }
        }
        CharacterInBox->GetFollowCamera()->PostProcessSettings.ColorGain.Set(1.f, 1.f, 1.f, 1.f);
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_ColorGain = false;
        /*
        CharacterInBox->GetFollowCamera()->PostProcessSettings.ColorSaturation.Set(1.f, 1.f, 1.f, 1.f);
        CharacterInBox->GetFollowCamera()->PostProcessSettings.WhiteTemp = 6500.f;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.WhiteTint = 0.f;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_AutoExposureBias = false;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_ColorSaturation = false;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTemp = false;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.bOverride_WhiteTint = false;
        CharacterInBox->GetFollowCamera()->PostProcessSettings.AutoExposureBias = 0.0f;
        */

        if (CharacterInBox->GetMissileDamageEffectComp()->IsActive())
        {
            CharacterInBox->GetMissileDamageEffectComp()->Deactivate();
        }

        if (ShooterController) ShooterController->PlayMissileDamageBoxSound(false);
    }

}

void AMissleSystem::DamageSphereEvent()
{
    if (bMissleHit)
    {
        GetWorldTimerManager().SetTimer(DamageTimer, this, &AMissleSystem::ApplyFireDamage, DamageDelay);
    }
}

void AMissleSystem::OnHitDamage()
{
    if (ShooterCharacterClass == nullptr) return;
    TArray<AActor*> OverlappingCharacters;
    DamageBoxMesh->GetOverlappingActors(OverlappingCharacters, ShooterCharacterClass);
    if (bMissleHit && OverlappingCharacters.Num() > 0)
    {   
        for (int32 i = 0; i < OverlappingCharacters.Num(); i++)
        {
            if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(OverlappingCharacters[i]))
            {
                ShooterInterface->Execute_OnMissileHit(OverlappingCharacters[i], this, 25.f);
            }
        }
    } 
}

void AMissleSystem::ApplyFireDamage()
{
    if (ShooterCharacterClass == nullptr) return;
    TArray<AActor*> OverlappingCharacters;
    DamageBoxMesh->GetOverlappingActors(OverlappingCharacters, ShooterCharacterClass);
    if (bMissleHit && OverlappingCharacters.Num() > 0)
    {   
        for (int32 i = 0; i < OverlappingCharacters.Num(); i++)
        {
            if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(OverlappingCharacters[i]))
            {
                ShooterInterface->Execute_OnMissileHit(OverlappingCharacters[i], this, 1.f);
            }
        }
    }
    DamageSphereEvent();
}

// Call in OnHit
void AMissleSystem::SpawnLastingEffects()
{
    /*
    //UE_LOG(LogTemp, Warning, TEXT("Location in actor = %i"), ParticleSpawnArray.Num());
    for (int32 i = 0; i < 50; i++)
    {
        int32 FireEffectIndex = FMath::RandRange(0, 2);
        //FVector Location = FMath::RandPointInBox(DamageBoxMesh->Bounds.GetBox()) + FVector(0.f, 0.f, 75000.f);
        FVector TopExtent = DamageBoxMesh->CalcBounds(DamageBoxMesh->GetComponentTransform()).BoxExtent;
        //FVector TopExtent = DamageBoxMesh->GetScaledBoxExtent();
        FVector Location = DamageBoxMesh->GetComponentLocation() + FVector(FMath::RandRange(-TopExtent.X, TopExtent.X), FMath::RandRange(-TopExtent.Y, TopExtent.Y), TopExtent.Z);
        FVector EndLocation = Location;
        EndLocation.Z = -200.f;
        //DrawDebugSphere(GetWorld(), Location, 100.f, 12, FColor::Red, true);
        UE_LOG(LogTemp, Warning, TEXT("Loc=%f"), Location.Z);

        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        GetWorld()->LineTraceSingleByChannel(HitResult, Location, EndLocation, ECollisionChannel::ECC_Visibility, QueryParams);

        if (MissileFireEffect[FireEffectIndex] && HitResult.bBlockingHit && HitResult.ImpactPoint.Z < Location.Z)
        {
            //DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 100.f, 12, FColor::Orange, true);
            
            UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),
                MissileFireEffect[FireEffectIndex],
                HitResult.ImpactPoint, // Spawn location
                {0.f, FMath::RandRange(0.f, 360.f), 0.f}, // Spawn rotation
                {3.f, 3.f, 3.f},
                false



            );
            
        }
    }

    for (int32 i = 0; i < 20; i++)
    {   
        int32 SmokeEffectIndex = FMath::RandRange(0, 1); 

        //FVector Location = FMath::RandPointInBox(DamageBoxMesh->Bounds.GetBox()) + FVector(0.f, 0.f, 50000.f);
        FVector TopExtent = DamageBoxMesh->CalcBounds(DamageBoxMesh->GetComponentTransform()).BoxExtent;
        FVector Location = DamageBoxMesh->GetComponentLocation() + FVector(FMath::RandRange(-TopExtent.X, TopExtent.X), FMath::RandRange(-TopExtent.Y, TopExtent.Y), TopExtent.Z);
        FVector EndLocation = Location;
        EndLocation.Z = -200.f;
        
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
        GetWorld()->LineTraceSingleByChannel(HitResult, Location, EndLocation, ECollisionChannel::ECC_Visibility, QueryParams);

        if (SmokeSystem[SmokeEffectIndex] && HitResult.bBlockingHit && HitResult.ImpactPoint.Z < Location.Z)
        {
            //DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 100.f, 12, FColor::Orange, true);
            
            SmokeSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
                SmokeSystem[SmokeEffectIndex],
                GetRootComponent(),
                FName(),
                HitResult.ImpactPoint,
                {0.f, FMath::RandRange(0.f, 360.f), 0.f},
                EAttachLocation::KeepWorldPosition,
                false
            );
            
        }
    }
    */

    if (FireSound)
    {
        UGameplayStatics::SpawnSoundAtLocation(
            GetWorld(),
            FireSound,
            GetActorLocation(),
            GetActorRotation(),
            2.2f, // volume multiplier
            1.0f, // pitch multiplier
            0.0f, // start time
            nullptr, //USoundAttenuation
            nullptr,
            false
        );
    }



    //FTimerHandle StopEffectTimer;
    //GetWorldTimerManager().SetTimer(StopEffectTimer, this, &AMissleSystem::StopEffects, 15.f);
}

void AMissleSystem::StopEffects()
{
    TrailSystemComponent->DestroyComponent();
}


void AMissleSystem::Destroyed()
{

}


void AMissleSystem::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bOverlappingDamageBox);
    DOREPLIFETIME(ThisClass, bMissleHit);

}


