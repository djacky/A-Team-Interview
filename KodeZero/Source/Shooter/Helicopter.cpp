// Fill out your copyright notice in the Description page of Project Settings.


#include "Helicopter.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "ShooterCharacter.h"
#include "Math/RandomStream.h"
#include "Shooter/Items/Weapons/HelicopterWeapon.h"
#include "Shooter/EnumTypes/HelicopterWeaponType.h"
#include "Shooter/GameMode/ShooterGameMode.h"
#include "ShooterGameState.h"
#include "Shooter/ShooterGameInstance.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "Shooter/ShooterPlayerState.h"
#include "Shooter/AI/ShooterAI.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Shooter/Misc/CharacterGlobalFuncs.h"

AHelicopter::AHelicopter()
{
    bReplicates = true;
	//SetReplicatingMovement(true);
	//NetDormancy = ENetDormancy::DORM_DormantAll;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	PrimaryActorTick.bCanEverTick = true;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	SetRootComponent(RootMesh);

	HeliSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HelicopterSceneComp"));
	HeliSceneComponent->SetupAttachment(RootComponent);

	FrontGuns = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontGuns"));
	FrontGuns->SetupAttachment(HeliSceneComponent);

	HeliSceneWeapon1 = CreateDefaultSubobject<USceneComponent>(TEXT("HelicopterWeapon1"));
	HeliSceneWeapon1->SetupAttachment(FrontGuns);
	HeliSceneWeapon1->SetRelativeLocation(FVector(52.f, 900.f, 56.7f));
	HeliSceneWeapon1->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));

	HeliSceneWeapon2 = CreateDefaultSubobject<USceneComponent>(TEXT("HelicopterWeapon2"));
	HeliSceneWeapon2->SetupAttachment(FrontGuns);
	HeliSceneWeapon2->SetRelativeRotation(FRotator(-52.f, 900.f, 56.7f));
	HeliSceneWeapon2->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));

	HeliDoorBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorCollisionBox"));
	HeliDoorBoxComponent->SetupAttachment(HeliSceneComponent);
	HeliDoorBoxComponent->ComponentTags.Add(FName("HelicopterDoor"));
	HeliDoorBoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	HeliDoorBoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	HeliDoorBoxComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	HeliDoorBoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeliDoorBoxComponent->SetRelativeLocation(FVector(-276.f, -190.f, 144.f));

	OpenDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OpenDoorMesh"));
	OpenDoorMesh->SetupAttachment(HeliDoorBoxComponent);
	OpenDoorMesh->SetRelativeLocation(FVector(-18.f, -16.528f, -45.08f));
	OpenDoorMesh->SetRelativeRotation(FRotator(360.f, 88.17f, 90.f));

	OpenDoorMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OpenDoorMesh2"));
	OpenDoorMesh2->SetupAttachment(HeliDoorBoxComponent);
	OpenDoorMesh2->SetRelativeLocation(FVector(26.f, 14.f, 24.8f));
	OpenDoorMesh2->SetRelativeRotation(FRotator(540.f, 84.49f, 449.94f));

	HeliCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	HeliCollisionBox->SetupAttachment(RootMesh);
	HeliCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeliCollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	HeliCollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	HeliCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	HeliCollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	HeliCollisionBox->SetNotifyRigidBodyCollision(true);

	//HeliSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelicopterMesh"));
	//HeliSkeletalMesh->SetupAttachment(HeliSceneComponent);

	// Create a camera boom (pulls in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1800.f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 1300.0f);
	CameraBoom->CameraLagSpeed = 30.f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->SetWorldRotation(FRotator(0.f, 342.5f, 0.f));

	HeliSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HelicopterSound"));
	HeliSoundComponent->SetupAttachment(CameraBoom);

	FloatingComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingComponent"));
	FloatingComponent->MaxSpeed = HeliMaxSpeed;
	FloatingComponent->Acceleration = 900.f;
	FloatingComponent->Deceleration = 900.f;
	FloatingComponent->TurningBoost = 0.5f;

	HeliSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	HeliSphereComponent->SetupAttachment(RootComponent);
	HeliSphereComponent->ComponentTags.Add(FName("Helicopter"));
	HeliSphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//HeliSphereComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	HeliSphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	HeliSphereComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	HeliSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HeliSphereComponent->SetRelativeLocation(FVector(510.f, 0.f, 130.f));
	HeliSphereComponent->SetRelativeScale3D(FVector(4.f, 4.f, 4.f));

	// Don't rotate when the controller rotates. Let the controller only affect the camera
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true; // we should rotate with the controller
	bUseControllerRotationRoll = false;	
}

void AHelicopter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (!IsPlayerControlled())
		{
			SetOwner(nullptr);
			OwnedShooter = nullptr;
		}
		OnTakeAnyDamage.AddDynamic(this, &AHelicopter::ReceiveDamage);
		//HeliCollisionBox->OnComponentHit.AddDynamic(this, &AHelicopter::OnHit);
		InitializeWeapons();
	}

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	FloatingComponent->Deactivate();

	HeliSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &AHelicopter::OnSphereOverlap);
	HeliSphereComponent->OnComponentEndOverlap.AddDynamic(this, &AHelicopter::OnSphereEndOverlap);

	HeliDoorBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AHelicopter::OnDoorBoxOverlap);
	HeliDoorBoxComponent->OnComponentEndOverlap.AddDynamic(this, &AHelicopter::OnDoorBoxEndOverlap);

	//HeliSoundComponent->SetFloatParameter(FName("Pitch"), 0.f);
	//HeliSoundComponent->Play();
	//HeliSoundComponent->Activate();
}

