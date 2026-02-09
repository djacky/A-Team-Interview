// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponPropertyStruct.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FWeaponData_Base
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString Type = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FTransform Offset = FTransform();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FTransform LeftHand = FTransform();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString ModelURI = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString CacheID = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	float FireRate = 1.f;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString PickupSoundID = TEXT("Gun_Pickup_2_Cue");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString EquipSoundID = TEXT("Sci-Fi_Device_10_Cue");
};

USTRUCT(BlueprintType, Blueprintable)
struct FWeaponData_Bullet
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString ID = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	float Damage = 10.f;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	float Radius = 10.f;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	float Speed = 10.f;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	bool Drop = true;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString ImpactSoundID = TEXT("ImpactBig6_Cue");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString ImpactEffectID = TEXT("NS_HitEffect7");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString TrailID = TEXT("");
};

USTRUCT(BlueprintType, Blueprintable)
struct FWeaponData_Magazine
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FTransform Location = FTransform();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString URI = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString CacheID = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString ReloadSoundID = TEXT("Reload13_Cue");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	int Capacity = 10;
};

USTRUCT(BlueprintType, Blueprintable)
struct FWeaponData_Muzzle
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FTransform Location = FTransform();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString FlashID = TEXT("NS_MuzzleFlash17");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString FireSoundID = TEXT("CyberGun_11_Cue");
};
USTRUCT(BlueprintType, Blueprintable)
struct FWeaponData_OffChain
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FWeaponData_Base Base = FWeaponData_Base();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FWeaponData_Magazine Magazine = FWeaponData_Magazine();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FWeaponData_Muzzle Muzzle = FWeaponData_Muzzle();
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FWeaponData_Bullet Bullet = FWeaponData_Bullet();
};
USTRUCT(BlueprintType, Blueprintable)
struct FWeaponDataStruct
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString _id = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString name = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString description = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString s3ImageUri = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString s3ModelUri = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString status = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	float price = 0.f;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	int amount = 0;
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FString creator = TEXT("");
	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	FWeaponData_OffChain offChainMetadata = FWeaponData_OffChain();
};
