// Fill out your copyright notice in the Description page of Project Settings.


#include "XPRewardPreview.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Animation/AnimInstance.h"
#include "Shooter/StructTypes/UnlockTypes.h"
#include "Shooter/ShooterGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"


AXPRewardPreview::AXPRewardPreview()
{
    PrimaryActorTick.bCanEverTick = true;
    
    // Create root component
    CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
    RootComponent = CameraRoot;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 300.f;  // Adjust distance
    SpringArm->bDoCollisionTest = false;
    
    // Create skeletal mesh component
    PreviewMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewMesh"));
    PreviewMeshComponent->SetupAttachment(RootComponent);
    PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HairMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
    HairMeshComponent->SetupAttachment(PreviewMeshComponent);
    HairMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    PreviewStaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewStaticMesh"));
    PreviewStaticMeshComponent->SetupAttachment(RootComponent);
    PreviewStaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Create scene capture component
    SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCaptureComponent->SetupAttachment(SpringArm);
    SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    SceneCaptureComponent->bCaptureEveryFrame = true;
    SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;

    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NiagaraComponent->SetupAttachment(SceneCaptureComponent);
    NiagaraComponent->SetAutoActivate(false);

    //SceneCaptureComponent->ShowOnlyActorComponents(this);

}

void AXPRewardPreview::BeginPlay()
{
    Super::BeginPlay();
    if (SceneCaptureComponent)
    {
        SceneCaptureComponent->ShowOnlyActorComponents(this);
    }
    InitialLocation = PreviewStaticMeshComponent->GetRelativeLocation();
}

void AXPRewardPreview::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bHoverAnim)
    {
        RunningTime += DeltaTime;

        // Continuous rotation (around local Z-axis/yaw)
        FRotator NewRotation = PreviewStaticMeshComponent->GetRelativeRotation();
        NewRotation.Yaw += RotationSpeed * DeltaTime;
        PreviewStaticMeshComponent->SetRelativeRotation(NewRotation);

        /*
        // Sine-wave hovering (up and down on local Z-axis)
        FVector NewLocation = InitialLocation;
        NewLocation.Z += FMath::Sin(RunningTime * HoverFrequency) * HoverAmplitude;
        PreviewStaticMeshComponent->SetRelativeLocation(NewLocation);
        */
    }
}

void AXPRewardPreview::SetCharacterMesh(USkeletalMesh* Mesh, TSubclassOf<UAnimInstance> AnimBP)
{
    if (PreviewMeshComponent && Mesh)
    {
        PreviewMeshComponent->SetSkeletalMesh(Mesh);
        
        if (AnimBP)
        {
            PreviewMeshComponent->SetAnimInstanceClass(AnimBP);
        }

        if (Mesh->GetName().Contains(TEXT("Leo")))
        {
            //Set Hair For Leo
            if (HairMeshComponent && SceneCaptureComponent)
            {
                HairMeshComponent->SetLeaderPoseComponent(PreviewMeshComponent);
                HairMeshComponent->SetHiddenInGame(false);
            }
        }
    }
}

void AXPRewardPreview::PlayEmoteMontage(UAnimMontage* Montage)
{
    if (PreviewMeshComponent && Montage)
    {
        UAnimInstance* AnimInstance = PreviewMeshComponent->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);
            AnimInstance->Montage_JumpToSection("Emote");
        }
    }
}

void AXPRewardPreview::SetPreviewRotation(FRotator Rotation)
{
    if (PreviewMeshComponent)
    {
        PreviewMeshComponent->SetRelativeRotation(Rotation);
    }
}

void AXPRewardPreview::SetPreviewScale(float Scale)
{
    if (PreviewMeshComponent)
    {
        PreviewMeshComponent->SetRelativeScale3D(FVector(Scale));
    }
}

