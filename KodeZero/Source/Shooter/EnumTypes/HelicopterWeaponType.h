#pragma once


UENUM(BlueprintType)
enum class EHelicopterWeaponType : uint8
{
	EHWT_PulsedBlaster UMETA(DisplayName = "PulsedBlaster"),
	EHWT_Missile UMETA(DisplayName = "Missile"),

	EHWT_MAX UMETA(DisplayName = "MAX")
};