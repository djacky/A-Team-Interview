#pragma once

UENUM(BlueprintType)
enum class EItemType : uint8
{
	EIT_Ammo UMETA(DisplayName = "Ammo"),
	EIT_Weapon UMETA(DisplayName = "Weapon"),
	EIT_Boost UMETA(DisplayName = "Boost"),
	EIT_Dummy UMETA(DisplayName = "Dummy"),

	EIT_MAX UMETA(DisplayName = "DefaultMAX")
};
