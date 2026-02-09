#pragma once

#include "CoreMinimal.h"
#include "EnemyTypeStruct.generated.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	EET_TwinBlast UMETA(DisplayName = "EET_TwinBlast"),
    EET_Howitzer UMETA(DisplayName = "EET_Howitzer"),

    EET_Default UMETA(DisplayName = "EET_Default"),

	EET_MAX UMETA(DisplayName = "MAX")
};

USTRUCT(BlueprintType)
struct FEnemyReplicationData
{
    GENERATED_BODY()

    UPROPERTY()
    EEnemyType EnemyType = EEnemyType::EET_MAX;

    UPROPERTY()
    USkeletalMesh* SkeletalMesh = nullptr;
};