// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "Projectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "ShooterCrosshairHUD.h"
#include "Components/CapsuleComponent.h"
#include "ShooterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/Drone/ItemContainer.h"
#include "ShooterPlayerState.h"
#include "ShooterCharacterAI.h"
#include "Helicopter.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "ScopeAttachment.h"

//UE_DEFINE_GAMEPLAY_TAG(TAG_WeaponTypePistol, "Weapon.Type.Pistol")
//UE_DEFINE_GAMEPLAY_TAG(TAG_WeaponTypeSubmachineGun, "Weapon.Type.SubmachineGun")

AWeapon::AWeapon()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
	SetReplicatingMovement(true);
}

EWeaponType AWeapon::GetWeaponType_Implementation()
{
	return WeaponType;
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Keep the weapon upright
    if (GetItemState() == EItemState::EIS_Falling && bFalling)
    {
		if (!bIsAnNFT && GetItemMesh())
		{
        	const FRotator MeshRotation{0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f};
        	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
		else if (bIsAnNFT && GetNFTMesh())
		{
        	const FRotator MeshRotation{0.f, GetNFTMesh()->GetComponentRotation().Yaw, 0.f};
        	GetNFTMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
		}
    }

    // Update slide on pistol
    UpdateSlideDisplacement();
    // Update slide on shotgun
    UpdateShotgunSlideDisplacement();
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();

	if (HasAuthority())
	{
		if (!bIsAnNFT && GetItemMesh())
		{
			GetItemMesh()->OnComponentHit.AddDynamic(this, &AWeapon::OnHit);
		}
	}

	HideBoneOfBelicaWeapons();
}

void AWeapon::HideBoneOfBelicaWeapons()
{
	switch (WeaponType)
	{
		case EWeaponType::EWT_SubmachineGun:
		GetItemMesh()->HideBoneByName(FName("pistol"), EPhysBodyOp::PBO_None);
		break;

		case EWeaponType::EWT_Pistol:
		GetItemMesh()->HideBoneByName(FName("weapon"), EPhysBodyOp::PBO_None);
		break;
	}
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

	UpdateWeapon();
}

void AWeapon::UpdateWeapon()
{
	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	const FString WeaponTablePath(TEXT("DataTable'/Game/_Game/DataTables/WeaponDataTable.WeaponDataTable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));
	if (WeaponTableObject)
	{
		FWeaponDataTable* WeaponRow = nullptr;
		switch (WeaponType)
		{
			case EWeaponType::EWT_SubmachineGun:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;

			case EWeaponType::EWT_AR:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssultRifle"), TEXT(""));
			break;

			case EWeaponType::EWT_Pistol:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("pistol"), TEXT(""));
			break;

			case EWeaponType::EWT_Shotgun:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Shotgun"), TEXT(""));
			break;

			case EWeaponType::EWT_Sniper:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Sniper"), TEXT(""));
			break;

			case EWeaponType::EWT_GrenadeLauncher:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("GrenadeLauncher"), TEXT(""));
			break;

			case EWeaponType::EWT_CyberPistol:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("CyberPistol"), TEXT(""));
			break;

			case EWeaponType::EWT_GravCannon:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("GravCannon"), TEXT(""));
			break;

			case EWeaponType::EWT_Test:
			WeaponRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Test"), TEXT(""));
			break;
		}
		if (WeaponRow)
		{
			ProjectileClass = WeaponRow->ProjectileClass;
			AmmoType = WeaponRow->AmmoType;
			if (HasAuthority())
			{
				Ammo = WeaponRow->WeaponAmmo;
				ReplCachedAmmo = Ammo;
			}
            
			SetItemType(EItemType::EIT_Weapon);
            MagazineCapacity = WeaponRow->MagazineCapacity;
            SetPickupSound(WeaponRow->PickupSound);
            SetEquipSound(WeaponRow->EquipSound);
            GetItemMesh()->SetSkeletalMesh(WeaponRow->ItemMesh);
            SetItemName(WeaponRow->ItemName);
            SetIconItem(WeaponRow->InventoryIcon);
            SetIconAmmo(WeaponRow->AmmoIcon);
            SetMaterialInstance(WeaponRow->MaterialInstance);
            PreviousMaterialIndex = GetMaterialIndex();
            GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr); // clear the previous material inst before setting new material
            SetMaterialIndex(WeaponRow->MaterialIndex);
            ClipBoneName = WeaponRow->ClipBoneName;
            ReloadMontageSection = WeaponRow->ReloadMontageSection;
            GetItemMesh()->SetAnimInstanceClass(WeaponRow->AnimBP);
            CrosshairLeft = WeaponRow->CrosshairLeft;
            CrosshairRight = WeaponRow->CrosshairRight;
            CrosshairTop = WeaponRow->CrosshairTop;
            CrosshairBottom = WeaponRow->CrosshairBottom;
            CrosshairMiddle = WeaponRow->CrosshairMiddle;
            AutoFireRate = WeaponRow->AutoFireRate;
			FixedAutoFireRate = WeaponRow->AutoFireRate; // used for setting the fixed fire rate to prevent cheating. Look at ServerFire_Validate
            MuzzleFlash = WeaponRow->MuzzleFlash;
            FireSound = WeaponRow->FireSound;
			HitSound = WeaponRow->HitSound;
			CharacterHitSound = WeaponRow->CharacterHitSound;
            BoneToHide = WeaponRow->BoneToHide; // Not being used
            bAutomatic = WeaponRow->bAutomatic;
            WeaponDamageAmount = WeaponRow->WeaponDamageAmount;
            FireAnimation = WeaponRow->FireAnimation;
			ImpactParticles = WeaponRow->ImpactParticles;
			ADSSettings = WeaponRow->ADSSettings;

			if (WeaponRow->ScopeClass && !ScopeAttachment && HasAuthority() && GetWorld())
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ScopeAttachment = GetWorld()->SpawnActor<AScopeAttachment>(WeaponRow->ScopeClass, GetActorTransform(), SpawnParams);
				if (ScopeAttachment)
				{
					ScopeAttachment->AttachToComponent(GetItemMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("Scope_Socket"));

					FName ChildAttachSocket = TEXT("AttachSocket");
					if (ScopeAttachment->ScopeMesh->DoesSocketExist(ChildAttachSocket))
					{
						// Get child's socket transform relative to its root
						FTransform ChildSocketRelative = ScopeAttachment->ScopeMesh->GetSocketTransform(ChildAttachSocket, RTS_Component);

						// Inverse to offset the root so socket aligns with parent's socket
						FTransform InverseOffset = ChildSocketRelative.Inverse();

						// Apply as relative transform (post-attachment)
						ScopeAttachment->SetActorRelativeTransform(InverseOffset);
					}
				}
			}
		}
	}  
	if (GetMaterialInstance())
	{
	    SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));
		GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
		GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
		EnableGlowMaterial();
	} 

	HideBoneOfBelicaWeapons();
	StartPulseTimer();
}

