// Fill out your copyright notice in the Description page of Project Settings.


#include "BoostComponent.h"
#include "ShooterCharacter.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "BoostItem.h"
#include "Async/Async.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/TargetPoint.h"
#include "Shooter/EnumTypes/MovementModifiers.h"
#include "Shooter/ShooterMovementComponent.h"
#include "Shooter/ShooterGameState.h"
//#include "CableComponent.h"


UBoostComponent::UBoostComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBoostComponent::BeginPlay()
{
	Super::BeginPlay();

	//InitializeTeleportLocations();
}

void UBoostComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBoostComponent::StartBoost()
{
	ABoostItem* BoostItem = Cast<ABoostItem>(Character->GetEquippedItem());
	bool bStopBoost = Character == nullptr || BoostItem == nullptr || Character->GetStartFlying() ||
			Character->GetCombatState() != ECombatState::ECS_Unoccupied || 
			BoostItem->GetBoostItemAmount() <= 0 || (Character->GetGhostMode() && BoostItem->GetBoostType() == EBoostType::EBT_Ghost) ||
			(Character->GetBoostProtect() && BoostItem->GetBoostType() == EBoostType::EBT_Protect) || 
			(Character->GetSlowDown() && BoostItem->GetBoostType() == EBoostType::EBT_Slow) ||
			Character->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying || Character->GetSprinting();
	if (bStopBoost) return;

	if (Character->bHandShieldButtonPressed)
	{
		Character->bHandShieldButtonPressed = false;
		Character->ShieldOn(false);
	}
	else if (Character->bAimingButtonPressed)
	{
		Character->bAimingButtonPressed = false;
		Character->SetAiming(false);
	}
	
	if (BoostItem->GetBoostType() != EBoostType::EBT_Grapple && 
			BoostItem->GetBoostType() != EBoostType::EBT_SuperPunch &&
			BoostItem->GetBoostType() != EBoostType::EBT_SuperJump && 
			Character->GetHoverState() == EHoverState::EHS_HoverFinish)
	{
		if (Character->GetCharacterMovement()->IsFalling()) return;
		if (Character->GetCrouching()) Character->CrouchButtonPressed();
		Character->SetCombatStateLocal(ECombatState::ECS_FireTimerInProgress);
		Character->SetCombatState(ECombatState::ECS_FireTimerInProgress);
		if (Character->IsLocallyControlled() && Character->GetShooterMovementComponent())
		{
			Character->GetShooterMovementComponent()->WantsState = EMovementModifiers::EMM_Stop;
		}
		if (!Character->HasAuthority()) 
		{	
			Character->bUsingItem = true;
			Character->PlayItemMontage(BoostItem->GetItemMontageSection());
		}
		ServerStartBoost(BoostItem->GetItemMontageSection(), BoostItem);
	}
	else if (BoostItem->GetBoostType() == EBoostType::EBT_Grapple && !Character->bGrappleThrown)
	{
		if (BoostItem->GrappleState == EGrappleState::EGS_Start) return;
		FHitResult TraceHit;
		TraceHit = Character->GrappleTraceHit();

		Character->SetCombatStateLocal(ECombatState::ECS_FireTimerInProgress);
		Character->SetCombatState(ECombatState::ECS_FireTimerInProgress);
		if (!Character->HasAuthority())
		{
			//Character->bGrappleThrown = true;
			//Character->PlayItemMontage(BoostItem->GetItemMontageSection());
			//StartGrappleLocal(BoostItem, TraceHit);
		}
		ServerStartGrapple(BoostItem, TraceHit);

	}
	else if (BoostItem->GetBoostType() == EBoostType::EBT_SuperPunch)
	{
		if (!Character->GetCharacterMovement()->IsFalling()) return;
		Character->SetCombatStateLocal(ECombatState::ECS_FireTimerInProgress);
		Character->SetCombatState(ECombatState::ECS_FireTimerInProgress);
		FString NameStr = BoostItem->GetItemMontageSection().ToString(); // Convert FName to FString
		TArray<FString> PunchArray;
		NameStr.ParseIntoArray(PunchArray, TEXT("/"), true);
		if (!Character->HasAuthority()) 
		{	
			Character->bSuperPunch = true;
			Character->PlayItemMontage(FName(*PunchArray[0]), false);
		}
		ServerStartSuperPunch(FName(*PunchArray[0]), BoostItem);
	}
	else if (BoostItem->GetBoostType() == EBoostType::EBT_SuperJump)
	{
		if (Character->GetCharacterMovement()->IsFalling()) return;
		// Setting combat state only locally here because jump happens so fast
		// and it's fine to just set it locally (to prevent multiple jumps during lag and button spamming)
		Character->SetCombatStateLocal(ECombatState::ECS_FireTimerInProgress);
		//Character->SetCombatState(ECombatState::ECS_FireTimerInProgress);
		if (!Character->HasAuthority()) 
		{	
			Character->PlayItemMontage(BoostItem->GetItemMontageSection());
		}
		ServerStartSuperJump(BoostItem->GetItemMontageSection());	
	}
}

