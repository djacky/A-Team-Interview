#pragma once


UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_AR UMETA(DisplayName = "AssaultRifle"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	EWT_Sniper UMETA(DisplayName = "SniperRifle"),
	EWT_GrenadeLauncher UMETA(DisplayName = "GrenadeLauncher"),
	EWT_CyberPistol UMETA(DisplayName = "CyberPistol"),
	EWT_GravCannon UMETA(DisplayName = "GravCannon"),

	EWT_HeliMissile UMETA(DisplayName = "HeliMissile"),
	EWT_HeliPulser UMETA(DisplayName = "HeliPulser"),
	EWT_Test UMETA(DisplayName = "Test"),
	EWT_MAX UMETA(DisplayName = "MAX")
};