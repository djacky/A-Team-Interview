// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/ShooterPlayerState.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"


AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = 0.f;
	ProjectileMovementComponent->MaxSpeed = 0.f;

    BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
    BulletMesh->SetupAttachment(CollisionBox);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//SetReplicateMovement(true);

	CollisionBox->SetCollisionObjectType(ECC_Projectile);

	DestroyTime = 2.f;

}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		//CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileBullet::OnHit);
	}
}

void AProjectileBullet::Initialize(const FEnemyAttackData& InProjectileData)
{
    if (HasAuthority())
    {
        ProjectileData = InProjectileData;
        ApplyProjectileData();
		MulticastApplyProjectileData(ProjectileData);
		StartDestroyCheck();
    }
}

void AProjectileBullet::MulticastApplyProjectileData_Implementation(const FEnemyAttackData& InProjectileData)
{
	if (HasAuthority()) return;
    ProjectileData = InProjectileData;
    ApplyProjectileData();
}

void AProjectileBullet::ApplyProjectileData()
{
    // Apply struct properties to components
	ProjectileMovementComponent->ProjectileGravityScale = ProjectileData.bGravityEnabled ? 1.0f : 0.f;
	SetVelocity();
	
	if (ProjectileData.HomingData.bIsHoming && ProjectileData.TargetActor)
	{
		ProjectileMovementComponent->bIsHomingProjectile = true;
		ProjectileMovementComponent->HomingAccelerationMagnitude = ProjectileData.HomingData.HomingAcceleration;
		ProjectileMovementComponent->HomingTargetComponent = ProjectileData.TargetActor->GetRootComponent();
	}
	
	ImpactSound = ProjectileData.SoundData.ImpactSound;
	ImpactParticles = ProjectileData.ParticleData.ImpactParticles;
	ImpactParticlesLegacy = ProjectileData.ParticleData.ImpactParticlesLegacy;
	TrailSystem = ProjectileData.ParticleData.TrailParticles;
	Tracer = ProjectileData.ParticleData.Tracer;
	SpawnTrailSystem(ProjectileData.ParticleData.TrailEffectScale);
	SpawnTracerSystem();
	if (ProjectileData.ProjectileMesh) BulletMesh->SetStaticMesh(ProjectileData.ProjectileMesh);
	if (ProjectileData.SoundData.ProjectileSoundLoop && ProjectileData.SoundData.LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileData.SoundData.ProjectileSoundLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			ProjectileData.SoundData.LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileBullet::MulticastSetBulletProperties_Implementation(const FWeaponDataStruct &InWeaponData, const EAmmoType &InAmmoType)
{
    WeaponData = InWeaponData;
    FString MainParticlePath = TEXT("/Game/_Game/Assets/FX/");
    ImpactParticles = LoadObject<UNiagaraSystem>(nullptr, *(FString::Printf(TEXT("%Impact/%s.%s"), *MainParticlePath, *WeaponData.offChainMetadata.Bullet.ImpactEffectID, *WeaponData.offChainMetadata.Bullet.ImpactEffectID)));
    TrailSystem = LoadObject<UNiagaraSystem>(nullptr, *(FString::Printf(TEXT("%Trail/%s.%s"), *MainParticlePath, *WeaponData.offChainMetadata.Bullet.ImpactEffectID, *WeaponData.offChainMetadata.Bullet.ImpactEffectID)));
	
    // *** Need to set the ammo static mesh based on the type of ammo selected
    switch (InAmmoType)
    {
    case EAmmoType::EAT_9mm:
        //
        break;
    
    case EAmmoType::EAT_AR:
        //
        break;

    case EAmmoType::EAT_45mm:
        //
        break;

    case EAmmoType::EAT_GrenadeRounds:
        //
        break;

    case EAmmoType::EAT_GravCharges:
        //
        break;
    
    default:
        break;
    }
    
    InitialSpeed = WeaponData.offChainMetadata.Bullet.Speed;
    ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
	ProjectileMovementComponent->ProjectileGravityScale = WeaponData.offChainMetadata.Bullet.Drop ? 1.0f : 0.f;
    SpawnTrailSystem();
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{

    if (OtherActor == GetInstigator()) return;

	// These next 2 lines need to be done in HasAuthority
	bRegisteredHit = true;
	if (ProjectileData.LaunchData.bIsTargetLaunched) LaunchTarget(1200.f);

	MulticastOnHit();
	
	if (ProjectileData.ExplosionData.bIsExplosion)
	{
		ExplodeDamage();
	}
	else
	{
		APawn* InstigatorPawn = GetInstigator(); 
		if (OtherActor && OtherActor->Implements<UWeaponHitInterface>() && InstigatorPawn)
		{
			IWeaponHitInterface::Execute_OnProjectileWeaponHit(
				OtherActor, 
				InstigatorPawn->GetController(),
				ProjectileData.Damage,
				this,
				Hit,
				UDamageType::StaticClass()
				);
		}
	}
}

void AProjectileBullet::MulticastOnHit_Implementation()
{
	StartDestroyTimer();
	OnRegisteredHit();
}

void AProjectileBullet::OnRegisteredHit()
{
	if (ImpactParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactParticles, GetActorLocation(), GetActorRotation(), FVector3d(ImpactParticleScale));
	}
	if (ImpactParticlesLegacy && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticlesLegacy, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (BulletMesh)
	{
		BulletMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent)
	{
		TrailSystemComponent->Deactivate();
	}
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
	}
	if (TracerComponent)
	{
		TracerComponent->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}

void AProjectileBullet::LaunchTarget(float SphereRadius)
{
    FVector MyLocation = GetActorLocation();
    TArray<FHitResult> HitResults;
	TArray<AActor*> FoundCharacters;

    // Perform a sphere trace to find nearby actors
    UWorld* World = GetWorld();
    if (World)
    {
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(GetInstigator());

        // Perform a sphere trace
        if (World->SweepMultiByChannel(
            HitResults,                // Array to store hit results
            MyLocation,                 // Start location
            MyLocation,                 // End location (same as start for a sphere trace)
            FQuat::Identity,            // Rotation (identity rotation for a sphere trace)
            ECC_Pawn,                   // Collision channel (adjust as needed)
            FCollisionShape::MakeSphere(SphereRadius), // Sphere shape
            QueryParams                  // Query parameters
        ))
        {
			for (auto HitResult : HitResults)
			{
				if (ACharacter* TargerChar = Cast<ACharacter>(HitResult.GetActor()))
				{
					FVector LaunchVelocity;
					bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
						this,
						LaunchVelocity,
						TargerChar->GetActorLocation(),
						ProjectileData.LaunchData.LaunchDirection
					);
					TargerChar->LaunchCharacter(LaunchVelocity, false, false);
				}
			}
        }
    }
}

void AProjectileBullet::ExplodeDamage(TSubclassOf<UDamageType> DamageType)
{
	APawn* InstigatorPawn = GetInstigator();
	if (HasAuthority() && InstigatorPawn)
	{
		if (AShooterPlayerState* PS = Cast<AShooterPlayerState>(InstigatorPawn->GetPlayerState())) PS->DamageType = EShooterDamageType::ESDT_Explosion;
        TArray<AActor*> ActorsToIgnore;

		AController* InstigatorController = InstigatorPawn ? InstigatorPawn->GetController() : nullptr;
		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this,
			ProjectileData.ExplosionData.ExplosionDamage,
			10.f, // Minimum damage
			GetActorLocation(),
			ProjectileData.ExplosionData.ExplosionInnerRadius,
			ProjectileData.ExplosionData.ExplosionOuterRadius,
			ProjectileData.ExplosionData.ExplosionDamageFalloff,
			DamageType,
			ActorsToIgnore, // Optional: Add actors to ignore
			this,
			InstigatorController
		);
	}
}

void AProjectileBullet::MulticastOnTimerExpired_Implementation()
{
	OnRegisteredHit();
}

void AProjectileBullet::SetVelocity()
{
	if (ProjectileData.bIsArcing && ProjectileData.TargetActor)
	{
		FVector LaunchVelocity;
		/*
		bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_MovingTarget(
			this,
			LaunchVelocity,
			GetActorLocation(),
			ProjectileData.TargetActor
		);
		*/
		
		bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
			this,
			LaunchVelocity,
			GetActorLocation(),
			ProjectileData.TargetActor->GetActorLocation()
		);
		
		ProjectileMovementComponent->Velocity = LaunchVelocity;
		
	}
	else
	{
		ProjectileMovementComponent->InitialSpeed = ProjectileData.ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = ProjectileData.ProjectileSpeed;
	}
}

void AProjectileBullet::CheckIfNotDestroyed()
{
	if (HasAuthority())
	{
		if (!bRegisteredHit)
		{
			MulticastOnTimerExpired();
			if (ProjectileData.ExplosionData.bIsExplosion) ExplodeDamage();
			Super::CheckIfNotDestroyed();
		}
	}
}

void AProjectileBullet::Destroyed()
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(CheckProjectileTimer);
	}
}