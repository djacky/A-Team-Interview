#include "AICharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "../ShooterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "../../ShooterGameState.h"
#include "../../ShooterCharacter.h"
#include "../../ShooterPlayerState.h"
#include "../../PlayerController/ShooterPlayerController.h"
#include "../../Items/Weapons/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "../../Misc/CharacterGlobalFuncs.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "../../Items/Weapons/ProjectileBullet.h"
#include "Sound/SoundCue.h"
#include "TeleportTrail.h"

#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2
#define ECC_Mantle ECollisionChannel::ECC_GameTraceChannel7
#define ECC_Missile ECollisionChannel::ECC_GameTraceChannel5
#define ECC_Helicopter ECollisionChannel::ECC_GameTraceChannel6
#define ECC_Item ECollisionChannel::ECC_GameTraceChannel3

AAICharacterBase::AAICharacterBase()
{
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	PrimaryActorTick.bCanEverTick = true;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	bUseControllerRotationYaw = true;

	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = BaseJumpVelocity;
	GetCharacterMovement()->AirControl = 0.4f;

	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FCollisionResponseContainer CapsuleCollisions;
	CapsuleCollisions.SetResponse(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CapsuleCollisions.SetResponse(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	CapsuleCollisions.SetResponse(ECC_Mantle, ECollisionResponse::ECR_Ignore);
	CapsuleCollisions.SetResponse(ECC_HitBox, ECollisionResponse::ECR_Overlap);
	CapsuleCollisions.SetResponse(ECC_Projectile, ECollisionResponse::ECR_Ignore);
	CapsuleCollisions.SetResponse(ECC_Missile, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannels(CapsuleCollisions);

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_Pawn);
	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	FCollisionResponseContainer MeshCollisions;
	MeshCollisions.SetResponse(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECC_Mantle, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECollisionChannel::ECC_Vehicle, ECollisionResponse::ECR_Ignore);
	MeshCollisions.SetResponse(ECC_Missile, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannels(MeshCollisions);

	GetMesh()->SetHiddenInGame(true);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	SceneCameraPositionForInterp = CreateDefaultSubobject<USceneComponent>(TEXT("CameraForInterp"));
	SceneCameraPositionForInterp->SetupAttachment(RootComponent);

	//GetMesh()->SetIsReplicated(true);

}

void AAICharacterBase::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AAICharacterBase::ReceiveDamage);
	}
}

void AAICharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PullToGravityProjectile();
}

void AAICharacterBase::Multicast_StartSpawnIn_Implementation()
{
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		if (DynamicDissolveMaterialInstance)
		{
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
			DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
			SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
		}
	}
	if (HasAuthority())
	{
		bSpawnedIn = true;
		OnRep_HasSpawnedIn();
	}
	StartSpawnIn();
	if (ElimSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimSound,
			GetActorLocation()
		);
	}
	if (SpawnInEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpawnInEffect, GetActorLocation());
	}
}

void AAICharacterBase::StartSpawnIn()
{
	DissolveTrack.BindDynamic(this, &AAICharacterBase::UpdateStartSpawnIn);
	if (SpawnInCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(SpawnInCurve, DissolveTrack);
		DissolveTimeline->PlayFromStart();
		if (HasAuthority())
		{
			FOnTimelineEvent FinishedCallback;
			FinishedCallback.BindUFunction(this, FName("OnSpawnInFinished"));
			DissolveTimeline->SetTimelineFinishedFunc(FinishedCallback);
		}
	}
}

void AAICharacterBase::UpdateStartSpawnIn(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
		//SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
	}
}

void AAICharacterBase::OnRep_HasFinishedDissolve()
{
	SetAllSkeletalMeshMaterials(true, nullptr);
}

void AAICharacterBase::OnRep_HasSpawnedIn()
{
	USkeletalMeshComponent* AIMesh = GetMesh();
	if (AIMesh)
	{
		AIMesh->SetHiddenInGame(false);
		AIMesh->SetVisibility(true);
	}
}

void AAICharacterBase::OnSpawnInFinished()
{
	bHasFinishedDissolve = true;
	OnRep_HasFinishedDissolve();
}

void AAICharacterBase::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
	
	if (auto AiPS = Cast<AShooterPlayerState>(GetPlayerState()))
	{
		EnemyReplicationData = AiPS->EnemyReplicationData;
		AIName = AiPS->GetPlayerName();
	}

	OnRep_EnemyData();
	
	ShooterAIController = Cast<AShooterAIController>(NewController);

	PreviousTargetDistance = TargetExitDistance;
	TargetMovementSpeed = BaseMovementSpeed * PatrolSpeedFactor;
	GetCharacterMovement()->MaxWalkSpeed = TargetMovementSpeed;

    if (ShooterAIController && BehaviorTree && ShooterAIController->GetBlackboardComponent())
    {
		ShooterAIController->GetBlackboardComponent()->InitializeBlackboard(*(BehaviorTree->BlackboardAsset));
        ShooterAIController->RunBehaviorTree(BehaviorTree);
    }
	StartSearchTimer();
	Multicast_StartSpawnIn();
}

