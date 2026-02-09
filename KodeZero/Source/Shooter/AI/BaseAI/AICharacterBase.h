// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Shooter/Misc/Interfaces/EnemyAIInterface.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "Shooter/StructTypes/EnemyTypeStruct.h"
#include "Shooter/StructTypes/EnemyDataStruct.h"
#include "Components/TimelineComponent.h"
#include "AICharacterBase.generated.h"

class UBehaviorTree;
class AProjectile;
class AShooterGameState;
class AShooterCharacter;
class UNavigationSystemV1;


UCLASS()
class SHOOTER_API AAICharacterBase : public ACharacter, public IEnemyAIInterface, public IWeaponHitInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAICharacterBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void PossessedBy(AController* NewController);

	void SetTargetCharacter(AActor* InCharacter);
	bool CanTarget(AActor* InCharacter);
	void SetNewTargetCharacter(AActor* CurrentTarget);

	AShooterGameState* GetShooter_GS();
	UPROPERTY()
	FString AIName;

	bool IsPointReachable(FVector TargetLocation, UNavigationSystemV1* InNavSystem);
	void StartGetRandomChaseLocationTimer();
	UFUNCTION()
	void GetRandomChaseLocationTimer();

	void StartSearchTimer();
	UFUNCTION()
	void FinishSearchTimer();

	TArray<AActor*> GetShooterPlayersInSphere(float SphereRadius);

	UFUNCTION(BlueprintCallable)
	FVector GetPatrolPoint();

	void StopSearchTimer();

	UFUNCTION()
	void SetCorrectTransform();

	void OnHitScanWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult, USoundCue* HitSound) override;
	void OnShotgunWeaponHit_Implementation(AShooterCharacter* InstigatorShooter, AShooterPlayerState* InstigatorPS, const TArray<FHitResult>& InHitResults, USoundCue* HitSound) override;
	void OnProjectileWeaponHit_Implementation(AController* InstigatorController, float Damage, AActor* DamageCauser, FHitResult InHitResult, TSubclassOf<UDamageType> DamageType = UDamageType::StaticClass()) override;
	void OnHandCombatHit_Implementation(AShooterCharacter* InstigatorShooter, float Damage, AShooterPlayerState* InstigatorPS, const FHitResult& InHitResult) override;
	void OnMissedShot_Implementation(AActor* InstigatorShooter, const FVector& ImpactPoint) override;
	void PlayTauntSound_Implementation() override;
	void OnElimmed_Implementation() override;
	void OnGravityProjectileHit_Implementation(bool bIsPulling, AActor* ProjectileGravity) override;
	USceneComponent* OnGameEndWinner_Implementation() override;

	UFUNCTION(BlueprintCallable)
	virtual void OnAIStopAttacking();

	void HitCharacter(AActor* DamagedCharacter, float HitDamage, AShooterPlayerState* InstigatorPS);
	UFUNCTION()
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	virtual bool CanTarget_Implementation() override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimAI();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttack(const FEnemyAttackData& ServerAttackData);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWinnerMontage(UAnimMontage* MontageToPlay, const FName& Section);

	UFUNCTION(BlueprintCallable)
	virtual void Fire();

	UFUNCTION(BlueprintCallable)
	void Attack();
	UFUNCTION(BlueprintCallable)
	void PerformJumpAttack();
	void PlayAttackMontage(UAnimMontage* MontageToPlay);
	void PlayDeathMontage(UAnimMontage* MontageToPlay);
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnStunMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void StartDissolveEffect();
	UPROPERTY(EditAnywhere, Category = "AI")
	USoundCue* ElimSound;

	FName GetNextFireSocket();

	UFUNCTION()
	void HandleAIDestroy();

	UPROPERTY()
	UAnimMontage* CurrentAttackMontage;

	UPROPERTY()
	FTimerHandle AttackHandle;

	UPROPERTY()
	FTimerHandle TeleportHandle;

	FVector PerformAdjustedLineTrace(const FVector& Start, const FVector& Target) const;

	void OnWinnerLogic();

	UFUNCTION(BlueprintCallable)
    void SetMovement(bool bIsStopMovement);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BT", meta = (AllowPrivateAccess = "true"))
	bool bCanAttack = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "AI", meta = (AllowPrivateAccess = "true"))
	AActor* TargetCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FEnemyAttackData CurrentAttack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
    float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

	void OnStunned_Implementation(AActor* StunnerActor) override;

	UFUNCTION()
	void OnStunEnded();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
	FORCEINLINE const FEnemyData& GetEnemyData() const { return EnemyData; }

	virtual void UpdateEnemy();

	void SetTarget_Implementation(AActor* InCharacter, bool bIsNew) override;
	void StopSearch_Implementation() override;

    // Server-side: Complete teleport
    void CompleteTeleport();

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Scene, meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneCameraPositionForInterp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BT", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ATeleportTrail> TeleportTrailActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	class AShooterAIController* ShooterAIController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BT", meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;

	UPROPERTY(ReplicatedUsing = OnRep_EnemyData)
	FEnemyReplicationData EnemyReplicationData;

	UPROPERTY()
	FEnemyData EnemyData;

	UFUNCTION()
	void OnRep_EnemyData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_TargetMovementSpeed, meta = (AllowPrivateAccess = "true"))
	float TargetMovementSpeed;

	UFUNCTION()
	void OnRep_TargetMovementSpeed(float OldTargetMovementSpeed);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_IsStunned, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bIsStunned = false;
	UFUNCTION()
	void OnRep_IsStunned();

	UPROPERTY(EditAnywhere, Category = Combat)
	UMaterialInstance* StunMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HandCombat, meta = (AllowPrivateAccess = "true"))
	USoundCue* StunnedSound;

	UPROPERTY()
	float TargetExitDistance = 5500.f;
	UPROPERTY()
	float PreviousTargetDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseMovementSpeed = 650.f;

	UPROPERTY()
	float BaseJumpVelocity = 530.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	float PatrolSpeedFactor = 0.6f;

	UPROPERTY()
	FTimerHandle ChaseLocationTimer;
	UPROPERTY()
	FTimerHandle SearchTimer;

	UPROPERTY()
	UNavigationSystemV1* NavSystem;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	FVector RandomTargetLocation;

	UPROPERTY()
	int32 CurrentFireNotifyIndex = 0;

	FTimerHandle TeleportTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    bool bIsTeleporting = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bCanTeleport = true;
    FVector TeleportStartLocation;
    FVector TeleportTargetLocation;
    float TeleportProgress = 0.f;

    // Server-side: Start teleport sequence
	UFUNCTION(BlueprintCallable)
    void ServerStartTeleport();
	UFUNCTION()
	void SetEnableTeleport();

    // Multicast: Set AI visibility
    UFUNCTION(NetMulticast, Reliable)
    void MulticastSetVisibility(bool bIsVisible);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartTeleport(UAnimMontage* MontageToPlay);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopCurrentMontage(UAnimMontage* CurrentMontage);

	UFUNCTION(BlueprintCallable)
	void SetAIVisibility(bool bIsVisible);

    // Get random teleport location around target
    FVector GetRandomTeleportLocation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	bool bStopMovement = false;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	virtual void StartDissolve();
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UTimelineComponent* DissolveTimeline;
	UPROPERTY()
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve;
	UPROPERTY(EditAnywhere, Category = Spawn)
	UCurveFloat* SpawnInCurve;

	void GetAllSkeletalMeshMaterials();
	void SetAllSkeletalMeshMaterials(bool bSetDefaultMat, UMaterialInstanceDynamic* DynamicInst);
	TMap<USkeletalMeshComponent*, TArray<UMaterialInterface*>> OriginalMaterialsMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* TransparentMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_HasFinishedDissolve)
	bool bHasFinishedDissolve = false;

	UFUNCTION()
	void OnRep_HasFinishedDissolve();

	UPROPERTY(ReplicatedUsing = OnRep_HasSpawnedIn)
	bool bSpawnedIn = false;

	UFUNCTION()
	void OnRep_HasSpawnedIn();

	UFUNCTION()
	void OnSpawnInFinished();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StartSpawnIn();

	void StartSpawnIn();
	UFUNCTION()
	void UpdateStartSpawnIn(float DissolveValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Spawn, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* SpawnInEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SlowDownFactor, Category = Boost, meta = (AllowPrivateAccess = "true"))
	bool bSlowDownTrigger = false;

	UFUNCTION()
	void OnRep_SlowDownFactor();

	void OnSlowDown_Implementation(bool bIsSlow, AShooterCharacter* InstigatorShooter) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bGravityPull = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AActor* GravityProjectileActor;

	void PullToGravityProjectile();

	UFUNCTION(BlueprintCallable)
	void StopCurrentMontage();

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
