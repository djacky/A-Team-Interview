#pragma once

#include "CoreMinimal.h"
#include "PlayerStakeMetrics.generated.h"


USTRUCT(BlueprintType)
struct FPlayerLatestMetric
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString userId = TEXT("");

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<float> kd = TArray<float>();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<float> dt = TArray<float>();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<float> pl = TArray<float>();

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float stake = 0.f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString multiplier = TEXT("None");
};


USTRUCT(BlueprintType)
struct FAllPlayersLatestMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FPlayerLatestMetric> playerMetrics = TArray<FPlayerLatestMetric>();
};