// Fill out your copyright notice in the Description page of Project Settings.

#include "Ammo.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/EnumTypes/RarityType.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"

AAmmo::AAmmo()
{
	bReplicates = true;
	SetReplicatingMovement(true);

    // Construct the AmmoMesh component and seti it as the root
    AmmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AmmoMesh"));
	AmmoMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    SetRootComponent(AmmoMesh);

    GetAreaSphere()->SetupAttachment(GetRootComponent());
    GetCollisionBox()->SetupAttachment(GetRootComponent());
    GetPickupWidget()->SetupAttachment(GetRootComponent());

    AmmoCollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AmmoCollisionSphere"));
	AmmoCollisionSphere->SetupAttachment(AmmoMesh);
    AmmoCollisionSphere->SetSphereRadius(50.f);

}

void AAmmo::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AAmmo::BeginPlay()
{
    Super::BeginPlay();

    AmmoCollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AAmmo::AmmoSphereOverlap);

	if (HasAuthority())
	{
		AmmoMesh->OnComponentHit.AddDynamic(this, &AAmmo::OnHit);
	}

	if (AmmoState == EItemState::EIS_Pickup)
	{
		StartPulseTimer();
	}
}

void AAmmo::UpdateAmmo()
{
	// Load the data in the Item Rarity Data Table

	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	SetItemRarity(EItemRarity::EIR_Common);
	const FString AmmoTablePath(TEXT("DataTable'/Game/_Game/DataTables/AmmoDT.AmmoDT'"));
	UDataTable* AmmoTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *AmmoTablePath));
	if (AmmoTableObject)
	{
		FAmmoDataTable* AmmoRow = nullptr;
		switch (AmmoType)
		{
			case EAmmoType::EAT_9mm:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("9mm"), TEXT(""));
			break;

			case EAmmoType::EAT_45mm:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("45mm"), TEXT(""));
			break;

			case EAmmoType::EAT_AR:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("AR"), TEXT(""));
			break;

			case EAmmoType::EAT_Shells:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("Shells"), TEXT(""));
			break;

			case EAmmoType::EAT_GrenadeRounds:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("GrenadeRounds"), TEXT(""));
			break;

			case EAmmoType::EAT_HelicopterMissiles:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("HelicopterMissiles"), TEXT(""));
			break;

			case EAmmoType::EAT_GravCharges:
			AmmoRow = AmmoTableObject->FindRow<FAmmoDataTable>(FName("GravCharges"), TEXT(""));
			break;

		}
		if (AmmoRow)
		{
			//AmmoMesh = AmmoRow->AmmoMeshTable;
			AmmoMesh->SetStaticMesh(AmmoRow->AmmoMeshTable);
			SetItemName(AmmoRow->ItemName);
			if (HasAuthority())
			{
				SetItemCount(AmmoRow->ItemCount);
			}
            SetPickupSound(AmmoRow->PickupSound);
            SetEquipSound(AmmoRow->EquipSound);
			AmmoTexture = AmmoRow->AmmoTexture;
			SetMaterialInstance(AmmoRow->MaterialInstance);
		}
	}  
	if (GetMaterialInstance())
	{
	    SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
		GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
		AmmoMesh->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
		EnableGlowMaterial();
	} 
}

void AAmmo::OnRep_AmmoType()
{
	UpdateAmmo();
}

void AAmmo::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
	UpdateAmmo();
}

void AAmmo::SetItemProperties()
{
	Super::SetItemProperties();
	switch (AmmoState)
	{
		case EItemState::EIS_Pickup:
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetEnableGravity(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoMesh->SetNotifyRigidBodyCollision(false);
			AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
			StartPulseTimer();
			break;

		case EItemState::EIS_Pickup2:

			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetEnableGravity(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoMesh->SetNotifyRigidBodyCollision(false);
			AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
			break;

		case EItemState::EIS_Equipped:
		{
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetEnableGravity(false);
			AmmoMesh->SetVisibility(false);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);

			if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(GetOwner()))
			{
				ShooterInterface->Execute_OnItemPickedUp(GetOwner(), this, TEXT("Ammo"));
			}
			break;
		}

		case EItemState::EIS_Falling:
			// Set Mesh Properties
			//SetReplicatingMovement(false);
			AmmoMesh->SetSimulatePhysics(true);
			AmmoMesh->SetEnableGravity(true);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			AmmoMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			AmmoMesh->SetNotifyRigidBodyCollision(true);
			//AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//GetCollisionBox()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			//GetCollisionBox()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//GetCollisionBox()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);

			EnableGlowMaterial();
			//EnableCustomDepth();
			break;

		case EItemState::EIS_EquipInterping:
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			AmmoMesh->SetSimulatePhysics(false);
			AmmoMesh->SetEnableGravity(false);
			AmmoMesh->SetVisibility(true);
			AmmoMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AmmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
			break;
	}
}

void AAmmo::SetAmmoState(EItemState State)
{
	SetItemState(State);
	AmmoState = State;
	SetItemProperties();
}

void AAmmo::OnRep_AmmoState()
{
	SetItemProperties();
}

void AAmmo::OnRep_ItemState()
{
	Super::OnRep_ItemState();
}

void AAmmo::AmmoSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor)
    {
		if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(OtherActor))
		{
			ShooterInterface->Execute_OnAmmoSphereOverlap(OtherActor, this);
		}
    }
}

void AAmmo::EnableCustomDepth()
{
	AmmoMesh->SetRenderCustomDepth(true);
}

void AAmmo::DisableCustomDepth()
{
	AmmoMesh->SetRenderCustomDepth(false);
}

void AAmmo::ThrowAmmo(FTransform CharacterTransform)
{
	AmmoMesh->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	FRotator MeshRotation = FRotator{ 0.f, CharacterTransform.GetRotation().Rotator().Yaw - 90, 0.f };
	AmmoMesh->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	//FVector MeshForward = FVector{ AmmoMesh->GetForwardVector() };
	//FVector MeshRight = FVector{ AmmoMesh->GetRightVector() };

	//float RandomRotation = FMath::FRandRange(-180.f, 180.f);
	AmmoMesh->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	AmmoMesh->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// Direction in which we throw the weapon
	FVector TargetLocation = GetActorLocation() + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 200.f);
	FVector ImpulseDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();
	AmmoMesh->AddImpulse(10000.f * ImpulseDirection);
    
    //bFalling = true;
    //GetWorldTimerManager().SetTimer(ThrowAmmoTimer, this, &AAmmo::StopFalling, ThrowAmmoTime);
	bFalling = true;
	bInAir = true;
}

void AAmmo::StopFalling()
{
	Super::StopFalling();
    SetAmmoState(EItemState::EIS_Pickup);
}

void AAmmo::OnItemPickedUp(AShooterCharacter* InShooter)
{
	if (InShooter == nullptr) return;
	InShooter->Server_TakeAmmo(this);
}

// This is only used for the AIs (because character uses the DropAmmo function)
void AAmmo::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
	if (HasAuthority())
	{
		SetAmmoState(EItemState::EIS_Falling);
		if (InShooter)
		{
			FTransform StartTransform = InShooter->GetCapsuleComponent()->GetComponentTransform();
			StartTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			ThrowAmmo(StartTransform);
		}
		else
		{
			CharacterTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			ThrowAmmo(CharacterTransform);
		}
	}

	EnableGlowMaterial();
	StartPulseTimer();
}

void AAmmo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, AmmoState);
	DOREPLIFETIME(ThisClass, AmmoType);
}