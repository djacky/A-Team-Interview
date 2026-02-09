#pragma once

UENUM(BlueprintType)
enum class EGameMatchState : uint8
{
	EMS_PreIdle UMETA(DisplayName = "PreIdle"),
	EMS_Idle UMETA(DisplayName = "Idle"),
	EMS_Warn UMETA(DisplayName = "Warn"),
	EMS_Start UMETA(DisplayName = "Start"),
	EMS_Stop UMETA(DisplayName = "Stop"),

	EMS_MAX UMETA(DisplayName = "DefaultMAX")
};