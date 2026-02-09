// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterCharacter.h"
#include "Sound/SoundCue.h"
#include "Camera/CameraComponent.h"
#include "Curves/CurveVector.h"
//#include "Items/Weapons/MissileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"

#define ECC_Helicopter ECollisionChannel::ECC_GameTraceChannel6
//#define ECC_DentDestructable ECollisionChannel::ECC_GameTraceChannel7
// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();

	//Hide the weapon widget
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
	
	// Sets ActiveStars array based on Item Rarity
	SetActiveStars();

	// Setup overlap for AreaSphere
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);

	// Set item properties based on ItemState
	SetItemProperties();

	// Set custom depth to disabled
	InitializeCustomDepth();

	if (ItemState == EItemState::EIS_Pickup && ItemType != EItemType::EIT_Boost)
	{
		StartPulseTimer();
	}
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle Item Interping when in the EquipInterping state
	ItemInterp(DeltaTime); 

	// Get curve values from PulseCurve and set dynamic material parameters
	if (ItemType != EItemType::EIT_Boost) UpdatePulse();

}

void AItem::UpdateItem()
{
	// Load the data in the Item Rarity Data Table

	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	FString RarityTablePath(TEXT("DataTable'/Game/_Game/DataTables/ItemRarity_DT.ItemRarity_DT'"));
	UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
	if (RarityTableObject)
	{
		FItemRarityTable* RarityRow = nullptr;
		switch (ItemRarity)
		{
			case EItemRarity::EIR_Damaged:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
			break;

			case EItemRarity::EIR_Common:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
			break;

			case EItemRarity::EIR_Uncommon:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Uncommon"), TEXT(""));
			break;

			case EItemRarity::EIR_Rare:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
			break;

			case EItemRarity::EIR_Legendary:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
			break;

			case EItemRarity::EIR_NFT:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("NFT"), TEXT(""));
			break;
		}
		if (RarityRow)
		{
			GlowColor = RarityRow->GlowColor;
			LightColor = RarityRow->LightColor;
			DarkColor = RarityRow->DarkColor;
			NumberOfStars = RarityRow->NumberOfStars;
			IconBackground = RarityRow->IconBackground;
			RarityDamagePercentage = RarityRow->RarityDamagePercentage;
			if (GetItemMesh())
			{
				GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
			}
		}
	}

	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		if (DynamicMaterialInstance)
		{
			DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
			ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
			EnableGlowMaterial();
		}
	}
	SetActiveStars();
}

void AItem::OnRep_ItemRarity()
{
	UpdateItem();
}

//This function gets called when item changes or when item is moved around in the world
void AItem::OnConstruction(const FTransform& Transform)
{
	UpdateItem();
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// OtherActor is the actor that overlaps with the sphere and triggers the event
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCount(1);
			ShooterCharacter->UnHighlightInventorySlot();
			AShooterPlayerController* ShooterPlayerController = ShooterCharacter->GetShooter_PC();
			if (ShooterPlayerController && ShooterPlayerController->KeyNameMap.Contains(FName(TEXT("Pick Up Weapon / Item"))))
			{
				UpdateSelectKey(*ShooterPlayerController->KeyNameMap.Find(FName(TEXT("Pick Up Weapon / Item"))));
			}
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCount(-1);
			ShooterCharacter->UnHighlightInventorySlot();
		}
	}
}

void AItem::SetActiveStars()
{
	// the 0 element isn't used
	for (int32 i=0; i<=5; i++)
	{
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
		case EItemRarity::EIR_Damaged:
			ActiveStars[1] = true;
			break;
		case EItemRarity::EIR_Common:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			break;
		case EItemRarity::EIR_Uncommon:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			break;
		case EItemRarity::EIR_Rare:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			break;
		case EItemRarity::EIR_Legendary:
			ActiveStars[1] = true;
			ActiveStars[2] = true;
			ActiveStars[3] = true;
			ActiveStars[4] = true;
			ActiveStars[5] = true;
			break;
	}
}

