// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "ShooterPlayerState.h"
#include "ShooterAI.h"
//#include "Kismet/GameplayStatics.h"

AShooterAIController::AShooterAIController()
{
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    check(BlackboardComponent);

    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    check(BehaviorTreeComponent);

    bWantsPlayerState = true;
}

void AShooterAIController::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    //AShooterPlayerState* ShooterPS = Cast<AShooterPlayerState>(PlayerState);
    //if (ShooterPS) ShooterPS->bIsAI = true; 

}

/*
void AShooterAIController::PreInit(AShooterAI* ShooterAI, FTransform SpawnTransform)
{
    auto ShooterAIPS = Cast<AShooterPlayerState>(PlayerState);
    UE_LOG(LogTemp, Warning, TEXT("AI PS Good"));
    if (ShooterAIPS)
    {
        
        ShooterAIPS->SetAIProperties();
    }
    UGameplayStatics::FinishSpawningActor(this, SpawnTransform);
    Possess(ShooterAI);
}
*/

//AShooterAIController::AShooterAIController(const FObjectInitializer& ObjectInitializer)
//     :Super(ObjectInitializer)
 //{
    
 //}

void AShooterAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    //UE_LOG(LogTemp, Warning, TEXT("OnPossess AI"));
    if (InPawn == nullptr) return;

    /*
    auto ShooterAI = Cast<AShooterAI>(InPawn);
    if (ShooterAI)
    {
        if (ShooterAI->GetBehaviorTree())
        {
            BlackboardComponent->InitializeBlackboard(*(ShooterAI->GetBehaviorTree()->BlackboardAsset));
        }
    }
    */
}

USceneComponent* AShooterAIController::OnGameEndForWinner()
{
	if (IEnemyAIInterface* EnemyInterface = Cast<IEnemyAIInterface>(GetPawn()))
	{
		return EnemyInterface->Execute_OnGameEndWinner(GetPawn());
	}
    return nullptr;
}