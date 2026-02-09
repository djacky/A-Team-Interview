// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileHelicopterMissile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Helicopter.h"
#include "HelicopterWeapon.h"
#include "MissileMovementComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "EnumTypes/WeaponType.h"
#include "ShooterCharacter.h"
#include "Shooter/ShooterPlayerState.h"

AProjectileHelicopterMissile::AProjectileHelicopterMissile()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HelicopterMissileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MissileMovementComponent = CreateDefaultSubobject<UMissileMovementComponent>(TEXT("MissileMovementComponent"));
	MissileMovementComponent->bRotationFollowsVelocity = true;
	MissileMovementComponent->SetIsReplicated(true);
	MissileMovementComponent->ProjectileGravityScale = 0.f;
    MissileMovementComponent->InitialSpeed = 4600.f;
    MissileMovementComponent->MaxSpeed = 4600.f;

    BoxSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BoxSceneComponent"));
    BoxSceneComponent->SetupAttachment(CollisionBox);
}

void AProjectileHelicopterMissile::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileHelicopterMissile::OnHit);
		CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
	}
    //ImpactParticleScale = 2.f;
	DamageInnerRadius = 400.f;
	DamageOuterRadius = 800.f;
	SpawnTrailSystem();
	SpawnTracerSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileHelicopterMissile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	APawn* FiringPawn = GetInstigator();
    if (FiringPawn == nullptr) return;
	auto OwnerHelicopter = Cast<AHelicopter>(FiringPawn);

    auto DamagedHelicopter = Cast<AHelicopter>(OtherActor);
	if (DamagedHelicopter && HasAuthority())
	{
		DamagedHelicopter->HelicopterDamageEffects(Hit.ImpactPoint);
		//DamagedHelicopter->HelicopterHitPoint = FVector_NetQuantize(0.f, 0.f, 0.f);
	}
	MissileExplodeDamage(DamagedHelicopter);
	StartDestroyTimer();

	if (ImpactParticles)
	{
		//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorLocation(), GetActorRotation(), FVector3d(ImpactParticleScale), true);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactParticles, GetActorLocation(), GetActorRotation(), FVector3d(ImpactParticleScale));
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->IsActive())
	{
		TrailSystemComponent->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
	/*
	if (HasAuthority())
	{
		auto GeometryComponent = Cast<UGeometryCollectionComponent>(Hit.GetComponent());
		if (GeometryComponent && OwnerHelicopter && OwnerHelicopter->OwnedShooter && BoxSceneComponent)
		{
			FHitResult InHit = Hit;
			InHit.Location = BoxSceneComponent->GetComponentLocation();
			OwnerHelicopter->OwnedShooter->ImplementChaos(InHit, EWeaponType::EWT_GrenadeLauncher);
		}
	}
	*/
}

void AProjectileHelicopterMissile::Destroyed()
{
	//ExplodeDamage();
	//Super::Destroyed();
}

void AProjectileHelicopterMissile::MissileExplodeDamage(AHelicopter* DamagedHelicopter)
{
	APawn* FiringPawn = GetInstigator();
	auto Helicopter = Cast<AHelicopter>(FiringPawn);
    if (Helicopter == nullptr) return;

	TArray<AActor*> ActorsToIgnoreInExplosion;
	if (DamagedHelicopter && HasAuthority() && GetInstigatorController())
	{
		ActorsToIgnoreInExplosion.Add(DamagedHelicopter);
		UGameplayStatics::ApplyDamage(DamagedHelicopter, 
			Helicopter->GetEquippedWeapon()->WeaponDamageAmount, 
			GetInstigatorController(), 
			this, 
			UDamageType::StaticClass()
			);
	}
	
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController && Helicopter && Helicopter->GetEquippedWeapon())
		{
			if (auto ShooterPS = Cast<AShooterPlayerState>(FiringController->PlayerState))
			{
				ShooterPS->DamageType = EShooterDamageType::ESDT_Helicopter;
			}
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Helicopter->GetEquippedWeapon()->WeaponDamageAmount, // Base Damage
				10.f, // MinimumDamage
				GetActorLocation(), // Origin
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.f, // DamageFalloff
				UDroneMissileType::StaticClass(), // DamageTypeClass
				ActorsToIgnoreInExplosion, // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatorController
			);
		}
	}
}

EWeaponType AProjectileHelicopterMissile::GetWeaponType_Implementation()
{
	return EWeaponType::EWT_HeliMissile;
}
