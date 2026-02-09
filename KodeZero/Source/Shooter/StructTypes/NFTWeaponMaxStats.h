#pragma once

#include "CoreMinimal.h"
#include "NFTWeaponMaxStats.generated.h"

USTRUCT(BlueprintType)
struct FWeaponMaxStatsValues
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float Damage = 120.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float DamageRadius = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float BulletSpeed = 800.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int MagazineCapacity = 30.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float FireRate = 10.f;
};