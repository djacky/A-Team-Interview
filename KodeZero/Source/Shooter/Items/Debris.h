// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debris.generated.h"

UCLASS()
class SHOOTER_API ADebris : public AActor
{
	GENERATED_BODY()
	
public:	
	ADebris();

	void SetAssetProps();

protected:
	virtual void BeginPlay() override;

	void SetPhysics();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TArray<UStaticMesh*> DebrisMeshArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Mesh", meta = (AllowPrivateAccess = "true"))	
	UStaticMeshComponent* MainMesh;

};
