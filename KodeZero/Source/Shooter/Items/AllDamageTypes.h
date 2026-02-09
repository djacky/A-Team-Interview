// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "GameplayTagContainer.h"
#include "AllDamageTypes.generated.h"

// Base class for all your custom damage types
UCLASS()
class SHOOTER_API UCustomDamageTypeBase : public UDamageType
{
    GENERATED_BODY()
    
public:
    // The gameplay tag identifying this damage type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
    FGameplayTag DamageTag;
    
    // Optional: XP reward for kills with this damage type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "XP")
    int32 BaseXPReward = 100;
};

// Now your specific damage types inherit from this
UCLASS()
class SHOOTER_API UCyberPunchType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UCyberPunchType()
    {
        // Set the tag in the constructor
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Item.CyberPunch"));
    }
};

UCLASS()
class SHOOTER_API UDroneMissileType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UDroneMissileType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Drone.Missile"));
    }
};

UCLASS()
class SHOOTER_API UDronePulsarType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UDronePulsarType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Drone.Pulsar"));
    }
};

UCLASS()
class SHOOTER_API USniperType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    USniperType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.Sniper"));
    }
};

UCLASS()
class SHOOTER_API UPistolType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UPistolType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.Pistol"));
    }
};

UCLASS()
class SHOOTER_API UARType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UARType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.AR"));
    }
};

UCLASS()
class SHOOTER_API USubmachineGunType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    USubmachineGunType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.SMG"));
    }
};

UCLASS()
class SHOOTER_API UGravitonType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UGravitonType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.Graviton"));
    }
};

UCLASS()
class SHOOTER_API UGLType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UGLType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.GL"));
    }
};

UCLASS()
class SHOOTER_API UCyperPistolType : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    UCyperPistolType()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Weapon.CyberPistol"));
    }
};

UCLASS()
class SHOOTER_API URushAttack : public UCustomDamageTypeBase
{
    GENERATED_BODY()
    
public:
    URushAttack()
    {
        DamageTag = FGameplayTag::RequestGameplayTag(FName("Attack.Character.RushAttack"));
    }
};