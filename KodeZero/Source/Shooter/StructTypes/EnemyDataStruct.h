#pragma once

#include "CoreMinimal.h"
//#include "../Items/Weapons/ProjectileBullet.h"
//#include "BehaviorTree/BehaviorTree.h"
#include "EnemyDataStruct.generated.h"


class USoundCue;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FMeshData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    USkeletalMesh* SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FSlateBrush IconImage = FSlateBrush();

};

USTRUCT(BlueprintType)
struct FTeleportData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCanTeleport = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UAnimMontage* TeleportMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    USoundCue* StopTeleportSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    USoundCue* TeleportingSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UNiagaraSystem* StopTeleportEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UNiagaraSystem* TeleportingEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TeleportSpeed = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TeleportLocationRandomization = FVector::ZeroVector;

};

USTRUCT(BlueprintType)
struct FExplosionData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsExplosion = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ExplosionDamage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ExplosionInnerRadius = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ExplosionOuterRadius = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ExplosionDamageFalloff = 1.f;

};

USTRUCT(BlueprintType)
struct FHomingData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsHoming = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float HomingAcceleration = 1000.f;

};

USTRUCT(BlueprintType)
struct FLaunchedData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsTargetLaunched = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector LaunchDirection = FVector::ZeroVector;

};

USTRUCT(BlueprintType)
struct FSoundData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    USoundCue* ProjectileSoundLoop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundAttenuation* LoopingSoundAttenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* FireSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USoundCue* ImpactSound = nullptr;

};

USTRUCT(BlueprintType)
struct FParticleData
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleFlash = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UNiagaraSystem* ImpactParticles = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* ImpactParticlesLegacy = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UNiagaraSystem* TrailParticles = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UParticleSystem* Tracer = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector TrailEffectScale = FVector(1.f, 1.f, 1.f);

};

USTRUCT(BlueprintType)
struct FEnemyAttackData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UAnimMontage* AttackMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<class AProjectileBullet> ProjectileClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UStaticMesh* ProjectileMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FSoundData SoundData = FSoundData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FParticleData ParticleData = FParticleData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FHomingData HomingData = FHomingData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FExplosionData ExplosionData = FExplosionData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLaunchedData LaunchData = FLaunchedData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float ProjectileSpeed = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Damage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bGravityEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsArcing = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bIsFiredDirectlyToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> FireSockets = TArray<FName>();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* TargetActor = nullptr;

};

USTRUCT(BlueprintType)
struct FEnemyData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBehaviorTree* BehaviorTree = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSubclassOf<UAnimInstance> AnimationBlueprint = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UAnimMontage* WinnerMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    UAnimMontage* DeathMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Health = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float MaxHealth = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float BaseMovementSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FEnemyAttackData> AttackVariants = TArray<FEnemyAttackData>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMeshData> MeshVariants = TArray<FMeshData>();

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FTeleportData TeleportData = FTeleportData();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USoundCue* TauntSound = nullptr;
};