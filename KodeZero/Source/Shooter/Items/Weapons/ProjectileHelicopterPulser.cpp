// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ProjectileHelicopterPulser.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterCharacter.h"
#include "Helicopter.h"
#include "Shooter/Drone/ItemContainer.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "HelicopterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "EnumTypes/WeaponType.h"

AProjectileHelicopterPulser::AProjectileHelicopterPulser()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = WeaponSpeed;
	ProjectileMovementComponent->MaxSpeed = WeaponSpeed;

    BoxSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("BoxSceneComponent"));
    BoxSceneComponent->SetupAttachment(CollisionBox);
}

void AProjectileHelicopterPulser::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileHelicopterPulser::OnHit);
        CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
	}
    SpawnTracerSystem();
}

void AProjectileHelicopterPulser::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	APawn* FiringPawn = GetInstigator();
    if (!FiringPawn) return;
	AHelicopter* OwnerHelicopter = Cast<AHelicopter>(FiringPawn);
    AController* InstigatorController = FiringPawn->GetController();
    
	if (OwnerHelicopter && OwnerHelicopter->GetEquippedWeapon() && InstigatorController)
	{
		if (OtherActor && OtherActor->Implements<UWeaponHitInterface>())
		{
            float DamageToApply = OwnerHelicopter->GetEquippedWeapon()->WeaponDamageAmount;
			IWeaponHitInterface::Execute_OnProjectileWeaponHit(
				OtherActor, 
				InstigatorController,
				DamageToApply,
				this,
				Hit,
				UDronePulsarType::StaticClass()
				);
            bool bHasShield = IWeaponHitInterface::Execute_HasShield(OtherActor);
            ShowHitNumber(OwnerHelicopter, DamageToApply, OtherActor->GetActorLocation(), false, bHasShield);
		}

        /*
        AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(OtherActor);
        if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled() && HitCharacter)
        {
            OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest(
                HitCharacter,
                TraceStart,
                InitialVelocity,
                OwnerController->GetServerTime() - OwnerController->SingleTripTime
            );
        }
        */
	}

	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileHelicopterPulser::ShowHitNumber(AHelicopter* HelicopterActor, float DamageAmount, FVector HitLocation, bool bHeadshot, bool bShield)
{
    if (HelicopterActor && HelicopterActor->IsLocallyControlled())
    {
        HelicopterActor->ShowHitNumber(DamageAmount, HitLocation, bHeadshot, bShield);
    }
}

float AProjectileHelicopterPulser::SetGunDamageAmount(AShooterCharacter* DamagedCharacter, UPrimitiveComponent* HitComponent, AHelicopter* OwnerHelicopter)
{
    if (!OwnerHelicopter) return 0.f;
	float Wd = OwnerHelicopter->GetEquippedWeapon()->WeaponDamageAmount;
	float ScaledWeaponDamage = (-3.f * Wd / (4.f * DamagedCharacter->GetMaxHandShieldStrength())) * DamagedCharacter->HandShieldStrength + Wd; // Damage to set based on shield strength
	float DamageToShow = 0.f;
	if (HitComponent->ComponentHasTag(FName("ShieldTag")))
	{
		// Dot product between attacker forward direction and victim forward direction
		// If player is shooting another player's shield from behind, doesn't count as damage
		float PlayerDot = UKismetMathLibrary::Dot_VectorVector(CollisionBox->GetForwardVector(), DamagedCharacter->GetCapsuleComponent()->GetForwardVector());
		if (PlayerDot > 0.f)
		{
			// condition should be < 0, but for some reason, the forward angles seem to be negated in Unreal.
			return -1.f;
		}
		DamageToShow = DamagedCharacter->GetBoostProtect() == false ?
					FMath::Floor(ScaledWeaponDamage) : 0.f;
	}
	else
	{
		DamageToShow = DamagedCharacter->GetBoostProtect() == false ? OwnerHelicopter->GetEquippedWeapon()->WeaponDamageAmount : 0.f;
	}

	return DamageToShow;
}

EWeaponType AProjectileHelicopterPulser::GetWeaponType_Implementation()
{
	return EWeaponType::EWT_HeliPulser;
}