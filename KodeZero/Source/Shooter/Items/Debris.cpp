// Fill out your copyright notice in the Description page of Project Settings.


#include "Debris.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ADebris::ADebris()
{
	PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
	SetReplicatingMovement(true);
	MainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(MainMesh);
	//MainMesh->SetupAttachment(RootComponent);
	MainMesh->SetIsReplicated(true);
}

void ADebris::BeginPlay()
{
	Super::BeginPlay();

}

void ADebris::SetAssetProps()
{
	if (HasAuthority())
	{
		int32 MeshIndex = FMath::RandRange(0, DebrisMeshArray.Num() - 1);
		MainMesh->SetStaticMesh(DebrisMeshArray[MeshIndex]);

		MainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MainMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
		MainMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		MainMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
		MainMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		MainMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
		MainMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		MainMesh->SetSimulatePhysics(true);
		MainMesh->SetEnableGravity(true);
		MainMesh->SetLinearDamping(0.1f);
		MainMesh->SetAngularDamping(0.0f);

		FTimerHandle DebrisPhysicsTimer;
		GetWorldTimerManager().SetTimer(DebrisPhysicsTimer, this, &ADebris::SetPhysics, 15.f);
	}
}

void ADebris::SetPhysics()
{
	MainMesh->SetSimulatePhysics(false);
	MainMesh->SetEnableGravity(false);
}

void ADebris::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MainMesh);
}