void AAICharacterBase::UpdateEnemy()
{
	FEnemyData* EnemyRow = UCharacterGlobalFuncs::GetEnemyDataRow(EnemyReplicationData.EnemyType);
	USkeletalMeshComponent* AIMesh = GetMesh();
	if (EnemyRow && AIMesh)
	{
		BehaviorTree = EnemyRow->BehaviorTree;
		AIMesh->SetSkeletalMesh(EnemyReplicationData.SkeletalMesh);
		GetAllSkeletalMeshMaterials();
		AIMesh->SetAnimInstanceClass(EnemyRow->AnimationBlueprint);
		Health = EnemyRow->Health;
		MaxHealth = EnemyRow->MaxHealth;
		BaseMovementSpeed = EnemyRow->BaseMovementSpeed;
		EnemyData = *EnemyRow;
	}
	SetCorrectTransform();
}

void AAICharacterBase::OnRep_EnemyData()
{
	UpdateEnemy();
}

void AAICharacterBase::SetCorrectTransform()
{
	if (USkeletalMeshComponent* AIMesh = GetMesh())
	{
		if (!AIMesh->GetSkeletalMeshAsset())
		{
			FTimerHandle RetryHandle;
			GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &AAICharacterBase::SetCorrectTransform, 0.05f, false);
			return;
		}

		FVector NewLocation;
		FRotator NewRotation;
		switch (EnemyReplicationData.EnemyType)
		{
			case EEnemyType::EET_TwinBlast:
			{
				NewLocation = FVector(0.f, 0.f, -95.f);
				NewRotation = FRotator(0.f, -90.f, 0.f);
				break;
			}
			case EEnemyType::EET_Howitzer:
			{
				NewLocation = FVector(0.f, 0.000856f, -97.f);
				NewRotation = FRotator(0.f, 270.f, 0.f);
				break;
			}

		}
		AIMesh->SetRelativeRotation(NewRotation);
		AIMesh->SetRelativeLocation(NewLocation);
		CacheInitialMeshOffset(NewLocation, NewRotation);
	}
}

bool AAICharacterBase::CanTarget(AActor* InCharacter)
{
	if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InCharacter))
	{
		return EnemyInterface->Execute_CanTarget(InCharacter);
	}
	return false;
}

bool AAICharacterBase::CanTarget_Implementation()
{
	return Health > 0.f;
}

void AAICharacterBase::StopCurrentMontage()
{
	bCanAttack = false;
	if (CurrentAttackMontage) MulticastStopCurrentMontage(CurrentAttackMontage);
}

void AAICharacterBase::SetTargetCharacter(AActor* InCharacter)
{
	if (InCharacter && !CanTarget(InCharacter)) return;
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS && (ShooterGS->GameMatchState != EGameMatchState::EMS_Start && ShooterGS->GameMatchState != EGameMatchState::EMS_Stop)) return;
	if (InCharacter && (TargetCharacter == InCharacter || InCharacter == this)) return;

	TargetCharacter = InCharacter;
	PreviousTargetDistance = TargetExitDistance;

	if (InCharacter == nullptr)
	{
		if (GetWorldTimerManager().IsTimerActive(ChaseLocationTimer)) GetWorldTimerManager().ClearTimer(ChaseLocationTimer);
		TargetMovementSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed * PatrolSpeedFactor;
		SetMovement(false);
	}
	else
	{
		if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InCharacter))
		{
			EnemyInterface->Execute_OnValidTarget(InCharacter, this);
		}
		GetRandomChaseLocationTimer();
		TargetMovementSpeed = BaseMovementSpeed;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}

	ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
	if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
	{
		ShooterAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetCharacter"), TargetCharacter);
	}
}

void AAICharacterBase::MulticastStopCurrentMontage_Implementation(UAnimMontage* CurrentMontage)
{
	if (CurrentMontage)
	{
		if (USkeletalMeshComponent* AIMesh = GetMesh())
		{
			if (UAnimInstance* AnimInstance = AIMesh->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.2f, CurrentMontage);
				if (HasAuthority()) CurrentAttackMontage = nullptr;
			}
		}
	}
}

void AAICharacterBase::SetNewTargetCharacter(AActor* CurrentTarget)
{
    //TArray<AActor*> OverlappingCharacters;
    //AgroSphere->GetOverlappingActors(OverlappingCharacters, AShooterCharacter::StaticClass());
	if (USkeletalMeshComponent* AIMesh = GetMesh())
	{
		UAnimInstance* AnimInstance = AIMesh->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->StopAllMontages(0.f);
		}
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
				//APawn* Pawn = Cast<APawn>(ShooterPlayerChar);
				//bool bIsAPlayer = Pawn && Pawn->GetController() && Pawn->GetController()->IsPlayerController();
				if (ShooterPlayerChar && ShooterPlayerChar != CurrentTarget)
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
}

