// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportActor.generated.h"

UCLASS()
class SHOOTER_API ATeleportActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATeleportActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTeleportBoxOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* TeleportBox;

	void LocalStartTeleport(AShooterCharacter* InCharacter);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastTeleport(class AShooterCharacter* InCharacter);

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TeleportEffect;

	UPROPERTY(EditAnywhere)
	class USoundCue* TeleportSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* MainParticle;

	UFUNCTION()
	void WorldTeleport(AShooterCharacter* InCharacter, const FVector_NetQuantize &TeleportLoc);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport", meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* TeleportMaterial;

	UFUNCTION(BlueprintImplementableEvent)
	void StartTeleport(AShooterCharacter* ShooterCharacter);

	UFUNCTION(BlueprintCallable)
	void SetMaterials(AShooterCharacter* InCharacter, bool bIsTeleporting);

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	UMaterialInstanceDynamic* DynamicTeleportMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = "Teleport")
	UMaterialInstance* TeleportMaterialInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport")
	TArray<UMaterialInterface*> AllItemMaterials;

	UPROPERTY()
	bool bCanEnter = true;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	// Point for the character to teleport to
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = "true", MakeEditWidget = "true"))
	FVector TeleportLocation;

};