void AWeapon::OnRep_WeaponType()
{
	UpdateWeapon();
}

void AWeapon::OnRep_ItemState()
{
	Super::OnRep_ItemState();
	
	auto CurrentState = GetItemState();
	if (CurrentState == EItemState::EIS_Pickup)
	{
		Ammo = ReplCachedAmmo;
	}
}

void AWeapon::ThrowWeapon(bool bUnequipped, FTransform CharacterTransform, float ThrowImpulse, bool bRandomDirection)
{
	FQuat CharacterRot = CharacterTransform.GetRotation();
	if (AutoFireRate != FixedAutoFireRate) AutoFireRate = FixedAutoFireRate;
	float ThrowDirection = bRandomDirection ? 180.f : 60.f;
    FRotator MeshRotation{ 0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f };
	FVector TargetLocation;

    // Set the location of the item to the character's location when character is not equipped with a weapon
    if (bRandomDirection)
    {
		float RandomRotation = FMath::FRandRange(-ThrowDirection, ThrowDirection);
        GetItemMesh()->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		MeshRotation = FRotator{ 0.f, CharacterRot.Rotator().Yaw - 90 + RandomRotation, 0.f };
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		// Direction in which we throw the weapon
		TargetLocation = GetActorLocation() + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 200.f);
    }
	else
	{
		if (bUnequipped) GetItemMesh()->SetWorldLocation(CharacterTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
		// Direction in which we throw the weapon
		//FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);
		ThrowImpulse = ThrowImpulse / 2.f;
		TargetLocation = GetActorLocation() + 100.f * CharacterRot.GetForwardVector() + FMath::FRandRange(-50.f, 50.f) * CharacterRot.GetRightVector() + FVector(0.f, 0.f, 50.f);
		
	}
	FVector ImpulseDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();

	//ImpulseDirection *= ThrowImpulse;
	GetItemMesh()->AddImpulse(ThrowImpulse * ImpulseDirection);
	
	if (HasAuthority())
	{
		bFalling = true;
		bInAir = true;
	}
    
    EnableGlowMaterial();
}

