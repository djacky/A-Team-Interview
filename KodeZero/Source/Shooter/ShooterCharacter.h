// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterTeamAgentInterface.h"
#include "Shooter/EnumTypes/AmmoType.h"
#include "Shooter/EnumTypes/ItemType.h"
#include "Shooter/EnumTypes/HoverState.h"
#include "Shooter/EnumTypes/CharacterState.h"
#include "Components/TimelineComponent.h"
#include "InputActionValue.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "Shooter/StructTypes/WeaponPropertyStruct.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "Shooter/Misc/Interfaces/EnemyAIInterface.h"
#include "Shooter/EnumTypes/MovementModifiers.h"
#include "Shooter/StructTypes/CharacterProperties.h"
#include "ShooterCharacter.generated.h"

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	ECS_Equipping UMETA(DisplayName = "Equipping"),
	ECS_HandCombat UMETA(DisplayName = "HandCombat"),
	ECS_Mantle UMETA(DisplayName = "Mantle"),
	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EHandCombatState : uint8
{
	EHCS_hand_r UMETA(DisplayName = "hand_r"),
	EHCS_hand_l UMETA(DisplayName = "hand_l"),
	EHCS_foot_r UMETA(DisplayName = "foot_r"),
	EHCS_foot_l UMETA(DisplayName = "foot_l"),
	EHCS_idle UMETA(DisplayName = "idle"),
	EHCS_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FInterpLocation
{
	GENERATED_BODY() // needed for structs

	// Scene component to use for its location for interping 
	// (weapons and ammos coming to screen when picking them up)
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		USceneComponent* SceneComponent = nullptr;

	// Number of items interping to/at this scene comp location
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int32 ItemCount = 0;
};

USTRUCT(BlueprintType)
struct FAmmoMap
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<EAmmoType> AmmoType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> AmmoAmount;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMontageEnded, UAnimMontage*, Montage, bool, bInterrupted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipItemDelegate, int32, CurrentSlotIndex, int32, NewSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHighlightIconDelegate, int32, SlotIndex, bool, bStartAnimation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHealthHudDelegate, float, HealthPercentage, bool, bUpdateHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShieldStrengthDelegate, float, ShieldStrength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChargeAttackDelegate, float, ChargeAttack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDashDelegate, float, DashChargeAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHoverSystemDelegate, float, HoverFuel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDamageTakenDelegate, bool, bDamaged, bool, bDamageEffect, float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShieldHudDelegate, float, ShieldPercentage, bool, bUpdateShield);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShieldDamageTakenDelegate, bool, bDamaged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIconRotateDelegate, bool, bStartWidgetRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHudAmmoDelegate, int32, AmmoCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHudCarriedAmmoDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHudUpdateAmmoDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyReadyDelegate, bool, bIsReady);

class UInputMappingContext;
class UInputAction;

