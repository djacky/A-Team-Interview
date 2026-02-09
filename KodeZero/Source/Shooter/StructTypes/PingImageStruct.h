#pragma once

#include "CoreMinimal.h"
#include "PingImageStruct.generated.h"

USTRUCT(BlueprintType)
struct FPingImage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Min = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Max = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSlateBrush Image = FSlateBrush();
};