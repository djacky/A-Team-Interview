// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterSpectatorPawn.h"
#include "Net/UnrealNetwork.h"

AShooterSpectatorPawn::AShooterSpectatorPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
}

void AShooterSpectatorPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (TargetActor)
    {
        FVector TargetLocation = TargetActor->GetActorLocation() + CameraOffset;
        FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, SmoothSpeed);
        SetActorLocation(NewLocation);
    }
}

void AShooterSpectatorPawn::SetTarget(AActor* NewTarget)
{
    if (HasAuthority())
    {
        TargetActor = NewTarget;
    }
    else
    {
        ServerSetTarget(NewTarget); // Request the server to set the target
    }
}

void AShooterSpectatorPawn::ServerSetTarget_Implementation(AActor* NewTarget)
{
    SetTarget(NewTarget);
}

// Required Unreal Engine function to replicate variables
void AShooterSpectatorPawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TargetActor);
}