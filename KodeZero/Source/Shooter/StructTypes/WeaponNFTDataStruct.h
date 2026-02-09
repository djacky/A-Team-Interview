#pragma once

#include "CoreMinimal.h"
#include "WeaponNFTDataStruct.generated.h"

USTRUCT(BlueprintType)
struct FWeaponNFTData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString NFT_ID = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString WeaponName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString WeaponType = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MainMeshName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ClipMeshName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FireSound = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString EquipSound = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PickupSound = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ImpactSound = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ReloadSound = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString CrosshairStyle = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCustomFireSound = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bWithGravity = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString AmmoType = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MagazineCapacity = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 AutoFireRate = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 WeaponDamageAmount = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 WeaponDamageRadius = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BulletSpeed = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString TracerEffect = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ImpactEffect = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MuzzleEffect = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> MuzzleEffectLocation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> LeftHandLocation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> LeftHandRotation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> RightHandLocation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> RightHandRotation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> ClipLocation = TArray<float>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> ClipRotation = TArray<float>();
};