void AXPRewardPreview::SetCameraOffset(FVector Offset)
{
    if (SceneCaptureComponent)
    {
        SceneCaptureComponent->SetRelativeLocation(Offset);
        
        // Look at the mesh
        FVector LookAtLocation = PreviewMeshComponent->GetComponentLocation();
        FRotator LookAtRotation = (LookAtLocation - SceneCaptureComponent->GetComponentLocation()).Rotation();
        SceneCaptureComponent->SetRelativeRotation(LookAtRotation);
    }
}

void AXPRewardPreview::SetPreviewContent_Implementation(const FUnlockData& UnlockData)
{
    switch (UnlockData.UnlockType)
    {
        case EUnlockType::CharacterSkin:
            if (!UnlockData.CharacterMesh.IsNull())
            {
                USkeletalMesh* Mesh = UnlockData.CharacterMesh.LoadSynchronous();
                TSubclassOf<UAnimInstance> AnimBP = UnlockData.AnimBlueprintClass.LoadSynchronous();
                SetCharacterMesh(Mesh, AnimBP);
                LoadParticleAsync(UnlockData.UnlockNiagaraEffect);
            }
            break;
            
        case EUnlockType::Emote:
            if (!UnlockData.EmoteMontage.IsNull())
            {
                UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this));
                if (ShooterGI)
                {
                    USkeletalMesh* SelectSkelMesh = ShooterGI->GetSelectedSkeletalMesh();
                    if (SelectSkelMesh == nullptr)
                    {
                        SelectSkelMesh = ShooterGI->GetDefaultSelectedSkeletalMesh().LoadSynchronous();
                    }
                    TSubclassOf<UAnimInstance> AnimBP = UnlockData.AnimBlueprintClass.LoadSynchronous();
                    SetCharacterMesh(SelectSkelMesh, AnimBP);
                    UAnimMontage* Montage = UnlockData.EmoteMontage.LoadSynchronous();
                    PlayEmoteMontage(Montage);
                    LoadParticleAsync(UnlockData.UnlockNiagaraEffect);
                }
            }
            break;
        case EUnlockType::Asset:
            if (!UnlockData.PreviewMesh.IsNull())
            {
                UStaticMesh* Mesh = UnlockData.PreviewMesh.LoadSynchronous();
                PreviewStaticMeshComponent->SetRelativeScale3D(FVector(UnlockData.PreviewScale));
                FVector CurrentLocation = PreviewStaticMeshComponent->GetRelativeLocation();
                PreviewStaticMeshComponent->SetRelativeLocation(CurrentLocation + UnlockData.PreviewLocationOffset);
                PreviewStaticMeshComponent->SetStaticMesh(Mesh);
                LoadParticleAsync(UnlockData.UnlockNiagaraEffect);
                bHoverAnim = true;
            }
            break;
            
        default:
            break;
    }
    
    // Apply preview settings
    //SetPreviewRotation(UnlockData.PreviewRotation);
    //SetPreviewScale(UnlockData.PreviewScale);
    //SetCameraOffset(UnlockData.PreviewCameraOffset);
}

void AXPRewardPreview::LoadParticleAsync(const TSoftObjectPtr<UNiagaraSystem>& NiagaraSoftPtr)
{
    CurrentSoftNiagara = NiagaraSoftPtr;
    if (CurrentSoftNiagara.IsValid())
    {
        // Already loaded (synchronous case)
        OnParticleLoaded();
        return;
    }
	if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
	{
		CurrentHandle = StreamableManager.RequestAsyncLoad(
			CurrentSoftNiagara.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &AXPRewardPreview::OnParticleLoaded)
		);
	}
}

void AXPRewardPreview::OnParticleLoaded()
{
    UNiagaraSystem* LoadedParticle = CurrentSoftNiagara.Get();
    if (LoadedParticle && NiagaraComponent)
    {
        NiagaraComponent->SetAsset(LoadedParticle);
        NiagaraComponent->Activate(true);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load particle"));
    }
}

void AXPRewardPreview::PlayParticleEffect()
{
    if (NiagaraComponent)
    {
        NiagaraComponent->ActivateSystem(true);  // Resets and plays
    }
}