// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportTrail.generated.h"

class AAICharacterBase;
class UNiagaraComponent;
class UAudioComponent;

UCLASS()
class SHOOTER_API ATeleportTrail : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATeleportTrail();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Trail, meta = (AllowPrivateAccess = "true"))
	USceneComponent* MainSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Trail, meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* NiagaraComp;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitTrail(FVector Start, FVector End, float Speed, AAICharacterBase* OwningAI);

private:
	UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayStopTeleportEffects(FVector Location, AAICharacterBase* OwningAI);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayTrail(bool bPlay, AAICharacterBase* OwningAI);

	void OnTrailReachedDestination();

	void DelayedDestroy();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Trail, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* TrailSoundComponent;

	UPROPERTY()
	AAICharacterBase* OwningCharacter = nullptr;

	UPROPERTY()
    FVector StartLocation;
	UPROPERTY()
    FVector EndLocation;
	UPROPERTY()
    float TrailSpeed;
	UPROPERTY()
	bool bMoving = false;
	
};