void AHelicopter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckHelicopterStateAndDestroy();

	SmoothAim(DeltaTime);

	if (HasAuthority() && GetVelocity().Size() > 0)
	{
		HelicopterSpeed = GetVelocity().Size();
	}

	if ((IsLocallyControlled()))
	{
		UpdateHitNumbers();
		if (bToggleHover)
		{
			FVector NewLocation;

			float Time = GetGameTimeSinceCreation();  // Get the current game time
			float DeltaZ = FMath::Sin(Time * 1.5f) * 1.f;  // Calculate the delta Z based on a sine wave with a period of 2pi/5 and amplitude of 50
			NewLocation = GetActorLocation() + FVector(0.0f, 0.0f, DeltaZ);  // Add the delta Z to the actor's current location
			
			//SetActorLocation(NewLocation);
		}
		if (FloatingComponent && HeliSceneComponent)
		{
			if (HeliSoundComponent) HeliSoundComponent->SetPitchMultiplier(FloatingComponent->Velocity.Size() / FloatingComponent->MaxSpeed);
			if (HelicopterState == EHelicopterState::EHS_StartPossess)
			{
				float RollInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Roll, 10.f * MovementAxisValue.X, DeltaTime, 1.5f);
				float PitchInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Pitch, -3.f * MovementAxisValue.Y, DeltaTime, 1.5f);
				HeliSceneComponent->SetRelativeRotation(FRotator(PitchInterp, 0.f, RollInterp));
				FRotator WorldRotation = UKismetMathLibrary::TransformRotation(HeliSceneComponent->GetRelativeTransform(), FRotator(PitchInterp, 0.f, RollInterp));
				WorldRotation.Yaw = GetActorRotation().Yaw;
				WorldRotation.Pitch = WorldRotation.Pitch + GetActorRotation().Pitch;
				SetActorRotation(WorldRotation);

				if (!HasAuthority())
				{
					//ServerSetHeliTransform(HeliSceneComponent->GetComponentTransform(), FloatingComponent->Velocity.Size());
				}
				else
				{
					HelicopterSpeed = FloatingComponent->Velocity.Size();
				}
				
				LastPitch = GetActorRotation().Pitch;
				LastRoll = GetActorRotation().Roll;
			}
			else if (HelicopterState == EHelicopterState::EHS_StartUnpossess)
			{
				float RollInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Roll, 0.f, DeltaTime, 2.5f);
				float PitchInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Pitch, 0.f, DeltaTime, 2.5f);
				HeliSceneComponent->SetRelativeRotation(FRotator(PitchInterp, 0.f, RollInterp));
				//FRotator WorldRotation = UKismetMathLibrary::TransformRotation(HeliSceneComponent->GetRelativeTransform(), FRotator(PitchInterp, 0.f, RollInterp));
				//WorldRotation.Yaw = GetActorRotation().Yaw;
				//WorldRotation.Pitch = WorldRotation.Pitch + GetActorRotation().Pitch;
				//SetActorRotation(WorldRotation);
				
				float FullPitchInterp = FMath::FInterpTo(LastPitch, 0.f, DeltaTime, 2.5f);
				float FullRollInterp = FMath::FInterpTo(LastRoll, 0.f, DeltaTime, 2.5f);
				SetActorRotation(FRotator(FullPitchInterp, GetActorRotation().Yaw, FullRollInterp));
				if (!HasAuthority())
				{
					//ServerSetHeliTransform(HeliSceneComponent->GetComponentTransform(), FloatingComponent->Velocity.Size());
				}
				else
				{
					HelicopterSpeed = FloatingComponent->Velocity.Size();
				}
				LastPitch = GetActorRotation().Pitch;
				LastRoll = GetActorRotation().Roll;
				//UE_LOG(LogTemp, Warning, TEXT("RelativeRot=%s"), *GetActorRotation().ToString());
			}
			else
			{
				if (!HasAuthority())
				{
					//ServerSetHeliTransform(HeliSceneComponent->GetComponentTransform(), FloatingComponent->Velocity.Size());
				}
			}
		}
	}
	SmoothCameraToAIWinner(DeltaTime);
}

void AHelicopter::InitializeWeapons()
{
	//Inventory.SetNum(2);
	UWorld* World = GetWorld();
	if (World)
	{
		AHelicopterWeapon* FirstWeapon = World->SpawnActor<AHelicopterWeapon>(DefaultWeaponClass, GetActorTransform());
		if (FirstWeapon)
		{
			FirstWeapon->SetWeaponType(EHelicopterWeaponType::EHWT_PulsedBlaster);
			FirstWeapon->UpdateWeapon();
			Multicast_InitWeapon(FirstWeapon, 0);
			Inventory.Add(FirstWeapon);
		}

		AHelicopterWeapon* SecondWeapon = World->SpawnActor<AHelicopterWeapon>(DefaultWeaponClass, GetActorTransform());
		if (SecondWeapon)
		{
			SecondWeapon->SetWeaponType(EHelicopterWeaponType::EHWT_Missile);
			SecondWeapon->UpdateWeapon();
			Multicast_InitWeapon(SecondWeapon, 1);
			Inventory.Add(SecondWeapon);
		}
	}
}

void AHelicopter::Multicast_InitWeapon_Implementation(AHelicopterWeapon* Weapon, int32 SlotIndex)
{
	if (Weapon == nullptr) return;
	Weapon->SlotIndex = SlotIndex;
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, false);
	Weapon->AttachToComponent(GetRootComponent(), AttachmentRules);
}

void AHelicopter::ServerSetHeliTransform_Implementation(FTransform Trans, float HeliSpeed)
{
	SetActorTransform(Trans);
	HelicopterSpeed = HeliSpeed;
}

void AHelicopter::OnRep_HelicopterSpeed()
{
	if (!IsLocallyControlled())
	{
		HeliSoundComponent->SetPitchMultiplier(HelicopterSpeed / FloatingComponent->MaxSpeed);
	}
}

void AHelicopter::CheckHelicopterStateAndDestroy()
{
	if (HasAuthority() && bBeginDestroy)
	{
		CurrentFallingSpeed = GetVelocity().Size();
		CurrentFallingHeight = GetActorLocation().Z;
		if (CurrentFallingSpeed < PreviousFallingSpeed && CurrentFallingHeight < PreviousFallingHeight)
		{
			bBeginDestroy = false;
			MulticastElimHelicopter();
		}
		PreviousFallingSpeed = CurrentFallingSpeed;
		PreviousFallingHeight = CurrentFallingHeight;
	}
}

AShooterPlayerController* AHelicopter::GetShooterController()
{
	if (Controller)
	{
		return Cast<AShooterPlayerController>(Controller);
	}
	return nullptr;
}

void AHelicopter::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// OtherActor is the actor that overlaps with the sphere and triggers the event
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter && !bDriveHelicopter)
		{
			bDriveHelicopter = true;
			AShooterPlayerController* ShooterPC = Cast<AShooterPlayerController>(ShooterCharacter->Controller);
			if (ShooterPC)
			{
				ShooterPC->GeneralMessage(TEXT("Drive the drone"), TEXT("Default"), 0.f);
			}
		}
	}
}

void AHelicopter::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			bDriveHelicopter = false;
		}
	}
}

void AHelicopter::OnDoorBoxOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// OtherActor is the actor that overlaps with the sphere and triggers the event
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter && !bDoorOverlap)
		{
			bDoorOverlap = true;
			AShooterPlayerController* ShooterPC = Cast<AShooterPlayerController>(ShooterCharacter->Controller);
			if (ShooterPC)
			{
				ShooterPC->GeneralMessage(TEXT("Trigger Back Door..."), TEXT("Default"), 0.f);
			}
		}
	}
}

void AHelicopter::OnDoorBoxEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			bDoorOverlap = false;
		}
	}
}

void AHelicopter::ServerStartPossess_Implementation()
{
	HelicopterState = EHelicopterState::EHS_StartPossess;
	OnRep_HelicopterState();
	EquippedWeapon = Inventory[0];
	if (!bHeliDoorOpen)
	{
		bHeliDoorOpen = true;
		OnRep_HeliDoorOpen();
	}
}

