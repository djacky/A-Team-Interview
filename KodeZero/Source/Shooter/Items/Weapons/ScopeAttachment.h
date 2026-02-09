// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScopeAttachment.generated.h"

UCLASS()
class SHOOTER_API AScopeAttachment : public AActor
{
	GENERATED_BODY()
	
public:
    AScopeAttachment();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Scope")
    void ActivateScope(bool bActivate);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* ScopeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    class USceneCaptureComponent2D* ScopeCapture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    UTextureRenderTarget2D* RenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    UMaterialInterface* LensBaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    float MagnifiedFOV = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    FName LensSocketName = FName("Lens_Socket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    int32 LensMaterialIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Scope")
    bool bIsActive = false;

    void SetScopeOwner(APawn* OwnerPawn);

protected:
    virtual void BeginPlay() override;

    UMaterialInstanceDynamic* LensDynamicMaterial;

    APawn* CachedOwningPawn;
    APlayerController* CachedPlayerController;

    void SetupDynamicLensMaterial();  // Helper for RTT setup

private:
    bool bNeedsCaptureUpdate = true;
	
};
