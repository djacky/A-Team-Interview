// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "ShooterSpringArmComp.h"
#include "ShooterGameState.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMeshSocket.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "ShooterCrosshairHUD.h"
#include "Item.h"
#include "Ammo.h"
#include "BoostItem.h"
#include "BoostComponent.h"
#include "Blockchain/Blockchain.h"
#include "ShooterAnimInstance.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/Items/Weapons/WeaponNFT.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"
#include <cstring>  
#include "Shooter/Drone/ItemContainer.h"
#include "Shooter/Items/Weapons/Projectile.h"
#include "GameMode/ShooterGameMode.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "ShooterSpectatorPawn.h"
#include "Shooter/EnumTypes/MatchState.h"
#include "ShooterPlayerState.h"
#include "ShooterCharacterAI.h"
#include "Helicopter.h"
#include "Camera/CameraShakeBase.h"
#include "Shooter/ShooterGameInstance.h"
#include "MusicManager.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Shooter/AI/ShooterAI.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Globals.h"
#include "Shooter/Widgets/PlayerNameWidget.h"
#include "Shooter/Misc/Interfaces/ItemInterface.h"
#include "Shooter/Items/DummyItem.h"
#include "JsonObjectConverter.h"
#include "WebSocketsModule.h"
#include "AIController.h"
#include "Shooter/Misc/CharacterGlobalFuncs.h"
#include "ShooterMovementComponent.h"
#include "Shooter/Items/GrappleItem.h"
#include "Shooter/Items/Weapons/ScopeAttachment.h"
#include "Shooter/StructTypes/UnlockTypes.h"
#include "Shooter/Items/AllDamageTypes.h"
//#include "Camera/CameraActor.h"
//#include "NiagaraDataInterfaceColorCurve.h"
//#include "aws/gamelift/GameLiftClient.h"
//#include "Animation/AnimMontage.h"
//#include "CableComponent.h"

//#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2
#define ECC_ItemContainer ECollisionChannel::ECC_GameTraceChannel3

// Sets default values
AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	// Create a camera boom (pulls in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<UShooterSpringArmComp>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 250.f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 45.0f, 70.0f);

	//CameraBoom->bEnableCameraLagAxisX = false;
	//CameraBoom->bEnableCameraLagAxisY = false;
	//CameraBoom->bEnableCameraLagAxisZ = true;

	/*
	// Create a camera boom for minimap
	MiniCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("MiniCameraBoom"));
	MiniCameraBoom->SetupAttachment(GetCapsuleComponent());
	MiniSceneComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MiniSceneComp"));
	MiniSceneComp->SetupAttachment(MiniCameraBoom);
	*/

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach camera to end of boom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	FPSCameraPivot = CreateDefaultSubobject<USceneComponent>(TEXT("FPSCameraPivot"));
	FPSCameraPivot->SetupAttachment(GetMesh());
	FPSCameraPivot->SetRelativeLocation(FVector(0.f, 0.f, 80.f)); // Example: Rough head height; tune based on your mesh

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetMesh(), FName("head"));  // Adjust socket name as needed
	FirstPersonCamera->bUsePawnControlRotation = true;  // Sync with player input
	FirstPersonCamera->SetActive(false);  // Default to inactive
	FirstPersonCamera->SetHiddenInGame(true);

	// Optional: First-person arms for weapon handling visuals
	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetOnlyOwnerSee(true);  // Visible only to owner
	FirstPersonArms->SetHiddenInGame(true);  // Hidden by default
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlayerNameWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PlayerNameWidget"));
	PlayerNameWidget->SetupAttachment(GetRootComponent());
	PlayerNameWidget->SetOwnerNoSee(true);
	PlayerNameWidget->SetWidgetSpace(EWidgetSpace::Screen);

	// Create Hand Scene Component
	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));
	SwingComponentH = CreateDefaultSubobject<USceneComponent>(TEXT("SwingComponentH"));
	SwingComponentH->SetupAttachment(RootComponent);
	SwingComponentL = CreateDefaultSubobject<USceneComponent>(TEXT("SwingComponentL"));
	SwingComponentL->SetupAttachment(RootComponent);
	SwingComponentR = CreateDefaultSubobject<USceneComponent>(TEXT("SwingComponentR"));
	SwingComponentR->SetupAttachment(RootComponent);
	SwingComponentLo = CreateDefaultSubobject<USceneComponent>(TEXT("SwingComponentLo"));
	SwingComponentLo->SetupAttachment(RootComponent);

	Boost = CreateDefaultSubobject<UBoostComponent>(TEXT("BoostComponent"));
	Boost->SetIsReplicated(true);

	Blockchain = CreateDefaultSubobject<UBlockchain>(TEXT("BlockchainComponent"));
	//Blockchain->SetIsReplicated(true);

	ShieldLocation = CreateDefaultSubobject<USceneComponent>(TEXT("ShieldLocation"));
	ShieldLocation->SetupAttachment(GetCapsuleComponent());

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	ShieldMesh->SetupAttachment(GetCapsuleComponent());
	//ShieldMesh->SetCollisionObjectType(ECC_Shield);
	ShieldMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ShieldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
	ShieldMesh->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block);
	ShieldMesh->SetNotifyRigidBodyCollision(true);
	ShieldMesh->SetVisibility(false);
	ShieldMesh->SetIsReplicated(true);

	RelevancySphere = CreateDefaultSubobject<USphereComponent>(TEXT("RelevancySphere"));
	RelevancySphere->SetupAttachment(GetMesh(), FName("Root"));
	RelevancySphere->SetSphereRadius(1000.f);

	FallingSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FallingSound"));
	FallingSoundComponent->SetupAttachment(GetMesh());
	FallingSoundComponent->SetAutoActivate(false);
	FallingSoundComponent->Deactivate();

	GenBoostSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BoostSound"));
	GenBoostSoundComponent->SetupAttachment(GetMesh());
	//GenBoostSoundComponent->SetAutoActivate(false);
	//GenBoostSoundComponent->Deactivate();

	MissileDamageEffectComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MissileDamageEffectComp"));
	MissileDamageEffectComp->SetupAttachment(GetMesh());
	MissileDamageEffectComp->SetAutoActivate(false);
	MissileDamageEffectComp->Deactivate();

	CyberBootEffectComponentLeft = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CyberBootEffectLeft"));
	CyberBootEffectComponentLeft->SetupAttachment(GetMesh(), FName("foot_l_effect"));
	CyberBootEffectComponentLeft->SetAutoActivate(false);
	CyberBootEffectComponentLeft->Deactivate();

	CyberBootEffectComponentRight = CreateDefaultSubobject<UNiagaraComponent>(TEXT("CyberBootEffectRight"));
	CyberBootEffectComponentRight->SetupAttachment(GetMesh(), FName("foot_r_effect"));
	CyberBootEffectComponentRight->SetAutoActivate(false);
	CyberBootEffectComponentRight->Deactivate();

    GrappleLandEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GrappleLandEffect"));
    GrappleLandEffect->SetupAttachment(RootComponent);
    //GrappleLandEffect->SetIsReplicated(true);
    GrappleLandEffect->Deactivate();
    GrappleLandEffect->bAutoActivate = false;


	GenBoostEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GeneralBoostEffect"));
	GenBoostEffect->SetupAttachment(GetMesh());
	GenBoostEffect->SetAutoActivate(false);
	GenBoostEffect->Deactivate();

	//GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("Cable"));
	//GrappleCable->SetupAttachment(GetCapsuleComponent());
	//GrappleCable->SetVisibility(false);

	// Don't rotate when the controller rotates. Let the controller only affect the camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; // we should rotate with the controller
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = BaseJumpVelocity;
	GetCharacterMovement()->AirControl = BaseAirControl;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchMovementSpeed;

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponInterpolationComponent"));
	WeaponInterpComp->SetupAttachment(FollowCamera);

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComp1->SetupAttachment(FollowCamera);

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComp2->SetupAttachment(FollowCamera);

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComp3->SetupAttachment(FollowCamera);

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComp4->SetupAttachment(FollowCamera);

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComp5->SetupAttachment(FollowCamera);

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComp6->SetupAttachment(FollowCamera);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Overlap);
	//GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Ignore);
	//GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

    SlowNiagaraEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SlowNiagaraEffect"));
    SlowNiagaraEffectComponent->SetupAttachment(GetRootComponent());
    SlowNiagaraEffectComponent->SetAutoActivate(false);

	/*
	hand_l = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_l"));
	hand_l->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("hand_l"), hand_l);
	hand_l->SetWorldScale3D({ 0.345, 0.1975, 0.1975 });

	lowerarm_l = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_l"));
	lowerarm_l->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("lowerarm_l"), lowerarm_l);
	lowerarm_l->SetWorldScale3D({ 0.345, 0.1975, 0.1975 });

	hand_r = CreateDefaultSubobject<UBoxComponent>(TEXT("hand_r"));
	hand_r->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("hand_r"), hand_r);
	hand_r->SetWorldScale3D({ 0.345, 0.1975, 0.1975 });

	lowerarm_r = CreateDefaultSubobject<UBoxComponent>(TEXT("lowerarm_r"));
	lowerarm_r->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("lowerarm_r"), lowerarm_r);
	lowerarm_r->SetWorldScale3D({ 0.345, 0.1975, 0.1975 });

	foot_r = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_r"));
	foot_r->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("foot_r"), foot_r);
	foot_r->SetWorldScale3D({ 0.345, 0.255, 0.3425 });
	foot_r->SetWorldLocation({ 0.f, 0.f, 10.f });

	foot_l = CreateDefaultSubobject<UBoxComponent>(TEXT("foot_l"));
	foot_l->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("foot_l"), foot_l);
	foot_l->SetWorldScale3D({ 0.345, 0.255, 0.3425 });
	foot_l->SetWorldLocation({ 0.f, 0.f, 10.f });

	calf_r = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_r"));
	calf_r->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("calf_r"), calf_r);
	calf_r->SetWorldScale3D({ 0.78, 0.1975, 0.1975 });

	calf_l = CreateDefaultSubobject<UBoxComponent>(TEXT("calf_l"));
	calf_l->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("calf_l"), calf_l);
	calf_l->SetWorldScale3D({ 0.78, 0.1975, 0.1975 });


	//spine_01 = CreateDefaultSubobject<UBoxComponent>(TEXT("spine_01"));
	//spine_01->SetupAttachment(GetMesh(), FName("spine_01"));
	//spine_01->SetWorldScale3D({ 2.2, 1.005, 0.7675 });
	//spine_01->ComponentTags = { "BodyTag" };
	//spine_01->SetCollisionObjectType(ECC_WorldDynamic);
	//spine_01->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Overlap);


	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			//Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
			Box.Value->SetCollisionResponseToChannel(ECC_ItemContainer, ECollisionResponse::ECR_Overlap);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
	*/

	SlowSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SlowCharactersSphere"));
	SlowSphere->SetupAttachment(GetMesh(), FName("spine_01"));
	SlowSphere->SetSphereRadius(1500.f);
	SlowSphere->SetComponentTickInterval(0.25f);
	SlowSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlowSphere->SetGenerateOverlapEvents(false);
	SlowSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SlowSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
	FlyingTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FlyingTimelineComponent"));
	SpawnInTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpawnInTimelineComponent"));
}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	OnActorHit.AddDynamic(this, &AShooterCharacter::OnShooterHit);

	//CyberBootEffectComponentRight->SetRelativeRotation(FRotator(180.f,810.f,810.f));
	//CyberBootEffectComponentRight->SetRelativeLocation(FVector(10.f, 0.f, 0.f));
	//CyberBootEffectComponentLeft->SetRelativeRotation(FRotator(0.f,450.f,-450.f));
	//CyberBootEffectComponentLeft->SetRelativeLocation(FVector(-10.f, 0.f, 0.f));

	//Blockchain->PostReq();

	//LOG_SHOOTER_NORMAL(FString::Printf(TEXT("TEST LOG BABY.")));
	
	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	// Spawn the default weapon and equip it on server
	if (HasAuthority())
	{
		ReplenishShieldStrength();
		ReplenishChargeAttackStrength();
		GetWorldTimerManager().SetTimer(HoverSystemReplenishTimer, this, &AShooterCharacter::ReplenishHoverSystem, 0.5f, true);
		InitialEquipWeapon(SpawnForDefaultWeapon());
		OnTakeAnyDamage.AddDynamic(this, &AShooterCharacter::ReceiveDamage);
		//RelevancySphere->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnRelevancyOverlap);
		//RelevancySphere->OnComponentEndOverlap.AddDynamic(this, &AShooterCharacter::OnRelevancyEndOverlap);

		GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AShooterCharacter::OnCapsuleHit);
		if (IsPracticeMode() && PlayerNameWidget) PlayerNameWidget->DestroyComponent();
	}
	ItemMontageFinishedDelegate.BindUObject(this, &AShooterCharacter::OnMontageFinished);
	FireingMontageFinishedDelegate.BindUObject(this, &AShooterCharacter::OnFireMontageFinished);
	// Initialized with values that are similar on clients and server, can be called on all machines
	//InitialzeAmmoMap();

	ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	// Create FInterpLocation structs for each interp location Add to array
	InitializeInterpLocations();

	SlowSphere->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnSlowDownOverlap);
	SlowSphere->OnComponentEndOverlap.AddDynamic(this, &AShooterCharacter::OnSlowDownEndOverlap);

	/*
	if (HasAuthority())
	{
		hand_r->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		hand_l->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		lowerarm_r->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		lowerarm_l->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		foot_r->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		foot_l->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		calf_r->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
		calf_l->OnComponentBeginOverlap.AddDynamic(this, &AShooterCharacter::OnHandOverlap);
	}
	*/

}

void AShooterCharacter::SetOnPossess()
{
	//if (GetWorld()) AShooterGameState* ShooterGS = GetWorld()->GetGameState<AShooterGameState>();
	ShieldOn(false);
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS)
	{
		if (HasAuthority() && ShooterGS->bIsTeamMode)
		{
			SetReplicateMovement(true);
			GetCharacterMovement()->SetIsReplicated(true);
		}		
	}
}

void AShooterCharacter::SetPlayerNameTag()
{
	if (HasAuthority()) return;
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (ShooterGI && ShooterGI->bTeamMode && !ShooterGI->bPracticeMode && !bIsAI)
	{
		FTimerHandle SetPlayerNameTagTimer;
		ShooterPS = GetShooter_PS();
		if ((ShooterPS == nullptr) || 
			(ShooterPS && !ShooterPS->bGotTeamID))
		{
			GetWorldTimerManager().SetTimer(SetPlayerNameTagTimer, this, &AShooterCharacter::SetPlayerNameTag, 0.25f);
			return;
		}

		if (PlayerNameWidget) ServerSetPlayerNameTag(ShooterPS->GetPlayerName());
	}
	else if (ShooterGI && !ShooterGI->bTeamMode)
	{
		PlayerNameWidget->DestroyComponent();
		//if (PlayerNameWidget->GetWidget()) PlayerNameWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AShooterCharacter::ServerSetPlayerNameTag_Implementation(const FString &InPlayerName)
{
	PlayerNameTag = InPlayerName;
}

void AShooterCharacter::OnRep_PlayerNameTag()
{
	if (PlayerNameWidget == nullptr || (PlayerNameWidget && PlayerNameWidget->GetWidget() == nullptr))
	{
		FTimerHandle PlayerNameTagTimer;
		GetWorldTimerManager().SetTimer(PlayerNameTagTimer, this, &AShooterCharacter::OnRep_PlayerNameTag, 0.25f);
		return;
	}

	auto PlayerWidget = Cast<UPlayerNameWidget>(PlayerNameWidget->GetWidget());
    if (PlayerWidget)
    {
        // Check if the player is a teammate
		if (IsTeamMate())
		{
			//LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Setting player name %s on team member widget"), *PlayerWidget->GetOwningPlayerPawn()->GetName()));
			PlayerWidget->SetPlayerName(PlayerNameTag);
			GetPlayerNameWidget()->GetWidget()->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			GetPlayerNameWidget()->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
		}
    }
}

bool AShooterCharacter::IsTeamMate()
{
    // Get the local player's TeamID
    const APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
    if (LocalPlayerController)
    {
        AShooterPlayerState* LocalPS = Cast<AShooterPlayerState>(LocalPlayerController->PlayerState);
		AShooterPlayerState* OwnerPS = GetShooter_PS();
        if (LocalPS && OwnerPS && LocalPS->bGotTeamID && OwnerPS->bGotTeamID && LocalPS != OwnerPS)
        {
            if (LocalPS->GetGenericTeamId() == OwnerPS->GetGenericTeamId()) return true;
        }
		else if ((LocalPS && !LocalPS->bGotTeamID) || (OwnerPS && !OwnerPS->bGotTeamID) || LocalPS == nullptr || OwnerPS == nullptr)
		{
			FTimerHandle IsTeamMateTimer;
			GetWorldTimerManager().SetTimer(IsTeamMateTimer, this, &AShooterCharacter::OnRep_PlayerNameTag, 0.5f);
		}
    }
	else
	{
		FTimerHandle IsTeamMateTimer;
		GetWorldTimerManager().SetTimer(IsTeamMateTimer, this, &AShooterCharacter::OnRep_PlayerNameTag, 0.5f);
	}
    return false;
}

void AShooterCharacter::OnShooterHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
    if (HoverState == EHoverState::EHS_HoverRush)
    {
        float NormPitch = UKismetMathLibrary::NormalizeToRange(GetControlRotation().Pitch, 270.f, 360.f);
        if (UKismetMathLibrary::InRange_FloatFloat(NormPitch, 0.f, 0.7f))
        {
            bLanded = true; // Still set locally for client-side consistency, though server will override for damage check
            
            if (IsLocallyControlled())
            {
				bUseControllerRotationYaw = false;
                // Local prediction: Set state and properties immediately for responsiveness
                EHoverState OldHoverState = HoverState;
                HoverState = EHoverState::EHS_HoverStop;
				//SetMovementState(EMovementModifiers::EMM_HoverStop);
                SetHoverProperties(); // Switches to MOVE_Falling locally
                bDisableGameplay = true;
                bFireButtonPressed = false;
				bHoverButtonPressed = false;
				FTimerHandle LocalLandHandle;
				GetWorldTimerManager().SetTimer(LocalLandHandle, this, &AShooterCharacter::SetYawAfterLanding, 1.45f);
                
                if (!HasAuthority())
				{
					PlayLandingMontage();
					OnRep_HoverState(OldHoverState);
				}
                ServerLand();
            }
        }
    }
}

void AShooterCharacter::SetYawAfterLanding()
{
	bUseControllerRotationYaw = true;
}

void AShooterCharacter::SetLandPitch(float DeltaTime)
{
	if (bLanded && IsLocallyControlled())
	{
		ShooterPlayerController = GetShooter_PC();
		if (ShooterPlayerController)
		{
			FRotator CurrentRotation = GetControlRotation();
			FRotator TargetRot = FRotator(0.f, CurrentRotation.Yaw, CurrentRotation.Roll);
			FRotator NewRot = FMath::RInterpTo(CurrentRotation, TargetRot, DeltaTime, 7.f);

			ShooterPlayerController->SetControlRotation(NewRot);

			if (FMath::IsNearlyEqual(NewRot.Pitch, 0.f, 1.f))
			{
				bLanded = false;
				//UE_LOG(LogTemp, Warning, TEXT("SetLandPitch"));
			}
		}
	}
}

void AShooterCharacter::PlayLandingMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && LandMontage)
	{
		AnimInstance->Montage_Play(LandMontage);
		AnimInstance->Montage_JumpToSection(FName("Land"));
	}
}

void AShooterCharacter::ServerLand_Implementation()
{
    // Server-authoritative state changes
	EHoverState OldHoverState = HoverState;
    HoverState = EHoverState::EHS_HoverStop;
    if (GetNetMode() == NM_ListenServer || GetNetMode() == NM_Standalone || bIsAI)
    {
        OnRep_HoverState(OldHoverState);
    }
    SetHoverProperties(); // Switches to MOVE_Falling on server
    bLanded = true; // Key: Set this on server to prevent damage in Landed
    bDisableGameplay = true;
    
    MulticastLand();
    
    FTimerHandle LandTimer;
    GetWorldTimerManager().SetTimer(LandTimer, this, &AShooterCharacter::OnLandEnded, 1.45f);

    StunPawnsAfterLand();
}

void AShooterCharacter::MulticastLand_Implementation()
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	
	PlayLandingMontage();
}

void AShooterCharacter::OnLandEnded()
{
	//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	if (HasAuthority())
	{
		bLanded = false;
		AShooterGameState* GS = GetShooter_GS();
		if (GS && GS->GameMatchState != EGameMatchState::EMS_Stop)
		{
			bDisableGameplay = false;
		}
	}
}

void AShooterCharacter::StunPawnsAfterLand()
{
	if (UWorld* World = GetWorld())
	{
		FVector CharacterLocation = GetActorLocation();

		FVector BoxHalfExtent = FVector(500.f, 500.f, 100.f); // 1000×1000×200 box
		FVector BoxCenter = CharacterLocation;
		BoxCenter.Z -= BoxHalfExtent.Z; // Center the box at feet

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn); // Detect pawns only
		ObjectQueryParams.AddObjectTypesToQuery(ECC_ItemContainer);

		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this); // Ignore self

		bool bHit = World->OverlapMultiByObjectType(
			Overlaps,
			BoxCenter,
			FQuat::Identity,
			ObjectQueryParams,
			FCollisionShape::MakeBox(BoxHalfExtent),
			QueryParams
		);

		//DrawDebugBox(World, BoxCenter, BoxHalfExtent, FColor::Green, false, 4.0f);

		if (bHit)
		{
			for (const FOverlapResult& Result : Overlaps)
			{
				AActor* HitActor = Result.GetActor();
				if (HitActor && HitActor != this)
				{
					if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitActor))
					{
						WeaponHitInterface->Execute_OnStunned(HitActor, this);
					}
					//UE_LOG(LogTemp, Warning, TEXT("Pawn found: %s"), *HitActor->GetName());
				}
			}
		}
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetLandPitch(DeltaTime);

    //float CurrentFPS = 1.0f / GetWorld()->GetDeltaSeconds();
	//UE_LOG(LogTemp, Warning, TEXT("FPS = %f"), CurrentFPS);

	//Handle interpolation for aiming
	SmoothAim(DeltaTime);

	//Change look sensitivity based on aiming
	SetLookRates();

	// Traces for items only if character is within sphere
	TraceForItems();

	InterpCapsuleHeight(DeltaTime);

	if (IsLocallyControlled())
	{
		UpdateHitNumbers();
		SetLocalRotation();
		SmoothCrouching();
		if (bHandShield)
		{
			ShieldMesh->SetWorldLocation(ShieldLocation->GetComponentLocation() + 100.f * FollowCamera->GetForwardVector());
			ShieldMesh->SetWorldRotation(FollowCamera->GetComponentRotation() + FRotator{0.f, 0.f, 90.f});
		}
		ServerUpdateShieldPosition();
		// Only called if streaming online
		if (bIsStreamingMode)
		{
			FVector2D MovementVector = StreamCalculateMovementVector();
			//FVector2D MouseVector = StreamCalculateMouseVector();
			if (MovementVector.SizeSquared() > 0)  // Only call movement if there's input
			{
				FInputActionValue InputValue(MovementVector);
				MainMovement(InputValue);
			}
			/*
			if (MouseVector.SizeSquared() > 0)
			{
				FInputActionValue MouseInputValue(MouseVector);
				Look(MouseInputValue);
			}
			*/
		}
	}
	else
	{
        // Interpolate on non-owning clients
        CurrentInterpolatedRotation = FMath::RInterpTo(
            CurrentInterpolatedRotation,
            DenormalizeRotation(BaseAimRotation),
            DeltaTime,
            RotationInterpSpeed
        );
		//UE_LOG(LogTemp, Log, TEXT("Non-owning client InterpolatedRotation: Pitch=%f, Yaw=%f, Roll=%f"), CurrentInterpolatedRotation.Pitch, CurrentInterpolatedRotation.Yaw, CurrentInterpolatedRotation.Roll);
		//UE_LOG(LogTemp, Log, TEXT("BaseAimRotation: Pitch=%f, Yaw=%f, Roll=%f"), BaseAimRotation.Pitch, BaseAimRotation.Yaw, BaseAimRotation.Roll);
	}

	StartDash();

	StartHovering();

	CameraOffsetWithDistance();

	CheckHittingInAir();

	// called when character is set to fly
	SmoothRise(DeltaTime);

	// Replenish dash charge
	ReplenishDashCharge(DeltaTime);

	StopHittingWhenShieldOn();

	// Fix camera for winner[s]
	WinnerFixCamera(DeltaTime);

	// Pull character towards gravity projectile
	PullToGravityProjectile();

	//UE_LOG(LogTemp, Warning, TEXT("WeaponHasAmmo: %i"), WeaponHasAmmo());

	// Pull character towards grapple boost item
	//StartGrapplePull();
	StartGrappleHook();

	RunChargeAttack(DeltaTime);

	SmoothCameraToAIWinner(DeltaTime);

	// if(GetMovementComponent()->IsCrouching() != bCrouching)
	// bCrouching = bIsCrouched;

	// Another way to auto collect the ammo
	// AutoCollectAmmo(); 
	//UE_LOG(LogTemp, Warning, TEXT("Hit: %i"), FireCharacterHit);
	//UE_LOG(LogTemp, Warning, TEXT("Miss: %i"), FireCharacterMiss);

}

void AShooterCharacter::SetLocalRotation()
{
        FRotator CurrentBaseAimRot = GetBaseAimRotation();
        CurrentBaseAimRot = DenormalizeRotation(CurrentBaseAimRot); // Denormalize before comparison
        bool IsRotating = !CurrentBaseAimRot.Equals(LastRotationInput, 0.1f);

        if (bWasRotating && !IsRotating)
        {
            // Force final update when rotation stops
            if (HasAuthority())
            {
                BaseAimRotation = CurrentBaseAimRot;
                OnRepBaseAimRotation();
            }
            else
            {
                ServerSetBaseAimRotation(CurrentBaseAimRot);
            }
            LastSentBaseAim = CurrentBaseAimRot;
        }
        else if (!CurrentBaseAimRot.Equals(LastSentBaseAim, RotationUpdateThreshold))
        {
            // Regular threshold-based update
            if (HasAuthority())
            {
                BaseAimRotation = CurrentBaseAimRot;
                OnRepBaseAimRotation();
            }
            else
            {
                ServerSetBaseAimRotation(CurrentBaseAimRot);
            }
            LastSentBaseAim = CurrentBaseAimRot;
        }

        CurrentInterpolatedRotation = CurrentBaseAimRot; // Owning client uses authoritative rotation
        LastRotationInput = CurrentBaseAimRot;
        bWasRotating = IsRotating;
}

void AShooterCharacter::ServerSetBaseAimRotation_Implementation(const FRotator& NewRotation)
{
    BaseAimRotation = NewRotation;
}

void AShooterCharacter::OnRepBaseAimRotation()
{
    // Ensure denormalized rotation for consistency
    BaseAimRotation = DenormalizeRotation(BaseAimRotation);
}

FRotator AShooterCharacter::DenormalizeRotation(const FRotator& Rotation) const
{
    return FRotator(
        FMath::UnwindDegrees(Rotation.Pitch),
        FMath::UnwindDegrees(Rotation.Yaw),
        0.f  // Force roll to 0 if not needed
    );
}

FVector2D AShooterCharacter::StreamCalculateMovementVector() const
{
    FVector2D MovementVector(0.0f, 0.0f);

    // Forward/Backward movement (W/S keys)
    if (StreamKeyStates.Contains("w") && StreamKeyStates["w"])
    {
        MovementVector.Y += 1.0f; // Move forward
    }
    if (StreamKeyStates.Contains("s") && StreamKeyStates["s"])
    {
        MovementVector.Y -= 1.0f; // Move backward
    }

    // Right/Left movement (D/A keys)
    if (StreamKeyStates.Contains("d") && StreamKeyStates["d"])
    {
        MovementVector.X += 1.0f; // Move right
    }
    if (StreamKeyStates.Contains("a") && StreamKeyStates["a"])
    {
        MovementVector.X -= 1.0f; // Move left
    }

    return MovementVector;  // Return combined movement vector
}

FVector2D AShooterCharacter::StreamCalculateMouseVector() const
{
    FVector2D MouseVector(0.0f, 0.0f);

    // Forward/Backward movement (W/S keys)
    if (StreamKeyStates.Contains("ArrowRight") && StreamKeyStates["ArrowRight"])
    {
        MouseVector.X += 0.3f;
    }
    if (StreamKeyStates.Contains("ArrowLeft") && StreamKeyStates["ArrowLeft"])
    {
        MouseVector.X -= 0.3f;
    }

    // Right/Left movement (D/A keys)
    if (StreamKeyStates.Contains("ArrowDown") && StreamKeyStates["ArrowDown"])
    {
        MouseVector.Y += 0.3f;
    }
    if (StreamKeyStates.Contains("ArrowUp") && StreamKeyStates["ArrowUp"])
    {
        MouseVector.Y -= 0.3f;
    }

    return MouseVector;  // Return combined movement vector
}

void AShooterCharacter::OnRelevancyOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	
	auto ShooterChar = Cast<AShooterCharacter>(OtherActor);
	if (ShooterChar && ShooterChar != this)
	{
		RelevantShooters.AddUnique(ShooterChar);
	}
}

void AShooterCharacter::OnRelevancyEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	auto ShooterChar = Cast<AShooterCharacter>(OtherActor);
	if (ShooterChar && ShooterChar != this)
	{
		if (RelevantShooters.Contains(ShooterChar))
		{
			RelevantShooters.Remove(ShooterChar);
		}
	}
}

void AShooterCharacter::OnRep_RelevantShooters()
{
    if (IsLocallyControlled())
    {
		TArray<AActor*> AllCharacters;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->GetClass(), AllCharacters);

		for (int32 i = 0; i < AllCharacters.Num(); i++)
		{
			auto ShooterChar = Cast<AShooterCharacter>(AllCharacters[i]);
			// If player get's elimmed, don't tigger relevancy sphere
			if (ShooterChar)
			{
				if (ShooterChar->bPlayerEliminated) return;
			}
			if (ShooterChar && RelevantShooters.Contains(ShooterChar))
			{
				auto OverlappedChar = RelevantShooters[RelevantShooters.Find(ShooterChar)];
				if (OverlappedChar && OverlappedChar != this)
				{
					DisableActor(false, OverlappedChar);
				}
			}
			else
			{
				if (ShooterChar && ShooterChar != this)
				{
					DisableActor(true, ShooterChar);
				}
			}
		}
    }
}

void AShooterCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) 
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (auto MoveComp = GetCharacterMovement())
	{
		//UE_LOG(LogTemp, Warning, TEXT("MovementMove: %s"), *UEnum::GetValueAsString(GetCharacterMovement()->MovementMode));
		switch (MoveComp->MovementMode)
		{
		case EMovementMode::MOVE_Falling:
			{
				if (IsLocallyControlled() && bCrouching)
				{
					TriggerCrouch();
				}
				if (HasAuthority())
				{
					if (!CharacterStateTags.HasTag(FGameplayTag::RequestGameplayTag("State.Character.InAir")))
					{
						CharacterStateTags.AddTag(FGameplayTag::RequestGameplayTag("State.Character.InAir"));
					}
					
					if (GetVelocity().Z < 0 && !bStartFalling)
					{
						bStartFalling = true;
						StartFallTime = GetWorld()->GetTimeSeconds();
					}
					//UE_LOG(LogTemp, Warning, TEXT("Falling!"));
				}

				if (GetVelocity().Z < -1000.f && FallingSound)
				{
					if (FallingSoundComponent)
					{
						if (!FallingSoundComponent->IsActive()) FallingSoundComponent->Activate();
						FallingSoundComponent->SetSound(FallingSound);
						FallingSoundComponent->Play();
					}
				}
				else
				{
					FallingDelegate.BindUFunction(this, FName("CheckVelocity"), MoveComp->MovementMode, MoveComp->CustomMovementMode);
					GetWorld()->GetTimerManager().SetTimer(FallingTimer, FallingDelegate, 1e-2f, false);
				}	
				break;
			}
		case EMovementMode::MOVE_Walking:
			{
				if (HasAuthority())
				{
					if (CharacterStateTags.HasTag(FGameplayTag::RequestGameplayTag("State.Character.InAir")))
					{
						CharacterStateTags.RemoveTag(FGameplayTag::RequestGameplayTag("State.Character.InAir"));
					}
				}
				break;
			}

		case EMovementMode::MOVE_Flying:
			{
				if (IsLocallyControlled() && bCrouching)
				{
					TriggerCrouch();
				}
				if (HasAuthority())
				{
					if (CharacterStateTags.HasTag(FGameplayTag::RequestGameplayTag("State.Character.InAir")))
					{
						CharacterStateTags.RemoveTag(FGameplayTag::RequestGameplayTag("State.Character.InAir"));
					}
				}
				//StartFallTime = GetWorld()->GetTimeSeconds();
				//break;
			}
		default:
			break;
		}

		if (MoveComp->MovementMode == EMovementMode::MOVE_Walking)
		{
			if (HoverState <= EHoverState::EHS_HoverStop) 
			{
				HoverState = EHoverState::EHS_HoverFinish;
			}
		}

		if (FallingSound && FallingSoundComponent && FallingSoundComponent->IsPlaying() &&
			MoveComp->MovementMode != EMovementMode::MOVE_Falling && PrevMovementMode == EMovementMode::MOVE_Falling &&
			(HoverState != EHoverState::EHS_HoverStart))
		{
			FallingSoundComponent->FadeOut(0.5f, 0.f);
		}
	}
}

void AShooterCharacter::CheckVelocity(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
}

void AShooterCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (HasAuthority() && !bIsAI)
	{
		bStartFalling = false;
		FallingVelocity = GetCharacterMovement()->Velocity.Z / 100.f;
		if (GetCharacterMovement()->IsFalling())
		{
			//UE_LOG(LogTemp, Warning, TEXT("FallingVelocity=%f"), FallingVelocity);
			FallingTime = GetWorld()->GetTimeSeconds() - StartFallTime;
			if (FallingTime > 1.5f && (!bSuperJump && !bStopFlying && !bSuperPunch) && FallingVelocity < -15.f && !bLanded && !bDisableGameplay)
			{
				UGameplayStatics::ApplyDamage(
					this, // Damaged Character
					(0.075f) * FMath::Pow(FMath::Abs(FallingVelocity), 2), // Set damage
					GetController(),
					this,
					UDamageType::StaticClass()
				);
			}

			if (bSuperJump)
			{
				bSuperJump = false;
				OnRep_SuperJump();
			}
			if (bStopFlying)
			{
				bStopFlying = false;
			}
			if (bSuperPunch)
			{
				auto BoostItem = Cast<ABoostItem>(EquippedItem);
				if (BoostItem && BoostItem->GetBoostType() == EBoostType::EBT_SuperPunch)
				{
					FString NameStr = BoostItem->GetItemMontageSection().ToString(); // Convert FName to FString
					TArray<FString> PunchArray;
					NameStr.ParseIntoArray(PunchArray, TEXT("/"), true);
					ServerEndSuperPunch(FName(*PunchArray[1]), BoostItem);
				}
			}
		}
	}
	
	if (IsLocallyControlled() && bSuperPunch)
	{
		SetMovementState(EMovementModifiers::EMM_Stop);
		LocalPlaySuperPunch();
	}

	if (IsLocallyControlled() && !IsGrappling() && (HoverState != EHoverState::EHS_HoverStart && HoverState != EHoverState::EHS_HoverRush))
	{
		if (ShooterMovementComponent && (ShooterMovementComponent->WantsState == EMovementModifiers::EMM_Crouch || ShooterMovementComponent->WantsState == EMovementModifiers::EMM_Normal))
		{
			EMovementModifiers DesiredState = bAiming ? EMovementModifiers::EMM_Crouch : EMovementModifiers::EMM_Normal;
			if (ShooterMovementComponent->WantsState != DesiredState)
			{
				SetMovementState(DesiredState);
			}
		}
	}
}