void AHelicopter::ServerStopPossess_Implementation(bool bPlayerWon)
{
	EquippedWeapon = nullptr;
	if (OwnedShooter == nullptr) return;
	HelicopterState = EHelicopterState::EHS_StartUnpossess;
	OnRep_HelicopterState();

	if (bHeliDoorOpen)
	{
		bHeliDoorOpen = false;
		OnRep_HeliDoorOpen();
	}

	FTimerHandle HeliUnpossessTimer;
	FTimerDelegate HeliUnpossessDelegate;
	HeliUnpossessDelegate.BindUFunction(this, FName("UnpossessFromHeli"), OwnedShooter, bPlayerWon);
	GetWorld()->GetTimerManager().SetTimer(
		HeliUnpossessTimer,
		HeliUnpossessDelegate,
		1.5f,
		false);
}

void AHelicopter::UnpossessFromHeli(AShooterCharacter* ShooterCharacter, bool bPlayerWon)
{
	// First stop movement and then run this code below
	auto ShooterPlayerController = Cast<AShooterPlayerController>(Controller);
	if (ShooterCharacter && ShooterPlayerController)
	{
		OwnedShooter = nullptr;
		ShooterPlayerController->UnPossess();
		SpawnDefaultController();
		ShooterPlayerController->Possess(ShooterCharacter);
		ShooterCharacter->MulticastOnPossessHeli(this, false);
		SetOwner(nullptr);
		HelicopterState = EHelicopterState::EHS_Unpossess;
		OnRep_HelicopterState();
		
		if (bPlayerWon)
		{
			ShooterCharacter->Winner();
		}
	}
}

// Do a line trace from the bottom fo the Helicopter to the ground (vertically down)
bool AHelicopter::TraceFromBase(FHitResult& OutHitResult)
{
	const FVector Start{ RootMesh->GetComponentLocation() };
	const FVector End{ Start + 50000.f * (-RootMesh->GetUpVector()) };
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility);

	if (OutHitResult.bBlockingHit)
	{
		return true;
	}
	else
	{
		OutHitResult.ImpactPoint = End;
		return false;
	}
}

void AHelicopter::OnRep_HelicopterState()
{
	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetOwner());
	switch (HelicopterState)
	{
	case EHelicopterState::EHS_PrePossess:
		break;

	case EHelicopterState::EHS_StartPossess:
		//NetDormancy = ENetDormancy::DORM_Awake;
		if (PlayerController)
		{
			EnableInput(PlayerController);
		}
		InitializeAnimRefs(true);
		RightThrottle(true);
		LeftThrottle(true);
		HeliSoundComponent->Activate();
		bToggleHover = true;
		FloatingComponent->Activate();
		RootMesh->SetSimulatePhysics(false);
		//RootMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (IsLocallyControlled())
		{
			InventorySelect = 0;
			bCanFire = true;
			SetOverlapSphereEnabled(false);
			auto ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this));
			if (ShooterGI)
			{
				UShooterSaveGame* SaveGame;
				bool bSaveGameSuccess = false;
				ShooterGI->LoadGame(SaveGame, bSaveGameSuccess);
				if (SaveGame)
				{
					CameraBoom->bEnableCameraLag = SaveGame->bVehicleCameraLag;
					CameraBoom->CameraLagSpeed = SaveGame->VehicleCameraLagSpeed;
				}
			}

			if (PlayerController)
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
				{
					Subsystem->AddMappingContext(HeliContext, 0);
				}
			}
		}
		break;

	case EHelicopterState::EHS_StartUnpossess:
		if (PlayerController)
		{
			DisableInput(PlayerController);
			if (OwnedShooter)
			{
				PlayerController->SetViewTargetWithBlend(OwnedShooter, 1.f);
			}
		}
		if (IsLocallyControlled())
		{
			bCanFire = false;
			GetWorldTimerManager().ClearTimer(AutoFireTimer);
		}
		break;

	case EHelicopterState::EHS_Unpossess:
	{
		InitializeAnimRefs(true);
		RightPropelPosition(TEXT("Idle"));
		LeftPropelPosition(TEXT("Idle"));
		RightThrottle(true);
		LeftThrottle(true);
		HeliSoundComponent->Activate();
		HeliSoundComponent->SetPitchMultiplier(0.f);

		FTimerHandle OverlapSphereTimer;
		FTimerDelegate OverlapSphereDelegate;
		OverlapSphereDelegate.BindUFunction(this, FName("SetOverlapSphereEnabled"), true);
		GetWorld()->GetTimerManager().SetTimer(OverlapSphereTimer, OverlapSphereDelegate, 1.f, false);
		break;
	}
	default:
		break;
	}
}

void AHelicopter::OnRep_OwnedShooter()
{
	if (OwnedShooter)
	{
		SetAmmo(OwnedShooter);
	}
}

void AHelicopter::OnRep_HeliDoorOpen()
{
	ToggleBackDoor(bHeliDoorOpen);
}

void AHelicopter::OnRep_CombatState()
{
	switch (CombatState)
	{
	case EHeliCombatState::EHCS_Unoccupied:
		/* code */
		break;
	case EHeliCombatState::EHCS_Equipping:
		break;
	case EHeliCombatState::EHCS_FireTimerInProgress:
		break;
	default:
		break;
	}
}

