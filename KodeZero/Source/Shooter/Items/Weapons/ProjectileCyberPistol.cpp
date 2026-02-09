// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ProjectileCyberPistol.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"


AProjectileCyberPistol::AProjectileCyberPistol()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;

	//SetReplicateMovement(true);

	CollisionBox->SetCollisionObjectType(ECC_Projectile);
}

void AProjectileCyberPistol::BeginPlay()
{
	Super::BeginPlay();
    SpawnTrailSystem(FVector(0.1f, 0.1f, 0.1f));
    
    if (HasAuthority())
    {
        //PerformConeScanAndSetHomingTarget();
        StartDestroyCheck();
    }
}

void AProjectileCyberPistol::PerformConeScanAndSetHomingTarget()
{
    AActor* ShooterOwner = GetOwner();
    if (!ShooterOwner) return;

    FVector Start = ShooterOwner->GetActorLocation();
    FVector Forward = ShooterOwner->GetActorForwardVector();
    float TraceLength = 7500.f;
    FVector End = Start + Forward * TraceLength;
    const FVector TraceCenter = Start + Forward * (TraceLength * 0.5f);
    const FVector BoxHalfExtent(TraceLength * 0.5f, 500.f, 500.f);

    // Box orientation (match projectile forward)
    FQuat BoxRotation = Forward.ToOrientationQuat();

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(ShooterOwner);

    TArray<FHitResult> HitResults;
    bool bHit = GetWorld()->SweepMultiByChannel(
        HitResults,
        TraceCenter,
        TraceCenter, // Stationary trace (centered in place)
        BoxRotation,
        ECC_Pawn,
        FCollisionShape::MakeBox(BoxHalfExtent),
        Params
    );

    AActor* ClosestTarget = nullptr;
    float MinDistSq = FLT_MAX;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && HitActor->IsA<APawn>() && HitActor != ShooterOwner)
        {
            float DistSq = FVector::DistSquared(HitActor->GetActorLocation(), Start);
            if (DistSq < MinDistSq)
            {
                MinDistSq = DistSq;
                ClosestTarget = HitActor;
            }
        }
    }

    DrawDebugBox(
        GetWorld(),
        TraceCenter,
        BoxHalfExtent,
        BoxRotation,
        FColor::Green,
        false,
        5.f
    );

    if (ClosestTarget && ProjectileMovementComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClosestTarget = %s"), *ClosestTarget->GetName());
        ProjectileMovementComponent->bIsHomingProjectile = true;
        ProjectileMovementComponent->HomingTargetComponent = ClosestTarget->GetRootComponent();
        ProjectileMovementComponent->HomingAccelerationMagnitude = 2000.f;
    }
}

void AProjectileCyberPistol::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor == GetInstigator()) return;

    ExplodeDamage(UCyperPistolType::StaticClass());
    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileCyberPistol::Destroyed()
{
    Super::Destroyed();
	if (ImpactParticlesLegacy && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticlesLegacy, GetActorTransform());
	}
	if (TrailSystemComponent)
	{
		TrailSystemComponent->Deactivate();
	}
}

EWeaponType AProjectileCyberPistol::GetWeaponType_Implementation()
{
	return EWeaponType::EWT_CyberPistol;
}
