#pragma once

#include "CoreMinimal.h"
#include "TournamentStruct.generated.h"

USTRUCT(BlueprintType)
struct FStake_stakeInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString network = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> limits = TArray<float>();

};

USTRUCT(BlueprintType)
struct FTournamentParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString _id = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString name = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString time = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString date = TEXT("");
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 timeUnixSeconds = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString description = TEXT("");
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString reward = TEXT("");
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool isPlayerRegistered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool isFull = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 numPlayers = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 limit = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 minPlayers = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString mode = TEXT("solo");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString region = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString type = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool hasPassword = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool needsStakeTicket = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString handicap = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString multiplier = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float totalStake = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FStake_stakeInfo stakeInfo = FStake_stakeInfo();
};