void AShooterCharacter::LocalPlaySuperPunch()
{
	auto BoostItem = Cast<ABoostItem>(EquippedItem);
	if (BoostItem && BoostItem->GetBoostType() == EBoostType::EBT_SuperPunch)
	{
		FString NameStr = BoostItem->GetItemMontageSection().ToString(); // Convert FName to FString
		TArray<FString> PunchArray;
		NameStr.ParseIntoArray(PunchArray, TEXT("/"), true);
		bUsingItem = true;
		//Set_TargetMovementSpeed(0.f);
		//GetCharacterMovement()->MaxWalkSpeed = 0.f;

		PlayItemMontage(FName(*PunchArray[1]));
		//UE_LOG(LogTemp, Warning, TEXT("LocalPlaySuperPunch"));
		//ServerEndSuperPunch(FName(*PunchArray[1]), BoostItem);
	}
}

void AShooterCharacter::ServerEndSuperPunch_Implementation(FName ItemMontageSection, ABoostItem* BoostItem)
{
	if (BoostItem && BoostItem->GetBoostType() != EBoostType::EBT_Grapple)
	{
		bUsingItem = true;
		//TargetMovementSpeed = 0.f;
		//GetCharacterMovement()->MaxWalkSpeed = 0.f;
		//Boost->OnRep_UsingItem();
		//UE_LOG(LogTemp, Warning, TEXT("ServerEndSuperPunch"));
	}
	MulticastEndSuperPunch(ItemMontageSection, BoostItem);
}

void AShooterCharacter::MulticastEndSuperPunch_Implementation(FName ItemMontageSection, ABoostItem* BoostItem)
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	if (BoostItem && BoostItem->GetBoostType() == EBoostType::EBT_SuperPunch)
	{
		PlayItemMontage(ItemMontageSection);
		//UE_LOG(LogTemp, Warning, TEXT("MulticastEndSuperPunch"));
	}
}

void AShooterCharacter::SuperPunchDamage()
{
	if (IsLocallyControlled())
	{
		ShooterPlayerController = GetShooter_PC();
		if (ShooterPlayerController)
		{
			ShooterPlayerController->CameraShake();
		}
	}
	
	if (HasAuthority() && Controller)
	{
		ShooterPS = GetShooter_PS();
		if (ShooterPS) ShooterPS->DamageType = EShooterDamageType::ESDT_Hand;
		TArray<AActor*> ActorsToIgnoreInPunch;
		ActorsToIgnoreInPunch.Add(this);
		UGameplayStatics::ApplyRadialDamageWithFalloff(
			this, // World context object
			150.f, // Base Damage
			10.f, // MinimumDamage
			GetActorLocation(), // Origin
			100.f, // DamageInnerRadius
			500.f, // DamageOuterRadius
			1.f, // DamageFalloff
			UCyberPunchType::StaticClass(), // DamageTypeClass
			ActorsToIgnoreInPunch, // IgnoreActors
			this, // DamageCauser
			Controller // InstigatorController
		);
		//auto GeometryComponent = Cast<UGeometryCollectionComponent>(WeaponTraceHit.GetComponent());
		FHitResult PunchHit;
		PunchHit.Location = GetMesh()->GetBoneLocation(FName("foot_r")) - FVector(0.f, 0.f, 20.f);
		ImplementChaos(PunchHit, EWeaponType::EWT_Sniper);
	}
}

void AShooterCharacter::DisableActor(bool toHide, AShooterCharacter* ShooterChar) 
{
	// Hides visible components
	ShooterChar->SetActorHiddenInGame(toHide);
	if (toHide)
	{
		//ShooterChar->SetActorEnableCollision(false);
		ShooterChar->SetActorTickEnabled(false);
		ShooterChar->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	}
	else
	{
		//ShooterChar->SetActorEnableCollision(true);
		ShooterChar->SetActorTickEnabled(true);
		ShooterChar->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	
	if (Inventory.Num() > 0)
	{
		//for (auto ItemInventory : Inventory)
		//{
		//	ItemInventory->SetActorHiddenInGame(toHide);
		//}
	}

}

void AShooterCharacter::ServerUpdateShieldPosition_Implementation()
{
	if (bHandShield)
	{
		ShieldMesh->SetWorldLocation(ShieldLocation->GetComponentLocation() + 100.f * FollowCamera->GetForwardVector());
		ShieldMesh->SetWorldRotation(FollowCamera->GetComponentRotation() + FRotator{0.f, 0.f, 90.f});
	}
}

void AShooterCharacter::CheckMatchState()
{
	AShooterGameState* ShooterGS = GetShooter_GS();
	check(ShooterGS);
	if (ShooterGS->GameMatchState <= EGameMatchState::EMS_Warn)
	{
		bStartFlying = true;
		//FlyingHeightTarget = GetActorLocation() + FVector{0.f, 0.f, 100.f};
		FlyingHeightTarget = GetActorLocation() + FVector{0.f, 0.f, 5000.f};
		OnRep_StartFlying();
	}
}

void AShooterCharacter::OnPossessRespawn()
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (ShooterGI && ShooterGI->GameType == EGameModeType::EGMT_Lobby) return;
	StartFlying();
	StartProtect(12.f);
}

void AShooterCharacter::StartMatchEffects()
{
	if (StartMatchEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			StartMatchEffect,
			GetMesh(),
			FName("Root"),
			FVector(0,0,0),
			FRotator(0,0,0),
			EAttachLocation::KeepRelativeOffset, 
			true
		);
	}

	if (StartMatchSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			StartMatchSound, // sound cue (USoundBase)
			GetMesh(), // mesh to attach to
			FName("Root"),   //socket name
			FVector(0,0,0),  //location relative to socket
			FRotator(0,0,0), //rotation 
			EAttachLocation::KeepRelativeOffset, 
			true //if true, will be deleted automatically
		);
	}
}

void AShooterCharacter::StopHittingWhenShieldOn()
{
	if (bHandShield)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ComboMontage && AnimInstance->Montage_IsPlaying(ComboMontage))
		{
			AnimInstance->Montage_Stop(0, ComboMontage);
		}
	}
}

void AShooterCharacter::CheckHittingInAir()
{
	if (bHittingInAir && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		bHittingInAir = false;
		//bComboHit = false;
		//SetHandCombatState(EHandCombatState::EHCS_idle, "Finish");

		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ComboMontage)
		{
			AnimInstance->Montage_Stop(0, ComboMontage);
		}
	}
	else if (bHittingInAir && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ComboMontage)
		{
			if (!AnimInstance->Montage_IsPlaying(ComboMontage))
			{
				bHittingInAir = false;
				//bComboHit = false;
				//SetHandCombatState(EHandCombatState::EHCS_idle, "Finish");
			}
		}
	}
}

void AShooterCharacter::SetComboDamageMap(TArray<int32> DamageArray)
{
	// Array of damage for each combo hit (last index is for air attack)
	for (int8 i = 0; i < ComboMontageNames.Num(); i++)
	{
		ComboDamageMap.Add(ComboMontageNames[i], DamageArray[i]);
	}
}

void AShooterCharacter::InitializeComboVariables(const FString& MeshName)
{
	TArray<FString> ShooterStringArray;
	MeshName.ParseIntoArray(ShooterStringArray, TEXT(" "));
	ShooterMeshName = ShooterStringArray.Last();
	if (HasAuthority()) SetShooterMeshName();

	LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Mesh name = %s"), *ShooterMeshName));
	//UE_LOG(LogTemp, Warning, TEXT("Mesh name = %s"), *ShooterMeshName);

	if (ShooterMeshName.Contains(TEXT("Leo")))
	{
		CharacterSelected = ECharacterProperty::ECP_Leo;
		ComboMontageNames.SetNum(4);
		ComboMontageNames = { FName("Combo1"), FName("Combo2"), FName("Combo3"), FName("AirAttack") };
		SetComboDamageMap({ 15, 10, 28, 18});

		TArray<UActorComponent*> ActorComps;
		ActorComps = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("HairTag"));
		if (ActorComps.IsValidIndex(0) && ActorComps[0])
		{
			auto HairMesh = Cast<USkeletalMeshComponent>(ActorComps[0]);
			if (HairMesh)
			{
				HairMesh->SetLeaderPoseComponent(GetMesh());
				HairMesh->SetHiddenInGame(false);
			}
		}
	}
	else if (ShooterMeshName.Contains(TEXT("Maverick")))
	{
		CharacterSelected = ECharacterProperty::ECP_Maverick;
		ComboMontageNames.SetNum(4);
		ComboMontageNames = { FName("Combo1"), FName("Combo2"), FName("Combo3"), FName("AirAttack") };
		SetComboDamageMap({ 12, 16, 25, 18});
	}
	else if (ShooterMeshName.Contains(TEXT("Dekker")) || ShooterMeshName.Contains(TEXT("Trinity")))
	{
		CharacterSelected = ECharacterProperty::ECP_Trinity;
		ComboMontageNames.SetNum(5);
		ComboMontageNames = { FName("Combo1"), FName("Combo2"), FName("Combo3"), FName("Combo4"), FName("AirAttack") };
		SetComboDamageMap({ 10, 8, 15, 20, 12});
	}
	else if (ShooterMeshName.Contains(TEXT("Phase")) || ShooterMeshName.Contains(TEXT("Raven")))
	{
		CharacterSelected = ECharacterProperty::ECP_Raven;
		ComboMontageNames.SetNum(5);
		ComboMontageNames = { FName("Combo1"), FName("Combo2"), FName("Combo3"), FName("Combo4"), FName("AirAttack") };
		SetComboDamageMap({ 15, 6, 10, 22, 12});
	}
	else
	{
		LOG_SHOOTER_WARNING(FString::Printf(TEXT("Mesh name not recognized.")));
	}
}

void AShooterCharacter::SetShooterMeshName()
{
	ShooterPS = GetShooter_PS();
	if (ShooterPS)
	{
		ShooterPS->ShooterMeshName = ShooterMeshName;
		ShooterPS->PlayerGameStats.ShooterMeshName = ShooterMeshName;
	}
	else
	{
		FTimerHandle SetShooterMeshNameTimer;
		GetWorldTimerManager().SetTimer(SetShooterMeshNameTimer, this, &AShooterCharacter::SetShooterMeshName, 0.5f);
	}
}

void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Boost)
	{
		Boost->Character = this;
	}

	if (Blockchain)
	{
		Blockchain->Character = this;
	}
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

/*
void AShooterCharacter::SmoothAim(float DeltaTime)
{
    if (!EquippedWeapon || !EquippedItem) return;

	const FADSSettings& Settings = EquippedWeapon->ADSSettings;
    bool bUseFPSADS = Settings.bUseRTTScope;  // Example: FPS ADS for non-snipers
    float TargetFOV = bAiming ? (bUseFPSADS ? 60.f : 30.f) : CameraDefaultFOV;  // Adjust FPS FOV as needed (narrower for ADS zoom)
    float InterpSpeed = 20.f;  // Shared interp speed

    if (bAiming)
    {
        if (bUseFPSADS)
        {
            // Switch to FPS mode
            GetFollowCamera()->SetActive(false);  // Deactivate third-person camera
            FirstPersonCamera->SetActive(true);
            FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFOV, DeltaTime, InterpSpeed));

            // Visibility adjustments for owner
            GetMesh()->SetOwnerNoSee(true);  // Hide body to avoid clipping (owner only)
            if (FirstPersonArms)
            {
                FirstPersonArms->SetVisibility(true);
                // Attach/reposition weapon to FP arms if needed
                //if (EquippedWeapon) EquippedWeapon->AttachToComponent(FirstPersonArms, FAttachmentTransformRules::SnapToTargetIncludingScale, FName("weapon_socket"));
            }

            // Handle reload visibility: Your reload logic should play FP animations on FirstPersonArms/Weapon
            // Example: If reloading, play anim montage on FirstPersonArms
        }
        else
        {
            // Keep third-person sniper zoom (your existing code)
            float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, AimingArmLength, DeltaTime, 10.f);
            CameraBoom->TargetArmLength = NewArmLength;

            FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, AimingSocketOffset, DeltaTime, 10.f);
            CameraBoom->SocketOffset = NewSocketOffset;

            CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, InterpSpeed);
            GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
        }
    }
    else
    {
        // Revert to third-person
        FirstPersonCamera->SetActive(false);
        GetFollowCamera()->SetActive(true);
        CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, TargetFOV, DeltaTime, InterpSpeed);
        GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);

        // Reset visibility
        GetMesh()->SetOwnerNoSee(false);
        if (FirstPersonArms) FirstPersonArms->SetVisibility(false);

        // Reset boom to default (your existing code)
        float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DefaultArmLength, DeltaTime, 10.f);
        CameraBoom->TargetArmLength = NewArmLength;

        FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DefaultSocketOffset, DeltaTime, 10.f);
        CameraBoom->SocketOffset = NewSocketOffset;

        // Reattach weapon to third-person mesh if needed
        //if (EquippedWeapon) EquippedWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("third_person_weapon_socket"));
    }

    // Handle hover/rush states as before (unchanged)
    if (HoverState == EHoverState::EHS_HoverRush)
    {
        CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraRushFOV, DeltaTime, 10.f);
        GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);  // Apply to active camera
    }
}
*/


void AShooterCharacter::SmoothAim(float DeltaTime)
{
	if (!EquippedWeapon || !EquippedItem) return;
	FADSSettings Settings = EquippedWeapon->ADSSettings;
	CameraZoomedFOV = Settings.ZoomedFOV;
	ZoomInterpSpeed = Settings.InterpSpeed;
	// Set current camera field of view

	if (bAiming)
	{
		if (UKismetMathLibrary::NearlyEqual_FloatFloat(CameraCurrentFOV, Settings.TargetArmLength, 0.05)) return;
		float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Settings.TargetArmLength, DeltaTime, 10.f);
		CameraBoom->TargetArmLength = NewArmLength;

		FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, Settings.SocketOffset, DeltaTime, 10.f);
		CameraBoom->SocketOffset = NewSocketOffset;

		// Interpolate to zoomed FOV
		//if (UKismetMathLibrary::NearlyEqual_FloatFloat(CameraCurrentFOV, CameraZoomedFOV, 0.01)) return;
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else if (HoverState == EHoverState::EHS_HoverRush)
	{
		if (UKismetMathLibrary::NearlyEqual_FloatFloat(CameraCurrentFOV, CameraRushFOV, 0.05)) return;
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraRushFOV, DeltaTime, 10.f);
	}
	else
	{
		// Interpolate to default FOV
		if (UKismetMathLibrary::NearlyEqual_FloatFloat(CameraCurrentFOV, CameraDefaultFOV, 0.05)) return;
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);

		float NewArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DefaultArmLength, DeltaTime, 10.f);
		CameraBoom->TargetArmLength = NewArmLength;

		FVector NewSocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DefaultSocketOffset, DeltaTime, 10.f);
		CameraBoom->SocketOffset = NewSocketOffset;
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}




/*
void AShooterCharacter::CameraOffsetWithDistance()
{
	if (!IsLocallyControlled()) return;
	FVector CameraLocation = GetFollowCamera()->GetComponentLocation();
	FVector CharacterLocation = GetMesh()->GetComponentLocation();
	float CameraDistance = (CharacterLocation - CameraLocation).Size();
	if (CameraDistance < 150.f)
	{
		float AimPitch = GetFollowCamera()->GetComponentRotation().Pitch;
		if (!bAiming || (bAiming && AimPitch > 40.f))
		{
			const float InitialY = 45.f;
			const float TargetY = 200.f;
			const float m1 = (InitialY - TargetY) / (150.f - 20.f);
			const float DynamicOffsetY = m1 * CameraDistance + InitialY - m1 * 150.f;

			//const float InterpCapsuleHeight = FMath::FInterpTo(, TargetCapsuleHeight, DeltaTime, 15.f);
			CameraBoom->SocketOffset = FVector(0.f, DynamicOffsetY, 70.f);
		}
	}

}
*/

void AShooterCharacter::CameraOffsetWithDistance()
{
    if (!IsLocallyControlled() || bIsAI || EquippedItem == nullptr || bDash) return;
    if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < 150.f)
    {
		bCameraDistanceOk = false;
		SetVisibilityOfShooter(false);
		if (EquippedItem->GetItemMesh())
		{
			EquippedItem->GetItemMesh()->bOwnerNoSee = true;
		}
		else if (EquippedItem->bIsAnNFT && EquippedItem->GetNFTMesh())
		{
			EquippedItem->GetNFTMesh()->bOwnerNoSee = true;
		}
    }
    else if (!bCameraDistanceOk)
    {
		bCameraDistanceOk = true;
        SetVisibilityOfShooter(true);
		for (auto InventoryItem : Inventory)
		{
			if (InventoryItem)
			{
				if (InventoryItem->GetItemMesh())
				{
					InventoryItem->GetItemMesh()->bOwnerNoSee = false;
				}
				else if (InventoryItem->bIsAnNFT && InventoryItem->GetNFTMesh())
				{
					InventoryItem->GetNFTMesh()->bOwnerNoSee = false;
				}
			}
		}
    }
}

/*
// Older function used to apply falling damage. Now using the Landed() function to do this
void AShooterCharacter::ServerFallingDamage_Implementation(float DeltaTime)
{
	if (Boost == nullptr) return;
    if (GetCharacterMovement()->IsFalling() && GetVelocity().Z < 0)
    {
        bStartFalling = true;
        FallingTime += DeltaTime; 
        FallingVelocity = GetVelocity().Z / 100.f;
    }

	if (!GetCharacterMovement()->IsFalling() && bStartFalling)
	{
		bStartFalling = false;
		
        if (FallingTime > 1.5f && (!bSuperJump && !bStopFlying))
        {
			UGameplayStatics::ApplyDamage(
				this, // Damaged Character
				(0.15f) * FMath::Pow(FallingVelocity, 2), // Set damage
				GetController(),
				this,
				UDamageType::StaticClass()
			);
        }
		
		if (bSuperJump)
		{
			bSuperJump = false;
			OnRep_SuperJump();
			//UE_LOG(LogTemp, Warning, TEXT("bSuperJump = %i"), Boost->bSuperJump);
		}
		if (bStopFlying)
		{
			bStopFlying = false;
		}
        FallingTime = 0.f;
        FallingVelocity = 0.f;
		
	}
}
*/

void AShooterCharacter::SmoothCrouching()
{
    if (GetCharacterMovement()->IsFalling() && !bCrouchLag)
    {
        bCrouchLag = true;
        // Disable Z-lag
		CameraBoom->bEnableCameraLagAxisZ = false;
    }
    if (!GetCharacterMovement()->IsFalling() && bCrouchLag)
    {
        bCrouchLag = false;
        // Enable Z-lag 
		CameraBoom->bEnableCameraLagAxisZ = true;
    }
}

void AShooterCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamID = TeamID;

	Super::PossessedBy(NewController);
	LastController = NewController;
	if (!bIsPossessed)
	{
		if (bIsAI || (NewController && NewController->IsA<AAIController>())) return;
		UpdateTeamMemberAttributes(100.f, -1000.f);
		Client_OnPossess(NewController);
		bIsPossessed = true;
		//SetSkin();
	}
}

void AShooterCharacter::Client_OnPossess_Implementation(AController* PawnController)
{
	//SetSkin();
	SetMovementState(EMovementModifiers::EMM_None);
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (!IsPracticeMode())
	{
		//ShooterPS = GetShooter_PS();
		//if (ShooterPS) ShooterPS->StartVoiceChat();
	}
	bIsPossessed = true;
	if (AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(PawnController))
	{
		PlayerController->CheckStillLoadingWidget();
		PlayerController->SetInventoryVisibility(true);
		//UE_LOG(LogTemp, Warning, TEXT("UEnhancedInputLocalPlayerSubsystem"));
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ShooterContext, 0);
		}
		if (PlayerController->PlayerState)
		{
			//StartWebSocketForStream(PlayerController->PlayerState->GetPlayerName());
		}
	}


	SetPlayerNameTag();
	if (ShooterGI)
	{
		if (!bIsAI)
		{
			bool bSaveGameSuccess = false;
			ShooterGI->LoadGame(SaveGame, bSaveGameSuccess);
			if (SaveGame)
			{
				CameraBoom->bEnableCameraLagAxisX = SaveGame->bCameraLagFront;
				CameraBoom->bEnableCameraLagAxisY = SaveGame->bCameraLagSide;
				CameraBoom->bEnableCameraLagAxisZ = SaveGame->bCameraLagUp;
				CameraBoom->CameraLagSpeed = SaveGame->CameraLagSpeed;
				MouseHipTurnRate = SaveGame->MouseTurnRate;
				MouseHipLookUpRate = SaveGame->MouseLookUpRate;
				MouseAimingTurnRate = SaveGame->MouseAimingTurnRate;
				MouseAimingLookUpRate = SaveGame->MouseAimingLookUpRate;
				GlobalZoomRatio = SaveGame->GlobalZoomRatio;
				bConstantItemSwap = SaveGame->bConstantItemSwap;
				if (!SaveGame->SelectedEmoteMontage.IsNone())
				{
					const FString UnlockTablePath(TEXT("DataTable'/Game/_Game/DataTables/UnlockData_DT.UnlockData_DT'"));
					UDataTable* UnlockTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *UnlockTablePath));
					if (UnlockTableObject)
					{
						if (FUnlockData* UnlockRow = UnlockTableObject->FindRow<FUnlockData>(SaveGame->SelectedEmoteMontage, TEXT("")))
						{
							EmoteMontage = UnlockRow->EmoteMontage.LoadSynchronous();
						}
					}
				}
				else
				{
					SetDefaultEmoteMontage();
				}
			}
		}
	}
	OnShooterPossessed();
	if (CameraBoom)
	{
		StandingCameraOffsetZ = CameraBoom->TargetOffset.Z;
		CrouchedCameraOffsetZ = StandingCameraOffsetZ - (StandingCapsuleHeight - CrouchingCapsuleHeight);
	}
}

FCharacterProperties* AShooterCharacter::GetCharacterRowProperties(UShooterAnimInstance* ShooterAnimInstance)
{
	const FString WeaponTablePath(TEXT("DataTable'/Game/_Game/DataTables/CharacterProperties_DT.CharacterProperties_DT'"));
	UDataTable* CharacterTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));
	if (CharacterTableObject && ShooterAnimInstance)
	{
		FCharacterProperties* CharacterProp = nullptr;
		switch (CharacterSelected)
		{
			case ECharacterProperty::ECP_Leo:
			CharacterProp = CharacterTableObject->FindRow<FCharacterProperties>(FName("Leo"), TEXT(""));
			ShooterAnimInstance->ComboToNotBlend = FName("Combo3");
			break;

			case ECharacterProperty::ECP_Maverick:
			CharacterProp = CharacterTableObject->FindRow<FCharacterProperties>(FName("Maverick"), TEXT(""));
			ShooterAnimInstance->ComboToNotBlend = FName("Combo3");
			break;

			case ECharacterProperty::ECP_Raven:
			CharacterProp = CharacterTableObject->FindRow<FCharacterProperties>(FName("Raven"), TEXT(""));
			ShooterAnimInstance->ComboToNotBlend = FName("Combo4");
			GetMesh()->SetWorldScale3D(FVector(1.02f));
			break;

			case ECharacterProperty::ECP_Trinity:
			CharacterProp = CharacterTableObject->FindRow<FCharacterProperties>(FName("Trinity"), TEXT(""));
			ShooterAnimInstance->ComboToNotBlend = FName("Combo4");
			break;
		}
		return CharacterProp;
	}
	return nullptr;
}

void AShooterCharacter::SetDefaultEmoteMontage()
{
	if (CharacterSelected != ECharacterProperty::ECP_MAX)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		UShooterAnimInstance* ShooterAnimInstance = nullptr;
		if (AnimInstance)
		{
			ShooterAnimInstance = Cast<UShooterAnimInstance>(AnimInstance);
			if (ShooterAnimInstance == nullptr) return;
		}
		FCharacterProperties* CharacterProp = GetCharacterRowProperties(ShooterAnimInstance);
		if (CharacterProp)
		{
			EmoteMontage = CharacterProp->EmoteMontage;
		}
	}
	else
	{
        FTimerHandle SetEmoteTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(SetEmoteTimerHandle, this, &AShooterCharacter::SetDefaultEmoteMontage, 1.f, false);
	}
}

void AShooterCharacter::SetSkin()
{
	LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Setting skin on character.")));
	auto ShooterGameInst = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this));
	//UE_LOG(LogTemp, Warning, TEXT("SetSkin: In Function"));
	if (ShooterGameInst)
	{
		//UE_LOG(LogTemp, Warning, TEXT("SetSkin: GetGameInstance"));
		TSubclassOf<AShooterCharacter> TargetCharacterClass = ShooterGameInst->GetSelectedCharacter();
		USkeletalMesh* SelectSkelMesh = ShooterGameInst->GetSelectedSkeletalMesh();

		if (TargetCharacterClass.Get() == nullptr)
		{
			// This only resolves to the defaults for when we did not select anything in the MainMenu selection screen
			SelectSkelMesh = ShooterGameInst->GetDefaultSelectedSkeletalMesh().LoadSynchronous();
			//UE_LOG(LogTemp, Warning, TEXT("SetSkin: TargetCharacterClass is not valid"));
		}

		// Just resolve the target skin override locally
		TargetSkeletalMeshOverride = SelectSkelMesh;
		InitializeOnPossess();
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_TargetSkeletalMeshOverride: In Client RPC"));
		// Tell server about it
		//ServerSetTargetSkeletalMeshOverride(SelectSkelMesh);
	}
}

void AShooterCharacter::InitializeOnPossess()
{
	if (TargetSkeletalMeshOverride && GetMesh())
	{
		//GetMesh()->SetSkeletalMeshAsset(TargetSkeletalMeshOverride);
		GetMesh()->SetSkeletalMesh(TargetSkeletalMeshOverride);
		GetMesh()->SetVisibility(true);
		//UE_LOG(LogTemp, Warning, TEXT("SetSkin: TargetCharacterClass is valid"));
		InitializeComboVariables(GetMesh()->GetReadableName());
		UpdateCharacter();
		GetAllSkeletalMeshMaterials();
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI && ShooterGI->GameType == EGameModeType::EGMT_Lobby)
		{
			InitializeLobbySpawnEffect();
		}
	}
}

// This function is not being used
void AShooterCharacter::ServerSetTargetSkeletalMeshOverride_Implementation(USkeletalMesh* InTargetSkeletalMeshOverride)
{
	//UE_LOG(LogTemp, Warning, TEXT("ServerSetTargetSkeletalMeshOverride: In Function"));
	LOG_SHOOTER_NORMAL(FString::Printf(TEXT("In Function")));
	// Server updates the replicated val for the skin
	if (InTargetSkeletalMeshOverride && GetMesh())
	{
		//UE_LOG(LogTemp, Warning, TEXT("ServerSetTargetSkeletalMeshOverride: InTargetSkeletalMeshOverride"));
		LOG_SHOOTER_NORMAL(FString::Printf(TEXT("InTargetSkeletalMeshOverride")));
		TargetSkeletalMeshOverride = InTargetSkeletalMeshOverride;
		OnRep_TargetSkeletalMeshOverride(TargetSkeletalMeshOverride);
		//GetAllSkeletalMeshMaterials();
	}
}

void AShooterCharacter::UpdateCharacter()
{
	// Path here is obtained by going to the Data Table in the folder, right click, and select "Copy Reference"
	// Path to Item Rarity Data Table
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	UShooterAnimInstance* ShooterAnimInstance = nullptr;
	if (AnimInstance)
	{
		ShooterAnimInstance = Cast<UShooterAnimInstance>(AnimInstance);
		if (ShooterAnimInstance == nullptr) return;
	}
	ShooterAnimInstance->CharacterSelected = CharacterSelected;
	FCharacterProperties* CharacterProp = GetCharacterRowProperties(ShooterAnimInstance);

	if (CharacterProp)
	{
		ChargeAttackMontage = CharacterProp->ChargeAttackMontage;
		ElimMontage = CharacterProp->ElimMontage;
		//EmoteMontage = CharacterProp->EmoteMontage;
		ComboMontage = CharacterProp->HandCombatMontage;
		WinMontage = CharacterProp->WinMontage;
		HitMontage = CharacterProp->HitMontage;
		RandomCharElimmedSound = CharacterProp->OpponentElimSound;
		if (SwingComponentH && SwingComponentL && SwingComponentR && SwingComponentLo)
		{
			SwingComponentH->SetRelativeLocation(CharacterProp->SwingHLocation);
			SwingComponentL->SetRelativeLocation(CharacterProp->SwingLLocation);
			SwingComponentR->SetRelativeLocation(CharacterProp->SwingRLocation);
			SwingComponentLo->SetRelativeLocation(CharacterProp->SwingLoLocation);
		}
	}
}

void AShooterCharacter::UnPossessed()
{
	AController* const OldController = Controller;

	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamID = TeamID;
	if (IShooterTeamAgentInterface* ControllerAsTeamProvider = Cast<IShooterTeamAgentInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	Super::UnPossessed();

	// Determine what the new team ID should be afterwards
	TeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
}

void AShooterCharacter::StartWebSocketForStream(const FString& PlayerName)
{
	if (PlayerName.Equals(TEXT("ackyshacky")))
	{
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI)
		{
			bIsStreamingMode = true;
			ShooterGI->OnWebSocketMessageReceived.AddDynamic(this, &AShooterCharacter::SetStreamActions);
		}
	}
}

void AShooterCharacter::SetStreamActions(const FString& MessageString)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MessageString);
	
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FString InputType;
		if (JsonObject->TryGetStringField(TEXT("type"), InputType))
		{
			TSharedPtr<FJsonObject> JsonDataObject = JsonObject->GetObjectField(TEXT("data"));
			if (InputType.Equals(TEXT("keyboard"))) 
			{
				FString Key = JsonDataObject->GetStringField(TEXT("key"));
				FString Type = JsonDataObject->GetStringField(TEXT("type"));
				if (Key.Equals(TEXT("w"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("s"), ESearchCase::IgnoreCase)
					|| Key.Equals(TEXT("d"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("a"), ESearchCase::IgnoreCase))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						StreamKeyStates.Add(Key, true);
					}
					else if (Type.Equals(TEXT("keyup")))
					{
						StreamKeyStates.Add(Key, false);
					}
				}
				else if (Key.Equals(TEXT("ArrowLeft"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("ArrowRight"), ESearchCase::IgnoreCase)
					|| Key.Equals(TEXT("ArrowUp"), ESearchCase::IgnoreCase) || Key.Equals(TEXT("ArrowDown"), ESearchCase::IgnoreCase))
				{
					/*
					if (Type.Equals(TEXT("keydown")))
					{
						StreamKeyStates.Add(Key, true);
					}
					else if (Type.Equals(TEXT("keyup")))
					{
						StreamKeyStates.Add(Key, false);
					}
					*/
				}
				else if (Key.Equals(TEXT(" ")))
				{
					ACharacter::Jump();
				}
				else if (Key.Equals(TEXT("r"), ESearchCase::IgnoreCase))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						ReloadButtonPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("e"), ESearchCase::IgnoreCase))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue SelectAction(true);
						SelectButtonPressed(SelectAction);
					}
				}
				else if (Key.Equals(TEXT("f")))
				{
					if (Type.Equals(TEXT("keydown"), ESearchCase::IgnoreCase))
					{
						FInputActionValue NoAction;
						FKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("1")))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						OneKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("2")))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						TwoKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("3")))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						ThreeKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("4")))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						FourKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("5")))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						FInputActionValue NoAction;
						FiveKeyPressed(NoAction);
					}
				}
				else if (Key.Equals(TEXT("shift"), ESearchCase::IgnoreCase))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						HoverButtonPressed();
					}
					else if (Type.Equals(TEXT("keyup")))
					{
						HoverButtonReleased();
					}
				}
				else if (Key.Equals(TEXT("c"), ESearchCase::IgnoreCase))
				{
					if (Type.Equals(TEXT("keydown")))
					{
						CrouchButtonPressed();
					}
				}

				// Handle keyboard input here (move character, etc.)
			}
			else if (InputType.Equals(TEXT("mouse"))) 
			{
				FString Type = JsonDataObject->GetStringField(TEXT("type"));
				if (Type.Equals(TEXT("move")))
				{
					double Xval = JsonDataObject->GetNumberField(TEXT("x"));
					double Yval = JsonDataObject->GetNumberField(TEXT("y"));
					FVector2D NormalizedMouseInput(Xval, Yval);
					FInputActionValue MouseInputValue(NormalizedMouseInput);
					Look(MouseInputValue);
				}
				else if (Type.Equals(TEXT("mousedown")))
				{
					double ButtonClick = JsonDataObject->GetNumberField(TEXT("button"));
					if (ButtonClick == 0)
					{
						FireButtonPressed();
					}
					else
					{
						AimingButtonPressed();
					}
				}
				else if (Type.Equals(TEXT("mouseup")))
				{
					double ButtonClick = JsonDataObject->GetNumberField(TEXT("button"));
					if (ButtonClick == 0)
					{
						FireButtonReleased();
					}
					else
					{
						AimingButtonReleased();
					}
				}
			}
		}
	}
}

void AShooterCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId MyOldTeamID = TeamID;
	TeamID = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, MyOldTeamID, TeamID);
}

void AShooterCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamID = TeamID;
			TeamID = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
		}
		else
		{
		}
	}
	else
	{
	}
}

FGenericTeamId AShooterCharacter::BPGetGenericTeamId() const
{
	return GetGenericTeamId();
}

FGenericTeamId AShooterCharacter::GetGenericTeamId() const
{
	return TeamID;
}

FOnShooterTeamIndexChangedDelegate* AShooterCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AShooterCharacter::OnRep_TargetSkeletalMeshOverride(USkeletalMesh* OldTargetSkeletalMeshOverride)
{
	// Once OnRep is resolved on all relevant clients, they will assign the skin accordingly
	//UE_LOG(LogTemp, Warning, TEXT("OnRep_TargetSkeletalMeshOverride: In Function"));
	LOG_SHOOTER_NORMAL(FString::Printf(TEXT("In Function")));
	InitializeOnPossess();
}

