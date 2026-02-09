#pragma once

UENUM(BlueprintType)
enum class EMapEvent : uint8
{
	EME_SendMultiplier UMETA(DisplayName = "SendMultiplier"),

	EME_MAX UMETA(DisplayName = "DefaultMAX")
};