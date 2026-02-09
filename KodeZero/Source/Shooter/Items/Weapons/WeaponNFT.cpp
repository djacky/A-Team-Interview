// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/WeaponNFT.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"
#include "Projectile.h"
#include "ProjectileBullet.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Shooter/Widgets/WeaponPickupWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

AWeaponNFT::AWeaponNFT()
{
    MainWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	MainWeaponMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    SetRootComponent(MainWeaponMesh);

    ClipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClipMesh"));
    ClipMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    ClipMesh->SetupAttachment(MainWeaponMesh);
    ClipMesh->SetSimulatePhysics(false);

    GetAreaSphere()->SetupAttachment(GetRootComponent());
    GetCollisionBox()->SetupAttachment(GetRootComponent());
    GetPickupWidget()->SetupAttachment(GetRootComponent());

	/*
	DroppedEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DroppedEffectComponent"));
	DroppedEffectComponent->SetupAttachment(GetRootComponent());
	DroppedEffectComponent->SetAutoActivate(false);
	DroppedEffectComponent->Deactivate();
	*/

	DropEffectTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DropEffectTimelineComponent"));
}

void AWeaponNFT::BeginPlay()
{
    Super::BeginPlay();
    GetItemMesh()->Deactivate();
    WeaponWidget = Cast<UWeaponPickupWidget>(GetPickupWidget()->GetWidget());
    if (WeaponWidget) WeaponWidget->HideStars();
}

void AWeaponNFT::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
	//UE_LOG(LogTemp, Warning, TEXT("testFloat = %f"), testFloat);
}

UStaticMeshComponent* AWeaponNFT::GetNFTMesh()
{
    return MainWeaponMesh;
}

void AWeaponNFT::SetItemProperties()
{
	Super::SetItemProperties();
	switch (GetItemState())
	{
		case EItemState::EIS_Pickup:
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			MainWeaponMesh->SetSimulatePhysics(false);
			MainWeaponMesh->SetEnableGravity(false);
			MainWeaponMesh->SetVisibility(true);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MainWeaponMesh->SetNotifyRigidBodyCollision(false);

			ClipMesh->SetVisibility(true);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ClipMesh->SetNotifyRigidBodyCollision(false);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
			StartPulseTimer();
            StartDropEffect(true);
			break;

		case EItemState::EIS_Pickup2:
			//MissileMovementComponent->Deactivate();
			// Set Mesh Properties
			MainWeaponMesh->SetSimulatePhysics(false);
			MainWeaponMesh->SetEnableGravity(false);
			MainWeaponMesh->SetVisibility(true);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MainWeaponMesh->SetNotifyRigidBodyCollision(false);

			ClipMesh->SetVisibility(true);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ClipMesh->SetNotifyRigidBodyCollision(false);
            StartDropEffect(true);
			break;

		case EItemState::EIS_Equipped:
		{
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			MainWeaponMesh->SetSimulatePhysics(false);
			MainWeaponMesh->SetEnableGravity(false);
			MainWeaponMesh->SetVisibility(true);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ClipMesh->SetVisibility(true);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
            StartDropEffect(false);
			break;
		}

		case EItemState::EIS_PickedUp:
        {
			MainWeaponMesh->SetSimulatePhysics(false);
			MainWeaponMesh->SetEnableGravity(false);
			MainWeaponMesh->SetVisibility(false);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ClipMesh->SetVisibility(false);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
            
			if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetOwner()))
			{
				//UE_LOG(LogTemp, Warning, TEXT("AttachItem"));
				//FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, false);
				//ItemMesh->AttachToComponent(ShooterChar->GetMesh(), AttachmentRules, FName("hand_r"));
				const USkeletalMeshSocket* HandSocket = ShooterChar->GetMesh()->GetSocketByName("RightHandSocket");
				if (HandSocket)
				{
					// Attach the weapon to the hand socket RightHandSocket
					HandSocket->AttachActor(this, ShooterChar->GetMesh());
				}

			}
            
            StartDropEffect(false);
			break;
        }

		case EItemState::EIS_Falling:
			// Set Mesh Properties
			//SetReplicatingMovement(false);
			MainWeaponMesh->SetSimulatePhysics(true);
			MainWeaponMesh->SetEnableGravity(true);
			MainWeaponMesh->SetVisibility(true);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MainWeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			MainWeaponMesh->SetNotifyRigidBodyCollision(true);

			ClipMesh->SetVisibility(true);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ClipMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			ClipMesh->SetNotifyRigidBodyCollision(true);
			//AmmoCollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//GetCollisionBox()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			//GetCollisionBox()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			//GetCollisionBox()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);

            StartDropEffect(false);
			break;

		case EItemState::EIS_EquipInterping:
			// Set Mesh Properties
			//SetReplicatingMovement(true);
			MainWeaponMesh->SetSimulatePhysics(false);
			MainWeaponMesh->SetEnableGravity(false);
			MainWeaponMesh->SetVisibility(true);
			MainWeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			MainWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ClipMesh->SetVisibility(true);
			ClipMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			ClipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GetCollisionBox()->SetNotifyRigidBodyCollision(false);
            StartDropEffect(false);
			break;
	}
}

