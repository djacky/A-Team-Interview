// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAI.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Animation/AnimMontage.h"
#include "Shooter/ShooterAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Shooter/ShooterSpringArmComp.h"
#include "Camera/CameraComponent.h"
#include "ShooterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/BoostItem.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "BehaviorTree/BehaviorTree.h"
#include "ShooterGameState.h"
#include "BoostComponent.h"
#include "BoostItem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/BoxComponent.h"
#include "ShooterPlayerState.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "NavigationSystem.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "Shooter/Items/DummyItem.h"
#include "../Misc/CharacterGlobalFuncs.h"
#include "ShooterMovementComponent.h"

/* 
Notes for BP: 
In Character Movement Component, make sure that "Use Acceleration For Paths" is true 
*/
AShooterAI::AShooterAI(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	bUseControllerRotationYaw = true;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = BaseJumpVelocity;
	GetCharacterMovement()->AirControl = 0.4f;

	SceneCameraPositionForInterp = CreateDefaultSubobject<USceneComponent>(TEXT("CameraForInterp"));
	SceneCameraPositionForInterp->SetupAttachment(RootComponent);

	/*
	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetCapsuleComponent());
	AgroSphere->SetSphereRadius(2000.f);
	AgroSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AgroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AgroSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AgroSphere->SetGenerateOverlapEvents(false);

	CheckTargetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CheckTargetSphere"));
	CheckTargetSphere->SetupAttachment(GetCapsuleComponent());
	CheckTargetSphere->SetSphereRadius(8500.f);
	CheckTargetSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CheckTargetSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CheckTargetSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CheckTargetSphere->SetGenerateOverlapEvents(false);
	*/
}

void AShooterAI::BeginPlay()
{
	Super::BeginPlay();
	bIsAI = true;
	UShooterGameInstance* ShooterGameInst = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (ShooterGameInst) ShooterGameInst->bPracticeMode == false ? SetCanTarget(true) : SetCanTarget(false);
    ShieldOn(false);
	if (CameraBoom) CameraBoom->DestroyComponent();
	if (FollowCamera) FollowCamera->DestroyComponent();
	BaseMovementSpeed = 500.f;
	SetMovementState(EMovementModifiers::EMM_Normal);
	//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	/*
	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->DestroyComponent();
		}
	}
	*/

	if (HasAuthority())
	{
		Health = 150.f;
		MaxHealth = 150.f;
		MulticastStartAI();
		//AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AShooterAI::OnPlayerOverlap);
		//CheckTargetSphere->OnComponentEndOverlap.AddDynamic(this, &AShooterAI::OnCheckTargetEndOverlap);
	} 
}

void AShooterAI::ActivateCamera(bool bActivate)
{
	if (bActivate)
	{
		if (CameraBoom) CameraBoom->Activate();
		if (FollowCamera) FollowCamera->Activate();
	}
	else
	{
		if (CameraBoom) CameraBoom->Deactivate();
		if (FollowCamera) FollowCamera->Deactivate();
	}
}

void AShooterAI::MulticastStartAI_Implementation()
{
	if (AIStartSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AIStartSound, GetActorLocation());
	}
}

void AShooterAI::Tick(float DeltaTime)
{
	if (HasAuthority())
	{
		FRotator const BaseAimRot = GetBaseAimRotation();
		ReplBaseAimRotationPitch = BaseAimRot.Pitch;
		ReplBaseAimRotationYaw = BaseAimRot.Yaw;
	}

	SmoothRise(DeltaTime);

	PullToGravityProjectile();
	//UE_LOG(LogTemp, Warning, TEXT("CombatStateAI = %i"), CanFire());
	//UE_LOG(LogTemp, Warning, TEXT("AI Inventory Num = %i"), Inventory.Num());
}

void AShooterAI::SmoothRise(float DeltaTime)
{
	if (bFlyingInterping && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying)
	{
		//SetActorLocation(FMath::VInterpTo(GetActorLocation(), FlyingHeightTarget, DeltaTime, 0.75f));
		AddMovementInput((FlyingHeightTarget - GetActorLocation()).GetSafeNormal(), 1.0f);
		if (UKismetMathLibrary::NearlyEqual_FloatFloat(GetActorLocation().Z, FlyingHeightTarget.Z, 150.f))
		{
			GetCharacterMovement()->MaxFlySpeed = BaseFlySpeed;
			//UE_LOG(LogTemp, Warning, TEXT("Final Height Target = %s"), *FlyingHeightTarget.ToString());
			bFlyingInterping = false;
		}
	}
}

void AShooterAI::ScreenMessage(FString Mess)
{
	if (GEngine)
	{
		if (HasAuthority())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Mess);
		}
		if (!HasAuthority())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, Mess);
		}
	}
}

