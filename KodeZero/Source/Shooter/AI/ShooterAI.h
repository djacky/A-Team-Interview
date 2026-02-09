// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shooter/ShooterCharacter.h"
#include "ShooterAI.generated.h"

/**
 * 
 */

class USoundCue;
class UBehaviorTree;
class AShooterAIController;
class USphereComponent;

UCLASS()
class SHOOTER_API AShooterAI : public AShooterCharacter
{
	GENERATED_BODY()

public:
	AShooterAI(const FObjectInitializer& ObjectInitializer);
	FORCEINLINE void SetElimMontageAI(UAnimMontage* InMontage) { ElimMontageAI = InMontage; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
	FORCEINLINE AActor* GetTargetCharacter() const { return TargetCharacter; }
	
	void SetNewTargetCharacter(AActor* CurrentTarget);
	UFUNCTION(BlueprintCallable)
	void SetTargetCharacter(AActor* InCharacter);

	void SetInitialItems();

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable)
	void MulticastPlayPatrolWaitSound(const FString& AIStage);
	void PlayPatrolSound(const FString& AIStage);

	UPROPERTY()
	FTimerHandle SearchTimer;
	UPROPERTY()
	FTimerHandle ItemTimer;
	UPROPERTY()
	FTimerHandle HoverTimer;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimAI();

	UPROPERTY()
	FString AIName;

	virtual void WinnerOnRespawnMode() override;

	void StopSearchTimer();

	virtual void InitialEquipWeapon(AItem* ItemToEquip) override;

	virtual void ReplenishShieldStrength() override;

	virtual void ReplenishChargeAttackStrength() override;

	virtual void ReplenishHoverSystem() override;

	virtual void ReplenishDashCharge(float DeltaTime) override;

	void ActivateCamera(bool bActivate);

	UPROPERTY()
	class AWeapon* CameraInterpWeapon = nullptr;

	FORCEINLINE USceneComponent* GetSceneCameraPositionForInterp() const { return SceneCameraPositionForInterp; }

	UPROPERTY(Replicated)
		float ReplBaseAimRotationYaw;

	UPROPERTY(Replicated)
		float ReplBaseAimRotationPitch;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual bool CanTarget_Implementation() override;
	
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser) override;
	
	void PossessedBy(AController* NewController);

	void PlayElimMontageAI();

	UFUNCTION(BlueprintImplementableEvent)
	void TriggerAnim();

	virtual void CameraOffsetWithDistance() override;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	//TArray<USkeletalMesh*> AISkinsArray;

	virtual void InitializeComboVariables(const FString& MeshName) override;

	virtual void SetInitialBoost() override;

	UFUNCTION()
	void OnPlayerOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	//UFUNCTION()
	//void OnPlayerEndOverlap(UPrimitiveComponent* OverlappedComponent,
	//AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnCheckTargetEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "AI", meta = (AllowPrivateAccess = "true"))
	AActor* TargetCharacter;

	virtual bool TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation) override;

	UFUNCTION(Client, Reliable)
	void ClientSetTarget(AShooterCharacter* ShooterCauser);

	UFUNCTION(Client, Reliable)
	void ClientSetBehaviorTree(AController* NewController);

	//UFUNCTION(BlueprintCallable)
	//void UseRandomBoost();

	UFUNCTION(BlueprintCallable)
	FVector GetPatrolPoint();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetAIBoost(ABoostItem* InitialBoost, int32 InSlotIndex, int32 BoostAmount);

	virtual void FireButtonPressed() override;

	virtual void BoostItemMontageFinished() override;
	
	UFUNCTION()
	void UseItem();

	void StartItemTimer();

	void StartGetRandomChaseLocationTimer();
	UFUNCTION()
	void GetRandomChaseLocationTimer();

	UFUNCTION()
	void FinishItemTimer();

	virtual void OnRep_EquippedItem(AItem* OldEquippedItem) override;

	virtual void StartFlying() override;

	virtual void SmoothRise(float DeltaTime) override;

	void ScreenMessage(FString Mess);

	void SwitchToBoost();

	TArray<AActor*> GetShooterPlayersInSphere(float SphereRadius);

	void StartSearchTimer();
	UFUNCTION()
	void FinishSearchTimer();

	UFUNCTION()
	void CheckUsingItem();

	bool CanTarget(AActor* InCharacter);

	bool IsPointReachable(FVector TargetLocation, class UNavigationSystemV1* InNavSystem);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStartAI();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWinnerMontage(const FName& RandomMontageName);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Scene, meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneCameraPositionForInterp;

	UFUNCTION()
	void HandleAIDestroy();

	UFUNCTION(BlueprintCallable)
	void StartRandomHover();

	UFUNCTION()
	void StartHovering();

	UFUNCTION(BlueprintCallable)
	void StopRandomHover();

	void SetTarget_Implementation(AActor* InCharacter, bool bIsNew) override;
	void StopSearch_Implementation() override;
	void OnElimmed_Implementation() override;
	void PlayTauntSound_Implementation() override;
	USceneComponent* OnGameEndWinner_Implementation() override;
	

