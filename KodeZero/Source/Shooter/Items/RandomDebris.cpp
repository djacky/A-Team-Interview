

#include "RandomDebris.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Debris.h"

ARandomDebris::ARandomDebris()
{
	PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
	SetReplicatingMovement(true);

	MainSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MainSceneComponent"));
	SetRootComponent(MainSceneComponent);

    SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SpawnBox->SetupAttachment(MainSceneComponent);
    SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//FireParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FireComponent"));
	//FireParticleComponent->SetupAttachment(MainSceneComponent);
	//FireParticleComponent->SetAutoActivate(true);
	//FireParticleComponent->Deactivate();
	//FireParticleComponent->SetTemplate(FireParticle);
}

void ARandomDebris::BeginPlay()
{
	Super::BeginPlay();
}

void ARandomDebris::StartSpawnDebrisProcess(FVector2D ScaleArea, float RotationValue)
{
	if (HasAuthority())
	{
		float ZScale = 12.f;
		SpawnBox->SetWorldRotation({0.f, 0.f, RotationValue});
    	SpawnBox->SetWorldScale3D({ScaleArea.X, ScaleArea.Y, ZScale});
		/*
		SpawnBox->SetWorldLocation(
			FVector(SpawnBox->GetComponentLocation().X,
			SpawnBox->GetComponentLocation().Y,
			SpawnBox->GetComponentLocation().Z + ZScale * SpawnBox->GetScaledBoxExtent().X)
			);
		*/
		SpawnDebris();
	}
}

void ARandomDebris::SpawnDebris()
{
	++NumToSpawn;
	if (NumToSpawn > 26)
	{
		FireLocations = ServerFireLocations;
		OnRep_FireLocations();
		return;
	}
	FVector TopExtent = SpawnBox->GetScaledBoxExtent();
	FVector Location = SpawnBox->GetComponentLocation() + FVector(FMath::RandRange(-TopExtent.X, TopExtent.X), FMath::RandRange(-TopExtent.Y, TopExtent.Y), TopExtent.Z);
	Location += FVector(0.f, 0.f, 1000.f);
	FVector EndLocation = Location;
	EndLocation.Z = -200.f;
	//DrawDebugSphere(GetWorld(), Location, 100.f, 12, FColor::Red, true);
	//UE_LOG(LogTemp, Warning, TEXT("Loc=%f"), Location.Z);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByChannel(HitResult, Location, EndLocation, ECollisionChannel::ECC_Visibility, QueryParams);

	if (HitResult.bBlockingHit && HitResult.ImpactPoint.Z < Location.Z && NumToSpawn >= NumFireToSpawn + 1)
	{
		ADebris* SpawnedDebris = GetWorld()->SpawnActor<ADebris>(DebrisClass, HitResult.TraceStart, FRotator(0.f,FMath::RandRange(0.f, 360.f),0.f));
		if (SpawnedDebris) SpawnedDebris->SetAssetProps();
		//DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 100.f, 12, FColor::Red, true);
	}
	else if (HitResult.bBlockingHit && HitResult.ImpactPoint.Z < Location.Z && NumToSpawn < NumFireToSpawn + 1)
	{
		ServerFireLocations.FireLocationArray.Add(HitResult.ImpactPoint);
		int32 ParticleIndex = FMath::RandRange(0, FireParticleArray.Num() - 1);
		ServerFireLocations.FireIndicies.Add(ParticleIndex);
	}

	FTimerHandle DebrisTimer;
	GetWorldTimerManager().SetTimer(DebrisTimer, this, &ARandomDebris::SpawnDebris, 1.f);
}

void ARandomDebris::OnRep_FireLocations()
{
	//UE_LOG(LogTemp, Warning, TEXT("FireEmitter"));
	//FireParticleComponent->Activate();
	if (FireLocations.FireIndicies.Num() != FireLocations.FireLocationArray.Num()) return;
	for (int32 i = 0; i < FireLocations.FireLocationArray.Num(); i++)
	{
		UGameplayStatics::SpawnEmitterAttached(
			FireParticleArray[FireLocations.FireIndicies[i]], 
			GetRootComponent(), 
			FName(""), 
			FireLocations.FireLocationArray[i],
			FRotator(0.f, 0.f, 0.f),
			FVector(2.f, 2.f, 2.f),
			EAttachLocation::KeepWorldPosition,
			false
		);

		UGameplayStatics::PlaySoundAtLocation(this, FireSound, FireLocations.FireLocationArray[i]);
	}
}

void ARandomDebris::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ThisClass, FireParticleComponent);
	DOREPLIFETIME(ThisClass, FireLocations);
	//DOREPLIFETIME(ThisClass, FireIndicies);
}