void AWeapon::StopFalling()
{
	Super::StopFalling();

	SetSlotIndex(0);
	// Ensure that client has the server's value for Ammo
	if(HasAuthority())
		ClientUpdateAmmo(Ammo, Sequence);
}

void AWeapon::OnEquipped()
{
	Super::OnEquipped();

	//Sequence = 0;
}

void AWeapon::SetHUDAmmo(int32 InAmmo)
{
	auto ShooterOwnerCharacter = Cast<AShooterCharacter>(GetOwner());
	if (ShooterOwnerCharacter && ShooterOwnerCharacter->IsLocallyControlled())
	{
		ShooterOwnerCharacter->GetHudAmmoDelegate().Broadcast(InAmmo);
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagazineCapacity);
	ReplCachedAmmo = Ammo;
	SetHUDAmmo(Ammo);

	if (HasAuthority())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Server Sequence = %i, Ammo = %i"), Sequence, Ammo);
		ClientUpdateAmmo(Ammo, Sequence); // Pass the weapon's unique sequence count
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Client Sequence = %i, Ammo = %i"), Sequence, Ammo);
		++Sequence; // Increment the unique sequence count for this weapon
	}
}

void AWeapon::ClientUpdateAmmo_Implementation(int32 ServerAmmo, int32 ServerWeaponSequence)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	/*
	if (ServerWeaponSequence > Sequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientUpdateAmmo"));
		// We only update the client's ammo if the server's sequence is greater than the client's sequence
		int32 AmmoDifference = ServerAmmo - Ammo;
		Ammo = ServerAmmo;
		Sequence = ServerWeaponSequence;

		// Adjust the Ammo by the difference between the server's Ammo and the client's previous Ammo
		// This will handle the case where the server's Ammo increased due to lag and we need to
		// apply the same increase to the client's Ammo.
		Ammo -= AmmoDifference;
	}
	*/

	// Update the cached Ammo on the client and the HUD
	ReplCachedAmmo = Ammo;
	SetHUDAmmo(Ammo);
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	// Prediction here not implemented correctly with high lag. Should see how to fix this in the future.
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagazineCapacity);
	ReplCachedAmmo = Ammo;
	SetHUDAmmo(Ammo);
	ClientAddAmmo(Ammo);
}

void AWeapon::ClientAddAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	Ammo = ServerAmmo;
	//Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagazineCapacity);
	ReplCachedAmmo = Ammo;
	
	auto ShooterOwnerCharacter = Cast<AShooterCharacter>(GetOwner());
	if (ShooterOwnerCharacter && Ammo == MagazineCapacity)
	{
		ShooterOwnerCharacter->JumpToShotgunEnd();
		ShooterOwnerCharacter->GetHudAmmoDelegate().Broadcast(Ammo);
	}
}

void AWeapon::StartSlideTimer()
{
    bMovingSlide = true;
    GetWorldTimerManager().SetTimer(SlideTimer, this, &AWeapon::FinishMovingSlide, SlideDisplacementTime);
}
void AWeapon::FinishMovingSlide()
{
    bMovingSlide = false;
}

