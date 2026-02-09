#pragma once

UENUM(BlueprintType)
enum class EMovementModifiers : uint8
{
	EMM_None UMETA(DisplayName = "HoverNone"),
	EMM_ChargeAttack UMETA(DisplayName = "ChargeAttack"),
	EMM_DashGround UMETA(DisplayName = "DashGround"),
	EMM_DashAir UMETA(DisplayName = "DashAir"),
	EMM_HoverStart UMETA(DisplayName = "HoverStart"),
	EMM_HoverRush UMETA(DisplayName = "HoverRush"),
	EMM_HoverStop UMETA(DisplayName = "HoverStop"),
	EMM_GrappleStart UMETA(DisplayName = "GrappleStart"),
	EMM_GrappleStop UMETA(DisplayName = "GrappleStop"),
	EMM_ItemFlyStart UMETA(DisplayName = "ItemFlyStart"),
	EMM_ItemFlyStop UMETA(DisplayName = "ItemFlyStop"),
	EMM_Crouch UMETA(DisplayName = "Crouch"),
	EMM_Normal UMETA(DisplayName = "Normal"),
	EMM_Stop UMETA(DisplayName = "Stop"),
	EMM_AIPatrol UMETA(DisplayName = "AIPatrol"),
	EMM_MAX UMETA(DisplayName = "DefaultMAX")
};