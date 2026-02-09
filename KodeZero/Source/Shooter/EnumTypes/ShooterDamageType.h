#pragma once

UENUM(BlueprintType)
enum class EShooterDamageType : uint8
{
	ESDT_Gun UMETA(DisplayName = "Gun"),
	ESDT_Sniper UMETA(DisplayName = "Sniper"),
	ESDT_SMG UMETA(DisplayName = "SMG"),
	ESDT_AR UMETA(DisplayName = "AR"),
	ESDT_Pistol UMETA(DisplayName = "Pistol"),
	ESDT_Shotgun UMETA(DisplayName = "Shotgun"),
	ESDT_Hand UMETA(DisplayName = "Hand"),
	ESDT_HeadShot UMETA(DisplayName = "HeadShot"),
	ESDT_Explosion UMETA(DisplayName = "Explosion"),
	ESDT_Helicopter UMETA(DisplayName = "Helicopter"),
	EVO_MAX UMETA(DisplayName = "MAX")
};