void AShooterCharacter::InitializeLobbySpawnEffect()
{
	if (TeleportMaterialInstance) DynamicTeleportMaterialInstance = UMaterialInstanceDynamic::Create(TeleportMaterialInstance, this);
	if (DynamicTeleportMaterialInstance)
	{
		SetAllSkeletalMeshMaterials(false, DynamicTeleportMaterialInstance);
	}
	StartTeleportIn();
	FTimerHandle RestoreMaterialsTimer;
	GetWorldTimerManager().SetTimer(RestoreMaterialsTimer, this, &AShooterCharacter::StartSpawnInEffects, 0.6f);
}

void AShooterCharacter::StartTeleportIn()
{
	SpawnInTrack.BindDynamic(this, &AShooterCharacter::UpdateTeleportInMaterial);
	if (SpawnInVectorCurve && SpawnInTimeline)
	{
		SpawnInTimeline->SetTimelineLength(1.2f);
		SpawnInTimeline->AddInterpVector(SpawnInVectorCurve, SpawnInTrack);
		SpawnInTimeline->PlayFromStart();
	}
}

void AShooterCharacter::UpdateTeleportInMaterial(FVector CurveValues)
{
	if (DynamicTeleportMaterialInstance)
	{
		DynamicTeleportMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), CurveValues.X);
		DynamicTeleportMaterialInstance->SetScalarParameterValue(TEXT("strength_VN"), CurveValues.Y);
		DynamicTeleportMaterialInstance->SetScalarParameterValue(TEXT("color Opacity"), CurveValues.Z);
		DynamicTeleportMaterialInstance->SetVectorParameterValue(TEXT("Color"), FVector{15.f, 0.f, 0.f});
	}
}

void AShooterCharacter::StartSpawnInEffects()
{
    if (SpawnInEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(
            SpawnInEffect,
            GetMesh(),
            FName(),
            GetActorLocation(),
            GetActorRotation(),
            FVector(1.f),
            EAttachLocation::KeepWorldPosition,
            true,
            ENCPoolMethod::None
        );
    }
	if (SpawnInSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			SpawnInSound, // sound cue (USoundBase)
			GetMesh(), // mesh to attach to
			FName("Root"),   //socket name
			FVector(0,0,0),  //location relative to socket
			FRotator(0,0,0), //rotation 
			EAttachLocation::KeepRelativeOffset, 
			true //if true, will be deleted automatically
		);
	}

	FTimerHandle RestoreMaterialsTimer;
	GetWorldTimerManager().SetTimer(
		RestoreMaterialsTimer,
		FTimerDelegate::CreateLambda([this]()
		{
			// Inline delayed code here
			// Same effect as putting code into RestoreMaterials()
			DynamicTeleportMaterialInstance = nullptr;
			SetDefaultMaterials();
			SpawnInTimeline->Deactivate();
		}),
		0.7f,
		false // not looping
	);
}

void AShooterCharacter::MainMovement(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && HoverState != EHoverState::EHS_HoverRush)
	{
		const FVector2D AxisValue = Value.Get<FVector2D>();
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		if (AxisValue.Y != 0)
		{
			const FVector ForwardDirection{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
			AddMovementInput(ForwardDirection, AxisValue.Y);
		}

		if (AxisValue.X != 0)
		{
			const FVector RightDirection{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
			AddMovementInput(RightDirection, AxisValue.X);	
		}

		/*
        if (AxisValue.Y > 0 && AxisValue.X == 0)
        {
            bIsOnlyMovingForward = true;
        }
		else
		{
			if (bIsOnlyMovingForward && bDash)
			{
				ServerSetSprinting(false);
			}
			bIsOnlyMovingForward = false;
		}
		*/
	}
}

void AShooterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::LookRate(const FInputActionValue& Value)
{
	if (GetController())
	{
		const float AxisValue = Value.Get<float>();
		AddControllerPitchInput(AxisValue * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	}
}

void AShooterCharacter::TurnRate(const FInputActionValue& Value)
{
	if (GetController())
	{
		const float AxisValue = Value.Get<float>();
		AddControllerYawInput(AxisValue * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Look(const FInputActionValue& Value)
{
	if (GetController())
	{
		const FVector2D AxisValue = Value.Get<FVector2D>();
		float TurnScaleFactor{};
		float LookUpScaleFactor{};
		if (bAiming)
		{
			TurnScaleFactor = ScaledAimingTurnRate;
			LookUpScaleFactor = ScaledAimingLookUpRate;
		}
		else
		{
			TurnScaleFactor = MouseHipTurnRate;
			LookUpScaleFactor = MouseHipLookUpRate;
		}
		if (AxisValue.X != 0) AddControllerYawInput(TurnScaleFactor * AxisValue.X);
		if (AxisValue.Y != 0) AddControllerPitchInput(LookUpScaleFactor * AxisValue.Y);
	}
}

void AShooterCharacter::Turn(float Value)
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

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(LookUpScaleFactor * Value);
}

void AShooterCharacter::SetMouseSensitivities(float FOVMult)
{
	ScaledAimingTurnRate = MouseAimingTurnRate * FOVMult * GlobalZoomRatio;
	ScaledAimingLookUpRate = MouseAimingLookUpRate * FOVMult * GlobalZoomRatio;
}

void AShooterCharacter::ScreenMessage(FString Mess)
{
	if (GEngine)
	{
		if (GetWorld()->GetNetMode() == NM_ListenServer)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Mess);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Mess);
		}
	}
}

TArray<FHitResult> AShooterCharacter::GetShotgunHits(const TArray<FVector_NetQuantize>& HitTargets)
{
	TArray<FHitResult> WeaponHits;
	if (EquippedItem == nullptr || EquippedItem->GetItemMesh() == nullptr || EquippedWeapon == nullptr) return WeaponHits;
	
	const USkeletalMeshSocket* BarrelSocket = EquippedItem->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedItem->GetItemMesh());
		for (FVector_NetQuantize ShotgunHitTarget : HitTargets)
		{
			FHitResult WeaponTraceHit;
			EquippedWeapon->ShotgunTraceHit(SocketTransform.GetLocation(), ShotgunHitTarget, WeaponTraceHit, this);
			WeaponHits.Add(WeaponTraceHit);
		}
		return WeaponHits;
	}
	return WeaponHits;
}

void AShooterCharacter::FireShotgun()
{
	if (EquippedWeapon == nullptr) return;
	FHitResult HitResult;
	FVector VectorPlaceholder;
	bool bHit = TraceUnderCrosshairs(HitResult, VectorPlaceholder);
	HitTarget = HitResult.ImpactPoint;

	TArray<FVector_NetQuantize> HitTargets;
	EquippedWeapon->ShotgunTraceEndWithScatter(HitTarget, HitTargets);

	if (!HasAuthority()) ShotgunLocalFire(GetShotgunHits(HitTargets));
	ServerShotgunFire(EquippedWeapon->AutoFireRate, GetShotgunHits(HitTargets), EquippedWeapon);
}

void AShooterCharacter::ShotgunLocalFire(const TArray<FHitResult>& WeaponTraceHits)
{
	if (CombatState == ECombatState::ECS_Reloading || CombatState == ECombatState::ECS_Unoccupied)
	{
		//bLocallyReloading = false;
		PlayGunfireMontage(FName("ShotgunFire"));
		PlayFireSound();
		EquippedWeapon->FireShotgun(this, WeaponTraceHits);
		EquippedWeapon->StartShotgunSlideTimer();
		EquippedWeapon->SpendRound();
		
		CombatState = ECombatState::ECS_Unoccupied;
		//SetCombatState(ECombatState::ECS_Unoccupied);
	}
}

void AShooterCharacter::ServerShotgunFire_Implementation(float FireDelay, const TArray<FHitResult>& WeaponTraceHits, const AWeapon* ClientWeapon)
{
	MulticastShotgunFire(WeaponTraceHits);
}

bool AShooterCharacter::ServerShotgunFire_Validate(float FireDelay, const TArray<FHitResult>& WeaponTraceHits, const AWeapon* ClientWeapon)
{
	if (EquippedWeapon && ClientWeapon && EquippedWeapon == ClientWeapon)
	{
		//bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->AutoFireRate, FireDelay, 0.001f);
		bool bNearlyEqual = EquippedWeapon->FixedAutoFireRate <= FireDelay + 0.001f;
		return bNearlyEqual;
	}
	return true;
}

void AShooterCharacter::MulticastShotgunFire_Implementation(const TArray<FHitResult>& WeaponTraceHits)
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	ShotgunLocalFire(WeaponTraceHits);
}

void AShooterCharacter::FireProjectileWeapon()
{
	if (EquippedWeapon == nullptr) return;
	FHitResult HitResult;
	FVector VectorPlaceholder;
	bool bHit = TraceUnderCrosshairs(HitResult, VectorPlaceholder);
	HitTarget = HitResult.ImpactPoint;

	if (!HasAuthority()) LocalFire(GetHitscanHits(HitTarget));
	ServerFire(GetHitscanHits(HitTarget), EquippedWeapon->AutoFireRate, EquippedWeapon);

}

void AShooterCharacter::FireWeapon()
{
	if (CanFire()) 
	{
		bCanFire = false;
		switch (EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_Shotgun:
			FireShotgun();
			break;
		case EWeaponType::EWT_GrenadeLauncher:
		case EWeaponType::EWT_CyberPistol:
		case EWeaponType::EWT_GravCannon:
			FireProjectileWeapon();
			break;
		default:
			FireHitScanWeapon();
			break;
		}
		StartFireTimer();

		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol 
			|| EquippedWeapon->GetWeaponType() == EWeaponType::EWT_CyberPistol)
		{
			EquippedWeapon->StartSlideTimer();
		}
	}
}

void AShooterCharacter::PlayFireSound()
{
	// Play fire sound
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->GetFireSound(), GetActorLocation());
		//UGameplayStatics::SpawnSoundAtLocation(this, EquippedWeapon->GetFireSound(), GetActorLocation());
	}
}

void AShooterCharacter::SendBullet(const FHitResult& WeaponTraceHit)
{
	// Send bullet
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		if (EquippedWeapon->GetMuzzleFlash())
		{
			//UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
			UGameplayStatics::SpawnEmitterAttached(
				EquippedWeapon->GetMuzzleFlash(),
				EquippedWeapon->GetItemMesh(),
				FName("BarrelSocket")
			);
		}

		//FHitResult WeaponTraceHit;
		//FVector_NetQuantize HitPointFinal = HitPoint;
		//bool bBeamEnd = GetBeamEndLocation(WeaponTraceHit, SocketTransform.GetLocation(), HitPointFinal);
		ShotCharacter(WeaponTraceHit);
		BulletEffect(WeaponTraceHit.ImpactPoint, SocketTransform);

		// Start bullet fire timer for crosshairs
		StartCrosshairBulletFire();
	}
}

void AShooterCharacter::BulletEffect(const FVector_NetQuantize& HitPoint, const FTransform& SocketTransform)
{
	if (EquippedWeapon && EquippedWeapon->GetImpactParticles())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetImpactParticles(), HitPoint);
	}

	if (EquippedWeapon->GetBeamParticles())
	{
		UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetBeamParticles(), SocketTransform);
		if (Beam)
		{
			Beam->SetVectorParameter(FName("Target"), HitPoint);
		}
	}
}

void AShooterCharacter::PlayGunfireMontage(FName MontageSectionName)
{
	// Play Gun Fire Montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(MontageSectionName);
		AnimInstance->Montage_SetEndDelegate(FireingMontageFinishedDelegate, HipFireMontage);
	}
}

void AShooterCharacter::PlayItemMontage(FName ItemMontageSectionName, bool bAutoBlend)
{
    // Play use item montage
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if(AnimInstance && ItemUseMontage)
    {
		CurrentItemMontageSection = ItemMontageSectionName;
		bAutoBlend ? ItemUseMontage->bEnableAutoBlendOut = true : ItemUseMontage->bEnableAutoBlendOut = false;
		bool bSlowMontages = ItemMontageSectionName == FName("Grapple") || ItemMontageSectionName == FName("StartSuperPunch") || ItemMontageSectionName == FName("EndSuperPunch");
		ItemUseMontage->RateScale = bSlowMontages ? 1.0f : 1.25f;
        AnimInstance->Montage_Play(ItemUseMontage);
        AnimInstance->Montage_JumpToSection(ItemMontageSectionName);
		AnimInstance->Montage_SetEndDelegate(ItemMontageFinishedDelegate, ItemUseMontage);
    }
}

void AShooterCharacter::OnMontageFinished(UAnimMontage* Montage, bool bInterrupted)
{
	FName ItemSectionName = CurrentItemMontageSection;
	//UE_LOG(LogTemp, Warning, TEXT("Montage Section = %s"), *ItemSectionName.ToString());
	bool bUseItemAfterMontage = ItemSectionName == FName("CyberHealthV1") || ItemSectionName == FName("CyberShieldV1") ||
								ItemSectionName == FName("CyberHealthV2") || ItemSectionName == FName("CyberShieldV2") ||
								ItemSectionName == FName("Ghost") || ItemSectionName == FName("Teleport") ||
								ItemSectionName == FName("Protect") || ItemSectionName == FName("Fly") ||
								ItemSectionName == FName("Slow") || ItemSectionName == FName("Copy") ||
								ItemSectionName == FName("EndSuperPunch");

	if (!bIsAI && IsLocallyControlled() && !IsGrappling() && bUseItemAfterMontage)
	{
		bCrouching ? SetMovementState(EMovementModifiers::EMM_Crouch) : SetMovementState(EMovementModifiers::EMM_Normal);
	}
    if (!bInterrupted)
    {
        if (bUseItemAfterMontage)
		{
			if (!bIsAI)
			{
				if (HasAuthority())
				{
					BoostItemMontageFinished();
				}
			}
			else
			{
				if (HasAuthority())
				{
					SetMovementState(EMovementModifiers::EMM_Normal);
					BoostItemMontageFinished();
				}
			}
		}
    }
}

void AShooterCharacter::OnFireMontageFinished(UAnimMontage* Montage, bool bInterrupted)
{
	//bIsFiring = false;
}

void AShooterCharacter::PlaySuperPunchMontage(FName ItemMontageSectionName, bool bAutoBlend)
{
    // Play use item montage
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if(AnimInstance && ItemUseMontage)
    {
		bAutoBlend ? ItemUseMontage->bEnableAutoBlendOut = true : ItemUseMontage->bEnableAutoBlendOut = false;
        AnimInstance->Montage_Play(ItemUseMontage);
        AnimInstance->Montage_JumpToSection(ItemMontageSectionName);
    }
}

void AShooterCharacter::Server_HandCombatAttack_Implementation()
{
	Multicast_HandCombatAttack();
}

void AShooterCharacter::Multicast_HandCombatAttack_Implementation()
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	HandCombatAttack_Impl();
}

void AShooterCharacter::HandCombatAttack_Impl()
{
	if (bHandShield) return;
	EMovementMode EnumValue = GetCharacterMovement()->MovementMode.GetValue();
	
	if (CombatState != ECombatState::ECS_Unoccupied) return;
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && ComboMontage)
    {
        bIsComboMontage = AnimInstance->Montage_IsPlaying(ComboMontage);
        if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
        {
            if (!bIsComboMontage)
            {
                ComboStartTimePrev = ComboStartTimeNow;
                ComboStartTimeNow = UGameplayStatics::GetTimeSeconds(GetWorld());
                if (ComboStartTimeNow - ComboStartTimePrev > 2)
                {
                    ComboMontageIndex = 0;
                }

                AnimInstance->Montage_Play(ComboMontage);
                AnimInstance->Montage_JumpToSection(ComboMontageNames[ComboMontageIndex]);
                ComboMontageIndex += 1;
                ComboMontageSectionPlaying = AnimInstance->Montage_GetCurrentSection(ComboMontage);
            }

            if (ComboMontageIndex == ComboMontageNames.Num() - 1)
            {
                ComboMontageIndex = 0;
            }
        }
        else if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying || GetCharacterMovement()->IsFalling())
        {
            if (!bIsComboMontage)
            {
                AnimInstance->Montage_Play(ComboMontage);
                AnimInstance->Montage_JumpToSection(ComboMontageNames.Last()); // Last montage is the air attack
                ComboMontageSectionPlaying = AnimInstance->Montage_GetCurrentSection(ComboMontage);
				bHittingInAir = true;
            }
        }

        //AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
    }
}

void AShooterCharacter::HandCombatAttack()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	bIsComboMontage = AnimInstance->Montage_IsPlaying(ComboMontage);
	if (bIsComboMontage) return;
	if (!HasAuthority()) HandCombatAttack_Impl();
	Server_HandCombatAttack();
}

void AShooterCharacter::SetHandCombatProperties(EHandCombatState HandState)
{
	/*
	switch (HandState)
	{
	case EHandCombatState::EHCS_idle:
		// Set Mesh Properties
		hand_r->SetGenerateOverlapEvents(false);
		hand_l->SetGenerateOverlapEvents(false);
		foot_r->SetGenerateOverlapEvents(false);
		foot_l->SetGenerateOverlapEvents(false);
		calf_r->SetGenerateOverlapEvents(false);
		calf_l->SetGenerateOverlapEvents(false);
		lowerarm_l->SetGenerateOverlapEvents(false);
		lowerarm_r->SetGenerateOverlapEvents(false);
		break;
	case EHandCombatState::EHCS_foot_r:
		// Set Mesh Properties
		hand_r->SetGenerateOverlapEvents(false);
		hand_l->SetGenerateOverlapEvents(false);
		foot_r->SetGenerateOverlapEvents(true);
		foot_l->SetGenerateOverlapEvents(false);
		calf_r->SetGenerateOverlapEvents(true);
		calf_l->SetGenerateOverlapEvents(false);
		lowerarm_l->SetGenerateOverlapEvents(false);
		lowerarm_r->SetGenerateOverlapEvents(false);
		break;
	case EHandCombatState::EHCS_foot_l:
		// Set Mesh Properties
		hand_r->SetGenerateOverlapEvents(false);
		hand_l->SetGenerateOverlapEvents(false);
		foot_r->SetGenerateOverlapEvents(false);
		foot_l->SetGenerateOverlapEvents(true);
		calf_r->SetGenerateOverlapEvents(false);
		calf_l->SetGenerateOverlapEvents(true);
		lowerarm_l->SetGenerateOverlapEvents(false);
		lowerarm_r->SetGenerateOverlapEvents(false);
		break;

	case EHandCombatState::EHCS_hand_r:
		// Set Mesh Properties
		hand_r->SetGenerateOverlapEvents(true);
		hand_l->SetGenerateOverlapEvents(false);
		foot_r->SetGenerateOverlapEvents(false);
		foot_l->SetGenerateOverlapEvents(false);
		calf_r->SetGenerateOverlapEvents(false);
		calf_l->SetGenerateOverlapEvents(false);
		lowerarm_l->SetGenerateOverlapEvents(false);
		lowerarm_r->SetGenerateOverlapEvents(true);
		break;
	case EHandCombatState::EHCS_hand_l:
		// Set Mesh Properties
		hand_r->SetGenerateOverlapEvents(false);
		hand_l->SetGenerateOverlapEvents(true);
		foot_r->SetGenerateOverlapEvents(false);
		foot_l->SetGenerateOverlapEvents(false);
		calf_r->SetGenerateOverlapEvents(false);
		calf_l->SetGenerateOverlapEvents(false);
		lowerarm_l->SetGenerateOverlapEvents(true);
		lowerarm_r->SetGenerateOverlapEvents(false);
		break;
	}
	*/
}

void AShooterCharacter::SetCombatCollisions()
{
	/*
	SetHandCombatProperties(EHandCombatState::EHCS_idle);
	if (bUnEquippedState)
	{
		//spine_01->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		for (auto Box : HitCollisionBoxes)
		{
			if (Box.Value)
			{
				Box.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}

	}
	else
	{
		//spine_01->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		for (auto Box : HitCollisionBoxes)
		{
			if (Box.Value)
			{
				Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
	*/
}

void AShooterCharacter::OnHandOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	/*
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnHandOverlap"));
		UE_LOG(LogTemp, Warning, TEXT("bComboHit = %i"), bComboHit);
	}
	if (bComboHit && OtherActor != this)
	{	
		//PunchedEffect(OverlappedComponent, false);

		auto DamagedCharacter = Cast<AShooterCharacter>(OtherActor);
		auto DamagedContainer = Cast<AItemContainer>(OtherActor);
		if (DamagedCharacter && DamagedCharacter != this)
		{
			//HitCharacter(DamagedCharacter, bComboHit);
			bComboHit = false;
			int32 HitDamage = ComboDamageMap.Contains(ComboMontageSectionPlaying) ? ComboDamageMap[ComboMontageSectionPlaying] : 10.f;
			ShowHitNumber(HitDamage, DamagedCharacter->GetActorLocation(), false);
		}
		else if (DamagedContainer)
		{
			//HitContainer(DamagedContainer, bComboHit);
			bComboHit = false;
			int32 HitDamage = ComboDamageMap.Contains(ComboMontageSectionPlaying) ? ComboDamageMap[ComboMontageSectionPlaying] : 10.f;
			ShowHitNumber(HitDamage, DamagedContainer->GetActorLocation(), false);
		}
	}
	*/
}

void AShooterCharacter::PunchedEffect(FVector CombatHitLocation, bool bIsChargeAttack)
{
	USoundCue* HitSound = bIsChargeAttack ? CharacterChargeSound : CharacterPunchedSound;
	UParticleSystem* HitEffect = bIsChargeAttack ? CharacterChargeEffect : ComboHitEffect;
	
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				CombatHitLocation,
				1.25f
			);
	}

	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, CombatHitLocation);
	}
	
}

void AShooterCharacter::CheckHandCombatHit()
{
	TArray<FHitResult> HitResults;
    UWorld* World = GetWorld();
	
	//const USkeletalMeshSocket* HitSocket = GetMesh()->GetSocketByName(HitSocketName);
    if (World && SwingLocationComponent)
    {
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this); // Ignore the character itself
		//FCollisionObjectQueryParams TraceObjectParams;
		//TraceObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		//TraceObjectParams.AddObjectTypesToQuery(ECC_ItemContainer); // Add more channels as needed

		//DrawDebugSphere(World, SwingLocationComponent->GetComponentLocation(), FCollisionShape::MakeSphere(20.f).GetSphereRadius(), 12, FColor::Purple, true);
		//UE_LOG(LogTemp, Warning, TEXT("HitSocketLocation = %s"), *GetMesh()->GetBoneLocation(FName("hand_r")).ToString());
        // Perform a sphere trace
        if (World->SweepMultiByChannel(
            HitResults,                // Array to store hit results
            SwingLocationComponent->GetComponentLocation(),                 // Start location
            SwingLocationComponent->GetComponentLocation(),                 // End location (same as start for a sphere trace)
            FQuat::Identity,            // Rotation (identity rotation for a sphere trace)
            ECC_Visibility,                   // Collision channel (adjust as needed)
            FCollisionShape::MakeSphere(20.f), // Sphere shape
            QueryParams                  // Query parameters
        ))
        {
			for (auto HitResult : HitResults)
			{
				AActor* HitActor = HitResult.GetActor();
				if (!HitActor || HitActor == this) continue;
				IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitActor);
				if (WeaponHitInterface)
				{
					int32 HitDamage = ComboDamageMap.Contains(ComboMontageSectionPlaying) ? ComboDamageMap[ComboMontageSectionPlaying] : 10.f;
					WeaponHitInterface->Execute_OnHandCombatHit(
						HitActor, 
						this, 
						HitDamage, 
						GetShooter_PS(), 
						HitResult
						);
					break;
				}
			}
        }
    }
}

void AShooterCharacter::OnHandCombatPlayEvent(const FString& State)
{
	switch (CharacterSelected)
	{
		case ECharacterProperty::ECP_Maverick:
			if (ComboMontageSectionPlaying == FName("Combo1"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo2"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentR;
			}
			else if (ComboMontageSectionPlaying == FName("Combo3"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentH;
			}
			break;
		case ECharacterProperty::ECP_Trinity:
			if (ComboMontageSectionPlaying == FName("Combo1"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo2"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo3"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo4"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentH;
			}
			break;
		case ECharacterProperty::ECP_Raven:
			if (ComboMontageSectionPlaying == FName("Combo1"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentR;
			}
			else if (ComboMontageSectionPlaying == FName("Combo2"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo3"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentH;
			}
			else if (ComboMontageSectionPlaying == FName("Combo4"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentLo;
			}
			break;
		case ECharacterProperty::ECP_Leo:
			if (ComboMontageSectionPlaying == FName("Combo1"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentR;
			}
			else if (ComboMontageSectionPlaying == FName("Combo2"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentL;
			}
			else if (ComboMontageSectionPlaying == FName("Combo3"))
			{
				if (State == TEXT("Start")) SwingLocationComponent = SwingComponentH;
			}
			break;
		
		default:
			break;
	}
}

void AShooterCharacter::HandleStopComboHit()
{
	//if (bComboHit) bComboHit = false;
	bComboPlaying = false;
	//HandCombatState = EHandCombatState::EHCS_idle;
}

void AShooterCharacter::ComboHit()
{
	//bComboHit = true;
	CheckHandCombatHit();
}

void AShooterCharacter::ComboAirStart()
{
	switch (CharacterSelected)
	{
		case ECharacterProperty::ECP_Maverick:
			SwingLocationComponent = SwingComponentLo;
			break;
		case ECharacterProperty::ECP_Trinity:
			SwingLocationComponent = SwingComponentLo;
			break;
		case ECharacterProperty::ECP_Raven:
			SwingLocationComponent = SwingComponentLo;
			break;
		case ECharacterProperty::ECP_Leo:
			SwingLocationComponent = SwingComponentLo;
			break;
	}
}

void AShooterCharacter::ComboAirFinish()
{
	//if (!HasAuthority()) return;
    //HandleStopComboHit();
}

void AShooterCharacter::CheckEmotePlaying()
{
	if (bEmoteButtonPressed && GetCharacterMovement())
	{
		bEmoteButtonPressed = false;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		ServerSetMovementMode(EMovementMode::MOVE_Walking);
	}
}

void AShooterCharacter::ServerSetMovementMode_Implementation(EMovementMode InMovementMode)
{
	MulticastSetMovementMode(InMovementMode);
}

void AShooterCharacter::MulticastSetMovementMode_Implementation(EMovementMode InMovementMode)
{
	if (IsLocallyControlled() && !HasAuthority()) return; 
	GetCharacterMovement()->SetMovementMode(InMovementMode);
}

bool AShooterCharacter::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	//if (EquippedWeapon->GetAmmo() > 0 && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun) return true;
	if (bLocallyReloading) return false;
	return WeaponHasAmmo() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void AShooterCharacter::FireActionButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) FireButtonPressed();
}

void AShooterCharacter::FireButtonPressed()
{
	//UE_LOG(LogTemp, Warning, TEXT("bStartGrapplePull = %i, bEquippedItemTypeReady = %i, EquippedItemType = %i, CombatState = %i"), bStartGrapplePull, bEquippedItemTypeReady, EquippedItemType, CombatState);
	if (bDisableGameplay || bChargeAttack || !bCanFire || EquippedItem == nullptr) return;

    bFireButtonPressed = true;
	CheckEmotePlaying();
	switch (EquippedItem->GetItemType())
	{
	case EItemType::EIT_Dummy:
		HandAttack();
		break;
	case EItemType::EIT_Weapon:
		FireWeapon();
		break;
	case EItemType::EIT_Boost:
		if (Boost)
		{
			Boost->StartBoost();
		}
		break;
	default:
		break;
	}
}

void AShooterCharacter::HandAttack()
{
	if (HoverState == EHoverState::EHS_HoverStart && InventorySelect == 0 &&
		GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying)
	{
		HoverState = EHoverState::EHS_HoverRush;
		SetMovementState(EMovementModifiers::EMM_HoverRush);
		TriggerHover();
		if (!HasAuthority()) OnRep_HoverState(EHoverState::EHS_HoverStart);
	}
	else if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking ||
			GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		HandCombatAttack();
	}
}


FHitResult AShooterCharacter::GetHitscanHits(const FVector_NetQuantize& TraceHitTarget)
{
	FHitResult WeaponTraceHit;
	//if (EquippedItem == nullptr || EquippedWeapon == nullptr) return
	if (EquippedWeapon->bIsAnNFT && EquippedWeapon->GetNFTMesh())
	{
		const UStaticMeshSocket* BarrelSocket = EquippedWeapon->GetNFTMesh()->GetSocketByName("BarrelSocket");
		if (BarrelSocket)
		{
			FTransform SocketTransform;
			BarrelSocket->GetSocketTransform(SocketTransform, EquippedWeapon->GetNFTMesh());
			bool bBeamEnd = GetBeamEndLocation(WeaponTraceHit, SocketTransform.GetLocation(), TraceHitTarget);
			return WeaponTraceHit;
		}
	} 
	else if (!EquippedWeapon->bIsAnNFT && EquippedWeapon->GetItemMesh())
	{
		const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
		if (BarrelSocket)
		{
			const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
			bool bBeamEnd = GetBeamEndLocation(WeaponTraceHit, SocketTransform.GetLocation(), TraceHitTarget);
			return WeaponTraceHit;
		}
	}

	return WeaponTraceHit;
}

FHitResult AShooterCharacter::GetHitscanHitsBoost(const FVector_NetQuantize& TraceHitTarget)
{
	FHitResult BoostTraceHit;
	//if (EquippedItem == nullptr || EquippedWeapon == nullptr) return 
	//const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
	const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("GrappleSocket"));
	if (HandSocket)
	{
		const FTransform SocketTransform = HandSocket->GetSocketTransform(GetMesh());
		bool bBeamEnd = GetBeamEndLocation(BoostTraceHit, SocketTransform.GetLocation(), TraceHitTarget);
		//DrawDebugLine(GetWorld(), BoostTraceHit.TraceStart, BoostTraceHit.ImpactPoint, FColor::Cyan, true);
		return BoostTraceHit;
	}
	return BoostTraceHit;
}

void AShooterCharacter::MissedShotTrace(const FHitResult& WeaponTraceHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		TSet<AActor*> UniqueMissedCharacters;
		FVector MissedShotDirection = (WeaponTraceHit.TraceEnd - WeaponTraceHit.TraceStart).GetSafeNormal();
		FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(MissedShotDirection).ToQuat();
		float CapsuleHalfHeight = (WeaponTraceHit.TraceEnd - WeaponTraceHit.TraceStart).Size() * 0.5f;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this); // Ignore the character itself
		if (WeaponTraceHit.GetActor()) QueryParams.AddIgnoredActor(WeaponTraceHit.GetActor());
		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

		if (World->SweepMultiByObjectType(
			MissedShotResults,
			WeaponTraceHit.TraceStart,
			WeaponTraceHit.TraceEnd,
			CapsuleRotation,
			ObjectQueryParams, // Only detect characters
			FCollisionShape::MakeCapsule(150.0f, CapsuleHalfHeight),
			QueryParams
		))
		{
			/*
			DrawDebugCapsule(World, (WeaponTraceHit.TraceStart + WeaponTraceHit.TraceEnd) * 0.5f, CapsuleHalfHeight, 50.0f,
				CapsuleRotation, FColor::Red, false, 2.0f, 0, 2.0f);
			*/
			for (auto HitResult : MissedShotResults)
			{
				if (HitResult.bBlockingHit && HitResult.GetActor() && !UniqueMissedCharacters.Contains(HitResult.GetActor()))
				{
					IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(HitResult.GetActor());
					if (WeaponHitInterface)
					{
						UniqueMissedCharacters.Add(HitResult.GetActor());
						WeaponHitInterface->Execute_OnMissedShot(
							HitResult.GetActor(), 
							this,
							HitResult.ImpactPoint
							);
					}
				}
			}
		}
	}
}

void AShooterCharacter::FireHitScanWeapon()
{
	if (EquippedWeapon)
	{
		FHitResult CrosshairHitResult;
		FVector OutBeamLocation;
		bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);
		HitTarget = CrosshairHitResult.ImpactPoint;

		if (!HasAuthority()) LocalFire(GetHitscanHits(HitTarget));
		ServerFire(GetHitscanHits(HitTarget), EquippedWeapon->AutoFireRate, EquippedWeapon);
	}
}

void AShooterCharacter::LocalFire(const FHitResult& WeaponTraceHit)
{
	if (EquippedWeapon == nullptr) return;
	if (CombatState == ECombatState::ECS_Unoccupied)
	{
		
		if (EquippedWeapon->ProjectileClass)
		{
			EquippedWeapon->FireProjectile(WeaponTraceHit, this);
			// When you incorporate bullet speed, you can use MissedShotedTrace() function here.
		}
		else
		{
			SendBullet(WeaponTraceHit);
			MissedShotTrace(WeaponTraceHit);
		}
		PlayFireSound();
		ShooterPS = GetShooter_PS();
		if (HasAuthority() && ShooterPS)
		{
			ShooterPS->PlayerGameStats.ShotsFired++;
		}
		
		/*
		FName FireMontageSection;
		if (!bAiming)
		{
			FireMontageSection = FName("StartFire");
		}
		else
		{
			switch (EquippedWeapon->GetWeaponType())
			{
			case EWeaponType::EWT_Pistol:
			case EWeaponType::EWT_CyberPistol:
				FireMontageSection = bAiming ? FName("StartPistolFire") : FName("StartFire");
				break;
			
			default:
				FireMontageSection = FName("StartFire");
				break;
			}
		}
		*/
		
		PlayGunfireMontage(FName("StartFire"));
		bIsFiring = true;
		EquippedWeapon->SpendRound();
		//Character->PlayFireMontage(bAiming);
		//EquippedWeapon->Fire(TraceHitTarget);
		// Play anims + update ammo
	}
}

void AShooterCharacter::ServerFire_Implementation(const FHitResult& WeaponTraceHit, float FireDelay, const AWeapon* ClientWeapon)
{
	MulticastFire(WeaponTraceHit);
}

bool AShooterCharacter::ServerFire_Validate(const FHitResult& WeaponTraceHit, float FireDelay, const AWeapon* ClientWeapon)
{
	if (EquippedWeapon && ClientWeapon && EquippedWeapon == ClientWeapon)
	{
		//bool bNearlyEqual = FMath::IsNearlyEqual(EquippedWeapon->AutoFireRate, FireDelay, 0.001f);
		bool bNearlyEqual = EquippedWeapon->FixedAutoFireRate <= FireDelay + 0.001f;
		return bNearlyEqual;
	}
	return true;
}

void AShooterCharacter::Server_OnNonZeroSequence_Implementation(AWeapon* OwnerWeapon, int32 ClientSequence)
{
	if (OwnerWeapon)
	{
		OwnerWeapon->OnNonZeroSequence(ClientSequence);
	}
}

void AShooterCharacter::MulticastFire_Implementation(const FHitResult& WeaponTraceHit)
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	LocalFire(WeaponTraceHit);
}

