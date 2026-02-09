// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ScopeAttachment.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneCaptureComponent2D.h"

AScopeAttachment::AScopeAttachment()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    ScopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScopeMesh"));
    RootComponent = ScopeMesh;

    ScopeCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ScopeCapture"));
    ScopeCapture->SetupAttachment(ScopeMesh, LensSocketName);
    ScopeCapture->bCaptureEveryFrame = false;
    ScopeCapture->bCaptureOnMovement = false;
    ScopeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    ScopeCapture->FOVAngle = MagnifiedFOV;
}

void AScopeAttachment::BeginPlay()
{
    Super::BeginPlay();

    if (RenderTarget) ScopeCapture->TextureTarget = RenderTarget;
    SetupDynamicLensMaterial();
    
}

void AScopeAttachment::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsActive && CachedOwningPawn && CachedOwningPawn->IsLocallyControlled())
    {
        if (CachedPlayerController)
        {
            ScopeCapture->SetWorldRotation(CachedPlayerController->GetControlRotation());
        }

        if (bNeedsCaptureUpdate)
        {
            ScopeCapture->CaptureScene();
            bNeedsCaptureUpdate = false;
        }
    }
}

void AScopeAttachment::ActivateScope(bool bActivate)
{
    if (bActivate == bIsActive) return;
    bIsActive = bActivate;
    bNeedsCaptureUpdate = true;

    if (bActivate)
    {
        ScopeCapture->CaptureScene();
		bNeedsCaptureUpdate = false;
    }
}

void AScopeAttachment::SetScopeOwner(APawn* OwnerPawn)
{
    UE_LOG(LogTemp, Warning, TEXT("OnRep_Owner 1"));
    if (OwnerPawn)
    {
        CachedOwningPawn = OwnerPawn;
        UE_LOG(LogTemp, Warning, TEXT("OnRep_Owner 2"));
        CachedPlayerController = Cast<APlayerController>(CachedOwningPawn->GetController());
        // Re-run RTT setup if now locally controlled
        SetupDynamicLensMaterial();
    }
}

void AScopeAttachment::SetupDynamicLensMaterial()
{
    if (LensBaseMaterial && CachedOwningPawn && CachedOwningPawn->IsLocallyControlled())
    {
        LensDynamicMaterial = UMaterialInstanceDynamic::Create(LensBaseMaterial, this);
        if (LensDynamicMaterial)
        {
            LensDynamicMaterial->SetTextureParameterValue(TEXT("RTTTexture"), (UTexture*)RenderTarget);
            ScopeMesh->SetMaterial(LensMaterialIndex, LensDynamicMaterial);
        }
    }
    else if (ScopeMesh && LensBaseMaterial)
    {
        ScopeMesh->SetMaterial(LensMaterialIndex, LensBaseMaterial);  // Fallback for non-local or no pawn
    }
}