void AItem::SetItemProperties()
{
	switch (ItemState)
	{
		case EItemState::EIS_Pickup:
			//MissileMovementComponent->Deactivate();
			// Set Mesh Properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetNotifyRigidBodyCollision(false);
			// Set AreaSphere Properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//NetCullDistanceSquared = 15000.f * 15000.f;
			break;

		/* This is a duplicate of EIS_Pickup. This is used here because when a weapon a spawned in the map, 
		the OnRep_ItemState in the Weapon class needs to be triggered to have the Ammo be replicated to all clients. 
		This is triggred when the state changes from EIS_Pickup2 to EIS_Pickup
		*/
		case EItemState::EIS_Pickup2:
			//MissileMovementComponent->Deactivate();
			// Set Mesh Properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ItemMesh->SetNotifyRigidBodyCollision(false);
			// Set AreaSphere Properties
			//AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//NetCullDistanceSquared = 15000.f * 15000.f;
			break;

		case EItemState::EIS_Equipped:
			//MissileMovementComponent->Deactivate();
			PickupWidget->SetVisibility(false); // Hides the widget for the weapon that's equipped
			// Set Mesh Properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set AreaSphere Properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			//NetCullDistanceSquared = 25000.f * 25000.f;
			DisableGlowMaterial();
			DisableCustomDepth();

			OnEquipped();

			break;

		case EItemState::EIS_Falling:
			//PickupWidget->SetVisibility(false);
			// Set Mesh Properties
			if (GetItemMesh()->GetAttachParent() != nullptr)
			{
				//UE_LOG(LogTemp, Warning, TEXT("Detaching ItemMesh from %s on client"), *GetItemMesh()->GetAttachParent()->GetName());
				FDetachmentTransformRules DetachmentRules(EDetachmentRule::KeepWorld, true);
				GetItemMesh()->DetachFromComponent(DetachmentRules);
			}
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetEnableGravity(true);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			ItemMesh->SetCollisionResponseToChannel(ECC_Helicopter, ECollisionResponse::ECR_Block);
			//ItemMesh->SetCollisionResponseToChannel(ECC_DentDestructable, ECollisionResponse::ECR_Block);
			ItemMesh->SetNotifyRigidBodyCollision(true);
			// Set AreaSphere Properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			//NetCullDistanceSquared = 15000.f * 15000.f;
			//MissileMovementComponent->Activate();
			break;

		case EItemState::EIS_EquipInterping:
			PickupWidget->SetVisibility(false); // Hides the widget for the weapon that's equipped
			// Set Mesh Properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(true);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set AreaSphere Properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			//MissileMovementComponent->Deactivate();
			//NetCullDistanceSquared = 15000.f * 15000.f;
			break;
		case EItemState::EIS_PickedUp:
			{
				//MissileMovementComponent->Deactivate();
				PickupWidget->SetVisibility(false); // Hides the widget for the weapon that's equipped
				// Set Mesh Properties
				ItemMesh->SetSimulatePhysics(false);
				ItemMesh->SetEnableGravity(false);
				ItemMesh->SetVisibility(false);
				ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				
				if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(GetOwner()))
				{
					ShooterInterface->Execute_OnItemPickedUp(GetOwner(), this, TEXT("SKM_Item"));
				}
				
				// Set AreaSphere Properties
				AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				// Set CollisionBox Properties
				CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
				CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				//NetCullDistanceSquared = 10000.f * 10000.f;
				break;		
			}

		case EItemState::EIS_Unequipped:
			//MissileMovementComponent->Deactivate();
			PickupWidget->SetVisibility(false); // Hides the widget for the weapon that's equipped
			// Set Mesh Properties
			ItemMesh->SetSimulatePhysics(false);
			ItemMesh->SetEnableGravity(false);
			ItemMesh->SetVisibility(false);
			ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set AreaSphere Properties
			AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Set CollisionBox Properties
			CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			//NetCullDistanceSquared = 10000.f * 10000.f;
			break;
			
	}
}

void AItem::OnEquipped()
{
}

void AItem::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemProperties();
}

void AItem::StartItemCurve(AShooterCharacter* Char, bool bIsSwappingCurrentItem)
{
	if(ItemState == EItemState::EIS_EquipInterping || !Char) return;

	if (bIsSwappingCurrentItem)
	{
		Char->ServerSetCurrentItemToSwap(Char->GetEquippedItem());
	}
	// Store a handle to the Character
	Character = Char;
	
	// Get array index in the InterpLocations with the lowest item count
	InterpLocIndex = Character->GetInterpLocationIndex();
	//Add 1 to the Item Count for this interp location struct
	Character->IncrementInterpLocItemCount(InterpLocIndex, 1);

	if (PickupSound)
	{
		UGameplayStatics::PlaySound2D(Char, PickupSound);
	}

	// Store initial location of the Item
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);

	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItem::FinishInterping, ZCurveTime);

	// Get initial Yaw of the Camera
	const float CameraRotationYaw{ float(Character->GetFollowCamera()->GetComponentRotation().Yaw) };
	// Get initial Yaw of the Item
	const float ItemRotationYaw{ float(GetActorRotation().Yaw) };
	// Initial Yaw offset between Camera and Item
	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;
}

void AItem::FinishInterping()
{
	bInterping = false;
	if (Character)
	{
		// Subtract 1 from the ItemCount of the interp location struct
		Character->IncrementInterpLocItemCount(InterpLocIndex, -1);
		
		// Call the ServerRPC's implementation and it will resolve self call as RPC when needed
		// if(Character->IsLocallyControlled())
		Character->GetPickupItem_Implementation(this);

		Character->UnHighlightInventorySlot();
	}
	// Set scale back to normal
	SetActorScale3D(FVector(1.f));

	DisableGlowMaterial();
	DisableCustomDepth();
}