void AAICharacterBase::StopSearchTimer()
{
	GetWorldTimerManager().ClearTimer(SearchTimer);
	OnTakeAnyDamage.RemoveDynamic(this, &AAICharacterBase::ReceiveDamage);
	SetTargetCharacter(nullptr);

	ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
	if (ShooterAIController && ShooterAIController->GetBlackboardComponent())
	{
		ShooterAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("GameEnded"), true);
	}
}

void AAICharacterBase::SetTarget_Implementation(AActor* InCharacter, bool bIsNew)
{
	bIsNew ? SetNewTargetCharacter(InCharacter) : SetTargetCharacter(InCharacter);
}

void AAICharacterBase::StopSearch_Implementation()
{
	StopSearchTimer();
}

AShooterGameState* AAICharacterBase::GetShooter_GS()
{
	UWorld* World = GetWorld();
	if (World)
	{
		AShooterGameState* ShooterGS = World->GetGameState<AShooterGameState>();
		if (ShooterGS) return ShooterGS;
	}
	return nullptr;
}

bool AAICharacterBase::IsPointReachable(FVector TargetLocation, UNavigationSystemV1* InNavSystem)
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

void AAICharacterBase::GetRandomChaseLocationTimer()
{
	StartGetRandomChaseLocationTimer();
}

void AAICharacterBase::StartGetRandomChaseLocationTimer()
{
	GetWorldTimerManager().SetTimer(ChaseLocationTimer, this, &AAICharacterBase::GetRandomChaseLocationTimer, FMath::FRandRange(1.f, 2.5f));
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

		if (!IsPointReachable(RandomTargetLocation, NavSystem) && !bStopMovement)
		{
        	FVector LaunchVel{0.5f, 0.5f, 1.f};
        	LaunchCharacter(650.f*LaunchVel, false, false);
		}
	}
}

void AAICharacterBase::OnRep_TargetMovementSpeed(float OldTargetMovementSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = TargetMovementSpeed;
}

void AAICharacterBase::StartSearchTimer()
{
	TArray<AActor*> FoundShooters;
	if (TargetCharacter)
	{
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
	GetWorldTimerManager().SetTimer(SearchTimer, this, &AAICharacterBase::FinishSearchTimer, 1.f);
}

void AAICharacterBase::FinishSearchTimer()
{
	StartSearchTimer();
}

TArray<AActor*> AAICharacterBase::GetShooterPlayersInSphere(float SphereRadius)
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

FVector AAICharacterBase::GetPatrolPoint()
{
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS && ShooterGS->AIPatrolPoints.Num() > 0)
	{
		int32 RandomPatrolIndex = FMath::RandRange(0, ShooterGS->AIPatrolPoints.Num() - 1);
		ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
		if (ShooterAIController)
		{
			if (GetWorld() && GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
			{
				NavSystem = NavSystem == nullptr ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()) : NavSystem;
				if (NavSystem)
				{
					if (!IsPointReachable(ShooterGS->AIPatrolPoints[RandomPatrolIndex], NavSystem) && !bStopMovement)
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

void AAICharacterBase::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (!HasAuthority()) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	AShooterPlayerController* VictimController = nullptr; AShooterPlayerController* AttackerController = nullptr;
	AShooterPlayerState* VictimPS = nullptr; AShooterPlayerState* AttackerPS = nullptr;
	UCharacterGlobalFuncs::GetShooterReferences(VictimController, AttackerController, VictimPS, AttackerPS, DamagedActor, DamageCauser, InstigatorController);
	UCharacterGlobalFuncs::UpdateDamageStat(DamagedActor, DamageCauser, VictimPS, AttackerPS, Damage, Health, DamageType);
	if (!CurrentAttackMontage) SetMovement(false);
	if (AttackerController && AttackerController->MainCharacter && TargetCharacter != AttackerController->MainCharacter)
	{
		OnMissedShot_Implementation(AttackerController->MainCharacter, FVector());
		//SetTargetCharacter(AttackerController->MainCharacter);
	}
	else if (InstigatorController && InstigatorController->GetPawn() && TargetCharacter != InstigatorController->GetPawn())
	{
		OnMissedShot_Implementation(InstigatorController->GetPawn(), FVector());
		//SetTargetCharacter(InstigatorController->GetPawn());
	}

	if (Health <= 0)
    {
		OnTakeAnyDamage.RemoveDynamic(this, &AAICharacterBase::ReceiveDamage);
		bCanAttack = false;
		AShooterGameState* ShooterGS = GetShooter_GS();

		if (AttackerPS && ShooterGS) 
		{
			if (DamageCauser && AttackerPS->IsABot())
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
				if (ShooterController)
				{
					ShooterController->ClientAIElimAnnouncement(AttackerPS, AttackerPS->DamageType, AIName);
				}
			}
		}
		MulticastElimAI();
	}
}

void AAICharacterBase::MulticastElimAI_Implementation()
{
	if (HasAuthority())
	{
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS && ShooterGS->AIShooterArray.Contains(this))
		{
			ShooterGS->AIShooterArray.Remove(this);
			//ShooterGS->CheckWinnerAfterAIKillTimer();
		}
		FTimerHandle DestroyTimer;
		GetWorldTimerManager().SetTimer(DestroyTimer, this, &AAICharacterBase::HandleAIDestroy, 3.5f);
		ShooterAIController = ShooterAIController == nullptr ? Cast<AShooterAIController>(GetController()) : ShooterAIController;
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

	SetMovement(true);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EnemyData.DeathMontage) PlayDeathMontage(EnemyData.DeathMontage);
	FTimerHandle DissolveTimer;
	GetWorldTimerManager().SetTimer(DissolveTimer, this, &AAICharacterBase::StartDissolveEffect, 1.2f);
}

void AAICharacterBase::OnElimmed_Implementation()
{
	MulticastElimAI();
}

void AAICharacterBase::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
		//SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
	}
}

