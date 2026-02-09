// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/Misc/Interfaces/PreviewActorInterface.h"
#include "Engine/StreamableManager.h"
#include "XPRewardPreview.generated.h"

class USceneCaptureComponent2D;
class USkeletalMeshComponent;

UCLASS()
class SHOOTER_API AXPRewardPreview : public AActor, public IPreviewActorInterface
{
	GENERATED_BODY()
	
public:    
    AXPRewardPreview();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneCaptureComponent2D* SceneCaptureComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* PreviewMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USkeletalMeshComponent* HairMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PreviewStaticMeshComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* CameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UNiagaraComponent* NiagaraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class USpringArmComponent* SpringArm;

    // Interface implementations
    void SetCharacterMesh(USkeletalMesh* Mesh, TSubclassOf<UAnimInstance> AnimBP);
    void PlayEmoteMontage(UAnimMontage* Montage);
    void SetPreviewRotation(FRotator Rotation);
    void SetPreviewScale(float Scale);
    void SetCameraOffset(FVector Offset);
    virtual void SetPreviewContent_Implementation(const FUnlockData& UnlockData) override;

    UFUNCTION(BlueprintCallable)
    void PlayParticleEffect();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> CurrentHandle;
    UPROPERTY()
    TSoftObjectPtr<UNiagaraSystem> CurrentSoftNiagara;
    
    UFUNCTION()
	void OnParticleLoaded();

    UFUNCTION()
    void LoadParticleAsync(const TSoftObjectPtr<UNiagaraSystem>& NiagaraSoftPtr);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float RotationSpeed = 90.0f; // Degrees per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float HoverAmplitude = 20.0f; // Hover distance in units

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
    float HoverFrequency = 2.0f; // Hover speed (higher = faster oscillation)

    float RunningTime = 0.0f;
    FVector InitialLocation;
    bool bHoverAnim = false;
	
};