void UBoostComponent::ServerStartBoost_Implementation(FName ItemMontageSection, ABoostItem* BoostItem)
{
	
	if (Character && BoostItem->GetBoostType() != EBoostType::EBT_Grapple)
	{
		Character->bUsingItem = true;
		//OnRep_UsingItem();
	}
	
	if (Character)
	{
		//Character->GetCharacterMovement()->MaxWalkSpeed = 0.f;
		//Character->TargetMovementSpeed = 0.f;
	}
	MulticastStartBoost(ItemMontageSection, BoostItem->GetBoostType());
}

void UBoostComponent::MulticastStartBoost_Implementation(FName ItemMontageSection, EBoostType InBoostType)
{
	if (Character == nullptr) return;
	if (Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	//Character->SetCombatState();
	Character->PlayItemMontage(ItemMontageSection);
}

void UBoostComponent::ServerStartSuperPunch_Implementation(FName ItemMontageSection, ABoostItem* BoostItem)
{
	Character->bSuperPunch = true;
	MulticastStartSuperPunch(ItemMontageSection, BoostItem);
}

void UBoostComponent::MulticastStartSuperPunch_Implementation(FName ItemMontageSection, ABoostItem* BoostItem)
{
	if (Character == nullptr) return;
	if (Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	Character->PlayItemMontage(ItemMontageSection, false);
}

void UBoostComponent::ServerStartSuperJump_Implementation(FName ItemMontageSection)
{
	if (Character == nullptr) return;
	MulticastStartSuperJump(ItemMontageSection);
	Character->BoostItemMontageFinished();
}

void UBoostComponent::MulticastStartSuperJump_Implementation(FName ItemMontageSection)
{
	if (Character == nullptr) return; 
	if (Character->IsLocallyControlled()) Character->SetCombatStateLocal(ECombatState::ECS_Unoccupied);
	if (Character->IsLocallyControlled() && !Character->HasAuthority()) return;
	//Character->SetCombatState();

	Character->PlayItemMontage(ItemMontageSection);
}

/*
void UBoostComponent::OnRep_UsingItem()
{
	if (Character == nullptr) return;
	if (Character->bUsingItem)
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_UsingItem"));
		Character->GetCharacterMovement()->MaxWalkSpeed = 0.f;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_NotUsingItem"));
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->GetBaseMovementSpeed();
	}
}
*/

void UBoostComponent::ItemEffect()
{
	if (Character == nullptr) return;
	ABoostItem* BoostItem = Cast<ABoostItem>(Character->GetEquippedItem());
	if (BoostItem == nullptr) return;
	UWorld* World = GetWorld();
	if (BoostItem->GetItemMontageEffect() && World)
	{
		ItemEffectComponent = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			BoostItem->GetItemMontageEffect(),
			Character->GetActorLocation(),
			Character->GetActorRotation(),
			true // auto destroy after playing
		);
	}
}

void UBoostComponent::ItemEffectSound()
{
	if (Character == nullptr) return;
    ABoostItem* BoostItem = Cast<ABoostItem>(Character->GetEquippedItem());
    if (BoostItem == nullptr) return;
    if (BoostItem->GetItemMontageSoundEffect())
    {
        // ItemEffectSoundComponent is a UAudioComponent, create it in .h (add #include "Components/AudioComponent.h")

		ItemEffectSoundComponent = UGameplayStatics::SpawnSoundAttached(
					BoostItem->GetItemMontageSoundEffect(), // sound cue (USoundBase)
					Character->GetMesh(), // mesh to attach to
					FName("Root"),   //socket name
					FVector(0,0,0),  //location relative to socket
					FRotator(0,0,0), //rotation 
					EAttachLocation::KeepRelativeOffset, 
					true //if true, will be deleted automatically
			);
    }
}

void UBoostComponent::UseBoost(ABoostItem* BoostItem)
{
	//ABoostItem* BoostItem = Cast<ABoostItem>(Character->GetEquippedItem());
	if (Character == nullptr || BoostItem == nullptr) return;

	switch (BoostItem->GetBoostType())
	{
	case EBoostType::EBT_HealthV1:
		Heal(BoostItem->GetAmountToHealV1(), BoostItem->GetHealingTimeV1());
		break;
	case EBoostType::EBT_HealthV2:
		Heal(BoostItem->GetAmountToHealV2(), BoostItem->GetHealingTimeV2());
		break;
	case EBoostType::EBT_ShieldV1:
		AddShield(BoostItem->GetAmountToReplenishV1(), BoostItem->GetReplenishTimeV1());
		break;
	case EBoostType::EBT_ShieldV2:
		AddShield(BoostItem->GetAmountToReplenishV2(),  BoostItem->GetReplenishTimeV2());
		break;
	case EBoostType::EBT_Ghost:
		Character->StartGhost();
		break;
	case EBoostType::EBT_SuperJump:
		SuperJump();
		break;
	case EBoostType::EBT_Teleport:
		SemiRandomTeleport();
		break;
	case EBoostType::EBT_Protect:
		Character->StartProtect();
		break;
	case EBoostType::EBT_Fly:
		Character->StartFlying();
		break;
	case EBoostType::EBT_Slow:
		Character->StartSlowMo();
		break;
	case EBoostType::EBT_Copy:
		Character->SpawnCopy();
		break;
	case EBoostType::EBT_Grapple:
		break;
	default:
		break;
	}
	//RemoveItem();
}

void UBoostComponent::RemoveItem(ABoostItem* BoostItem)
{
	if (Character == nullptr || BoostItem == nullptr) return;

	if (BoostItem->GetBoostItemAmount() <= 0)
	{
		Character->Inventory.Remove(BoostItem);
		UpdateSlotIndex();

		if (BoostItem->GetSlotIndex() == Character->Inventory.Num())
		{
			// Equip item to the left of BoostItem if BoostItem is the last item in inventory
			ClientInventoryAnim(BoostItem->GetSlotIndex(), BoostItem->GetSlotIndex() - 1);
			Character->Server_ExchangeInventoryItems(BoostItem->GetSlotIndex(), BoostItem->GetSlotIndex() - 1);
		}
		else
		{
			ClientInventoryAnim(BoostItem->GetSlotIndex() + 1, BoostItem->GetSlotIndex());
			Character->Server_ExchangeInventoryItems(BoostItem->GetSlotIndex() + 1, BoostItem->GetSlotIndex(), true);
		}
		if (BoostItem->GetBoostType() != EBoostType::EBT_Grapple) BoostItem->Destroy();
		
	}
	else
	{
		// Only run this for the Grapple boost (for now)
		EBoostType OldBoostType = BoostItem->GetBoostType();
		if (OldBoostType != EBoostType::EBT_Grapple) return;
		// Code that destroy's the boost that is used, and then spawns and equips a new one
		UClass* BoostClass = BoostItem->GetClass();
		int32 OldSlotIndex = BoostItem->GetSlotIndex();
		
		BoostItem->Destroy();
		Character->SetCombatState(ECombatState::ECS_Equipping);
		UWorld* World = GetWorld();
		if (World)
		{
			ABoostItem* NewBoost = World->SpawnActor<ABoostItem>(BoostClass, Character->GetActorTransform());
			NewBoost->SetBoostType(OldBoostType);
			NewBoost->UpdateBoost();
			NewBoost->SetSlotIndex(OldSlotIndex);
			Character->Inventory[OldSlotIndex] = NewBoost;
			Character->EquipItem(NewBoost, false, true);
		}
	}
}

void UBoostComponent::ClientInventoryAnim_Implementation(int32 CurrentIndex, int32 NewIndex)
{
	if (Character == nullptr) return;
	Character->LocalInventoryAnim(CurrentIndex, NewIndex);
}

ItemAsync::ItemAsync(ABoostItem* BoostItem)
{
	BoostItemAsync = BoostItem;
}

void UBoostComponent::RunUseBoost(ABoostItem* BoostItem)
{
	ItemAsync* task = new ItemAsync(BoostItem);
	task->DoWork(this);
}

void ItemAsync::DoWork(UBoostComponent* BoostClass)
{
	BoostClass->UseBoost(BoostItemAsync);
}

// Not being used. Calling ServerBoostItemMontageFinished in ShooterCharacter instead.
void UBoostComponent::ItemMontageFinished()
{
	if (Character == nullptr) return;
	EquippedBoost = Cast<ABoostItem>(Character->GetEquippedItem());
	if (EquippedBoost == nullptr) return;

	if (!Character->HasAuthority()) EquippedBoost->SpendBoost();
}

void UBoostComponent::UsingItemFinished(ABoostItem* BoostItem)
{
	//if (BoostItem) BoostItem->Destroy();
}

void UBoostComponent::Heal(float HealAmount, float HealingTime)
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBoostComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || Character == nullptr || (Character && Character->GetPlayerEliminated())) return; // or character is eliminated (check bool in character)

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth())); //Add this function in ShooterCharacter
	AmountToHeal -= HealThisFrame;
	//UE_LOG(LogTemp, Warning, TEXT("Health = %f"), Character->GetHealth());
	Character->HealthHudDelegate.Broadcast(Character->GetHealth() / Character->GetMaxHealth(), true);
	if (Character->HasAuthority()) Character->OnRep_Health(Character->GetHealth());

	if (AmountToHeal <= 0 || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;
		//UpdateBoostInventory();
	}
}