void AShooterAI::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

	ShooterAIController = Cast<AShooterAIController>(NewController);
	ShooterPS = GetShooter_PS();
	PreviousTargetDistance = TargetExitDistance;
	//int32 RandomSkinIndex = FMath::RandRange(0, AISkinsArray.Num() - 1);
	if (GetMesh())
	{
		if (ShooterPS)
		{
			AIName = ShooterPS->GetPlayerName();
			TargetSkeletalMeshOverride = ShooterPS->EnemyReplicationData.SkeletalMesh;
			GetMesh()->SetSkeletalMesh(TargetSkeletalMeshOverride);
		}
		//UE_LOG(LogTemp, Warning, TEXT("ServerSetTargetSkeletalMeshOverride: InTargetSkeletalMeshOverride"));
		//TargetSkeletalMeshOverride = AISkinsArray[RandomSkinIndex];
		GetAllSkeletalMeshMaterials();
	}
	//TargetMovementSpeed = BaseMovementSpeed * PatrolSpeedFactor;
	//GetCharacterMovement()->MaxWalkSpeed = TargetMovementSpeed;
	SetMovementState(EMovementModifiers::EMM_AIPatrol);

    if (ShooterAIController && BehaviorTree && ShooterAIController->GetBlackboardComponent())
    {
		ShooterAIController->GetBlackboardComponent()->InitializeBlackboard(*(BehaviorTree->BlackboardAsset));
        ShooterAIController->RunBehaviorTree(BehaviorTree);
    }
	//ClientSetBehaviorTree(NewController);
	StartSearchTimer();
	SetInitialItems();
}

void AShooterAI::ClientSetBehaviorTree_Implementation(AController* NewController)
{
    //FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
	ShooterAIController = Cast<AShooterAIController>(NewController);
    if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
    {
		ShooterAIController->GetBlackboardComponent()->InitializeBlackboard(*(BehaviorTree->BlackboardAsset));
        //ShooterAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), WorldPatrolPoint);
        ShooterAIController->RunBehaviorTree(BehaviorTree);
    }
}

void AShooterAI::InitializeComboVariables(const FString& MeshName)
{
    // Don't initialize commbo vars for AI (he will only be shooting)
}


void AShooterAI::InitialEquipWeapon(AItem* WeaponToEquip)
{
	// Will add the initial weapon in slot 0 here instead of in the parent ShooterCharacter class
}

// Function is overriding setting the cyber-grapple item on the ShooterCharacter class
void AShooterAI::SetInitialBoost()
{
	// Set the initial weapons/items in the SetInitialItems function
}

void AShooterAI::ReplenishShieldStrength()
{
	// Nothing to do here
}

void AShooterAI::ReplenishChargeAttackStrength()
{
	// Nothing to do here
}

void AShooterAI::ReplenishHoverSystem()
{
	// Nothing to do here
}

void AShooterAI::ReplenishDashCharge(float DeltaTime)
{
	// Nothing to do here
}