private:

	// Starting random shooting deviation
	UPROPERTY(EditAnywhere, Category = "AI")
	float r1 = 60.f;

	// Ending random shooting deviation
	UPROPERTY(EditAnywhere, Category = "AI")
	float r2 = 140.f;

	// Ending random shooting distance
	UPROPERTY(EditAnywhere, Category = "AI")
	float d2 = 6500.f;

	UPROPERTY(EditAnywhere, Category = "AI")
	USoundCue* AIStartSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> PatrolPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float PatrolSpeedFactor = 0.6f;

	UPROPERTY(EditAnywhere, Category = "AI")
	USoundCue* ElimSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ElimMontageAI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BT", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;

	// Point for the AI to move to
	UPROPERTY(EditAnywhere, Category = "BT", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector PatrolPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	AShooterAIController* ShooterAIController;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	//USphereComponent* AgroSphere;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	//USphereComponent* CheckTargetSphere;

	// Sound played when missle is spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class USoundCue* PatrolWaitSound;

	// Sound played when missle is spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	USoundCue* NewEnemySound;

	// Sound played when missle is spawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	USoundCue* KilledPlayerSound;

	bool bSetInitialItems = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	bool bCanFireAgain = true;
	bool bUsingItemAI = false;

	UPROPERTY()
	FTimerHandle CheckUsingItemTimer;
	UPROPERTY()
	FTimerHandle ChaseLocationTimer;
	int32 RandomBoostIndexSelected;

	float TargetExitDistance = 5500.f;
	float PreviousTargetDistance;

	UPROPERTY()
	class UNavigationSystemV1* NavSystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	FVector RandomTargetLocation;

	UPROPERTY()
	TArray<FVector> BackupPatrolPoints = {
					{-2150.0f,-14720.0f,120.0f}, {-2220.0f,2410.0f,120.0f},
					{-3460.0f,33120.0f,120.0f},{24580.0f,32030.0f,120.0f},
					{24550.0f,13210.0f,120.0f},{31110.0f,-1260.0f,120.0f},
					{66320.0f,13690.0f,120.0f},{39700.0f,35950.0f,120.0f},
					{-40790.0f,-19190.0f,120.0f},{-23450.0f,33040.0f,120.0f},
					{-52560.0f,38400.0f,120.0f}, {-75440.0f,11370.0f,120.0f},
					{-70660.0f,-20380.0f,120.0f},{-52440.0f,-20900.0f,120.0f},
					{59050.0f,-34440.0f,120.0f}, {58330.0f,-2370.0f,120.0f},
					{-45280.0f,-38490.0f,120.0f}, {-2520.0f,14500.0f,120.0f},
					{-22970.0f,14500.0f,120.0f},{24240.0f,-10970.0f,200.0f},
					{62310.0f,33340.0f,120.0f},{-49240.0f,-4720.0f,120.0f},
					{60.0f,23040.0f,120.0f}, {-59960.0f,22650.0f,1150.0f},
					{-58100.0f,5670.0f,370.0f}, {39770.0f,-2330.0f,100.0f},
					{24250.0f,-14660.0f,470.0f}, {42360.0f,-30180.0f,350.0f},
					{40740.0f,23330.0f,1530.0f}, {52890.0f,21980.0f,670.0f}
					};
	
};