void UBoostComponent::AddShield(float ShieldAmount, float ShieldTime)
{
	bAddingShield = true;
	AddShieldRate = ShieldAmount / ShieldTime;
	ShieldAddAmount += ShieldAmount;
}

void UBoostComponent::ShieldRampUp(float DeltaTime)
{
	if (!bAddingShield || Character == nullptr || (Character && Character->GetPlayerEliminated())) return; // or character is eliminated (check bool in character)

	const float AddShieldThisFrame = AddShieldRate * DeltaTime;
	Character->SetShield(FMath::Clamp(Character->GetShield() + AddShieldThisFrame, 0.f, Character->GetMaxShield())); //Add this function in ShooterCharacter
	ShieldAddAmount -= AddShieldThisFrame;
	//UE_LOG(LogTemp, Warning, TEXT("Shield = %f"), Character->GetShield());
	Character->ShieldHudDelegate.Broadcast(Character->GetShield() / Character->GetMaxShield(), true);
	if (Character->HasAuthority()) Character->OnRep_Shield(Character->GetShield());

	if (ShieldAddAmount <= 0 || Character->GetShield() >= Character->GetMaxHealth())
	{
		bAddingShield = false;
		ShieldAddAmount = 0.f;
		//UpdateBoostInventory();
	}
}