void AAICharacterBase::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AAICharacterBase::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->PlayFromStart();
	}
}

void AAICharacterBase::PlayTauntSound_Implementation()
{
	if (EnemyData.TauntSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EnemyData.TauntSound, GetActorLocation());
	}
}

void AAICharacterBase::GetAllSkeletalMeshMaterials()
{
    // Clear previous data
    OriginalMaterialsMap.Empty();

    // Iterate through all SkeletalMeshComponents in the character
    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

    for (USkeletalMeshComponent* MeshComp : SkeletalMeshes)
    {
        if (!MeshComp) continue;

        // Store all materials per mesh
        TArray<UMaterialInterface*> Materials;
        for (int32 MatIndex = 0; MatIndex < MeshComp->GetNumMaterials(); ++MatIndex)
        {
            Materials.Add(MeshComp->GetMaterial(MatIndex));
        }

        // Save to map
        OriginalMaterialsMap.Add(MeshComp, Materials);
    }
	//GetMaterialsToHideForFPS();
}

void AAICharacterBase::SetAllSkeletalMeshMaterials(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst)
{
    // Iterate through all SkeletalMeshComponents
    TArray<USkeletalMeshComponent*> SkeletalMeshes;
    GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

    for (USkeletalMeshComponent* MeshComp : SkeletalMeshes)
    {
        if (!MeshComp) continue;

        // Apply effect material OR restore original
        for (int32 MatIndex = 0; MatIndex < MeshComp->GetNumMaterials(); ++MatIndex)
        {
            if (!bSetDefaultMat && DynamicInst && MeshComp->GetMaterial(MatIndex) && MeshComp->GetMaterial(MatIndex)->GetName() != TEXT("Transparent2"))
            {
                MeshComp->SetMaterial(MatIndex, DynamicInst);
            }
            else
            {
                // Restore original material if it was stored
                if (OriginalMaterialsMap.Contains(MeshComp))
                {
                    TArray<UMaterialInterface*>& StoredMaterials = OriginalMaterialsMap[MeshComp];
                    if (StoredMaterials.IsValidIndex(MatIndex))
                    {
                        MeshComp->SetMaterial(MatIndex, StoredMaterials[MatIndex]);
                    }
                }
            }
        }
    }
}

void AAICharacterBase::PlayDeathMontage(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay && GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			MontageToPlay->bEnableAutoBlendOut = false;
			AnimInstance->StopAllMontages(0.f);
			float MontageDuration = AnimInstance->Montage_Play(MontageToPlay);
		}
	}
}

void AAICharacterBase::StartDissolveEffect()
{
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

	if (ElimSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimSound,
			GetActorLocation()
		);
	}
}

void AAICharacterBase::HandleAIDestroy()
{
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

void AAICharacterBase::OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || !InstigatorShooter->GetEquippedWeapon()) return;

	bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head") ? true : false;
	float DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
	if (DamageAmount == -1.f) return;
	if (InstigatorShooter->HasAuthority())
	{
		TSubclassOf<UDamageType> DamageTypeClass = UDamageType::StaticClass();
		if (InstigatorPS)
		{
			//InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Gun;
			if (bHeadShot && !InstigatorPS->IsABot())
			{
				InstigatorPS->AddToXPMetric(EProgressMetric::EPM_AiHeadShots, 1);
			}
			switch (InstigatorShooter->GetEquippedWeapon()->GetWeaponType())
			{
				case EWeaponType::EWT_AR:
					DamageTypeClass = UARType::StaticClass();
					InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_AR;
					break;
				case EWeaponType::EWT_Pistol:
					DamageTypeClass = UPistolType::StaticClass();
					InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Pistol;
					break;
				case EWeaponType::EWT_Sniper:
					DamageTypeClass = USniperType::StaticClass();
					InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Sniper;
					break;
				case EWeaponType::EWT_SubmachineGun:
					DamageTypeClass = USubmachineGunType::StaticClass();
					InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_SMG;
					break;
				
				default:
					InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_SMG;
					break;
			}
		}

		
		UGameplayStatics::ApplyDamage(
			this,
			DamageAmount,
			InstigatorShooter->GetController(),
			InstigatorShooter,
			DamageTypeClass
		);
	}

	if (InstigatorShooter->IsLocallyControlled())
	{
		if (bHeadShot && InstigatorShooter->GetHeadShotSound()) UGameplayStatics::PlaySound2D(InstigatorShooter, InstigatorShooter->GetHeadShotSound());
		InstigatorShooter->ShowHitNumber(DamageAmount, InHitResult.ImpactPoint, bHeadShot, false);
	}
}

