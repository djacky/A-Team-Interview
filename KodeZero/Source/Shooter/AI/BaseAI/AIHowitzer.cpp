// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BaseAI/AIHowitzer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"


AAIHowitzer::AAIHowitzer()
{
	LeftExhaust = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("LeftExhaust"));
	LeftExhaust->SetAutoActivate(false);
	LeftExhaust->Deactivate();

	RightExhaust = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("RightExhaust"));
	RightExhaust->SetAutoActivate(false);
	RightExhaust->Deactivate();
}

void AAIHowitzer::BeginPlay()
{
	Super::BeginPlay();

}

void AAIHowitzer::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
    Super::ReceiveDamage(DamagedActor, Damage, DamageType, InstigatorController, DamageCauser);
    if (Health <= 0.f)
    {
        OnAIStopAttacking();
    }
}

void AAIHowitzer::OnAIStopAttacking()
{
    Super::OnAIStopAttacking();
    if (bIsFlyingToTarget)
    {
        FinishAttack();
    }
}

void AAIHowitzer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    /*
    if (bIsRising)
    {
        FVector Location = GetActorLocation();
        float TargetZ = StartZ + TargetAltitude;

        if (Location.Z < TargetZ)
        {
            FVector NewLocation = Location;
            NewLocation.Z = FMath::FInterpConstantTo(Location.Z, TargetZ, DeltaTime, RiseSpeed);

            // Perform a sweep upwards to check for blocking
            FHitResult Hit;
            SetActorLocation(NewLocation, true, &Hit);

            if (Hit.bBlockingHit)
            {
                // Could not move further
                //UE_LOG(LogTemp, Warning, TEXT("Blocked while rising"));
            }

            // Check if we’ve reached altitude
            if (FMath::IsNearlyEqual(NewLocation.Z, TargetZ, 1.0f))
            {
                OnReachedAltitude();
            }
        }
        else
        {
            OnReachedAltitude(); // Redundant fallback
        }
        return;
    }
    */

    if (bIsFlyingToTarget && TargetCharacter)
    {
        FlightTime += DeltaTime;

        FVector CurrentLocation = GetActorLocation();
        FVector TargetLocation = TargetCharacter->GetActorLocation();

        // Step 1: Compute target position with fixed Z (altitude)
        FVector TargetWithAltitude = TargetLocation;
        TargetWithAltitude.Z = DesiredFlightAltitude;

        // Step 2: Add gentle random offset (not additive every frame!)
        float OffsetX = FMath::Sin(FlightTime * 1.3f) * FlyRandomnessMagnitude;
        float OffsetY = FMath::Cos(FlightTime * 1.7f) * FlyRandomnessMagnitude;
        FVector RandomOffset = FVector(OffsetX, OffsetY, 0.f);

        // Step 3: Apply offset to target, not to current position
        FVector FinalTargetLocation = TargetWithAltitude + RandomOffset;

        // Step 4: Interpolate smoothly toward the final target position
        FVector NewLocation = FMath::VInterpTo(CurrentLocation, FinalTargetLocation, DeltaTime, 1.5f); // lower speed factor = smoother

        // Step 5: Apply new location
        //FHitResult Hit;
        SetActorLocation(NewLocation, true);

        // Step 6: Optional: look at target
        FVector FlatDirection = (TargetLocation - CurrentLocation);
        FlatDirection.Z = 0;
        if (!FlatDirection.IsNearlyZero())
        {
            FRotator LookAt = FlatDirection.Rotation();
            SetActorRotation(FRotator(0, LookAt.Yaw, 0));
        }

        return;
    }
}

void AAIHowitzer::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
}

void AAIHowitzer::StartMissileAttack()
{
    LaunchCharacter(FVector(0, 0, 2200), false, false);

    /*
    if (bIsRising || GetCharacterMovement()->IsFalling())
        return;
    SetMovement(true);
    StartRise();
    GetCharacterMovement()->SetMovementMode(MOVE_Flying); // Allow vertical movement manually
    */
}

void AAIHowitzer::StartFlyingToTarget()
{
    if (!TargetCharacter) return;
    if (HasAuthority())
    {
        bIsFlyingToTarget = true;
        OnRep_IsFlyingToTarget();
    }

    //UE_LOG(LogTemp, Log, TEXT("Started flying toward target"));
}

void AAIHowitzer::OnRep_IsFlyingToTarget()
{
    if (bIsFlyingToTarget)
    {
        SetMovement(true);
        GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        FlightTime = 0.f;
        if (LeftExhaust && RightExhaust)
        {
            LeftExhaust->Activate();
            RightExhaust->Activate();
        }
    }
    else
    {
        SetMovement(false);
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        FlightTime = 0.f;
        if (LeftExhaust && RightExhaust)
        {
            LeftExhaust->Deactivate();
            RightExhaust->Deactivate();
        }
    }
}

void AAIHowitzer::StartRise()
{
    bIsRising = true;
    StartZ = GetActorLocation().Z;
}

void AAIHowitzer::OnReachedAltitude()
{
    if (!bIsRising) return;
    
    //UE_LOG(LogTemp, Log, TEXT("Reached Target Altitude"));
    bIsRising = false;
    GetCharacterMovement()->Velocity = FVector::ZeroVector;
    // Wait for Anim Notify to call FireMissile
}

void AAIHowitzer::FinishAttack()
{
    if (HasAuthority())
    {
        bIsFlyingToTarget = false;
        OnRep_IsFlyingToTarget();
    }
}

void AAIHowitzer::Fire()
{
    CurrentAttack.LaunchData.LaunchDirection = GetActorLocation();
    Super::Fire();
}

void AAIHowitzer::UpdateEnemy()
{
    Super::UpdateEnemy();
    AttachExhaustComponents();
}

void AAIHowitzer::AttachExhaustComponents()
{
    if (GetMesh() && GetMesh()->DoesSocketExist(FName("LeftExhaust")) && GetMesh()->DoesSocketExist(FName("RightExhaust")) && LeftExhaust && RightExhaust)
    {
        LeftExhaust->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("LeftExhaust"));
        RightExhaust->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("RightExhaust"));
        return;
    }
    FTimerHandle ExhaustHandle;
    GetWorldTimerManager().SetTimer(ExhaustHandle, this, &AAIHowitzer::AttachExhaustComponents, 0.2f, false);
}

void AAIHowitzer::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
    if (HasAuthority() && bIsFlyingToTarget)
    {
        FinishAttack();
    }
}

void AAIHowitzer::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsFlyingToTarget);

}