void AShooterCharacter::FireActionButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) FireButtonReleased();
}

void AShooterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	bFireButtonPressed = false;
	if (HoverState == EHoverState::EHS_HoverRush)
	{
		HoverState = EHoverState::EHS_HoverStart;
		TriggerHover();
		OnRep_HoverState(EHoverState::EHS_HoverRush);
	}
}

void AShooterCharacter::StartFireTimer()
{
	if (EquippedWeapon == nullptr) return;
	//CombatState = ECombatState::ECS_FireTimerInProgress;
	//SetCombatState(ECombatState::ECS_FireTimerInProgress);

	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, EquippedWeapon->GetAutoFireRate());
}

void AShooterCharacter::AutoFireReset()
{
	bCanFire = true;
	if (EquippedItem && EquippedItem->GetItemType() != EItemType::EIT_Weapon) return;
	//CombatState = ECombatState::ECS_Unoccupied;
	//SetCombatState(ECombatState::ECS_Unoccupied);

	if (EquippedWeapon == nullptr) return;
	if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
	{
		FireWeapon();
	}
	ReloadEmptyWeapon();
}

void AShooterCharacter::ReloadEmptyWeapon()
{
	if (InventorySelect != 0 && EquippedWeapon && !WeaponHasAmmo() && CarryingAmmo())
	{
		Reload();
	}
}

void AShooterCharacter::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		if (!IsLocallyControlled()) ReloadWeapon();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed && EquippedItem && EquippedItem->GetItemType() == EItemType::EIT_Weapon)
		{
			FireWeapon();
		}
		if (IsLocallyControlled())
		{
			ReloadEmptyWeapon();
		}
		break;
	case ECombatState::ECS_FireTimerInProgress:

		break;
	case ECombatState::ECS_Equipping:
		if (!IsLocallyControlled())
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance && EquipMontage)
			{
				AnimInstance->Montage_Play(EquipMontage, 1.0f);
				AnimInstance->Montage_JumpToSection(FName("Equip"));
				//UGameplayStatics::PlaySound2D(this, NewWeapon->GetEquipSound());
			}
		}
		break;
	}
}

AItem* AShooterCharacter::SpawnForDefaultWeapon()
{
	// Check the TSubclassOf variable
	if (DefaultItemClass)
	{
		// Spawn the Weapon
		return GetWorld()->SpawnActor<AItem>(DefaultItemClass);
	}
	return nullptr;
}

void AShooterCharacter::EquipItem(class AItem* ItemToEquip, bool bSwap, bool bItemRemoved)
{
    if (ItemToEquip)
    {
        // Refresh ownership to ensure RPCs to work
        if (EquippedItem)
            EquippedItem->SetOwner(nullptr);
			//EquippedItem->SetOwner(this);

        AItem* OldEquippedItem = EquippedItem;

		if (ItemToEquip->GetItemType() == EItemType::EIT_Weapon)
		{
			EquippedWeapon = !ItemToEquip->bIsAnNFT ? Cast<AWeapon>(ItemToEquip) : Cast<AWeaponNFT>(ItemToEquip);
			//ReloadEmptyWeapon();
		}
        EquippedItem = ItemToEquip;
        EquippedItem->SetOwner(this);
        OnRep_EquippedItem(OldEquippedItem);
		ClientInventoryAnim(bSwap, OldEquippedItem, EquippedItem, bItemRemoved);
		if (Cast<ABoostItem>(OldEquippedItem))
		{
			bUsingItem = false;
			//TargetMovementSpeed = BaseMovementSpeed;
			//if (GetCharacterMovement()) GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed; 
		}
    }
}

void AShooterCharacter::ClientInventoryAnim_Implementation(bool bSwap, AItem* OldItem, AItem* ItemEquipped, bool bItemRemoved)
{
	if (ItemEquipped == nullptr) return;
	if (IsLocallyControlled() && !bIsAI)
	{
		if ((ItemEquipped->GetSlotIndex() != 0 && !bSwap) || (ItemEquipped->GetSlotIndex() != 0 && bItemRemoved))
		{
			UGameplayStatics::PlaySound2D(this, ItemEquipped->GetEquipSound());
		}
	}
}

void AShooterCharacter::OnRep_EquippedItem(AItem* OldItem)
{
	if (EquippedItem == nullptr) return;
	
	//if (bAiming) SetAiming(false);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	StopItemMontageEffects(AnimInstance); 
	StopReloadMontage(AnimInstance);
	UpdateItemState();
	bIsFiring = false;

	if (EquippedItem->Implements<UItemInterface>()) EquippedItem->OnEquipItem(this, AnimInstance);

    if (bGhostMode && !IsLocallyControlled())
    {
		if (!EquippedItem->bIsAnNFT && EquippedItem->GetItemMesh())
		{
			EquippedItem->GetItemMesh()->SetVisibility(false);
		}
		else if (EquippedItem->bIsAnNFT && EquippedItem->GetNFTMesh())
		{
			EquippedItem->GetNFTMesh()->SetVisibility(false, true);
		}
        // Hide new equipped item, and show the old weapon (old weapon will be in Picked up mode anyway).
		
    }

	if (IsLocallyControlled())
	{
		bCanFire = true;
		CheckAimingWidget();
		//SetMovementState(EMovementModifiers::EMM_Normal);
	}
}

void AShooterCharacter::InitialEquipWeapon(AItem* ItemToEquip)
{
	Inventory.Add(ItemToEquip);
    Multicast_SetInitialItem(ItemToEquip);
	//SetInitialBoost();
	/*
	InitialItem = WeaponToEquip;
	
	if (InitialItem == nullptr) return;
	//EquippedItem = InitialItem;
	
	OnRep_InitialItem();
	*/
}

void AShooterCharacter::Multicast_SetInitialItem_Implementation(AItem* WeaponToEquip)
{
    if (WeaponToEquip == nullptr) return;

    if (WeaponToEquip)
        WeaponToEquip->SetOwner(nullptr);

    WeaponToEquip->SetOwner(this);

    const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
    if (HandSocket)
    {
        // Attach the weapon to the hand socket RightHandSocket
        HandSocket->AttachActor(WeaponToEquip, GetMesh());
    }
	
	WeaponToEquip->SetSlotIndex(0);
	WeaponToEquip->DisableCustomDepth();
	WeaponToEquip->DisableGlowMaterial();
	EquippedItem = WeaponToEquip;
	ADummyItem* DummyItem = Cast<ADummyItem>(WeaponToEquip);
	if (DummyItem)
	{
		DummyItem->UpdateDummyItem();
	}
}

void AShooterCharacter::SetInitialBoost()
{
	if (BoostItemClass)
	{
		ABoostItem* InitialBoost = GetWorld()->SpawnActor<ABoostItem>(BoostItemClass, GetActorTransform());
		if (InitialBoost == nullptr) return;
		Inventory.Add(InitialBoost);
		InitialBoost->SetBoostType(EBoostType::EBT_Grapple);
		InitialBoost->UpdateBoost();
		Multicast_SetInitialBoost_Implementation(InitialBoost);
	}
}

void AShooterCharacter::Multicast_SetInitialBoost_Implementation(ABoostItem* InitialBoost)
{
	if (InitialBoost)
	{
		InitialBoost->SetOwner(nullptr);
		InitialBoost->SetOwner(this);
		InitialBoost->SetBoostItemAmount(10);
		InitialBoost->SetSlotIndex(1);
		if (HasAuthority()) InitialBoost->SetItemState(EItemState::EIS_PickedUp);
	}
}

int32 AShooterCharacter::GetAmmoAmount(EAmmoType AmmoType)
{
	if (AmmoStruct.AmmoType.Contains(AmmoType))
	{
		return AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(AmmoType)];
	}
	else
	{
		return -1;
	}
}

void AShooterCharacter::DropAmmo_Implementation(AAmmo* AmmoToThrow)
{
    if (AmmoToThrow)
    {
		//AmmoToThrow->SetOwner(nullptr);
        FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
        //AmmoToThrow->GetAmmoMesh()->DetachFromComponent(DetachmentTransformRules);
		AmmoToThrow->DetachFromActor(DetachmentTransformRules);
		AmmoToThrow->SetAmmoState(EItemState::EIS_Falling);
        AmmoToThrow->ThrowAmmo(GetCapsuleComponent()->GetComponentTransform());
		int32 AmmoAmount = GetAmmoAmount(AmmoToThrow->GetAmmoType());
		if (AmmoAmount != -1)
		{
			AmmoToThrow->SetItemCount(AmmoAmount);
		}
		AmmoToThrow->EnableGlowMaterial();
		AmmoToThrow->StartPulseTimer();
		//Multicast_Item_EnableGlowMaterial(AmmoToThrow);
    }
}

void AShooterCharacter::DropItem_Implementation(AItem* ItemToThrow, bool bPlayerElimed)
{
    if (ItemToThrow)
    {
		if (ItemToThrow->Implements<UItemInterface>()) ItemToThrow->OnDropItem(this, FTransform(), bPlayerElimed);
    }
    else if (EquippedItem)
    {
		if (EquippedItem->Implements<UItemInterface>()) EquippedItem->OnDropItem(this, FTransform(), bPlayerElimed);
    }
}

void AShooterCharacter::Multicast_Item_EnableGlowMaterial_Implementation(AItem* InItem)
{
	if(!InItem) return;
	InItem->EnableGlowMaterial();
	InItem->StartPulseTimer();
}

AActor* AShooterCharacter::FindSingleActorForCasting(FName Tag, TSubclassOf<AActor> StaticClass)
{
	//AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), 
	//        AItem::StaticClass());

	// StaticClass should be like AItem::StaticClass() (where AItem is the Actor class you want to import from)
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(
		GetWorld(),
		StaticClass,
		Tag,
		FoundActors
	);

	if (FoundActors.Num() > 0)
	{
		for (int32 i = 0; i < FoundActors.Num(); i++)
		{
			if (FoundActors[i]->ActorHasTag(Tag))
			{
				return FoundActors[i];
			}
		}
	}
	return nullptr;
}

AActor* AShooterCharacter::GetOwningActor(FName Tag) const
{
	TArray<UPrimitiveComponent*> Components;
	GetOverlappingComponents(Components); //no output for this function. 
	for (int32 i = 0; i < Components.Num(); i++)
	{
		bool IsInSphere = Components[i]->ComponentHasTag(Tag);
		if (IsInSphere)
		{
			//UE_LOG(LogTemp, Display, TEXT("Overlapping actor name: %s"), *Components[i]->GetName());
			return Components[i]->GetOwner();
		}
	}
	return nullptr;
}

void AShooterCharacter::SwapItem(AWeapon* WeaponToSwap, ABoostItem* BoostToSwap)
{
    if (EquippedItem == nullptr) return;
    int32 ItemIndex = CurrentItemToSwap ? CurrentItemToSwap->GetSlotIndex() : EquippedItem->GetSlotIndex();

    if (Inventory.Num() - 1 >= ItemIndex && ItemIndex != 0)
    {
        if (WeaponToSwap)
        {
            Inventory[ItemIndex] = WeaponToSwap;
            WeaponToSwap->SetSlotIndex(ItemIndex);
            WeaponToSwap->SetItemState(EItemState::EIS_PickedUp);
            DropItem(CurrentItemToSwap);
            if (!CurrentItemToSwap || EquippedItem == CurrentItemToSwap) EquipItem(WeaponToSwap, true);
        }
        else if (BoostToSwap)
        {
            Inventory[ItemIndex] = BoostToSwap;
            BoostToSwap->SetSlotIndex(ItemIndex);
            BoostToSwap->SetItemState(EItemState::EIS_PickedUp);
            DropItem(CurrentItemToSwap);
            if (!CurrentItemToSwap || EquippedItem == CurrentItemToSwap) EquipItem(BoostToSwap, true);
        }
    }
    // If character is in hand combat mode (slot 0)
    else if (Inventory.Num() > 1 && ItemIndex == 0) // Ensure slot 1 exists
    {
        AItem* OldItemInSlot1 = Inventory[1];
        if (OldItemInSlot1) // Extra safety check
        {
            DropItem(OldItemInSlot1);
        }
        if (WeaponToSwap)
        {
            Inventory[1] = WeaponToSwap;
            WeaponToSwap->SetSlotIndex(1);
            WeaponToSwap->SetItemState(EItemState::EIS_PickedUp);
        }
        else if (BoostToSwap)
        {
            Inventory[1] = BoostToSwap;
            BoostToSwap->SetSlotIndex(1);
            BoostToSwap->SetItemState(EItemState::EIS_PickedUp);
        }
        // Handle case where player switched to slot 1 during animation
        if (OldItemInSlot1 && EquippedItem == OldItemInSlot1)
        {
            EquipItem(Inventory[1], true);
        }
    }
    CurrentItemToSwap = nullptr;
    TraceHitItem = nullptr;
    TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector VectorPlaceholder;
		TraceUnderCrosshairs(ItemTraceResult, VectorPlaceholder);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			if (TraceHitItem && ItemTraceResult.GetActor()->Implements<UItemInterface>() && TraceHitItem->IsAnItem())
			{
				if (HighlightedSlot == -1)
				{
					// Not currently highlighting a slot; highlight one
					HighlightInventorySlot();
					//IconRotateDelegate.Broadcast(true);
				}
			}
			else
			{
				// Is a slot being highlighted?
				if (HighlightedSlot != -1)
				{
					// Not currently highlighting a slot; highlight one
					UnHighlightInventorySlot();
					//IconRotateDelegate.Broadcast(false);
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}

			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				// Show Item's Pickup Widget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();

				if (Inventory.Num() >= InventoryMaxCapacity)
				{
					TraceHitItem->SetInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetInventoryFull(false);
				}
				//TraceHitItem->EnableGlowMaterial();
			}

			// We hit an AItem last frame
			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					// We are hitting a different AItem this frame from last frame, or AItem is null
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
					//TraceHitItemLastFrame->DisableGlowMaterial();
				}

			}

			// Store a reference to HitItem for next frame
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		// No longer overlapping any items
		// Item last frame should not show widget
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
		//TraceHitItemLastFrame->DisableGlowMaterial();
	}
}

void AShooterCharacter::SetGunInfoVisible()
{
	//AActor* ItemActor = FindSingleActorForCasting("ItemTag", AItem::StaticClass());
	//Item = Cast<AItem>(ItemActor);
		//UE_LOG(LogTemp, Warning, TEXT("Distance = %f"), dist);

		//Show Widet if character is within 5 meters of weapon and there is a trace hit with the weapon

	AActor* GoodActor = GetOwningActor("ItemTag");
	if (GoodActor != nullptr)
	{
		//UE_LOG(LogTemp, Display, TEXT("Overlapping actor name:"));
		FHitResult ItemTraceResult;
		FVector VectorPlaceholder;
		TraceUnderCrosshairs(ItemTraceResult, VectorPlaceholder);
		float DistanceToItem;

		if (ItemTraceResult.bBlockingHit)
		{
			//Item->GetPickupWidget()->SetVisibility(true);

			// Good code to cast when you have a hit with an actor

			AItem* HitItem = Cast<AItem>(ItemTraceResult.GetActor());
			if (HitItem && HitItem->GetPickupWidget())
			{
				DistanceToItem = GetDistanceTo(HitItem);
				// Show Item's Pickup Widget
				HitItem->GetPickupWidget()->SetVisibility(true);

			}
			// We hit an AItem last frame
			if (TraceHitItemLastFrame)
			{
				if (HitItem != TraceHitItemLastFrame)
				{
					// We are hitting a different AItem this frame from last frame, or AItem is null
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}

			}
			// Store a reference to HitItem for next frame
			TraceHitItemLastFrame = HitItem;
		}
	}
}

bool AShooterCharacter::GetBeamEndLocation(
	FHitResult &WeaponTraceHit,
	const FVector &MuzzleSocketLocation, 
	const FVector_NetQuantize &OutBeamLocation)
{
	//OutBeamLocation = bCrosshairHit ? CrosshairHitResult.ImpactPoint : OutBeamLocation;
	// Perform a second trace, this time from the gun barrel
	FCollisionQueryParams CollisionParam;
	CollisionParam.AddIgnoredActor(this);
	if (Inventory[0]) CollisionParam.AddIgnoredActor(Inventory[0]);
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

void AShooterCharacter::HitCharacter(AActor* DamagedActor, float HitDamage, AShooterPlayerState* InstigatorPS)
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
		//HandCombatState = EHandCombatState::EHCS_idle;
		//bComboHit = false;
		//UE_LOG(LogTemp, Warning, TEXT("Health: %f"), DamagedCharacter->Health);
	}
}

void AShooterCharacter::ShotCharacter(FHitResult WeaponTraceHit)
{
    if (EquippedWeapon == nullptr) return;

    if (WeaponTraceHit.bBlockingHit && WeaponTraceHit.GetActor())
    {
		if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(WeaponTraceHit.GetActor()))
		{
			WeaponHitInterface->Execute_OnHitScanWeaponHit(
				WeaponTraceHit.GetActor(), 
				this, 
				GetShooter_PS(), 
				WeaponTraceHit, 
				EquippedWeapon->GetCharacterHitSound()
				);
		}
        else
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                EquippedWeapon->GetHitSound(),
                WeaponTraceHit.ImpactPoint,
                0.2f
            );
			
			/*
			if (HasAuthority())
			{
				if (ShooterPS) ShooterPS->PlayerGameStats.FireCharacterMiss++;
				auto GeometryComponent = Cast<UGeometryCollectionComponent>(WeaponTraceHit.GetComponent());
				if (GeometryComponent && EquippedWeapon)
				{
					ImplementChaos(WeaponTraceHit, EquippedWeapon->GetWeaponType());
				}
			}
			*/
        }
    }
}

void AShooterCharacter::OnMissedShot_Implementation(AActor* InstigatorShooter, const FVector& ImpactPoint)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter == this)) return;
	if (IsLocallyControlled() && IsPlayerControlled())
	{
		ShooterPlayerController = GetShooter_PC();
		if (ShooterPlayerController)
		{
			ShooterPlayerController->ShowAttackerLocationMissed(InstigatorShooter, ImpactPoint);
		}
	}
}

void AShooterCharacter::OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter == this) || !InstigatorShooter->GetEquippedWeapon()) return;

	auto VictimPS = Cast<AShooterPlayerState>(this->GetPlayerState());
	bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head") ? true : false;
	float BaseDamage = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
	float DamageToApply = SetGunDamageAmount(this, InHitResult, InstigatorShooter, BaseDamage);
	if (DamageToApply == -1.f) return; 

	if (InstigatorShooter->HasAuthority())
	{
		TSubclassOf<UDamageType> DamageTypeClass = UDamageType::StaticClass(); 
		if (InstigatorPS)
		{
			//InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Gun;
			if (bHeadShot && !InstigatorPS->IsABot() && VictimPS)
			{
				VictimPS->IsABot() ? InstigatorPS->AddToXPMetric(EProgressMetric::EPM_AiHeadShots, 1) : 
					InstigatorPS->AddToXPMetric(EProgressMetric::EPM_PlayerHeadShots, 1);
				
				// This means that hand shield was hit
				if (BaseDamage != DamageToApply) VictimPS->HandShieldHits++;
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
			DamageToApply,
			InstigatorShooter->GetController(),
			InstigatorShooter,
			DamageTypeClass
		);
	}

	if (InstigatorShooter->IsLocallyControlled() && VictimPS && InstigatorPS && VictimPS->GetGenericTeamId() != InstigatorPS->GetGenericTeamId())
	{
		InstigatorShooter->ShowHitNumber(DamageToApply, InHitResult.ImpactPoint, bHeadShot, this->GetShield() > 0);
		if (bHeadShot && InstigatorShooter->GetHeadShotSound()) UGameplayStatics::PlaySound2D(InstigatorShooter, InstigatorShooter->GetHeadShotSound());
	}
	else if (InstigatorShooter->IsLocallyControlled() && this->bIsAI)
	{
		InstigatorShooter->ShowHitNumber(DamageToApply, InHitResult.ImpactPoint, bHeadShot, this->GetShield() > 0);
	}

	UGameplayStatics::PlaySoundAtLocation(
		InstigatorShooter,
		HitSound,
		InHitResult.ImpactPoint,
		0.2f
	);
}

EWeaponType AShooterCharacter::GetWeaponType_Implementation()
{
	return EquippedWeapon ? EquippedWeapon->GetWeaponType() : EWeaponType::EWT_MAX;
}

bool AShooterCharacter::HasShield_Implementation()
{
	return GetShield() > 0.f;
}

void AShooterCharacter::OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType)
{
	if (!InstigatorController || !HasAuthority()) return;

	APawn* InstigatorPawn = InstigatorController->GetPawn();
	AShooterCharacter* InstigatorShooter = Cast<AShooterCharacter>(InstigatorPawn);

	if (this && InstigatorController != this->GetController())
	{
		bool bHeadShot = InHitResult.BoneName.ToString() == TEXT("head");
		float DamageToApply = 0.f, BaseDamage = 0.f;

		auto VictimPS = Cast<AShooterPlayerState>(this->GetPlayerState());
		AShooterPlayerState* InstigatorPS = Cast<AShooterPlayerState>(InstigatorController->PlayerState);
		if (InstigatorShooter && InstigatorShooter->GetEquippedWeapon())
		{
			BaseDamage = InstigatorShooter->GetEquippedWeapon()->SetWeaponDamage(InHitResult.BoneName.ToString());
			DamageToApply = SetGunDamageAmount(this, InHitResult, InstigatorShooter, BaseDamage);
			if (InstigatorPS && !InstigatorPS->IsABot() && VictimPS && VictimPS->GetGenericTeamId() != InstigatorPS->GetGenericTeamId()) InstigatorShooter->ClientOnProjectileHit(this->GetActorLocation(), DamageToApply, bHeadShot, this->GetShield() > 0);
		}
		else if (Damage > 0.f)
		{
			BaseDamage = Damage;
			DamageToApply = SetGunDamageAmount(this, InHitResult, InstigatorPawn, Damage);
		}

		if (InstigatorPS)
		{
			InstigatorPS->DamageType = bHeadShot ? EShooterDamageType::ESDT_HeadShot : EShooterDamageType::ESDT_Gun;
			if (bHeadShot && !InstigatorPS->IsABot() && VictimPS)
			{
				VictimPS->IsABot() ? InstigatorPS->AddToXPMetric(EProgressMetric::EPM_AiHeadShots, 1) : 
					InstigatorPS->AddToXPMetric(EProgressMetric::EPM_PlayerHeadShots, 1);

				if (BaseDamage != DamageToApply) VictimPS->HandShieldHits++;
			}
		}
		UGameplayStatics::ApplyDamage(this, DamageToApply, InstigatorController, DamageCauser, DamageType);
	}
}

void AShooterCharacter::ClientOnProjectileHit_Implementation(const FVector& HitLocation, float DamageToApply, bool bHeadShot, bool bHasShield)
{
	if (IsLocallyControlled())
	{
		ShowHitNumber(DamageToApply, HitLocation, false, bHasShield);
		if (bHeadShot && GetHeadShotSound())
		{
			UGameplayStatics::PlaySound2D(this, GetHeadShotSound());
		}
	}
}

void AShooterCharacter::OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound)
{
	if (InstigatorShooter == nullptr || (InstigatorShooter && InstigatorShooter->GetEquippedWeapon() == nullptr)) return;
	float DamageAmount;
	TMap<AShooterCharacter*, float> HitMap;

	for (const FHitResult& WeaponTraceHit : InHitResults)
	{
		if (WeaponTraceHit.GetActor() == this)
		{
			DamageAmount = InstigatorShooter->GetEquippedWeapon()->SetShotgunDamageAmount(InstigatorShooter, this, WeaponTraceHit);
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

	for (const TPair<AShooterCharacter*, float>& HitPair : HitMap)
	{
		if (HitPair.Key && HitPair.Key != InstigatorShooter)
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
			auto VictimPS = Cast<AShooterPlayerState>(HitPair.Key->GetPlayerState());
			if (InstigatorShooter->IsLocallyControlled() && VictimPS && InstigatorPS && VictimPS->GetGenericTeamId() != InstigatorPS->GetGenericTeamId())
			{
				InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false, HitPair.Key->GetShield() > 0);
			}
			else if (InstigatorShooter->IsLocallyControlled() && HitPair.Key->bIsAI)
			{
				InstigatorShooter->ShowHitNumber(HitPair.Value, HitPair.Key->GetActorLocation(), false, HitPair.Key->GetShield() > 0);
			}
		}
	}
}

float AShooterCharacter::SetGunDamageAmount(AShooterCharacter* DamagedCharacter, const FHitResult& WeaponTraceHit, AActor* InstigatorActor, float BaseDamage)
{
	if (BaseDamage < 0.f) return 0.f;
	//float Wd = BaseDamage > 0.f ? BaseDamage : EquippedWeapon ? EquippedWeapon->SetWeaponDamage(WeaponTraceHit.BoneName.ToString()) : 0;
	if (DamagedCharacter)
	{
		if (DamagedCharacter->bBoostProtect == true) return 0.f;
		float DamageToShow = 0.f;
		
		if (WeaponTraceHit.GetComponent()->ComponentHasTag(FName("ShieldTag")))
		{
			// Dot product between attacker forward direction and victim forward direction
			// If player is shooting another player's shield from behind, doesn't count as damage
			float PlayerDot = InstigatorActor ? 
				UKismetMathLibrary::Dot_VectorVector(InstigatorActor->GetActorForwardVector(), DamagedCharacter->GetCapsuleComponent()->GetForwardVector()) : 0.f;
			if (PlayerDot > 0.f)
			{
				// condition should be < 0, but for some reason, the forward angles seem to be negated in Unreal.
				return -1.f;
			}
			float ScaledWeaponDamage = (-4.f * BaseDamage / (5.f * MaxHandShieldStrength)) * DamagedCharacter->HandShieldStrength + BaseDamage; // Damage to set based on shield strength
			DamageToShow = FMath::CeilToFloat(ScaledWeaponDamage);
			if (HasAuthority())
			{
				this->HandShieldStrength = (FMath::Clamp(this->HandShieldStrength - FMath::RoundToInt32(DamageToShow / 3.f), 0.f, this->GetMaxHandShieldStrength()));
				this->OnRep_HandShieldStrength();
			}
		}
		else
		{
			DamageToShow = BaseDamage;
		}
		return DamageToShow;
	}

	return BaseDamage;
}

void AShooterCharacter::OnHandCombatHit_Implementation(AShooterCharacter* InstigatorShooter, float Damage, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult)
{
	HitCharacter(this, Damage, InstigatorPS);
	if (InstigatorShooter)
	{
		if (InstigatorShooter->IsLocallyControlled())
		{
			InstigatorShooter->ShowHitNumber(Damage, this->GetActorLocation(), false, this->GetShield() > 0);
		}
		InstigatorShooter->PunchedEffect(InHitResult.Location, false);
	}
}

void AShooterCharacter::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
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

void AShooterCharacter::DestroyHitNumber(UUserWidget* HitNumber)
{
	if(!HitNumber) return;

	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

void AShooterCharacter::UpdateHitNumbers()
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

void AShooterCharacter::Multicast_PlayBigDamageMontage_Implementation(float DamageAmount)
{
	// Does this work for multiplayer (sound should be local to character hit)?
	if (this == nullptr || EquippedItem == nullptr) return;
	if ((EquippedItem->GetItemType() == EItemType::EIT_Dummy || EquippedItem->GetItemType() == EItemType::EIT_Boost) && DamageAmount > 60.f)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitMontage)
		{
			AnimInstance->Montage_Play(HitMontage);
			AnimInstance->Montage_JumpToSection(FName("HandModeHit"));

		}
	}
	else if (EquippedItem->GetItemType() == EItemType::EIT_Weapon && DamageAmount > 60.f)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && HitMontage)
		{
			AnimInstance->Montage_Play(HitMontage);
			AnimInstance->Montage_JumpToSection(FName("GunModeHit"));

		}
	}
}

void AShooterCharacter::DropAllItems()
{
	if (Inventory.Num() > 1)
	{
		for (int32 i = 1; i < Inventory.Num(); i++)
		{
			DropItem(Inventory[i], true);
			//Inventory[i]->SetOwner(nullptr);
		}
	}

	if (AmmoInventory.Num() > 0)
	{
		for (int32 i = 0; i < AmmoInventory.Num(); i++)
		{	
			DropAmmo(AmmoInventory[i]);
		}
	}
}

void AShooterCharacter::WinnerOnRespawnMode()
{
	OnTakeAnyDamage.RemoveDynamic(this, &AShooterCharacter::ReceiveDamage);
	bCanTarget = false;
}

void AShooterCharacter::Winner()
{
	bPlayerWon = true;
	OnTakeAnyDamage.RemoveDynamic(this, &AShooterCharacter::ReceiveDamage);
	if (EquippedItem) EquippedItem->SetItemState(EItemState::EIS_Unequipped);
	bCanTarget = false;
	MulticastWinner();
}

void AShooterCharacter::WinnerFixCamera(float DeltaTime)
{
	if (bPlayerWon && bWinnerCameraInterp)
	{
		ShooterPlayerController = GetShooter_PC();

		if (ShooterPlayerController)
		{
			FRotator CurrentRotation = ShooterPlayerController->GetControlRotation();
			FRotator TargetRotation = CurrentRotation;
			// Check if the angle is negative, and rotate it back to positive
			if (CurrentRotation.Pitch < 0.f && CurrentRotation.Pitch > -360.f) CurrentRotation.Pitch += 360.f;
			
			if (CurrentRotation.Pitch > 0.5f && CurrentRotation.Pitch <= 90.f)
			{
				TargetRotation.Pitch = 0.5f;
			}
			else if (CurrentRotation.Pitch >= 270.f && CurrentRotation.Pitch < 359.5f)
			{
				TargetRotation.Pitch = 359.5f;
			}
			ShooterPlayerController->SetControlRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 3.f));
			if (FMath::Abs(CurrentRotation.Pitch - TargetRotation.Pitch) < 1.f) bWinnerCameraInterp = false;
		}
	}
}

void AShooterCharacter::MulticastWinner_Implementation()
{
	if (bHandShield)
	{
		ShieldOn(false);
	}

	if (bAiming)
	{
		SetAiming(false);
	}

	GetWorldTimerManager().ClearTimer(AutoFireTimer);
	bCanFire = false;
	bDisableGameplay = true;
	if (HasAuthority())
	{
		ShooterPlayerController = GetShooter_PC();
		ShooterPS = GetShooter_PS();
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterPlayerController && ShooterPS && ShooterGS)
		{
			ClientShowReturnMenu(true, ShooterPlayerController, ShooterPS->PlayerGameStats, ShooterGS->PlayersStillAlive, 0);
		}
	}

	if (IsLocallyControlled())
	{
		bWinnerCameraInterp = true;
		ShooterPlayerController = GetShooter_PC();
		if (ShooterPlayerController && ShooterPlayerController->MusicActor)
		{
			auto WinnerMusicActor = ShooterPlayerController->MusicActor;
			if (WinnerMusicActor && WinnerSound)
			{
				bool bWasMusicPlaying = WinnerMusicActor->bMusicPlaying;
				if (bWasMusicPlaying) WinnerMusicActor->StopPlayingMusic();
				UGameplayStatics::PlaySound2D(this, WinnerSound);
				FTimerDelegate RestartMusicDel;
				FTimerHandle RestartMusicTimer;
				RestartMusicDel.BindUFunction(this, FName("RestartMusic"), WinnerMusicActor, bWasMusicPlaying);
				GetWorld()->GetTimerManager().SetTimer(RestartMusicTimer, RestartMusicDel, 3.f, false);
			}
		}
		//FTimerDelegate ReturnMenuDel;
		//ReturnMenuDel.BindUFunction(this, FName("ShowReturnMenu"), true, ShooterPlayerController);
		//GetWorld()->GetTimerManager().SetTimer(ShowMenuTimer, ReturnMenuDel, 1.5f, false);
	}
	
	/*
	if (ShooterPlayerController)
	{
		
		if (HasAuthority() && AttackerPS) ;

	}
	*/
}

void AShooterCharacter::RestartMusic(AMusicManager* MusicManager, bool bMusicPlaying)
{
	if (bMusicPlaying) MusicManager->PlayNextSong();
}


void AShooterCharacter::Elim(AShooterPlayerState* AttackerPS, int32 RespawnsLeft)
{
	// Check if is already eliminated
	if(bPlayerEliminated) return;
	bPlayerEliminated = true;

	if (HasAuthority()) DropAllItems();
	MulticastElim(AttackerPS, RespawnsLeft);
	
	AShooterGameState* ShooterGS = GetShooter_GS();
	check(ShooterGS);

	ShooterGS->OnCharacterEliminated(this);
}

void AShooterCharacter::DisableCharacterCollisions()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//spine_01->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	/*
	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	*/
}

void AShooterCharacter::MulticastElim_Implementation(AShooterPlayerState* AttackerPS, int32 RespawnsLeft)
{
	bPlayerEliminated = true;
	ShooterPlayerController = GetShooter_PC();
	if (FlyingTimeline && FlyingTimeline->IsPlaying()) FlyingTimeline->Stop();
	if (bHandShield) ShieldOn(false);

	if (IsLocallyControlled())
	{
		OnElimmed();
		if (ShooterPlayerController)
		{
			ShooterPlayerController->SetInventoryVisibility(false);
			if (bManageInventory) ShooterPlayerController->ManageInventory(bManageInventory);
			if (!IsPracticeMode())
			{
				ShooterPlayerController->GeneralMessage(TEXT("Respawning...Get Ready"), TEXT("Yellow"), 0.5f);
				CheckAimingWidget();
			}
		}
		if (HoverState == EHoverState::EHS_HoverStart || HoverState == EHoverState::EHS_HoverRush)
		{
			HoverState = EHoverState::EHS_HoverStop;
			TriggerHover();
			OnRep_HoverState(EHoverState::EHS_HoverStart);
		}
		if (bGamePaused) PauseButtonPressed();
		if (bAimingButtonPressed) AimingButtonReleased();
		if (EquippedItem) EquipItemDelegate.Broadcast(EquippedItem->GetSlotIndex(), 0);
	}

	PlayElimMontage();
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
	GetWorldTimerManager().ClearTimer(AutoFireTimer);

	bCanFire = false;
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	// If character is in the air or flying, freeze character at the position he was killed
	if (GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Walking)
	{
		GetCharacterMovement()->Velocity = FVector(0.f, 0.f, 0.f);
		GetCharacterMovement()->GravityScale = 0.f;
	}

	if (GetNetMode() == NM_Client)
	{
		FTimerHandle DisableCollisionsTimer;
		GetWorldTimerManager().SetTimer(DisableCollisionsTimer, this, &AShooterCharacter::DisableCollisions, 0.2f);
	}
	else
	{
		DisableCharacterCollisions();
	}

	// Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	if (AttackerPS && AttackerPS->IsABot())
	{
		FTimerDelegate AIElimTimerDel;
		FTimerHandle AITimer;
		AIElimTimerDel.BindUFunction(this, FName("PlayAITauntSound"), AttackerPS->GetPawn());
		GetWorld()->GetTimerManager().SetTimer(AITimer, AIElimTimerDel, ElimDelay, false);
	}

	if (ShooterPlayerController)
	{
		if (HasAuthority()) 
		{
			if (IsPracticeMode())
			{
				auto VictimPS = Cast<AShooterPlayerState>(ShooterPlayerController->PlayerState);
				AShooterGameState* ShooterGS = GetShooter_GS();
				if (VictimPS && ShooterGS && !ShooterPlayerController->bMatchEnded)
				{
					ClientShowReturnMenu(false, ShooterPlayerController, VictimPS->PlayerGameStats, ShooterGS->PlayersStillAlive, RespawnsLeft);
				}
			}
		}
			
		FTimerDelegate ElimTimerDel;
		ElimTimerDel.BindUFunction(this, FName("ElimTimerFinished"), ShooterPlayerController, RespawnsLeft);
		if (IsPracticeMode()) ElimDelay = 2.f;
		GetWorld()->GetTimerManager().SetTimer(ElimTimer, ElimTimerDel, ElimDelay, false);
	}
}

