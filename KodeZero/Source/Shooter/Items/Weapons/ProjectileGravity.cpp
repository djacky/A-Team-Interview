// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileGravity.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "ShooterCharacter.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"

AProjectileGravity::AProjectileGravity()
{
    PrimaryActorTick.bCanEverTick = false;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;

	GravitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("GravitySphere"));
	GravitySphere->SetupAttachment(CollisionBox);
	GravitySphere->SetSphereRadius(1000.f);
}

void AProjectileGravity::BeginPlay()
{
	Super::BeginPlay();

    SpawnTrailSystem();
    SpawnTracerSystem();

    if (HasAuthority())
    {
        GravitySphere->OnComponentBeginOverlap.AddDynamic(this, &AProjectileGravity::OnGravitySphereOverlap);
        GravitySphere->OnComponentEndOverlap.AddDynamic(this, &AProjectileGravity::OnGravitySphereEndOverlap);
    }
    DestroyTime = 8.f;
    DamageInnerRadius = 100.f;
    DamageOuterRadius = 500.f;
    ImpactParticleScale = 2.5f;
}

void AProjectileGravity::Destroyed()
{
	ExplodeDamage(UGravitonType::StaticClass());
    SetShooterGravityPars(false);
    if (StartPullSoundComponent && StartPullSoundComponent->IsValidLowLevel()) 
    {
        StartPullSoundComponent->Stop();
        //StartPullSoundComponent->DestroyComponent();
    }
	Super::Destroyed();
}

void AProjectileGravity::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    bStartPull = true;
    SetShooterGravityPars(bStartPull);
    MulticastSpawnEffects(Hit);
}

void AProjectileGravity::MulticastSpawnEffects_Implementation(const FHitResult& Hit)
{
    ProjectileMovementComponent->Deactivate();
    SetActorRotation(UKismetMathLibrary::MakeRotFromZ(Hit.Normal)); // set the rotation so that the niagara effect selected shows correctly (in the direction of the surface normal)
    StartDestroyTimer();

    if (GravityEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
                GetWorld(),
                GravityEffect,
                GetActorLocation(),
                GetActorRotation(),
                true // auto destroy after playing
            );
    }

	if (StartPullSound)
	{
		StartPullSoundComponent = UGameplayStatics::SpawnSoundAttached(
            StartPullSound, // sound cue (USoundBase)
            CollisionBox, // mesh to attach to
            FName(""),   //socket name
            FVector(0,0,0),  //location relative to socket
            FRotator(0,0,0), //rotation 
            EAttachLocation::KeepRelativeOffset, 
            true //if true, will be deleted automatically
        );
	}
}

void AProjectileGravity::SetShooterGravityPars(bool bSetPull)
{
    if (!bStartPull) return;
    //if (ShooterCharacterClass == nullptr) return;
    //AShooterCharacter* OwnerCharacter = Cast<AShooterCharacter>(GetOwner());
    TArray<AActor*> OverlappingCharacters;
    GravitySphere->GetOverlappingActors(OverlappingCharacters);

    if (OverlappingCharacters.Num() > 0)
    {   
        for (int32 i = 0; i < OverlappingCharacters.Num(); i++)
        {
            if (OverlappingCharacters[i] && OverlappingCharacters[i] != GetOwner())
            {
                OnOverlappedTriggered(bSetPull, OverlappingCharacters[i]);
            }
        }
    }	
}

void AProjectileGravity::OnGravitySphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != GetOwner())
    {
        OnOverlappedTriggered(true, OtherActor);
    }
}

void AProjectileGravity::OnGravitySphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor != GetOwner())
    {
        OnOverlappedTriggered(false, OtherActor);
    }
}

void AProjectileGravity::OnOverlappedTriggered(bool bSetPull, AActor* OverlappedActor)
{
    if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(OverlappedActor))
    {
        WeaponHitInterface->Execute_OnGravityProjectileHit(OverlappedActor, bSetPull, this);
    }
}

EWeaponType AProjectileGravity::GetWeaponType_Implementation()
{
	return EWeaponType::EWT_GravCannon;
}