void AWeaponNFT::OnRep_ItemState()
{
	Super::OnRep_ItemState();
	//SetItemProperties();
}

void AWeaponNFT::OnRep_WeaponData()
{
    bIsAnNFT = true;
    SetItemType(EItemType::EIT_Weapon);
    SetItemName(WeaponData.name);

    FString MainSoundPath = TEXT("/Game/_Game/Assets/Sounds/");
    FString MainParticlePath = TEXT("/Game/_Game/Assets/FX/");
    FString MainImagePath = TEXT("/Game/_Game/Assets/Textures/");
    FString OwnedPath = FString::Printf(TEXT("/Game/_Game/Weapons/BaseWeapon/NFTWeapon/OwnedNFTs/%s/"), *WeaponData.name);
    if (!WeaponData.offChainMetadata.Base.CacheID.IsEmpty())
    {
        FString MeshPath = FString::Printf(TEXT("%s%s.%s"), *OwnedPath, *WeaponData.offChainMetadata.Base.CacheID, *WeaponData.offChainMetadata.Base.CacheID);
        UStaticMesh* MainMesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        if (MainMesh) MainWeaponMesh->SetStaticMesh(MainMesh);
    }

    if (!WeaponData.offChainMetadata.Magazine.CacheID.IsEmpty())
    {
        FString MeshClipPath = FString::Printf(TEXT("%s%s.%s"), *OwnedPath, *WeaponData.offChainMetadata.Magazine.CacheID, *WeaponData.offChainMetadata.Magazine.CacheID);
        UStaticMesh* Clip = LoadObject<UStaticMesh>(nullptr, *MeshClipPath);
        if (Clip) ClipMesh->SetStaticMesh(Clip);
    }

	/*
    if (!WeaponData.FireSound.IsEmpty())
    {
        FString SoundPath;
        if (WeaponData.bCustomFireSound)
        {
            SoundPath = FString::Printf(TEXT("%s%s.%s"), *OwnedPath, *WeaponData.FireSound, *WeaponData.FireSound);
            FireSound = LoadObject<USoundCue>(nullptr, *SoundPath);
            if (FireSound && FireSoundAtt) FireSound->AttenuationSettings = FireSoundAtt;
        }
        else
        {
            SoundPath = FString::Printf(TEXT("%sGunShots/Cues/%s.%s"), *MainSoundPath, *WeaponData.FireSound, *WeaponData.FireSound);
            FireSound = LoadObject<USoundCue>(nullptr, *SoundPath);
        }
    }
	*/

    if (HasAuthority())
    {
		GetNFTMesh()->OnComponentHit.AddDynamic(this, &AWeaponNFT::OnHit);
    }

    AllMaterialsMain = MainWeaponMesh->GetMaterials();
    AllMaterialsClip = ClipMesh->GetMaterials();
    if (DropMaterialInstance) DynamicDropMaterialInstance = UMaterialInstanceDynamic::Create(DropMaterialInstance, this);

    FString AmmoIconPath;

    if (WeaponData.offChainMetadata.Bullet.ID.Equals(TEXT("9mm")))
    {
        AmmoType = EAmmoType::EAT_9mm;
		AmmoIconPath = FString::Printf(TEXT("%sIcons/SmallAmmo.SmallAmmo"), *MainImagePath);
    }
    else if (WeaponData.offChainMetadata.Bullet.ID.Equals(TEXT("45mm")))
    {
        AmmoType = EAmmoType::EAT_45mm;
		AmmoIconPath = FString::Printf(TEXT("%sIcons/SniperAmmo.SniperAmmo"), *MainImagePath);
    }
    else if (WeaponData.offChainMetadata.Bullet.ID.Equals(TEXT("Shells")))
    {
        AmmoType = EAmmoType::EAT_Shells;
		AmmoIconPath = FString::Printf(TEXT("%sIcons/ShotgunAmmo.ShotgunAmmo"), *MainImagePath);
    }
    else if (WeaponData.offChainMetadata.Bullet.ID.Equals(TEXT("33mm")))
    {
        AmmoType = EAmmoType::EAT_AR;
		AmmoIconPath = FString::Printf(TEXT("%sIcons/ARAmmo.ARAmmo"), *MainImagePath);
    }
    else if (WeaponData.offChainMetadata.Bullet.ID.Equals(TEXT("Rounds")))
    {
        AmmoType = EAmmoType::EAT_GrenadeRounds;
		AmmoIconPath = FString::Printf(TEXT("%sIcons/GrenadeRounds.GrenadeRounds"), *MainImagePath);
    }
    SetIconAmmo(LoadObject<UTexture2D>(nullptr, *AmmoIconPath));

    if (HasAuthority())
    {
        Ammo = WeaponData.offChainMetadata.Magazine.Capacity;
        ReplCachedAmmo = Ammo;
    }

	FireSound = LoadObject<USoundCue>(nullptr, *(FString::Printf(TEXT("%sGunShots/Cues/%s.%s"), *MainSoundPath, *WeaponData.offChainMetadata.Muzzle.FireSoundID, *WeaponData.offChainMetadata.Muzzle.FireSoundID)));
    bAutomatic = WeaponData.offChainMetadata.Base.Type.Equals(TEXT("Rifle")); // *** Need to add a property to set this in the struct
    SetPickupSound(LoadObject<USoundCue>(nullptr, *(FString::Printf(TEXT("%sGunShots/Pickup/%s.%s"), *MainSoundPath, *WeaponData.offChainMetadata.Base.PickupSoundID, *WeaponData.offChainMetadata.Base.PickupSoundID))));
    SetEquipSound(LoadObject<USoundCue>(nullptr, *(FString::Printf(TEXT("%sGunShots/Equip/%s.%s"), *MainSoundPath, *WeaponData.offChainMetadata.Base.EquipSoundID, *WeaponData.offChainMetadata.Base.EquipSoundID))));
    // *** Need to set the reload sound on the anim notify and correct path to the Reload sound folder
	ReloadSound = LoadObject<USoundCue>(nullptr, *(FString::Printf(TEXT("%sGunShots/Reload/%s.%s"), *MainSoundPath, *WeaponData.offChainMetadata.Magazine.ReloadSoundID, *WeaponData.offChainMetadata.Magazine.ReloadSoundID)));
	HitSound = LoadObject<USoundCue>(nullptr, *(FString::Printf(TEXT("%sImpacts/%s.%s"), *MainSoundPath, *WeaponData.offChainMetadata.Bullet.ImpactSoundID, *WeaponData.offChainMetadata.Bullet.ImpactSoundID)));
    MagazineCapacity = WeaponData.offChainMetadata.Magazine.Capacity;
    AutoFireRate = WeaponData.offChainMetadata.Base.FireRate;
    FixedAutoFireRate = WeaponData.offChainMetadata.Base.FireRate;
    WeaponDamageAmount = WeaponData.offChainMetadata.Bullet.Damage;
	ClipTransformNFT = WeaponData.offChainMetadata.Magazine.Location;
	//MuzzleFlashNiagara = LoadObject<UNiagaraSystem>(nullptr, *(FString::Printf(TEXT("%sMuzzle/%s.%s"), *MainParticlePath, *WeaponData.offChainMetadata.Muzzle.FlashID, *WeaponData.offChainMetadata.Muzzle.FlashID)));

	// *** Need to add a property to set this in the struct
	if (true)
	{
		CrosshairLeft = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Arrow_Left.Crosshair_Arrow_Left"), *MainImagePath)));
		CrosshairRight = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Arrow_Right.Crosshair_Arrow_Right"), *MainImagePath)));
		CrosshairTop = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Arrow_Top.Crosshair_Arrow_Top"), *MainImagePath)));
		CrosshairBottom = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Arrow_Bottom.Crosshair_Arrow_Bottom"), *MainImagePath)));
	}
	else
	{
		CrosshairLeft = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Quad_9_Left.Crosshair_Quad_9_Left"), *MainImagePath)));
		CrosshairRight = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Quad_9_Right.Crosshair_Quad_9_Right"), *MainImagePath)));
		CrosshairTop = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Quad_9_Top.Crosshair_Quad_9_Top"), *MainImagePath)));
		CrosshairBottom = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Quad_9_Bottom.Crosshair_Quad_9_Bottom"), *MainImagePath)));
	}
	CrosshairMiddle = LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sCrossHairs/Crosshair_Dot_Small.Crosshair_Dot_Small"), *MainImagePath)));

	if (WeaponData.offChainMetadata.Base.Type.Equals(TEXT("Rifle")))
	{
		ReloadMontageSection = FName(TEXT("Reload NFT Rifle"));
		SetIconItem(LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sIcons/WeaponIcons/NFTIconRifle.NFTIconRifle"), *MainImagePath))));
	}
	else if (WeaponData.offChainMetadata.Base.Type.Equals(TEXT("Pistol")))
	{
		ReloadMontageSection = FName(TEXT("Reload NFT Pistol"));
		SetIconItem(LoadObject<UTexture2D>(nullptr, *(FString::Printf(TEXT("%sIcons/WeaponIcons/NFTIconPistol.NFTIconPistol"), *MainImagePath))));
	}
    
    /*
	if (GetMaterialInstance())
	{
	    SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
		GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
		MainWeaponMesh->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
		EnableGlowMaterial();
	} 

	HideBoneOfBelicaWeapons();
	StartPulseTimer();
    */
}


void AWeaponNFT::UpdateWeapon()
{
    // Weapon is updated in the OnRep_WeaponData function
}

void AWeaponNFT::UpdateDropMaterial(float DropValue)
{
	if (DynamicDropMaterialInstance)
	{
		for (int32 i = 0; i < AllMaterialsMain.Num(); i++)
		{
			MainWeaponMesh->SetMaterial(i, DynamicDropMaterialInstance);
			DynamicDropMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DropValue);
		}
		for (int32 i = 0; i < AllMaterialsClip.Num(); i++)
		{
			ClipMesh->SetMaterial(i, DynamicDropMaterialInstance);
			DynamicDropMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DropValue);
		}
	}
}

