// Fill out your copyright notice in the Description page of Project Settings.


#include "AIAnimInstanceBase.h"
#include "AICharacterBase.h"



void UAIAnimInstanceBase::UpdateAnimationProperties(float DeltaTime)
{
    if (EnemyAI == nullptr)
    {
        EnemyAI = Cast<AAICharacterBase>(TryGetPawnOwner());
    }
    if (EnemyAI == nullptr) return;

}
