#pragma once

#include "CoreMinimal.h"
#include "Shooter/EnumTypes/GameModeType.h"
#include "LobbyStruct.generated.h"

USTRUCT(BlueprintType)
struct FLobbyInfo
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString IpAddress = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 Port = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString GameSessionId = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString Region = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString TeamGUID = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	EGameModeType GameType = EGameModeType::EGMT_Lobby;

	void setTeamGUID(const EGameModeType& Mode)
	{
		FGuid Guid = FGuid::NewGuid();
        switch (Mode)
        {
        case EGameModeType::EGMT_freeTeam:
			TeamGUID = Guid.ToString();
            break;
        
        default:
            break;
        }
	}
};