#pragma once

UENUM(BlueprintType)
enum class EEquippedItemType : uint8
{
	EEIT_weapon UMETA(DisplayName = "Weapon"),
	EEIT_boost UMETA(DisplayName = "Boost"),
	EEIT_unequipped UMETA(DisplayName = "Unequipped"),

	EEIT_MAX UMETA(DisplayName = "DefaultMAX")
};