void AWeapon::StartShotgunSlideTimer()
{
    bMovingShotgunSlide = true;
    GetWorldTimerManager().SetTimer(ShotgunSlideTimer, this, &AWeapon::FinishMovingShotgunSlide, ShotgunSlideDisplacementTime);
    bUseFABRIK = false;
}
void AWeapon::FinishMovingShotgunSlide()
{
    bMovingShotgunSlide = false;
    bUseFABRIK = true;
}

void AWeapon::UpdateSlideDisplacement()
{
	
	if (SlideDisplacementCurve && bMovingSlide)
	{
        const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(SlideTimer) };
		const float CurveValue{ SlideDisplacementCurve->GetFloatValue(ElapsedTime) };
        SlideDisplacement = CurveValue * MaxSlideDisplacement;
        PistolRotation = CurveValue * MaxPistolRotation;
	}
}

void AWeapon::UpdateShotgunSlideDisplacement()
{
	
	if (ShotgunSlideDisplacementCurve && bMovingShotgunSlide)
	{
        const float ElapsedTime{ GetWorldTimerManager().GetTimerElapsed(ShotgunSlideTimer) };
		const float CurveValue{ ShotgunSlideDisplacementCurve->GetFloatValue(ElapsedTime) };
        ShotgunSlideDisplacement = CurveValue * MaxShotgunSlideDisplacement;
	}
}

void AWeapon::ShotgunTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit, AShooterCharacter* AttackerCharacter)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		FCollisionQueryParams CollisionParam;
		if (AttackerCharacter)
		{
			CollisionParam.AddIgnoredActor(AttackerCharacter);
			if (AttackerCharacter->Inventory[0]) CollisionParam.AddIgnoredActor(AttackerCharacter->Inventory[0]);
		}
		
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility,
			CollisionParam
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
			//BeamEnd = OutHit.Location;
			//OutHit.ImpactPoint = BeamEnd;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}

		//ShotgunTraceHit_Effect(TraceStart, BeamEnd, World);
	}
}

void AWeapon::ShotgunTraceHit_Effect(FVector InTraceStart, FVector BeamVal, UWorld* World)
{
	if (World && BeamParticles)
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
			World,
			BeamParticles,
			InTraceStart,
			FRotator::ZeroRotator,
			true
		);

		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"), BeamVal);
		}
	}

	if (FireAnimation)
	{
		GetItemMesh()->PlayAnimation(FireAnimation, false);
	}
}

void AWeapon::ScreenMessage(FString Mess)
{
	if (GEngine)
	{
		if (GetWorld()->GetNetMode() == NM_ListenServer)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Mess);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, Mess);
		}
	}
}


