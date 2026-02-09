// Fill out your copyright notice in the Description page of Project Settings.


#include "DummyItem.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Shooter/ShooterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"

ADummyItem::ADummyItem()
{
	bReplicates = true;

    MainComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(MainComponent);
    NetCullDistanceSquared = 15000 * 15000; // 150m relevancy

    GetItemMesh()->SetAutoActivate(false);
    GetAreaSphere()->SetAutoActivate(false);
    GetCollisionBox()->SetAutoActivate(false);
    GetPickupWidget()->SetAutoActivate(false);
}

void ADummyItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    GetItemMesh()->Deactivate();
    GetAreaSphere()->Deactivate();
    GetCollisionBox()->Deactivate();
    GetPickupWidget()->Deactivate();

	UpdateDummyItem();
}

void ADummyItem::UpdateDummyItem()
{
    SetItemType(EItemType::EIT_Dummy);
}

void ADummyItem::SetItemProperties()
{
    // Don't change properties for dummy item
}

void ADummyItem::OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance)
{
	if (InShooter == nullptr) return;

	if (InShooter->GetUnEquippedState())
	{
		SetItemState(EItemState::EIS_PickedUp);
		if (InShooter->IsLocallyControlled())
		{
			InShooter->HudAmmoDelegate.Broadcast(-1);
            InShooter->GetHudCarriedAmmoDelegate().Broadcast();
			//FinishEquipping();
		}
	}
    InShooter->InventorySelect = 0;
    const USkeletalMeshSocket* HandSocket = InShooter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
    if (HandSocket) HandSocket->AttachActor(this, InShooter->GetMesh());
}

