#pragma once

#include "CoreMinimal.h"
#include "CallbackStruct.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FResponseCallback, const FString&, Value);

USTRUCT(BlueprintType)
struct FPendingRequest
{
    GENERATED_BODY()

    UPROPERTY()
    FString Value;

    FResponseCallback Callback;
};