void AAICharacterBase::OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter->GetEquippedWeapon() == nullptr)) return;
	float DamageAmount;
	TMap<AAICharacterBase*, float> HitMap;

	for (const FHitResult& WeaponTraceHit : InHitResults)
	{
		if (WeaponTraceHit.GetActor() == this)
		{
			DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetShotgunDamageAmount(InstigatorShooter, nullptr, WeaponTraceHit);
			if (DamageAmount == -1.f) continue;
			//Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
			if (HitMap.Contains(this)) HitMap[this] += DamageAmount;
			else HitMap.Emplace(this, DamageAmount);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				InstigatorShooter,
				HitSound,
				WeaponTraceHit.ImpactPoint,
				0.4f
			);
		}
	}

	// Calculate body shot damage by multiplying times hit x Damage - store in DamageMap

	for (const TPair<AAICharacterBase*, float>& HitPair : HitMap)
	{
		if (HitPair.Key)
		{
			if (HasAuthority())
			{
				//ScreenMessage(TEXT("Hit Character!!"));
				if (InstigatorPS)
				{
					InstigatorPS->DamageType = EShooterDamageType::ESDT_Shotgun;
				}
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorShooter->GetController(),
					InstigatorShooter,
					UDamageType::StaticClass()
				);
				//UE_LOG(LogTemp, Warning, TEXT("Health: %f"), HitPair.Key->GetHealth());

			}
			if (InstigatorShooter->IsLocallyControlled())
			{
				InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false, false);
			}
		}
	}
}

void AAICharacterBase::OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType)
{
	if (!InstigatorController || !HasAuthority()) return;

	APawn* InstigatorPawn = InstigatorController->GetPawn();
	AShooterCharacter* InstigatorShooter = Cast<AShooterCharacter>(InstigatorPawn);
	if (InstigatorPawn && this && InstigatorPawn != this)
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnProjectileWeaponHit %s, %s"), *InstigatorActor->GetName(), *InstigatorActor->GetInstigatorController()->GetName());
		bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head");
        float DamageToApply = InstigatorShooter && InstigatorShooter->GetEquippedWeapon() ? 
            InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString()) : Damage;

		AShooterPlayerState* InstigatorPS = Cast<AShooterPlayerState>(InstigatorController->PlayerState);
		if (InstigatorPS)
		{
			InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Gun;
			if (bHeadShot && !InstigatorPS->IsABot())
			{
				InstigatorPS->AddToXPMetric(EProgressMetric::EPM_AiHeadShots, 1);
			}
			if (InstigatorShooter && InstigatorPS && !InstigatorPS->IsABot()) InstigatorShooter->ClientOnProjectileHit(GetActorLocation(), DamageToApply, bHeadShot);
		}
		UGameplayStatics::ApplyDamage(this, DamageToApply, InstigatorController, DamageCauser, DamageType);
	}
}

void AAICharacterBase::OnHandCombatHit_Implementation(AShooterCharacter* InstigatorShooter, float Damage, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult)
{
	HitCharacter(this, Damage, InstigatorPS);
	if (InstigatorShooter)
	{
		if (InstigatorShooter->IsLocallyControlled())
		{
			InstigatorShooter->ShowHitNumber(Damage, this->GetActorLocation(), false, false);
		}
		InstigatorShooter->PunchedEffect(InHitResult.Location, false);
	}
}

void AAICharacterBase::HitCharacter(AActor* DamagedActor, float HitDamage, AShooterPlayerState* InstigatorPS)
{
	if (HasAuthority() && InstigatorPS && DamagedActor)
	{
		InstigatorPS->DamageType = EShooterDamageType::ESDT_Hand;

		UGameplayStatics::ApplyDamage(
			DamagedActor,
			HitDamage,
			InstigatorPS->GetOwningController(),
			InstigatorPS->GetPawn(),
			UDamageType::StaticClass()
		);
	}
}

void AAICharacterBase::PerformJumpAttack()
{
    if (!TargetCharacter) return;

    FVector StartLocation = GetActorLocation();
    FVector EndLocation = TargetCharacter->GetActorLocation();

    FVector LaunchVelocity;
	UGameplayStatics::FSuggestProjectileVelocityParameters Params(GetWorld(), StartLocation, EndLocation, 600.f);
	Params.OverrideGravityZ = 0.f;
	Params.ResponseParam.CollisionResponse.SetResponse(ECC_Pawn, ECollisionResponse::ECR_Block);
	Params.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;
	Params.ActorsToIgnore.Add(this); // Optional: ignore self in traces

	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity(Params, LaunchVelocity);

    if (bSuccess)
    {
        LaunchCharacter(LaunchVelocity, true, true);
    }
}