void AShooterCharacter::DisableCollisions()
{
	DisableCharacterCollisions();
}

void AShooterCharacter::ClientShowReturnMenu_Implementation(bool bWinner, AShooterPlayerController* ShooterController, FPlayerGameStats InGameStats, int32 PlayersLeft, int32 RespawnsLeft)
{
	ShooterController->ShowReturnToMainMenu(bWinner, InGameStats, PlayersLeft, RespawnsLeft);
	DisableInput(ShooterController);
}

// Destroy the dead character and let them spectate the player that killed them.
void AShooterCharacter::RespawnTimerFinished(AShooterCharacter* VictimCharacter)
{
	ShooterGameMode = GetShooter_GM();
	if (HasAuthority() && ShooterGameMode)
	{
		ShooterGameMode->RequestRespawn(VictimCharacter, Controller);
	}
}

void AShooterCharacter::PracticeRespawn()
{
	//EquipItemDelegate.Broadcast(EquippedItem->GetSlotIndex(), 0);
	//RespawnTimerFinished(this);
}

// Destroy the dead character and let them spectate the player that killed them.
void AShooterCharacter::ElimTimerFinished(AShooterPlayerController* VictimController, int32 RespawnsLeft)
{
	if (RespawnsLeft != 0)
	{
		if (VictimController && HasAuthority() && !IsPracticeMode())
		{
			VictimController->ClientRespawn(GetActorLocation());
		}
	}
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Destroy();
	return;

	/*
	if (HasAuthority())
	{
		auto VictimPS = Cast<AShooterPlayerState>(VictimController->PlayerState);
		auto AttackerPS = AttackerCharacter->GetShooter_PS();
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS)
		{
			int32 PlayersAlive = ShooterGS->PlayersStillAlive;
			//UE_LOG(LogTemp, Warning, TEXT("Players Alive = %i"), PlayersAlive);

			// Condition when you were killed by someone else
			if (PlayersAlive > 0 && VictimController && AttackerCharacter && !AttackerCharacter->bIsAI && ShooterSpectatorClass && VictimController != AttackerCharacter->GetShooter_PC())
			{
				auto ShooterSpectator = GetWorld()->SpawnActor<AShooterSpectatorPawn>(ShooterSpectatorClass);
				VictimController->UnPossess();
				VictimController->Possess(ShooterSpectator);
				if (!ShooterGS->bIsTeamMode)
				{
					// If in FreeForAll, set view to attacker
					SetVictimView(VictimController, VictimPS, AttackerPS, RespawnsLeft);
				}
				else
				{
					// If in Team mode, set view to a teammate. If all teammates are dead, set view to attacker
					
					if (VictimPS)
					{
						TArray<AShooterPlayerState*> AliveTeamMembers = ShooterGS->GetAliveTeamMembers(VictimPS->GetGenericTeamId());
						if (AliveTeamMembers.Num() > 0)
						{
							auto TeamMateShooter = Cast<AShooterPlayerState>(AliveTeamMembers[0]);
							TeamMateShooter == nullptr ? SetVictimView(VictimController, VictimPS, AttackerPS, RespawnsLeft) : SetVictimView(VictimController, VictimPS, TeamMateShooter, RespawnsLeft);
						}
						else
						{
							SetVictimView(VictimController, VictimPS, AttackerPS, RespawnsLeft);
						}
					}
				}

			}
			//Condition when you killed yourself. Set all of your dead player controllers veiw target to a random alive character.
			else if ((PlayersAlive > 0 && VictimController && ShooterSpectatorClass && (AttackerCharacter == nullptr || VictimController == AttackerCharacter->GetShooter_PC())) ||
						(PlayersAlive > 0 && VictimController && ShooterSpectatorClass && AttackerCharacter && AttackerCharacter->bIsAI))
			{
				auto ShooterSpectator = GetWorld()->SpawnActor<AShooterSpectatorPawn>(ShooterSpectatorClass);
				VictimController->UnPossess();
				VictimController->Possess(ShooterSpectator);
				VictimPS->DeadPlayerControllers.Add(VictimController);

				// If in Team mode, set view to a teammate. If all teammates are dead, set view to a random alive player
				AShooterPlayerState* AliveShooterPS = nullptr;
				if (ShooterGS->bIsTeamMode)
				{
					if (VictimPS)
					{
						TArray<AShooterPlayerState*> AliveTeamMembers = ShooterGS->GetAliveTeamMembers(VictimPS->GetGenericTeamId());
						AliveShooterPS = AliveTeamMembers.Num() > 0 ? AliveTeamMembers[0] : ShooterGS->GetRandomAlivePlayer();
					}
				}
				// If in FreeForAll, set view to a random alive player
				else
				{
					AliveShooterPS = ShooterGS->GetRandomAlivePlayer();
				}

				//auto AlivePlayerPC = Cast<AShooterPlayerController>(AliveShooterPS->GetPlayerController());
				if (AliveShooterPS)
				{
					for (int32 i = 0; i < VictimPS->DeadPlayerControllers.Num(); i++)
					{
						AliveShooterPS->DeadPlayerControllers.Add(VictimPS->DeadPlayerControllers[i]);
						VictimPS->DeadPlayerControllers[i]->SetViewTargetWithBlend(AliveShooterPS->GetPawn());
					}
				}
			}
		}
	}
	Destroy();
	*/
}

void AShooterCharacter::SetVictimView(AShooterPlayerController* VictimController, AShooterPlayerState* VictimPlayerState, AShooterPlayerState* AttackerPlayerState, int32 RespawnsLeft)
{
	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return;
	AttackerPlayerState->DeadPlayerControllers.Add(VictimController);
	for (int32 i = 0; i < VictimPlayerState->DeadPlayerControllers.Num(); i++)
	{
		AttackerPlayerState->DeadPlayerControllers.Add(VictimPlayerState->DeadPlayerControllers[i]);
	}

	for (int32 i = 0; i < AttackerPlayerState->DeadPlayerControllers.Num(); i++)
	{
		AttackerPlayerState->DeadPlayerControllers[i]->SetViewTargetWithBlend(AttackerPlayerState->GetPawn());
	}
}

void AShooterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && ElimMontage)
	{
		ElimMontage->bEnableAutoBlendOut = false;
		AnimInstance->Montage_Play(ElimMontage);
		AnimInstance->Montage_JumpToSection(FName("Eliminate"));

	}
}

void AShooterCharacter::PlayWinningMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if(AnimInstance && WinMontage)
	{
		AnimInstance->Montage_Play(WinMontage);
		AnimInstance->Montage_JumpToSection(FName("Win"));
	}
}

void AShooterCharacter::PlayAITauntSound(AActor* AttackerCharacter)
{
	if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(AttackerCharacter))
	{
		EnemyInterface->Execute_PlayTauntSound(AttackerCharacter);
	}
}

void AShooterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
		SetAllSkeletalMeshMaterials(false, DynamicDissolveMaterialInstance);
	}
}

void AShooterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AShooterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->PlayFromStart();
	}
}

void AShooterCharacter::StopMatch()
{
	ShooterPlayerController = GetShooter_PC();
	if (HasAuthority())
	{
		OnTakeAnyDamage.RemoveDynamic(this, &AShooterCharacter::ReceiveDamage);
	}

	if (bHandShield) ShieldOn(false);
	if (IsLocallyControlled()) 
	{
		if (HoverState == EHoverState::EHS_HoverStart)
		{
			HoverState = EHoverState::EHS_HoverStop;
			TriggerHover();
			OnRep_HoverState(EHoverState::EHS_HoverStart);
		}
		if (bGamePaused) PauseButtonPressed();
	}
	DisableInput(ShooterPlayerController);
	GetWorldTimerManager().ClearTimer(AutoFireTimer);
	bCanFire = false;
	bDisableGameplay = true;
	//GetCharacterMovement()->DisableMovement();
}

void AShooterCharacter::SetHairMaterialForLeo(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst)
{
	if (ShooterMeshName == TEXT("Leo"))
	{
		TArray<UActorComponent*> ActorComps;
		ActorComps = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("HairTag"));
		if (ActorComps.IsValidIndex(0) && ActorComps[0])
		{
			auto HairMesh = Cast<USkeletalMeshComponent>(ActorComps[0]);
			if (HairMesh)
			{
				if (!bSetDefaultMat && DynamicInst)
				{
					HairMesh->SetMaterial(0, DynamicInst);
				}
				else
				{
					if (LeoHairMaterial) HairMesh->SetMaterial(0, LeoHairMaterial);
				}
			}
		}
	}
}

void AShooterCharacter::GetHairMaterialForLeo()
{
	if (ShooterMeshName == TEXT("Leo"))
	{
		TArray<UActorComponent*> ActorComps;
		ActorComps = GetComponentsByTag(USkeletalMeshComponent::StaticClass(), FName("HairTag"));
		if (ActorComps.IsValidIndex(0) && ActorComps[0])
		{
			auto HairMesh = Cast<USkeletalMeshComponent>(ActorComps[0]);
			if (HairMesh)
			{
				LeoHairMaterial = HairMesh->GetMaterial(0);
			}
		}
	}
}

void AShooterCharacter::GetAllSkeletalMeshMaterials()
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

void AShooterCharacter::SetAllSkeletalMeshMaterials(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst)
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

void AShooterCharacter::SetVisibilityOfShooter(bool bVisible)
{
	TArray<UActorComponent*> ActorComps;
	GetComponents(ActorComps, true);
	for (auto ActorMesh : ActorComps)
	{
		auto SkelMesh = Cast<USkeletalMeshComponent>(ActorMesh);
		if (SkelMesh)
		{
			SkelMesh->SetVisibility(bVisible);
		}
	}
}

void AShooterCharacter::ReplenishShieldStrength()
{
	FTimerHandle HandShieldReplinishTimer;
	GetWorldTimerManager().SetTimer(HandShieldReplinishTimer, this, &AShooterCharacter::ReplenishShieldStrength, 0.5f);
	
	if (HandShieldStrength < 100.f && !bHandShield)
	{
		const float StrengthToUpdate = 1.f;
		HandShieldStrength = (FMath::Clamp(HandShieldStrength + StrengthToUpdate, 0.f, MaxHandShieldStrength));
		OnRep_HandShieldStrength();
	}
	else if (bHandShield)
	{
		const float StrengthToUpdate = 1.f;
		HandShieldStrength = (FMath::Clamp(HandShieldStrength - StrengthToUpdate, 0.f, MaxHandShieldStrength));
		OnRep_HandShieldStrength();
	}

	//if (bHandShield)
	//{
	//	UpdateShieldColor(HandShieldStrength);
	//}

	//UE_LOG(LogTemp, Warning, TEXT("HandShieldStrength = %f"), HandShieldStrength);
}

void AShooterCharacter::ReplenishHoverSystem()
{
    float FuelDelta = 0.f;

    // Consumption (always apply if active, regardless of fuel level)
    if (HoverState == EHoverState::EHS_HoverStart || HoverState == EHoverState::EHS_HoverRush)
    {
        FuelDelta -= 4.f;
    }
    // Regeneration (only if below max and in regen state)
    else if (HoverSystemFuel < MaxHoverFuel)
    {
        if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking || HoverState == EHoverState::EHS_HoverFinish)
        {
            FuelDelta += 2.f;
        }
        else if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling && HoverState != EHoverState::EHS_HoverStart)
        {
            FuelDelta += 1.f;
        }
    }

    if (FuelDelta != 0.f)
    {
        HoverSystemFuel = FMath::Clamp(HoverSystemFuel + FuelDelta, 0.f, MaxHoverFuel);
        OnRep_HoverSystemFuel();
    }
}

void AShooterCharacter::ReplenishChargeAttackStrength()
{
	FTimerHandle ChargeAttackReplinishTimer;
	GetWorldTimerManager().SetTimer(ChargeAttackReplinishTimer, this, &AShooterCharacter::ReplenishChargeAttackStrength, 0.3f);
	
	if (ChargeAttackStrength < 100.f)
	{
		const float StrengthToUpdate = 2.f;
		ChargeAttackStrength = (FMath::Clamp(ChargeAttackStrength + StrengthToUpdate, 0.f, 100.f));
		OnRep_ChargeAttackStrength();
	}
}

void AShooterCharacter::ReplenishDashCharge(float DeltaTime)
{
	if (DashCharge <= 100.f - 1e-4)
	{
		const float ChargeThisFrame = (100 / 5) * DeltaTime; // (Amount to charge / time to charge)
		DashCharge = FMath::Clamp(DashCharge + ChargeThisFrame, 0.f, 100.f);
		DashChargeDelegate.Broadcast(DashCharge / 100.f);
		//UE_LOG(LogTemp, Warning, TEXT("ReplenishDashCharge: %f"), DashCharge);
	}
}

void AShooterCharacter::ShieldOn(bool bIsHandShield)
{
	bHandShield = bIsHandShield;
	ShieldTrigger(bHandShield);
	ServerSetShield(bIsHandShield);
	//ShieldMesh->SetVisibility(bHandShield);
	bIsHandShield ? ShieldMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly) : ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (IsLocallyControlled()) bHandShieldButtonPressed = bIsHandShield;

	if (bHandShield)
	{
		if (ShieldOnSound)
		{
			UGameplayStatics::SpawnSoundAttached(
				ShieldOnSound, // sound cue (USoundBase)
				GetMesh(), // mesh to attach to
				FName("RightHandSocket"),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);
		}
	}
}

float AShooterCharacter::GetAngleBetweenPlayers(const FVector& player1, const FVector& player2)
{
    float dotProd = FVector::DotProduct(player1, player2);
	float det = player1.X * player2.Y - player1.Y * player2.X; // determinant
	float angle = FMath::Atan2(det, dotProd) * 180 / PI;
	if (angle < 0.f)
	{
		return angle + 360.f;
	}
	return angle;
}

void AShooterCharacter::UpdateTeamMemberAttributes(float NewHealth, float NewShield)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (ShooterGI && ShooterGI->bTeamMode && !ShooterGI->bPracticeMode)
	{
		ShooterPS = GetShooter_PS();
		if (ShooterPS)
		{
			if (NewHealth != -1000.f) ShooterPS->CharacterHealth = NewHealth / MaxHealth;
			if (NewShield != -1000.f) ShooterPS->CharacterShield = NewShield / MaxShield;
		}
	}
}

void AShooterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (!HasAuthority()) return;

	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS->GameMatchState == EGameMatchState::EMS_Idle || ShooterGS->GameMatchState == EGameMatchState::EMS_Warn) return;

	float DamageToHealth = 0.0f;
	float OldShield = Shield;
	float OldHealth = Health;

	AShooterPlayerController* VictimController = nullptr; AShooterPlayerController* AttackerController = nullptr;
	AShooterPlayerState* VictimPS = nullptr; AShooterPlayerState* AttackerPS = nullptr;
	UCharacterGlobalFuncs::GetShooterReferences(VictimController, AttackerController, VictimPS, AttackerPS, DamagedActor, DamageCauser, InstigatorController);
	if (CheckFriendlyHit(DamagedActor, DamageCauser, VictimPS, AttackerPS, ShooterGS)) return;

	float DmgRemaining = FMath::Abs(FMath::Min(Shield - Damage, 0.f));
	if (Shield - Damage >= 0.f || (Shield > 0.f && Shield - Damage < 0.f ))
	{
		Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
		OnRep_Shield(OldShield);
	}

	DamageToHealth = DmgRemaining;
	if (DamageToHealth > 0.f)
	{
		Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);
		OnRep_Health(OldHealth);
	}

	//AShooterPlayerController* AttackerController = Cast<AShooterPlayerController>(InstigatorController);
	AActor* InstigatorActor = InstigatorController ? InstigatorController->GetPawn() : DamageCauser;
	UCharacterGlobalFuncs::UpdateDamageStat(DamagedActor, DamageCauser, VictimPS, AttackerPS, Damage, Health, DamageType);
	
    if (Health <= 1e-4f)
    {
		OnTakeAnyDamage.RemoveDynamic(this, &AShooterCharacter::ReceiveDamage);
		if (OwnedPawn)
		{
			auto OwnedHelicopter = Cast<AHelicopter>(OwnedPawn);
			if (OwnedHelicopter)
			{
				//VictimController = GetShooter_PC();
				//VictimPS = GetShooter_PS();
				OwnedHelicopter->UnpossessFromHeli(this, false);
			}
		}
		HandlePlayerElimmed(DamagedActor, DamageCauser, VictimController, AttackerController, VictimPS, AttackerPS, ShooterGS);
		if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(InstigatorActor))
		{
			EnemyInterface->Execute_SetTarget(InstigatorActor, this, true);
		}
    }
    else
    {
		if (DamagedActor != nullptr && InstigatorActor != nullptr && DamagedActor != InstigatorActor && VictimController)
		{
			VictimController->ClientShowAttackerLocation(InstigatorActor, DamagedActor, Damage);
		}
        Multicast_PlayBigDamageMontage(Damage);
    }
}

bool AShooterCharacter::CheckFriendlyHit(const AActor* DamagedCharacter, const AActor* AttackerCharacter, 
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

FGameplayTagContainer AShooterCharacter::GetStateTags_Implementation()
{
	return CharacterStateTags;
}

// Used for ShooterInterface
AShooterCharacter* AShooterCharacter::GetShooterCharacter_Implementation()
{
    return this;
}

// Used for ShooterInterface
FVector AShooterCharacter::GetPawnLocation_Implementation()
{
	return GetActorLocation();
}

// Used for ShooterInterface
float AShooterCharacter::GetPawnRotationYaw_Implementation()
{
	return GetMesh()->GetComponentRotation().Yaw;
}

bool AShooterCharacter::CanTarget_Implementation()
{
	return GetCanTarget() && GetHealth() > 0.f && !GetGhostMode();
}

void AShooterCharacter::OnValidTarget_Implementation(AActor* AttackerAI)
{
	TrackingAI = AttackerAI;
}

void AShooterCharacter::HandlePlayerElimmed(AActor* DamagedCharacter, AActor* DamageCauserCharacter,
	AShooterPlayerController* VictimController, AShooterPlayerController* ShooterAttackerController, 
	AShooterPlayerState* VictimShooterPS, AShooterPlayerState* DamageCauserShooterPS, 
	AShooterGameState* ShooterGameState)
{

	ShooterGameMode = GetShooter_GM();
	if (ShooterGameMode)
	{
		//UE_LOG(LogTemp, Warning, TEXT("HandlePlayerElimmed"));
		ShooterGameMode->PlayerEliminated(this, VictimController, ShooterAttackerController, VictimShooterPS, DamageCauserShooterPS);
	}

	if (ShooterGameState)
	{
		ShooterGameState->UpdateElimmedPlayerCount();
		//ShooterGameState->CheckWinners();
	}
}

AShooterGameMode* AShooterCharacter::GetShooter_GM()
{
    // Add AShooterGameMode* ShooterGameMode in .h;
	UWorld* World = GetWorld();
	if (World)
	{
		ShooterGameMode = ShooterGameMode == nullptr ? World->GetAuthGameMode<AShooterGameMode>() : ShooterGameMode;
	}
    
    if (ShooterGameMode)
    {
        return ShooterGameMode;
    }
    return nullptr;
}

AShooterPlayerController* AShooterCharacter::GetShooter_PC()
{
	if (OwnedPawn == nullptr)
	{
		ShooterPlayerController = ShooterPlayerController == nullptr ? Cast<AShooterPlayerController>(Controller) : ShooterPlayerController;
	}
	else
	{
		ShooterPlayerController = Cast<AShooterPlayerController>(OwnedPawn->Controller);
	}
    
    if (ShooterPlayerController)
    {
        return ShooterPlayerController;
    }
    return nullptr;
}

AShooterGameState* AShooterCharacter::GetShooter_GS()
{
	UWorld* World = GetWorld();
	if (World)
	{
		AShooterGameState* ShooterGS = World->GetGameState<AShooterGameState>();
		if (ShooterGS) return ShooterGS;
	}
	return nullptr;
}

AShooterPlayerState* AShooterCharacter::GetShooter_PS()
{
	if (OwnedPawn == nullptr)
	{
		ShooterPS = ShooterPS == nullptr ? Cast<AShooterPlayerState>(GetPlayerState()) : ShooterPS;
	}
	else
	{
		ShooterPS = ShooterPS == nullptr ? Cast<AShooterPlayerState>(OwnedPawn->GetPlayerState()) : ShooterPS;
	}
	
	if (ShooterPS)
	{
		return ShooterPS;
	}
	return nullptr;
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();

	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	//if (EquippedWeapon)
	//{
	//	EquippedWeapon->Destroy();
	//}
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	if (bScreenToWorld)
	{
		// Trace from Crosshair world location outward
        FCollisionQueryParams CollisionParam;
        CollisionParam.AddIgnoredActor(this);
		if (EquippedItem) CollisionParam.AddIgnoredActor(EquippedItem);
		if (Inventory.Num() > 0 && Inventory[0]) CollisionParam.AddIgnoredActor(Inventory[0]);

		const FVector Start{ ShotPosition };
		const FVector End{ Start + 50000.f * ShotDirection };
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
	return false;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AShooterCharacter::AimingActionButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) AimingButtonPressed();
}

void AShooterCharacter::AimingActionButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) AimingButtonReleased();
}

void AShooterCharacter::AimingButtonPressed()
{
    if (bDisableGameplay || bUsingItem || bEmoteButtonPressed || bChargeAttack ||
        EquippedItem == nullptr || CombatState == ECombatState::ECS_Mantle || HoverState == EHoverState::EHS_HoverRush) return;
    
    if (InventorySelect == 0)  // Use predicted InventorySelect for mode check
    {
        bHandShieldButtonPressed = true;
        ShieldOn(true);
    }
    else if (InventorySelect != 0)
    {
        bAimingButtonPressed = true;
        if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping)
        {
            SetAiming(true);
            //OnFPSAiming(true);
        }
    }
}

void AShooterCharacter::AimingButtonReleased()
{
    if (bDisableGameplay || bUsingItem || EquippedItem == nullptr) return;
    
    if (InventorySelect == 0)  // Use predicted InventorySelect for mode check
    {
        bHandShieldButtonPressed = false;
        ShieldOn(false);
    }
    else
    {
        bAimingButtonPressed = false;
        SetAiming(false);
        //OnFPSAiming(false);
    }
}

/*
void AShooterCharacter::OnFPSAiming(bool bStartFPS)
{
	if (!EquippedWeapon || !FPSCameraPivot || !FollowCameraFPS || !FollowCamera) return;
	if (bStartFPS)
	{

		if (EquippedWeapon)
		{
			FPSCameraPivot->AttachToComponent(EquippedWeapon->GetItemMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("IronSightSocket"));
			FPSCameraPivot->SetRelativeLocation(FVector(0, 0, 10));
			GetMesh()->HideBoneByName(FName("head"), EPhysBodyOp::PBO_Term);
			SetMaterialsOnFPS(true);
		}
		FollowCameraFPS->SetActive(true);
		FollowCamera->SetActive(false);
	}
	else
	{
		GetMesh()->UnHideBoneByName(FName("head"));
		SetMaterialsOnFPS(false);
		FollowCameraFPS->SetActive(false);
		FollowCamera->SetActive(true);
	}
}
*/

void AShooterCharacter::SetMaterialsOnFPS(bool bStartFPS)
{
	if (bStartFPS)
	{
		if (!TransparentMaterial) return;
		for (auto MatPair : FPSMaterialsToHideMap)
		{
			GetMesh()->SetMaterial(MatPair.Key, TransparentMaterial);
		}
	}
	else
	{
		for (auto MatPair : FPSMaterialsToHideMap)
		{
			if (MatPair.Value) GetMesh()->SetMaterial(MatPair.Key, MatPair.Value);
		}
	}
}

void AShooterCharacter::GetMaterialsToHideForFPS()
{
	switch (CharacterSelected)
	{
	case ECharacterProperty::ECP_Leo:
		if (GetMesh()->GetMaterial(7)) FPSMaterialsToHideMap.Add(7, GetMesh()->GetMaterial(7));
		break;
	case ECharacterProperty::ECP_Maverick:
		if (GetMesh()->GetMaterial(0)) FPSMaterialsToHideMap.Add(0, GetMesh()->GetMaterial(0));
		break;
	case ECharacterProperty::ECP_Raven:
		if (GetMesh()->GetMaterial(5)) FPSMaterialsToHideMap.Add(5, GetMesh()->GetMaterial(5));
		break;
	case ECharacterProperty::ECP_Trinity:
		if (GetMesh()->GetMaterial(0)) FPSMaterialsToHideMap.Add(0, GetMesh()->GetMaterial(0));
		break;
	
	default:
		break;
	}
}

void AShooterCharacter::OnRep_Aiming()
{
    if (IsLocallyControlled())
    {
        bAiming = bAimingButtonPressed;
    }
}

void AShooterCharacter::OnRep_HandShield()
{
	if (IsLocallyControlled())
	{
		bHandShield = bHandShieldButtonPressed;
	}
	//ShieldMesh->SetVisibility(bHandShield);
	ShieldTrigger(bHandShield);
	bHandShield ? ShieldMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly) : ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AShooterCharacter::SetAiming(bool bIsAiming)
{
    if (EquippedWeapon == nullptr || EquippedItem == nullptr) return;

    bAiming = bIsAiming;
    ServerSetAiming(bIsAiming);

    // Set predicted aiming state for movement (replaces Set_TargetMovementSpeed)
    if (!IsGrappling() && (HoverState != EHoverState::EHS_HoverStart && HoverState != EHoverState::EHS_HoverRush))
    {
        bIsAiming ? SetMovementState(EMovementModifiers::EMM_Crouch) : SetMovementState(EMovementModifiers::EMM_Normal);
    }

    if (IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Sniper && EquippedItem->GetItemType() == EItemType::EIT_Weapon)
    {
        ShowSniperScopeWidget(bIsAiming);
    }

    if (IsLocallyControlled())
	{
		bAimingButtonPressed = bIsAiming;
		if (EquippedWeapon && EquippedWeapon->ADSSettings.bUseRTTScope)
		{
			EquippedWeapon->ActivateScope(bAiming);  // Direct call, or via weapon if preferred
		}
	}
}

void AShooterCharacter::ServerSetAiming_Implementation(bool bIsAiming)
{
    bAiming = bIsAiming;

    // No longer set movement speed here (handled predictively)
}

void AShooterCharacter::ServerSetShield_Implementation(bool bIsShield)
{
	bHandShield = bIsShield;
	//bIsShield ? Set_TargetMovementSpeed(CrouchMovementSpeed) : Set_TargetMovementSpeed(BaseMovementSpeed);

	//ShieldMesh->SetVisibility(bHandShield);
	ShieldTrigger(bHandShield);
	bHandShield ? ShieldMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly) : ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

bool AShooterCharacter::IsPracticeMode()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
    if (ShooterGI)
    {
        return ShooterGI->bPracticeMode;
    }
    return false;
}

void AShooterCharacter::SelectButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay || !Value.Get<bool>()) return;
	//DropAllItems();
	
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS)
	{
		if (IsPracticeMode() && bCanSpawnAI && bCanSpawnAITimer)
		{
			bCanSpawnAITimer = false;
			ShooterGS->SpawnAIShooters(GetShooter_PC());
			if (SpawnAISoundPractice && ShooterGS->AIShooterArray.Num() != 10) UGameplayStatics::PlaySound2D(this, SpawnAISoundPractice);
			if (!bCanTarget) bCanTarget = true;
			FTimerHandle SpawnAITimer;
			GetWorldTimerManager().SetTimer(SpawnAITimer, this, &AShooterCharacter::SetSpawnAI, 1.5f);
		}

		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI && ShooterGI->GameType == EGameModeType::EGMT_Lobby && bCanLobbyReadyUp) SetLobbyReady(!bIsLobbyReady);
		if (ShooterGS->GameMatchState <= EGameMatchState::EMS_Warn) return;
	}


	if (CombatState == ECombatState::ECS_Unoccupied)
	{
		if (TraceHitItem)
		{
			TraceHitItem->StartItemCurve(this, bConstantItemSwap);
			TraceHitItem = nullptr;
			//auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			//SwapItem(TraceHitWeapon);
		}

		AActor* OwningActor = GetOwningActor(FName("Helicopter"));
		if (OwningActor)
		{
			auto HeliActor = Cast<AHelicopter>(OwningActor);
			if (HeliActor && HeliActor->bDriveHelicopter && HeliActor->OwnedShooter == nullptr)
			{
				ServerPossessHelicopter(HeliActor);
				
			}
		}

		AActor* OwningDoorActor = GetOwningActor(FName("HelicopterDoor"));
		if (OwningDoorActor)
		{
			auto HeliActor = Cast<AHelicopter>(OwningDoorActor);
			HeliActor->CheckDoorAnimation();
			if (HeliActor && HeliActor->bDoorOverlap && !HeliActor->bDoorAnimPlaying)
			{
				ServerHelicopterDoor(HeliActor);
			}
		}
	}
}

void AShooterCharacter::ServerSetCurrentItemToSwap_Implementation(AItem* ClientItem)
{
    CurrentItemToSwap = nullptr;
    if (ClientItem && Inventory.Contains(ClientItem))
    {
        CurrentItemToSwap = ClientItem;
    }
}

void AShooterCharacter::SetLobbyReady(bool bIsReady)
{
	ShooterPS = GetShooter_PS();
	if (ShooterPS)
	{
		bIsLobbyReady = bIsReady;
		//auto ShooterPC = Cast<AShooterPlayerController>(PlayerController);
		const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
		if (GetNetMode() == NM_ListenServer)
		{
			AShooterPlayerController* HostPC = GetShooter_PC();
			if (HostPC)
			{
				HostPC->GeneralMessage(TEXT("Hosts Can't Ready Up"), TEXT("Purple"));
			}
			return;
		}
		ShooterPS->ServerOnLobbyReady(bIsReady, Ids.PlayerID, Ids.PlayerName);
		OnLobbyReadyDelegate.Broadcast(bIsReady);
	}
}

void AShooterCharacter::SetSpawnAI()
{
	bCanSpawnAITimer = true;
}

void AShooterCharacter::SelectButtonReleased(const FInputActionValue& Value)
{
	
}

void AShooterCharacter::ServerHelicopterDoor_Implementation(AHelicopter* HelicopterActor)
{
	HelicopterActor->bHeliDoorOpen = !HelicopterActor->bHeliDoorOpen;
	HelicopterActor->OnRep_HeliDoorOpen();
}

void AShooterCharacter::ServerPossessHelicopter_Implementation(AHelicopter* HelicopterActor)
{
	ShooterPlayerController = GetShooter_PC();
	if (ShooterPlayerController && HelicopterActor && HelicopterActor->GetOwner() == nullptr)
	{
		ShooterPlayerController->UnPossess();
		ShooterPlayerController->Possess(HelicopterActor);
		HelicopterActor->SetOwner(ShooterPlayerController);
		HelicopterActor->OwnedShooter = this;
		HelicopterActor->SetAmmo(this);

		OwnedPawn = HelicopterActor;
		MulticastOnPossessHeli(HelicopterActor, true);

		StartPawnPossessTimer();
	}
}

void AShooterCharacter::MulticastOnPossessHeli_Implementation(AHelicopter* HeliActor, bool bHeliPossessed)
{
	if (bHeliPossessed)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
		AttachToActor(HeliActor, AttachmentRules);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		DetachFromActor(DetachmentTransformRules);
		GetCapsuleComponent()->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		//FollowCamera->ResetRelativeTransform();
    	//FTimerHandle DetachCharTimer;
    	//GetWorldTimerManager().SetTimer(DetachCharTimer, this, &AShooterCharacter::ResetCharacterRotation, 2.f);
		if (HasAuthority())
		{
			GetWorldTimerManager().ClearTimer(PawnPossessTimer);
		}
	}
}

void AShooterCharacter::ResetCharacterRotation()
{
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
}

void AShooterCharacter::StartPawnPossessTimer()
{
	GetWorldTimerManager().ClearTimer(PawnPossessTimer);
	GetWorldTimerManager().SetTimer(PawnPossessTimer, this, &AShooterCharacter::OnPawnPossesFinish, 120.f);
}

void AShooterCharacter::OnPawnPossesFinish()
{
	ShooterPS = GetShooter_PS();
	if (ShooterPS)
	{
		ShooterPS->AddToXPMetric(EProgressMetric::EPM_DriveAttackDroneFor2Min, 1);
	}
}

// No longer needed...AItem has GetInterpLocation
/*
FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation{ FollowCamera->GetComponentLocation() };
	const FVector CameraForward{ FollowCamera->GetForwardVector() };
	//Desired location = CameraWOrldLocation + A * Forward + B * Up
	return CameraWorldLocation +
		CameraInterpDistance * CameraForward +
		FVector(0.f, 0.f, CameraInterpElevation);
}
*/

void AShooterCharacter::OnAmmoSphereOverlap_Implementation(AAmmo* OverlappedAmmo)
{
	if (!IsLocallyControlled() || GetPlayerEliminated() || bIsAI) return;

	OverlappedAmmo->StartItemCurve(this);
	OverlappedAmmo->GetAmmoCollisionSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision); //Stop all collisions and prevents spamming
}

void AShooterCharacter::OnItemPickedUp_Implementation(AActor* ItemActor, const FString& ItemType)
{
	if (ItemType.Equals(TEXT("Ammo")))
	{
		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepWorld, true);
		ItemActor->AttachToActor(this, AttachmentRules);
	}
	else
	{
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName("RightHandSocket");
		if (HandSocket)
		{
			// Attach the weapon to the hand socket RightHandSocket
			HandSocket->AttachActor(ItemActor, GetMesh());
		}
	}
}

void AShooterCharacter::OnUpdateHUDBoostAmount_Implementation(int32 BoostAmount, ABoostItem* BoostItem)
{
	if (IsLocallyControlled())
	{
		if (EquippedItem && EquippedItem == BoostItem) GetHudAmmoDelegate().Broadcast(BoostAmount);
	}
}