void AShooterAI::SetInitialItems()
{
	//UE_LOG(LogTemp, Warning, TEXT("SetInitialBoost"));
	UWorld* World = GetWorld();
	if (World && DefaultWeaponClass && DefaultItemClass)
	{
		ADummyItem* PlaceholderWeapon = World->SpawnActor<ADummyItem>(DefaultItemClass, GetActorTransform());
		if (PlaceholderWeapon == nullptr) return;
		Inventory.Add(PlaceholderWeapon);
    	Multicast_SetInitialItem(PlaceholderWeapon);

		AWeapon* FirstWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass, GetActorTransform());
		if (FirstWeapon == nullptr) return;
		Inventory.Add(FirstWeapon);

		TArray<EWeaponType> WeaponTypeArray = {EWeaponType::EWT_AR,EWeaponType::EWT_SubmachineGun, 
												EWeaponType::EWT_Pistol, EWeaponType::EWT_GrenadeLauncher};
		EWeaponType SelectedWeapon = WeaponTypeArray[FMath::RandRange(0, WeaponTypeArray.Num() - 1)];

		//EWeaponType SelectedWeapon = EWeaponType::EWT_Pistol;
		FirstWeapon->SetWeaponType(SelectedWeapon);
        FirstWeapon->UpdateWeapon();
		FirstWeapon->SetSlotIndex(1);
		//FirstWeapon->AddAmmo(20);
		FirstWeapon->SetItemState(EItemState::EIS_PickedUp);
		FirstWeapon->SetOwner(this);

		auto Item = Cast<AItem>(FirstWeapon);
		if (Item)
		{
			//Item->SetItemRarity(EItemRarity::EIR_Common);
			Item->SetItemRarity(EItemRarity::EIR_Damaged);
			Item->UpdateItem();
		}
		
		switch (SelectedWeapon)
		{
		case EWeaponType::EWT_AR:
			AmmoStruct.AmmoType.Add(EAmmoType::EAT_AR);
			break;
		case EWeaponType::EWT_Pistol:
			AmmoStruct.AmmoType.Add(EAmmoType::EAT_9mm);
			ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
			if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
			{
				ShooterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("IsPistol"), true);
			}
			break;
		case EWeaponType::EWT_SubmachineGun:
			AmmoStruct.AmmoType.Add(EAmmoType::EAT_9mm);
			FirstWeapon->SetWeaponDamageAmount(0.8f * FirstWeapon->GetWeaponDamageAmount());
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			AmmoStruct.AmmoType.Add(EAmmoType::EAT_GrenadeRounds);
			ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
			if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
			{
				ShooterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("IsGrenadeLauncher"), true);
			}
			break;
		default:
			break;
		}
		
		AmmoStruct.AmmoAmount.Add(100000);

		//FirstWeapon->SetWeaponDamageAmount(0.8f * FirstWeapon->GetWeaponDamageAmount());

		InventorySelect = 1;
		ExchangeInventoryItems(0, 1);
		
		ABoostItem* FirstBoost = World->SpawnActor<ABoostItem>(BoostItemClass, GetActorTransform());
		ABoostItem* SecondBoost = World->SpawnActor<ABoostItem>(BoostItemClass, GetActorTransform());
		if (FirstBoost == nullptr || SecondBoost == nullptr) return;
		Inventory.Add(FirstBoost);
		Inventory.Add(SecondBoost);
		TArray<EBoostType> BoostTypeArray;
		if (SelectedWeapon == EWeaponType::EWT_Pistol)
		{
			// Flying doesn't work well with pistol because we are using AddMovementInput for move the AI
			// towards the target, and pistol is not being shot in a tick way (there is a wait node for the pistol in BT)
			BoostTypeArray = {EBoostType::EBT_Slow, EBoostType::EBT_Protect, EBoostType::EBT_Ghost, EBoostType::EBT_HealthV2};
		}
		else
		{
			BoostTypeArray = {EBoostType::EBT_Slow, EBoostType::EBT_Protect, EBoostType::EBT_Fly, EBoostType::EBT_Ghost, EBoostType::EBT_HealthV2};
		}

		//BoostTypeArray = {EBoostType::EBT_Ghost};
		EBoostType FirstSelectedBoost = BoostTypeArray[FMath::RandRange(0, BoostTypeArray.Num() - 1)];
		EBoostType SecondSelectedBoost = BoostTypeArray[FMath::RandRange(0, BoostTypeArray.Num() - 1)];

		FirstBoost->SetBoostType(FirstSelectedBoost);
		FirstBoost->UpdateBoost();
		Multicast_SetAIBoost_Implementation(FirstBoost, 2, 10000);

		SecondBoost->SetBoostType(SecondSelectedBoost);
		SecondBoost->UpdateBoost();
		Multicast_SetAIBoost_Implementation(SecondBoost, 3, 10000);
		
		bSetInitialItems = true;
	}
}

void AShooterAI::Multicast_SetAIBoost_Implementation(ABoostItem* InitialBoost, int32 InSlotIndex, int32 BoostAmount)
{
	if (InitialBoost)
	{
		InitialBoost->SetOwner(nullptr);
		InitialBoost->SetOwner(this);
		InitialBoost->SetSlotIndex(InSlotIndex);
		if (HasAuthority()) InitialBoost->SetItemState(EItemState::EIS_PickedUp);
		InitialBoost->SetBoostItemAmount(BoostAmount);
	}
}

void AShooterAI::OnPlayerOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
	//if (!HasAuthority()) return;
    if (OtherActor && OtherActor != this && TargetCharacter == nullptr)
    {
        auto PlayerCharacter = Cast<AShooterCharacter>(OtherActor);
		//auto AICharacter = Cast<AShooterAI>(OtherActor);
		if (PlayerCharacter)
		{
			SetTargetCharacter(PlayerCharacter);
		}
    }
}

/*
void AShooterAI::OnPlayerEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
    if (OtherActor != this)
    {
		//SetNewTargetCharacter();
    }
}
*/

void AShooterAI::OnCheckTargetEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor != this)
    {
        auto PlayerCharacter = Cast<AShooterCharacter>(OtherActor);
		//auto AICharacter = Cast<AShooterAI>(OtherActor);
		if (PlayerCharacter)
		{
			if (PlayerCharacter == TargetCharacter)
			{
				SetNewTargetCharacter(TargetCharacter);
			}
		}
    }
}

