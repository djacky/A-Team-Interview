// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "InputActionValue.h"
#include "Helicopter.generated.h"

UENUM(BlueprintType)
enum class EHelicopterState : uint8
{
	EHS_PrePossess UMETA(DisplayName = "PrePossess"),
	EHS_StartPossess UMETA(DisplayName = "StartPossess"),
	EHS_StartUnpossess UMETA(DisplayName = "StartUnpossess"),
	EHS_Unpossess UMETA(DisplayName = "Unpossess"),
	EHS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EHeliCombatState : uint8
{
	EHCS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	EHCS_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	EHCS_Equipping UMETA(DisplayName = "Equipping"),
	EHCS_MAX UMETA(DisplayName = "DefaultMAX")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeliHealthHudDelegate, float, HealthPercentage, bool, bUpdateHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHeliDamageTakenDelegate, bool, bDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHeliEquipItemDelegate, int32, CurrentSlotIndex, int32, NewSlotIndex);

class UInputMappingContext;
class UInputAction;

UCLASS()
class SHOOTER_API AHelicopter : public APawn, public IWeaponHitInterface, public IShooterInterface
{
	GENERATED_BODY()

public:
	AHelicopter();

protected:

	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* HeliContext;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookRateAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* TurnRateAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* MovementAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimingActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* SelectActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_1;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_2;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryActionSwitchLeft;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryActionSwitchRight;

	UPROPERTY()
	FVector2D MovementAxisValue;

	void MainMovement(const FInputActionValue& Value);
	void TurnRate(const FInputActionValue& Value);
	void LookRate(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void AimingActionButtonPressed(const FInputActionValue& Value);
	void AimingActionButtonReleased(const FInputActionValue& Value);
	void FireActionButtonPressed(const FInputActionValue& Value);
	void FireActionButtonReleased(const FInputActionValue& Value);
	void SelectButtonPressed(const FInputActionValue& Value);
	void SelectButtonReleased(const FInputActionValue& Value);
	void OneKeyPressed(const FInputActionValue& Value);
	void TwoKeyPressed(const FInputActionValue& Value);

	void InventorySwitchLeft(const FInputActionValue& Value);
	void InventorySwitchRight(const FInputActionValue& Value);
	int32 InventorySelect = 0;

	virtual void BeginPlay() override;

	// Called for forwards/backwards movement
	void MoveForward(float Value);

	// Called for side to side movement
	void MoveRight(float Value);

	// Called via input to turn at a given rate
	// @param Rate, This is a normalized rate, i.e., 1.0 means 100% of desired turn rate
	void TurnAtRate(float Rate);

	// Called via input to look up/down at a given rate.
	// @param Rate, This is a normalized rate, i.e., 1.0 means 100% of desired rate
	void LookUpAtRate(float Rate);

	//Rotate controller based on mouse X movement
	// @param Value, The input value from the mouse movement
	void Turn(float Value);

	//Rotate controller based on mouse Y movement
	// @param Value, The input value from the mouse movement
	void LookUp(float Value);

	//void SelectButtonPressed();
	//void SelectButtonReleased();

	void FireButtonPressed();
	void FireButtonReleased();

	void AimingButtonPressed();
	void AimingButtonReleased();

	UFUNCTION(BlueprintImplementableEvent)
	void RightThrottle(bool bThrottleTurn);

	UFUNCTION(BlueprintImplementableEvent)
	void LeftThrottle(bool bThrottleTurn);

	UFUNCTION(BlueprintImplementableEvent)
	void RightPropelPosition(const FString& PropPosition);

	UFUNCTION(BlueprintImplementableEvent)
	void LeftPropelPosition(const FString& PropPosition);

	UFUNCTION(BlueprintImplementableEvent)
	void InitializeAnimRefs(bool bInitAnimRefs);

	//void SetThrottlePositions();
	void OnMovementStopped();

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	// Called when end overlapping AreaSphere
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnDoorBoxOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	// Called when end overlapping AreaSphere
	UFUNCTION()
	void OnDoorBoxEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(Server, Reliable)
	void ServerSetHeliTransform(FTransform Trans, float HeliSpeed);

	bool TraceFromBase(FHitResult& OutHitResult);
	bool TraceFromCrosshairs(FHitResult& CrosshairHitResult);
	bool TraceFromWeapon(FHitResult &WeaponTraceHit, const FVector &MuzzleSocketLocation, const FVector_NetQuantize &OutBeamLocation);

	void FireWeapon();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FHitResult& WeaponTraceHit, float FireDelay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FHitResult& WeaponTraceHit, float FireDelay);

	void LocalFire(const FHitResult& WeaponTraceHit);

	FHitResult GetHitscanHits(const FVector_NetQuantize& TraceHitTarget);

	bool CanFire();

	void StartFireTimer();
	void AutoFireReset();

	void FireProjectile(const FHitResult& WeaponTraceHit, AShooterCharacter* AttackerCharacter);

	UFUNCTION(BlueprintCallable)
	void StoreHitNumber(UUserWidget* HitNumber, FVector Location);

