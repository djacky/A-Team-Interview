#pragma once

UENUM(BlueprintType)
enum class EBoostType : uint8
{
	EBT_HealthV1 UMETA(DisplayName = "HealthV1"),
	EBT_HealthV2 UMETA(DisplayName = "HealthV2"),
	EBT_ShieldV1 UMETA(DisplayName = "ShieldV1"),
	EBT_ShieldV2 UMETA(DisplayName = "ShieldV2"),
	EBT_Ghost UMETA(DisplayName = "Ghost"),
	EBT_SuperJump UMETA(DisplayName = "SuperJump"),
	EBT_Teleport UMETA(DisplayName = "Teleport"),
	EBT_Protect UMETA(DisplayName = "Protect"),
	EBT_Fly UMETA(DisplayName = "Fly"),
	EBT_Slow UMETA(DisplayName = "Slow"),
	EBT_Copy UMETA(DisplayName = "Copy"),
	EBT_Grapple UMETA(DisplayName = "Grapple"),
	EBT_SuperPunch UMETA(DisplayName = "SuperPunch"),

	EBT_MAX UMETA(DisplayName = "MAX")
};