void AAICharacterBase::Attack()
{
	GetWorldTimerManager().ClearTimer(AttackHandle);
	if (!bCanAttack || !TargetCharacter || bIsTeleporting || bIsStunned) return;
	if (!EnemyData.AttackVariants.IsValidIndex(0)) 
	{
		FTimerHandle AttackInvalidTimer;
		GetWorld()->GetTimerManager().SetTimer(AttackInvalidTimer, this, &AAICharacterBase::Attack, FMath::FRandRange(0.25f, 1.f), false);
	}

	float RandomValue = FMath::FRand();
	if (RandomValue <= 0.6f)
	{
		if (EnemyData.AttackVariants.Num() > 1)
		{
			CurrentAttack = EnemyData.AttackVariants[FMath::RandRange(1, EnemyData.AttackVariants.Num() - 1)];
		}
		else
		{
			CurrentAttack = EnemyData.AttackVariants[0];
		}
	}
	else
	{
		CurrentAttack = EnemyData.AttackVariants[0];
	}
	
	//CurrentAttack = EnemyData.AttackVariants[1];
	//if (CurrentAttack.AttackMontage) UE_LOG(LogTemp, Warning, TEXT("CURRENT ATTACK = %s"), *CurrentAttack.AttackMontage->GetName());
	MulticastAttack(CurrentAttack);
}

void AAICharacterBase::MulticastAttack_Implementation(const FEnemyAttackData& ServerAttackData)
{
	CurrentAttack = ServerAttackData;
	PlayAttackMontage(CurrentAttack.AttackMontage);
}

void AAICharacterBase::PlayAttackMontage(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay && GetMesh() && GetMesh()->GetAnimInstance())
	{
		CurrentFireNotifyIndex = 0;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		float MontageDuration = AnimInstance->Montage_Play(MontageToPlay);

		if (HasAuthority() && MontageDuration > 0.f)
		{
			// Bind delegate
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &AAICharacterBase::OnAttackMontageEnded);

			AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

			// Optional: store the current montage or attack data if needed
			CurrentAttackMontage = MontageToPlay;
		}
	}
}

void AAICharacterBase::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == CurrentAttackMontage)
	{
		CurrentAttackMontage = nullptr;
        GetWorld()->GetTimerManager().SetTimer(AttackHandle, this, &AAICharacterBase::Attack, FMath::FRandRange(0.25f, 1.f), false);
		//UE_LOG(LogTemp, Warning, TEXT("OnAttackMontageEnded"));
		//GetWorld()->GetTimerManager().SetTimer(AttackHandle, this, &AAICharacterBase::Attack, 8.f, false);
	}
}

void AAICharacterBase::SetMovement(bool bIsStopMovement)
{
	bStopMovement = bIsStopMovement;
	if (bIsStopMovement && ShooterAIController)
	{
		ShooterAIController->StopMovement();
		if (TargetCharacter) ShooterAIController->SetFocus(TargetCharacter);
	}
}

void AAICharacterBase::Fire()
{
	UWorld* World = GetWorld();
	APawn* InstigatorPawn = Cast<APawn>(this);
	FName SocketName = GetNextFireSocket();
	const USkeletalMeshSocket* MuzzleFlashSocket = GetMesh()->GetSocketByName(SocketName);
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetMesh());
		if (CurrentAttack.ParticleData.MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, CurrentAttack.ParticleData.MuzzleFlash, SocketTransform);
			//UGameplayStatics::SpawnEmitterAttached(CurrentAttack.MuzzleFlash, GetMesh(), SocketName, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator(), EAttachLocation::KeepRelativeOffset);
		}
		if (CurrentAttack.SoundData.FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(World, CurrentAttack.SoundData.FireSound, GetActorLocation());
		}
		
		if (HasAuthority() && InstigatorPawn && CurrentAttack.ProjectileClass && TargetCharacter)
		{
			FVector TargetLocation = TargetCharacter->GetActorLocation() + FVector(0.f, 0.f, 40.f) - SocketTransform.GetLocation();
			FRotator TargetRotation = CurrentAttack.bIsFiredDirectlyToTarget ? 
				TargetLocation.Rotation() : SocketTransform.GetRotation().Rotator();
			FActorSpawnParameters SpawnParams;
			//SpawnParams.Owner = AttackerCharacter;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = InstigatorPawn;
			//SetInstigator(InstigatorPawn);
			
			AProjectileBullet* Projectile  = World->SpawnActor<AProjectileBullet>(
				CurrentAttack.ProjectileClass,
				MuzzleFlashSocket->GetSocketLocation(GetMesh()),
				TargetRotation,
				SpawnParams
				);

			if (Projectile)
			{
				CurrentAttack.TargetActor = TargetCharacter;
				Projectile->Initialize(CurrentAttack);
				if (auto AiPS = Cast<AShooterPlayerState>(GetPlayerState())) AiPS->PlayerGameStats.ShotsFired++;
			}
		}
	}
}