void AShooterAI::StartSearchTimer()
{
	TArray<AActor*> FoundShooters;
	if (TargetCharacter)
	{
		//FoundShooters = GetShooterPlayersInSphere(4000.f);
		//if (!FoundShooters.Contains(TargetCharacter))
		//{
		//	SetNewTargetCharacter(TargetCharacter);
		//}
		float TargetDistance = GetDistanceTo(TargetCharacter);
		if (PreviousTargetDistance < TargetExitDistance && TargetDistance >= TargetExitDistance)
		{
			SetNewTargetCharacter(TargetCharacter);
		}
		PreviousTargetDistance = TargetDistance;
	}
	else
	{
		//DrawDebugSphere(GetWorld(), GetActorLocation(), FCollisionShape::MakeSphere(1500.f).GetSphereRadius(), 50, FColor::Purple, true);
		FoundShooters = GetShooterPlayersInSphere(2800.f);
		if (FoundShooters.Num() > 0)
		{
			SetTargetCharacter(FoundShooters[0]);
		}
	}
	GetWorldTimerManager().SetTimer(SearchTimer, this, &AShooterAI::FinishSearchTimer, 0.5f);
}

void AShooterAI::FinishSearchTimer()
{
	StartSearchTimer();
}

void AShooterAI::StopSearchTimer()
{
	SetCanTarget(false);
	GetWorldTimerManager().ClearTimer(SearchTimer);
	OnTakeAnyDamage.RemoveDynamic(this, &AShooterAI::ReceiveDamage);
	SetTargetCharacter(nullptr);

	ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
	if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
	{
		ShooterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("GameEnded"), true);
	}
}

void AShooterAI::SetNewTargetCharacter(AActor* CurrentTarget)
{
    //TArray<AActor*> OverlappingCharacters;
    //AgroSphere->GetOverlappingActors(OverlappingCharacters, AShooterCharacter::StaticClass());

	TArray<AActor*> FoundShooters;
	FoundShooters = GetShooterPlayersInSphere(2800.f);

	if (FoundShooters.Num() > 0)
	{
		TArray<float> DistanceArray;
		TArray<AActor*> ShooterArray;
		int32 MinIndex;
		float MinDistance;
		
		for (auto ShooterPlayerChar : FoundShooters)
		{
			APawn* Pawn = Cast<APawn>(ShooterPlayerChar);
			bool bIsAPlayer = Pawn && Pawn->GetController() && Pawn->GetController()->IsPlayerController();
			//auto ShooterPlayerChar = Cast<AShooterCharacter>(ShooterActor);
			if (ShooterPlayerChar && bIsAPlayer && ShooterPlayerChar != CurrentTarget)
			{
				DistanceArray.Add(GetDistanceTo(ShooterPlayerChar));
				ShooterArray.Add(ShooterPlayerChar);
			}
		}
		if (ShooterArray.Num() > 0)
		{
			UKismetMathLibrary::MinOfFloatArray(DistanceArray, MinIndex, MinDistance);
			if (ShooterArray[MinIndex]) SetTargetCharacter(ShooterArray[MinIndex]);
			return;
		}
		else
		{
			SetTargetCharacter(nullptr);
			return;
		}
	}
	SetTargetCharacter(nullptr);
}

TArray<AActor*> AShooterAI::GetShooterPlayersInSphere(float SphereRadius)
{
    FVector MyLocation = GetActorLocation();
    TArray<FHitResult> HitResults;
	TArray<AActor*> FoundCharacters;

    // Perform a sphere trace to find nearby actors
    UWorld* World = GetWorld();
    if (World)
    {
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this); // Ignore the character itself

        // Perform a sphere trace
        if (World->SweepMultiByChannel(
            HitResults,                // Array to store hit results
            MyLocation,                 // Start location
            MyLocation,                 // End location (same as start for a sphere trace)
            FQuat::Identity,            // Rotation (identity rotation for a sphere trace)
            ECC_Pawn,                   // Collision channel (adjust as needed)
            FCollisionShape::MakeSphere(SphereRadius), // Sphere shape
            QueryParams                  // Query parameters
        ))
        {
			for (auto HitResult : HitResults)
			{
				auto FoundShooter = Cast<ACharacter>(HitResult.GetActor());
				if (FoundShooter && CanTarget(HitResult.GetActor()))
				{
					FoundCharacters.Add(HitResult.GetActor());
				}
			}
			return FoundCharacters;
        }
    }
    // No actor found in the radius
    return FoundCharacters;
}

bool AShooterAI::CanTarget(AActor* InCharacter)
{
	if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InCharacter))
	{
		return EnemyInterface->Execute_CanTarget(InCharacter);
	}
	return false;
}

bool AShooterAI::CanTarget_Implementation()
{
	return GetHealth() > 0.f && !GetGhostMode();
}

