// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/TempInitialCamera.h"
#include "Camera/CameraComponent.h"

// Sets default values
ATempInitialCamera::ATempInitialCamera()
{
	PrimaryActorTick.bCanEverTick = false;

    CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    RootComponent = CameraComp;

}
