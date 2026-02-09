// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Shooter/Shooter.h"
#include "Weapon.h"
#include "Shooter/ShooterCharacter.h"
#include "Components/BoxComponent.h"
#include "EnumTypes/WeaponType.h"
#include "Shooter/ShooterPlayerState.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetNotifyRigidBodyCollision(true);

}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
}

void AProjectile::StartDestroyCheck()
{
	if (HasAuthority())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FTimerDelegate CheckProjectileDel;
			CheckProjectileDel.BindUFunction(this, FName("CheckIfNotDestroyed"));
			World->GetTimerManager().SetTimer(CheckProjectileTimer, CheckProjectileDel, 12.f, false);
		}
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

void AProjectile::SpawnTrailSystem(const FVector& InScale)
{
	if (TrailSystem)
	{
		TrailSystemComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailSystem,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
		if (TrailSystemComponent)
		{
			// Set the desired scale
			TrailSystemComponent->SetWorldScale3D(InScale); // Replace with your desired scale
		}
	}
}

void AProjectile::SpawnTracerSystem()
{
	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}
}

void AProjectile::ExplodeDamage(TSubclassOf<UDamageType> DamageType)
{
	APawn* FiringPawn = GetInstigator();
	auto Character = Cast<AShooterCharacter>(FiringPawn);
	
	if (FiringPawn && HasAuthority())
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController && Character && Character->GetEquippedWeapon())
		{
			if (Character->GetShooter_PS()) Character->GetShooter_PS()->DamageType = EShooterDamageType::ESDT_Explosion;

			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Character->GetEquippedWeapon()->SetWeaponDamage(""), // Base Damage
				10.f, // MinimumDamage
				GetActorLocation(), // Origin
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.f, // DamageFalloff
				DamageType, // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors
				this, // DamageCauser
				FiringController // InstigatorController
			);

			//FHitResult InHit;
			//InHit.Location = GetActorLocation();
			//Character->ImplementChaos(InHit, EWeaponType::EWT_GrenadeLauncher);
		}
	}
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::StartDestroyTimer()
{
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		this,
		&AProjectile::DestroyTimerFinished,
		DestroyTime
	);
}

void AProjectile::DestroyTimerFinished()
{
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(CheckProjectileTimer);
	}
	if (ImpactParticles)
	{
		//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorLocation(), GetActorRotation(), FVector3d(ImpactParticleScale), true);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactParticles, GetActorLocation(), GetActorRotation(), FVector3d(ImpactParticleScale));
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::CheckIfNotDestroyed()
{
	if (IsValid(this)) Destroy();
}

AShooterCharacter* AProjectile::GetShooterCharacter_Implementation()
{
    return Cast<AShooterCharacter>(GetOwner());
}

