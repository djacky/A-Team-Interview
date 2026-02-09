// Fill out your copyright notice in the Description page of Project Settings.


#include "XPProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

void UXPProgressBar::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (AnimationCurve)
    {
        AnimationCurve->GetTimeRange(MinCurveTime, MaxCurveTime);
        AnimationDuration = MaxCurveTime - MinCurveTime;
    }
}

void UXPProgressBar::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    
    if (bIsAnimating)
    {
        AnimationElapsed += InDeltaTime;
        float NormalizedTime = FMath::Clamp(AnimationElapsed / AnimationDuration, 0.f, 1.f);
        
        float Alpha = NormalizedTime;
        if (AnimationCurve)
        {            
            // Map our normalized time (0-1) to the curve's actual time range
            float CurveTime = FMath::Lerp(MinCurveTime, MaxCurveTime, NormalizedTime);
            Alpha = AnimationCurve->GetFloatValue(CurveTime);
            Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
        }
        
        UpdateAnimation(Alpha);
        
        // Check if animation finished
        if (AnimationElapsed >= AnimationDuration)
        {
            bIsAnimating = false;
            OnAnimFinished();
        }
    }
}

void UXPProgressBar::SetMetric(const FString& MetricName, int32 InOldValue, int32 InNewValue, int32 InMaxValue)
{
    if (MetricTxt)
    {
        MetricTxt->SetText(FText::FromString(MetricName));
    }
    
    OldCount = FMath::Clamp(InOldValue, 0, InMaxValue);
    NewCount = FMath::Clamp(InNewValue, 0, InMaxValue);
    MaxCount = InMaxValue;
    
    // Calculate fill: 1 (0%) to -0.2 (100%)
    OldFill = 1.f - (1.2f * (static_cast<float>(OldCount) / MaxCount));
    NewFill = 1.f - (1.2f * (static_cast<float>(NewCount) / MaxCount));

    if (Border)
    {
        ProgressMaterial = Border->GetDynamicMaterial();
    }

    if (OldCount == NewCount)
    {
        // Immediate/static: Set final state without animation
        if (ProgressMaterial)
        {
            ProgressMaterial->SetScalarParameterValue(TEXT("Min"), NewFill);
            SetCompletedBar();
        }
        UpdateProgressText(NewCount);
    }
    else
    {
        // Start with old value, ready to animate
        UpdateProgressText(OldCount);
        if (ProgressMaterial)
        {
            ProgressMaterial->SetScalarParameterValue(TEXT("Min"), OldFill);
        }
        // Animation will be triggered externally via PlayFillAnimation()
    }
}

void UXPProgressBar::PlayFillAnimation()
{
    if (OldCount != NewCount)
    {
        bIsAnimating = true;
        AnimationElapsed = 0.f;
        if (ChargeStart) UGameplayStatics::PlaySound2D(this, ChargeStart);
        if (BarAnim) PlayAnimation(BarAnim);
    }
    else
    {
        // No animation needed, but still fire completion event
        OnAnimFinished();
    }
}

void UXPProgressBar::StopFillAnimation()
{
    bIsAnimating = false;
    AnimationElapsed = 0.f;
}

void UXPProgressBar::UpdateAnimation(float Alpha)
{
    float CurrentFill = FMath::Lerp(OldFill, NewFill, Alpha);
    if (ProgressMaterial)
    {
        ProgressMaterial->SetScalarParameterValue(TEXT("Min"), CurrentFill);
    }
    
    int32 CurrentCount = FMath::RoundToInt(FMath::Lerp(static_cast<float>(OldCount), static_cast<float>(NewCount), Alpha));
    UpdateProgressText(CurrentCount);
}

void UXPProgressBar::OnAnimFinished()
{
    // Ensure we're at final state
    SetCompletedBar();
    UpdateProgressText(NewCount);
    
    // Broadcast completion event
    OnAnimationComplete.Broadcast();
}

void UXPProgressBar::SetCompletedBar()
{
    if (ProgressMaterial)
    {
        ProgressMaterial->SetScalarParameterValue(TEXT("Min"), NewFill);
        if (NewCount == MaxCount)
        {
            ProgressMaterial->SetVectorParameterValue(TEXT("InsideColor"), FLinearColor(0.022865f, 0.359375f, 0.014303f, 1.f));
        }
    }
}

void UXPProgressBar::UpdateProgressText(int32 Count)
{
    if (CountTxt)
    {
        CountTxt->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Count, MaxCount)));
    }
}