void AHelicopter::SetOverlapSphereEnabled(bool bOverlapSphere)
{
	if (HeliSphereComponent == nullptr) return;
	if (bOverlapSphere)
	{
		HeliSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		HeliSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AHelicopter::SetAmmo(AShooterCharacter* ShooterCharacter)
{
	if (ShooterCharacter == nullptr) return;
	for (auto HeliWeapon : Inventory)
	{
		if (HeliWeapon && HeliWeapon->bUsesAmmo)
		{
			int32 AmmoAmount = ShooterCharacter->GetAmmoAmount(HeliWeapon->GetAmmoType());
			if (AmmoAmount != -1)
			{
				HeliWeapon->SetAmmo(AmmoAmount);
			}
		}
	}
}

bool AHelicopter::TraceFromCrosshairs(FHitResult& CrosshairHitResult)
{
	if (bScreenToWorld)
	{
		// Trace from Crosshair world location outward
        FCollisionQueryParams CollisionParam;
        CollisionParam.AddIgnoredActor(this);

		const FVector Start{ ShotPosition };
		const FVector End{ Start + 50000.f * ShotDirection };
		GetWorld()->LineTraceSingleByChannel(
			CrosshairHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility,
			CollisionParam);

		if (CrosshairHitResult.bBlockingHit)
		{
			return true;
		}
		else
		{
			CrosshairHitResult.ImpactPoint = End;
		}
	}
	return false;
}

bool AHelicopter::TraceFromWeapon(
	FHitResult &WeaponTraceHit,
	const FVector &MuzzleSocketLocation, 
	const FVector_NetQuantize &OutBeamLocation)
{
	//OutBeamLocation = bCrosshairHit ? CrosshairHitResult.ImpactPoint : OutBeamLocation;
	// Perform a second trace, this time from the gun barrel
	FCollisionQueryParams CollisionParam;
	CollisionParam.AddIgnoredActor(this);
	//CollisionParam.AddIgnoredActor(EquippedItem);

	const FVector_NetQuantize WeaponTraceStart{ MuzzleSocketLocation };
	const FVector_NetQuantize StartToEnd{ OutBeamLocation - MuzzleSocketLocation };
	const FVector_NetQuantize WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f};
	GetWorld()->LineTraceSingleByChannel(
		WeaponTraceHit, 
		WeaponTraceStart, 
		WeaponTraceEnd, 
		ECollisionChannel::ECC_Visibility,
		CollisionParam);

	if (WeaponTraceHit.bBlockingHit) // Hits object between barrel and BeamEndPoint?
	{
		//UE_LOG(LogTemp, Warning, TEXT("Line Trace Hit!!!"));
		//OutBeamLocation = WeaponTraceHit.ImpactPoint;
		return true;
	}
	else
	{
		WeaponTraceHit.ImpactPoint = WeaponTraceEnd;
		return false;
	}
	return false;
}

void AHelicopter::MainMovement(const FInputActionValue& Value)
{
	MovementAxisValue = Value.Get<FVector2D>();
	if (Controller != nullptr && MovementAxisValue.Y != 0.f)
	{
		const FVector ForwardDirection = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::X);
        FVector Force = ForwardDirection * MovementAxisValue.Y;
        FloatingComponent->AddInputVector(Force);

		if (MovementAxisValue.Y > 0.f)
		{
			RightPropelPosition(TEXT("Back"));
			LeftPropelPosition(TEXT("Back"));
		}
		else if (MovementAxisValue.Y < 0.f)
		{
			RightPropelPosition(TEXT("Front"));
			LeftPropelPosition(TEXT("Front"));
		}
	}
	//PreviousMoveForwardInput = AxisValue.Y;

	if ((Controller != nullptr) && (MovementAxisValue.X != 0.0f))
	{
		FVector RightDirection = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);
		//AddMovementInput(Direction, Value);
        FVector Force = RightDirection * MovementAxisValue.X;
        FloatingComponent->AddInputVector(Force);
		//float RollInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Roll, 10.f * Value, GetWorld()->DeltaTimeSeconds, 1.5f);
		//HeliSceneComponent->SetRelativeRotation(FRotator(0.f, 0.f, RollInterp)); 
	}
}

void AHelicopter::OnMovementStopped()
{
	MovementAxisValue.X = 0.f;
	MovementAxisValue.Y = 0.f;
	RightPropelPosition(TEXT("Idle"));
	LeftPropelPosition(TEXT("Idle"));
}

void AHelicopter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::X);
		//AddMovementInput(Direction, Value);
        FVector Force = Direction * Value;
        FloatingComponent->AddInputVector(Force);

		HeliSoundComponent->SetPitchMultiplier(FloatingComponent->Velocity.Size() / FloatingComponent->MaxSpeed);
		//float PitchInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Pitch, 5.f * Value, GetWorld()->DeltaTimeSeconds, 1.5f);
		//HeliSceneComponent->SetRelativeRotation(FRotator(PitchInterp, 0.f, 0.f)); 

		if (Value != PreviousMoveForwardInput)
		{
			if (Value > 0.f)
			{
				RightPropelPosition(TEXT("Back"));
				LeftPropelPosition(TEXT("Back"));
			}
			else if (Value < 0.f)
			{
				RightPropelPosition(TEXT("Front"));
				LeftPropelPosition(TEXT("Front"));
			}
		}
		//HeliSoundComponent->SetFloatParameter(FName("Pitch"), FloatingComponent->Velocity.Size() / FloatingComponent->MaxSpeed);
	}

	if ((Controller != nullptr) && (Value == 0.0f))
	{
		HeliSoundComponent->SetPitchMultiplier(FloatingComponent->Velocity.Size() / FloatingComponent->MaxSpeed);
		//float PitchInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Pitch, 5.f * Value, GetWorld()->DeltaTimeSeconds, 1.5f);
		//HeliSceneComponent->SetRelativeRotation(FRotator(PitchInterp, 0.f, 0.f)); 
		if (Value != PreviousMoveForwardInput)
		{
			RightPropelPosition(TEXT("Idle"));
			LeftPropelPosition(TEXT("Idle"));
		}
		//HeliSoundComponent->SetFloatParameter(FName("Pitch"), FloatingComponent->Velocity.Size() / FloatingComponent->MaxSpeed);
	}
	PreviousMoveForwardInput = Value;
}

void AHelicopter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);
		//AddMovementInput(Direction, Value);
        FVector Force = Direction * Value;
        FloatingComponent->AddInputVector(Force);
		//float RollInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Roll, 10.f * Value, GetWorld()->DeltaTimeSeconds, 1.5f);
		//HeliSceneComponent->SetRelativeRotation(FRotator(0.f, 0.f, RollInterp)); 
	}

	if ((Controller != nullptr) && (Value == 0.0f))
	{
		//float RollInterp = FMath::FInterpTo(HeliSceneComponent->GetRelativeRotation().Roll, 10.f * Value, GetWorld()->DeltaTimeSeconds, 1.5f);
		//HeliSceneComponent->SetRelativeRotation(FRotator(0.f, 0.f, RollInterp)); 
	}
}

