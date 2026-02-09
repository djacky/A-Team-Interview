// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shooter/StructTypes/LevelRequirement.h"
#include "ProgressContainer.generated.h"

UENUM(BlueprintType)
enum class EProgressMode : uint8
{
    Static,    // No anim, just show current
    Animated   // Fill anims, sequential levels with rewards
};

class UVerticalBox;
class UXPProgressBar;
class UXPCounter;
class UDataTable;
class UTextBlock;

UCLASS()
class SHOOTER_API UProgressContainer : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    bool bEnableAnimations = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float DelayBetweenBars = 0.2f; // Stagger bar animations
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    float DelayAfterLevelUp = 0.5f; // Pause before showing reward
    
    // References
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* ProgressBarsContainer;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* MainContainer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<UXPCounter> XPCounterClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Widgets")
    UXPCounter* XPCounter;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<UXPProgressBar> ProgressBarClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets")
    TSubclassOf<class UXPRewardsWidget> LevelUpRewardWidgetClass;
    
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    //UDataTable* LevelRequirementsTable;

    // Main API
    UFUNCTION(BlueprintCallable, Category = "Progress")
    void InitializeForMainMenu();
    
    UFUNCTION(BlueprintCallable, Category = "Progress")
    void InitializeForEndMatch(const FPlayerProgress& OldProgress, const FPlayerProgress& DeltaProgress);

    UFUNCTION(BlueprintCallable, Category = "Progress")
    void InitializeForPauseMenu(const FPlayerProgress& DeltaProgress);
    
    // Called from reward widget's continue button
    UFUNCTION(BlueprintCallable, Category = "Progress")
    void OnRewardWidgetContinue();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
    FPlayerProgress CachedOldProgress;

protected:
    virtual void NativeConstruct() override;

private:
    // Internal state

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* LevelTxtBox;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* LevelTxt;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* SplitBox;
    
    FPlayerProgress CachedNewProgress;
    FPlayerProgress FreshProgress;
    TArray<int32> LevelsToProcess; // Queue of levels to animate
    int32 CurrentProcessingLevelIndex = 0;
    TArray<UXPProgressBar*> ActiveProgressBars;
    
    FTimerHandle AnimationDelayTimer;
    int32 CurrentAnimatingBarIndex = 0;
    
    // Core logic
    void PopulateProgressBars(int32 TargetLevel, const FPlayerProgress& OldProgress, const FPlayerProgress& NewProgress, bool bAnimate);
    void StartNextBarAnimation();
    
    UFUNCTION()
    void OnBarAnimationComplete();
    
    void CheckForLevelCompletion();
    void ShowLevelUpReward(int32 CompletedLevel);
    void ProcessNextLevel();
    int32 EffectiveNewLevel = 0;

	FString GetMetricDisplayName(EProgressMetric Type) const;
	FString SplitCamelCase(const FString& Input);

    void OnAllProcessed();
    void CreateXPCounter();
    
    // Helpers
    FLevelRequirement* GetLevelRequirement(int32 Level);
    int32 GetMetricValue(EProgressMetric Type, const TArray<FProgressMetricEntry>& Metrics);
    void SetMetricValue(EProgressMetric Metric, int32 NewValue, TArray<FProgressMetricEntry>& Metrics);
	
	
};