void UBoostComponent::SuperJump()
{
    ACharacter* CharacterClass = Cast<ACharacter>(Character);
    if (CharacterClass)
    {
        Character->bSuperJump = true;
        FVector LaunchVel{Character->GetVelocity().X, Character->GetVelocity().Y, 4000.f};
        CharacterClass->LaunchCharacter(LaunchVel, false, false);
		Character->OnRep_SuperJump();
    }
}

void UBoostComponent::SemiRandomTeleport()
{
    if (Character == nullptr) return;
    FRotator TeleportRotation{0.f, FMath::RandRange(0.f, 360.f), 0.f};

	FVector_NetQuantize TeleportLocation = GetTeleportLocation();

	//const FVector CellSize = FVector(25600.f, 25600.f, 25600.f); // Set the size of each cell
	//const FVector CellIndex = FVector(FMath::FloorToInt(TeleportLocation.X / CellSize.X), FMath::FloorToInt(TeleportLocation.Y / CellSize.Y), FMath::FloorToInt(TeleportLocation.Z / CellSize.Z)); // Calculate the index of the cell

	//const FString LevelName = FString::Printf(TEXT("PartitionedLevel_Cell_%d_%d_%d"), CellIndex.X, CellIndex.Y, CellIndex.Z); // Generate the level name
	//UGameplayStatics::LoadStreamLevel(this, *LevelName, true, false, FLatentActionInfo());

	Character->SetActorLocationAndRotation(
		TeleportLocation,
		TeleportRotation,
		false
	);
}