void AHelicopter::LookRate(const FInputActionValue& Value)
{
	if (GetController())
	{
		const float AxisValue = Value.Get<float>();
		AddControllerPitchInput(AxisValue * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	}
}

void AHelicopter::TurnRate(const FInputActionValue& Value)
{
	if (GetController())
	{
		const float AxisValue = Value.Get<float>();
		AddControllerYawInput(AxisValue * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void AHelicopter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHelicopter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHelicopter::Turn(float Value)
{
	float TurnScaleFactor{};
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}

	AddControllerYawInput(TurnScaleFactor * Value);
}

void AHelicopter::Look(const FInputActionValue& Value)
{
	if (GetController())
	{
		FVector2D AxisValue = Value.Get<FVector2D>();
		float TurnScaleFactor{};
		float NewPitch;
		AxisValue.Y *= -1.f;
		// Limit the pitch angle between -45 and 45 degrees
		AxisValue.Y = FMath::Clamp(AxisValue.Y, -1.f, 1.f);

		if (bAiming)
		{
			TurnScaleFactor = MouseAimingTurnRate;
			NewPitch = GetControlRotation().Pitch + MouseAimingLookUpRate * AxisValue.Y;
		}
		else
		{
			TurnScaleFactor = MouseHipTurnRate;
			NewPitch = GetControlRotation().Pitch + MouseHipLookUpRate * AxisValue.Y;
		}

		AddControllerYawInput(TurnScaleFactor * AxisValue.X);
		//float NewPitch = GetControlRotation().Pitch + MouseHipLookUpRate * AxisValue.Y;
		NewPitch = FMath::ClampAngle(NewPitch, -45.f, 45.f);
		FRotator NewRotation = FRotator(NewPitch, GetControlRotation().Yaw, GetControlRotation().Roll);

		// Apply the new pitch angle to the controller
		GetController()->SetControlRotation(NewRotation);
	}
}

void AHelicopter::LookUp(float Value)
{
	Value *= -1.f;
    // Limit the pitch angle between -45 and 45 degrees
    Value = FMath::Clamp(Value, -1.f, 1.f);
    float NewPitch = GetControlRotation().Pitch + MouseHipLookUpRate * Value;
    NewPitch = FMath::ClampAngle(NewPitch, -45.f, 45.f);
    FRotator NewRotation = FRotator(NewPitch, GetControlRotation().Yaw, GetControlRotation().Roll);

    // Apply the new pitch angle to the controller
    GetController()->SetControlRotation(NewRotation);
}

void AHelicopter::AimingActionButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) AimingButtonPressed();
}

void AHelicopter::AimingActionButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) AimingButtonReleased();
}

void AHelicopter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != EHeliCombatState::EHCS_Equipping)
	{
		SetAiming(true);
	}
}

void AHelicopter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	SetAiming(false);	
}

void AHelicopter::SetAiming(bool bIsAiming)
{
	if (EquippedWeapon == nullptr) return;
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
	//bIsAiming ? Set_TargetMovementSpeed(HeliMaxSpeed / 2.f) : Set_TargetMovementSpeed(HeliMaxSpeed);

	if (IsLocallyControlled()) bAimingButtonPressed = bIsAiming;
}

void AHelicopter::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	//bIsAiming ? Set_TargetMovementSpeed(HeliMaxSpeed / 2.f) : Set_TargetMovementSpeed(HeliMaxSpeed);
}

void AHelicopter::SmoothAim(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	// Set current camera field of view
	if (bAiming)
	{
		// Interpolate to zoomed FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		// Interpolate to default FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	FollowCamera->SetFieldOfView(CameraCurrentFOV);
}

void AHelicopter::StartSmoothCameraToAIWinner(FVector InAILocation, FRotator InAIRotation)
{
	AIWinnerLocation = InAILocation;
	AIWinnerRotation = InAIRotation;
	bStartCameraMoveToAIWinner = true;
}

void AHelicopter::SmoothCameraToAIWinner(float DeltaTime)
{
	if (bStartCameraMoveToAIWinner && FollowCamera)
	{
		FollowCamera->SetWorldLocation(FMath::VInterpTo(FollowCamera->GetComponentLocation(), AIWinnerLocation, DeltaTime, 4.f));
		FollowCamera->SetWorldRotation(FMath::RInterpTo(FollowCamera->GetComponentRotation(), AIWinnerRotation, DeltaTime, 4.f));
	}
}

void AHelicopter::Set_TargetMovementSpeed_Implementation(const float InTargetMovementSpeed)
{
	TargetMovementSpeed = InTargetMovementSpeed;
	FloatingComponent->MaxSpeed = TargetMovementSpeed;
}

void AHelicopter::OnRep_TargetMovementSpeed(float OldTargetMovementSpeed)
{
	FloatingComponent->MaxSpeed = TargetMovementSpeed;
}

void AHelicopter::SelectButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) ServerStopPossess();
}

void AHelicopter::SelectButtonReleased(const FInputActionValue& Value)
{

}

void AHelicopter::FireActionButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) FireButtonPressed();
}

void AHelicopter::FireActionButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) FireButtonReleased();
}


void AHelicopter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AHelicopter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AHelicopter::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorld()->GetTimerManager().SetTimer(
		HitNumberTimer,
		HitNumberDelegate,
		HitNumberDestroyTime,
		false);
}

void AHelicopter::DestroyHitNumber(UUserWidget* HitNumber)
{
	if(!HitNumber) return;

	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

void AHelicopter::UpdateHitNumbers()
{
	for (auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber{ HitPair.Key };
		
		if(!HitNumber) return;

		const FVector Location{ HitPair.Value };
		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(
			GetWorld()->GetFirstPlayerController(),
			Location,
			ScreenPosition);
		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

bool AHelicopter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) { return false; }
	if (EquippedWeapon->bUsesAmmo)
	{
		return EquippedWeapon->GetAmmo() > 0;
	}
	else
	{
		return true;
	}
}

bool AHelicopter::CanFire()
{
	return WeaponHasAmmo() && bCanFire && EquippedWeapon;
}

void AHelicopter::FireWeapon()
{
	if (CanFire() && EquippedWeapon) 
	{
		bCanFire = false;
		FHitResult HitResult;
		bool bHit = TraceFromCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		if (!HasAuthority()) LocalFire(GetHitscanHits(HitTarget));
		ServerFire(GetHitscanHits(HitTarget), EquippedWeapon->AutoFireRate);

		StartFireTimer();
	}
}

void AHelicopter::StartFireTimer()
{
	if (EquippedWeapon == nullptr) return;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AHelicopter::AutoFireReset, EquippedWeapon->AutoFireRate);
}

void AHelicopter::AutoFireReset()
{
	bCanFire = true;
	if (bFireButtonPressed)
	{
		if (WeaponIndex == 1)
		{
			WeaponIndex = 0;
		}
		else
		{
			++WeaponIndex;
		}
		FireWeapon();
	}
}

FHitResult AHelicopter::GetHitscanHits(const FVector_NetQuantize& TraceHitTarget)
{
	FHitResult WeaponTraceHit;
	if (WeaponIndex == 0)
	{
		bool bBeamEnd = TraceFromWeapon(WeaponTraceHit, HeliSceneWeapon2->GetComponentLocation(), TraceHitTarget);
		return WeaponTraceHit;
	}
	else if (WeaponIndex == 1)
	{
		bool bBeamEnd = TraceFromWeapon(WeaponTraceHit, HeliSceneWeapon1->GetComponentLocation(), TraceHitTarget);
		return WeaponTraceHit;
	}
	return WeaponTraceHit;
}

void AHelicopter::ServerFire_Implementation(const FHitResult& WeaponTraceHit, float FireDelay)
{
	MulticastFire_Implementation(WeaponTraceHit, FireDelay);
}

void AHelicopter::MulticastFire_Implementation(const FHitResult& WeaponTraceHit, float FireDelay)
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	LocalFire(WeaponTraceHit);
}