void AWeapon::FireShotgun(AShooterCharacter* AttackerCharacter, const TArray<FHitResult>& WeaponTraceHits)
{
	// Send bullet
	if (AttackerCharacter == nullptr) return;
	AShooterPlayerState* ShooterPSLoc = AttackerCharacter->GetShooter_PS();
	const USkeletalMeshSocket* BarrelSocket = GetItemMesh()->GetSocketByName("BarrelSocket");
	UWorld* World = GetWorld();
	if (World && BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetItemMesh());
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
		}

		TArray<AActor*> ActorsToConsider;

		/*
		TMap<AShooterCharacter*, float> HitMap;
		TMap<AItemContainer*, float> HitMapContainer;
		TMap<AShooterCharacterAI*, float> HitMapAIChar;
		TMap<AHelicopter*, float> HitMapHelicopter;
		float Damage;
		*/

		for (auto WeaponTraceHit : WeaponTraceHits)
		{
			if (ShooterPSLoc) ShooterPSLoc->PlayerGameStats.ShotsFired++;
			if (WeaponTraceHit.bBlockingHit && WeaponTraceHit.GetActor())
			{
				if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(WeaponTraceHit.GetActor()))
				{
					if (!ActorsToConsider.Contains(WeaponTraceHit.GetActor()))
					{
						ActorsToConsider.AddUnique(WeaponTraceHit.GetActor());
						if (WeaponHitInterface)
						{
							WeaponHitInterface->Execute_OnShotgunWeaponHit(
								WeaponTraceHit.GetActor(), 
								AttackerCharacter, 
								AttackerCharacter->GetShooter_PS(), 
								WeaponTraceHits, 
								CharacterHitSound
								);
						}
					}
				}
				else
				{
					if (HitSound)
					{
						UGameplayStatics::PlaySoundAtLocation(
							this,
							HitSound,
							WeaponTraceHit.ImpactPoint,
							0.4f
						);
					}
				}
			}

			/*
			AShooterCharacter* HitCharacter = Cast<AShooterCharacter>(WeaponTraceHit.GetActor());
			AItemContainer* HitContainer = Cast<AItemContainer>(WeaponTraceHit.GetActor());
			AShooterCharacterAI* HitAIChar = Cast<AShooterCharacterAI>(WeaponTraceHit.GetActor());
			AHelicopter* HitHelicopter = Cast<AHelicopter>(WeaponTraceHit.GetActor());

			if (HitAIChar)
			{
				
				Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
				if (HitMapAIChar.Contains(HitAIChar)) HitMapAIChar[HitAIChar] += Damage;
				else HitMapAIChar.Emplace(HitAIChar, Damage);

				if (CharacterHitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						CharacterHitSound,
						WeaponTraceHit.ImpactPoint,
						0.4f
					);
				}
				
			}
			else if (HitCharacter)
			{
				
				Damage = SetShotgunDamageAmount(AttackerCharacter, HitCharacter, WeaponTraceHit);
				if (Damage == -1.f) continue;
				//Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
				if (HitMap.Contains(HitCharacter)) HitMap[HitCharacter] += Damage;
				else HitMap.Emplace(HitCharacter, Damage);

				if (CharacterHitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						CharacterHitSound,
						WeaponTraceHit.ImpactPoint,
						0.4f
					);
				}
				
			}
			else if (HitHelicopter)
			{
				Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
				if (HitMapHelicopter.Contains(HitHelicopter)) HitMapHelicopter[HitHelicopter] += Damage;
				else HitMapHelicopter.Emplace(HitHelicopter, Damage);

				if (CharacterHitSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						CharacterHitSound,
						WeaponTraceHit.ImpactPoint,
						0.4f
					);
				}
				if (HasAuthority())
				{
					HitHelicopter->HelicopterDamageEffects(WeaponTraceHit.ImpactPoint);
					//HitHelicopter->HelicopterHitPoint = FVector_NetQuantize(0.f, 0.f, 0.f);
				}
			}
			else if (HitContainer)
			{
				Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
				//Damage = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
				if (HitMapContainer.Contains(HitContainer)) HitMapContainer[HitContainer] += Damage;
				else HitMapContainer.Emplace(HitContainer, Damage);
			}
			else
			{
				AShooterPlayerState* ShooterPSLoc = AttackerCharacter->GetShooter_PS();
				if (HasAuthority())
				{
					if (ShooterPSLoc) ShooterPSLoc->PlayerGameStats.FireCharacterMiss++;
				}
			}
			*/

			ShotgunTraceHit_Effect(WeaponTraceHit.TraceStart, WeaponTraceHit.ImpactPoint, World);

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					World,
					ImpactParticles,
					WeaponTraceHit.ImpactPoint,
					WeaponTraceHit.ImpactNormal.Rotation()
				);
			}
		}
		//ShotGunDamage(HitMap, HitMapAIChar, HitMapHelicopter, HitMapContainer, AttackerCharacter);
	}
}