void AWeaponNFT::StartDropEffect(bool bIsDropped)
{
	if (bIsDropped)
	{
		DropEffectTrack.BindDynamic(this, &AWeaponNFT::UpdateDropMaterial);
		if (DropEffectCurve && DropEffectTimeline)
		{
			DropEffectTimeline->AddInterpFloat(DropEffectCurve, DropEffectTrack);
			DropEffectTimeline->PlayFromStart();
			DropEffectTimeline->SetLooping(true);
		}
		if (DroppedEffectSystem)
		{
			DroppedEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DroppedEffectSystem, GetActorLocation(), GetActorRotation(), FVector(1.f, 1.f, 1.f), false);
		}
	}
	else
	{
		if (DropEffectTimeline && DropEffectTimeline->IsPlaying())
		{
			DropEffectTimeline->Stop();
			for (int32 i = 0; i < AllMaterialsMain.Num(); i++)
			{
				MainWeaponMesh->SetMaterial(i, AllMaterialsMain[i]);
			}
			for (int32 i = 0; i < AllMaterialsClip.Num(); i++)
			{
				ClipMesh->SetMaterial(i, AllMaterialsClip[i]);
			}
		}
		if (DroppedEffectComponent && DroppedEffectComponent->IsActive()) DroppedEffectComponent->Deactivate();
	}
}