void AHelicopter::LocalFire(const FHitResult& WeaponTraceHit)
{
	//DrawDebugLine(GetWorld(), WeaponTraceHit.TraceStart, WeaponTraceHit.ImpactPoint, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), WeaponTraceHit.ImpactPoint, 4.f, 12, FColor::Orange, true);
	if (OwnedShooter && EquippedWeapon)
	{
		EquippedWeapon->FireProjectile(WeaponTraceHit, OwnedShooter, Controller->GetPawn());
		if (EquippedWeapon->bUsesAmmo)
		{
			EquippedWeapon->SpendRound(OwnedShooter);
		}
		PlayFireSound();
		auto ShooterPS = Cast<AShooterPlayerState>(GetPlayerState());
		if (HasAuthority() && ShooterPS)
		{
			ShooterPS->PlayerGameStats.ShotsFired++;
		}
	}
}

void AHelicopter::PlayFireSound()
{
	// Play fire sound
	if (EquippedWeapon && EquippedWeapon->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->FireSound, GetActorLocation());
	}
}



void AHelicopter::OnRep_EquippedWeapon()
{
	if (EquippedWeapon == nullptr) return;

	// Play equip sound
	if (EquippedWeapon && EquippedWeapon->EquipSound)
	{
		//UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, GetActorLocation());
		if (IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(this, EquippedWeapon->EquipSound);
		}
		
		EquippedWeapon->OnEquipped();
	}
}

AShooterGameMode* AHelicopter::GetShooter_GM()
{
	if (GetWorld())
	{
		ShooterGameMode = ShooterGameMode == nullptr ? GetWorld()->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	}
    
    if (ShooterGameMode)
    {
        return ShooterGameMode;
    }
    return nullptr;
}

AShooterGameState* AHelicopter::GetShooter_GS()
{
	if (GetWorld())
	{
		AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
		if (ShooterGameState) return ShooterGameState;
	}
    return nullptr;
}

void AHelicopter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (!HasAuthority()) return;

	if (AShooterGameState* ShooterGS = GetShooter_GS())
	{
		AShooterPlayerController* VictimController = nullptr; AShooterPlayerController* AttackerController = nullptr;
		AShooterPlayerState* VictimPS = nullptr; AShooterPlayerState* AttackerPS = nullptr;
		UCharacterGlobalFuncs::GetShooterReferences(VictimController, AttackerController, VictimPS, AttackerPS, DamagedActor, DamageCauser, InstigatorController);
		if (CheckFriendlyHit(DamagedActor, DamageCauser, VictimPS, AttackerPS, ShooterGS)) return;

		float OldHealth = Health;
		Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
		OnRep_Health(OldHealth);
		//UE_LOG(LogTemp, Warning, TEXT("HIT!"));
		UCharacterGlobalFuncs::UpdateDamageStat(DamagedActor, DamageCauser, VictimPS, AttackerPS, Damage, 100.f, DamageType);

		if (Health <= 0.f)
		{
			OnTakeAnyDamage.RemoveDynamic(this, &AHelicopter::ReceiveDamage);
			bBeginDestroy = true;
			if (OwnedShooter)
			{
				if (AttackerPS && !AttackerPS->IsABot())
				{
					AttackerPS->AddToXPMetric(EProgressMetric::EPM_TakeDownAttackDrone, 1);
				}
				UnpossessFromHeli(OwnedShooter, false);
			}
		}
	}
}

bool AHelicopter::CheckFriendlyHit(const AActor* DamagedCharacter, const AActor* AttackerCharacter, 
		AShooterPlayerState* VictimPS, AShooterPlayerState* AttackerPS, AShooterGameState* ShooterGS)
{
	if (ShooterGS && ShooterGS->bIsTeamMode)
	{
		if (AttackerPS && VictimPS)
		{
			if (VictimPS->GetGenericTeamId() == AttackerPS->GetGenericTeamId() && VictimPS != AttackerPS)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return false;
}

void AHelicopter::GetShooterReferences(AShooterCharacter*& AttackerCharacter, AActor* AttackerActor)
{
    if (AttackerActor)
    {
        if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(AttackerActor))
        {
            AttackerCharacter = ShooterInterface->Execute_GetShooterCharacter(AttackerActor);
        }
    }
}

FVector AHelicopter::GetPawnLocation_Implementation()
{
	return GetActorLocation();
}

float AHelicopter::GetPawnRotationYaw_Implementation()
{
	return GetRootMesh()->GetComponentRotation().Yaw - 90.f;
}

void AHelicopter::OnRep_Health(float OldHealth)
{
	if (Health <= 0.f)
	{
		RootMesh->SetSimulatePhysics(true);
		HeliCollisionBox->SetSimulatePhysics(true);
		FloatingComponent->Deactivate();
		if (FallingSound)
		{
			FallingSoundComponent = UGameplayStatics::SpawnSoundAttached(
					FallingSound, // sound cue (USoundBase)
					RootMesh, // mesh to attach to
					FName(""),   //socket name
					FVector(0,0,0),  //location relative to socket
					FRotator(0,0,0), //rotation 
					EAttachLocation::KeepRelativeOffset, 
					true //if true, will be deleted automatically
			);	
		}

		bCanFire = false;
		GetWorldTimerManager().ClearTimer(AutoFireTimer);
		
	
		ToggleBackDoor(false);
	}

    if (Health < OldHealth)
    {
        DamageTakenDelegate.Broadcast(true);
    }
    HealthHudDelegate.Broadcast(Health / MaxHealth, true);
}

void AHelicopter::OnRep_HelicopterHitPoint1()
{
	float HeliHealthPercent = Health / MaxHealth * 100.f;
	if (HeliHealthPercent < 80.f)
	{
		SpawnHelicopterDamageEffects(HelicopterHitPoint1, ParticleHitEffect1);
	}
}

void AHelicopter::OnRep_HelicopterHitPoint2()
{
	float HeliHealthPercent = Health / MaxHealth * 100.f;
	if (HeliHealthPercent < 55.f)
	{
		SpawnHelicopterDamageEffects(HelicopterHitPoint2, ParticleHitEffect2);
	}
}

void AHelicopter::OnRep_HelicopterHitPoint3()
{
	float HeliHealthPercent = Health / MaxHealth * 100.f;
	if (HeliHealthPercent < 30.f)
	{
		SpawnHelicopterDamageEffects(HelicopterHitPoint3, ParticleHitEffect3);
	}
}

void AHelicopter::HelicopterDamageEffects(FVector_NetQuantize HitPointWorld)
{
	float HeliHealthPercent = Health / MaxHealth * 100.f;
	if (HeliHealthPercent < 80.f && HeliHealthPercent >= 55.f && !bHitEffect1)
	{
		// Convert HitPointWorld position vector in world space and return the corresponding position vector in the local space
		FVector LocalHitLocation = RootMesh->GetComponentTransform().InverseTransformPosition(HitPointWorld);
		bHitEffect1 = true;
		HelicopterHitPoint1 = LocalHitLocation;
		OnRep_HelicopterHitPoint1();
		//DrawDebugSphere(GetWorld(), HelicopterHitPoint1 + RootMesh->GetComponentLocation(), 50.f, 40, FColor::Blue, false, 15.f);
	}
	if (HeliHealthPercent < 55.f && HeliHealthPercent >= 30.f && !bHitEffect2)
	{
		FVector LocalHitLocation = RootMesh->GetComponentTransform().InverseTransformPosition(HitPointWorld);
		bHitEffect2 = true;
		HelicopterHitPoint2 = LocalHitLocation;
		OnRep_HelicopterHitPoint2();
	}
	if (HeliHealthPercent < 30.f && !bHitEffect3)
	{
		FVector LocalHitLocation = RootMesh->GetComponentTransform().InverseTransformPosition(HitPointWorld);
		bHitEffect3 = true;
		HelicopterHitPoint3 = LocalHitLocation;
		OnRep_HelicopterHitPoint3();
	}
}

void AHelicopter::SpawnHelicopterDamageEffects(FVector_NetQuantize HitLocation, UParticleSystem* InParticleEffect)
{
	if (InParticleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			InParticleEffect, 
			RootMesh, 
			NAME_None, 
			HitLocation, 
			FRotator::ZeroRotator, 
			EAttachLocation::KeepRelativeOffset);
	}
}