	UFUNCTION()
	void DestroyHitNumber(UUserWidget* HitNumber);

	UFUNCTION()
	void UpdateHitNumbers();

	void PlayFireSound();

	UFUNCTION()
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	//UFUNCTION()
	//virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void Destroyed() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimHelicopter();

	UFUNCTION()
	void DestroyHelicopter();

	void CheckHelicopterStateAndDestroy();

	void HelicopterFallApart();

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	// Movement replication
	UFUNCTION(Server, Reliable)
	void Set_TargetMovementSpeed(const float InTargetMovementSpeed);

	UPROPERTY(ReplicatedUsing = OnRep_TargetMovementSpeed)
	float TargetMovementSpeed;

	UFUNCTION()
	void OnRep_TargetMovementSpeed(float OldTargetMovementSpeed);

	void SmoothAim(float DeltaTime);

	//void OneKeyPressed();
	//void TwoKeyPressed();

	void ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex);

	UFUNCTION(Server, Reliable)
	void Server_ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex);

	void ExchangeInventoryLocal(int32 CurrentItemIndex, int32 NewItemIndex);

	void InitializeWeapons();

	bool WeaponHasAmmo();

	class AShooterGameMode* GetShooter_GM();
	class AShooterGameState* GetShooter_GS();

	void GetShooterReferences(AShooterCharacter*& AttackerCharacter, AActor* AttackerActor);

	void OnMatchEnded_Implementation(AShooterPlayerController* ShooterController) override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound) override;

	void OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound) override;

	void OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass()) override;

	FVector GetPawnLocation_Implementation() override;

	float GetPawnRotationYaw_Implementation() override;

private:
	// Camera boom positioning the camera behind the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	// Camera that follows the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* RootMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* OpenDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* OpenDoorMesh2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* FrontGuns;

	// Enables overlapping to open door
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* HeliDoorBoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* HeliSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HeliSceneWeapon1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HeliSceneWeapon2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* HeliSoundComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UFloatingPawnMovement* FloatingComponent;

	// Enables overlapping to detect shooter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* HeliSphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* HeliCollisionBox;

	// Base turn rate, in deg/sec
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float BaseTurnRate = 30.f;

	// Base look up/down rate, in deg/sec
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float BaseLookUpRate = 30.f;

	// Turn rate while not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float HipTurnRate = 90.f;

	// Look up rate while not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float HipLookUpRate = 90.f;

	// Turn rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float AimingTurnRate = 20.f;

	// Look up rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float AimingLookUpRate = 20.f;

	// Scale factor for mouse look sensitivity. Turn rate when not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseHipTurnRate = 0.5f;

	// Scale factor for mouse look sensitivity. Look up rate when not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseHipLookUpRate = 2.f;

	// Scale factor for mouse look sensitivity. Turn rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseAimingTurnRate = 0.15f;

	// Scale factor for mouse look sensitivity. Look up rate when aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseAimingLookUpRate = 0.6f;

	// True when aiming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAiming = false;

	float PreviousMoveForwardInput = 0.f;
	float PreviousMoveRightInput = 0.f;

	bool bAimingButtonPressed = false;

	// Default camera field of view
	float CameraDefaultFOV = 0.f; //will be set automatically in BeginPlay

	// Field of view value when zoomed in
	float CameraZoomedFOV = 30.f;

	// Current field of view this frame
	float CameraCurrentFOV = 0.f;

	// Interp speed for zooming when aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed = 20.f;

	bool bToggleHover = false;

	float LastPitch;
	float LastRoll;

	// Did the deprojection true from the HUD?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		bool bScreenToWorld = false;

	// The shot position generated in ShooterCrosshairHUD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		FVector ShotPosition {
		0.f, 0.f, 0.f
	};

	// The shot direction generated in ShooterCrosshairHUD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		FVector ShotDirection {
		0.f, 0.f, 0.f
	};

	bool bFireButtonPressed = false;

	UPROPERTY()
	FVector_NetQuantize HitTarget;

	bool bCanFire = false;

	int8 WeaponIndex = 0;

	// Set a timer beterrn gunshots
	UPROPERTY()
	FTimerHandle AutoFireTimer;

	// Map to store HitNumber widgets and hit locations
	UPROPERTY(VisibleAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TMap<UUserWidget*, FVector> HitNumbers;

	// Time before a HitNumber is removed from the screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HitNumberDestroyTime = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float Health = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 700.f;

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class USoundCue* ExplodeSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundCue* FallingSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* FallingSoundComponent;

	UPROPERTY(EditAnywhere, Category = Combat)
	UParticleSystem* ExplodeEffect;

	bool bBeginDestroy = false;

	float PreviousFallingSpeed = 0.f;
	float CurrentFallingSpeed = 0.f;

	float PreviousFallingHeight = 0.f;
	float CurrentFallingHeight = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CombatState, Category = Combat, meta = (AllowPrivateAccess = "true"))
	EHeliCombatState CombatState = EHeliCombatState::EHCS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float HeliMaxSpeed = 1800.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_EquippedWeapon, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class AHelicopterWeapon* EquippedWeapon;

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AHelicopterWeapon> DefaultWeaponClass;

	// Delegate for updating the health progress bar in the HUD
	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FHeliDamageTakenDelegate DamageTakenDelegate;

	// Delegate for sending slot information to InventoryBar when equipping
	UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
		FHeliEquipItemDelegate EquipItemDelegate;

	UPROPERTY()
	UParticleSystemComponent* ParticleHitComponent1;
	UPROPERTY()
	UParticleSystemComponent* ParticleHitComponent2;
	UPROPERTY()
	UParticleSystemComponent* ParticleHitComponent3;
	UPROPERTY(EditAnywhere, Category = Effects)
	UParticleSystem* ParticleHitEffect1;
	UPROPERTY(EditAnywhere, Category = Effects)
	UParticleSystem* ParticleHitEffect2;
	UPROPERTY(EditAnywhere, Category = Effects)
	UParticleSystem* ParticleHitEffect3;

	bool bHitEffect1 = false;
	bool bHitEffect2 = false;
	bool bHitEffect3 = false;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_InitWeapon(AHelicopterWeapon* Weapon, int32 SlotIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HelicopterSpeed, Category = Combat, meta = (AllowPrivateAccess = "true"));
	float HelicopterSpeed;

	UFUNCTION()
	void OnRep_HelicopterSpeed();

	UPROPERTY()
	AShooterGameMode* ShooterGameMode;
	//UPROPERTY()
	//AShooterGameState* ShooterGameState;

	bool bStartCameraMoveToAIWinner = false;
	UPROPERTY()
	FVector AIWinnerLocation;
	UPROPERTY()
	FRotator AIWinnerRotation;