void AShooterAI::SetTargetCharacter(AActor* InCharacter)
{
	if (InCharacter && !CanTarget(InCharacter)) return;
	if (!bSetInitialItems) return;
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS && (ShooterGS->GameMatchState != EGameMatchState::EMS_Start && ShooterGS->GameMatchState != EGameMatchState::EMS_Stop)) return;
	if (InCharacter && (TargetCharacter == InCharacter || InCharacter == this)) return;

	TargetCharacter = InCharacter;
	PreviousTargetDistance = TargetExitDistance;

	if (InCharacter == nullptr && GetCharacterMovement())
	{
		if (GetWorldTimerManager().IsTimerActive(ItemTimer)) GetWorldTimerManager().ClearTimer(ItemTimer);
		if (GetWorldTimerManager().IsTimerActive(ChaseLocationTimer)) GetWorldTimerManager().ClearTimer(ChaseLocationTimer);
		//TargetMovementSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		SetMovementState(EMovementModifiers::EMM_AIPatrol);
	}
	else
	{
		StartItemTimer();
		if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InCharacter))
		{
			EnemyInterface->Execute_OnValidTarget(InCharacter, this);
		}
		GetRandomChaseLocationTimer();
		//TargetMovementSpeed = BaseMovementSpeed;
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		SetMovementState(EMovementModifiers::EMM_Normal);
	}

	ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
	if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
	{
		ShooterAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetCharacter"), TargetCharacter);
	}
}

void AShooterAI::StartGetRandomChaseLocationTimer()
{
	GetWorldTimerManager().SetTimer(ChaseLocationTimer, this, &AShooterAI::GetRandomChaseLocationTimer, FMath::FRandRange(1.f, 2.5f));
	UWorld* World = GetWorld();
	if (World && GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		NavSystem = NavSystem == nullptr ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : NavSystem;
		if (NavSystem == nullptr || TargetCharacter == nullptr)
		{
			return;
		}
		FVector Origin = TargetCharacter->GetActorLocation();
		FNavLocation RandomPoint(Origin);
		bool bRandomNav = NavSystem->GetRandomReachablePointInRadius(
							Origin,
							1200.f,
							RandomPoint,
							NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
		RandomTargetLocation = RandomPoint.Location;

		if (!IsPointReachable(RandomTargetLocation, NavSystem) && !GetStartFlying() && GetHoverState() != EHoverState::EHS_HoverStart)
		{
        	FVector LaunchVel{0.5f, 0.5f, 1.f};
        	LaunchCharacter(650.f*LaunchVel, false, false);
		}
	}
}

void AShooterAI::GetRandomChaseLocationTimer()
{
	StartGetRandomChaseLocationTimer();
}

bool AShooterAI::IsPointReachable(FVector TargetLocation, UNavigationSystemV1* InNavSystem)
{
    if (GetController() == nullptr || InNavSystem == nullptr)
    {
        return false;
    }

    // Create a pathfinding query
    FPathFindingQuery Query(
		GetController(), 
		*InNavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate),
		GetActorLocation(),
		TargetLocation);

    // Perform the pathfinding query synchronously
    if (InNavSystem && InNavSystem->TestPathSync(Query, EPathFindingMode::Regular))
    {
        // If a path exists, the point is reachable
        return true;
    }

    // If no path exists, the point is not reachable
    return false;
}