FName AAICharacterBase::GetNextFireSocket()
{
	CurrentFireNotifyIndex = CurrentAttack.FireSockets.IsValidIndex(CurrentFireNotifyIndex) ? CurrentFireNotifyIndex : 0;
	FName SocketName = CurrentAttack.FireSockets[CurrentFireNotifyIndex];
	CurrentFireNotifyIndex++;
	return SocketName;
}

void AAICharacterBase::SetEnableTeleport()
{
	bCanTeleport = true;
}

void AAICharacterBase::MulticastStartTeleport_Implementation(UAnimMontage* MontageToPlay)
{
	if (MontageToPlay && GetMesh())
	{
		if (HasAuthority())
		{
			bIsTeleporting = true;
			//if (ShooterAIController) ShooterAIController->StopMovement();
		}
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(MontageToPlay);
		}
	}
}

void AAICharacterBase::ServerStartTeleport()
{
    if (!HasAuthority() || !TeleportTrailActorClass)
    {
        return;
    }
	
	bCanTeleport = false;
	GetWorld()->GetTimerManager().SetTimer(TeleportHandle, this, &AAICharacterBase::SetEnableTeleport, 2.f, false);

    TeleportStartLocation = GetActorLocation();
    TeleportTargetLocation = GetRandomTeleportLocation();
    MulticastSetVisibility(false);

    ATeleportTrail* TrailActor = GetWorld()->SpawnActor<ATeleportTrail>(TeleportTrailActorClass, TeleportStartLocation, FRotator::ZeroRotator);
    if (TrailActor)
    {
        TrailActor->InitTrail(TeleportStartLocation, TeleportTargetLocation, EnemyData.TeleportData.TeleportSpeed, this);
    }
    
    // Reset teleport progress
    TeleportProgress = 0.f;
}

void AAICharacterBase::CompleteTeleport()
{
	SetActorLocation(TeleportTargetLocation);
	MulticastSetVisibility(true);
	bIsTeleporting = false;
	GetWorld()->GetTimerManager().SetTimer(AttackHandle, this, &AAICharacterBase::Attack, 0.5f, false);
}

void AAICharacterBase::MulticastSetVisibility_Implementation(bool bIsVisible)
{
    SetAIVisibility(bIsVisible);
}

void AAICharacterBase::SetAIVisibility(bool bIsVisible)
{
    //GetMesh()->SetVisibility(bIsVisible);
	if (!bIsVisible)
	{
		if (TransparentMaterial)
		{
			DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(TransparentMaterial, this);
			SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
		}
	}
	else
	{
		SetAllSkeletalMeshMaterials(true, nullptr);
		DynamicDissolveMaterialInstance = nullptr;
	}
    SetActorEnableCollision(bIsVisible);
}


FVector AAICharacterBase::GetRandomTeleportLocation() const
{
    if (!TargetCharacter)
    {
        return GetActorLocation(); // Fallback to current location
    }

    FVector PlayerLocation = TargetCharacter->GetActorLocation();
    FVector RandomOffset(
        FMath::RandRange(-EnemyData.TeleportData.TeleportLocationRandomization.X, EnemyData.TeleportData.TeleportLocationRandomization.X),
        FMath::RandRange(-EnemyData.TeleportData.TeleportLocationRandomization.Y, EnemyData.TeleportData.TeleportLocationRandomization.Y),
        FMath::RandRange(double(0), EnemyData.TeleportData.TeleportLocationRandomization.Z)
    );

    return PerformAdjustedLineTrace(GetActorLocation(), PlayerLocation + RandomOffset);
}

FVector AAICharacterBase::PerformAdjustedLineTrace(const FVector& Start, const FVector& Target) const
{
	UWorld* World = GetWorld();
	if (!World) return Target;
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = World->LineTraceSingleByChannel(HitResult, Start, Target, ECC_Visibility, Params);

    if (bHit)
    {
        FVector HitLocation = HitResult.Location;
        FVector TraceDirection = (Target - Start).GetSafeNormal();
        return HitLocation - TraceDirection * 50.0f; // 50 units = 50 cm
    }

    return Target;
}

void AAICharacterBase::OnSlowDown_Implementation(bool bIsSlow, AShooterCharacter* InstigatorShooter)
{
	if (bIsSlow)
	{
		bSlowDownTrigger = true;
		CustomTimeDilation = 0.5f;
	}
	else if (!bIsSlow)
	{
		bSlowDownTrigger = false;
		CustomTimeDilation = 1.f;
	}
}

void AAICharacterBase::OnRep_SlowDownFactor()
{
	if (bSlowDownTrigger)
	{
		CustomTimeDilation = 0.5f;
	} 
	else
	{
		CustomTimeDilation = 1.f;
	}
}

void AAICharacterBase::OnGravityProjectileHit_Implementation(bool bIsPulling, AActor* ProjectileGravity)
{
	//bIsPulling ? SetMovement(true) : SetMovement(false);
	bGravityPull = bIsPulling;
	GravityProjectileActor = ProjectileGravity;
}

