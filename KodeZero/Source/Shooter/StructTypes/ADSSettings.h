#pragma once

#include "CoreMinimal.h"
#include "ADSSettings.generated.h"

USTRUCT(BlueprintType)
struct FADSSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ZoomedFOV = 30.f;  // Magnification level (lower = more zoom)

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InterpSpeed = 20.f;  // Transition speed for FOV/arm/offset

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TargetArmLength = 0.f;  // Closer camera distance during ADS

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector SocketOffset = FVector::ZeroVector;  // Camera nudge for alignment

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ADSSocketName = FName("ADS_Socket");  // Socket for rotation blend

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseRTTScope = false;  // True for RTT, false for legacy PNG/FOV
    
};