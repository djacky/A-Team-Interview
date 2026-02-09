// Fill out your copyright notice in the Description page of Project Settings.


#include "ProgressContainer.h"
#include "XPProgressBar.h"
#include "Components/VerticalBox.h"
#include "XPRewardsWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "XPCounter.h"
#include "Shooter/ShooterGameInstance.h"


// XPProgressContainer.cpp

void UProgressContainer::NativeConstruct()
{
    Super::NativeConstruct();
}

void UProgressContainer::InitializeForPauseMenu(const FPlayerProgress& DeltaProgress)
{
    if (UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        if (SplitBox) SplitBox->SetVisibility(ESlateVisibility::Collapsed);
        bEnableAnimations = false;
        CachedOldProgress = ShooterGI->CurrentPlayerProgress;
        UE_LOG(LogTemp, Warning, TEXT("InitializeForPauseMenu"));
        
        // Compute CachedNewProgress = Old + Deltas
        CachedNewProgress = CachedOldProgress;
        for (const FProgressMetricEntry& DeltaEntry : DeltaProgress.CurrentMetrics)
        {
            int32 OldValue = GetMetricValue(DeltaEntry.Type, CachedNewProgress.CurrentMetrics);
            SetMetricValue(DeltaEntry.Type, OldValue + DeltaEntry.Value, CachedNewProgress.CurrentMetrics);  // Add helper if not there (below)
        }
        int32 NextLevel = CachedOldProgress.CurrentLevel + 1;
        PopulateProgressBars(NextLevel, CachedNewProgress, CachedNewProgress, false);

        if (LevelTxtBox && LevelTxt)
        {
            FString LevelStr = FString::Printf(TEXT("Next Level: %i"), NextLevel);
            LevelTxt->SetText(FText::FromString(LevelStr));
            LevelTxtBox->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UProgressContainer::InitializeForMainMenu()
{
    if (UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        bEnableAnimations = false;
        CachedOldProgress = ShooterGI->CurrentPlayerProgress;
        int32 NextLevel = CachedOldProgress.CurrentLevel + 1;
        PopulateProgressBars(NextLevel, CachedOldProgress, CachedOldProgress, false);
        CreateXPCounter();
    }
}

void UProgressContainer::InitializeForEndMatch(const FPlayerProgress& OldProgress, const FPlayerProgress& DeltaProgress)
{
    // Note that DeltaProgress here is only the new metrics calculated per match.
    // The only
    bEnableAnimations = true;
    CachedOldProgress = OldProgress;
    FreshProgress = DeltaProgress;
    CreateXPCounter();
    
    // Compute CachedNewProgress = Old + Deltas
    CachedNewProgress = OldProgress;
    for (const FProgressMetricEntry& DeltaEntry : DeltaProgress.CurrentMetrics)
    {
        int32 OldValue = GetMetricValue(DeltaEntry.Type, CachedNewProgress.CurrentMetrics);
        SetMetricValue(DeltaEntry.Type, OldValue + DeltaEntry.Value, CachedNewProgress.CurrentMetrics);  // Add helper if not there (below)
    }

    // If using MatchXP, add it too: CachedNewProgress.TotalXP += DeltaProgress.MatchXP;

    // This is the new level got from the backend
    EffectiveNewLevel = DeltaProgress.CurrentLevel;
    UE_LOG(LogTemp, Warning, TEXT("EffectiveNewLevel = %i"), EffectiveNewLevel);
    
    LevelsToProcess.Empty();
    for (int32 Level = OldProgress.CurrentLevel + 1; Level <= EffectiveNewLevel + 1; ++Level)
    {
        UE_LOG(LogTemp, Warning, TEXT("Level %i"), Level);
        LevelsToProcess.Add(Level);
    }
    
    CurrentProcessingLevelIndex = 0;
    ProcessNextLevel();
}

void UProgressContainer::CreateXPCounter()
{
    if (XPCounterClass)
    {
        XPCounter = CreateWidget<UXPCounter>(this, XPCounterClass);
        if (XPCounter)
        {
            XPCounter->UpdateXPText(CachedOldProgress.TotalXP);
            UVerticalBoxSlot* BoxSlot = ProgressBarsContainer->AddChildToVerticalBox(XPCounter);
            if (BoxSlot)
            {
                FSlateChildSize ChildSize(ESlateSizeRule::Fill);
                ChildSize.Value = 0.2f;
                BoxSlot->SetSize(ChildSize);
                BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
                BoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
            }
        }
    }
}

void UProgressContainer::SetMetricValue(EProgressMetric Metric, int32 NewValue, TArray<FProgressMetricEntry>& Metrics)
{
    FProgressMetricEntry* Entry = Metrics.FindByPredicate([Metric](FProgressMetricEntry& E) { return E.Type == Metric; });
    if (Entry)
    {
        Entry->Value = NewValue;

    }
    else
    {
        FProgressMetricEntry NewEntry;
        NewEntry.Type = Metric;
        NewEntry.Value = NewValue;
        Metrics.Add(NewEntry);
    }
}

void UProgressContainer::ProcessNextLevel()
{
    if (CurrentProcessingLevelIndex >= LevelsToProcess.Num())
    {
        OnAllProcessed();  // New: XP anim here
        // All done!
        return;
    }
    int32 TargetLevel = LevelsToProcess[CurrentProcessingLevelIndex];
    
    // Determine old/new values for this specific level
    FPlayerProgress OldForThisLevel = CachedOldProgress;  // Always use global old for anim
    FPlayerProgress NewForThisLevel = CachedNewProgress;
    
    // If we've already completed this level, show it as complete from start
    if (!bEnableAnimations && TargetLevel <= CachedNewProgress.CurrentLevel)
    {
        // Only for static mode: set old to max for instant full bars
        FLevelRequirement* Requirement = GetLevelRequirement(TargetLevel);
        if (Requirement)
        {
            for (const FProgressMetricEntry& Entry : Requirement->RequiredMetrics)
            {
                int32 OldIndex = OldForThisLevel.CurrentMetrics.IndexOfByPredicate([&](const FProgressMetricEntry& E) {
                    return E.Type == Entry.Type;
                });
                if (OldIndex != INDEX_NONE)
                {
                    OldForThisLevel.CurrentMetrics[OldIndex].Value = Entry.Value;
                }
                else
                {
                    FProgressMetricEntry NewEntry;
                    NewEntry.Type = Entry.Type;
                    NewEntry.Value = Entry.Value;
                    OldForThisLevel.CurrentMetrics.Add(NewEntry);
                }
            }
        }
    }
    
    PopulateProgressBars(TargetLevel, OldForThisLevel, NewForThisLevel, bEnableAnimations);
    
    if (bEnableAnimations)
    {
        CurrentAnimatingBarIndex = 0;
        StartNextBarAnimation();
    }
    else
    {
        CheckForLevelCompletion();
    }
}

void UProgressContainer::PopulateProgressBars(int32 TargetLevel, const FPlayerProgress& OldProgress, 
                                                 const FPlayerProgress& NewProgress, bool bAnimate)
{
    // Clear existing bars
    if (MainContainer)
    {
        MainContainer->ClearChildren();
    }
    ActiveProgressBars.Empty();
    FLevelRequirement* Requirement = GetLevelRequirement(TargetLevel);
    if (!Requirement || !ProgressBarClass)
    {
        return;
    }

    FMargin SlotPadding = FMargin(80.f, 50.f, 80.f, 50.f);
    if (Requirement->RequiredMetrics.Num() == 4)
    {
        SlotPadding.Top = 25.f;
        SlotPadding.Bottom = 25.f; 
    }
    if (Requirement->RequiredMetrics.Num() > 4 && Requirement->RequiredMetrics.Num() <= 6)
    {
        SlotPadding.Top = 15.f;
        SlotPadding.Bottom = 15.f;
    }
    else if (Requirement->RequiredMetrics.Num() > 6)
    {
        SlotPadding.Top = 5.f;
        SlotPadding.Bottom = 5.f;
    }
    
    // Create a bar for each required metric
    for (const FProgressMetricEntry& RequiredMetric : Requirement->RequiredMetrics)
    {
        UXPProgressBar* NewBar = CreateWidget<UXPProgressBar>(this, ProgressBarClass);
        if (NewBar)
        {
            int32 OldValue = GetMetricValue(RequiredMetric.Type, OldProgress.CurrentMetrics);
            int32 NewValue = GetMetricValue(RequiredMetric.Type, NewProgress.CurrentMetrics);
            int32 MaxValue = RequiredMetric.Value;
            
            // Convert enum to string for display (you might want a better method)
            FString MetricName = GetMetricDisplayName(RequiredMetric.Type);
            NewBar->SetMetric(SplitCamelCase(MetricName), OldValue, NewValue, MaxValue);
            //UE_LOG(LogTemp, Warning, TEXT("Type: %s | ReqVal = %i | OldVal = %i | NewValue = %i | MaxValue = %i"), *UEnum::GetValueAsString(RequiredMetric.Type), RequiredMetric.Value, OldValue, NewValue, MaxValue);
            
            UVerticalBoxSlot* BoxSlot = MainContainer->AddChildToVerticalBox(NewBar);
            if (BoxSlot)
            {
                BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
                BoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
                BoxSlot->SetPadding(SlotPadding);
            }
            ActiveProgressBars.Add(NewBar);
        }
    }
}

FString UProgressContainer::SplitCamelCase(const FString& Input)
{
    if (Input.IsEmpty()) return Input;

    FString Result;
    TCHAR PrevChar = Input[0];
    Result.AppendChar(PrevChar);

    for (int32 i = 1; i < Input.Len(); ++i)
    {
        TCHAR CurrentChar = Input[i];

        // Insert space before uppercase letter if preceded by lowercase letter or digit
        // Or before digit if preceded by any letter (uppercase or lowercase)
        if ((FChar::IsUpper(CurrentChar) && (FChar::IsLower(PrevChar) || FChar::IsDigit(PrevChar))) ||
            (FChar::IsDigit(CurrentChar) && FChar::IsAlpha(PrevChar)))
        {
            Result.AppendChar(' ');
        }

        Result.AppendChar(CurrentChar);
        PrevChar = CurrentChar;
    }

    return Result;
}

void UProgressContainer::StartNextBarAnimation()
{
    if (CurrentAnimatingBarIndex >= ActiveProgressBars.Num())
    {
        // All bars animated, check for level completion
        GetWorld()->GetTimerManager().SetTimer(
            AnimationDelayTimer,
            this,
            &UProgressContainer::CheckForLevelCompletion,
            DelayAfterLevelUp,
            false
        );
        return;
    }
    
    // Get current bar
    UXPProgressBar* CurrentBar = ActiveProgressBars[CurrentAnimatingBarIndex];
    if (CurrentBar)
    {
        // Bind to completion event
        CurrentBar->OnAnimationComplete.AddDynamic(this, &UProgressContainer::OnBarAnimationComplete);
        
        // Start animation
        CurrentBar->PlayFillAnimation();
    }
    else
    {
        // No bar, move to next immediately
        OnBarAnimationComplete();
    }
}

void UProgressContainer::OnBarAnimationComplete()
{
    // Unbind from current bar to avoid duplicate calls
    if (CurrentAnimatingBarIndex < ActiveProgressBars.Num())
    {
        UXPProgressBar* CurrentBar = ActiveProgressBars[CurrentAnimatingBarIndex];
        if (CurrentBar)
        {
            CurrentBar->OnAnimationComplete.RemoveDynamic(this, &UProgressContainer::OnBarAnimationComplete);
        }
    }
    
    CurrentAnimatingBarIndex++;
    
    // Stagger the next bar
    GetWorld()->GetTimerManager().SetTimer(
        AnimationDelayTimer,
        this,
        &UProgressContainer::StartNextBarAnimation,
        DelayBetweenBars,
        false
    );
}

void UProgressContainer::CheckForLevelCompletion()
{
    int32 CurrentLevel = LevelsToProcess[CurrentProcessingLevelIndex];
    
    UE_LOG(LogTemp, Warning, TEXT("CheckForLevelCompletion: %i"), CurrentLevel);
    if (CurrentLevel <= EffectiveNewLevel)
    {
        UE_LOG(LogTemp, Warning, TEXT("CheckForLevelCompletion Completed"));
        // Level completed! Show reward
        ShowLevelUpReward(CurrentLevel);
    }
    else
    {
        // Not completed, we're done
        CurrentProcessingLevelIndex = LevelsToProcess.Num(); // End processing
        OnAllProcessed();
    }
}

void UProgressContainer::ShowLevelUpReward(int32 CompletedLevel)
{
    //OnRewardWidgetContinue();
    
    FLevelRequirement* Requirement = GetLevelRequirement(CompletedLevel);
    if (!Requirement || !LevelUpRewardWidgetClass || Requirement->UnlockID.IsNone())
    {
        OnRewardWidgetContinue(); // Skip to next
        return;
    }
    
    UXPRewardsWidget* RewardWidget = CreateWidget<UXPRewardsWidget>(this, LevelUpRewardWidgetClass);
    if (RewardWidget)
    {
        // Set the unlock using the UnlockID from the level requirement
        RewardWidget->SetUnlockReward(Requirement->UnlockID, CompletedLevel, Requirement->Quantity, Requirement->bIsInventory);
        
        // Bind continue event
        RewardWidget->OnContinueClicked.AddDynamic(this, &UProgressContainer::OnRewardWidgetContinue);
        
        // Add to viewport
        RewardWidget->AddToViewport(100);
    }
    else
    {
        OnRewardWidgetContinue(); // Fallback
    }
    
}

void UProgressContainer::OnRewardWidgetContinue()
{
    // Move to next level
    CurrentProcessingLevelIndex++;
    ProcessNextLevel();
}

// Helper implementations
FLevelRequirement* UProgressContainer::GetLevelRequirement(int32 Level)
{
    if (UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        return ShooterGI->GetCumulativeLevelRequirement(Level);
    }
    
    return nullptr;
}

int32 UProgressContainer::GetMetricValue(EProgressMetric Type, const TArray<FProgressMetricEntry>& Metrics)
{
    const FProgressMetricEntry* Found = Metrics.FindByPredicate([Type](const FProgressMetricEntry& Entry) {
        return Entry.Type == Type;
    });
    
    return Found ? Found->Value : 0;
}

FString UProgressContainer::GetMetricDisplayName(EProgressMetric Type) const
{
    UEnum* EnumPtr = StaticEnum<EProgressMetric>();
    if (!EnumPtr) return TEXT("Unknown");
    
    FString EnumName = EnumPtr->GetNameStringByValue(static_cast<int64>(Type));
    if (EnumName.StartsWith(TEXT("EPM_")))
    {
        EnumName.RightChopInline(4); // Remove first 4 characters
    }
    
    return EnumName;
}

void UProgressContainer::OnAllProcessed()
{
    // Compute MatchXP using Old + Deltas (from NewProgress.CurrentMetrics, since fresh)
    int32 OldTotalXP = CachedOldProgress.TotalXP;
    int32 EarnedXP = FreshProgress.TotalXP - OldTotalXP;
    
    UE_LOG(LogTemp, Warning, TEXT("EarnedXP = %i | OldXP = %i"), EarnedXP, OldTotalXP);
    
    if (XPCounter)
    {
        XPCounter->AnimateFromOldToNew(OldTotalXP, OldTotalXP + EarnedXP);
    }
    if (UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        ShooterGI->CurrentPlayerProgress.CurrentLevel = EffectiveNewLevel;
    }
}