void AWeaponNFT::MulticastSpawnEffects_Implementation()
{	
	if (SpawnSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, GetActorLocation());
	if (SpawnEffect) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpawnEffect, GetActorLocation());
}

void AWeaponNFT::ThrowWeapon(bool bUnequipped, FTransform CharacterTransform, float ThrowImpulse, bool bRandomDirection)
{
	FQuat CharacterRot = CharacterTransform.GetRotation();
	if (AutoFireRate != FixedAutoFireRate) AutoFireRate = FixedAutoFireRate;
	float ThrowDirection = bRandomDirection ? 180.f : 60.f;
    FRotator MeshRotation{ 0.f, MainWeaponMesh->GetComponentRotation().Yaw, 0.f };
	FVector TargetLocation;

    // Set the location of the item to the character's location when character is not equipped with a weapon
    if (bRandomDirection)
    {
		float RandomRotation = FMath::FRandRange(-ThrowDirection, ThrowDirection);
        MainWeaponMesh->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		MeshRotation = FRotator{ 0.f, CharacterRot.Rotator().Yaw - 90 + RandomRotation, 0.f };
		MainWeaponMesh->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		// Direction in which we throw the weapon
		TargetLocation = GetActorLocation() + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 200.f);
    }
	else
	{
		if (bUnequipped) MainWeaponMesh->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		MainWeaponMesh->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
		// Direction in which we throw the weapon
		//FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);
		ThrowImpulse = ThrowImpulse / 2.f;
		TargetLocation = GetActorLocation() + 100.f * CharacterRot.GetForwardVector() + FMath::FRandRange(-50.f, 50.f) * CharacterRot.GetRightVector() + FVector(0.f, 0.f, 50.f);

	}
	FVector ImpulseDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();

	//ImpulseDirection *= ThrowImpulse;
	MainWeaponMesh->AddImpulse(ThrowImpulse * ImpulseDirection);
	
	if (HasAuthority())
	{
		bFalling = true;
		bInAir = true;
	}
}

