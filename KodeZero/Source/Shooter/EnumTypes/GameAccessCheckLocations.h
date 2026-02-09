#pragma once


UENUM(BlueprintType)
enum class EGameAccessLocations : uint8
{
	EGAL_Character UMETA(DisplayName = "Character"),
	EGAL_Settings UMETA(DisplayName = "Settings"),
	EGAL_Play UMETA(DisplayName = "Play"),
	EGAL_MAX UMETA(DisplayName = "MAX")
};