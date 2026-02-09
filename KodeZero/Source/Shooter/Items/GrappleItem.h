// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleItem.generated.h"

#define ECC_Helicopter ECollisionChannel::ECC_GameTraceChannel6

UENUM(BlueprintType)
enum class EHookState : uint8
{
    EGS_None UMETA(DisplayName = "None"),
    EGS_Thrown UMETA(DisplayName = "Thrown"),
    EGS_Hooked UMETA(DisplayName = "Hooked"),
    EGS_Pulling UMETA(DisplayName = "Pulling"), // Not strictly needed but for clarity
    EGS_Cancelled UMETA(DisplayName = "Cancelled")
};

UCLASS()
class SHOOTER_API AGrappleItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrappleItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* ItemMesh;

	bool bIsPredicted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Niagara, meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* TrailComponent;

	void SpawnTrailSystem(const FVector& InScale);

	FTimerHandle TimeoutHandle;
	UFUNCTION()
	void OnTimeout();

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	UPROPERTY(ReplicatedUsing = OnRep_bTrailDeactivated)
	bool bTrailDeactivated;

	UFUNCTION()
	void OnRep_bTrailDeactivated();

	void DeactivateTrail();

	FTimerHandle DestroyHandle;
	float FadeTime = 2.5f;

	UFUNCTION()
	void DelayedDestroy();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetIsPredicted(bool bPredicted);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UProjectileMovementComponent* ProjectileMovementComponent;

	void SetVelocity(const FVector& Direction);

	
};