void AHelicopter::HelicopterFallApart()
{
	TArray<UActorComponent*> ActorComps;
	ActorComps = GetComponentsByTag(UStaticMeshComponent::StaticClass(), FName("HelicopterPart"));
	for (auto uActor : ActorComps)
	{
		auto HeliStaticMesh = Cast<UStaticMeshComponent>(uActor);
		if (HeliStaticMesh)
		{
			FRandomStream RandomStream;
			HeliStaticMesh->SetSimulatePhysics(true);
			//HeliStaticMesh->AddImpulse(30000.f * RandomStream.GetUnitVector());
			HeliStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			HeliStaticMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			HeliStaticMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
			HeliStaticMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
		}
	}
}

void AHelicopter::MulticastElimHelicopter_Implementation()
{
	HeliSphereComponent->DestroyComponent();
	HeliSoundComponent->FadeOut(0.5f, 0.f);
	if (ExplodeSound)
	{
        UGameplayStatics::SpawnSoundAtLocation(
            GetWorld(),
            ExplodeSound,
            GetActorLocation(),
            GetActorRotation(),
            1.0f, // volume multiplier
            1.0f, // pitch multiplier
            0.0f, // start time
            nullptr, //USoundAttenuation
            nullptr,
            true
        );
	}

	if (ExplodeEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplodeEffect, 
			GetActorLocation(), 
			FRotator(0.f,0.f,0.f), 
			FVector3d(3.5f), 
			true);
	}
	//HelicopterFallApart();
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Destroy();
	//FTimerHandle DestroyTimer;
	//GetWorldTimerManager().SetTimer(DestroyTimer, this, &AHelicopter::DestroyHelicopter, 5.f);
	
}

void AHelicopter::DestroyHelicopter()
{
	Destroy();
}

void AHelicopter::Destroyed()
{
	Super::Destroyed();

	if (HeliSoundComponent)
	{
		HeliSoundComponent->DestroyComponent();
	}

	if (FallingSoundComponent)
	{
		FallingSoundComponent->DestroyComponent();
	}

	if (ParticleHitComponent1)
	{
		ParticleHitComponent1->DestroyComponent();
	}

	if (ParticleHitComponent2)
	{
		ParticleHitComponent2->DestroyComponent();
	}

	if (ParticleHitComponent3)
	{
		ParticleHitComponent3->DestroyComponent();
	}
}


void AHelicopter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHelicopter::Look);
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &AHelicopter::MainMovement);
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Completed, this, &AHelicopter::OnMovementStopped);
		EnhancedInputComponent->BindAction(LookRateAction, ETriggerEvent::Triggered, this, &AHelicopter::LookRate);
		EnhancedInputComponent->BindAction(TurnRateAction, ETriggerEvent::Triggered, this, &AHelicopter::TurnRate);

		EnhancedInputComponent->BindAction(AimingActionPressed, ETriggerEvent::Triggered, this, &AHelicopter::AimingActionButtonPressed);
		EnhancedInputComponent->BindAction(AimingActionPressed, ETriggerEvent::Completed, this, &AHelicopter::AimingActionButtonReleased);

		EnhancedInputComponent->BindAction(FireActionPressed, ETriggerEvent::Triggered, this, &AHelicopter::FireActionButtonPressed);
		EnhancedInputComponent->BindAction(FireActionPressed, ETriggerEvent::Completed, this, &AHelicopter::FireActionButtonReleased);

		EnhancedInputComponent->BindAction(SelectActionPressed, ETriggerEvent::Triggered, this, &AHelicopter::SelectButtonPressed);
		EnhancedInputComponent->BindAction(SelectActionPressed, ETriggerEvent::Completed, this, &AHelicopter::SelectButtonReleased);

		EnhancedInputComponent->BindAction(InventoryAction_1, ETriggerEvent::Triggered, this, &AHelicopter::OneKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_2, ETriggerEvent::Triggered, this, &AHelicopter::TwoKeyPressed);

		EnhancedInputComponent->BindAction(InventoryActionSwitchLeft, ETriggerEvent::Triggered, this, &AHelicopter::InventorySwitchLeft);
		EnhancedInputComponent->BindAction(InventoryActionSwitchRight, ETriggerEvent::Triggered, this, &AHelicopter::InventorySwitchRight);
	}

	/*
	PlayerInputComponent->BindAxis("MoveForward", this, &AHelicopter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHelicopter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHelicopter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHelicopter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AHelicopter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AHelicopter::LookUp);
	

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AHelicopter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AHelicopter::AimingButtonReleased);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AHelicopter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AHelicopter::FireButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AHelicopter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AHelicopter::SelectButtonReleased);

	PlayerInputComponent->BindAction("1Key", IE_Pressed, this, &AHelicopter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this, &AHelicopter::TwoKeyPressed);
	*/

}

void AHelicopter::InventorySwitchLeft(const FInputActionValue& Value)
{
	if (InventorySelect > 0)
	{
		--InventorySelect;
		ExchangeInventoryLocal(EquippedWeapon->SlotIndex, InventorySelect);
		Server_ExchangeInventoryItems(EquippedWeapon->SlotIndex, InventorySelect);
	}
}

void AHelicopter::InventorySwitchRight(const FInputActionValue& Value)
{
	if (InventorySelect < Inventory.Num())
	{
		++InventorySelect;
		ExchangeInventoryLocal(EquippedWeapon->SlotIndex, InventorySelect);
		Server_ExchangeInventoryItems(EquippedWeapon->SlotIndex, InventorySelect);
	}
}