bool AShooterAI::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// Trace from Crosshair world location outward
	FCollisionQueryParams CollisionParam;
	CollisionParam.AddIgnoredActor(this);
	if (GetEquippedItem() == nullptr) return false;
	CollisionParam.AddIgnoredActor(GetEquippedItem());

	if (TargetCharacter && GetEquippedItem()->GetItemMesh())
	{
		const USkeletalMeshSocket* BarrelSocket = GetEquippedItem()->GetItemMesh()->GetSocketByName("BarrelSocket");
		if (BarrelSocket)
		{
			float Rd;
			if (GetEquippedWeapon())
			{
				switch (GetEquippedWeapon()->GetWeaponType())
				{
					case EWeaponType::EWT_Pistol:
						// pistol has better accuracy (less random deviation)
						Rd = 0.65f * ((r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1);
						break;
					case EWeaponType::EWT_SubmachineGun:
						Rd = (r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1;
						break;
					case EWeaponType::EWT_AR:
						Rd = 0.85f * ((r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1);
						break;
					default:
						Rd = (r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1;
						break;
				}
			}
			else
			{
				Rd = (r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1;
			}

			//float Rd = (r2 - r1) / d2 * GetDistanceTo(TargetCharacter) + r1;
			const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetEquippedItem()->GetItemMesh());
			const FVector Start{ SocketTransform.GetLocation() };
			const FVector End{ Start + (((TargetCharacter->GetActorLocation() + FVector(0.f, 0.f, 40.f)) - Start) + FVector(FMath::FRandRange(-Rd, Rd), FMath::FRandRange(-Rd, Rd), FMath::FRandRange(-Rd, Rd))) * 100.f };
			OutHitLocation = End;
			GetWorld()->LineTraceSingleByChannel(
				OutHitResult,
				Start,
				End,
				ECollisionChannel::ECC_Visibility,
				CollisionParam);

			if (OutHitResult.bBlockingHit)
			{
				OutHitLocation = OutHitResult.ImpactPoint;
				return true;
			}
			else
			{
				OutHitResult.ImpactPoint = End;
			}
		}

	}
	return false;
}

void AShooterAI::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
    if (!HasAuthority()) return;
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);

	AShooterPlayerController* VictimController = nullptr; AShooterPlayerController* AttackerController = nullptr;
	AShooterPlayerState* VictimPS = nullptr; AShooterPlayerState* AttackerPS = nullptr;
	UCharacterGlobalFuncs::GetShooterReferences(VictimController, AttackerController, VictimPS, AttackerPS, DamagedActor, DamageCauser, InstigatorController);
	UCharacterGlobalFuncs::UpdateDamageStat(DamagedActor, DamageCauser, VictimPS, AttackerPS, Damage, Health, DamageType);
	if (AttackerController)
	{
		SetTargetCharacter(AttackerController->MainCharacter);
	}
	else if (InstigatorController)
	{
		SetTargetCharacter(InstigatorController->GetPawn());
	}

    if (Health <= 0)
    {
		OnTakeAnyDamage.RemoveDynamic(this, &AShooterAI::ReceiveDamage);
		AShooterGameState* ShooterGS = GetShooter_GS();

		if (AttackerPS && ShooterGS) 
		{
			if (AttackerPS->IsABot() && DamagedActor)
			{
				AActor* InstigatorActor = InstigatorController ? InstigatorController->GetPawn() : DamageCauser;
				if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InstigatorActor))
				{
					EnemyInterface->Execute_SetTarget(InstigatorActor, DamagedActor, true);
				}
			}

			if (AttackerController)
			{
				AttackerController->ClientUpdateHUDNumOfKills(AttackerPS->PlayerGameStats, AttackerController->MainCharacter);
				if (ShooterGS->IsPracticeMode()) AttackerController->UpdateAICount(ShooterGS->AIShooterArray.Num());
			}
			
			ShooterGS->SpawnItemsAfterAIKill(this->GetCapsuleComponent()->GetComponentTransform());
			for (auto ShooterController : ShooterGS->ShooterControllerArray)
			{
				//AShooterPlayerController* ShooterController = Cast<AShooterPlayerController>(PS->GetPlayerController());
				if (ShooterController)
				{
					ShooterController->ClientAIElimAnnouncement(AttackerPS, AttackerPS->DamageType, AIName);
				}
			}
		}
        MulticastElimAI();
		//if (GetEquippedItem()) GetEquippedItem()->Destroy();
    }
}

void AShooterAI::ClientSetTarget_Implementation(AShooterCharacter* ShooterCauser)
{
	SetTargetCharacter(ShooterCauser);
}

void AShooterAI::OnElimmed_Implementation()
{
	MulticastElimAI();
}

void AShooterAI::MulticastElimAI_Implementation()
{
    //PlayElimMontageAI();
	bPlayerEliminated = true;
	bDisableGameplay = true;
	TriggerAnim();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		if (DynamicDissolveMaterialInstance)
		{
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
			SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
		}
	}
	StartDissolve();
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetEquippedItem() && GetEquippedItem()->GetItemMesh())
	{
		GetEquippedItem()->GetItemMesh()->SetVisibility(false);
		//FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
    	//GetEquippedItem()->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		//GetEquippedItem()->SetItemState(EItemState::EIS_Falling);
	}
	StopAllHoverEffects();

	if (ElimSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimSound,
			GetActorLocation()
		);
	}
	if (HasAuthority())
	{
		FTimerHandle DestroyTimer;
		GetWorldTimerManager().SetTimer(DestroyTimer, this, &AShooterAI::HandleAIDestroy, 3.f);
		
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS && ShooterGS->AIShooterArray.Contains(this))
		{
			ShooterGS->AIShooterArray.Remove(this);
			//ShooterGS->CheckWinnerAfterAIKillTimer();
		}
		if (ShooterAIController && ShooterAIController->BrainComponent)
		{
			if (ShooterAIController->GetBlackboardComponent())
			{
				ShooterAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetCharacter"), nullptr);
			}
			ShooterAIController->BrainComponent->StopLogic(TEXT("Killed"));
			ShooterAIController->UnPossess();
		}
	}
}

