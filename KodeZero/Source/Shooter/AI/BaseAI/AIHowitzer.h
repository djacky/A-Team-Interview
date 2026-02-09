// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/BaseAI/AICharacterBase.h"
#include "AIHowitzer.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AAIHowitzer : public AAICharacterBase
{
	GENERATED_BODY()
	

public:
	AAIHowitzer();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	void PossessedBy(AController* NewController);

protected:
    UFUNCTION(BlueprintCallable)
    void StartMissileAttack(); // Triggered from Anim Notify (start of montage)

	UFUNCTION(BlueprintCallable)
	void StartFlyingToTarget();

    UFUNCTION()
    void OnReachedAltitude();

    void StartRise();
    void StopRise();

    void CheckAltitude(float DeltaTime);

	UFUNCTION(BlueprintCallable)
    void FinishAttack();

	virtual void Fire() override;
	virtual void UpdateEnemy() override;
	virtual void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser) override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnAIStopAttacking() override;

private:

	UPROPERTY(VisibleAnywhere, Category = "Exhaust")
	class UParticleSystemComponent* LeftExhaust;

	UPROPERTY(VisibleAnywhere, Category = "Exhaust")
	UParticleSystemComponent* RightExhaust;

    UPROPERTY(EditAnywhere, Category = "Missile Attack")
    float TargetAltitude = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Missile Attack")
    float RiseSpeed = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Missile Attack")
    float MaxRiseDuration = 3.0f;

    bool bIsRising = false;
    bool bHasFired = false;

    float StartZ = 0.0f;

    FTimerHandle TimeoutFireHandle;

	UFUNCTION()
	void AttachExhaustComponents();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_IsFlyingToTarget, Category = "Attack")
	bool bIsFlyingToTarget = false;

	UFUNCTION()
	void OnRep_IsFlyingToTarget();

	UPROPERTY(EditAnywhere, Category = "Attack")
	float FlyToTargetSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Attack")
	float FlyRandomnessMagnitude = 1000.f;

	float FlightTime = 0.f;

	float DesiredFlightAltitude = 600.f;
	
};
