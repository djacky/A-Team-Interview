// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BaseAI/TeleportTrail.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "AICharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


ATeleportTrail::ATeleportTrail()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	MainSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MainSceneComponent"));
	SetRootComponent(MainSceneComponent);
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));
	NiagaraComp->SetupAttachment(MainSceneComponent);

}

void ATeleportTrail::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATeleportTrail::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (!bMoving) return;

    FVector CurrentLocation = GetActorLocation();
    FVector Delta = EndLocation - CurrentLocation;
    float RemainingDistance = Delta.Size();

    // Optional: Add a small epsilon check for floating-point precision to stop early if already very close
    if (FMath::IsNearlyZero(RemainingDistance))
    {
        SetActorLocation(EndLocation);
        bMoving = false;
        OnTrailReachedDestination();
        return;
    }

    float StepSize = TrailSpeed * DeltaTime;

    if (StepSize >= RemainingDistance)
    {
        // If the step would overshoot, snap exactly to the end location
        SetActorLocation(EndLocation);
        bMoving = false;
        OnTrailReachedDestination();
    }
    else
    {
        // Move towards the target without overshooting
        FVector Direction = Delta.GetSafeNormal();
        FVector NewLocation = CurrentLocation + Direction * StepSize;
        SetActorLocation(NewLocation);
    }
}

void ATeleportTrail::InitTrail(FVector Start, FVector End, float Speed, AAICharacterBase* OwningAI)
{
	if (OwningAI == nullptr) return;
    StartLocation = Start;
    EndLocation = End;
    TrailSpeed = Speed;
    OwningCharacter = OwningAI;

    SetActorLocation(StartLocation);
	MulticastPlayTrail(true, OwningCharacter);

    bMoving = true;
}

void ATeleportTrail::MulticastPlayTrail_Implementation(bool bPlay, AAICharacterBase* OwningAI)
{
	if (!OwningAI) return;
	if (bPlay)
	{
		if (OwningAI->GetEnemyData().TeleportData.TeleportingEffect)
		{
			NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				OwningAI->GetEnemyData().TeleportData.TeleportingEffect,
				RootComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				true
			);
		}

		if (OwningAI->GetEnemyData().TeleportData.TeleportingSound)
		{
			TrailSoundComponent = UGameplayStatics::SpawnSoundAttached(OwningAI->GetEnemyData().TeleportData.TeleportingSound, RootComponent);
		}
	}
	else
	{
		if (NiagaraComp) NiagaraComp->Deactivate();
		if (TrailSoundComponent) TrailSoundComponent->Deactivate();
	}
}

void ATeleportTrail::OnTrailReachedDestination()
{
    if (OwningCharacter)
    {
		MulticastPlayTrail(false, OwningCharacter);
        MulticastPlayStopTeleportEffects(EndLocation, OwningCharacter);
        OwningCharacter->CompleteTeleport();
    }

	FTimerHandle DestroyHandle;
	GetWorld()->GetTimerManager().SetTimer(DestroyHandle, this, &ATeleportTrail::DelayedDestroy, 2.f, false);
}

void ATeleportTrail::DelayedDestroy()
{
	Destroy();
}

void ATeleportTrail::MulticastPlayStopTeleportEffects_Implementation(FVector Location, AAICharacterBase* OwningAI)
{
	if (!OwningAI) return;
    // Play stop sound
    if (OwningAI->GetEnemyData().TeleportData.StopTeleportSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            OwningAI->GetEnemyData().TeleportData.StopTeleportSound,
            Location
        );
    }

    // Play stop particle effect
    if (OwningAI->GetEnemyData().TeleportData.StopTeleportEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            OwningAI->GetEnemyData().TeleportData.StopTeleportEffect,
            Location,
            OwningAI->GetMesh()->GetComponentRotation(),
            FVector::OneVector,
            true, // Auto-destroy
            true, // Auto-activate
            ENCPoolMethod::AutoRelease
        );
    }
}

