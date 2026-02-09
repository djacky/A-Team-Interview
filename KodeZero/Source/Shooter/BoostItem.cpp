// Fill out your copyright notice in the Description page of Project Settings.


#include "BoostItem.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
//#include "GameFramework/ProjectileMovementComponent.h"
#include "ShooterCharacter.h"
#include "BoostComponent.h"
//#include "CableComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"

ABoostItem::ABoostItem()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(true);
}

void ABoostItem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

	//RotateMeshEffect(DeltaTime);
}

void ABoostItem::BeginPlay()
{
    Super::BeginPlay();
	if (HasAuthority() && GetItemMesh())
	{
		GetItemMesh()->OnComponentHit.AddDynamic(this, &ABoostItem::OnHit);
	}
}

void ABoostItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
	UpdateBoost();
}

void ABoostItem::RotateMeshEffect(float DeltaTime)
{
    if (GetItemMesh() && GetItemState() == EItemState::EIS_Pickup)
    {
        GetItemMesh()->AddWorldRotation(FRotator(0.f, 45.f * DeltaTime, 0.f));
    }
}

void ABoostItem::UpdateBoost()
{
	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	const FString BoostTablePath(TEXT("DataTable'/Game/_Game/DataTables/BoostItem_DT.BoostItem_DT'"));
	UDataTable* BoostTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *BoostTablePath));
	if (BoostTableObject)
	{
		FBoostDataTable* BoostRow = nullptr;
		switch (BoostType)
		{
			case EBoostType::EBT_HealthV1:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("HealthV1"), TEXT(""));
			break;

			case EBoostType::EBT_HealthV2:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("HealthV2"), TEXT(""));
			break;

			case EBoostType::EBT_ShieldV1:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("ShieldV1"), TEXT(""));
			break;

			case EBoostType::EBT_ShieldV2:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("ShieldV2"), TEXT(""));
			break;

			case EBoostType::EBT_Ghost:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Ghost"), TEXT(""));
			break;

			case EBoostType::EBT_SuperJump:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("SuperJump"), TEXT(""));
			break;

			case EBoostType::EBT_Teleport:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Teleport"), TEXT(""));
			break;

			case EBoostType::EBT_Protect:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Protect"), TEXT(""));
			break;

			case EBoostType::EBT_Fly:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Fly"), TEXT(""));
			break;

			case EBoostType::EBT_Slow:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Slow"), TEXT(""));
			break;

			case EBoostType::EBT_Copy:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Copy"), TEXT(""));
			break;

			case EBoostType::EBT_Grapple:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("Grapple"), TEXT(""));
			break;

			case EBoostType::EBT_SuperPunch:
			BoostRow = BoostTableObject->FindRow<FBoostDataTable>(FName("SuperPunch"), TEXT(""));
			break;

		}
		if (BoostRow)
		{
			SetItemType(EItemType::EIT_Boost);
            SetPickupSound(BoostRow->PickupSound);
            SetEquipSound(BoostRow->EquipSound);
            GetItemMesh()->SetSkeletalMesh(BoostRow->ItemMesh);
            GetItemMesh()->SetWorldScale3D(BoostRow->ItemScale);
            ItemScale = BoostRow->ItemScale;
            HandSocketName = BoostRow->HandSocketName;
            SetItemName(BoostRow->ItemName);
			if (HasAuthority())
			{
            	SetItemCount(BoostRow->ItemCount);
				BoostItemAmount = BoostRow->ItemCount;
			}
            SetIconItem(BoostRow->InventoryIcon);
            SetIconBackground(BoostRow->BoostIconBackground);
            SetIconAmmo(BoostRow->AmmoIcon);
            SetMaterialInstance(BoostRow->MaterialInstance);
            PreviousMaterialIndex = GetMaterialIndex();
            GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr); // clear the previous material inst before setting new material
            SetMaterialIndex(BoostRow->MaterialIndex);
            ItemMontageSection = BoostRow->ItemMontageSection;
            ItemMontageEffect = BoostRow->ItemMontageEffect;
			ItemMontageSoundEffect = BoostRow->ItemMontageSoundEffect;
            GetItemMesh()->SetAnimInstanceClass(BoostRow->AnimBP);
            //FireSound = BoostRow->FireSound;
		}
	}  
}

void ABoostItem::OnRep_BoostType()
{
	UpdateBoost();
}