float AWeapon::SetShotgunDamageAmount(AShooterCharacter* AttackerCharacter, AShooterCharacter* DamagedCharacter, const FHitResult& WeaponTraceHit)
{
	if (DamagedCharacter && AttackerCharacter)
	{
		float Wd = SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
		float ScaledWeaponDamage = (-3.f * Wd / (4.f * DamagedCharacter->GetMaxHandShieldStrength())) * DamagedCharacter->GetHandShieldStrength() + Wd; // Damage to set based on shield strength
		float DamageToShow = 0.f;
		bool bHeadShot = WeaponTraceHit.BoneName.ToString() == TEXT("head") ? true : false;
		if (WeaponTraceHit.GetComponent()->ComponentHasTag(FName("ShieldTag")))
		{
			if (HasAuthority())
			{
				DamagedCharacter->HandShieldStrength = (FMath::Clamp(DamagedCharacter->HandShieldStrength - FMath::RoundToInt32(SetWeaponDamage(WeaponTraceHit.BoneName.ToString()) / 3.f), 0.f, DamagedCharacter->GetMaxHandShieldStrength()));
				DamagedCharacter->OnRep_HandShieldStrength();
			}

			// Dot product between attacker forward direction and victim forward direction
			// If player is shooting another player's shield from behind, doesn't count as damage
			float PlayerDot = UKismetMathLibrary::Dot_VectorVector(AttackerCharacter->GetCapsuleComponent()->GetForwardVector(), DamagedCharacter->GetCapsuleComponent()->GetForwardVector());
			if (PlayerDot > 0.f)
			{
				// condition should be < 0, but for some reason, the forward angles seem to be negated in Unreal.
				return -1.f;
			}
			DamageToShow = DamagedCharacter->GetBoostProtect() == false ?
						FMath::Floor(ScaledWeaponDamage) : 0.f;
		}
		else
		{
			DamageToShow = DamagedCharacter->GetBoostProtect() == false ? SetWeaponDamage(WeaponTraceHit.BoneName.ToString()) : 0.f;
		}

		return DamageToShow;
	}
	else
	{
		return SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
	}
}

void AWeapon::ShotgunTraceEndWithScatter(const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets)
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetItemMesh()->GetSocketByName("BarrelSocket");
	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetItemMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;

	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		float TRACE_LENGTH = 80000.f;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();

		HitTargets.Add(ToEndLoc);
		//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
		//DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
		//DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart+ToEndLoc*TRACE_LENGTH/ToEndLoc.Size()), FColor::Cyan, true);

	}
}

void AWeapon::ShotGunDamage(TMap<AShooterCharacter*, float> HitMap, TMap<AShooterCharacterAI*, float> HitMapAICharDummy, TMap<AHelicopter*, float> HitMapHelicopter, TMap<AItemContainer*, float> HitMapContainer, AShooterCharacter* AttackerCharacter)
{
	AController* InstigatorController = AttackerCharacter->GetController();
	TArray<AShooterCharacter*> HitCharacters;
	// Calculate body shot damage by multiplying times hit x Damage - store in DamageMap

	AShooterPlayerState* ShooterPSLoc = AttackerCharacter->GetShooter_PS();
	
	for (auto HitPair : HitMap)
	{
		if (HitPair.Key && InstigatorController && AttackerCharacter && HitPair.Key != AttackerCharacter)
		{
			if (HasAuthority())
			{
				//ScreenMessage(TEXT("Hit Character!!"));
				if (ShooterPSLoc)
				{
					ShooterPSLoc->DamageType = EShooterDamageType::ESDT_Gun;
				}
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
				//UE_LOG(LogTemp, Warning, TEXT("Health: %f"), HitPair.Key->GetHealth());

			}
			auto VictimPS = Cast<AShooterPlayerState>(HitPair.Key->GetPlayerState());
			if (AttackerCharacter->IsLocallyControlled() && VictimPS && VictimPS->GetGenericTeamId() != ShooterPSLoc->GetGenericTeamId())
			{
				AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
			}
			else if (AttackerCharacter->IsLocallyControlled() && HitPair.Key->bIsAI)
			{
				AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
			}

			//HitCharacters.AddUnique(HitPair.Key);

		}
	}
	

	for (auto HitPair : HitMapHelicopter)
	{
		if (HitPair.Key && InstigatorController && AttackerCharacter)
		{
			if (HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorController,
					AttackerCharacter,
					UDamageType::StaticClass()
				);
			}
			if (HitPair.Key->OwnedShooter)
			{
				auto VictimPS = Cast<AShooterPlayerState>(HitPair.Key->GetPlayerState());
				if (AttackerCharacter->IsLocallyControlled() && VictimPS && VictimPS->GetGenericTeamId() != ShooterPSLoc->GetGenericTeamId())
				{
					AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
				}
			}
			else
			{
				if (AttackerCharacter->IsLocallyControlled())
				{
					AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
				}
			}
		}
	}

	
	for (auto HitPair : HitMapContainer)
	{
		
		if (HitPair.Key && InstigatorController && AttackerCharacter)
		{
			if (HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorController,
					AttackerCharacter,
					UDamageType::StaticClass()
				);
			}
			if (AttackerCharacter->IsLocallyControlled())
			{
				AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
			}
		}
	}
	

	
	for (auto HitPair : HitMapAICharDummy)
	{
		if (HitPair.Key && InstigatorController && AttackerCharacter)
		{
			if (HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key, // Character that was hit
					HitPair.Value, // Damage calculated in the two for loops above
					InstigatorController,
					AttackerCharacter,
					UDamageType::StaticClass()
				);
			}
			if (AttackerCharacter->IsLocallyControlled())
			{
				AttackerCharacter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
			}
		}
	}
	
	/*
	if (!HasAuthority() && bUseServerSideRewind)
	{
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(OwnerPawn) : BlasterOwnerCharacter;
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(InstigatorController) : BlasterOwnerController;
		if (BlasterOwnerController && BlasterOwnerCharacter && BlasterOwnerCharacter->GetLagCompensation() && BlasterOwnerCharacter->IsLocallyControlled())
		{
			BlasterOwnerCharacter->GetLagCompensation()->ShotgunServerScoreRequest(
				HitCharacters,
				Start,
				HitTargets,
				BlasterOwnerController->GetServerTime() - BlasterOwnerController->SingleTripTime
			);
		}
	}
	*/
}

