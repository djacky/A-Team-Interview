#pragma once

// Enum for all possible metrics (scalable: add new ones here)
UENUM(BlueprintType)
enum class EProgressMetric : uint8
{
    EPM_StyleXP UMETA(DisplayName = "StyleXP"),          // Cumulative XP from style points
    EPM_SoloMatchesPlayed UMETA(DisplayName = "SoloMatchesPlayed"),      // Number of solo matches completed
	EPM_TeamMatchesPlayed UMETA(DisplayName = "TeamMatchesPlayed"), // Number of team matches played
	EPM_StakingMatchesPlayed UMETA(DisplayName = "StakingMatchesPlayed"), // Number of staking matches played

	// Player Elim tasks
	EPM_ElimPlayerWithSniper UMETA(DisplayName = "ElimPlayerWithSniper"),
	EPM_ElimPlayerWithGraviton UMETA(DisplayName = "ElimPlayerWithGraviton"),
	EPM_ElimPlayerWithPistol UMETA(DisplayName = "ElimPlayerWithPistol"),
	EPM_ElimPlayerWithCyberPunch UMETA(DisplayName = "ElimPlayerWithCyberPunch"),
	EPM_ElimPlayerWhileCrouched UMETA(DisplayName = "ElimPlayerWhileCrouched"),
	EPM_ElimWithDroneMissile UMETA(DisplayName = "ElimWithDroneMissile"),
	EPM_ElimWithDronePulser UMETA(DisplayName = "ElimWithDronePulser"),
	EPM_KillDuringGrapple UMETA(DisplayName = "KillDuringGrapple"),
	EPM_ElimFrom100mPlus UMETA(DisplayName = "ElimFrom100mPlus"),

	// AI-based Elim tasks
	EPM_ElimAiWithSniper UMETA(DisplayName = "ElimAiWithSniper"),
    EPM_ElimMechWithGraviton UMETA(DisplayName = "ElimMechWithGraviton"),
	EPM_ElimMechWithCyberPistol UMETA(DisplayName = "ElimMechWithCyberPistol"),
	EPM_ElimAiWithPistol UMETA(DisplayName = "ElimAiWithPistol"),
	EPM_ElimAiWithAR UMETA(DisplayName = "ElimAiWithAR"),
	EPM_ElimAiWithRushAttack UMETA(DisplayName = "ElimAiWithRushAttack"),
	EPM_ElimAiWithSMG UMETA(DisplayName = "ElimAiWithSMG"),
	EPM_ElimAiWithGrenadeLauncher UMETA(DisplayName = "ElimAiWithGrenadeLauncher"),
	EPM_AiHeadShots UMETA(DisplayName = "AiHeadShots"),
	EPM_ElimAiWhileInAir UMETA(DisplayName = "ElimAiWhileInAir"),

	// General tasks
	EPM_StunPlayer UMETA(DisplayName = "StunPlayer"),
	EPM_KdRatioAtLeast2 UMETA(DisplayName = "KdRatioAtLeast2"),
	EPM_KdRatioAtLeast3 UMETA(DisplayName = "KdRatioAtLeast3"),
	EPM_KdRatioAtLeast4 UMETA(DisplayName = "KdRatioAtLeast4"),
	EPM_DamageEfficiency75Percent UMETA(DisplayName = "DamageEfficiency75Percent"),
	EPM_DamageEfficiency50Percent UMETA(DisplayName = "DamageEfficiency50Percent"),
	EPM_DamageEfficiency35Percent UMETA(DisplayName = "DamageEfficiency35Percent"),
	EPM_AccuracyAtLeast30Percent UMETA(DisplayName = "AccuracyAtLeast30Percent"),
	EPM_PlayerHeadShots UMETA(DisplayName = "PlayerHeadShots"),
	EPM_DestroyHoverDrone UMETA(DisplayName = "DestroyHoverDrone"),
	EPM_TakeDownAttackDrone UMETA(DisplayName = "TakeDownAttackDrone"),
	EPM_NoDamageFromMapMissile UMETA(DisplayName = "NoDamageFromMapMissile"),
	EPM_DriveAttackDroneFor2Min UMETA(DisplayName = "DriveDroneFor2Min"),
	EPM_BlockShotsWithShield UMETA(DisplayName = "BlockShotsWithShield"),
	EPM_PlayFullEmote UMETA(DisplayName = "PlayFullEmote"),
	EPM_Get3KillStreak UMETA(DisplayName = "Get3KillStreak"),
	EPM_Get5KillStreak UMETA(DisplayName = "Get5KillStreak"),
	EPM_GrappleSwings UMETA(DisplayName = "GrappleSwings"),
	EPM_FinishTopTen UMETA(DisplayName = "FinishTopTen"),
	EPM_FinishTopThree UMETA(DisplayName = "FinishTopThree"),
	EPM_FinishFirstPlace UMETA(DisplayName = "FinishFirstPlace"),
	EPM_Deal1000Damage UMETA(DisplayName = "Deal1000Damage"),

    EPM_MAX UMETA(DisplayName = "MAX")
};