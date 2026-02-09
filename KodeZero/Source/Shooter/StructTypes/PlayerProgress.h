#pragma once

#include "CoreMinimal.h"
#include "Shooter/EnumTypes/ProgressMetric.h"
#include "PlayerProgress.generated.h"

USTRUCT(BlueprintType)
struct FProgressMetricEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)  // Ensure replicable
    EProgressMetric Type = EProgressMetric::EPM_MAX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Value = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 XPPerUnit = 0;

};

USTRUCT(BlueprintType)
struct FInventory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)  // Ensure replicable
    FName Name = FName();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Amount = 0;

};

USTRUCT(BlueprintType)
struct FPlayerProgress
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    int32 CurrentLevel = 0;  // Starts at 0

    // Map of metric to current value (updated incrementally)
    UPROPERTY(BlueprintReadWrite)
    TArray<FProgressMetricEntry> CurrentMetrics = TArray<FProgressMetricEntry>();  // Array instead of map

    UPROPERTY(BlueprintReadWrite)
    TArray<FInventory> Inventory = TArray<FInventory>();  // Array instead of map

    // Array of unlocked badges/unlocks (for quick client-side access post-verification)
    UPROPERTY(BlueprintReadWrite)
    TArray<FName> UnlockedBadges = TArray<FName>();

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> UnlockedRewards = TArray<FName>();

    UPROPERTY(BlueprintReadWrite)
    int32 TotalXP = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 MatchXP = 0;
};