void ABoostItem::ThrowItem(bool bUnequipped, FTransform CharacterTransform, float ThrowImpulse, bool bRandomDirection)
{
	FQuat CharacterRot = CharacterTransform.GetRotation();
	float ThrowDirection = bRandomDirection ? 180.f : 60.f;
    FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
    FVector TargetLocation;

    // Set the location of the item when character gets killed
    if (bRandomDirection)
    {
		float RandomRotation = FMath::FRandRange(-ThrowDirection, ThrowDirection);
        GetItemMesh()->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		MeshRotation = FRotator{ 0.f, CharacterRot.Rotator().Yaw - 90 + RandomRotation, -90.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		TargetLocation = GetActorLocation() + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 200.f);
    }
	else
	{
		if (bUnequipped) GetItemMesh()->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
		
		ThrowImpulse = ThrowImpulse / 2.f;
		TargetLocation = GetActorLocation() + 100.f * CharacterRot.GetForwardVector() + FMath::FRandRange(-50.f, 50.f) * CharacterRot.GetRightVector() + FVector(0.f, 0.f, 50.f);
	}
	// Direction in which we throw the weapon
	FVector ImpulseDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();
	GetItemMesh()->AddImpulse(ThrowImpulse * ImpulseDirection);
	
	if (HasAuthority())
	{
		bFalling = true;
		bInAir = true;
	}
    EnableGlowMaterial();
}

void ABoostItem::StopFalling()
{
	Super::StopFalling();
	SetSlotIndex(0);
}

void ABoostItem::GrappleOnHit()
{
	SetItemState(EItemState::EIS_Equipped);
	auto ShooterChar = Cast<AShooterCharacter>(GetOwner());
	if (ShooterChar && ShooterChar->GetBoost())
	{
		GrappleState = EGrappleState::EGS_Hit;
		OnRep_GrappleState();
		ShooterChar->GrapplePull(this);
	}	
}

void ABoostItem::OnEquipped()
{
	Super::OnEquipped();

	Sequence = 0;
}

void ABoostItem::SetHUDBoost(int32 InBoost)
{
	//UE_LOG(LogTemp, Warning, TEXT("SetHUDBoost"));
	auto ShooterOwnerCharacter = Cast<AShooterCharacter>(GetOwner());
	if (ShooterOwnerCharacter && ShooterOwnerCharacter->IsLocallyControlled())
	{
		//UE_LOG(LogTemp, Warning, TEXT("SetHUDBoost checked"));
		ShooterOwnerCharacter->GetHudAmmoDelegate().Broadcast(InBoost);
	}
}

void ABoostItem::SpendBoost()
{
	BoostItemAmount = BoostItemAmount - 1;
	SetHUDBoost(BoostItemAmount);
	//SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateBoost(BoostItemAmount);
	}
	else
	{
		++Sequence;
	}
}

void ABoostItem::ClientUpdateBoost_Implementation(int32 ServerBoost)
{
	if (HasAuthority()) return;
	BoostItemAmount = ServerBoost;
	--Sequence;
	BoostItemAmount -= Sequence;
	SetHUDBoost(BoostItemAmount);
}

void ABoostItem::AddBoost(int32 BoostToAdd, AShooterCharacter* ShooterOwnerCharacter)
{
	BoostItemAmount = BoostItemAmount + BoostToAdd;
	//if (ShooterOwnerCharacter) UE_LOG(LogTemp, Warning, TEXT("ShooterOwnerCharacter = %s"), *ShooterOwnerCharacter->GetName());
	//ClientAddBoost(BoostToAdd, ShooterOwnerCharacter);
	if (ShooterOwnerCharacter)
	{
		auto EquippedBoost = Cast<ABoostItem>(ShooterOwnerCharacter->GetEquippedItem());
		if (EquippedBoost && EquippedBoost == this) SetHUDBoost(BoostItemAmount);
	}
}

void ABoostItem::ClientAddBoost_Implementation(int32 BoostToAdd, AShooterCharacter* InShooterCharacter)
{
	//if (HasAuthority()) return;
	//BoostItemAmount = BoostItemAmount + BoostToAdd;
	auto EquippedBoost = Cast<ABoostItem>(InShooterCharacter->GetEquippedItem());
	if (EquippedBoost && EquippedBoost == this) SetHUDBoost(BoostItemAmount);

	//if (InShooterCharacter) 
	//{
	//	InShooterCharacter->GetHudUpdateAmmoDelegate().Broadcast();
	//	UE_LOG(LogTemp, Warning, TEXT("InShooterCharacter = %s"), *InShooterCharacter->GetName());
	//}
	//SetHUDBoost(BoostItemAmount);
}

void ABoostItem::OnRep_BoostItemAmount()
{
	if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(GetOwner()))
	{
		ShooterInterface->Execute_OnUpdateHUDBoostAmount(GetOwner(), BoostItemAmount, this);
	}
}

void ABoostItem::OnRep_ItemState()
{
	Super::OnRep_ItemState();

}