void AHelicopter::OneKeyPressed(const FInputActionValue& Value)
{
	if (EquippedWeapon->SlotIndex == 0) return;
	ExchangeInventoryLocal(EquippedWeapon->SlotIndex, 0);
	Server_ExchangeInventoryItems(EquippedWeapon->SlotIndex, 0);

}

void AHelicopter::TwoKeyPressed(const FInputActionValue& Value)
{
	if (EquippedWeapon->SlotIndex == 1) return;
	ExchangeInventoryLocal(EquippedWeapon->SlotIndex, 1);
	Server_ExchangeInventoryItems(EquippedWeapon->SlotIndex, 1);

}

void AHelicopter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
	bool bExchangeConditions = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		((CombatState != EHeliCombatState::EHCS_Unoccupied) || (CombatState != EHeliCombatState::EHCS_Equipping));

	if (!bExchangeConditions) return;

	EquippedWeapon = Inventory[NewItemIndex];
	EquippedWeapon->SlotIndex = NewItemIndex;
}

void AHelicopter::Server_ExchangeInventoryItems_Implementation(int32 CurrentItemIndex, int32 NewItemIndex)
{
	ExchangeInventoryItems(CurrentItemIndex, NewItemIndex);
}

void AHelicopter::ExchangeInventoryLocal(int32 CurrentItemIndex, int32 NewItemIndex)
{
	if (EquippedWeapon == nullptr)
	{
		// -1 == no EquippedWeapon yet. No need to reverse the inventory icon animation
		EquipItemDelegate.Broadcast(-1, NewItemIndex);
	}
	else
	{
		EquipItemDelegate.Broadcast(CurrentItemIndex, NewItemIndex);
	}
	if (EquippedWeapon == nullptr)
	{
		InventorySelect = CurrentItemIndex;
		return; 
	}
	InventorySelect = NewItemIndex;
}

void AHelicopter::OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr) return;
	float DamageToShow = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
	if (InstigatorShooter->HasAuthority())
	{				
		const float DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
		//if (InstigatorPS && IsPlayerControlled()) InstigatorPS->PlayerGameStats.FireCharacterHit++;
		
		UGameplayStatics::ApplyDamage(
			this,
			DamageAmount,
			InstigatorShooter->GetController(),
			InstigatorShooter,
			UDamageType::StaticClass()
		);

		HelicopterDamageEffects(InHitResult.ImpactPoint);
	}

	//DamagedHelicopter->SpawnHelicopterDamageEffects(WeaponTraceHit.ImpactPoint);

	if (OwnedShooter)
	{
		auto VictimPS = Cast<AShooterPlayerState>(GetPlayerState());
		if (InstigatorShooter->IsLocallyControlled() && VictimPS && InstigatorPS && VictimPS->GetGenericTeamId() != InstigatorPS->GetGenericTeamId())
		{
			InstigatorShooter->ShowHitNumber(DamageToShow, InHitResult.ImpactPoint, false);
		}
	}
	else
	{
		if (InstigatorShooter->IsLocallyControlled())
		{
			InstigatorShooter->ShowHitNumber(DamageToShow, InHitResult.ImpactPoint, false);
		}	
	}


	UGameplayStatics::PlaySoundAtLocation(
		InstigatorShooter,
		HitSound,
		InHitResult.ImpactPoint,
		0.35f
	);
}

void AHelicopter::OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType)
{
	if (!InstigatorController || !HasAuthority()) return;
	AShooterCharacter* InstigatorShooter = Cast<AShooterCharacter>(InstigatorController->GetPawn());

	float DamageToApply = 0.f;
	if (InstigatorShooter && InstigatorShooter->GetEquippedWeapon())
	{
		DamageToApply = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
	}
	else if (Damage > 0.f)
	{
		DamageToApply = Damage;
	}
			
	UGameplayStatics::ApplyDamage(
		this,
		DamageToApply,
		InstigatorController,
		DamageCauser,
		DamageType
	);

	HelicopterDamageEffects(InHitResult.ImpactPoint);
	if (InstigatorShooter) InstigatorShooter->ClientOnProjectileHit(GetActorLocation(), DamageToApply, false, false);
}

void AHelicopter::OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter->GetEquippedWeapon() == nullptr)) return;
	float DamageAmount;
	TMap<AHelicopter*, float> HitMapHelicopter;
	for (const FHitResult& WeaponTraceHit : InHitResults)
	{
		if (WeaponTraceHit.GetActor() == this)
		{
			DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(WeaponTraceHit.BoneName.ToString());
			if (HitMapHelicopter.Contains(this)) HitMapHelicopter[this] += DamageAmount;
			else HitMapHelicopter.Emplace(this, DamageAmount);
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
		if (HasAuthority())
		{
			this->HelicopterDamageEffects(WeaponTraceHit.ImpactPoint);
			//HitHelicopter->HelicopterHitPoint = FVector_NetQuantize(0.f, 0.f, 0.f);
		}
	}

	for (const TPair<AHelicopter*, float>& HitPair : HitMapHelicopter)
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
			if (HitPair.Key->OwnedShooter)
			{
				auto VictimPS = Cast<AShooterPlayerState>(HitPair.Key->GetPlayerState());
				if (InstigatorShooter->IsLocallyControlled() && VictimPS && InstigatorPS && VictimPS->GetGenericTeamId() != InstigatorPS->GetGenericTeamId())
				{
					InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
				}
			}
			else
			{
				if (InstigatorShooter->IsLocallyControlled())
				{
					InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false);
				}
			}
		}
	}
}

void AHelicopter::OnMatchEnded_Implementation(AShooterPlayerController* ShooterController)
{
	OnTakeAnyDamage.RemoveDynamic(this, &AHelicopter::ReceiveDamage);
	FloatingComponent->Deactivate();
	if (ShooterController)
	{
		DisableInput(ShooterController);
		ShooterController->RemoveHeliHUDElements();
	}
}

void AHelicopter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bAiming);
	DOREPLIFETIME(ThisClass, FloatingComponent);
	DOREPLIFETIME(ThisClass, HelicopterState);
	DOREPLIFETIME(ThisClass, Health);
	DOREPLIFETIME(ThisClass, CombatState);
	DOREPLIFETIME(ThisClass, EquippedWeapon);
	DOREPLIFETIME(ThisClass, Inventory);
	DOREPLIFETIME(ThisClass, OwnedShooter);
	DOREPLIFETIME(ThisClass, bHeliDoorOpen);
	DOREPLIFETIME(ThisClass, HelicopterSpeed);
	DOREPLIFETIME(ThisClass, HelicopterHitPoint1);
	DOREPLIFETIME(ThisClass, HelicopterHitPoint2);
	DOREPLIFETIME(ThisClass, HelicopterHitPoint3);
	DOREPLIFETIME(ThisClass, TargetMovementSpeed);
	//DOREPLIFETIME(ThisClass, bHitEffect1);
}

