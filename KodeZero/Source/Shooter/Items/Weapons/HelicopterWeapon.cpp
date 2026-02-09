// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/HelicopterWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Helicopter.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterCharacter.h"
#include "ProjectileHelicopterPulser.h"


AHelicopterWeapon::AHelicopterWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
	SetReplicatingMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	SetRootComponent(WeaponMesh);
}

void AHelicopterWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AHelicopterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHelicopterWeapon::UpdateWeapon()
{
	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	const FString WeaponTablePath(TEXT("DataTable'/Game/_Game/DataTables/HelicopterDateTable.HelicopterDateTable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));
	if (WeaponTableObject)
	{
		FHelicopterWeaponDataTable* WeaponRow = nullptr;
		switch (WeaponType)
		{
			case EHelicopterWeaponType::EHWT_PulsedBlaster:
			WeaponRow = WeaponTableObject->FindRow<FHelicopterWeaponDataTable>(FName("PulsedBlaster"), TEXT(""));
			break;

			case EHelicopterWeaponType::EHWT_Missile:
			WeaponRow = WeaponTableObject->FindRow<FHelicopterWeaponDataTable>(FName("Missile"), TEXT(""));
			break;
		}
		if (WeaponRow)
		{
			ProjectileClass = WeaponRow->ProjectileClass;
        	EquipSound = WeaponRow->EquipSound;
            WeaponMesh->SetSkeletalMesh(WeaponRow->WeaponMesh);
           	WeaponName = WeaponRow->WeaponName;
			bAutomatic = WeaponRow->bAutomatic;
			WeaponDamageAmount = WeaponRow->WeaponDamageAmount;
			InventoryIcon = WeaponRow->InventoryIcon;
			IconBackground = WeaponRow->IconBackground;
			CrosshairMiddle = WeaponRow->CrosshairMiddle;
			AutoFireRate = WeaponRow->AutoFireRate;
			FireSound = WeaponRow->FireSound;
			HitSound = WeaponRow->HitSound; //not used for projectiles
			MuzzleFlash = WeaponRow->MuzzleFlash;
			bUsesAmmo = WeaponRow->bUsesAmmo;
			AmmoType = WeaponRow->AmmoType;
			WeaponAmmoCapacity = WeaponRow->WeaponAmmoCapacity;
		}
	}  
}

void AHelicopterWeapon::OnRep_WeaponType()
{
	UpdateWeapon();
}

void AHelicopterWeapon::FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter, APawn* ControllingPawn)
{
	APawn* InstigatorPawn = ControllingPawn;
	//UE_LOG(LogTemp, Warning, TEXT("InstigatorPawn=%s"), *GetInstigator()->GetName());

	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, WeaponTraceHit.TraceStart);
	}
	// From muzzle flash socket to hit location from TraceUnderCrosshairs
	FRotator TargetRotation = (WeaponTraceHit.ImpactPoint - WeaponTraceHit.TraceStart).Rotation();
	if (InstigatorPawn && ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = AttackerCharacter;
		SpawnParams.Instigator = InstigatorPawn;
		SetInstigator(InstigatorPawn);
		//UE_LOG(LogTemp, Warning, TEXT("Shooting"));
		
		if (InstigatorPawn->HasAuthority())
		{
			UWorld* World = GetWorld();
			if (World)
			{
				AProjectile* HeliProjectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					WeaponTraceHit.TraceStart,
					TargetRotation,
					SpawnParams
					);
				if (HeliProjectile) HeliProjectile->StartDestroyCheck();
			}
		}
	}
}

void AHelicopterWeapon::OnEquipped()
{
	Sequence = 0;
}

void AHelicopterWeapon::SpendRound(AShooterCharacter* ShooterChar)
{
	Ammo = FMath::Clamp(Ammo - 1, 0, WeaponAmmoCapacity);
	if (HasAuthority())
	{
		ClientUpdateAmmo(Ammo);
		ShooterChar->UpdateAmmoForHelicopter(AmmoType, Ammo);
	}
	else
	{
		++Sequence;
	}
}

void AHelicopterWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
}

void AHelicopterWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, SlotIndex);
	DOREPLIFETIME(ThisClass, WeaponType);
}