// To be called as the ServerRPC's implementation and it will resolve self call as RPC when needed
void AShooterCharacter::GetPickupItem_Implementation(AItem* Item)
{
	if (!Item || bPlayerEliminated) return;

	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}

	// If this is not the authority call this as an RPC
	if (!HasAuthority()) {
		GetPickupItem(Item);
		return;
	}

	Item->SetOwner(this);
	if (Item->Implements<UItemInterface>()) Item->OnItemPickedUp(this);
}

bool AShooterCharacter::BoostInInventory(ABoostItem* TakenBoost)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		ABoostItem* BoostItem = Cast<ABoostItem>(Inventory[i]);
		if (BoostItem && TakenBoost && TakenBoost->GetBoostType() == BoostItem->GetBoostType())
		{
			return true;
		}
	}
	return false;
}

void AShooterCharacter::TakeAmmo(AAmmo* Ammo)
{
	if (EquippedWeapon == nullptr) { return; }
	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if (EquippedWeapon->GetAmmo() == 0)
		{
			Reload();
		}
	}
	//Ammo->Destroy();
}

void AShooterCharacter::Client_TakeAmmo_Implementation(AAmmo* Ammo)
{
	if (EquippedWeapon == nullptr) { return; }
	if (Ammo && EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if (EquippedWeapon->GetAmmo() == 0)
		{
			Reload();
		}
	}
}

void AShooterCharacter::UpdateAmmoForHelicopter(EAmmoType InAmmoType, int32 UpdatedAmmo)
{
	AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(InAmmoType)] = UpdatedAmmo;
}

void AShooterCharacter::Server_TakeAmmo(AAmmo* Ammo)
{
	// TakeAmmo(Ammo);
	int32 AmmoCount = GetAmmoAmount(Ammo->GetAmmoType());
	if (AmmoCount != -1)
	{
		AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(Ammo->GetAmmoType())] += Ammo->GetItemCount();
		if (Ammo) Ammo->Destroy();
	}
	else
	{
		AmmoStruct.AmmoType.Add(Ammo->GetAmmoType());
		AmmoStruct.AmmoAmount.Add(Ammo->GetItemCount());
		AmmoInventory.Add(Ammo);
		if (Ammo) Ammo->SetAmmoState(EItemState::EIS_Equipped);
		Client_TakeAmmo(Ammo);
	}
	OnRep_AmmoStruct();
	
	//Multicast_TakeAmmo(Ammo);
}

void AShooterCharacter::OnRep_AmmoStruct()
{
	if (IsLocallyControlled())
	{
		HudCarriedAmmoDelegate.Broadcast();
	}
}

void AShooterCharacter::Multicast_TakeAmmo_Implementation(AAmmo* Ammo)
{
	TakeAmmo(Ammo);
}

void AShooterCharacter::InitialzeAmmoMap()
{
	//AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	//AmmoMap.Add(EAmmoType::EAT_45mm, Starting45mmAmmo);
	//AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
	//AmmoMap.Add(EAmmoType::EAT_Shells, StartingShotgunAmmo);
	//AmmoMap.Add(EAmmoType::EAT_GrenadeRounds, StartingGrenadeLauncherAmmo);
}

void AShooterCharacter::Rush()
{
	if (HoverState == EHoverState::EHS_HoverRush && IsLocallyControlled())
	{
		FVector RushDirection = UKismetMathLibrary::Conv_RotatorToVector(GetControlRotation());
		float ZRotation = RushDirection.Z;
		float Z1, Z2;
		Z1 = UKismetMathLibrary::InRange_FloatFloat(ZRotation, -1.f, -0.1f) ? ZRotation : 0.f;
		Z2 = UKismetMathLibrary::InRange_FloatFloat(ZRotation, 0.1f, 1.f) ? ZRotation : 0.f;
		RushDirection.Z = Z1 + Z2;
		AddMovementInput(RushDirection, 1.f);
	}
}

void AShooterCharacter::HoverActionButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>()) HoverButtonPressed();
}

void AShooterCharacter::HoverActionButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>()) HoverButtonReleased();
}

void AShooterCharacter::HoverButtonPressed()
{
	if (bGrappleThrown || bSuperPunch || bUsingItem || HoverSystemFuel <= 1e-2 || bDisableGameplay || bHoverButtonPressed || bGravityPull ||
		GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying || bEmoteButtonPressed || !bHoverReady || bDash || CombatState == ECombatState::ECS_Mantle) return;

	//UE_LOG(LogTemp, Display, TEXT("HoverButtonPressed"));
	//if (HoverState == EHoverState::EHS_HoverStart) return;
	bHoverReady = false;
	bHoverButtonPressed = true;
	EHoverState OldHoverState = HoverState;
	HoverState = EHoverState::EHS_HoverStart;
	TriggerHover();
	if (!HasAuthority()) OnRep_HoverState(OldHoverState);

	//SetMovementState(EMovementModifiers::EMM_HoverStart);

	FTimerHandle HoverReadyTimer;
	GetWorldTimerManager().SetTimer(HoverReadyTimer, this, &AShooterCharacter::SetHoverReady, 0.15f);
}

void AShooterCharacter::SetHoverReady()
{
	bHoverReady = true;
}

void AShooterCharacter::HoverButtonReleased()
{
	if (bGrappleThrown || bSuperPunch || bDisableGameplay || !bHoverButtonPressed) return;
	bHoverButtonPressed = false;
	EHoverState OldHoverState = HoverState;
	HoverState = EHoverState::EHS_HoverStop;
	//SetMovementState(EMovementModifiers::EMM_HoverStop);
	TriggerHover();
	if (!HasAuthority()) OnRep_HoverState(OldHoverState);
}

void AShooterCharacter::TriggerHover()
{
	SetHoverProperties();
	ServerTriggerHover(HoverState);
}

void AShooterCharacter::ServerTriggerHover_Implementation(EHoverState InHoverState)
{
	HoverState = InHoverState;
	if (GetNetMode() == NM_ListenServer || GetNetMode() == NM_Standalone || bIsAI) OnRep_HoverState(InHoverState);
	SetHoverProperties();
	if (bStopFlying) bStopFlying = false;
}

void AShooterCharacter::OnRep_HoverState(EHoverState OldHoverState)
{
	switch (HoverState)
	{
	case EHoverState::EHS_HoverStart:
		{
			if (FallingSoundComponent && CyberBootsSound)
			{
				FallingSoundComponent->SetSound(CyberBootsSound);
				FallingSoundComponent->Play(0.f);
			}
			if (RushEffectNiagComponent) RushEffectNiagComponent->Deactivate();
			if (IsLocallyControlled())
			{
				SetMovementState(EMovementModifiers::EMM_HoverStart);
			}
			if (CyberBootEffectComponentLeft && CyberBootEffectComponentRight)
			{
				FLinearColor HoverColor(1.0f, 1.0f, 1.0f, 1.0f);
				CyberBootEffectComponentLeft->SetVariableLinearColor(TEXT("User.SmokeColor"), HoverColor);
				CyberBootEffectComponentRight->SetVariableLinearColor(TEXT("User.SmokeColor"), HoverColor);
				CyberBootEffectComponentLeft->Activate();
				CyberBootEffectComponentRight->Activate();
			}	
		}
		break;
	case EHoverState::EHS_HoverRush:
		{
			if (FallingSoundComponent && CyberBootsRushSound)
			{
				FallingSoundComponent->SetSound(CyberBootsRushSound);
				FallingSoundComponent->Play(0.f);
			}
			if (CyberBootEffectComponentLeft && CyberBootEffectComponentRight)
			{
				//FLinearColor HoverColor(0.0f, 1.0f, 50.0f, 1.0f);
				//CyberBootEffectComponentLeft->SetVariableLinearColor(TEXT("User.SmokeColor"), HoverColor);
				//CyberBootEffectComponentRight->SetVariableLinearColor(TEXT("User.SmokeColor"), HoverColor);
				CyberBootEffectComponentLeft->Deactivate();
				CyberBootEffectComponentRight->Deactivate();
			}
			if (RushEffectSystem)
			{
				RushEffectNiagComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
					RushEffectSystem,
					GetRootComponent(),
					FName(),
					GetActorLocation(),
					GetActorRotation(),
					EAttachLocation::KeepWorldPosition,
					false
				);
			}
		}
		break;
	case EHoverState::EHS_HoverStop:
		{
			if (FallingSoundComponent && CyberBootsOffSound)
			{
				FallingSoundComponent->SetSound(CyberBootsOffSound);
				FallingSoundComponent->Play(0.f);
			}
			if (CyberBootEffectComponentLeft && CyberBootEffectComponentRight)
			{
				CyberBootEffectComponentLeft->Deactivate();
				CyberBootEffectComponentRight->Deactivate();
			}
			if (RushEffectNiagComponent)
			{
				RushEffectNiagComponent->Deactivate();
			}
		}
		break;
	case EHoverState::EHS_HoverFinish:
		{
			StopAllHoverEffects();
		}
		break;
	default:
		break;
	}
}

void AShooterCharacter::SetHoverProperties()
{
	if (auto MoveComp = GetCharacterMovement())
	{
		switch (HoverState)
		{
		case EHoverState::EHS_HoverStart:
			{
				if (IsLocallyControlled())
				{
					if (CameraBoom)
					{
						CameraBoom->bEnableCameraLag = false;
						CameraBoom->bEnableCameraLagAxisZ = false;
					}
				}
				MoveComp->SetMovementMode(EMovementMode::MOVE_Flying);
				MoveComp->MaxFlySpeed = (4.f / 5.f) * BaseFlySpeed;
				MoveComp->MaxAcceleration = BaseAcceleration / 1.2f;
				MoveComp->AirControl = 2.f * BaseAirControl;
				MoveComp->RotationRate = BaseRotationRate;
			}
			break;
		case EHoverState::EHS_HoverRush:
			{
				if (IsLocallyControlled())
				{
					if (CameraBoom)
					{
						//CameraBoom->bEnableCameraLag = true;
						//CameraBoom->bEnableCameraLagAxisY = true;
						//CameraBoom->CameraLagSpeed = CameraBoom->CameraLagSpeed / 1.5f;
					}
				}
				MoveComp->SetMovementMode(EMovementMode::MOVE_Flying);
				MoveComp->MaxFlySpeed = 1.75f * BaseFlySpeed;
				MoveComp->MaxAcceleration = 5.f * BaseAcceleration;
				MoveComp->AirControl = BaseAirControl;
				MoveComp->RotationRate = FRotator(0.f, 0.f, 720.f);
			}
			break;
		case EHoverState::EHS_HoverFinish:
		case EHoverState::EHS_HoverStop:
			{
				if (IsLocallyControlled())
				{
					if (CameraBoom)
					{
						CameraBoom->bEnableCameraLag = true;
						CameraBoom->bEnableCameraLagAxisZ = true;
						//CameraBoom->CameraLagSpeed = 1.5f * CameraBoom->CameraLagSpeed;
					}
				}
				MoveComp->SetMovementMode(EMovementMode::MOVE_Falling);
				MoveComp->MaxFlySpeed = BaseFlySpeed;
				MoveComp->MaxAcceleration = BaseAcceleration;
				MoveComp->AirControl = BaseAirControl;
				MoveComp->RotationRate = BaseRotationRate;
			}	
			break;
		}
	}
}

void AShooterCharacter::StartHovering()
{
	if (IsLocallyControlled() && HoverState == EHoverState::EHS_HoverStart && bHoverButtonPressed)
	{
		//FVector CurrentLoc = GetActorLocation();
		//CurrentLoc.Z += 500.f;
		AddMovementInput(FVector(0.f, 0.f, 1.f), 1.f); 
		if (HoverSystemFuel <= 1e-2)
		{
			HoverButtonReleased();
		}
	}
	else if (IsLocallyControlled() && HoverState == EHoverState::EHS_HoverRush &&
		bFireButtonPressed && bHoverButtonPressed && InventorySelect == 0)
	{
		Rush();
		if (HoverSystemFuel <= 1e-2)
		{
			HoverButtonReleased();
		}
	}
}

void AShooterCharacter::StopAllHoverEffects()
{
	if (FallingSoundComponent && FallingSoundComponent->IsPlaying())
	{
		FallingSoundComponent->Stop();
	}
	if (CyberBootEffectComponentLeft && CyberBootEffectComponentRight)
	{
		CyberBootEffectComponentLeft->Deactivate();
		CyberBootEffectComponentRight->Deactivate();
	}
	if (RushEffectNiagComponent)
	{
		RushEffectNiagComponent->Deactivate();
	}
}

void AShooterCharacter::StartDash()
{
	if (IsLocallyControlled() && bDash)
	{
		//FVector CurrentLoc = GetActorLocation();
		//CurrentLoc.Z += 500.f;
		
		AddMovementInput(MovingDirection, 1.f); 
	}
}

void AShooterCharacter::StartChargeAttack()
{
	bChargeAttack = true;
	//TargetChargeLocation = GetActorLocation() + 1000.f * GetActorForwardVector();
	CombatState = ECombatState::ECS_FireTimerInProgress;
	if (!IsGrappling()) SetMovementState(EMovementModifiers::EMM_Stop);
	//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	PlayChargeAttackMontage(FName("StartCharge"));
	ServerStartChargeAttack(FName("StartCharge"), TargetChargeLocation);
}

void AShooterCharacter::ServerStartChargeAttack_Implementation(FName MontageSectionName, FVector InTargetChargeLocation)
{
	CombatState = ECombatState::ECS_FireTimerInProgress;
	MulticastStartChargeAttack(MontageSectionName, InTargetChargeLocation);
}

void AShooterCharacter::MulticastStartChargeAttack_Implementation(FName MontageSectionName, FVector InTargetChargeLocation)
{
	//if (!HasAuthority()) TargetChargeLocation = InTargetChargeLocation;
	if (IsLocallyControlled() && !HasAuthority()) return;
	//TargetChargeLocation = InTargetChargeLocation;
	//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	PlayChargeAttackMontage(FName("StartCharge"));
}

void AShooterCharacter::RunChargeAttack(float DeltaTime)
{
    if (bInterpChargeAttack)
    {
		if (IsLocallyControlled())
		{
			AddMovementInput((TargetChargeLocation - GetActorLocation()), 2.f);
			//FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetChargeLocation, DeltaTime, 3.f);
			//GetCharacterMovement()->AddForce(1000.f * (TargetChargeLocation - GetActorLocation()));
			//SetActorLocation(NewLocation);
			//ClientRunChargeAttack(GetActorLocation(), GetVelocity(), DeltaTime);
		}
		else
		{
			//SetActorLocation(GetActorLocation() + GetVelocity() * DeltaTime);
			//ClientChargeLocation = GetActorLocation();
		}
    }
}

void AShooterCharacter::ClientRunChargeAttack_Implementation(FVector InLocation, FVector InVelocity, float DeltaTime)
{
	FVector LocDelta = InLocation - ClientChargeLocation;
	SetActorLocation(GetActorLocation() + InVelocity * DeltaTime);
}

void AShooterCharacter::OnChargeAttack(float ChargeNotifyTime)
{
	if (bStartChargeAttack)
	{
		TargetChargeLocation = GetActorLocation() + (5.f * BaseMovementSpeed * ChargeNotifyTime / 2) * GetActorForwardVector();
		//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		//GetCharacterMovement()->MaxWalkSpeed = 5.f * BaseMovementSpeed;
		//GetCharacterMovement()->MaxAcceleration = 6.f * BaseAcceleration;
		if (IsLocallyControlled() && !IsGrappling())
		{
			SetMovementState(EMovementModifiers::EMM_ChargeAttack);
		}
		bInterpChargeAttack = true;
		if (HasAuthority()) ChargeAttackStrength = 0.f;
	}
	else
	{
		if (IsLocallyControlled() && !IsGrappling())
		{
			bCrouching ? SetMovementState(EMovementModifiers::EMM_Crouch) : SetMovementState(EMovementModifiers::EMM_Normal);
		}
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		//GetCharacterMovement()->MaxAcceleration = BaseAcceleration;
		bInterpChargeAttack = false;
		if (HasAuthority()) CombatState = ECombatState::ECS_Unoccupied;
		if (IsLocallyControlled()) bChargeAttack = false;
	}
}

void AShooterCharacter::PlayChargeAttackMontage(FName MontageSectionName)
{
	// Play Gun Fire Montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ChargeAttackMontage)
	{
		AnimInstance->Montage_Play(ChargeAttackMontage);
		AnimInstance->Montage_JumpToSection(MontageSectionName);
	}
}

void AShooterCharacter::OnCapsuleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (bStartChargeAttack)
	{
		auto HitShooter = Cast<AShooterCharacter>(OtherActor);
		if (HitShooter && HitShooter != this)
		{
			bStartChargeAttack = false;
			float DamageAmount = 100.f;
			ShooterPS = GetShooter_PS();
			if (ShooterPS) ShooterPS->DamageType = EShooterDamageType::ESDT_Hand;

			UGameplayStatics::ApplyDamage(
				HitShooter,
				DamageAmount,
				GetController(),
				this,
				URushAttack::StaticClass()
			);
			if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(OtherActor))
			{
				WeaponHitInterface->Execute_OnStunned(OtherActor, this);
			}
			MulticastChargeHit(HitShooter, HitComp, DamageAmount);
		}
	}
}

void AShooterCharacter::MulticastChargeHit_Implementation(AShooterCharacter* ChargeHitShooter, UPrimitiveComponent* HitComponent, float DamageAmount)
{
	PunchedEffect(HitComponent->GetComponentLocation(), true);
	if (IsLocallyControlled() && ChargeHitShooter) ShowHitNumber(DamageAmount, ChargeHitShooter->GetActorLocation(), false, ChargeHitShooter->GetShield() > 0);	
}

void AShooterCharacter::OnStunned_Implementation(AActor* StunnerActor)
{
	if (Health <= 0.f || bIsStunned) return;
	bIsStunned = true;
	bDisableGameplay = true;
	OnRep_IsStunned();

	if (APawn* InstigatorPawn = StunnerActor->GetInstigator())
	{
		AShooterPlayerState* PS = Cast<AShooterPlayerState>(InstigatorPawn->GetPlayerState());
		if (PS && !PS->IsABot())
		{
			PS->AddToXPMetric(EProgressMetric::EPM_StunPlayer, 1);
		}
	}
	FTimerHandle FreezeTimerHandle;
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<AShooterCharacter> WeakThis = this;  // Assuming this is in AYourCharacterClass
		World->GetTimerManager().SetTimer(
			FreezeTimerHandle,
			[WeakThis]()
			{
				if (AShooterCharacter* StrongThis = WeakThis.Get())
				{
					StrongThis->bIsStunned = false;
					StrongThis->OnRep_IsStunned();
				}
			}, 2.1f, false
		);
	}
}

void AShooterCharacter::OnRep_IsStunned()
{
	if (bIsStunned)
	{
		if (StunMaterialInstance && GetMesh())
		{
			GetMesh()->SetOverlayMaterial(StunMaterialInstance);
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

			if(AnimInstance && ElimMontage && StunnedSound)
			{
				ElimMontage->bEnableAutoBlendOut = true;
				AnimInstance->Montage_Play(ElimMontage);
				AnimInstance->Montage_JumpToSection(FName("Stun"));
				UGameplayStatics::PlaySoundAtLocation(this, StunnedSound, GetActorLocation());
			}
		}
	}
	else
	{
		if (HasAuthority()) bDisableGameplay = false;
		if (GetMesh()) GetMesh()->SetOverlayMaterial(nullptr);
	}
}

// Not being used right now. Sometimes the button action did not work on the didicated server.
void AShooterCharacter::ChargeAttackButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay || !bCanFire) return;
	if (EquippedItem && EquippedItem->GetItemType() == EItemType::EIT_Dummy && !bChargeAttack && !bHandShieldButtonPressed && ChargeAttackStrength == 100.f && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		StartChargeAttack();
	}
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) { return false; }

	return EquippedWeapon->GetAmmo() > 0;
}

void AShooterCharacter::ReloadButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	if (bUnEquippedState && !bChargeAttack && !bHandShieldButtonPressed && ChargeAttackStrength == 100.f && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		StartChargeAttack();
		//UE_LOG(LogTemp, Warning, TEXT("ReloadButtonPressed"));
	}
	else
	{
		Reload();
	}
}

/*
void AShooterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (bUnEquippedState && !bChargeAttack && !bHandShieldButtonPressed && ChargeAttackStrength == 100.f && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		StartChargeAttack();
		//UE_LOG(LogTemp, Warning, TEXT("ReloadButtonPressed"));
	}
	else
	{
		Reload();
	}
}
*/

void AShooterCharacter::Reload(bool bNoAmmoWhenSwitching)
{
	if (EquippedWeapon == nullptr || EquippedItem == nullptr) return;
	if (!bNoAmmoWhenSwitching)
	{
		if (CombatState != ECombatState::ECS_Unoccupied) { return; }
	}

	if (CarryingAmmo() && EquippedWeapon->GetMagazineCapacity() != EquippedWeapon->GetAmmo() && !bUnEquippedState && !bLocallyReloading && CombatState == ECombatState::ECS_Unoccupied && EquippedItem->GetItemType() == EItemType::EIT_Weapon)
	{
		ReloadWeapon();
		ServerReload();
		bLocallyReloading = true;
	}
}

void AShooterCharacter::ServerReload_Implementation()
{
	if (EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	if (!IsLocallyControlled()) ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (EquippedWeapon == nullptr) { return; }

	if (bAiming)
	{
		SetAiming(false);
	}

	// Play Reload Montage
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		FOnMontageEnded ReloadEndDelegate;
		ReloadEndDelegate.BindUObject(this, &AShooterCharacter::OnReloadMontageEnded);
		AnimInstance->Montage_SetEndDelegate(ReloadEndDelegate, ReloadMontage);
	}
}

void AShooterCharacter::OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted) FinishReloading();
}

void AShooterCharacter::FinishEquipping()
{
	if (!IsLocallyControlled()) return;
	//Set_TargetMovementSpeed(BaseMovementSpeed);
	SetCombatState(ECombatState::ECS_Unoccupied);
	if (EquippedItem && bUnEquippedState)
	{
		if (bAimingButtonPressed)
		{
			ShieldOn(true);
		}
	}
	else if (EquippedItem && !bUnEquippedState)
	{
		if (bAimingButtonPressed && EquippedItem->GetItemType() == EItemType::EIT_Weapon)
		{
			SetAiming(true);
		}
	}
}

void AShooterCharacter::OnFinishedReloading_Implementation()
{
	//FinishReloading();
}

void AShooterCharacter::OnShotgunShellReload_Implementation()
{
	ShotgunShellReload();
}

void AShooterCharacter::FinishReloading()
{
	UE_LOG(LogTemp, Warning, TEXT("FinishReloading"));
	// Update the Combat State
	bLocallyReloading = false;
	//CombatState = ECombatState::ECS_Unoccupied;
	SetCombatState(ECombatState::ECS_Unoccupied);

	if (bAimingButtonPressed)
	{
		SetAiming(true);
	}

	if (EquippedWeapon == nullptr) { return; }
	// Update the AmmoMap
	const auto AmmoType = EquippedWeapon->GetAmmoType();

	int32 AmmoAmount = GetAmmoAmount(AmmoType);
	if (AmmoAmount != -1)
	{
		// Amount of ammo the Character is carrying of the EquippedWeapon type
		int32 CarriedAmmo = AmmoAmount;

		// Space left in the magazine of EquippedWeapon
		const int32 MagEmptySpace =
			EquippedWeapon->GetMagazineCapacity()
			- EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			// Reload the magazine with all the ammo we are carrying
			EquippedWeapon->AddAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(AmmoType)] = CarriedAmmo;
		}
		else
		{
			// fill the magazine
			EquippedWeapon->AddAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(AmmoType)] = CarriedAmmo;
		}
		if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	}
	HudCarriedAmmoDelegate.Broadcast();
}

void AShooterCharacter::ShotgunShellReload()
{
	if (HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void AShooterCharacter::UpdateShotgunAmmoValues()
{
	if (EquippedWeapon == nullptr) return;

	int32 AmmoAmount = GetAmmoAmount(EquippedWeapon->GetAmmoType());
	if (AmmoAmount != -1)
	{
		AmmoStruct.AmmoAmount[AmmoStruct.AmmoType.Find(EquippedWeapon->GetAmmoType())] -= 1;
	}

	EquippedWeapon->AddAmmo(1);

	if (EquippedWeapon->GetMagazineCapacity() == EquippedWeapon->GetAmmo() || AmmoAmount - 1 == 0)
	{
		Multicast_JumpToShotgunEnd();
	}
}

void AShooterCharacter::JumpToShotgunEnd()
{
	// Jump to ShotgunEnd section
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void AShooterCharacter::Multicast_JumpToShotgunEnd_Implementation()
{
	JumpToShotgunEnd();
}

bool AShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr) { return false; }

	int32 AmmoAmount = GetAmmoAmount(EquippedWeapon->GetAmmoType());
	if (AmmoAmount != -1)
	{
		return AmmoAmount > 0;
	}
	return false;
}

void AShooterCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr) { return; }
	if (HandSceneComponent == nullptr) { return; }

	if (EquippedWeapon->bIsAnNFT)
	{
		EquippedWeapon->AttachClip(this, true);
	}
	else
	{
		// Get transform of the clip (rotation, location, etc...)
		// Index for the clip bone on the Equipped Weapon
		int32 ClipBoneIndex{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };
		// Store the transfrom of the clip
		ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);
		//UE_LOG(LogTemp, Warning, TEXT("GrabClip"));

		FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true); // Set the rules for the attachment
		HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
		HandSceneComponent->SetWorldTransform(ClipTransform);

		EquippedWeapon->SetMovingClip(true);
	}
}

void AShooterCharacter::ReplaceClip()
{
	if (EquippedWeapon == nullptr) { return; }
	EquippedWeapon->SetMovingClip(false);
	if (EquippedWeapon->bIsAnNFT)
	{
		EquippedWeapon->AttachClip(this, false);
		if (EquippedWeapon->GetReloadSound())
		{
			UGameplayStatics::SpawnSoundAttached(
				EquippedWeapon->GetReloadSound(), // sound cue (USoundBase)
				GetMesh(), // mesh to attach to
				FName("Root"),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("ReplaceClip"));
}

void AShooterCharacter::CrouchActionPressed(const FInputActionValue& Value)
{
	CrouchButtonPressed();
}

void AShooterCharacter::CrouchButtonPressed()
{
	//ServerSpawnNFT(TEXT("123"));
	if (bDisableGameplay || HoverState == EHoverState::EHS_HoverStart || HoverState == EHoverState::EHS_HoverRush) return;
	//UE_LOG(LogTemp, Warning, TEXT("CrouchButtonPressed: In Function"));
	//UE_LOG(LogTemp, Warning, TEXT("CrouchButtonPressed: bUsingItem = %i"), bUsingItem);
	if (!GetCharacterMovement()->IsFalling() && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking && !bUsingItem && !IsGrappling())
	{
		TriggerCrouch();
	}
}

void AShooterCharacter::TriggerCrouch()
{
	bCrouching = !bCrouching;
	SetCrouching(bCrouching);
	if(bCrouching)
	{
		Crouch();
		if (!IsGrappling() && HoverState != EHoverState::EHS_HoverStart && HoverState != EHoverState::EHS_HoverRush)
		{
			SetMovementState(EMovementModifiers::EMM_Crouch);
		}
	}
	else
	{
		UnCrouch();
		if (!IsGrappling() && HoverState != EHoverState::EHS_HoverStart && HoverState != EHoverState::EHS_HoverRush)
		{
			SetMovementState(EMovementModifiers::EMM_Normal);
		}
	}
}

void AShooterCharacter::OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
    Super::OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
    if (CameraBoom)
    {
        CameraBoom->TargetOffset.Z = CrouchedCameraOffsetZ;
    }
}

void AShooterCharacter::OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
    Super::OnEndCrouch(HeightAdjust, ScaledHeightAdjust);
    if (CameraBoom)
    {
        CameraBoom->TargetOffset.Z = StandingCameraOffsetZ;
    }
}

void AShooterCharacter::InventoryManageButtonPressed(const FInputActionValue& Value)
{
	if (bGamePaused || (CombatState != ECombatState::ECS_Unoccupied) ||
		bDisableGameplay || bUsingItem || bEmoteButtonPressed || bChargeAttack) return;
	bManageInventory = !bManageInventory;
	if (bManageInventory) CheckAimingWidget();
	ShooterPlayerController = GetShooter_PC();
	if (ShooterPlayerController)
	{
		ShooterPlayerController->ManageInventory(bManageInventory);
	}
}

void AShooterCharacter::PauseActionButtonPressed(const FInputActionValue& Value)
{
	//bool bPaused = Value.Get<bool>();
	//UE_LOG(LogTemp, Warning, TEXT("Pause Button Pressed"));
	PauseButtonPressed();
}

void AShooterCharacter::PauseButtonPressed()
{
	bGamePaused ? ShowPauseMenu(TArray<FPlayerGameStats>()) : ServerShowPauseMenu();
}

void AShooterCharacter::ServerShowPauseMenu_Implementation()
{
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS)
	{
		ShooterGS->GetPings();
		ClientShowPauseMenu(ShooterGS->GetRankedPlayerScores());
	}
}

void AShooterCharacter::ClientShowPauseMenu_Implementation(const TArray<FPlayerGameStats>& PlayerStatsArray)
{
	ShowPauseMenu(PlayerStatsArray);
}

void AShooterCharacter::ShowPauseMenu(const TArray<FPlayerGameStats>& PlayerStatsArray)
{
	bGamePaused = !bGamePaused;
	ShooterPlayerController = GetShooter_PC();
	if (ShooterPlayerController)
	{
		ShooterPlayerController->ShowPauseMenu(bGamePaused, PlayerStatsArray);
	}
}

void AShooterCharacter::Set_TargetMovementSpeed_Implementation(const float InTargetMovementSpeed)
{
	TargetMovementSpeed = InTargetMovementSpeed;
	GetCharacterMovement()->MaxWalkSpeed = TargetMovementSpeed;
}

void AShooterCharacter::OnRep_TargetMovementSpeed(float OldTargetMovementSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = TargetMovementSpeed;
}

void AShooterCharacter::SetCrouching_Implementation(const bool InCrouching)
{
	bCrouching = InCrouching;
	if (bCrouching )
	{
		CharacterStateTags.AddTag(FGameplayTag::RequestGameplayTag("State.Character.Crouched"));
	}
	else
	{
		CharacterStateTags.RemoveTag(FGameplayTag::RequestGameplayTag("State.Character.Crouched"));
	}
}

void AShooterCharacter::InterpCapsuleHeight(float DeltaTime)
{
	float TargetCapsuleHeight{};
	if (bCrouching)
	{
		TargetCapsuleHeight = CrouchingCapsuleHeight;
	}
	else
	{
		TargetCapsuleHeight = StandingCapsuleHeight;
	}
	const float InterpCapsuleHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHeight, DeltaTime, 15.f);

	// Negative is crouching, positive is standing
	const float DeltaCapsuleHeight = InterpCapsuleHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector MeshOffset{ 0.f, 0.f, -DeltaCapsuleHeight };
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpCapsuleHeight);
	GetMesh()->AddLocalOffset(MeshOffset);
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
	if (Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1, 0 };
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2{ InterpComp2, 0 };
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3{ InterpComp3, 0 };
	InterpLocations.Add(InterpLoc3);

	FInterpLocation InterpLoc4{ InterpComp4, 0 };
	InterpLocations.Add(InterpLoc4);

	FInterpLocation InterpLoc5{ InterpComp5, 0 };
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6{ InterpComp6, 0 };
	InterpLocations.Add(InterpLoc6);
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for (int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if (InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount > 1) return;

	if (InterpLocations.Num() > Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::LocalInventoryAnim(int32 CurrentItemIndex, int32 NewItemIndex)
{
	bool bExchangeConditions = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		((CombatState != ECombatState::ECS_Unoccupied) || (CombatState != ECombatState::ECS_Equipping)) &&
		bStartGrapplePull == false && bSuperPunch == false;

	if (EquippedItem == nullptr)
	{
		// -1 == no EquippedWeapon yet. No need to reverse the inventory icon animation
		EquipItemDelegate.Broadcast(-1, NewItemIndex);
	}
	else
	{
		EquipItemDelegate.Broadcast(CurrentItemIndex, NewItemIndex);
	}
}

void AShooterCharacter::ExchangeInventoryLocal(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved)
{
	bool bExchangeConditions = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		((CombatState != ECombatState::ECS_Unoccupied) || (CombatState != ECombatState::ECS_Equipping)) &&
		bStartGrapplePull == false && bSuperPunch == false && HoverState != EHoverState::EHS_HoverRush;

	if (!bExchangeConditions || EquippedItem == nullptr)
	{
		InventorySelect = CurrentItemIndex;
		return; 
	}
	InventorySelect = NewItemIndex;

	if (NewItemIndex == 0 && bAiming)
	{
		ShieldOn(true);
		SetAiming(false);
	}
	else if (NewItemIndex != 0 && bAiming && EquippedWeapon && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Sniper && EquippedItem->GetItemType() == EItemType::EIT_Weapon)
	{
		//SetAiming(false);
		CheckAimingWidget();
	}
	else if (NewItemIndex != 0 && bHandShield)
	{
		ShieldOn(false);
		SetAiming(true);
	}
	bLocallyReloading = false;
	bCanFire = false;
	
	CheckEmotePlaying();
	LocalInventoryAnim(CurrentItemIndex, NewItemIndex);
}

void AShooterCharacter::InventorySwitchLeft(const FInputActionValue& Value)
{
	if (bChargeAttack || bWorldTeleporting) return;
	if (InventorySelect > 0)
	{
		--InventorySelect;
		ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), InventorySelect);
		Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), InventorySelect);
	}
}

void AShooterCharacter::InventorySwitchRight(const FInputActionValue& Value)
{
	if (bChargeAttack || bWorldTeleporting) return;
	if (InventorySelect < Inventory.Num())
	{
		++InventorySelect;
		ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), InventorySelect);
		Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), InventorySelect);
	}
}

void AShooterCharacter::ItemScrolled(const FInputActionValue& Value)
{
	if (!EquippedItem || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	const float AxisValue = Value.Get<float>();
	if (AxisValue < 0.f)
	{
		if (InventorySelect > 0)
		{
			--InventorySelect;
			ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), InventorySelect);
			Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), InventorySelect);
		}
	}
	else
	{
		if (InventorySelect < Inventory.Num())
		{
			++InventorySelect;
			ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), InventorySelect);
			Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), InventorySelect);
		}
	}
}

void AShooterCharacter::FKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 0) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 0);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 1) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 1);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 2) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 2);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 3) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 3);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 4) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 4);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeyPressed(const FInputActionValue& Value)
{
	if ((EquippedItem && EquippedItem->GetSlotIndex() == 5) || bChargeAttack || bWorldTeleporting || CombatState == ECombatState::ECS_Mantle || bDisableGameplay) return;
	ExchangeInventoryLocal(EquippedItem->GetSlotIndex(), 5);
	Server_ExchangeInventoryItems(EquippedItem->GetSlotIndex(), 5);
}

