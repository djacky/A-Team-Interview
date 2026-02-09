#pragma once

#include "CoreMinimal.h"
#include "RequirementsCheckStruct.generated.h"

USTRUCT(BlueprintType)
struct FRequirementsCheck
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsFullVersion = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bGotStats = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bKYC = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString WalletAddress = TEXT("");
};