UCLASS()
class SHOOTER_API AShooterCharacter : public ACharacter, public IShooterTeamAgentInterface, public IWeaponHitInterface, public IShooterInterface, public IEnemyAIInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* ShooterContext;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* PauseAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ChargeAttackAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* FireActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* AimingActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryManageAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* HoverActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* SelectActionPressed;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* EmoteAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_0;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_1;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_2;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_3;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_4;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryAction_5;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryActionSwitchLeft;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* InventoryActionSwitchRight;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* LookRateAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* TurnRateAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* MovementAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ItemScrollAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* GrappleAction;

	void GrappleButtonPressed(const FInputActionValue& Value);

	void SprintButtonPressed(const FInputActionValue& Value);
	void SprintButtonReleased(const FInputActionValue& Value);

	void PauseActionButtonPressed(const FInputActionValue& Value);
	void ChargeAttackButtonPressed(const FInputActionValue& Value);
	void ReloadButtonPressed(const FInputActionValue& Value);

	void InventorySwitchLeft(const FInputActionValue& Value);
	void InventorySwitchRight(const FInputActionValue& Value);

	// Called for forwards/backwards movement
	void MoveForward(float Value);

	// Called for side to side movement
	void MoveRight(float Value);
	void MainMovement(const FInputActionValue& Value);

	// Called via input to turn at a given rate
	// @param Rate, This is a normalized rate, i.e., 1.0 means 100% of desired turn rate
	void TurnAtRate(float Rate);
	void TurnRate(const FInputActionValue& Value);
	void ItemScrolled(const FInputActionValue& Value);

	// Called via input to look up/down at a given rate.
	// @param Rate, This is a normalized rate, i.e., 1.0 means 100% of desired rate
	void LookUpAtRate(float Rate);
	void LookRate(const FInputActionValue& Value);

	//Rotate controller based on mouse X movement
	// @param Value, The input value from the mouse movement
	void Turn(float Value);

	//Rotate controller based on mouse Y movement
	// @param Value, The input value from the mouse movement
	void LookUp(float Value);
	void Look(const FInputActionValue& Value);

	// Called when the fire button is pressed
	UFUNCTION(BlueprintCallable)
	void FireWeapon();

	bool GetBeamEndLocation(FHitResult &WeaponTraceHit,
	const FVector &MuzzleSocketLocation, 
	const FVector_NetQuantize &OutBeamLocation);

	// Set bAiming to true or false with button press
	void AimingActionButtonPressed(const FInputActionValue& Value);
	void AimingActionButtonReleased(const FInputActionValue& Value);
	void AimingButtonPressed();
	void AimingButtonReleased();
	void SmoothAim(float DeltaTime);

	//Set BaseTurnRate and BackLookUpRate based on aiming
	void SetLookRates();

	// Start and end timers for crosshair animation when a shot is fired
	void StartCrosshairBulletFire();
	UFUNCTION()
		void FinishCrosshairBulletFire();

	void FireActionButtonPressed(const FInputActionValue& Value);
	void FireActionButtonReleased(const FInputActionValue& Value);
	UFUNCTION(BlueprintCallable)
	virtual void FireButtonPressed();
	UFUNCTION(BlueprintCallable)
	void FireButtonReleased();
	void StartFireTimer();

	UFUNCTION() //we need to put this because it's a callback function
		void AutoFireReset();

	// Line trace for items under the crosshairs
	virtual bool TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation);

	// Trace for items if OverlappedItemCount > 0
	void TraceForItems();

	void SetGunInfoVisible();

	// Get actor for casting
	AActor* FindSingleActorForCasting(FName Tag, TSubclassOf<AActor> StaticClass);

	AActor* GetOwningActor(FName Tag) const;

	// Spawns a default weapons and equips it
	class AItem* SpawnForDefaultWeapon();

	virtual void InitialEquipWeapon(AItem* ItemToEquip);
	//UFUNCTION(NetMulticast, Reliable)
	
	void LocalInitialEquip(AWeapon* WeaponToEquip);

	UFUNCTION(Client, Unreliable)
	void ClientInventoryAnim(bool bSwap, AItem* OldItem, AItem* ItemEquipped, bool bItemRemoved);

	// Detach weapon and let it fall to the ground
	UFUNCTION(NetMulticast, Reliable)
	void DropItem(class AItem* ItemToThrow = nullptr, bool bPlayerElimed = false);
	UFUNCTION(NetMulticast, Reliable)
	void DropAmmo(class AAmmo* AmmoToThrow = nullptr);
	//UFUNCTION(Server, Reliable)

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Item_EnableGlowMaterial(AItem* InItem);

	void SelectButtonPressed(const FInputActionValue& Value);
	void SelectButtonReleased(const FInputActionValue& Value);
	UFUNCTION()
	void SetSpawnAI();

	void EmoteButtonPressed(const FInputActionValue& Value);

	// Initialize the ammo map with ammo values
	void InitialzeAmmoMap();

	// Check to make sure the weapon has ammo
	bool WeaponHasAmmo();

	// FireWeapon functions
	void PlayFireSound();
	void SendBullet(const FHitResult& WeaponTraceHit);
	void PlayGunfireMontage(FName MontageSectionName);

	void PlayChargeAttackMontage(FName MontageSectionName);
	
	void PlayEmoteMontage(UAnimMontage* InMontage);

	UFUNCTION(Server, Reliable)
	void ServerPlayEmote(UAnimMontage* InMontage);

	UFUNCTION(Server, Reliable)
	void ServerStopFlying();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayEmote(UAnimMontage* InMontage);

	UFUNCTION()
	void SetDefaultEmoteMontage();

	FCharacterProperties* GetCharacterRowProperties(class UShooterAnimInstance* ShooterAnimInstance);

	//void ReloadButtonPressed();
	void HoverActionButtonPressed(const FInputActionValue& Value);
	void HoverActionButtonReleased(const FInputActionValue& Value);
	void HoverButtonPressed();
	void HoverButtonReleased();

	// Handle reloading for the weapon
	void ReloadWeapon();
	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	void UpdateShotgunAmmoValues();

    // Reload on the server side
	UFUNCTION(Server, Reliable)
	void ServerReload();

    // Run reload function
    void Reload(bool bNoAmmoWhenSwitching = false);

	UFUNCTION(BlueprintCallable)
		void FinishReloading();

	UFUNCTION()
	void OnReloadMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintCallable)
		void FinishEquipping();

	// Checks to see if we have ammo for the EquippedWeapons's ammo type
	bool CarryingAmmo();

	// Called from animation blueprint with GrabClip Notify
	UFUNCTION(BlueprintCallable)
		void GrabClip();

	// Called from animation blueprint with ReleaseClip Notify
	UFUNCTION(BlueprintCallable)
		void ReplaceClip();

	UFUNCTION(BlueprintCallable)
	void PauseButtonPressed();

	void InventoryManageButtonPressed(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable)
	void ShowPauseMenu(const TArray<FPlayerGameStats>& PlayerStatsArray);

	// Set the capsule size when crouching and standing
	void InterpCapsuleHeight(float DeltaTime);

	// Take the ammo and add it to your AmmoMap
	void TakeAmmo(class AAmmo* Ammo);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_TakeAmmo(class AAmmo* Ammo);
	UFUNCTION(Client, Reliable)
	void Client_TakeAmmo(AAmmo* Ammo);

	void AutoCollectAmmo();

	void InitializeInterpLocations();

	void FKeyPressed(const FInputActionValue& Value);
	void OneKeyPressed(const FInputActionValue& Value);
	void TwoKeyPressed(const FInputActionValue& Value);
	void ThreeKeyPressed(const FInputActionValue& Value);
	void FourKeyPressed(const FInputActionValue& Value);
	void FiveKeyPressed(const FInputActionValue& Value);

	int32 GetEmptyInventorySlot();

	// Check to see if we shot an actor with line trace, and apply damage
	void ShotCharacter(FHitResult WeaponTraceHit);

	// Check to see if we hit an actor in hand combat mode, and apply damage
	//UFUNCTION(Server, Reliable)
	void HitCharacter(AActor* DamagedCharacter, float HitDamage, AShooterPlayerState* InstigatorPS);

	// Play hit animation when big damage is done
	UFUNCTION(NetMulticast, Unreliable)
		void Multicast_PlayBigDamageMontage(float DamageAmount);

	// Elim functionality
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim(AShooterPlayerState* AttackerPS, int32 RespawnsLeft);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastWinner();

	void PlayElimMontage();
	void PlayWinningMontage();
	virtual void Destroyed() override;
	UFUNCTION()
	void ElimTimerFinished(AShooterPlayerController* VictimController, int32 RespawnsLeft);
	UFUNCTION()
	void RespawnTimerFinished(AShooterCharacter* VictimCharacter);
	void DisableCharacterCollisions();
	UFUNCTION()
	void PlayAITauntSound(AActor* AttackerCharacter);
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	virtual void StartDissolve();

	UFUNCTION()
		virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
		
	UFUNCTION()
		void OnHandOverlap(UPrimitiveComponent* OverlappedComponent,
			AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
			bool bFromSweep, const FHitResult& SweepResult);

	// Play montages for hand combat mode
	void HandCombatAttack();
	void HandCombatAttack_Impl();

	UFUNCTION(Server, Reliable)
		void Server_HandCombatAttack();

	UFUNCTION(NetMulticast, Reliable)
		void Multicast_HandCombatAttack();

	void SetHandCombatProperties(EHandCombatState HandState);
	void SetCombatCollisions();

	// Fire shotgun (with random scatter)
	void FireShotgun();
	void ShotgunLocalFire(const TArray<FHitResult>& WeaponTraceHits);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerShotgunFire(float FireDelay, const TArray<FHitResult>& WeaponTraceHits, const AWeapon* ClientWeapon);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastShotgunFire(const TArray<FHitResult>& WeaponTraceHits);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFire(const FHitResult& WeaponTraceHit, float FireDelay, const AWeapon* ClientWeapon);

	UFUNCTION(NetMulticast, Reliable)
		void MulticastFire(const FHitResult& WeaponTraceHit);

	void BulletEffect(const FVector_NetQuantize& HitPoint, const FTransform& SocketTransform);
	void LocalFire(const FHitResult& WeaponTraceHit);
	UFUNCTION(BlueprintCallable)
	void FireHitScanWeapon();
	void FireProjectileWeapon();

	bool CanFire();
	bool bCanFire = true;
	bool bLocallyReloading = false;

	// Logic when combo starts and finishes
	UFUNCTION(BlueprintCallable)
		virtual void ComboAirStart();
	UFUNCTION(BlueprintCallable)
		virtual void ComboAirFinish();
	UFUNCTION(BlueprintCallable)
		void ComboHit();

	UFUNCTION(Server, Reliable)
		void SetHandCombatState(EHandCombatState InHandCombatState, const FString& State);

	virtual void InitializeComboVariables(const FString& MeshName);
	void SetComboDamageMap(TArray<int32> DamageArray);
	void CheckHittingInAir();

	// Move camera boom a bit to the right when camera is to close to the ShooterCharacter
	// The prevents crosshairs overlapping with the character.
	virtual void CameraOffsetWithDistance();

	UFUNCTION(BlueprintCallable)
		void StoreHitNumber(UUserWidget* HitNumber, FVector Location);

	UFUNCTION()
		void DestroyHitNumber(UUserWidget* HitNumber);

	UFUNCTION()
		void UpdateHitNumbers();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetShield(bool bIsShield);

	// Damage player when falling from a certain velocity
	//UFUNCTION(Server, Reliable)
	//void ServerFallingDamage(float DeltaTime);

	void SmoothCrouching();

	UFUNCTION()
	void OnSlowDownOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSlowDownEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SetSlowCharacter();

	UFUNCTION()
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	UFUNCTION(Client, Reliable)
	void Client_OnPossess(AController* PawnController);

	UFUNCTION(Server, Reliable)
	void ServerSetTargetSkeletalMeshOverride(USkeletalMesh* InTargetSkeletalMeshOverride);

	void SetSkin();
	void InitializeOnPossess();

	UFUNCTION(BlueprintImplementableEvent)
	void OnShooterPossessed();

	UFUNCTION()
	void SetStreamActions(const FString& MessageString);
	UPROPERTY()
	TMap<FString, bool> StreamKeyStates;
	// Used to move player for the streams
	FVector2D StreamCalculateMovementVector() const;
	FVector2D StreamCalculateMouseVector() const;
	bool bIsStreamingMode = false;

	UPROPERTY()
	FGenericTeamId TeamID;

	UPROPERTY(ReplicatedUsing = OnRep_TargetSkeletalMeshOverride)
	USkeletalMesh* TargetSkeletalMeshOverride;

	UPROPERTY()
	FOnShooterTeamIndexChangedDelegate OnTeamChangedDelegate;

	UFUNCTION()
	void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	virtual void SmoothRise(float DeltaTime);

	class AShooterGameMode* GetShooter_GM();
	class AShooterGameState* GetShooter_GS();
	//UPROPERTY()
	//AShooterGameState* ShooterGS;
	UPROPERTY()
	class AShooterPlayerState* ShooterPS;

	void HandlePlayerElimmed(AActor* DamagedCharacter, AActor* DamageCauserCharacter,
		AShooterPlayerController* VictimController, AShooterPlayerController* ShooterAttackerController, 
		AShooterPlayerState* VictimShooterPS, AShooterPlayerState* DamageCauserShooterPS, 
		AShooterGameState* ShooterGameState);
		
	virtual void ReplenishShieldStrength();

	virtual void ReplenishChargeAttackStrength();

	UFUNCTION()
	virtual void ReplenishHoverSystem();

	virtual void ReplenishDashCharge(float DeltaTime);

	UFUNCTION(Server, Unreliable)
	void ServerUpdateShieldPosition();

	UFUNCTION(Server, Reliable)
	void ServerShowPauseMenu();

	UFUNCTION(Client, Reliable)
	void ClientShowPauseMenu(const TArray<FPlayerGameStats>& PlayerStatsArray);

	void StopHittingWhenShieldOn();

	UFUNCTION(Client, Reliable)
	void ClientShowReturnMenu(bool bWinner, AShooterPlayerController* ShooterController, FPlayerGameStats InGameStats, int32 PlayersLeft, int32 RespawnsLeft);

	// In in team mode, check if a team mate is being hit, and prevent damage
	bool CheckFriendlyHit(const AActor* DamagedCharacter, const AActor* AttackerCharacter, 
			AShooterPlayerState* VictimPS, AShooterPlayerState* AttackerPS, AShooterGameState* ShooterGS);

	// Set the dead character as a spectator to an alive player
	void SetVictimView(AShooterPlayerController* VictimController, AShooterPlayerState* VictimPlayerState, AShooterPlayerState* AttackerPlayerState, int32 RespawnsLeft);

	// Set relevancy of the character
	void DisableActor(bool toHide, AShooterCharacter* ShooterChar);

	// Called when overlapping AreaSphere
	UFUNCTION()
	void OnRelevancyOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	// Called when end overlapping AreaSphere
	UFUNCTION()
	void OnRelevancyEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Debug function to show when thing are being called from (Client or Server)
	void ScreenMessage(FString Mess);

	// Set camera rotation for the winning player
	void WinnerFixCamera(float DeltaTime);

	float GetAngleBetweenPlayers(const FVector& player1, const FVector& player2);

	void GetHairMaterialForLeo();

	TMap<USkeletalMeshComponent*, TArray<UMaterialInterface*>> OriginalMaterialsMap;

	void SetVisibilityOfShooter(bool bVisible);

	void PullToGravityProjectile();

	void StartGrapplePull();

	void GrapplePullTimeout();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDestroyGrappleItem(ABoostItem* InGrappleBoost);

	UFUNCTION(Server, Reliable)
	void ServerPossessHelicopter(class AHelicopter* HelicopterActor);

	virtual void SetInitialBoost();

	UFUNCTION(Server, Reliable)
	void ServerHelicopterDoor(AHelicopter* HelicopterActor);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetInitialItem(AItem* WeaponToEquip);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetInitialBoost(ABoostItem* InitialBoost);

	void LocalPlaySuperPunch();

	UFUNCTION(Server, Reliable)
	void ServerEndSuperPunch(FName ItemMontageSection, ABoostItem* BoostItem);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEndSuperPunch(FName ItemMontageSection, ABoostItem* BoostItem);

	UFUNCTION(BlueprintCallable)
	void SuperPunchDamage();

	void HandleStopComboHit();

	void StartChargeAttack();

	UFUNCTION(Server, Reliable)
	void ServerStartChargeAttack(FName MontageSectionName, FVector InTargetChargeLocation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartChargeAttack(FName MontageSectionName, FVector InTargetChargeLocation);

	UFUNCTION()
	virtual void OnCapsuleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastChargeHit(AShooterCharacter* ChargeHitShooter, UPrimitiveComponent* HitComponent, float DamageAmount);

	UFUNCTION()
	void RestartMusic(AMusicManager* MusicManager, bool bMusicPlaying);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSwitchInventoryItems(int32 OldSlotIndex, int32 NewSlotIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerDropItemManual(int32 ItemRemoveIndex);

	UFUNCTION()
	void OnMontageFinished(class UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnFireMontageFinished(class UAnimMontage* Montage, bool bInterrupted);

	FOnMontageEnded ItemMontageFinishedDelegate;
	FOnMontageEnded FireingMontageFinishedDelegate;

	UFUNCTION()
	void ResetCharacterRotation();

	AShooterCharacter* GetShooterCharacter_Implementation();

	EWeaponType GetWeaponType_Implementation() override;

	virtual bool CanTarget_Implementation() override;
	void OnValidTarget_Implementation(AActor* AttackerAI) override;

	void JumpButtonPressed();
	void JumpButtonReleased();

	void StopAllHoverEffects();

	void SetLocalRotation();

    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    float RotationUpdateThreshold = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Rotation")
    float RotationInterpSpeed = 10.0f; // Degrees per second for interpolation

    FRotator LastSentBaseAim;
    FRotator LastRotationInput;
	bool bWasRotating;

	FRotator DenormalizeRotation(const FRotator& Rotation) const;

	UFUNCTION(Server, Unreliable)
	void ServerSetBaseAimRotation(const FRotator& NewRotation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameplayTags")
    FGameplayTagContainer CharacterStateTags;

	FGameplayTagContainer GetStateTags_Implementation() override;

	void OnMissileHit_Implementation(AActor* MissileActor, float DamageAmount) override;

private:

	void CheckHandCombatHit();

	void TriggerHover();

	UFUNCTION(Server, Reliable)
	void ServerTriggerHover(EHoverState InHoverState);

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bIsSprinting);

	void StartHovering();

	void SetHoverProperties();

	UFUNCTION()
	void SetPlayerNameTag();

	UFUNCTION()
	void SetHoverReady();

	void StartDash();

	UFUNCTION()
	void StopDash();

	UFUNCTION()
	void DeactivateDashEffect();

	bool bHoverReady = true;
	bool bHoverButtonPressed = false;
	bool bIsFiring = false;

	void MissedShotTrace(const FHitResult& WeaponTraceHit);

	UFUNCTION()
	void DisableCollisions();

	UPROPERTY(EditDefaultsOnly, Category = "Grapple")
	TSubclassOf<class AGrappleItem> GrappleItemClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_GrappleState, Category = "Grapple", meta = (AllowPrivateAccess = "true"))
	bool bIsGrappling = false;

	UPROPERTY(ReplicatedUsing = OnRep_GrappleState)
	FVector GrappleTarget;

	UFUNCTION()
	void OnRep_GrappleState();

	bool bClientIsGrappling = false;

	bool bPredictedGrappling = false;
	FVector PredictedGrappleTarget;
	UPROPERTY()
	AGrappleItem* PredictedGrappleItem = nullptr;

	FTimerHandle GrappleTimeoutHandle;
	FTimerHandle PredictedConfirmHandle;

	float GrappleMaxTime = 8.f; // Max time before auto-stop
	float StopDistance = 150.f; // Distance threshold to stop
	float PredictedConfirmTime = 0.5f; // Time to wait for server confirmation

	UPROPERTY(EditDefaultsOnly, Category = "Grapple")
	UAnimMontage* GrappleThrowMontage;
	FHitResult PredictedHit;
	FHitResult PendingHit;

	UFUNCTION(Server, Reliable)
	void ServerThrowGrapple(const FVector& StartLocation, const FVector& ThrowDirection);

	UFUNCTION(Server, Reliable)
	void ServerStopGrapple();

	void StopGrapple();
	void PredictedStopGrapple();
	void CheckPredictedConfirm();

	UFUNCTION(Server, Reliable)
	void ServerStartGrappleThrow(const FHitResult& HitInfo);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayThrowMontage();

	UFUNCTION(BlueprintCallable)
	void ThrowGrappleItem();

	void PlayGrappleThrowMontage();

	void StartGrappleHook();

	UFUNCTION()
	void ResetGrappleCooldown();

	FTimerHandle GrappleCooldownHandle;
	bool bGrappleOnCooldown = false;

protected:
	// Called to determine what happens to the team ID when possession ends
	virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	{
		// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
		return FGenericTeamId::NoTeam;
	}

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//~INLTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	UFUNCTION(BlueprintCallable)
		FGenericTeamId BPGetGenericTeamId() const;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnShooterTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of INLTeamAgentInterface interface

	void StartGrapple(const FVector& TargetLocation);
	void StartPredictedGrapple(const FVector& TargetLocation);

	void ReloadEmptyWeapon();

	//UFUNCTION(Server, Reliable)
	void CheckMatchState();

	//UFUNCTION(Server, Reliable)
	void OnPossessRespawn();

	UFUNCTION(BlueprintImplementableEvent)
	void ShieldTrigger(bool bShieldOn);

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateShieldColor(float HandShieldStrengthVal);

	UFUNCTION(BlueprintImplementableEvent)
	void CheckAimingWidget();

    UPROPERTY(ReplicatedUsing=OnRepBaseAimRotation)
    FRotator BaseAimRotation;

    UFUNCTION()
    void OnRepBaseAimRotation();

	FRotator CurrentInterpolatedRotation; // Client-side interpolated rotation

	AShooterPlayerState* GetShooter_PS();

	TArray<FHitResult> GetShotgunHits(const TArray<FVector_NetQuantize>& HitTargets);

	// Functions to execute to local client when changing inventory item/weapon
	UFUNCTION(BlueprintCallable)
	void ExchangeInventoryLocal(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved = false);

	// Set animations when changing inventory item/weapon
	UFUNCTION(BlueprintCallable)
	void LocalInventoryAnim(int32 CurrentItemIndex, int32 NewItemIndex);

	FHitResult GrappleTraceHit();

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	virtual void Landed(const FHitResult& Hit) override;

	EMovementModifiers DesiredMovementState = EMovementModifiers::EMM_Normal;

	UFUNCTION()
	void CheckVelocity(EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void CrouchActionPressed(const FInputActionValue& Value);
	void CrouchButtonPressed();

	void PracticeRespawn();

	UFUNCTION(BlueprintCallable)
	bool IsPracticeMode();

	void SetFireRateDuringSlowMo(AShooterCharacter* SlowedChar, bool IsSlowActive);

	UFUNCTION()
	void StartWebSocketForStream(const FString& PlayerName);

	UFUNCTION()
	void SetYawAfterLanding();

	void GetAllSkeletalMeshMaterials();

	void SetAllSkeletalMeshMaterials(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst);

	UFUNCTION()
	void OnRep_TargetSkeletalMeshOverride(USkeletalMesh* OldTargetSkeletalMeshOverride);

	void PunchedEffect(FVector CombatHitLocation, bool bIsChargeAttack);

protected:
	// Camera boom positioning the camera behind the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UShooterSpringArmComp* CameraBoom;

	// Camera that follows the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	// Camera that follows the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USkeletalMeshComponent* FirstPersonArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USceneComponent* FPSCameraPivot;

	// Player name widget to display team member player names
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		class UWidgetComponent* PlayerNameWidget;

	UPROPERTY()
	USceneComponent* SwingLocationComponent;

	// Array of all materials of character (used for elim dissolve animation)
	UPROPERTY()
	TArray<UMaterialInterface*> AllMaterials;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	// Set this in Blueprints for the default weapon class
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<AWeapon> DefaultWeaponClass;

	// Set this in Blueprints for the default item class
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<AItem> DefaultItemClass;

	// Map to keep track of ammo of the different ammo types
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AmmoStruct, Category = Items, meta = (AllowPrivateAccess = "true"))
		FAmmoMap AmmoStruct;

	UFUNCTION()
		void OnRep_AmmoStruct();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	// Regular movement speed
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
		float BaseMovementSpeed = 650.f;

	// Crouching movement speed
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
		float CrouchMovementSpeed = 300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FRotator BaseRotationRate = FRotator(0.f, 540.f, 0.f);

	UPROPERTY(EditAnywhere, Category = BoostItem);
	TSubclassOf<ABoostItem> BoostItemClass;

	UPROPERTY()
		TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBoostComponent* Boost;

	UFUNCTION()
	virtual void OnRep_EquippedItem(AItem* OldEquippedItem);

	UPROPERTY(VisibleAnywhere, Replicated, Category = BoostItem)
	FVector_NetQuantize FlyingHeightTarget;

	//UPROPERTY(VisibleAnywhere, Replicated, Category = BoostItem)
	bool bFlyingInterping = false;

	float BaseFlySpeed = 1000.f;

	float BaseAirControl = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
		float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
		// UPROPERTY(VisibleAnywhere, Category = "Player Stats")
		float Health = 100.f;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Player Stats")
	bool bPlayerWon = false;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Player Stats")
	bool bPlayerEliminated = false;

	void SetAbilityStatus(const FName& AbilityName, bool IsActive = false);

	UPROPERTY()
	FTimerHandle FlyTimer;

private:

	// Camera boom positioning the camera above the character (minimap)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* MiniCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneCaptureComponent2D* MiniSceneComp;
	// Mesh for shield
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ShieldMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* FallingSoundComponent;

	// Base turn rate, in deg/sec
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float BaseTurnRate = 45.f;

	// Base look up/down rate, in deg/sec
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float BaseLookUpRate = 45.f;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float MouseHipTurnRate = 1.0f;

	// Scale factor for mouse look sensitivity. Look up rate when not aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float MouseHipLookUpRate = 1.0f;

	// Scale factor for mouse look sensitivity. Turn rate when aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float MouseAimingTurnRate = 0.4f/3.f;

	// Scale factor for mouse look sensitivity. Look up rate when aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
		float MouseAimingLookUpRate = 0.4f/3.f;

	bool bConstantItemSwap = false;
	UPROPERTY()
	AItem* CurrentItemToSwap = nullptr;

	// Montage for firing weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HipFireMontage;

	// Montage for firing weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ChargeAttackMontage;

	// Montage for Reloading
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	// Montage for Equipping (changing weapons from inventory)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EquipMontage;

	// Montage for hand combo attack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ComboMontage;

	// Montage for hand combo attack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EmoteMontage;

	// Montage for when character is hit with big damage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitMontage;

	// Montage for character elimination
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* LandMontage;

	// Montage for character winning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* WinMontage;

	// Montage when character uses item
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ItemUseMontage;

	// True when aiming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Aiming, Category = Combat, meta = (AllowPrivateAccess = "true"))
		bool bAiming = false;

	UFUNCTION()
	void OnRep_Aiming();

	// Default camera field of view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float CameraDefaultFOV = 0.f; //will be set automatically in BeginPlay

	// Field of view value when zoomed in
	float CameraZoomedFOV = 30.f;

	// Current field of view this frame
	float CameraCurrentFOV = 0.f;

	// Field of view value when rushing
	float CameraRushFOV = 120.f;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float ScaledAimingTurnRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float ScaledAimingLookUpRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (AllowPrivateAccess = "true"))
	float GlobalZoomRatio = 1.f;

	// Get current size of the viewport
	UPROPERTY()
	FVector2D ViewportSize;

	// Interp speed for zooming when aiming
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
		float ZoomInterpSpeed = 20.f;

	//class AShooterCrosshairHUD* CrosshairHUD;
	//float MoveFactor;

	// Variables for slightly expanding the crosshair when shooting
	float ShootTimeDuration = 0.075f;
	bool bFiringBullet = false;
	UPROPERTY()
	FTimerHandle CrosshairShootTimer;

	// Fire button pressed
	bool bFireButtonPressed = false;

	// Set a timer beterrn gunshots
	UPROPERTY()
	FTimerHandle AutoFireTimer;

	// True if we should trace every frame for items
	bool bShouldTraceForItems = false;

	// Number of overlapped AItems
	int8 OverlappedItemCount;

	// The AItem we hit last frame
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
		class AItem* TraceHitItemLastFrame;

	// Currently equipped weapon
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		AWeapon* EquippedWeapon;

	// Currently equipped weapon
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_EquippedItem, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		AItem* EquippedItem;

	// The item currently hit by our trace in TraceForItems (could be null)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		AItem* TraceHitItem;

	// Distance outward from the camera for the interp destination
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
		float CameraInterpDistance = 250.f;

	// Distance upward from the camera for the interp destination
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
		float CameraInterpElevation = 65.f;

	// Starting amount of 9mm ammo (pistol + SMG)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 Starting9mmAmmo = 85;

	// Starting amount of 45mm ammo (sniper rifle)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 Starting45mmAmmo = 6;

	// Starting amount of AR ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 StartingARAmmo = 120;

	// Starting amount of pistol ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 StartingPistolAmmo = 40;

	// Starting amount of shotgun ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 StartingShotgunAmmo = 12;

	// Starting amount of grenade launcher
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Items, meta = (AllowPrivateAccess = "true"))
		int32 StartingGrenadeLauncherAmmo = 10;

	// Combat state, can only fire or relaod is Unoccupied
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CombatState, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	// Combat state, can only fire or relaod is Unoccupied
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		bool bUnEquippedState = true;

	// Transform of the clip when we first grab the clip during reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		FTransform ClipTransform;

	// Scene component to attach to the characters hand during reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		USceneComponent* HandSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		USceneComponent* SwingComponentH;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		USceneComponent* SwingComponentL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		USceneComponent* SwingComponentR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		USceneComponent* SwingComponentLo;

	// Is character crouching?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Crouching, Category = Movement, meta = (AllowPrivateAccess = "true"))
		bool bCrouching = false;

	// The current capsule height
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Capsule, meta = (AllowPrivateAccess = "true"))
		float CurrentCapsuleHeight = 88.f;

	// The capsule height while standing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Capsule, meta = (AllowPrivateAccess = "true"))
		float StandingCapsuleHeight = 88.f;

	// The capsule height while crouching
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Capsule, meta = (AllowPrivateAccess = "true"))
		float CrouchingCapsuleHeight = 44.f;

	float StandingCameraOffsetZ;
	float CrouchedCameraOffsetZ;

	// Ground friction while not crouching
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
		float BaseGroundFriction = 3.f;

	// Ground friction while crouching
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
		float CrouchingGroundFriction = 15.f;

	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	// Setup interpolation components
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* WeaponInterpComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USceneComponent* InterpComp6;

	// Array of interp location structs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		TArray<FInterpLocation> InterpLocations;

	// Inventory of AItems
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	TArray<AAmmo*> AmmoInventory;

	const int32 InventoryMaxCapacity = 6;

	// Delegate for sending slot information to InventoryBar when equipping
	UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
		FEquipItemDelegate EquipItemDelegate;

	// Delegate for sending slot information for playing inventory icon animation
	UPROPERTY(BlueprintAssignable, Category = Delegates, meta = (AllowPrivateAccess = "true"))
		FHighlightIconDelegate HighlightIconDelegate;

	// Delegate for rotating boost icon in widget(animation)
	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FIconRotateDelegate IconRotateDelegate;

	// Delegate for updating the health progress bar in the HUD
	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FDamageTakenDelegate DamageTakenDelegate;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FShieldDamageTakenDelegate ShieldDamageTakenDelegate;

	// The index or the currently highlighted  inventory slot
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
		int32 HighlightedSlot = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
		float MaxShield = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Player Stats", meta = (AllowPrivateAccess = "true"))
		// UPROPERTY(VisibleAnywhere, Category = "Player Stats")
		float Shield = 0.f;

	bool bBigDamage = false;

	// The shot position randomely generated in ShooterCrosshairHUD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		FVector ShotPosition {
		0.f, 0.f, 0.f
	};

	// The shot direction randomely generated in ShooterCrosshairHUD
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		FVector ShotDirection {
		0.f, 0.f, 0.f
	};

	// Did the deprojection true from the HUD?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		bool bScreenToWorld = false;

	// State in hand combat modee (hitting or idle)
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_HandCombatState, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		EHandCombatState HandCombatState = EHandCombatState::EHCS_idle;

	// Time (in seconds) when a combo Montage starts
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		float ComboStartTimeNow = 0;

	// Time (in seconds) of the previous Montage animation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		float ComboStartTimePrev = 0;

	// Array of section names contained in the combo montage
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		TArray<FName> ComboMontageNames = { FName("Combo1"), FName("Combo2"), FName("Combo3"), FName("AirAttack") };

	// Section name of combo montage currently playing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		FName ComboMontageSectionPlaying;

	// Map of combo hit and hit damage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		TMap<FName, int32> ComboDamageMap;

	// Index of ComboMontageNames: gets incremented when a montage section plays
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		int32 ComboMontageIndex = 0;

	// Hit notification from montage
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
	//	bool bComboHit = false;

	// Is character hitting in the air?
	bool bHittingInAir = false;

	// Is combo montage playing?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		bool bComboPlaying = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		class USoundCue* CharacterPunchedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		UParticleSystem* ComboHitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		USoundCue* CharacterChargeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		USoundCue* StunnedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
		UParticleSystem* CharacterChargeEffect;

	bool bIsComboMontage = false;

	// Map to store HitNumber widgets and hit locations
	UPROPERTY(VisibleAnywhere, Category = Combat, meta = (AllowPrivateAccess = "true"))
		TMap<UUserWidget*, FVector> HitNumbers;

	// Time before a HitNumber is removed from the screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
		float HitNumberDestroyTime = 1.5f;

	/*
	UPROPERTY(EditAnywhere)
		class UBoxComponent* hand_l;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* lowerarm_l;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* hand_r;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* lowerarm_r;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* foot_r;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* foot_l;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* calf_r;

	UPROPERTY(EditAnywhere)
		class UBoxComponent* calf_l;
	*/

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	//	class UBoxComponent* spine_01;

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UTimelineComponent* DissolveTimeline;
	UPROPERTY()
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = BoostItem)
	UMaterialInstanceDynamic* DynamicGhostMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UMaterialInstance* GhostMaterialInstance;

	UPROPERTY()
	FTimerHandle ElimTimer;
	UPROPERTY()
	FTimerHandle RespawnTimer;
	UPROPERTY()
	FTimerHandle ShowMenuTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	UPROPERTY(EditDefaultsOnly)
	float RespawnDelay = 5.f;

	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere, Category = Elim)
	USoundCue* ElimBotSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBlockchain* Blockchain;

    // Variables needed to determine damage when character is falling
	float FallingTime = 0.f;
	float FallingVelocity = 0.f;
	float StartFallTime = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bStartFalling = false;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_GhostMode, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bGhostMode = false;

	//UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_InitialItem, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	//AItem* InitialItem;

	//UFUNCTION()
	//void OnRep_InitialItem();

	//UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_InitialBoost, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	//ABoostItem* InitialBoost;

	//UFUNCTION()
	//void OnRep_InitialBoost();

	// Variables used for protect boost
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_BoostProtect, Category = BoostItem)
	bool bBoostProtect = false; 
	UFUNCTION()
	void OnRep_BoostProtect();

	UPROPERTY()
	UParticleSystemComponent* ProtectComponent;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* ProtectionEffect;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	class USoundBase* ProtectionStartSound;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* ProtectionEndSound;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* ProtectionEndEffect;

	UPROPERTY(EditAnywhere, Category = BoostItem)
	class USoundBase* GhostStartSound;

	// Sound for local player when ghost mode stops
	UPROPERTY(EditAnywhere, Category = BoostItem)
	class USoundCue* GhostStopSound;

	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* GhostStopEffect;

	UPROPERTY()
	UParticleSystemComponent* SlowParticleEffectComponent;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* SlowParticleEffect;

	UPROPERTY(EditAnywhere, Category = Shield)
	USoundBase* ShieldOnSound;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* FallingSound;

	// Sound for local player when ghost mode stops
	UPROPERTY(EditAnywhere, Category = Combat)
	USoundCue* HeadShotSound;

	UPROPERTY(VisibleAnywhere, Category = BoostItem)
	UMaterialInstanceDynamic* DynamicProtectMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UMaterialInstance* ProtectMaterialInstance;

	bool bCrouchLag = false;

	UPROPERTY()
	FString ShooterMeshName;

	UPROPERTY(VisibleAnywhere, Category = BoostItem)
	FOnTimelineFloat FlyingTrack;

	UPROPERTY(EditAnywhere, Category = BoostItem)
	UCurveFloat* FlyingCurve;

	UPROPERTY(VisibleAnywhere, Category = BoostItem)
	UTimelineComponent* FlyingTimeline;

	float StartZLocation = 0.f;
	UPROPERTY()
	UAudioComponent* FlyingSoundComponent;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* FlyingStartSound;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* FlyEndSound;

	UPROPERTY()
	UParticleSystemComponent* FlyingEffectComponent;

	UPROPERTY()
	UParticleSystemComponent* HandFlyingEffectComponent;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* HandFlyingEffect;

	UPROPERTY(EditAnywhere, Category = StartMatch)
	UParticleSystem* StartMatchEffect;

	UPROPERTY(EditAnywhere, Category = StartMatch)
	class USoundBase* StartMatchSound;

	UPROPERTY(EditAnywhere, Category = "Spawn In")
	USoundBase* SpawnInSound;

	UPROPERTY()
	UAudioComponent* SlowMoSoundComponent;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* SlowMoStartSound;

	// Enables Item tracing when overlapped
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* SlowSphere;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SlowDown, Category = BoostItem)
	bool bSlowDown = false;
	UFUNCTION()
	void OnRep_SlowDown();

	UPROPERTY()
	class AShooterGameMode* ShooterGameMode;
	UPROPERTY()
	class AShooterPlayerController* ShooterPlayerController;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AShooterSpectatorPawn> ShooterSpectatorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HandShield, Category = Shield, meta = (AllowPrivateAccess = "true"))
	bool bHandShield = true;

	UFUNCTION()
	void OnRep_HandShield();

	float MaxHandShieldStrength = 100.f;
	float MaxHoverFuel = 100.f;

	UPROPERTY()
	FTimerHandle HoverSystemReplenishTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Shield, meta = (AllowPrivateAccess = "true"))
	USceneComponent* ShieldLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SlowDownFactor, Category = Boost, meta = (AllowPrivateAccess = "true"))
	bool bSlowDownTrigger = false;

	UFUNCTION()
	void OnRep_SlowDownFactor();

	// Overlap sphere for picking up component
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Relevancy, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* RelevancySphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_RelevantShooters, Category = Relevancy, meta = (AllowPrivateAccess = "true"))
	TArray<AShooterCharacter*> RelevantShooters;

	UFUNCTION()
	void OnRep_RelevantShooters();

	bool bWinnerCameraInterp = false;

	UPROPERTY()
	UMaterialInterface* LeoHairMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bGravityPull = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AActor* GravityProjectileActor;

	UPROPERTY(EditAnywhere, Category = Combat);
	TSubclassOf<AShooterCharacter> AIShooterClass;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cable, meta = (AllowPrivateAccess = "true"))
	//class UCableComponent* GrappleCable;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_StartGrapplePull, Category = BoostItem)
	bool bStartGrapplePull = false;

	UFUNCTION()
	void OnRep_StartGrapplePull();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	ABoostItem* GrappleBoost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bGrappleInterp = false;

	UPROPERTY()
	FTimerHandle GrappleTimer;

	float BaseAcceleration = 1524.f;

	UPROPERTY()
	FTimerHandle FallingTimer;

	FTimerDelegate FallingDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bGrappleItemEquipped = false;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FHudUpdateAmmoDelegate HudUpdateAmmoOnEquip;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FHudCarriedAmmoDelegate HudCarriedAmmoDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	ECharacterProperty CharacterSelected = ECharacterProperty::ECP_MAX;

	UPROPERTY()
	bool bChargeAttack = false;

	UPROPERTY()
	bool bWorldTeleporting = false;

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* WinnerSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bCanTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bManageInventory = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAimingTriggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* MissileDamageEffectComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HoverState, Category = Combat, meta = (AllowPrivateAccess = "true"))
	EHoverState HoverState = EHoverState::EHS_HoverFinish;

	UFUNCTION()
	void OnRep_HoverState(EHoverState OldHoverState);

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* CyberBootsSound;

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* CyberBootsOffSound;

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* CyberBootsRushSound;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* CyberBootEffect;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* CyberBootRushEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* CyberBootEffectComponentLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* SlowNiagaraEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* CyberBootEffectComponentRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* GrappleLandEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	UNiagaraSystem* GrappleLandSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* GenBoostEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* GenBoostSoundComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UShooterGameInstance* ShooterGI;

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* SpawnAISoundPractice;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FName CurrentItemMontageSection;

	bool bStartCameraMoveToAIWinner = false;
	UPROPERTY()
	FVector AIWinnerLocation;
	UPROPERTY()
	FRotator AIWinnerRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UShooterSaveGame* SaveGame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bIsPossessed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* GrappleFlyingEffectComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* GrappleSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Dash, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bDash = false;

	UFUNCTION()
	void OnRep_Dash();

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* DashSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bCameraDistanceOk = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector MovingDirection;

	UPROPERTY(VisibleAnywhere)
	TArray<FHitResult> MissedShotResults;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UShooterMovementComponent* ShooterMovementComponent;

	UPROPERTY(EditAnywhere, Category = "Boost Properties")
	UParticleSystem* EndGrappleEffect;

	UPROPERTY(EditAnywhere, Category = "Boost Properties")
	USoundBase* EndGrappleSound;

	void SetGrappleState(bool IsGrappling);