void AShooterAI::HandleAIDestroy()
{
	if (GetEquippedItem()) GetEquippedItem()->Destroy();
	if (AShooterGameState* ShooterGS = GetShooter_GS())
	{
		if (ShooterAIController)
		{
			if (ShooterGS->IsPracticeMode())
			{
				ShooterAIController->Destroy();
			}
			else
			{
				if (AShooterPlayerState* VictimPS = Cast<AShooterPlayerState>(ShooterAIController->PlayerState))
				{
					ShooterGS->SpawnNewAI(ShooterAIController, VictimPS->EnemyReplicationData.EnemyType);
				}
			}
		}
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Destroy();
}

void AShooterAI::PlayTauntSound_Implementation()
{
	PlayPatrolSound(TEXT("KilledPlayer"));
}

/*
void AShooterAI::UseRandomBoost()
{
	ExchangeInventoryItems(1, 2);
	FireButtonPressed();
}
*/

void AShooterAI::StartItemTimer()
{
	//GetWorldTimerManager().SetTimer(ItemTimer, this, &AShooterAI::FinishItemTimer, FMath::FRandRange(6.f, 9.f));
	GetWorldTimerManager().SetTimer(ItemTimer, this, &AShooterAI::FinishItemTimer, FMath::FRandRange(9.f, 16.f));
}

void AShooterAI::FinishItemTimer()
{
	bUsingItemAI = true;
}

void AShooterAI::FireButtonPressed()
{
	//UE_LOG(LogTemp, Warning, TEXT("bCanFireAgain = %i"), bCanFireAgain);
	if (!bCanFireAgain) return;
	Super::FireButtonPressed();

	if (bUsingItemAI && !bLocallyReloading && !GetGhostMode() && !GetStartFlying() && !GetBoostProtect() && 
		!GetSlowDown() && GetCharacterMovement() && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking &&
		GetCombatState() == ECombatState::ECS_Unoccupied)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		//ScreenMessage(TEXT("UsingItem"));
		bUsingItemAI = false;
		bCanFireAgain = false;
		bCanFire = false;
		GetWorldTimerManager().ClearTimer(ItemTimer);
		SwitchToBoost();
	}
}

void AShooterAI::SwitchToBoost()
{
	GetWorldTimerManager().ClearTimer(HoverTimer);
	bCanFireAgain = false;
	RandomBoostIndexSelected = FMath::RandRange(2, Inventory.Num() - 1);
	ExchangeInventoryItems(1, RandomBoostIndexSelected);
	FTimerHandle UseItemTimer;
	GetWorldTimerManager().SetTimer(UseItemTimer, this, &AShooterAI::UseItem, 0.5f);
}

void AShooterAI::BoostItemMontageFinished()
{
	//UE_LOG(LogTemp, Warning, TEXT("InventoryNum = %i, RandomBoostIndexSelected = %i"), Inventory.Num(), RandomBoostIndexSelected);
	Super::BoostItemMontageFinished();
	if (Inventory.Num() > RandomBoostIndexSelected && Inventory[RandomBoostIndexSelected])
	{
		auto BoostItem = Cast<ABoostItem>(Inventory[RandomBoostIndexSelected]);
		if (BoostItem)
		{
			if (BoostItem->GetBoostType() != EBoostType::EBT_Fly) GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		}
		ExchangeInventoryItems(RandomBoostIndexSelected, 1);
	}
	
	if (TargetCharacter == nullptr && GetCharacterMovement()->MaxWalkSpeed == BaseMovementSpeed)
	{
		//TargetMovementSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		SetMovementState(EMovementModifiers::EMM_AIPatrol);
	}
}

void AShooterAI::OnRep_EquippedItem(AItem* OldItem)
{
	Super::OnRep_EquippedItem(OldItem);

	if (HasAuthority() && GetEquippedItem() && !bUsingItemAI && !bCanFireAgain && Cast<ABoostItem>(GetEquippedItem()))
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_EquippedItem AI: Starting Boost"));
		//FTimerHandle UseItemTimer;
		//GetWorldTimerManager().SetTimer(UseItemTimer, this, &AShooterAI::UseItem, 0.5f);
	}
	else if (HasAuthority() && GetEquippedItem() && !bCanFireAgain && Cast<AWeapon>(GetEquippedItem()))
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_EquippedItem AI: Resume weapon fire"));
		ReloadWeapon();
		bCanFireAgain = true;
		StartItemTimer();
		GetWorldTimerManager().SetTimer(HoverTimer, this, &AShooterAI::StartRandomHover, FMath::RandRange(1.f, 4.f));
	}

}

void AShooterAI::UseItem()
{
	if (Boost)
	{
		Boost->StartBoost();
		GetWorldTimerManager().SetTimer(CheckUsingItemTimer, this, &AShooterAI::CheckUsingItem, 0.5f);
	}
}

void AShooterAI::CheckUsingItem()
{
	if (!bUsingItem)
	{
		UseItem();
	}
}

void AShooterAI::StartFlying()
{
    bStartFlying = true;
	FlyingHeightTarget = GetActorLocation() + FVector{0.f, 0.f, 350.f};
	OnRep_StartFlying();

    GetWorldTimerManager().SetTimer(FlyTimer, this, &AShooterCharacter::StopFlying, 12.f);
}

