// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Shooter/Misc/Interfaces/EnemyAIInterface.h"
#include "ShooterAIController.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AShooterAIController : public AAIController, public IEnemyAIInterface
{
	GENERATED_BODY()

public:
	AShooterAIController();
	virtual void OnPossess(APawn* InPawn) override;
	//AShooterAIController(const FObjectInitializer& ObjectInitializer);

	USceneComponent* OnGameEndForWinner();

	//void PreInit(class AShooterAI* ShooterAI, FTransform SpawnTransform);

private:
	UPROPERTY(BlueprintReadWrite, Category = "AI Behavior", meta = (AllowPrivateAccess = "true"))
	class UBlackboardComponent* BlackboardComponent;

	UPROPERTY(BlueprintReadWrite, Category = "AI Behavior", meta = (AllowPrivateAccess = "true"))
	class UBehaviorTreeComponent* BehaviorTreeComponent;

public:
	FORCEINLINE UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComponent; }

protected:

	virtual void PostInitializeComponents() override;
	
};