void AShooterCharacter::Server_ExchangeInventoryItems_Implementation(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved)
{
	ExchangeInventoryItems(CurrentItemIndex, NewItemIndex, bItemRemoved);
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved)
{
	bool bExchangeConditions = (CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		((CombatState != ECombatState::ECS_Unoccupied) || (CombatState != ECombatState::ECS_Equipping)) &&
		bStartGrapplePull == false && bSuperPunch == false && HoverState != EHoverState::EHS_HoverRush;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const bool bDoOnServer = HasAuthority();

	if (bDoOnServer && bExchangeConditions) {

		if (bAiming)
		{
			SetAiming(false);
		}

		auto OldEquippedItem = EquippedItem;
		auto OldEquippedBoost = Cast<ABoostItem>(OldEquippedItem);
		// If old item is a grapple boost and is in the air while character is switching items, destroy the thrown grapple boost
		if (OldEquippedBoost && OldEquippedBoost->GetBoostType() == EBoostType::EBT_Grapple)
		{
			switch (OldEquippedBoost->GrappleState)
			{
			case EGrappleState::EGS_Start:
				ServerStopGrapplePull(OldEquippedBoost, true, true);
				break;
			case EGrappleState::EGS_Hit:
			case EGrappleState::EGS_Release:
				break;
			default:
				//OldEquippedItem->SetItemState(EItemState::EIS_PickedUp);
				break;
			}
		}
		else
		{
			// Otherwise, set all items to picked up
			//OldEquippedItem->SetItemState(EItemState::EIS_PickedUp);
		}

		if (Inventory[NewItemIndex])
		{
			switch (Inventory[NewItemIndex]->GetItemType())
			{
			case EItemType::EIT_Weapon:
			{
				bUnEquippedState = false;
				//Inventory[NewItemIndex]->SetItemState(EItemState::EIS_Equipped);
				CombatState = ECombatState::ECS_Equipping;

				if (bComboPlaying)
				{
					bComboPlaying = false;
					Set_TargetMovementSpeed(BaseMovementSpeed);
				}
				break;
			}
			case EItemType::EIT_Boost:
			{
				bUnEquippedState = false;
				//Inventory[NewItemIndex]->SetItemState(EItemState::EIS_Equipped);
				CombatState = ECombatState::ECS_Equipping;
				break;
			}
			case EItemType::EIT_Dummy:
			{
				StopReloadMontage(AnimInstance);
				bUnEquippedState = true;
				CombatState = ECombatState::ECS_Unoccupied;
				break;
			}
			
			default:
				break;
			}
			EquipItem(Inventory[NewItemIndex], false, bItemRemoved);
		}
		
		if (EquippedItem && bGhostMode && (!IsLocallyControlled() || bIsAI))
		{
			if (!EquippedItem->bIsAnNFT && EquippedItem->GetItemMesh())
			{
				EquippedItem->GetItemMesh()->SetVisibility(false);
			}
			else if (EquippedItem->bIsAnNFT && EquippedItem->GetNFTMesh())
			{
				EquippedItem->GetNFTMesh()->SetVisibility(false, true);
			}
			// Hide new equipped item, and show the old weapon (old weapon will be in Picked up mode anyway).
			
			//OldItem->GetItemMesh()->SetVisibility(true);
		}
	}
}

void AShooterCharacter::UpdateItemState()
{
    for (AItem* Item : Inventory)
    {
        if (!Item) continue;
        
        if (Item == EquippedItem && Item->GetItemType() != EItemType::EIT_Dummy)
        {
            Item->SetItemState(EItemState::EIS_Equipped);
        }
        else
        {
            Item->SetItemState(EItemState::EIS_PickedUp);
        }
    }
}

void AShooterCharacter::StopItemMontageEffects(UAnimInstance* AnimInstance)
{
	if (Boost == nullptr) return;
	
	if(AnimInstance && ItemUseMontage && AnimInstance->Montage_IsPlaying(ItemUseMontage))
	{
		AnimInstance->Montage_Stop(0, ItemUseMontage);
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		/*
		// It seems that this part of the function is crashing the game on the dedicated server when playing a item montage and then switching inventory item (spamming the buttons)
		if (Boost->ItemEffectSoundComponent && Boost->ItemEffectSoundComponent->IsActive() && Boost->ItemEffectSoundComponent->IsPlaying())
		{
			Boost->ItemEffectSoundComponent->Stop();
		}

		if (Boost->ItemEffectComponent && Boost->ItemEffectComponent->IsActive())
		{
			Boost->ItemEffectComponent->DestroyComponent();
		}
		*/
	}
}

void AShooterCharacter::StopReloadMontage(UAnimInstance* AnimInstance)
{
	if (AnimInstance && ReloadMontage && AnimInstance->Montage_IsPlaying(ReloadMontage))
	{
		AnimInstance->Montage_Stop(0, ReloadMontage);
		ReplaceClip();
	}
}

int32 AShooterCharacter::GetEmptyInventorySlot()
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}

	if (Inventory.Num() < InventoryMaxCapacity)
	{
		return Inventory.Num();
	}

	return -1; // Inventory is full
}

void AShooterCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot{ GetEmptyInventorySlot() };
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

void AShooterCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

void AShooterCharacter::EmoteButtonPressed(const FInputActionValue& Value)
{
	if (!bCanFire || (EquippedItem && EquippedItem->GetItemType() != EItemType::EIT_Dummy) || GetCharacterMovement() == nullptr) return;
	bool bEmoteConditions = bDisableGameplay || bUsingItem || bEmoteButtonPressed ||
		GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Walking || bAimingButtonPressed ||
		bHandShieldButtonPressed || CombatState != ECombatState::ECS_Unoccupied;
	if (bEmoteConditions) return;
	bEmoteButtonPressed = true;

	if (IsLocallyControlled())
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		PlayEmoteMontage(EmoteMontage);
		ServerPlayEmote(EmoteMontage);
	}
}

void AShooterCharacter::ServerPlayEmote_Implementation(UAnimMontage* InMontage)
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
    PlayEmoteMontage(InMontage);
	MulticastPlayEmote(InMontage);
}

void AShooterCharacter::MulticastPlayEmote_Implementation(UAnimMontage* InMontage)
{
	if ((IsLocallyControlled() && !HasAuthority()) || HasAuthority()) return;
    PlayEmoteMontage(InMontage);
}

void AShooterCharacter::PlayEmoteMontage(UAnimMontage* InMontage)
{
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && InMontage)
    {
		AnimInstance->Montage_Play(InMontage);
		AnimInstance->Montage_JumpToSection("Emote");
	}
}

void AShooterCharacter::OnEmoteFinished_Implementation()
{
	if (HasAuthority())
	{
		ShooterPS = GetShooter_PS();
		if (ShooterPS) ShooterPS->PlayedEmotes++;
		//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
	else if (IsLocallyControlled() && bEmoteButtonPressed)
	{
		//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		//bEmoteButtonPressed = false;
	}
}

void AShooterCharacter::JumpButtonPressed()
{
	if (GetVelocity().Size() <= 1.05f * BaseMovementSpeed)
	{
		bIsAI ? ACharacter::Jump() : OnJump();
		//ACharacter::Jump();
	}
	
	if (bStartFlying)
	{
		ServerStopFlying();
	}
}

void AShooterCharacter::ServerStopFlying_Implementation()
{
	AShooterGameState* ShooterGS = GetShooter_GS();
	if (ShooterGS && ShooterGS->GameMatchState == EGameMatchState::EMS_Start)
	{
		GetWorldTimerManager().ClearTimer(FlyTimer);
		StopFlying();
	}
}

void AShooterCharacter::JumpButtonReleased()
{
	ACharacter::StopJumping();
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//EnhancedInputComponent->BindAction(ChargeAttackAction, ETriggerEvent::Triggered, this, &AShooterCharacter::ChargeAttackButtonPressed);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AShooterCharacter::ReloadButtonPressed);

		EnhancedInputComponent->BindAction(FireActionPressed, ETriggerEvent::Triggered, this, &AShooterCharacter::FireActionButtonPressed);
		EnhancedInputComponent->BindAction(FireActionPressed, ETriggerEvent::Completed, this, &AShooterCharacter::FireActionButtonReleased);

		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &AShooterCharacter::PauseActionButtonPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AShooterCharacter::CrouchActionPressed);
		EnhancedInputComponent->BindAction(InventoryManageAction, ETriggerEvent::Triggered, this, &AShooterCharacter::InventoryManageButtonPressed);

		EnhancedInputComponent->BindAction(AimingActionPressed, ETriggerEvent::Triggered, this, &AShooterCharacter::AimingActionButtonPressed);
		EnhancedInputComponent->BindAction(AimingActionPressed, ETriggerEvent::Completed, this, &AShooterCharacter::AimingActionButtonReleased);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AShooterCharacter::SprintButtonPressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AShooterCharacter::SprintButtonReleased);

		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AShooterCharacter::JumpButtonPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AShooterCharacter::JumpButtonReleased);

		EnhancedInputComponent->BindAction(HoverActionPressed, ETriggerEvent::Triggered, this, &AShooterCharacter::HoverActionButtonPressed);
		EnhancedInputComponent->BindAction(HoverActionPressed, ETriggerEvent::Completed, this, &AShooterCharacter::HoverActionButtonReleased);

		EnhancedInputComponent->BindAction(SelectActionPressed, ETriggerEvent::Triggered, this, &AShooterCharacter::SelectButtonPressed);
		EnhancedInputComponent->BindAction(SelectActionPressed, ETriggerEvent::Completed, this, &AShooterCharacter::SelectButtonReleased);

		EnhancedInputComponent->BindAction(EmoteAction, ETriggerEvent::Triggered, this, &AShooterCharacter::EmoteButtonPressed);
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Triggered, this, &AShooterCharacter::GrappleButtonPressed);

		EnhancedInputComponent->BindAction(InventoryAction_0, ETriggerEvent::Triggered, this, &AShooterCharacter::FKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_1, ETriggerEvent::Triggered, this, &AShooterCharacter::OneKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_2, ETriggerEvent::Triggered, this, &AShooterCharacter::TwoKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_3, ETriggerEvent::Triggered, this, &AShooterCharacter::ThreeKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_4, ETriggerEvent::Triggered, this, &AShooterCharacter::FourKeyPressed);
		EnhancedInputComponent->BindAction(InventoryAction_5, ETriggerEvent::Triggered, this, &AShooterCharacter::FiveKeyPressed);

		EnhancedInputComponent->BindAction(ItemScrollAction, ETriggerEvent::Triggered, this, &AShooterCharacter::ItemScrolled);
		EnhancedInputComponent->BindAction(InventoryActionSwitchLeft, ETriggerEvent::Triggered, this, &AShooterCharacter::InventorySwitchLeft);
		EnhancedInputComponent->BindAction(InventoryActionSwitchRight, ETriggerEvent::Triggered, this, &AShooterCharacter::InventorySwitchRight);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterCharacter::Look);
		EnhancedInputComponent->BindAction(MovementAction, ETriggerEvent::Triggered, this, &AShooterCharacter::MainMovement);
		EnhancedInputComponent->BindAction(LookRateAction, ETriggerEvent::Triggered, this, &AShooterCharacter::LookRate);
		EnhancedInputComponent->BindAction(TurnRateAction, ETriggerEvent::Triggered, this, &AShooterCharacter::TurnRate);
	}
	//PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	//PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	//PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	//PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	//PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	//PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	//PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	//PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);
	//PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &AShooterCharacter::PauseButtonPressed);
	//PlayerInputComponent->BindAction("InventoryManage", IE_Pressed, this, &AShooterCharacter::InventoryManageButtonPressed);

	//PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	//PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	//PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	//PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	//PlayerInputComponent->BindAction("HoverButton", IE_Pressed, this, &AShooterCharacter::HoverButtonPressed);
	//PlayerInputComponent->BindAction("HoverButton", IE_Released, this, &AShooterCharacter::HoverButtonReleased);

	//PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	//PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	//PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	//PlayerInputComponent->BindAction("EmoteButton", IE_Pressed, this, &AShooterCharacter::EmoteButtonPressed);

	//PlayerInputComponent->BindAction("FKey", IE_Pressed, this, &AShooterCharacter::FKeyPressed);
	//PlayerInputComponent->BindAction("1Key", IE_Pressed, this, &AShooterCharacter::OneKeyPressed);
	//PlayerInputComponent->BindAction("2Key", IE_Pressed, this, &AShooterCharacter::TwoKeyPressed);
	//PlayerInputComponent->BindAction("3Key", IE_Pressed, this, &AShooterCharacter::ThreeKeyPressed);
	//PlayerInputComponent->BindAction("4Key", IE_Pressed, this, &AShooterCharacter::FourKeyPressed);
	//PlayerInputComponent->BindAction("5Key", IE_Pressed, this, &AShooterCharacter::FiveKeyPressed);
}

// RPCs and Networking
void AShooterCharacter::SetHandCombatState_Implementation(EHandCombatState InHandCombatState, const FString& State) {
	/*
	if (!HasAuthority())
	{
		SetHandCombatState(InHandCombatState, State);
		// Don't return to allow for prediction
		// return;
	}
	if (State == TEXT("Start"))
	{
		bComboPlaying = true;
	}
	else
	{
		bComboHit = false;
		bComboPlaying = false;
	}
	HandCombatState = InHandCombatState;
	SetHandCombatProperties(HandCombatState);
	*/
}

void AShooterCharacter::SetCombatState_Implementation(ECombatState InCombatState)
{
	CombatState = InCombatState;
}

void AShooterCharacter::OnRep_Inventory(TArray<AItem*> OldInventory)
{
	HudUpdateAmmoOnEquip.Broadcast();
}

void AShooterCharacter::OnRep_Health(float OldHealth)
{
    //if (!IsLocallyControlled()) return;
    if (Health < OldHealth)
    {
        DamageTakenDelegate.Broadcast(true, SaveGame == nullptr ? true : SaveGame->bDamageEffect, OldHealth - Health);
    }
    HealthHudDelegate.Broadcast(Health / MaxHealth, true);
	if (HasAuthority()) UpdateTeamMemberAttributes(Health, -1000.f);
}

void AShooterCharacter::OnRep_Shield(float OldShield)
{
	//if (!IsLocallyControlled()) return;
	if (Shield < OldShield)
	{
		ShieldDamageTakenDelegate.Broadcast(true);
	}
	ShieldHudDelegate.Broadcast(Shield / MaxShield, true);
	if (HasAuthority()) UpdateTeamMemberAttributes(-1000.f, Shield);
}

void AShooterCharacter::OnRep_HandShieldStrength()
{
	//if (!IsLocallyControlled()) return;
	ShieldStrengthDelegate.Broadcast(HandShieldStrength / MaxHandShieldStrength);
	if (bHandShield)
	{
		UpdateShieldColor(HandShieldStrength);
	}
}

void AShooterCharacter::OnRep_ChargeAttackStrength()
{
	ChargeAttackDelegate.Broadcast(ChargeAttackStrength / 100.f);
}

void AShooterCharacter::OnRep_HoverSystemFuel()
{
	HoverSystemDelegate.Broadcast(HoverSystemFuel / 100.f);
}

void AShooterCharacter::OnRep_HandCombatState(EHandCombatState OldHandState)
{
	//UE_LOG(LogTemp, Warning, TEXT("HandCombatState=%i"), HandCombatState);
	SetHandCombatProperties(HandCombatState);
}

void AShooterCharacter::OnRep_Crouching(bool OldCrouching)
{
	if (bCrouching)
		Crouch();
	else
		UnCrouch();
}

void AShooterCharacter::BoostItemMontageFinished()
{
	if (!HasAuthority()) return;
	auto EquippedBoost = Cast<ABoostItem>(EquippedItem);
	if (EquippedBoost == nullptr) return;
	EquippedBoost->SpendBoost();
	//UE_LOG(LogTemp, Warning, TEXT("ItemMontageFinished"));
	if (EquippedBoost->GetBoostType() == EBoostType::EBT_SuperPunch)
	{
		//UE_LOG(LogTemp, Warning, TEXT("ItemMontageFinished: EBT_SuperPunch"));
		bSuperPunch = false;
	}
	bUsingItem = false;
	//OnRep_UsingItem();
	//GetCharacterMovement()->MaxWalkSpeed = GetBaseMovementSpeed();
	//TargetMovementSpeed = GetBaseMovementSpeed();

	if (Boost)
	{
		Boost->UseBoost(EquippedBoost);
		APlayerState* PS = GetPlayerState();
		if (PS && !PS->IsABot()) Boost->RemoveItem(EquippedBoost);
	}
	SetCombatState(ECombatState::ECS_Unoccupied);

	//FTimerDelegate BoostFinishedDel;
	//FTimerHandle BoostFinishedTimer;
	//BoostFinishedDel.BindUFunction(this, FName("UsingItemFinished"), EquippedBoost);
	//GetWorld()->GetTimerManager().SetTimer(BoostFinishedTimer, BoostFinishedDel, 0.75f, false);
}

void AShooterCharacter::StartGhost()
{
	bGhostMode = true;
	OnRep_GhostMode();
	SetAbilityStatus(FName("Ghost"), true);

    FTimerHandle GhostTimer;
    GetWorldTimerManager().SetTimer(GhostTimer, this, &AShooterCharacter::StopGhost, 10.f);
}

void AShooterCharacter::StopGhost()
{
	bGhostMode = false;
	OnRep_GhostMode();
	SetAbilityStatus(FName("Ghost"), false);
}

void AShooterCharacter::GhostMaterialEffect()
{
	if (!GetMesh()) return;
	if (GhostMaterialInstance)
	{
		DynamicGhostMaterialInstance = UMaterialInstanceDynamic::Create(GhostMaterialInstance, this);
		if (DynamicGhostMaterialInstance)
		{
			DynamicGhostMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.123f);
			DynamicGhostMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
			SetAllSkeletalMeshMaterials(false, DynamicGhostMaterialInstance);
		}
	}
}

void AShooterCharacter::ProtectMaterialEffect()
{
	if (!GetMesh()) return;
	if (ProtectMaterialInstance && TargetSkeletalMeshOverride)
	{
		DynamicProtectMaterialInstance = UMaterialInstanceDynamic::Create(ProtectMaterialInstance, this);
		if (DynamicProtectMaterialInstance)
		{
			SetAllSkeletalMeshMaterials(false, DynamicProtectMaterialInstance);
		}
	}
}

void AShooterCharacter::OnRep_GhostMode()
{
	if (bGhostMode)
	{
		if (TrackingAI && (HasAuthority() || IsLocallyControlled()))
		{
			if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(TrackingAI))
			{
				EnemyInterface->Execute_SetTarget(TrackingAI, nullptr, false);
			}
			TrackingAI = nullptr;
		}
		if (bBoostProtect)
		{
			if (GenBoostSoundComponent && GenBoostSoundComponent->IsPlaying()) GenBoostSoundComponent->Stop();
			if (GenBoostEffect && GenBoostEffect->IsActive()) GenBoostEffect->Deactivate();
		}

		if (IsLocallyControlled() && !bIsAI)
		{
			GhostMaterialEffect();
			if (GenBoostEffect)
			{
				//FLinearColor ProtectColor{};
				GenBoostEffect->SetAsset(GhostSystem);
				GenBoostEffect->Activate();
			}
		}
		else
		{
			SetVisibilityOfShooter(false);
			if (EquippedItem) 
			{
				if (!EquippedItem->bIsAnNFT && EquippedItem->GetItemMesh())
				{
					EquippedItem->GetItemMesh()->SetVisibility(false);
				}
				else if (EquippedItem->bIsAnNFT && EquippedItem->GetNFTMesh())
				{
					EquippedItem->GetNFTMesh()->SetVisibility(false, true);
				}
			}
		}

		if (GenBoostSoundComponent && GhostStartSound)
		{
			LOG_SHOOTER_NORMAL(FString::Printf(TEXT("GenBoostSoundComponent Valid")));
			GenBoostSoundComponent->SetSound(GhostStartSound);
			GenBoostSoundComponent->Play();
		}
	}
	else
	{
		if (IsLocallyControlled() && !bIsAI)
		{
			if (GenBoostEffect && GenBoostEffect->IsActive() && !bBoostProtect)
			{
				GenBoostEffect->Deactivate();
			}
			if (bBoostProtect)
			{
				ProtectMaterialEffect();
			}
			else
			{
				SetDefaultMaterials();
			}
			UGameplayStatics::PlaySound2D(this, GhostStopSound);
		}
		else
		{
			SetVisibilityOfShooter(true);
			// Hide the unused weapon in slot 0 (unequipped mode)
			if (EquippedItem)
			{
				if (!EquippedItem->bIsAnNFT && EquippedItem->GetItemMesh())
				{
					EquippedItem->GetItemMesh()->SetVisibility(true);
				}
				else if (EquippedItem->bIsAnNFT && EquippedItem->GetNFTMesh())
				{
					EquippedItem->GetNFTMesh()->SetVisibility(true, true);
				}
			}
		}

		if (GhostStopEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				GhostStopEffect,
				GetActorLocation(),
				GetActorRotation()
			);
		}
		
		if (GenBoostSoundComponent && GenBoostSoundComponent->IsPlaying() && !bBoostProtect)
		{
			GenBoostSoundComponent->Stop();
		}
	}
}

void AShooterCharacter::StartProtect(float ProtectTime)
{
    bBoostProtect = true;
	OnRep_BoostProtect();
	SetAbilityStatus(FName("Protect"), true);

    FTimerHandle ProtectTimer;
    GetWorldTimerManager().SetTimer(ProtectTimer, this, &AShooterCharacter::StopProtect, ProtectTime);
}

void AShooterCharacter::StopProtect()
{
    bBoostProtect = false;
	OnRep_BoostProtect();
	SetAbilityStatus(FName("Protect"), false);
}

void AShooterCharacter::OnRep_BoostProtect()
{
	if (bBoostProtect)
	{
		if (bGhostMode)
		{
			if (GenBoostSoundComponent && GenBoostSoundComponent->IsPlaying()) GenBoostSoundComponent->Stop();
			if (GenBoostEffect && GenBoostEffect->IsActive()) GenBoostEffect->Deactivate();
		}
		
		if (ProtectionEffect && GetMesh())
		{
			ProtectComponent = UGameplayStatics::SpawnEmitterAttached(
				ProtectionEffect,          //particle system
				GetMesh(),      //mesh to attach to
				FName("RightHandSocket"),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);
		}

		if (GenBoostSoundComponent && ProtectionStartSound)
		{
			LOG_SHOOTER_NORMAL(FString::Printf(TEXT("GenBoostSoundComponent Valid")));
			GenBoostSoundComponent->SetSound(ProtectionStartSound);
			GenBoostSoundComponent->Play();
		}

		if (ProtectSystem && GenBoostEffect)
		{
			//FLinearColor ProtectColor{};
			GenBoostEffect->SetAsset(ProtectSystem);
			//GenBoostEffect->SetColorParameter(FName("OrbitColor"), {4.818821f, 0.f, 150.f, 1.f});
			GenBoostEffect->Activate();
		}

		ProtectMaterialEffect();
	}
	else
	{
		/*
		if (ProtectComponent && !bGhostMode)
		{
			ProtectComponent->DestroyComponent();
		}
		*/
		
		if (GenBoostSoundComponent && GenBoostSoundComponent->IsPlaying() && !bGhostMode) GenBoostSoundComponent->Stop();
		
		if (ProtectionEndSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(
					GetWorld(),
					ProtectionEndSound,
					GetActorLocation(),
					GetActorRotation()
			);
		}

		if (ProtectionEndEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ProtectionEndEffect,
				GetActorLocation(),
				GetActorRotation()
			);
		}

		if (GenBoostEffect && GenBoostEffect->IsActive() && !bGhostMode)
		{
			GenBoostEffect->Deactivate();
		}

		if (IsLocallyControlled() && bGhostMode)
		{
			//GhostMaterialEffect();
		}
		else
		{
			SetDefaultMaterials();
		}
	}
}

void AShooterCharacter::StartFlying()
{
    bStartFlying = true;
	FlyingHeightTarget = GetActorLocation() + FVector{0.f, 0.f, 3500.f};
	OnRep_StartFlying();

    GetWorldTimerManager().SetTimer(FlyTimer, this, &AShooterCharacter::StopFlying, 15.f);
}

void AShooterCharacter::StopFlying()
{
	bStartFlying = false;
	bStopFlying = true;
	OnRep_StartFlying();
}

void AShooterCharacter::SmoothRise(float DeltaTime)
{
	if (bFlyingInterping && GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying)
	{
		//SetActorLocation(FMath::VInterpTo(GetActorLocation(), FlyingHeightTarget, DeltaTime, 0.75f));
		AddMovementInput((FlyingHeightTarget - GetActorLocation()).GetSafeNormal(), 2.0f);
		if (UKismetMathLibrary::NearlyEqual_FloatFloat(GetActorLocation().Z, FlyingHeightTarget.Z, 1000.f) || !bStartFlying)
		{
			//GetCharacterMovement()->MaxFlySpeed = BaseFlySpeed;
			//UE_LOG(LogTemp, Warning, TEXT("Final Height Target = %s"), *FlyingHeightTarget.ToString());
			bFlyingInterping = false;
		}
	}
}

void AShooterCharacter::StartSmoothCameraToAIWinner(FVector InAILocation, FRotator InAIRotation)
{
	AIWinnerLocation = InAILocation;
	AIWinnerRotation = InAIRotation;
	bStartCameraMoveToAIWinner = true;
}

void AShooterCharacter::SmoothCameraToAIWinner(float DeltaTime)
{
	if (bStartCameraMoveToAIWinner && FollowCamera)
	{
		FollowCamera->SetWorldLocation(FMath::VInterpTo(FollowCamera->GetComponentLocation(), AIWinnerLocation, DeltaTime, 4.f));
		FollowCamera->SetWorldRotation(FMath::RInterpTo(FollowCamera->GetComponentRotation(), AIWinnerRotation, DeltaTime, 4.f));
	}
}


void AShooterCharacter::OnRep_StartFlying()
{
	//if (!IsLocallyControlled()) return;
	AShooterGameState* ShooterGS = GetShooter_GS();
	//UE_LOG(LogTemp, Warning, TEXT("OnRep_StartFlying: ShooterGS fine"));
	bool bStartMatchCond = false;
	if (ShooterGS)
	{
		bStartMatchCond = ShooterGS->GameMatchState <= EGameMatchState::EMS_Warn;
	}
	
	if (GetCharacterMovement() == nullptr) return;
	if (bStartFlying)
	{
		//UE_LOG(LogTemp, Warning, TEXT("OnRep_StartFlying: In bStartFlying"));
		bFlyingInterping = true;
		//UE_LOG(LogTemp, Warning, TEXT("Initial Height Target = %s"), *GetActorLocation().ToString());

		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		GetCharacterMovement()->MaxFlySpeed = 2.f * BaseFlySpeed;
		
		/*
		if (IsLocallyControlled())
		{
			SetMovementState(EMovementModifiers::EMM_ItemFlyStart);
		}
		*/
		
		//GetCharacterMovement()->MaxAcceleration = BaseAcceleration;

		if (FlyingStartSound && GetMesh())
		{
			FlyingSoundComponent = UGameplayStatics::SpawnSoundAttached(
					FlyingStartSound, // sound cue (USoundBase)
					GetMesh(), // mesh to attach to
					FName("RightHandSocket"),   //socket name
					FVector(0,0,0),  //location relative to socket
					FRotator(0,0,0), //rotation 
					EAttachLocation::KeepRelativeOffset, 
					true //if true, will be deleted automatically
			);
		}

		if (FlyingEffect && HandFlyingEffect && GetMesh())
		{
			FlyingEffectComponent = UGameplayStatics::SpawnEmitterAttached(
				FlyingEffect,
				GetMesh(),
				FName("Root"),
				FVector(0,0,0),
				FRotator(-90.f,0,0),
				EAttachLocation::KeepRelativeOffset, 
				false
			);

			HandFlyingEffectComponent = UGameplayStatics::SpawnEmitterAttached(
				HandFlyingEffect,
				GetMesh(),
				FName("Root"),
				FVector(0,0,0),
				FRotator(0,0,0),
				EAttachLocation::KeepRelativeOffset, 
				false
			);
		}
	}
	else
	{
		if (!bCanTarget && (HasAuthority() || IsLocallyControlled())) bCanTarget = true;
		
		/*
		if (IsLocallyControlled())
		{
			SetMovementState(EMovementModifiers::EMM_ItemFlyStop);
		}
		*/
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		GetCharacterMovement()->MaxFlySpeed = BaseFlySpeed;
		GetCharacterMovement()->MaxAcceleration = BaseAcceleration;

		if (FlyingSoundComponent)
		{
			FlyingSoundComponent->DestroyComponent();
		}

		if (FlyingEffectComponent)
		{
			FlyingEffectComponent->DestroyComponent();
		}

		if (HandFlyingEffectComponent)
		{
			HandFlyingEffectComponent->DestroyComponent();
		}

		// Using the same end effects as protection
		if (FlyEndSound && GetWorld())
		{
			UGameplayStatics::SpawnSoundAtLocation(
					GetWorld(),
					FlyEndSound,
					GetActorLocation(),
					GetActorRotation()
			);
		}

		if (ProtectionEndEffect && GetWorld())
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ProtectionEndEffect,
				GetActorLocation(),
				GetActorRotation()
			);
		}
	}
}

void AShooterCharacter::UpdateFlying(float HeightValue)
{
    SetActorLocation(
        {GetActorLocation().X, GetActorLocation().Y, StartZLocation + HeightValue},
        false
    );
	
	if (bStartFlying && !GetCharacterMovement()->IsFalling() && HeightValue > 100.f)
	{
		bStartFlying = false;
		bStopFlying = false;
		FlyingTimeline->Stop();
	}
}

void AShooterCharacter::StartSlowMo()
{
	bSlowDown = true;
	OnRep_SlowDown();
	SetSlowCharacter();
	
    FTimerHandle SlowMoTimer;
    GetWorldTimerManager().SetTimer(SlowMoTimer, this, &AShooterCharacter::StopSlowMo, 20.f);
}

void AShooterCharacter::StopSlowMo()
{
	bSlowDown = false;
	SetSlowCharacter();
	OnRep_SlowDown();
}

void AShooterCharacter::OnRep_SlowDown()
{
	//SetSlowCharacter();
	if (SlowSphere == nullptr) return;
	if (bSlowDown)
	{
		SlowSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SlowSphere->SetGenerateOverlapEvents(true);
		if (SlowMoStartSound)
		{
			SlowMoSoundComponent = UGameplayStatics::SpawnSoundAttached(
					SlowMoStartSound, // sound cue (USoundBase)
					GetMesh(), // mesh to attach to
					FName("RightHandSocket"),   //socket name
					FVector(0,0,0),  //location relative to socket
					FRotator(0,0,0), //rotation 
					EAttachLocation::KeepRelativeOffset, 
					true //if true, will be deleted automatically
			);
		}

		if (SlowNiagaraEffectComponent)
		{
			SlowNiagaraEffectComponent->SetRelativeScale3D(FVector(10.2f));
			SlowNiagaraEffectComponent->Activate(true);
		}

		if (HandFlyingEffect)
		{
			HandFlyingEffectComponent = UGameplayStatics::SpawnEmitterAttached(
				HandFlyingEffect,
				GetMesh(),
				FName("Root"),
				FVector(0,0,0),
				FRotator(0,0,0),
				EAttachLocation::KeepRelativeOffset, 
				false
			);
		}
	}
	else
	{
		// Using the same end effects as protection
		if (SlowNiagaraEffectComponent)
		{
			SlowNiagaraEffectComponent->Deactivate();
		}
		if (ProtectionEndSound)
		{
			UGameplayStatics::SpawnSoundAtLocation(
					GetWorld(),
					ProtectionEndSound,
					GetActorLocation(),
					GetActorRotation()
			);
		}
		if (FlyingStartSound && SlowMoSoundComponent)
		{
			SlowMoSoundComponent->DestroyComponent();
		}
		if (HandFlyingEffectComponent)
		{
			HandFlyingEffectComponent->DestroyComponent();
		}
		SlowSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SlowSphere->SetGenerateOverlapEvents(false);
	}
}

void AShooterCharacter::SetSlowCharacter()
{
    TArray<AActor*> OverlappingCharacters;
    SlowSphere->GetOverlappingActors(OverlappingCharacters, ACharacter::StaticClass());
	//UE_LOG(LogTemp, Warning, TEXT("SetSlowCharacter: bSlowDown = %i"), bSlowDown);
    if (OverlappingCharacters.Num() > 0)
    {   
		//UE_LOG(LogTemp, Warning, TEXT("SetSlowCharacter: OverlappingCharacters.Num() > 0: %i"), OverlappingCharacters.Num());
		
        for (int32 i = 0; i < OverlappingCharacters.Num(); i++)
        {
			if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(OverlappingCharacters[i]))
			{
				WeaponHitInterface->Execute_OnSlowDown(OverlappingCharacters[i], bSlowDown, this);
			}
        }
    }	
}

void AShooterCharacter::OnSlowDownOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
    if (bSlowDown && OtherActor != this)
    {
		if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(OtherActor))
		{
			WeaponHitInterface->Execute_OnSlowDown(OtherActor, true, this);
		}
    }
}

void AShooterCharacter::OnSlowDownEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
    if (bSlowDown && OtherActor != this)
    {
		if (IWeaponHitInterface* WeaponHitInterface = Cast<IWeaponHitInterface>(OtherActor))
		{
			WeaponHitInterface->Execute_OnSlowDown(OtherActor, false, this);
		}
    }
}

void AShooterCharacter::SetFireRateDuringSlowMo(AShooterCharacter* SlowedChar, bool IsSlowActive)
{
	if (SlowedChar == nullptr) return;
	if (IsSlowActive)
	{
		for (auto InvItem : SlowedChar->Inventory)
		{
			if (InvItem)
			{
				auto InvWeapon = Cast<AWeapon>(InvItem);
				if (InvWeapon)
				{
					InvWeapon->AutoFireRate = InvWeapon->FixedAutoFireRate * 2.f;
				}
			}
		}
	}
	else
	{
		for (auto InvItem : SlowedChar->Inventory)
		{
			if (InvItem)
			{
				auto InvWeapon = Cast<AWeapon>(InvItem);
				if (InvWeapon)
				{
					InvWeapon->AutoFireRate = InvWeapon->FixedAutoFireRate;
				}
			}
		}
	}
}

void AShooterCharacter::OnRep_SlowDownFactor()
{
	if (bSlowDownTrigger)
	{
		CustomTimeDilation = 0.5f;
		SetFireRateDuringSlowMo(this, true);
	} 
	else
	{
		CustomTimeDilation = 1.f;
		SetFireRateDuringSlowMo(this, false);
	}
}

