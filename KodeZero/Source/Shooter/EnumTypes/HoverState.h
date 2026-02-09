#pragma once

UENUM(BlueprintType)
enum class EHoverState : uint8
{
	EHS_None UMETA(DisplayName = "HoverNone"),
	EHS_HoverStart UMETA(DisplayName = "HoverStart"),
	EHS_HoverRush UMETA(DisplayName = "HoverRush"),
	EHS_HoverStop UMETA(DisplayName = "HoverStop"),
	EHS_HoverFinish UMETA(DisplayName = "HoverFinish"),
	EHS_MAX UMETA(DisplayName = "DefaultMAX")
};