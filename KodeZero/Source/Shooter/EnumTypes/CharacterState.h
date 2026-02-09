#pragma once

UENUM(BlueprintType)
enum class ECharacterProperty : uint8
{
	ECP_Leo UMETA(DisplayName = "Leo"),
	ECP_Maverick UMETA(DisplayName = "Maverick"),
	ECP_Raven UMETA(DisplayName = "Raven"),
	ECP_Trinity UMETA(DisplayName = "Trinity"),

	ECP_MAX UMETA(DisplayName = "DefaultMAX")
};