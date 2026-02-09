#pragma once

UENUM(BlueprintType)
enum class EHandicapState : uint8
{
	EHANS_NoShield UMETA(DisplayName = "NoShield"),
	EHANS_SecondPlaceReward UMETA(DisplayName = "SecondPlaceReward"),
	EHANS_XWeaponDamage UMETA(DisplayName = "XWeaponDamage"),
	EHANS_LessHealth UMETA(DisplayName = "LessHealth"),
	EHANS_NoFallingDamage UMETA(DisplayName = "NoFallingDamage"),
	EHANS_XFasterDash UMETA(DisplayName = "XFasterDash"),
	EHANS_OnlyGrenadeLaunch UMETA(DisplayName = "OnlyGrenadeLaunch"),
	EHANS_NoWeapons UMETA(DisplayName = "NoWeapons"),
	EHANS_NoBoosts UMETA(DisplayName = "NoBoosts"),
	EHANS_OnlyPistols UMETA(DisplayName = "OnlyPistols"),
	EHANS_XScoreFirstPlace UMETA(DisplayName = "XScoreFirstPlace"),

	EHANS_MAX UMETA(DisplayName = "DefaultMAX")
};