void AWeapon::FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter)
{
	//APawn* InstigatorPawn = Cast<APawn>(AttackerCharacter);
	UWorld* World = GetWorld();
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetItemMesh()->GetSocketByName("BarrelSocket");

	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetItemMesh());
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
				AProjectile* Projectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					WeaponTraceHit.TraceStart,
					TargetRotation,
					SpawnParams
					);
			}
		}
	}
}

float AWeapon::SetWeaponDamage(const FString& HitBone)
{
    const float HeadPercentage = 0.5;
    float OutDamage = 0;
    // Set a weapon damage percentage amount in the item data table (AItem class). Call it RarityDamagePercentage
    switch (GetItemRarity())
    {
        case EItemRarity::EIR_Damaged:
            OutDamage = HitBone == TEXT("head") ? WeaponDamageAmount * (GetRarityDamagePercentage() + HeadPercentage) : WeaponDamageAmount * GetRarityDamagePercentage();
            break;
        case EItemRarity::EIR_Common:
            OutDamage = HitBone == TEXT("head") ? WeaponDamageAmount * (GetRarityDamagePercentage() + HeadPercentage) : WeaponDamageAmount * GetRarityDamagePercentage();
            break;
        case EItemRarity::EIR_Uncommon:
            OutDamage = HitBone == TEXT("head") ? WeaponDamageAmount * (GetRarityDamagePercentage() + HeadPercentage) : WeaponDamageAmount * GetRarityDamagePercentage();
            break;
        case EItemRarity::EIR_Rare:
            OutDamage = HitBone == TEXT("head") ? WeaponDamageAmount * (GetRarityDamagePercentage() + HeadPercentage) : WeaponDamageAmount * GetRarityDamagePercentage();
            break;
		case EItemRarity::EIR_NFT:
        case EItemRarity::EIR_Legendary:
            OutDamage = HitBone == TEXT("head") ? WeaponDamageAmount * (GetRarityDamagePercentage() + HeadPercentage) : WeaponDamageAmount * GetRarityDamagePercentage();
            break;
    }
    return FMath::RoundToInt32(OutDamage);
}

