#pragma once

#include "CoreMinimal.h"
#include "StatsStruct.generated.h"

USTRUCT(BlueprintType)
struct FPlayerAttributes
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FSlateBrush IconImage = FSlateBrush();

};

USTRUCT(BlueprintType)
struct FVictimStats
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class AShooterPlayerState* Victim = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float DamageDealt = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Kills = 0;
};

USTRUCT(BlueprintType)
struct FPlayerGameStats
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlayerId = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlayerName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 FireCharacterHit = 0;
	//~Count number of times shot and missed
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 FireCharacterMiss = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ShotsFired = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ShotsHit = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageTaken = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageDealt = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NumOfKills = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NumOfDeaths = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Score = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BaseScore = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float KDR_raw = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DTR_raw = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Accuracy = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ShooterMeshName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FPlayerAttributes Attributes = FPlayerAttributes();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StartTimeMinutes = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float StopTimeMinutes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TimeInCombat = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CombatFactor = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 KillStreak = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MatchRank = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PingMs = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FVictimStats> VictimInteractions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ValorBoost = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ValorNorm = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float GraphScore = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ValorPenalty = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ValorPenaltyNorm = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NetValor = 0.f;

	float GetKDR(bool bIsRawValue) const
	{
		if (bIsRawValue)
		{
			return NumOfDeaths != 0 ? float(NumOfKills) / float(NumOfDeaths) : float(NumOfKills);
		}
		else
		{
			return float(NumOfKills) / (float(NumOfDeaths) + 1);
		}
	}

	float GetDTR(bool bIsRawValue) const
	{
        float TotalDamage = DamageDealt + DamageTaken;
        return TotalDamage > 0.f ? float(DamageDealt) / float(TotalDamage) : 0.f;
		/*
		if (bIsRawValue)
		{
			return DamageTaken != 0 ? float(DamageDealt) / float(DamageTaken) : 1.f;
		}
		else
		{
			return float(DamageDealt) / (float(DamageTaken) + 1);
		}
		*/
	}

	float GetTimeMinutesPlayed() const
	{
		return StopTimeMinutes - StartTimeMinutes;
	}

	float GetAccuracy() const
	{
		return ShotsFired > 0 ? float(ShotsHit) / float(ShotsFired) : 0.0f;
	}
};