FVector AItem::GetInterpLocation()
{
	if (Character == nullptr) return FVector(0.f);

	switch (ItemType)
	{
		case EItemType::EIT_Ammo:
			return Character->GetInterpLocation(InterpLocIndex).SceneComponent->GetComponentLocation();
		break;

		case EItemType::EIT_Weapon:
			return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();

		case EItemType::EIT_Boost:
			return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
		break;
	}

	return FVector();
}

void AItem::ItemInterp(float DeltaTime)
{
	if (!bInterping) return; // !bInterping means "not bInterping"

	if (Character && ItemZCurve)
	{
		// Time that has passed since we started out timer ItemInterpTimer
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		// Get curve value corresponding to ElapsedTime (like f(t))
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);
		// Get the item's initial location when the curve started
		FVector ItemLocation = ItemInterpStartLocation;
		// Get location in front of the camera (target location)
		const FVector CameraInterpLocation{ GetInterpLocation() };

		// Vector from Item to Camera Interp Location, X and Y are zeroed out
		const FVector ItemToCamera{FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z)};
		// scale factor to multiply with CurveValue
		const float DeltaZ = ItemToCamera.Size();
		
		const FVector CurrentLocation{ GetActorLocation() };
		// Interpolated X value
		const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.0f);
		// Interpolated Y value
		const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.0f);
		
		//Set X and Y of ItemLocation to Interped values
		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;

		// Adding curve value to the Z component of the Initial Location (scaled by DeltaZ)
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

		// Get the rotation of the camera this frame
		const FRotator CameraRotation{ Character->GetFollowCamera()->GetComponentRotation() };
		// Camera rotation plus initial Yaw offset
		FRotator ItemRotation{ 0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f };
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

		if (ItemScaleCurve)
		{
			const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			//SetActorScale3D(ScaleCurveValue * GetActorScale3D()); // Use this if item is originally not scaled to 1
			// SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
		}
		
	}
}

void AItem::EnableCustomDepth()
{
	ItemMesh->SetRenderCustomDepth(true);
}

void AItem::DisableCustomDepth()
{
	ItemMesh->SetRenderCustomDepth(false);
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
	//DisableGlowMaterial();
}

void AItem::EnableGlowMaterial()
{
	if (DynamicMaterialInstance)
	{
		// GLowBlendAlpha is name of parameter we created in M_SMG_Mat
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}

void AItem::DisableGlowMaterial()
{
	if (DynamicMaterialInstance)
	{
		// GLowBlendAlpha is name of parameter we created in M_SMG_Mat
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
	}	
}

void AItem::StartPulseTimer()
{
	if (ItemState == EItemState::EIS_Pickup)
	{
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer, PulseCurveTime);
	}
}

void AItem::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItem::UpdatePulse()
{
	if (ItemState == EItemState::EIS_Pickup || ItemState == EItemState::EIS_Pickup2)
	{
		const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(PulseTimer) };
		if (PulseCurve)
		{
			const FVector CurveValue{ PulseCurve->GetVectorValue(ElapsedTime) };

			DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
			DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
			DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
		}
	}
}

void AItem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bFallingFromContainer)
	{
		bFallingFromContainer = false;
		SetItemState(EItemState::EIS_Pickup);
	}
	else
	{
		if (bInAir)
		{
			bInAir = false;
			GetWorldTimerManager().SetTimer(VelocityCheckTimer, this, &AItem::CheckIfSettled, 0.1f, true);
		}
	}
}

void AItem::StopFalling()
{
    bFalling = false;
    SetItemState(EItemState::EIS_Pickup);
	SetOwner(nullptr);
    StartPulseTimer();
}

void AItem::CheckIfSettled()
{
    if (GetItemMesh() && GetItemMesh()->GetPhysicsLinearVelocity().Size() < 5.f)
    {
        // It's settled—clear the timer and finalize
        GetWorldTimerManager().ClearTimer(VelocityCheckTimer);
        StopFalling();
    }
}

void AItem::OnItemPickedUp(AShooterCharacter* InShooter)
{
	// no default behavior
}

void AItem::OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance)
{
	// no default behavior
}

void AItem::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
	// no default behavior
}

bool AItem::IsAnItem()
{
	return false;
}

UStaticMeshComponent* AItem::GetNFTMesh()
{
	return nullptr;
}

// Replication and Networking

void AItem::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ItemState);
	DOREPLIFETIME(ThisClass, SlotIndex);
	DOREPLIFETIME(ThisClass, ItemCount);
	DOREPLIFETIME(ThisClass, ItemRarity);
}

void AItem::OnRep_ItemState()
{
	//UE_LOG(LogTemp, Warning, TEXT("OnRep_ItemState = %i"), ItemState);
	SetItemProperties();
}