/*
// Put in BeginPlay
void UBoostComponent::InitializeTeleportLocations()
{
    TeleportLocations.Add({-580.f, 2200.f, 90.f});
    TeleportLocations.Add({-300.f, 2200.f, 90.f});
    TeleportLocations.Add({-580.f, 1800.f, 90.f});
}
*/

// This function is no longer used. It was used when TargetPoint actors were used in the level (i.e., TeleportLocations)
int32 UBoostComponent::GetTeleportLocationIndex(TArray<AActor*> TeleportLocations)
{
    int32 TeleportIndex = FMath::RandRange(0, TeleportLocations.Num() - 1); 

	int32 TeleportLocationIter = 0;
    // Check if chosen teleport location is too close to current character location
    while ((TeleportLocations[TeleportIndex]->GetActorLocation() - Character->GetActorLocation()).Size() < 2000.f)
    {
        // If teleport location is too close, choose another random location
        TeleportIndex = FMath::RandRange(0, TeleportLocations.Num() - 1);
		TeleportLocationIter++;

		if (TeleportLocationIter >= TeleportLocations.Num())
		{
			return TeleportIndex;
		}
    }
	
    return TeleportIndex;
}

FVector_NetQuantize UBoostComponent::GetTeleportLocation()
{
	if (!Character) return FVector_NetQuantize{0.f, 0.f, 0.f};
	if (UWorld* World = GetWorld())
	{
		if (AShooterGameState* GS = World->GetGameState<AShooterGameState>())
		{
			TArray<FVector_NetQuantize> AllTeleportLocations = GS->AllTeleportLocations;

			if (AllTeleportLocations.Num() > 0)
			{
				int32 TeleportIndex = FMath::RandRange(0, AllTeleportLocations.Num() - 1); 
				/*
				int32 TeleportLocationIter = 0;
				// Check if chosen teleport location is too close to current character location
				while ((AllTeleportLocations[TeleportIndex] - Character->GetActorLocation()).Size() < 2000.f)
				{
					// If teleport location is too close, choose another random location
					TeleportIndex = FMath::RandRange(0, AllTeleportLocations.Num() - 1);
					TeleportLocationIter++;

					if (TeleportLocationIter >= AllTeleportLocations.Num())
					{
						return AllTeleportLocations[TeleportIndex];
					}
				}
				*/
				return AllTeleportLocations[TeleportIndex];
			}
		}
	}
	return Character->GetActorLocation();
}

void UBoostComponent::DetachItem()
{
	ABoostItem* BoostItem = Cast<ABoostItem>(Character->GetEquippedItem());
	if (Character == nullptr || BoostItem == nullptr) return;
	//if (!Character->HasAuthority()) return;
	if (Character->IsLocallyControlled())
	{
		//ServerStartGrapple(Character);
	}
}

