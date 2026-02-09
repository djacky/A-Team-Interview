#pragma once

#include "CoreMinimal.h"
#include "BlockchainStruct.generated.h"

USTRUCT(BlueprintType)
struct FBlockchainInfo
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<uint8> WalletPrivateKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<uint8> WalletPublicKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString WalletAddress = TEXT("");
};