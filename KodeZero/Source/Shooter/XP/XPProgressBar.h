// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TimelineComponent.h"
#include "XPProgressBar.generated.h"

/**
 * 
 */

class UTextBlock;
class UBorder;
class UMaterialInstanceDynamic;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProgressBarAnimationComplete);

UCLASS()
class SHOOTER_API UXPProgressBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Widget bindings
    UPROPERTY(meta = (BindWidget))
    UTextBlock* MetricTxt;
    
    UPROPERTY(meta = (BindWidget))
    UTextBlock* CountTxt;
    
    UPROPERTY(meta = (BindWidget))
    UBorder* Border;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* BarAnim;

    // Animation settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UCurveFloat* AnimationCurve;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    float AnimationDuration = 2.0f;

    // Event for when animation completes
    UPROPERTY(BlueprintAssignable, Category = "Animation")
    FOnProgressBarAnimationComplete OnAnimationComplete;

    // Main API
    UFUNCTION(BlueprintCallable, Category = "Progress")
    void SetMetric(const FString& MetricName, int32 InOldValue, int32 InNewValue, int32 InMaxValue);
    
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void PlayFillAnimation();
    
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void StopFillAnimation();
    
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsAnimating() const { return bIsAnimating; }

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class USoundCue* ChargeStart;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    // Material reference
    UPROPERTY()
    UMaterialInstanceDynamic* ProgressMaterial;

    // Animation state
    bool bIsAnimating = false;
    float AnimationElapsed = 0.f;
    
    // Progress values
    int32 OldCount = 0;
    int32 NewCount = 0;
    int32 MaxCount = 100;
    float OldFill = 1.f;
    float NewFill = 1.f;
    float MinCurveTime = 0.f;
    float MaxCurveTime = 1.f;

    // Internal methods
    void UpdateAnimation(float Alpha);
    void OnAnimFinished();
    void UpdateProgressText(int32 Count);
    void SetCompletedBar();
	
};