public:

	FORCEINLINE UStaticMeshComponent* GetRootMesh() const { return RootMesh; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE USphereComponent* GetHeliSphereComponent() const { return HeliSphereComponent; }
	FORCEINLINE UFloatingPawnMovement* GetFloatingComponent() const { return FloatingComponent; }
	FORCEINLINE AHelicopterWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE bool GetAiming() const { return bAiming; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE EHeliCombatState GetCombatState() const { return CombatState; }
	FORCEINLINE void SetRandomShotInfo(bool bScreen, FVector Pos, FVector Dir) { bScreenToWorld = bScreen; ShotPosition = Pos; ShotDirection = Dir; }
	UFUNCTION(Server, Reliable)
	void ServerStartPossess();
	UFUNCTION(Server, Reliable)
	void ServerStopPossess(bool bPlayerWon = false);

	UFUNCTION()
	void UnpossessFromHeli(AShooterCharacter* ShooterCharacter, bool bPlayerWon);

	bool bDriveHelicopter = false;

	bool bDoorOverlap = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat)
	bool bDoorAnimPlaying = false;

	UFUNCTION(BlueprintImplementableEvent)
	void CheckDoorAnimation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HelicopterState);
	EHelicopterState HelicopterState = EHelicopterState::EHS_PrePossess;

	UFUNCTION()
	void OnRep_HelicopterState();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowHitNumber(float DamageAmount, FVector HitLocation, bool bHeadshot, bool bShield = false);

	class AShooterPlayerController* GetShooterController();

    // Delegate for updating the health progress bar in the HUD
    UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
    FHeliHealthHudDelegate HealthHudDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TArray<AHelicopterWeapon*> Inventory;

	void SpawnHelicopterDamageEffects(FVector_NetQuantize HitLocation, UParticleSystem* InParticleEffect);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_OwnedShooter, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AShooterCharacter* OwnedShooter = nullptr;

	UFUNCTION()
	void OnRep_OwnedShooter();

	// Initialize ammo on the helicopter
	void SetAmmo(AShooterCharacter* ShooterCharacter);

	UFUNCTION(BlueprintImplementableEvent)
	void ToggleBackDoor(bool bToggleBackDoor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HeliDoorOpen, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bHeliDoorOpen = false;

	UFUNCTION()
	void OnRep_HeliDoorOpen();

	// The addition "Replicated" argument is needed here for FVector variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HelicopterHitPoint1, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FVector_NetQuantize HelicopterHitPoint1;

	UFUNCTION()
	void OnRep_HelicopterHitPoint1();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HelicopterHitPoint2, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FVector_NetQuantize HelicopterHitPoint2;

	UFUNCTION()
	void OnRep_HelicopterHitPoint2();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HelicopterHitPoint3, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FVector_NetQuantize HelicopterHitPoint3;

	UFUNCTION()
	void OnRep_HelicopterHitPoint3();

	void HelicopterDamageEffects(FVector_NetQuantize HitPointWorld);

	bool CheckFriendlyHit(const AActor* DamagedCharacter, const AActor* AttackerCharacter, 
			AShooterPlayerState* VictimPS, AShooterPlayerState* AttackerPS, AShooterGameState* ShooterGS);

	void StartSmoothCameraToAIWinner(FVector InAILocation, FRotator InAIRotation);
	void SmoothCameraToAIWinner(float DeltaTime);

	UFUNCTION()
	void SetOverlapSphereEnabled(bool bOverlapSphere);

};
