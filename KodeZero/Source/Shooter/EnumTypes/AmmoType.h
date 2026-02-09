#pragma once

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(DisplayName = "9mm"),
	EAT_45mm UMETA(DisplayName = "45mm"),
	EAT_AR UMETA(DisplayName = "AssaultRifle"),
	EAT_Shells UMETA(DisplayName = "Shells"),
	EAT_GrenadeRounds UMETA(DisplayName = "GrenadeRounds"),
	EAT_HelicopterMissiles UMETA(DisplayName = "HelicopterMissiles"),
	EAT_GravCharges UMETA(DisplayName = "GravCharges"),
	EAT_MAX UMETA(DisplayName = "MAX")
};