public:
	// Returns CameraBoom subobject
	FORCEINLINE UShooterSpringArmComp* GetCameraBoom() const { return CameraBoom; }
	// Returns FollowCamera subobject
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE UShooterMovementComponent* GetShooterMovementComponent() const { return ShooterMovementComponent; }

	FORCEINLINE FVector2D GetViewportVector() const { return ViewportSize; }
	FORCEINLINE AItem* GetTraceHitItem() const { return TraceHitItem; }
	FORCEINLINE bool GetCanTarget() const { return bCanTarget; }
	FORCEINLINE void SetCanTarget(bool bInCanTarget) { bCanTarget = bInCanTarget; }
	FORCEINLINE bool GetAiming() const { return bAiming; }
	FORCEINLINE bool GetBulletFire() const { return bFiringBullet; }
	void TimerElapsed();

	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE void SetDisableGameplay(bool bInDisable) { bDisableGameplay = bInDisable; }
	
	FORCEINLINE bool GetSprinting() const { return bDash; }
	FORCEINLINE void SetSprinting(bool bIsDash) { bDash = bIsDash; }

	FORCEINLINE int8 GetOverlappedItemCount() const { return OverlappedItemCount; }
	
	FORCEINLINE UParticleSystemComponent* GetMissileDamageEffectComp() const { return MissileDamageEffectComp; }
	// Adds/Subtracts to/from OverlappedItemCount and updates bShouldTraceForItems
	void IncrementOverlappedItemCount(int8 Amount);

	// No longer needed...AItem has GetInterpLocation
	//FVector GetCameraInterpLocation();

	UFUNCTION(Server, Reliable)
		void GetPickupItem(AItem* Item);

	FORCEINLINE void SetRandomShotInfo(bool bScreen, FVector Pos, FVector Dir) { bScreenToWorld = bScreen; ShotPosition = Pos; ShotDirection = Dir; }

	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void SetCombatStateLocal(ECombatState NewCombatState) { CombatState = NewCombatState; }
	FORCEINLINE bool GetCrouching() const { return bCrouching; }

	//FORCEINLINE UCableComponent* GetGrappleCable() const { return GrappleCable; }
	FORCEINLINE ECharacterProperty GetCharacterSelected() const { return CharacterSelected; }
	FORCEINLINE void SetCharacterSelected(ECharacterProperty InCharacterType) { CharacterSelected = InCharacterType; }
	FORCEINLINE bool GetUnEquippedState() const { return bUnEquippedState; }
	FORCEINLINE bool GetPlayerEliminated() const { return bPlayerEliminated; }
	FORCEINLINE bool GetPlayerWon() const { return bPlayerWon; }
	FORCEINLINE bool GetComboPlaying() const { return bIsComboMontage; }
	FORCEINLINE bool GetHittingInAir() const { return bHittingInAir; }
	FORCEINLINE bool GetBoostProtect() const { return bBoostProtect; }
	FORCEINLINE bool GetHandShield() const { return bHandShield; }
	FORCEINLINE bool GetIsFiring() const { return bIsFiring; }
	FORCEINLINE FName GetComboMontageSectionPlaying() { return ComboMontageSectionPlaying; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE void SetEquippedWeapon(AWeapon* InWeapon) { EquippedWeapon = InWeapon; }
	FORCEINLINE AItem* GetEquippedItem() const { return EquippedItem; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE void SetHealth(float NewHealth) { Health = NewHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float NewShield) { Shield = NewShield; }
	FORCEINLINE float GetMaxHandShieldStrength() const { return MaxHandShieldStrength; }
	FORCEINLINE float GetHandShieldStrength() const { return HandShieldStrength; }
	FORCEINLINE float GetBaseMovementSpeed() const { return BaseMovementSpeed; }
	FORCEINLINE float GetCrouchMovementSpeed() const { return CrouchMovementSpeed; }
	FORCEINLINE float GetBaseFlySpeed() const { return BaseFlySpeed; }
	FORCEINLINE float GetBaseAirControl() const { return BaseAirControl; }
	FORCEINLINE float GetBaseAcceleration() const { return BaseAcceleration; }
	FORCEINLINE float GetCameraDefaultFOV() const { return CameraDefaultFOV; }
	
	FORCEINLINE FRotator GetBaseRotationRate() const { return BaseRotationRate; }

	FORCEINLINE TArray<UMaterialInterface*> GetAllMaterials() const { return AllMaterials; }
	FORCEINLINE FHudAmmoDelegate GetHudAmmoDelegate() const { return HudAmmoDelegate; }
	FORCEINLINE FHudUpdateAmmoDelegate GetHudUpdateAmmoDelegate() const { return HudUpdateAmmoOnEquip; }
	FORCEINLINE FHudCarriedAmmoDelegate GetHudCarriedAmmoDelegate() const { return HudCarriedAmmoDelegate; }
	FORCEINLINE void SetCurrentItemToSwap(AItem* InItemToSwap) { CurrentItemToSwap = InItemToSwap; }

	FORCEINLINE UWidgetComponent* GetPlayerNameWidget() const { return PlayerNameWidget; }
	FORCEINLINE UBoostComponent* GetBoost() const { return Boost; }
	FORCEINLINE USoundCue* GetHeadShotSound() const { return HeadShotSound; }
	FORCEINLINE bool GetStartFlying() const { return bStartFlying; }
	FORCEINLINE EHoverState GetHoverState() const { return HoverState; }
	FORCEINLINE bool GetGhostMode() const { return bGhostMode; }
	FORCEINLINE bool GetSlowDown() const { return bSlowDown; }
	FORCEINLINE bool GetIsStunned() const { return bIsStunned; }
	FORCEINLINE bool GetStartGrapplePull() const { return bStartGrapplePull; }
	FORCEINLINE bool GetGrappleItemEquipped() const { return bGrappleItemEquipped; }
	FORCEINLINE void SetGrappleItemEquipped(bool InBool) { bGrappleItemEquipped = InBool; }
	FORCEINLINE void SetTargetSkeletalMeshOverride(USkeletalMesh* InTargetSkeletalMeshOverride) { TargetSkeletalMeshOverride = InTargetSkeletalMeshOverride; }
	FORCEINLINE int32 GetInventoryMaxCapacity() const { return InventoryMaxCapacity; }
	FORCEINLINE UAnimMontage* GetEquipMontage() const { return EquipMontage; }
	FORCEINLINE void SetWorldTeleporting(bool bIsTeleporting) { bWorldTeleporting = bIsTeleporting; }
	FORCEINLINE bool GetIsGrappling() const { return bIsGrappling; }

	FORCEINLINE UAnimMontage* GetWinMontage() const { return WinMontage; }
	FORCEINLINE FString GetShooterMeshName() const { return ShooterMeshName; }
	FInterpLocation GetInterpLocation(int32 Index);
	// Returns the index in InterpLocations array with the lowest item count
	int32 GetInterpLocationIndex();

	void IncrementInterpLocItemCount(int32 Index, int32 Amount);
	void UnHighlightInventorySlot();
	void HighlightInventorySlot();

	UFUNCTION(Server, Reliable)
	void ServerSetCurrentItemToSwap(AItem* ClientItem);

	void SetMouseSensitivities(float FOVMult);

    // Delegate for updating the health progress bar in the HUD
    UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
    FHealthHudDelegate HealthHudDelegate;
		
	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FShieldHudDelegate ShieldHudDelegate;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FShieldStrengthDelegate ShieldStrengthDelegate;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FChargeAttackDelegate ChargeAttackDelegate;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FHoverSystemDelegate HoverSystemDelegate;

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FDashDelegate DashChargeDelegate;

	// Start the Ghost feature for the Ghost item
	void StartGhost();
	void StopGhost();

	void StartProtect(float ProtectTime = 8.f); 
	void StopProtect(); 

	virtual void StartFlying();
	void StopFlying();
	UFUNCTION()
	void UpdateFlying(float HeightValue);

	void StartSlowMo();
	void StopSlowMo();

	UFUNCTION(BlueprintCallable)
	bool IsGrappling() const;

	float BaseJumpVelocity = 530.f;

	UFUNCTION(Server, Reliable)
		void SetCombatState(ECombatState InCombatState);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_JumpToShotgunEnd();
	void JumpToShotgunEnd();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	int32 InMissileDamageBoxCount = 0;
	
	void Elim(AShooterPlayerState* AttackerPS, int32 RespawnsLeft);
	void Winner();
	virtual void WinnerOnRespawnMode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_StartFlying, Category = BoostItem)
	bool bStartFlying = false;

	bool bStopFlying = false;
	UFUNCTION()
	void OnRep_StartFlying();

	void StartMatchEffects();

	// Play sound for attacker when attacker kills an opponent
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	USoundBase* RandomCharElimmedSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HandShieldStrength, Category = Shield, meta = (AllowPrivateAccess = "true"))
	float HandShieldStrength = 100.f;

	UFUNCTION()
	void OnRep_HandShieldStrength();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ChargeAttackStrength, Category = "Charge Attack", meta = (AllowPrivateAccess = "true"))
	float ChargeAttackStrength = 0.f;

	UFUNCTION()
	void OnRep_ChargeAttackStrength();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Charge Attack", meta = (AllowPrivateAccess = "true"))
	float DashCharge = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_HoverSystemFuel, Category = Shield, meta = (AllowPrivateAccess = "true"))
	float HoverSystemFuel = 100.f;

	UFUNCTION()
	void OnRep_HoverSystemFuel();

	void SetOnPossess();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pause, meta = (AllowPrivateAccess = "true"))
	bool bGamePaused = false;

	UFUNCTION(Client, Reliable)
	void Client_DisableHover();

	// Use the Copy boost item
	void SpawnCopy();

	void EquipItem(class AItem* ItemToEquip, bool bSwap = false, bool bItemRemoved = false);

	// Pull character towards Grapple Boost Item
	void GrapplePull(ABoostItem* BoostItem);

	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* FlyingEffect;

	UPROPERTY()
	UParticleSystemComponent* SuperJumpEffectComponent = nullptr;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* SuperJumpEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* SuperJumpEffectNiagComponent;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* RushEffectSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* RushEffectNiagComponent;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* SpawnInEffect;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* SuperJumpEffectSystem;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* ProtectSystem;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* GhostSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* DashNiagComponent;

	UPROPERTY(EditAnywhere, Category = "Niagara Systems")
	UNiagaraSystem* DashSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SuperJump, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bSuperJump = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bSuperPunch = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BoostItem, meta = (AllowPrivateAccess = "true"))
	bool bGrappleThrown = false;

	UFUNCTION()
	void OnRep_SuperJump();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPossessHeli(AHelicopter* HeliActor, bool bHeliPossessed);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Helicopter, meta = (AllowPrivateAccess = "true"))
	APawn* OwnedPawn;

	int32 GetAmmoAmount(EAmmoType AmmoType);

	void UpdateAmmoForHelicopter(EAmmoType InAmmoType, int32 UpdatedAmmo);

	UFUNCTION(Server, Reliable)
	void ServerStopGrapplePull(ABoostItem* InGrappleBoost, bool bNoHit = false, bool bSwitchedItem = false);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsAI = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Boost, meta = (AllowPrivateAccess = "true"))
	bool bUsingItem = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAimingButtonPressed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bEmoteButtonPressed = false;

	UFUNCTION(Server, Reliable)
	void ServerSetMovementMode(EMovementMode InMovementMode);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetMovementMode(EMovementMode InMovementMode);

	UFUNCTION(BlueprintCallable)
	void SetAiming(bool bIsAiming);

	UFUNCTION(BlueprintCallable)
	void SetMovementState(EMovementModifiers InMovementState);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Shield, meta = (AllowPrivateAccess = "true"))
	bool bHandShieldButtonPressed = false;
	
	void ShieldOn(bool bIsHandShield);

	UFUNCTION(BlueprintCallable)
	void OnHandCombatPlayEvent(const FString& State);

	//UFUNCTION(Server, Reliable, BlueprintCallable)
	UFUNCTION(BlueprintCallable)
	virtual void BoostItemMontageFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bStartChargeAttack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bInterpChargeAttack = false;

	UFUNCTION(BlueprintCallable)
	void OnChargeAttack(float ChargeNotifyTime);

	void RunChargeAttack(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FVector TargetChargeLocation;

	UFUNCTION(Client, Reliable)
	void ClientRunChargeAttack(FVector InLocation, FVector InVelocity, float DeltaTime);

	UPROPERTY()
	FVector ClientChargeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class AActor* TrackingAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Practice, meta = (AllowPrivateAccess = "true"))
	bool bCanSpawnAI = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Practice, meta = (AllowPrivateAccess = "true"))
	bool bCanSpawnAITimer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby", meta = (AllowPrivateAccess = "true"))
	bool bCanLobbyReadyUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby", meta = (AllowPrivateAccess = "true"))
	bool bIsLobbyReady = false;

	UPROPERTY(BlueprintAssignable, Category = Delegates)
	FOnLobbyReadyDelegate OnLobbyReadyDelegate;

	void StartDestoryGrappleIfNoHit(ABoostItem* InGrapple);
	UFUNCTION()
	void DestoryGrappleIfNoHit();

	UPROPERTY()
	ABoostItem* GrappleToDestroy;
	UPROPERTY()
	FTimerHandle DestroyGrappleTimer;

	UFUNCTION(BlueprintImplementableEvent)
	void ImplementChaos(FHitResult InWeaponTrace, EWeaponType InWeaponType);

	void StartSmoothCameraToAIWinner(FVector InAILocation, FRotator InAIRotation);
	void SmoothCameraToAIWinner(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent)
	void StartVoiceChat();

	UFUNCTION(BlueprintImplementableEvent)
	void OnElimmed();

	void SetGrappleEffect(bool bIsOn);

	bool IsTeamMate();

	UFUNCTION(Server, Reliable)
	void ServerSetPlayerNameTag(const FString &InPlayerName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerNameTag, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FString PlayerNameTag;

	UFUNCTION()
	void OnRep_PlayerNameTag();

	// Drops currently equipped Weapon and Equips TraceHitItem
	void SwapItem(class AWeapon* WeaponToSwap, class ABoostItem* BoostToSwap);

	bool BoostInInventory(ABoostItem* TakenBoost);

	//UFUNCTION(Server, Reliable)
	void Server_TakeAmmo(class AAmmo* Ammo);

	void StopItemMontageEffects(UAnimInstance* AnimInstance);
	void StopReloadMontage(UAnimInstance* AnimInstance);

	// Delegate that updates ammo count in the HUD (for both weapons and boost items)
	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FHudAmmoDelegate HudAmmoDelegate;

	float SetGunDamageAmount(AShooterCharacter* DamagedCharacter, const FHitResult& WeaponTraceHit, AActor* InstigatorActor, float BaseDamage);

	void UpdateTeamMemberAttributes(float NewHealth, float NewShield);

	//UFUNCTION()
	//void OnRep_UsingItem();

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_Shield(float OldShield);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 InventorySelect = 0;

	UPROPERTY()
	AController* LastController;

private:
	// Item RPCs
	UPROPERTY()
	FVector_NetQuantize HitTarget;

	FHitResult GetHitscanHits(const FVector_NetQuantize& TraceHitTarget);

	FHitResult GetHitscanHitsBoost(const FVector_NetQuantize& TraceHitTarget);

	void OnMissedShot_Implementation(AActor* InstigatorShooter, const FVector& ImpactPoint) override;

	void OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound) override;
	
	void OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound) override;

	void OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass()) override;

	bool HasShield_Implementation() override;
	
	void OnHandCombatHit_Implementation(AShooterCharacter* InstigatorShooter, float Damage, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult) override;

	void OnAmmoSphereOverlap_Implementation(AAmmo* OverlappedAmmo) override;

	void OnItemPickedUp_Implementation(AActor* ItemActor, const FString& ItemType) override;

	void OnUpdateHUDBoostAmount_Implementation(int32 BoostAmount, ABoostItem* BoostItem) override;

	void OnFinishedReloading_Implementation() override;

	void OnShotgunShellReload_Implementation() override;

	void OnSlowDown_Implementation(bool bIsSlow, AShooterCharacter* InstigatorShooter) override;

	void OnGravityProjectileHit_Implementation(bool bIsPulling, AActor* ProjectileGravity) override;

	void OnStunned_Implementation(AActor* StunnerActor) override;

	void OnMatchEnded_Implementation(AShooterPlayerController* ShooterController) override;

	FVector GetPawnLocation_Implementation() override;

	float GetPawnRotationYaw_Implementation() override;

	void OnEmoteFinished_Implementation() override;

	UFUNCTION()
		void OnRep_Inventory(TArray<AItem*> OldInventory);

	UFUNCTION()
		void OnRep_GhostMode();

	UFUNCTION()
		void OnRep_HandCombatState(EHandCombatState OldHandState);

	UFUNCTION()
		void OnRep_Crouching(bool OldCrouching);

	UFUNCTION()
	void SetShooterMeshName();

	void SpawnNFT(const FString &NFT_ID);

	void StartPawnPossessTimer();

	UPROPERTY()
	FTimerHandle PawnPossessTimer;

	UFUNCTION()
	void OnPawnPossesFinish();

	UFUNCTION()
	bool LoadWeaponDataFromJSON(const FString& JSONString, FWeaponDataStruct& OutWeaponData);

	UPROPERTY(EditAnywhere, Category = Combat);
	TSubclassOf<class AWeaponNFT> WeaponNFTClass;

	void HandAttack();
	void Rush();

	void UpdateCharacter();

	void OnFPSAiming(bool bStartFPS);
	void GetMaterialsToHideForFPS();
	void SetMaterialsOnFPS(bool bStartFPS);
	UPROPERTY(EditAnywhere)
	UMaterialInterface* TransparentMaterial;
	TMap<int8, UMaterialInterface*> FPSMaterialsToHideMap;

	void InitializeLobbySpawnEffect();

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = "Spawn In")
	UMaterialInstanceDynamic* DynamicTeleportMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = "Spawn In")
	UMaterialInstance* TeleportMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Combat)
	UMaterialInstance* StunMaterialInstance;

	
	UFUNCTION()
	void UpdateTeleportInMaterial(FVector CurveValues);
	void StartTeleportIn();
	UFUNCTION()
	void StartSpawnInEffects();

	UPROPERTY(VisibleAnywhere, Category = "Spawn In")
	UTimelineComponent* SpawnInTimeline;
	UPROPERTY()
	FOnTimelineVector SpawnInTrack;

	UPROPERTY(EditAnywhere, Category = "Spawn In")
	UCurveVector* SpawnInVectorCurve;

	void SetLobbyReady(bool bIsReady);

	void UpdateItemState();

	float AimingArmLength = 0.f;
	FVector AimingSocketOffset = FVector(0.f, 24.5f, 72.f); // aligned with sniper scope
	float DefaultArmLength = 250.f;
	FVector DefaultSocketOffset = FVector(0.f, 45.0f, 70.0f);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_IsStunned, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bIsStunned = false;
	UFUNCTION()
	void OnRep_IsStunned();

    UFUNCTION()
    void OnShooterHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	void PlayLandingMontage();

	UFUNCTION(Server, Reliable)
	void ServerLand();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastLand();

	UFUNCTION(BlueprintCallable)
	void OnLandEnded();
	void SetLandPitch(float DeltaTime);
	bool bLanded = false;

	void StunPawnsAfterLand();

