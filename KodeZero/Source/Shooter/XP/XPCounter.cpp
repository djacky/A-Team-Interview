// Fill out your copyright notice in the Description page of Project Settings.


#include "XP/XPCounter.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"


void UXPCounter::NativeConstruct()
{
    Super::NativeConstruct();
    if (AnimationCurve)
    {
        AnimationCurve->GetTimeRange(MinCurveTime, MaxCurveTime);
        AnimationDuration = MaxCurveTime - MinCurveTime;
    }
}

void UXPCounter::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
        
        int32 CurrentXP = FMath::RoundToInt(FMath::Lerp(static_cast<float>(StartXP), static_cast<float>(TargetXP), Alpha));
        UpdateXPText(CurrentXP);
        
        if (AnimationElapsed >= AnimationDuration)
        {
            bIsAnimating = false;
            UpdateXPText(TargetXP);  // Force final exact value
            OnXPAnimFinished();
        }
    }
}

void UXPCounter::OnXPAnimFinished()
{
    if (XPUpdateAnim) PlayAnimation(XPUpdateAnim);
}

void UXPCounter::AnimateFromOldToNew(int32 OldXP, int32 NewXP)
{
    StartXP = OldXP;
    TargetXP = NewXP;
    
    if (StartXP == TargetXP)
    {
        UpdateXPText(TargetXP);  // Instant
    }
    else
    {
        if (CoinANim) PlayAnimation(CoinANim, 0, 0);
        if (XPChargeStart) UGameplayStatics::PlaySound2D(this, XPChargeStart);
        bIsAnimating = true;
        AnimationElapsed = 0.f;
        UpdateXPText(StartXP);  // Start at old
    }
}

void UXPCounter::UpdateXPText(int32 CurrentXP)
{
    if (XPText)
    {
        XPText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentXP)));
    }
}

