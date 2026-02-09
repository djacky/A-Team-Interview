// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacterAI.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimMontage.h"
#include "ShooterAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ShooterSpringArmComp.h"
#include "Camera/CameraComponent.h"
#include "Shooter/AI/ShooterAI.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/ShooterPlayerState.h"
#include "ShooterMovementComponent.h"

AShooterCharacterAI::AShooterCharacterAI(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = BaseJumpVelocity;
	GetCharacterMovement()->AirControl = 0.4f;
}

void AShooterCharacterAI::BeginPlay()
{
	Super::BeginPlay();
	bIsAI = true;
	SetCanTarget(true);
    ShieldOn(false);
	CameraBoom->DestroyComponent();
	FollowCamera->DestroyComponent();
}

void AShooterCharacterAI::OnSpawn(AShooterCharacter* ShooterOwner)
{
	if (HasAuthority() && ShooterOwner)
	{
		SetOwner(ShooterOwner);
		FAIMeshProperties MeshProps;
		MeshProps.AnimInstanceClass = ShooterOwner->GetMesh()->GetAnimInstance()->GetClass();
		MeshProps.MeshAsset = ShooterOwner->GetMesh()->GetSkeletalMeshAsset();
		MeshProps.CharacterSelected = ShooterOwner->GetCharacterSelected();
		MeshProperties = MeshProps;
		OnRep_OnSpawn();
	}
}

void AShooterCharacterAI::OnRep_OnSpawn()
{
	//ShooterAI->GetMesh()->SetAnimClass(GetMesh()->AnimClass);
	GetMesh()->SetSkeletalMesh(MeshProperties.MeshAsset);
	GetMesh()->SetAnimInstanceClass(MeshProperties.AnimInstanceClass);
	SetCharacterSelected(MeshProperties.CharacterSelected);
	switch (MeshProperties.CharacterSelected)
	{
		case ECharacterProperty::ECP_Leo:
			{
				TArray<UActorComponent*> ActorComps;
				ActorComps = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("HairTag"));
				if (ActorComps.IsValidIndex(0) && ActorComps[0])
				{
					auto HairMesh = Cast<USkeletalMeshComponent>(ActorComps[0]);
					if (HairMesh)
					{
						UE_LOG(LogTemp, Warning, TEXT("OnRep_OnSpawn"));
						HairMesh->SetLeaderPoseComponent(GetMesh());
						HairMesh->SetHiddenInGame(false);
					}
				}
			}
			break;
		default:
			break;
	}
	if (SpawnCopyEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			SpawnCopyEffect,
			GetActorLocation() + FVector(0.f, 0.f, 150.f),
			GetActorRotation(),
			true // auto destroy after playing
		);
	}

	if (SpawnCopySound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			SpawnCopySound,
			GetActorLocation()
		);
	}
}

void AShooterCharacterAI::Tick(float DeltaTime)
{

}

void AShooterCharacterAI::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
    if (!HasAuthority()) return;

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
    if (Health <= 0)
    {
		AActor* InstigatorActor = InstigatorController ? InstigatorController->GetPawn() : DamageCauser;
		OnTakeAnyDamage.RemoveDynamic(this, &AShooterCharacterAI::ReceiveDamage);
		if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InstigatorActor))
		{
			EnemyInterface->Execute_SetTarget(InstigatorActor, this, true);
		}
        MulticastElimAI();
    }
}

void AShooterCharacterAI::PlayElimMontageAI()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ElimMontageAI)
	{
		AnimInstance->Montage_Play(ElimMontageAI);
		AnimInstance->Montage_JumpToSection(FName("Eliminate"));
	}
}

void AShooterCharacterAI::MulticastElimAI_Implementation()
{
    //PlayElimMontageAI();
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
    
	if (ElimSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimSound,
			GetActorLocation()
		);
	}
}

// Since the camera is destroyed, should stop functions that involved camera
void AShooterCharacterAI::CameraOffsetWithDistance()
{
	//
}

void AShooterCharacterAI::OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || !InstigatorShooter->GetEquippedWeapon()) return;

	bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head") ? true : false;
	float DamageToShow = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
	if (DamageToShow == -1.f) return;
	if (InstigatorShooter->HasAuthority())
	{
		UGameplayStatics::ApplyDamage(
			this,
			DamageToShow,
			InstigatorShooter->GetController(),
			InstigatorShooter,
			UDamageType::StaticClass()
		);
	}

	if (InstigatorShooter->IsLocallyControlled())
	{
		InstigatorShooter->ShowHitNumber(DamageToShow, InHitResult.ImpactPoint, bHeadShot);
	}

	UGameplayStatics::PlaySoundAtLocation(
		InstigatorShooter,
		HitSound,
		InHitResult.ImpactPoint,
		0.2f
	);
}

void AShooterCharacterAI::OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter->GetEquippedWeapon() == nullptr)) return;
	TMap<AShooterCharacterAI*, float> HitMapAIChar;
	float DamageAmount;
	for (const FHitResult& WeaponTraceHit : InHitResults)
	{
		if (WeaponTraceHit.GetActor() == this)
		{
			DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
			if (HitMapAIChar.Contains(this)) HitMapAIChar[this] += DamageAmount;
			else HitMapAIChar.Emplace(this, DamageAmount);
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

	for (auto HitPair : HitMapAIChar)
	{
		if (HitPair.Key)
		{
			if (InstigatorShooter->HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorShooter->GetController(),
					InstigatorShooter,
					UDamageType::StaticClass()
				);
			}
			if (InstigatorShooter->IsLocallyControlled())
			{
				InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
			}
		}
	}
}

void AShooterCharacterAI::OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType)
{
	if (!InstigatorController) return;
	APawn* InstigatorPawn = InstigatorController->GetPawn();
	AShooterCharacter* InstigatorShooter = Cast<AShooterCharacter>(InstigatorPawn);

	if (this && InstigatorPawn && InstigatorPawn != this)
	{
		bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head");
		float DamageToApply = 0.f;

		AShooterPlayerState* InstigatorPS = Cast<AShooterPlayerState>(InstigatorController->PlayerState);
		// You can now support AI agents or turrets with their own logic
		if (InstigatorShooter && InstigatorShooter->GetEquippedWeapon())
		{
			DamageToApply = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
		}
		else if (Damage > 0.f)
		{
			DamageToApply = Damage;
		}

		if (InstigatorPawn->HasAuthority())
		{
			UGameplayStatics::ApplyDamage(this, DamageToApply, InstigatorController, DamageCauser, DamageType);
			if (InstigatorShooter && InstigatorPS && !InstigatorPS->IsABot()) InstigatorShooter->ClientOnProjectileHit(GetActorLocation(), DamageToApply, bHeadShot, false);
		}
	}
}


void AShooterCharacterAI::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MeshProperties);
}

