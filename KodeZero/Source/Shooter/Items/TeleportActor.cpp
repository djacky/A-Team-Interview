// Fill out your copyright notice in the Description page of Project Settings.


#include "TeleportActor.h"
#include "Components/BoxComponent.h"
#include "Shooter/ShooterCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/Item.h"

// Sets default values
ATeleportActor::ATeleportActor()
{
	bReplicates = true;
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	TeleportBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TeleportBox"));
	TeleportBox->SetupAttachment(RootSceneComponent);
	TeleportBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TeleportBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	TeleportBox->SetGenerateOverlapEvents(true);

	MainParticle = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MainParticleEffect"));
	MainParticle->SetupAttachment(TeleportBox);

}

// Called when the game starts or when spawned
void ATeleportActor::BeginPlay()
{
	Super::BeginPlay();

	TeleportBox->OnComponentBeginOverlap.AddDynamic(this, &ATeleportActor::OnTeleportBoxOverlap);
}

void ATeleportActor::OnTeleportBoxOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCanEnter)
	{
		if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(OtherActor))
		{
			if (ShooterChar->bIsAI || ShooterChar->OwnedPawn) return;
			bCanEnter = false;
			if (HasAuthority())
			{
				MulticastTeleport(ShooterChar);
				UWorld* World = GetWorld();
				if (World)
				{
					FTimerDelegate TeleportDel;
					FTimerHandle TeleportTimer;
					TeleportDel.BindUFunction(this, FName(TEXT("WorldTeleport")), ShooterChar, RootSceneComponent->GetComponentTransform().TransformPosition(TeleportLocation));
					World->GetTimerManager().SetTimer(TeleportTimer, TeleportDel, 0.7f, false);
				}
			}
			if (ShooterChar->IsLocallyControlled())
			{
				ShooterChar->SetWorldTeleporting(true);
				LocalStartTeleport(ShooterChar);
			}
		}
	}
}

void ATeleportActor::LocalStartTeleport(AShooterCharacter* InCharacter)
{
	if (InCharacter)
	{
		if (TeleportMaterialInstance) DynamicTeleportMaterialInstance = UMaterialInstanceDynamic::Create(TeleportMaterialInstance, this);
		if (DynamicTeleportMaterialInstance)
		{
			InCharacter->SetAllSkeletalMeshMaterials(false, DynamicTeleportMaterialInstance);
			AItem* EquippedItem = InCharacter->GetEquippedItem();
			if (EquippedItem)
			{
				if (EquippedItem->bIsAnNFT)
				{
					AllItemMaterials = EquippedItem->GetNFTMesh()->GetMaterials();
					for (int32 i = 0; i < AllItemMaterials.Num(); i++)
					{
						if (AllItemMaterials[i])
						{
							EquippedItem->GetNFTMesh()->SetMaterial(i, DynamicTeleportMaterialInstance);
						}
					}
				}
				else if (EquippedItem->GetItemMesh())
				{
					AllItemMaterials = EquippedItem->GetItemMesh()->GetMaterials();
					for (int32 i = 0; i < AllItemMaterials.Num(); i++)
					{
						if (AllItemMaterials[i])
						{
							EquippedItem->GetItemMesh()->SetMaterial(i, DynamicTeleportMaterialInstance);
						}
					}
				}
			}
		}
		StartTeleport(InCharacter);
		
		if (TeleportSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TeleportSound, InCharacter->GetActorLocation());
		}
	}
}

void ATeleportActor::MulticastTeleport_Implementation(AShooterCharacter* InCharacter)
{
	if (InCharacter)
	{
		if (!InCharacter->IsLocallyControlled()) LocalStartTeleport(InCharacter);
	}
}

void ATeleportActor::WorldTeleport(AShooterCharacter* InCharacter, const FVector_NetQuantize &TeleportLoc)
{
	if (InCharacter)
	{
		InCharacter->SetActorLocation(TeleportLoc);
	}
}

void ATeleportActor::SetMaterials(AShooterCharacter* InCharacter, bool bIsTeleporting)
{
	if (InCharacter)
	{
		if (bIsTeleporting)
		{

		}
		else
		{
			if (InCharacter->GetGhostMode())
			{
				// This is down below in locally controlled
			}
			else if (InCharacter->GetBoostProtect())
			{
				InCharacter->ProtectMaterialEffect();
			}
			else
			{
				InCharacter->SetDefaultMaterials();
			}

			AItem* EquippedItem = InCharacter->GetEquippedItem();
			if (EquippedItem)
			{
				if (EquippedItem->bIsAnNFT)
				{
					for (int32 i = 0; i < AllItemMaterials.Num(); i++)
					{
						if (AllItemMaterials[i])
						{
							EquippedItem->GetNFTMesh()->SetMaterial(i, AllItemMaterials[i]);
						}
					}
				}
				else if (EquippedItem->GetItemMesh())
				{
					for (int32 i = 0; i < AllItemMaterials.Num(); i++)
					{
						if (AllItemMaterials[i])
						{
							EquippedItem->GetItemMesh()->SetMaterial(i, AllItemMaterials[i]);
						}
					}
				}
			}
			if (InCharacter->IsLocallyControlled())
			{
				InCharacter->SetWorldTeleporting(false);
				if (InCharacter->GetGhostMode())
				{
					InCharacter->GhostMaterialEffect();
				}
			}
		}
	}
	bCanEnter = true;
}