void AShooterCharacter::OnSlowDown_Implementation(bool bIsSlow, AShooterCharacter* InstigatorShooter)
{
	if (bIsSlow && InstigatorShooter != this)
	{
		bSlowDownTrigger = true;
		CustomTimeDilation = 0.5f;
		SetFireRateDuringSlowMo(this, true);
	}
	else if (!bIsSlow && InstigatorShooter != this)
	{
		bSlowDownTrigger = false;
		CustomTimeDilation = 1.f;
		SetFireRateDuringSlowMo(this, false);
	}
}

void AShooterCharacter::OnRep_SuperJump()
{
	if (bSuperJump)
	{
		if (SuperJumpEffectSystem)
		{
			SuperJumpEffectNiagComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				SuperJumpEffectSystem,
				GetRootComponent(),
				FName(),
				GetActorLocation(),
				GetActorRotation(),
				EAttachLocation::KeepWorldPosition,
				false
			);
		}
		/*
		if (SuperJumpEffect)
		{
			SuperJumpEffectComponent = UGameplayStatics::SpawnEmitterAttached(
				SuperJumpEffect,
				GetMesh(),
				FName("Root"),
				FVector(0,0,0),
				FRotator(0,0,0),
				EAttachLocation::KeepRelativeOffset, 
				false
			);
		}
		*/
	}
	else
	{
		if (SuperJumpEffectNiagComponent)
		{
			SuperJumpEffectNiagComponent->Deactivate();
		}	
	}
}

void AShooterCharacter::SpawnCopy()
{
	if (AIShooterClass)
	{
		FVector SpawnCopyLoc = GetActorLocation() + 150.0*GetActorForwardVector();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		auto AIShooterActor = GetWorld()->SpawnActor<AShooterCharacterAI>(AIShooterClass, SpawnCopyLoc, GetActorRotation(), SpawnParams);
		if (AIShooterActor) AIShooterActor->OnSpawn(this);
	}
}

void AShooterCharacter::OnGravityProjectileHit_Implementation(bool bIsPulling, AActor* ProjectileGravity)
{
	bGravityPull = bIsPulling;
	GravityProjectileActor = ProjectileGravity;
	if (bGravityPull)
	{
		if (HoverState == EHoverState::EHS_HoverStart || HoverState == EHoverState::EHS_HoverRush)
		{
			EHoverState OldHoverState = HoverState;
			HoverState = EHoverState::EHS_HoverStop;
			TriggerHover();
			OnRep_HoverState(OldHoverState);
		}
		Client_DisableHover();
	}
}

void AShooterCharacter::Client_DisableHover_Implementation()
{
	if ((HoverState == EHoverState::EHS_HoverStart || HoverState == EHoverState::EHS_HoverRush) && bHoverButtonPressed)
	{
		HoverButtonReleased();
	}
}

void AShooterCharacter::PullToGravityProjectile()
{
    if (!bGravityPull || !GravityProjectileActor) return;

	FVector VecToProjectile = GravityProjectileActor->GetActorLocation() - GetActorLocation();
	AddMovementInput(VecToProjectile.GetSafeNormal(), 1.05f);
}

FHitResult AShooterCharacter::GrappleTraceHit()
{
	FHitResult HitResult;
	FVector VectorPlaceholder;
	bool bHit = TraceUnderCrosshairs(HitResult, VectorPlaceholder);
	FVector_NetQuantize GrappleHitTarget = HitResult.ImpactPoint;

	return GetHitscanHitsBoost(GrappleHitTarget);
}

void AShooterCharacter::GrapplePull(ABoostItem* BoostItem)
{
    if (!HasAuthority()) return;
    bStartGrapplePull = true;
    OnRep_StartGrapplePull();
}

void AShooterCharacter::GrapplePullTimeout()
{
	if (bStartGrapplePull)
	{
		bStartGrapplePull = false;
		bGrappleInterp = false;
		ServerStopGrapplePull(GrappleBoost);
		if (bGrappleThrown) bGrappleThrown = false;
	}
}

void AShooterCharacter::OnRep_StartGrapplePull()
{
    ABoostItem* BoostItem = Cast<ABoostItem>(EquippedItem);

    if (bStartGrapplePull)
    {
        if (IsLocallyControlled())
        {
			SetMovementState(EMovementModifiers::EMM_GrappleStart);
            bGrappleThrown = true;
            GetWorldTimerManager().SetTimer(GrappleTimer, this, &AShooterCharacter::GrapplePullTimeout, 10.f);
        }
        if (HoverState == EHoverState::EHS_HoverStart)
        {
            HoverState = EHoverState::EHS_HoverFinish;
            OnRep_HoverState(EHoverState::EHS_HoverStart);
        }
        if (BoostItem == nullptr) return;
        // Removed: MaxFlySpeed/MaxAcceleration sets (now handled in GetMax*)
        GrappleBoost = BoostItem;
        bGrappleInterp = true;
    }
    else
    {
		if (IsLocallyControlled())
		{
			SetMovementState(EMovementModifiers::EMM_GrappleStop);
		}
        // Removed: Conditional MaxFlySpeed/MaxAcceleration resets (now handled in GetMax*)
        bGrappleInterp = false;
    }
}

void AShooterCharacter::StartGrapplePull()
{
    if (bGrappleInterp && GrappleBoost && GrappleBoost->GetBoostType() == EBoostType::EBT_Grapple)
    {
        FVector GrappleBoostDirection = GrappleBoost->GetActorLocation() - GetActorLocation();
        float Distance = GrappleBoostDirection.Size();
        
        float SpeedFactor = FMath::Clamp(Distance * 0.01f, 0.1f, 2.f); // Scale movement based on distance
        
        FVector Velocity = GetVelocity();
        FVector Damping = -Velocity * 0.5f; // Apply damping to reduce oscillation
        
        AddMovementInput(GrappleBoostDirection * SpeedFactor + Damping, 1.f);

        if (IsLocallyControlled() && !bFireButtonPressed)
        {
			SetMovementState(EMovementModifiers::EMM_GrappleStop);
            GetWorldTimerManager().ClearTimer(GrappleTimer);
            bStartGrapplePull = false;
            bGrappleInterp = false;
            ServerStopGrapplePull(GrappleBoost);
            if (bGrappleThrown) bGrappleThrown = false;
        }
    }
}

void AShooterCharacter::ServerStopGrapplePull_Implementation(ABoostItem* InGrappleBoost, bool bNoHit, bool bSwitchedItem)
{
	if (GetWorldTimerManager().IsTimerActive(DestroyGrappleTimer)) GetWorldTimerManager().ClearTimer(DestroyGrappleTimer);
	if (!bNoHit && !bSwitchedItem)
	{
		bStartGrapplePull = false;
		OnRep_StartGrapplePull();
	}
	if (bGrappleThrown) bGrappleThrown = false;

	//GrappleBoost = InGrappleBoost;

	if (InGrappleBoost && Boost)
	{
		if (InGrappleBoost->GetBoostItemAmount() <= 0)
		{
			//UE_LOG(LogTemp, Warning, TEXT("ServerStopGrapplePull GetBoostItemAmount() <= 0"));
			if (!bSwitchedItem)
			{
				if (InGrappleBoost->GetSlotIndex() == Inventory.Num())
				{
					// Equip item to the left of BoostItem if BoostItem is the last item in inventory
					Boost->ClientInventoryAnim(InGrappleBoost->GetSlotIndex(), InGrappleBoost->GetSlotIndex() - 1);
					ExchangeInventoryItems(InGrappleBoost->GetSlotIndex(), InGrappleBoost->GetSlotIndex() - 1);
				}
				else
				{
					//UE_LOG(LogTemp, Warning, TEXT("ServerStopGrapplePull switch item"));
					Boost->ClientInventoryAnim(InGrappleBoost->GetSlotIndex() + 1, InGrappleBoost->GetSlotIndex());
					ExchangeInventoryItems(InGrappleBoost->GetSlotIndex() + 1, InGrappleBoost->GetSlotIndex(), true);
				}
			}
		}
		else
		{
			// Code that spawns and equips a new grapple boost (if enough in inventory)
			SetCombatState(ECombatState::ECS_Equipping);
			UWorld* World = GetWorld();
			if (World && BoostItemClass)
			{
				ABoostItem* NewBoost = World->SpawnActor<ABoostItem>(BoostItemClass, GetActorTransform());
				NewBoost->SetBoostType(InGrappleBoost->GetBoostType());
				NewBoost->UpdateBoost();
				NewBoost->SetSlotIndex(InGrappleBoost->GetSlotIndex());
				NewBoost->SetBoostItemAmount(InGrappleBoost->GetBoostItemAmount());
				Inventory[InGrappleBoost->GetSlotIndex()] = NewBoost;
				EquipItem(NewBoost, false, true);
				if (bSwitchedItem)
				{
					NewBoost->SetItemState(EItemState::EIS_PickedUp);
				}
			}
		}

		if (!bNoHit && !bSwitchedItem)
		{
			InGrappleBoost->GrappleState = EGrappleState::EGS_Release;
			InGrappleBoost->OnRep_GrappleState();
		}
		else
		{
			InGrappleBoost->GrappleState = EGrappleState::EGS_MAX;
			//UE_LOG(LogTemp, Warning, TEXT("ServerStopGrapplePull Destroy Grapple"));
			// A crash occurs here from the StartGrappleDestoryTimer function in the Boost component if I destroy.
			InGrappleBoost->Destroy();
		}

		//InGrappleBoost->SpendBoost();
		//Boost->RemoveItem(InGrappleBoost);
		
	}
}

void AShooterCharacter::MulticastDestroyGrappleItem_Implementation(ABoostItem* InGrappleBoost)
{
	if (InGrappleBoost) InGrappleBoost->Destroy();
	//UE_LOG(LogTemp, Warning, TEXT("Grapple Destroyed"));
}

void AShooterCharacter::StartDestoryGrappleIfNoHit(ABoostItem* InGrapple)
{
	GrappleToDestroy = InGrapple;
	if (GetWorldTimerManager().IsTimerActive(DestroyGrappleTimer)) GetWorldTimerManager().ClearTimer(DestroyGrappleTimer);
	GetWorldTimerManager().SetTimer(DestroyGrappleTimer, this, &AShooterCharacter::DestoryGrappleIfNoHit, 7.f);
}

void AShooterCharacter::DestoryGrappleIfNoHit()
{
	if (GrappleToDestroy == nullptr) return;
	if (HasAuthority() && GrappleToDestroy && GrappleToDestroy->GrappleState == EGrappleState::EGS_Start)
	{
		ServerStopGrapplePull(GrappleToDestroy, true, false);
	}

	if (IsLocallyControlled() && GrappleToDestroy && GrappleToDestroy->GrappleState == EGrappleState::EGS_Start)
	{
		bGrappleThrown = false;
	}
}

void AShooterCharacter::SetGrappleEffect(bool bIsOn)
{
	if (bIsOn && GrappleSound && FlyingEffect)
	{
		//UE_LOG(LogTemp, Warning, TEXT("SetGrappleEffect ON"));
		UGameplayStatics::SpawnSoundAttached(
			GrappleSound, // sound cue (USoundBase)
			GetMesh(), // mesh to attach to
			FName("Root"),   //socket name
			FVector(0,0,0),  //location relative to socket
			FRotator(0,0,0), //rotation 
			EAttachLocation::KeepRelativeOffset, 
			true //if true, will be deleted automatically
		);

		if (!GrappleFlyingEffectComponent)
		{
			GrappleFlyingEffectComponent = UGameplayStatics::SpawnEmitterAttached(
				FlyingEffect,
				GetMesh(),
				FName("Root"),
				FVector(0,0,0),
				FRotator(-90.f,0,0),
				EAttachLocation::KeepRelativeOffset, 
				false
			);
		}

	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("SetGrappleEffect OFF"));
		if (GrappleFlyingEffectComponent)
		{
			GrappleFlyingEffectComponent->DestroyComponent();
			GrappleFlyingEffectComponent = nullptr;
		}

		if (EndGrappleSound && EndGrappleEffect)
		{
			UGameplayStatics::SpawnSoundAttached(
				EndGrappleSound, // sound cue (USoundBase)
				GetMesh(), // mesh to attach to
				FName(""),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);

			UGameplayStatics::SpawnEmitterAttached(
				EndGrappleEffect,
				GetMesh(),
				FName(""),
				FVector(0,0,0),
				FRotator(0,0,0),
				EAttachLocation::KeepRelativeOffset, 
				true
			);
		}
	}
}

void AShooterCharacter::SetDefaultMaterials()
{
	SetAllSkeletalMeshMaterials(true, nullptr);
}

void AShooterCharacter::ServerSwitchInventoryItems_Implementation(int32 OldSlotIndex, int32 NewSlotIndex)
{
	if (Inventory.IsValidIndex(OldSlotIndex) && Inventory.IsValidIndex(NewSlotIndex) && Inventory[OldSlotIndex] && Inventory[NewSlotIndex] &&
		(CombatState == ECombatState::ECS_Unoccupied) && EquippedItem)
	{	
		auto NewItem = Inventory[NewSlotIndex];
		int32 EquippedItemSlotIndex = EquippedItem->GetSlotIndex();
		Inventory[NewSlotIndex] = Inventory[OldSlotIndex];
		Inventory[NewSlotIndex]->SetSlotIndex(NewSlotIndex);
		Inventory[OldSlotIndex] = NewItem;
		Inventory[OldSlotIndex]->SetSlotIndex(OldSlotIndex);
		//UGameplayStatics::PlaySound2D(this, ItemEquipped->GetEquipSound());

		if (NewSlotIndex == EquippedItemSlotIndex)
		{
			ExchangeInventoryItems(OldSlotIndex, NewSlotIndex);
		}
		else if (OldSlotIndex == EquippedItemSlotIndex)
		{
			ExchangeInventoryItems(NewSlotIndex, OldSlotIndex);
		}
	}
}

void AShooterCharacter::ServerDropItemManual_Implementation(int32 ItemRemoveIndex)
{
	if (Inventory.IsValidIndex(ItemRemoveIndex) && EquippedItem && EquippedItem->GetSlotIndex() != ItemRemoveIndex)
	{
		AItem* ItemToDrop = Inventory[ItemRemoveIndex];
		DropItem(ItemToDrop);
		Inventory.Remove(ItemToDrop);
		for (int32 i=1; i < Inventory.Num(); i++)
		{
			if (Inventory[i] && i != Inventory[i]->GetSlotIndex())
			{
				Inventory[i]->SetSlotIndex(i);
			}
		}
	}
}

void AShooterCharacter::SpawnNFT(const FString &NFT_ID)
{
	ShooterPlayerController = GetShooter_PC();
	if (ShooterPlayerController) ShooterPlayerController->GetWeaponData(NFT_ID);
}

void AShooterCharacter::ServerSpawnNFT_Implementation(const FString &ResponseJSONData)
{
	if (!HasAuthority()) return;
	/*
    FWeaponDataStruct WeaponData;
    if (LoadWeaponDataFromJSON(ResponseJSONData, WeaponData) && WeaponNFTClass)
    {
		UE_LOG(LogTemp, Display, TEXT("LoadWeaponDataFromJSON"));
		FVector SpawnEndPoint = FVector(GetActorLocation() + 200.f * GetActorForwardVector() + FVector(0.f, 0.f, 100.f));
		FCollisionQueryParams CollisionParam;
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SpawnEndPoint);
		SpawnTransform.SetRotation(FQuat(0.f, 0.f, 0.f, 0.f));
        AWeaponNFT* NewNFTWeapon = GetWorld()->SpawnActor<AWeaponNFT>(WeaponNFTClass, SpawnTransform);
        if (NewNFTWeapon)
        {
            NewNFTWeapon->WeaponData = WeaponData;
			NewNFTWeapon->OnRep_WeaponData();
			NewNFTWeapon->SetItemState(EItemState::EIS_Falling);
			NewNFTWeapon->bFallingFromContainer = true;
			NewNFTWeapon->SetItemRarity(EItemRarity::EIR_NFT);
			NewNFTWeapon->UpdateItem();
			NewNFTWeapon->MulticastSpawnEffects();
        }
    }
	*/
}

bool AShooterCharacter::LoadWeaponDataFromJSON(const FString& JSONString, FWeaponDataStruct& OutWeaponData)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON file"));
        return false;
    }

    return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &OutWeaponData, 0, 0);
}

void AShooterCharacter::ServerSetSprinting_Implementation(bool bIsRushing)
{
	bDash = bIsRushing;
	OnRep_Dash();
	DashCharge = 0.f;
}

void AShooterCharacter::OnRep_Dash()
{
	if (bDash)
	{
		//GetCharacterMovement()->MaxAcceleration = 1000.f * BaseAcceleration;
		SetVisibilityOfShooter(false);
		if (DashSound)
		{
			UGameplayStatics::SpawnSoundAttached(
				DashSound, // sound cue (USoundBase)
				GetMesh(), // mesh to attach to
				FName("Root"),   //socket name
				FVector(0,0,0),  //location relative to socket
				FRotator(0,0,0), //rotation 
				EAttachLocation::KeepRelativeOffset, 
				true //if true, will be deleted automatically
			);
		}

		if (DashSystem)
		{
			if (DashNiagComponent && DashNiagComponent->IsActive()) DashNiagComponent->Deactivate();
			DashNiagComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				DashSystem,
				GetRootComponent(),
				FName(),
				GetActorLocation(),
				GetActorRotation(),
				EAttachLocation::KeepWorldPosition,
				false,
				false
			);
			
			if (DashNiagComponent)
			{
				DashNiagComponent->SetWorldScale3D(FVector(1.5f));
				//DashNiagComponent->SetVariableObject(FName("SkeletalMesh"), GetMesh());
				DashNiagComponent->Activate();
			}
		}
		FTimerHandle DashTimer;
		GetWorldTimerManager().SetTimer(DashTimer, this, &AShooterCharacter::StopDash, 0.2f);
		if (IsLocallyControlled()) DashChargeDelegate.Broadcast(0.f);
	}
	else
	{
		FTimerHandle DashTimer;
		GetWorldTimerManager().SetTimer(DashTimer, this, &AShooterCharacter::DeactivateDashEffect, 0.25f);
		//GetCharacterMovement()->MaxAcceleration = BaseAcceleration;
		//GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		SetVisibilityOfShooter(true);
	}
}

void AShooterCharacter::DeactivateDashEffect()
{
	if (DashNiagComponent)
	{
		//DashNiagComponent->DetachFromParent();
		DashNiagComponent->Deactivate();
	}
}

void AShooterCharacter::StopDash()
{
	if (HasAuthority() || IsLocallyControlled())
	{
		bDash = false;
		OnRep_Dash();
		
		if (IsLocallyControlled() && !IsGrappling() && (HoverState != EHoverState::EHS_HoverStart && HoverState != EHoverState::EHS_HoverRush))
		{
			bCrouching ? SetMovementState(EMovementModifiers::EMM_Crouch) : SetMovementState(EMovementModifiers::EMM_Normal);
		}
	}
}

void AShooterCharacter::GrappleButtonPressed(const FInputActionValue& Value)
{
	if (bIsGrappling || bPredictedGrappling)
	{
		// Cancel
		ServerStopGrapple();
		return;
	}
	if (Value.Get<bool>() && CombatState == ECombatState::ECS_Unoccupied && 
		GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Flying && !bGrappleOnCooldown)
	{
		FHitResult TraceHit;
		TraceHit = GrappleTraceHit();

		if (!HasAuthority())
		{
			PlayGrappleThrowMontage();
		}
		ServerStartGrappleThrow(FHitResult());
		bGrappleOnCooldown = true;
		GetWorldTimerManager().SetTimer(GrappleCooldownHandle, this, &AShooterCharacter::ResetGrappleCooldown, 1.0f, false);
	}
}

bool AShooterCharacter::IsGrappling() const
{
    if (IsLocallyControlled())
    {
        return bIsGrappling || bPredictedGrappling;
    }
    return bIsGrappling;
}

void AShooterCharacter::ResetGrappleCooldown()
{
    bGrappleOnCooldown = false;
}

void AShooterCharacter::StartGrappleHook()
{
    if (IsLocallyControlled() && (bIsGrappling || bPredictedGrappling))
    {
        FVector CurrentTarget = bPredictedGrappling ? PredictedGrappleTarget : GrappleTarget;
        FVector CurrentLocation = GetActorLocation();
        float CurrentDistance = FVector::Dist(CurrentLocation, CurrentTarget);

        if (CurrentDistance <= StopDistance)
        {
            ServerStopGrapple(); // Call server to stop; predicts locally if needed
        }
        else
        {
			FVector GrappleBoostDirection = CurrentTarget - CurrentLocation;
			float SpeedFactor = FMath::Clamp(CurrentDistance * 0.01f, 0.1f, 2.f); // Scale movement based on distance
			
			FVector Velocity = GetVelocity();
			FVector Damping = -Velocity * 0.5f; // Apply damping to reduce oscillation
			
			AddMovementInput(GrappleBoostDirection * SpeedFactor + Damping, 1.f);
        }
    }
}

void AShooterCharacter::PlayGrappleThrowMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && GrappleThrowMontage)
	{
		AnimInstance->Montage_Play(GrappleThrowMontage);
		AnimInstance->Montage_JumpToSection(FName("Grapple1"));
	}
}

void AShooterCharacter::ServerStartGrappleThrow_Implementation(const FHitResult& HitInfo)
{
	//PendingHit = HitInfo;
	MulticastPlayThrowMontage();
}

void AShooterCharacter::MulticastPlayThrowMontage_Implementation()
{
	if (IsLocallyControlled() && !HasAuthority()) return;
	PlayGrappleThrowMontage();
}

void AShooterCharacter::ThrowGrappleItem()
{
    if (!IsLocallyControlled()) return;

    UWorld* World = GetWorld();
    if (!GrappleItemClass || !World) return;

    FHitResult CurrentTraceHit = GrappleTraceHit(); // Compute trace at throw time
    FVector ThrowDir = (CurrentTraceHit.ImpactPoint - CurrentTraceHit.TraceStart).GetSafeNormal();
    FRotator ThrowRot = ThrowDir.Rotation();

    AGrappleItem* LocalGrapple = World->SpawnActor<AGrappleItem>(GrappleItemClass, CurrentTraceHit.TraceStart, ThrowRot);
    if (LocalGrapple)
    {
        LocalGrapple->SetInstigator(this);
        LocalGrapple->SetOwner(this);
        LocalGrapple->SetVelocity(ThrowDir);

        if (HasAuthority())
        {
            // Listen server: treat as server grapple
            LocalGrapple->SetIsPredicted(false);
        }
        else
        {
            // Remote client: predicted grapple
            LocalGrapple->SetIsPredicted(true);
            PredictedGrappleItem = LocalGrapple;

            // Request server to spawn authoritative with same params
            ServerThrowGrapple(CurrentTraceHit.TraceStart, ThrowDir);
        }
    }
}

void AShooterCharacter::ServerThrowGrapple_Implementation(const FVector& StartLocation, const FVector& ThrowDirection)
{
    if (CombatState == ECombatState::ECS_Unoccupied && GrappleItemClass)
    {
        AGrappleItem* ServerGrapple = GetWorld()->SpawnActor<AGrappleItem>(GrappleItemClass, StartLocation, ThrowDirection.Rotation());
        if (ServerGrapple)
        {
            ServerGrapple->SetInstigator(this);
            ServerGrapple->SetOwner(this);
            ServerGrapple->SetIsPredicted(false);
			ServerGrapple->SetVelocity(ThrowDirection);
        }
    }
}

void AShooterCharacter::SetGrappleState(bool IsGrappling)
{
	if (auto MoveComp = GetCharacterMovement())
	{
		if (IsGrappling)
		{
			MoveComp->SetMovementMode(EMovementMode::MOVE_Flying);
			MoveComp->MaxFlySpeed = 2.f * BaseFlySpeed;
			MoveComp->MaxAcceleration= 2.f * BaseAcceleration;
		}
		else
		{
			MoveComp->SetMovementMode(EMovementMode::MOVE_Falling);
			MoveComp->MaxFlySpeed = BaseFlySpeed;
			MoveComp->MaxAcceleration= BaseAcceleration;
		}
	}
}

void AShooterCharacter::StartGrapple(const FVector& TargetLocation)
{
    if (HasAuthority())
    {
		//UE_LOG(LogTemp, Warning, TEXT("StartGrapple"));
        bIsGrappling = true;
        GrappleTarget = TargetLocation;
        //SetMovementState(EMovementModifiers::EMM_GrappleStart);
		SetGrappleState(true);

        GetWorldTimerManager().SetTimer(GrappleTimeoutHandle, this, &AShooterCharacter::StopGrapple, GrappleMaxTime, false);

        if (GrappleLandEffect && GrappleLandSystem)
        {
            GrappleLandEffect->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            GrappleLandEffect->SetAsset(GrappleLandSystem);
            GrappleLandEffect->SetWorldLocation(GrappleTarget);
            GrappleLandEffect->Activate(true);
        }
		SetGrappleEffect(true);
		SetAbilityStatus(FName("Grapple"), true);
		ShooterPS = GetShooter_PS();
		if (ShooterPS) ShooterPS->NumGrapples++;
    }
}

void AShooterCharacter::StartPredictedGrapple(const FVector& TargetLocation)
{
    bPredictedGrappling = true;
    PredictedGrappleTarget = TargetLocation;
    //SetMovementState(EMovementModifiers::EMM_GrappleStart);
	SetGrappleState(true);
	//UE_LOG(LogTemp, Warning, TEXT("StartPredictedGrapple"));

    GetWorldTimerManager().SetTimer(PredictedConfirmHandle, this, &AShooterCharacter::CheckPredictedConfirm, PredictedConfirmTime, false);

    if (GrappleLandEffect && GrappleLandSystem)
    {
        GrappleLandEffect->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        GrappleLandEffect->SetAsset(GrappleLandSystem);
        GrappleLandEffect->SetWorldLocation(PredictedGrappleTarget);
        GrappleLandEffect->Activate(true);
    }
	//SetGrappleEffect(true);
}

void AShooterCharacter::StopGrapple()
{
    if (HasAuthority())
    {
		//UE_LOG(LogTemp, Warning, TEXT("StopGrapple"));
        bIsGrappling = false;
        GrappleTarget = FVector::ZeroVector;
        GetWorldTimerManager().ClearTimer(GrappleTimeoutHandle);
        //SetMovementState(EMovementModifiers::EMM_GrappleStop);
		SetGrappleState(false);

        if (GrappleLandEffect)
        {
            GrappleLandEffect->Deactivate();
        }
		SetGrappleEffect(false);
		SetAbilityStatus(FName("Grapple"), false);
    }
}

void AShooterCharacter::PredictedStopGrapple()
{
	//UE_LOG(LogTemp, Warning, TEXT("PredictedStopGrapple"));
    bPredictedGrappling = false;
    PredictedGrappleTarget = FVector::ZeroVector;
    //SetMovementState(EMovementModifiers::EMM_GrappleStop);
	SetGrappleState(false);
    GetWorldTimerManager().ClearTimer(PredictedConfirmHandle);
    if (GrappleLandEffect)
    {
        GrappleLandEffect->Deactivate();
    }
	//SetGrappleEffect(false);
    if (PredictedGrappleItem)
    {
        PredictedGrappleItem->Destroy();
        PredictedGrappleItem = nullptr;
    }
}

void AShooterCharacter::ServerStopGrapple_Implementation()
{
    StopGrapple();
}

void AShooterCharacter::CheckPredictedConfirm()
{
    if (bPredictedGrappling && !bIsGrappling)
    {
        PredictedStopGrapple();
    }
}

void AShooterCharacter::OnRep_GrappleState()
{
    if (bIsGrappling != bClientIsGrappling)
    {
        bClientIsGrappling = bIsGrappling;

        if (bIsGrappling)
        {
			//UE_LOG(LogTemp, Warning, TEXT("OnRep_GrappleState bIsGrappling ON"));
            //SetMovementState(EMovementModifiers::EMM_GrappleStart);
			SetGrappleState(true);
            if (GrappleLandEffect && GrappleLandSystem)
            {
                GrappleLandEffect->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
                GrappleLandEffect->SetAsset(GrappleLandSystem);
                GrappleLandEffect->SetWorldLocation(GrappleTarget);
                GrappleLandEffect->Activate(true);
            }
            SetGrappleEffect(true);
        }
        else
        {
			//UE_LOG(LogTemp, Warning, TEXT("OnRep_GrappleState bIsGrappling OFF"));
            //SetMovementState(EMovementModifiers::EMM_GrappleStop);
			SetGrappleState(false);
            if (GrappleLandEffect)
            {
                GrappleLandEffect->Deactivate();
            }
            SetGrappleEffect(false);
        }
    }

    if (bIsGrappling)
    {
        if (bPredictedGrappling)
        {
            // Validate target
            float TargetDist = FVector::Dist(GrappleTarget, PredictedGrappleTarget);
            if (TargetDist > 100.f)
            {
                // Significant difference; snap to server target (optional correction)
                PredictedGrappleTarget = GrappleTarget;
                if (GrappleLandEffect)
                {
                    GrappleLandEffect->SetWorldLocation(GrappleTarget);
                }
            }
            bPredictedGrappling = false;
        }
    }
    else
    {
        if (bPredictedGrappling)
        {
            PredictedStopGrapple();
        }
    }
}


void AShooterCharacter::SprintButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>() && DashCharge >= 100.f &&
		CombatState == ECombatState::ECS_Unoccupied && GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Flying)
	{
		FVector ShooterVelocity = GetVelocity();
		if (!ShooterVelocity.IsNearlyZero())
		{
			MovingDirection = ShooterVelocity.GetSafeNormal();
			//MovingDirection = FollowCamera->GetForwardVector();

			bDash = true;
			OnRep_Dash();

			switch (GetCharacterMovement()->MovementMode)
			{
			case EMovementMode::MOVE_Walking:
				SetMovementState(EMovementModifiers::EMM_DashGround);
				break;
			default:
				SetMovementState(EMovementModifiers::EMM_DashAir);
				break;
			}

			ServerSetSprinting(true);
		}
	}
}

void AShooterCharacter::SprintButtonReleased(const FInputActionValue& Value)
{
	/*
	if (!Value.Get<bool>())
	{
		ServerSetSprinting(false);
	}
	*/
}

void AShooterCharacter::SetMovementState(EMovementModifiers InMovementState)
{
	ShooterMovementComponent = ShooterMovementComponent == nullptr ? Cast<UShooterMovementComponent>(GetCharacterMovement()) : ShooterMovementComponent;
	if (ShooterMovementComponent)
	{
		DesiredMovementState = InMovementState;
		//UE_LOG(LogTemp, Warning, TEXT("SetMovementState = %s"), *UEnum::GetValueAsString(InMovementState));
		ShooterMovementComponent->WantsState = InMovementState;
	}
}

void AShooterCharacter::OnMissileHit_Implementation(AActor* MissileActor, float DamageAmount)
{ 
	ShooterPS = GetShooter_PS();
	if (!HasAuthority() || !ShooterPS) return;
	
	if (Health > 0 && !ShooterPS->IsABot())
	{
		ShooterPS->bHitByMapMissile = true;
		if (!bBoostProtect)
		{
			UGameplayStatics::ApplyDamage(
				this,
				DamageAmount,
				GetController(),
				MissileActor,
				UDamageType::StaticClass()
			);
		}
	}
}

void AShooterCharacter::SetAbilityStatus(const FName& AbilityName, bool IsActive)
{
	ShooterPS = GetShooter_PS();
	if (ShooterPS) ShooterPS->SetAbilityStatus(AbilityName, IsActive);
}

void AShooterCharacter::OnMatchEnded_Implementation(AShooterPlayerController* ShooterController)
{
	StopMatch();
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Inventory);
	DOREPLIFETIME(ThisClass, EquippedWeapon);
	DOREPLIFETIME(ThisClass, EquippedItem);
	DOREPLIFETIME(ThisClass, CombatState);
	DOREPLIFETIME(ThisClass, bUnEquippedState);
	DOREPLIFETIME(ThisClass, BaseAimRotation);
	DOREPLIFETIME(ThisClass, Health);
	DOREPLIFETIME(ThisClass, Shield);
	DOREPLIFETIME(ThisClass, bAiming);
	DOREPLIFETIME(ThisClass, bCrouching);
	DOREPLIFETIME(ThisClass, TargetMovementSpeed);
	DOREPLIFETIME(ThisClass, bGhostMode);
	//DOREPLIFETIME(ThisClass, InitialItem);
	//DOREPLIFETIME(ThisClass, InitialBoost);
	DOREPLIFETIME(ThisClass, bBoostProtect);
	DOREPLIFETIME(ThisClass, bStartFlying);
	DOREPLIFETIME(ThisClass, bSlowDown);
	//DOREPLIFETIME(ThisClass, TeamID);
	DOREPLIFETIME(ThisClass, TargetSkeletalMeshOverride);
	DOREPLIFETIME_CONDITION(ThisClass, HoverState, COND_SkipOwner);
	DOREPLIFETIME(ThisClass, bPlayerEliminated);
	DOREPLIFETIME(ThisClass, bPlayerWon);
	DOREPLIFETIME(ThisClass, FlyingHeightTarget);
	DOREPLIFETIME(ThisClass, bHandShield);
	DOREPLIFETIME_CONDITION(ThisClass, bDash, COND_SkipOwner);
	DOREPLIFETIME(ThisClass, DashCharge);
	DOREPLIFETIME(ThisClass, HandShieldStrength);
	DOREPLIFETIME(ThisClass, bSlowDownTrigger);
	DOREPLIFETIME(ThisClass, RelevantShooters);
	DOREPLIFETIME(ThisClass, AmmoStruct);
	DOREPLIFETIME(ThisClass, bDisableGameplay);
	DOREPLIFETIME(ThisClass, bGravityPull);
	DOREPLIFETIME(ThisClass, GravityProjectileActor);
	DOREPLIFETIME(ThisClass, bStartGrapplePull);
	DOREPLIFETIME(ThisClass, bSuperJump);
	DOREPLIFETIME(ThisClass, bSuperPunch);
	DOREPLIFETIME(ThisClass, OwnedPawn);
	DOREPLIFETIME(ThisClass, bUsingItem);
	DOREPLIFETIME(ThisClass, HandCombatState);
	DOREPLIFETIME(ThisClass, ChargeAttackStrength);
	DOREPLIFETIME(ThisClass, HoverSystemFuel);
	DOREPLIFETIME(ThisClass, PlayerNameTag);
	DOREPLIFETIME(ThisClass, bIsStunned);
	DOREPLIFETIME(ThisClass, bIsGrappling);
	DOREPLIFETIME(ThisClass, GrappleTarget);
	//DOREPLIFETIME(ThisClass, bHoverFinished);
	//DOREPLIFETIME(ThisClass, bStartChargeAttack);
	//DOREPLIFETIME(ThisClass, GrappleBoostDirection);

	// DOREPLIFETIME_CONDITION(ThisClass, ReplBaseAimRotation, COND_SkipOwner);
}