FVector AShooterAI::GetPatrolPoint()
{
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS && ShooterGS->AIPatrolPoints.Num() > 0)
	{
		int32 RandomPatrolIndex = FMath::RandRange(0, ShooterGS->AIPatrolPoints.Num() - 1);
		ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
		if (ShooterAIController)
		{
			if (GetWorld() && GetCharacterMovement() && !GetCharacterMovement()->IsFalling() && !GetStartFlying())
			{
				NavSystem = NavSystem == nullptr ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()) : NavSystem;
				if (NavSystem)
				{
					if (!IsPointReachable(ShooterGS->AIPatrolPoints[RandomPatrolIndex], NavSystem))
					{
						FVector LaunchVel{0.5f, 0.5f, 1.f};
						LaunchCharacter(800.f*LaunchVel, false, false);
					}
				}
			}
			if (ShooterAIController->GetBlackboardComponent())
			{
				ShooterAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), ShooterGS->AIPatrolPoints[RandomPatrolIndex]);
				return ShooterGS->AIPatrolPoints[RandomPatrolIndex];
			}
		} 
	}
	else
	{
		int32 RandomPatrolIndex = FMath::RandRange(0, BackupPatrolPoints.Num() - 1);
		ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
		if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
		{
			ShooterAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), BackupPatrolPoints[RandomPatrolIndex]);
			return BackupPatrolPoints[RandomPatrolIndex];
		}
	}
	return BackupPatrolPoints[0];
}

// Since the camera is destroyed, should stop functions that involved camera
void AShooterAI::CameraOffsetWithDistance()
{
	//
}

void AShooterAI::MulticastPlayPatrolWaitSound_Implementation(const FString& AIStage)
{
	PlayPatrolSound(AIStage);
}

void AShooterAI::PlayPatrolSound(const FString& AIStage)
{
	if (AIStage == TEXT("Patroling"))
	{
		if (PatrolWaitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PatrolWaitSound, GetActorLocation());
		}
	}
	else if (AIStage == TEXT("NewEnemy"))
	{
		if (NewEnemySound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, NewEnemySound, GetActorLocation());
		}
	}
	else if (AIStage == TEXT("KilledPlayer"))
	{
		if (KilledPlayerSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, KilledPlayerSound, GetActorLocation());
		}
	}
}

void AShooterAI::WinnerOnRespawnMode()
{
	bPlayerWon = true;
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	bCanFire = false;
	bDisableGameplay = true;
	SetTargetCharacter(nullptr);

	/*
	if (GetEquippedItem() && Cast<ABoostItem>(GetEquippedItem()) && RandomBoostIndexSelected >= 2)
	{
		ExchangeInventoryItems(RandomBoostIndexSelected, 1);
	}
	*/
	if (GetWorldTimerManager().IsTimerActive(SearchTimer)) GetWorldTimerManager().ClearTimer(SearchTimer);

	//TArray<FName> WinnerMontageSections = {FName("Winner1"), FName("Winner2"), FName("Winner3")};
	//Multicast_PlayWinnerMontage(WinnerMontageSections[FMath::RandRange(0, WinnerMontageSections.Num() - 1)]);
}

void AShooterAI::Multicast_PlayWinnerMontage_Implementation(const FName& RandomMontageName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && GetWinMontage())
	{
		AnimInstance->Montage_Play(GetWinMontage());
		AnimInstance->Montage_JumpToSection(RandomMontageName);
	}
}

void AShooterAI::StartRandomHover()
{
	HoverButtonReleased();
    GetWorldTimerManager().SetTimer(HoverTimer, this, &AShooterAI::StartHovering, FMath::RandRange(1.f, 4.f));
}

void AShooterAI::StartHovering()
{
	ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
	if (ShooterAIController) ShooterAIController->StopMovement();
	HoverButtonPressed();
    GetWorldTimerManager().SetTimer(HoverTimer, this, &AShooterAI::StartRandomHover, FMath::RandRange(1.f, 4.f));
}

void AShooterAI::StopRandomHover()
{
	if (GetWorldTimerManager().IsTimerActive(HoverTimer)) GetWorldTimerManager().ClearTimer(HoverTimer);
	HoverButtonReleased();
}

void AShooterAI::SetTarget_Implementation(AActor* InCharacter, bool bIsNew)
{
	bIsNew ? SetNewTargetCharacter(InCharacter) : SetTargetCharacter(InCharacter);
}

void AShooterAI::StopSearch_Implementation()
{
	StopSearchTimer();
}

USceneComponent* AShooterAI::OnGameEndWinner_Implementation()
{
	WinnerOnRespawnMode();
	return SceneCameraPositionForInterp;
}

void AShooterAI::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ThisClass, HealthAI);
	DOREPLIFETIME(ThisClass, TargetCharacter);
	DOREPLIFETIME(ThisClass, ReplBaseAimRotationYaw);
	DOREPLIFETIME(ThisClass, ReplBaseAimRotationPitch);
	//DOREPLIFETIME(ThisClass, bCanFireAgain);
}