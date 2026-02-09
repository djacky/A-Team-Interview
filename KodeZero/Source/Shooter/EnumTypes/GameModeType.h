#pragma once

UENUM(BlueprintType)
enum class EGameModeType : uint8
{
	EGMT_test UMETA(DisplayName = "Test"),
	EGMT_practice UMETA(DisplayName = "Practice"),
	EGMT_freeSolo UMETA(DisplayName = "FreeSolo"),
	EGMT_freeTeam UMETA(DisplayName = "FreeTeam"),
	EGMT_Lobby UMETA(DisplayName = "Lobby"),
	EGMT_tournament UMETA(DisplayName = "Tournament"),
	EGMT_tournamentTeam UMETA(DisplayName = "TournamentTeam"),

	EGMT_MAX UMETA(DisplayName = "DefaultMAX")
};