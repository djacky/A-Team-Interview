#pragma once

#include "CoreMinimal.h"
#include "StatsStructOld.generated.h"


USTRUCT(BlueprintType)
struct FStakeStats
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 numKills = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 numDeaths = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float numHit = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float numMiss = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 numGamesPlayed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float totalDamageDealt = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float totalDamageTaken = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float totalMinutesPlayed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float KDR = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DTR = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ShootingAcc = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ethEarned = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float polEarned = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float rating = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 privilege = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool postedVideo = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool reviewCompleted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool boughtGame = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool passedKyc = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString publicWalletKey = TEXT("");

};

USTRUCT(BlueprintType)
struct FPlayerStatisticsOld
{
	GENERATED_BODY() // needed for structs

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FStakeStats stakeStats = FStakeStats();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, int32> metrics = TMap<FString, int32>();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, int32> inventory = TMap<FString, int32>();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> badges = TArray<FString>();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FString> unlocks = TArray<FString>();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 currentLevel = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 totalXP = 0;

};