void AWeaponNFT::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AWeaponNFT::FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter)
{
	//APawn* InstigatorPawn = Cast<APawn>(AttackerCharacter);
	UWorld* World = GetWorld();
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const UStaticMeshSocket* MuzzleFlashSocket = MainWeaponMesh->GetSocketByName("BarrelSocket");

	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform;
        MuzzleFlashSocket->GetSocketTransform(SocketTransform, MainWeaponMesh);
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
		}
		// From muzzle flash socket to hit location from TraceUnderCrosshairs
		FRotator TargetRotation = (WeaponTraceHit.ImpactPoint - SocketTransform.GetLocation()).Rotation();
		if (InstigatorPawn && ProjectileClass)
		{
			FActorSpawnParameters SpawnParams;
			//SpawnParams.Owner = AttackerCharacter;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			//SetInstigator(InstigatorPawn);
			
			if (InstigatorPawn->HasAuthority())
			{
				AProjectileBullet* Projectile = World->SpawnActor<AProjectileBullet>(
					ProjectileClass,
					WeaponTraceHit.TraceStart,
					TargetRotation,
					SpawnParams
					);

				if (Projectile)
				{
					Projectile->MulticastSetBulletProperties(WeaponData, AmmoType);
				}
				/*
                FTransform ProjectileTransform;
                ProjectileTransform.SetLocation(WeaponTraceHit.TraceStart);
                ProjectileTransform.SetRotation(TargetRotation.Quaternion());
				AProjectileBullet* Projectile = World->SpawnActorDeferred<AProjectileBullet>(
					ProjectileClass,
					ProjectileTransform,
					GetOwner(),
					InstigatorPawn,
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
					);

                if (Projectile) Projectile->SetBulletProperties(WeaponData);
                UGameplayStatics::FinishSpawningActor(Projectile, ProjectileTransform);
				*/
			}
		}
	}
}

void AWeaponNFT::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
	float ThrowPower = 10000.f;
	if (InShooter)
	{
		ResetSequence(InShooter);
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		SetItemState(EItemState::EIS_Falling);
		ThrowWeapon(InShooter->GetUnEquippedState(), InShooter->GetCapsuleComponent()->GetComponentTransform(), ThrowPower, bPlayerElimed);
	}
	else
	{
		SetItemState(EItemState::EIS_Falling);
		ThrowWeapon(false, CharacterTransform, ThrowPower, true);
	}
	EnableGlowMaterial();
	StartPulseTimer();
}

void AWeaponNFT::AttachClip(AShooterCharacter* ShooterChar, bool bAttach)
{
	if (ShooterChar == nullptr) return;
	if (bAttach)
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
		ClipMesh->AttachToComponent(ShooterChar->GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
	}
	else
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		ClipMesh->DetachFromComponent(DetachmentTransformRules);
		ClipMesh->SetRelativeTransform(ClipTransformNFT);
	}
}

void AWeaponNFT::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, WeaponData);
}



