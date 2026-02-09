#pragma once

UENUM(BlueprintType)
enum class EVoiceOptions : uint8
{
	EVO_Volume UMETA(DisplayName = "Volume"),
	EVO_MicGain UMETA(DisplayName = "MicGain"),
	EVO_MuteMic UMETA(DisplayName = "MuteMic"),
	EVO_Deafen UMETA(DisplayName = "Deafen"),
	EVO_LocalParticipantVolume UMETA(DisplayName = "LocalParticipantVolume"),
	EVO_MAX UMETA(DisplayName = "MAX")
};