protected:
	UPROPERTY(EditAnywhere, Category = BoostItem)
	UParticleSystem* SpawnCopyEffect;
	UPROPERTY(EditAnywhere, Category = BoostItem)
	USoundBase* SpawnCopySound;

	UFUNCTION(BlueprintImplementableEvent)
	void OnJump();

public:

	class AShooterPlayerController* GetShooter_PC();

	UFUNCTION(BlueprintImplementableEvent)
		void ShowSniperScopeWidget(bool bShowScope);

	UFUNCTION(BlueprintImplementableEvent)
		void ShowHitNumber(float DamageAmount, FVector HitLocation, bool bHeadshot, bool bShield = false);

	void PlayItemMontage(FName ItemMontageSectionName, bool bAutoBlend = true);
	void PlaySuperPunchMontage(FName ItemMontageSectionName, bool bAutoBlend = true);

		void ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved = false);
	UFUNCTION(Server, Reliable)
		void Server_ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex, bool bItemRemoved = false);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Inventory, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
    	TArray<AItem*> Inventory;

	// Movement replication
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Set_TargetMovementSpeed(const float InTargetMovementSpeed);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_TargetMovementSpeed)
	float TargetMovementSpeed;

	UFUNCTION()
	void OnRep_TargetMovementSpeed(float OldTargetMovementSpeed);

	UFUNCTION(Server, Reliable)
	void SetCrouching(const bool InCrouching);

	void TriggerCrouch();

	UFUNCTION(BlueprintCallable)
	bool IsPlayerEliminated() const { return bPlayerEliminated; };

	void CheckEmotePlaying();

	void StopMatch();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInMissileDamageBox = false;

	void SetDefaultMaterials();
	void SetHairMaterialForLeo(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst);
	void GhostMaterialEffect();
	void ProtectMaterialEffect();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSpawnNFT(const FString &ResponseJSONData);

	UFUNCTION(Server, Reliable)
	void Server_OnNonZeroSequence(AWeapon* OwnerWeapon, int32 ClientSequence);

	void DropAllItems();

	UFUNCTION(Client, Unreliable)
	void ClientOnProjectileHit(const FVector& HitLocation, float DamageToApply, bool bHeadShot, bool bHasShield = false);

};