void AWeapon::OnItemPickedUp(AShooterCharacter* InShooter)
{
	if (InShooter == nullptr) return;
	if (InShooter->Inventory.Num() >= InShooter->GetInventoryMaxCapacity()) // Inventory is full, swap weapon
	{
		InShooter->SwapItem(this, nullptr);
	}
	else
	{
		SetSlotIndex(InShooter->Inventory.Num());
		InShooter->Inventory.Add(this);
		SetItemState(EItemState::EIS_PickedUp);
	}
}

void AWeapon::OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance)
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
			InShooter->HudAmmoDelegate.Broadcast(ReplCachedAmmo);
			InShooter->GetHudCarriedAmmoDelegate().Broadcast();

			float FOVMult = GetEffectiveADSMultiplier(InShooter->GetCameraDefaultFOV());
			InShooter->SetMouseSensitivities(FOVMult);
		}
		ResetSequence(InShooter);
	}
	// *** For NFT weapon, may need to override this and place attach the weapon differently
    const USkeletalMeshSocket* HandSocket = InShooter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
    if (HandSocket) HandSocket->AttachActor(this, InShooter->GetMesh());
}

void AWeapon::OnNonZeroSequence(int32 ClientSequence)
{
	UE_LOG(LogTemp, Warning, TEXT("SERVER On Switch Ammo = %i"), Ammo);
	Ammo -= ClientSequence;
	ReplCachedAmmo = Ammo;
}

void AWeapon::ResetSequence(AShooterCharacter* InShooter)
{
	// Set the correct sequence and ammo value when sequence value is positive
	// This correction is usually done when there is high lag, and firing a weapon while switching weapons
	if (!HasAuthority() && Sequence > 0)
	{
		InShooter->Server_OnNonZeroSequence(this, Sequence);
		Sequence = 0;
	} 
}

void AWeapon::OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed)
{
	float ThrowPower = 10000.f;
	if (HasAuthority()) SetItemState(EItemState::EIS_Falling);

	if (InShooter)
	{
		ResetSequence(InShooter);
		//FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		//GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		//SetItemState(EItemState::EIS_Falling);
		if (HasAuthority())
		{
			FTransform StartTransform = InShooter->GetCapsuleComponent()->GetComponentTransform();
			StartTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			ThrowWeapon(InShooter->GetUnEquippedState(), StartTransform, ThrowPower, bPlayerElimed);
		}
	}
	else
	{
		if (HasAuthority())
		{
			CharacterTransform.AddToTranslation(FVector(0.f, 0.f, 40.f));
			//SetItemState(EItemState::EIS_Falling);
			ThrowWeapon(false, CharacterTransform, ThrowPower, true);
		}
	}
	EnableGlowMaterial();
	StartPulseTimer();
}

bool AWeapon::IsAnItem()
{
	return true;
}

void AWeapon::AttachClip(AShooterCharacter* ShooterChar, bool bAttach)
{
	// this function is used in the AWeaponNFT class
}

AShooterCharacter* AWeapon::GetShooterCharacter_Implementation()
{
    return Cast<AShooterCharacter>(GetOwner());
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (ScopeAttachment)
	{
		ScopeAttachment->SetScopeOwner(Cast<APawn>(GetOwner()));
	}
}

void AWeapon::ActivateScope(bool bActivate)
{
	if (ScopeAttachment)
	{
		ScopeAttachment->ActivateScope(bActivate);
	}
}

FTransform AWeapon::GetScopeLensTransform()
{
	if (ScopeAttachment)
	{
		return ScopeAttachment->ScopeMesh->GetSocketTransform(ScopeAttachment->LensSocketName, RTS_World);
	}
	return FTransform();
}

float AWeapon::GetEffectiveADSMultiplier(float InHipFOV) const
{
    float HipRadHalf = FMath::DegreesToRadians(InHipFOV * 0.5f);
    float AdsRadHalf = FMath::DegreesToRadians(ADSSettings.ZoomedFOV * 0.5f);
    return FMath::Tan(AdsRadHalf) / FMath::Tan(HipRadHalf);  // <1.0 for zooms
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, WeaponType);
	DOREPLIFETIME(ThisClass, ReplCachedAmmo);
	DOREPLIFETIME(ThisClass, ScopeAttachment);
}