void UBoostComponent::StartGrappleLocal(ABoostItem* BoostItem, FHitResult TraceHit)
{
	if (Character == nullptr || BoostItem == nullptr) return;
	
	
	FTimerDelegate BoostFinishedDel;
	FTimerHandle BoostFinishedTimer;
	BoostFinishedDel.BindUFunction(this, FName("RunGrappleLocal"), BoostItem, TraceHit);
	UWorld* World = GetWorld();
	if (World) World->GetTimerManager().SetTimer(BoostFinishedTimer, BoostFinishedDel, 0.225f, false);

}

void UBoostComponent::RunGrappleLocal(ABoostItem* BoostItem, FHitResult TraceHit)
{
	if (Character == nullptr || BoostItem == nullptr) return;
	if (BoostItem)
	{	
		BoostItem->GrappleState = EGrappleState::EGS_Start;
		BoostItem->SetItemState(EItemState::EIS_Falling);
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		BoostItem->DetachFromActor(DetachmentTransformRules);

		auto ShooterChar = Cast<AShooterCharacter>(TraceHit.GetActor());
		if (ShooterChar == nullptr)
		{
			BoostItem->SetActorRotation((TraceHit.ImpactPoint - TraceHit.TraceStart).Rotation(), ETeleportType::TeleportPhysics);
			BoostItem->GetItemMesh()->AddImpulse(5220.f * (TraceHit.ImpactPoint - TraceHit.TraceStart).GetSafeNormal());
		}

		if (Character->HasAuthority() || Character->IsLocallyControlled())
		{
			Character->StartDestoryGrappleIfNoHit(BoostItem);
		}
	}
}

void UBoostComponent::ServerStartGrapple_Implementation(ABoostItem* BoostItem, FHitResult TraceHit)
{
	if (Character == nullptr || BoostItem == nullptr) return;
	MulticastStartGrapple(BoostItem, TraceHit);
	if (BoostItem->GetBoostItemAmount() <= 0)
	{
		Character->Inventory.Remove(BoostItem);
		UpdateSlotIndex();
	}
	//Character->bGrappleThrown = true;
}

void UBoostComponent::MulticastStartGrapple_Implementation(ABoostItem* BoostItem, FHitResult TraceHit)
{
	if (Character == nullptr || BoostItem == nullptr) return;
	BoostItem->SpendBoost();
	Character->PlayItemMontage(BoostItem->GetItemMontageSection());
	StartGrappleLocal(BoostItem, TraceHit);
}

void UBoostComponent::TakeItem(ABoostItem* BoostItem, bool bNewItem)
{
	bNewBoostItem = bNewItem;
	BoostPickup = BoostItem;
	UpdateBoostAmount(BoostPickup);
}

void UBoostComponent::UpdateBoostAmount(ABoostItem* TakenBoost)
{
	if (Character == nullptr) return;
	ABoostItem* BoostInInventory;

	for (int32 i = 0; i < Character->Inventory.Num(); i++)
	{
		BoostInInventory = Cast<ABoostItem>(Character->Inventory[i]);
		if (BoostInInventory && TakenBoost && TakenBoost->GetBoostType() == BoostInInventory->GetBoostType())
		{
			if (!bNewBoostItem)
			{
				//UE_LOG(LogTemp, Warning, TEXT("TakenItemAmount = %i"), TakenBoost->GetBoostItemAmount());
				BoostInInventory->AddBoost(TakenBoost->GetBoostItemAmount(), Character);
				//UE_LOG(LogTemp, Warning, TEXT("Already have this Boost"));
			}
			else
			{
				BoostInInventory->AddBoost(TakenBoost->GetBoostItemAmount() - BoostInInventory->GetBoostItemAmount(), Character);
				//UE_LOG(LogTemp, Warning, TEXT("New Boost"));
			}
			return;
		}
	}
}

void UBoostComponent::UpdateSlotIndex()
{
    for (int32 i = 1; i < Character->Inventory.Num(); i++)
    {
        if (i != Character->Inventory[i]->GetSlotIndex())
        {
            Character->Inventory[i]->SetSlotIndex(i);
        }
    }
}

void UBoostComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME_CONDITION(ThisClass, CarriedItems, COND_OwnerOnly);
	DOREPLIFETIME(ThisClass, BoostPickup);
}