void AAICharacterBase::PullToGravityProjectile()
{
    if (!bGravityPull || !GravityProjectileActor) return;

	FVector VecToProjectile = GravityProjectileActor->GetActorLocation() - GetActorLocation();
	AddMovementInput(VecToProjectile.GetSafeNormal(), 1.05f);
}

USceneComponent* AAICharacterBase::OnGameEndWinner_Implementation()
{
	OnWinnerLogic();
	return SceneCameraPositionForInterp;
}

void AAICharacterBase::OnWinnerLogic()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	bCanAttack = false;
	SetTargetCharacter(nullptr);
	if (GetWorldTimerManager().IsTimerActive(SearchTimer)) GetWorldTimerManager().ClearTimer(SearchTimer);
	if (ShooterAIController && GetController())
	{
		ShooterAIController->UnPossess();
		if (EnemyData.WinnerMontage)
		{
			int32 RandSectionInd = FMath::RandRange(0, EnemyData.WinnerMontage->GetNumSections() - 1);
			FName Section = EnemyData.WinnerMontage->GetSectionName(RandSectionInd);
			Multicast_PlayWinnerMontage(EnemyData.WinnerMontage, Section);
		}
	}
}

void AAICharacterBase::Multicast_PlayWinnerMontage_Implementation(UAnimMontage* MontageToPlay, const FName& Section)
{
	if (MontageToPlay && GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->StopAllMontages(0.f);
			AnimInstance->Montage_Play(MontageToPlay);
			AnimInstance->Montage_JumpToSection(Section);
		}
	}
}

void AAICharacterBase::OnAIStopAttacking()
{
	bCanAttack = false;
}

void AAICharacterBase::OnMissedShot_Implementation(AActor* InstigatorShooter, const FVector& ImpactPoint)
{
	if (HasAuthority() && InstigatorShooter && !CurrentAttackMontage)
	{
		if (bCanTeleport && !bIsTeleporting && EnemyData.TeleportData.bCanTeleport)
		{
			if (TargetCharacter != InstigatorShooter) SetTargetCharacter(InstigatorShooter);
			MulticastStartTeleport(EnemyData.TeleportData.TeleportMontage);
		}
	}
}

void AAICharacterBase::OnStunned_Implementation(AActor* StunnerActor)
{
	if (Health <= 0.f || bIsStunned || bIsTeleporting) return;
	bIsStunned = true;
	if (StunnerActor) SetTargetCharacter(StunnerActor);
	StopCurrentMontage();
	OnRep_IsStunned();
	SetMovement(true);
	GetWorldTimerManager().ClearTimer(AttackHandle);
}

void AAICharacterBase::OnRep_IsStunned()
{
	if (bIsStunned)
	{
		if (StunMaterialInstance && GetMesh())
		{
			GetMesh()->SetOverlayMaterial(StunMaterialInstance);
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

			if(AnimInstance && EnemyData.DeathMontage && StunnedSound)
			{
				EnemyData.DeathMontage->bEnableAutoBlendOut = true;
				AnimInstance->Montage_Play(EnemyData.DeathMontage);
				AnimInstance->Montage_JumpToSection(FName("Stun"));
				UGameplayStatics::PlaySoundAtLocation(this, StunnedSound, GetActorLocation());

				if (HasAuthority())
				{
					/*
					FOnMontageEnded EndDelegate;
					EndDelegate.BindUObject(this, &AAICharacterBase::OnStunMontageEnded);
					AnimInstance->Montage_SetEndDelegate(EndDelegate, EnemyData.DeathMontage);
					*/
					if (UWorld* World = GetWorld())
					{
						FTimerHandle StunTimer;
						World->GetTimerManager().SetTimer(StunTimer, this, &AAICharacterBase::OnStunEnded, 2.1f, false);
					}
				}
			}
		}
	}
	else
	{
		if (GetMesh()) GetMesh()->SetOverlayMaterial(nullptr);
		if (UWorld* World = GetWorld())
		{
			if (HasAuthority())
			{
				bCanAttack = true;
				World->GetTimerManager().SetTimer(AttackHandle, this, &AAICharacterBase::Attack, 0.5f, false);
			}
		}
	}
}

void AAICharacterBase::OnStunEnded()
{
	bIsStunned = false;
	OnRep_IsStunned();
	SetMovement(false);
}

void AAICharacterBase::OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	OnStunEnded();
}
	
void AAICharacterBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bGravityPull);
	DOREPLIFETIME(ThisClass, GravityProjectileActor);
	DOREPLIFETIME(ThisClass, bSlowDownTrigger);
	DOREPLIFETIME(ThisClass, bSpawnedIn);
	DOREPLIFETIME(ThisClass, bHasFinishedDissolve);
	DOREPLIFETIME(ThisClass, EnemyReplicationData);
	DOREPLIFETIME(ThisClass, TargetMovementSpeed);
	DOREPLIFETIME(ThisClass, TargetCharacter);
	DOREPLIFETIME(ThisClass, bIsStunned);

}
