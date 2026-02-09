#pragma once

#include "CoreMinimal.h"
#include "Shooter/StructTypes/PlayerProgress.h"
#include "LevelRequirement.generated.h"

USTRUCT(BlueprintType)
struct FLevelRequirement : public FTableRowBase
{
    GENERATED_BODY()

    // Map of metric to required value (flexible: any metric can be added per level)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FProgressMetricEntry> RequiredMetrics = TArray<FProgressMetricEntry>();

    // Badge and unlock rewards (use FName for asset refs, or strings for backend IDs)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BadgeID = FName("");  // e.g., "Badge_Level1"

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName UnlockID = FName(""); // e.g., "Skin_RedCharacter" or "Mode_Deathmatch"

    // Amount of the unlock reward to give
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 1;

    // Is the unlock an inventory with limited/modifiable amounts
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsInventory = false;

};