void ABoostItem::OnRep_GrappleState()
{
	//auto ShooterChar = Cast<AShooterCharacter>(GetOwner());
	ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterCharacter;
	switch (GrappleState)
	{
	case EGrappleState::EGS_Start:
		{	

		}
		break;
	case EGrappleState::EGS_Hit:
		{
			if (GrappleEffect)
			{
				GetItemMesh()->SetVisibility(false);
				/*
				GrappleEffectComponent = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					GrappleEffect,
					GetActorLocation(),
					GetActorRotation(),
					false // auto destroy after playing
				);
				*/
				UGameplayStatics::SpawnEmitterAttached(
					GrappleEffect,
					GetItemMesh(),
					FName(""),
					FVector(0,0,0),
					FRotator(0,0,0),
					EAttachLocation::KeepRelativeOffset, 
					true
				);
			}
			//UE_LOG(LogTemp, Warning, TEXT("Boost Location = %s"), *GetActorLocation().ToString());
			if (ShooterCharacter) ShooterCharacter->SetGrappleEffect(true);	
		}
		break;
	case EGrappleState::EGS_Release:
		if (EndGrappleSound && EndGrappleEffect)
		{
			UGameplayStatics::SpawnSoundAttached(
				EndGrappleSound, // sound cue (USoundBase)
				GetItemMesh(), // mesh to attach to
				FName(""),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);

			UGameplayStatics::SpawnEmitterAttached(
				EndGrappleEffect,
				GetItemMesh(),
				FName(""),
				FVector(0,0,0),
				FRotator(0,0,0),
				EAttachLocation::KeepRelativeOffset, 
				true
			);
		}

		if (HasAuthority())
		{
			FTimerHandle EndGrappleTimer;
			GetWorldTimerManager().SetTimer(EndGrappleTimer, this, &ABoostItem::EndGrappleSystem, 0.4f);
		}
		break;
	}
}

void ABoostItem::EndGrappleSystem()
{
	Destroy();
}

void ABoostItem::Destroyed()
{
	Super::Destroyed();

	ShooterCharacter = ShooterCharacter == nullptr ? Cast<AShooterCharacter>(GetOwner()) : ShooterCharacter;

	if (GrappleEffectComponent)
	{
		GrappleEffectComponent->DestroyComponent();
	}

	//if (ShooterCharacter) ShooterCharacter->SetGrappleEffect(false);
}

void ABoostItem::OnItemPickedUp(AShooterCharacter* InShooter)
{
	if (InShooter == nullptr || InShooter->GetBoost() == nullptr) return;

	bool bNewItem;
	bNewItem = !InShooter->BoostInInventory(this);
	if (InShooter->Inventory.Num() >= InShooter->GetInventoryMaxCapacity() && bNewItem)
	{
		InShooter->SwapItem(nullptr, this);
	}
	else if (InShooter->Inventory.Num() < InShooter->GetInventoryMaxCapacity() && bNewItem)
	{
		SetSlotIndex(InShooter->Inventory.Num());
		InShooter->Inventory.Add(this);
		SetItemState(EItemState::EIS_PickedUp);
	}
	else if (!bNewItem)
	{
		SetItemState(EItemState::EIS_PickedUp);
	}

	InShooter->GetBoost()->TakeItem(this, bNewItem);
}

void ABoostItem::OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance)
{
	if (InShooter == nullptr) return;

	if (!InShooter->GetUnEquippedState())
	{
		if (InAnimInstance && InShooter->GetEquipMontage())
		{
			InAnimInstance->Montage_Play(InShooter->GetEquipMontage(), 1.0f);
			InAnimInstance->Montage_JumpToSection(FName("Equip"));
		}	
		SetItemState(EItemState::EIS_Equipped);
		if (InShooter->IsLocallyControlled())
		{
			InShooter->HudAmmoDelegate.Broadcast(GetBoostItemAmount());
			InShooter->GetHudCarriedAmmoDelegate().Broadcast();
		}
		if (GetBoostType() == EBoostType::EBT_Grapple)
		{
			InShooter->SetGrappleItemEquipped(true);
		}
		else
		{
			InShooter->SetGrappleItemEquipped(false);
		}
	}
    const USkeletalMeshSocket* HandSocket = InShooter->GetMesh()->GetSocketByName(this == nullptr ? FName("RightHandSocket") : GetHandSocketName());
    if (HandSocket) HandSocket->AttachActor(this, InShooter->GetMesh());
}

void ABoostItem::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
	float ThrowPower = 10000.f;
	if (InShooter)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		if (HasAuthority())
		{
			FTransform StartTransform = InShooter->GetCapsuleComponent()->GetComponentTransform();
			StartTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			SetItemState(EItemState::EIS_Falling);
			ThrowItem(InShooter->GetUnEquippedState(), StartTransform, ThrowPower/(15.32f), bPlayerElimed);
		}
	}
	else
	{
		if (HasAuthority())
		{
			CharacterTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			SetItemState(EItemState::EIS_Falling);
			ThrowItem(false, CharacterTransform, ThrowPower/(15.32f), true);
		}
	}
	EnableGlowMaterial();
	StartPulseTimer();
}

bool ABoostItem::IsAnItem()
{
	return true;
}

void ABoostItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, BoostItemAmount);
	DOREPLIFETIME(ThisClass, BoostType);
	DOREPLIFETIME(ThisClass, GrappleState);
}
