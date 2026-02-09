// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Shooter/MenuMusicManager.h"
#include "Shooter/ShooterGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "Shooter/Misc/Requests/RequestsObject.h"
#include "../Globals.h"

void AMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();

    bEnableClickEvents = true;
    InitializeMenu();
    //UE_LOG(LogTemp, Warning, TEXT("Player Name = %s"), *PlayerState->GetPlayerName());
}

void AMenuPlayerController::InitializeMenu()
{
    if (MusicClass)
    {
        MenuMusicManager = GetWorld()->SpawnActor<AMenuMusicManager>(MusicClass);
    }

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
    if (ShooterGI)
    {
        bool bSaveGameSuccess = false;
        ShooterGI->LoadGame(SaveGame, bSaveGameSuccess);
        if (SaveGame && SaveGame->bMusicMainMenu)
        {
            MenuMusicManager->PlayNextSong();
        }
    }
}

int32 AMenuPlayerController::GetLumen()
{
	int32 Value = -1;
	if (GConfig && GConfig->GetInt(
		TEXT("/Script/Engine.RendererSettings"), 
		TEXT("r.ReflectionMethod"), 
		Value, 
		GEngineIni))
    {
        return Value;
    }
    return Value;
}

void AMenuPlayerController::SetLumen(bool bIsLumen)
{
	if (GetLumen() != -1)
	{
		// Write the modified value back to the config file
        if (bIsLumen)
        {
            GConfig->SetInt(
            TEXT("/Script/Engine.RendererSettings"),
            TEXT("r.ReflectionMethod"), 
            1, 
            GEngineIni);
        }
        else
        {
            GConfig->SetInt(
            TEXT("/Script/Engine.RendererSettings"),
            TEXT("r.ReflectionMethod"), 
            2, 
            GEngineIni);
        }

		GConfig->Flush(false, GEngineIni);
	}
}

FString AMenuPlayerController::GetConfigStrParam(FString Section, FString Key)
{
	FString Value = TEXT("");
	if (GConfig && GConfig->GetString(
		*Section, 
		*Key, 
		Value, 
		GEngineIni))
    {
        return Value;
    }
    return Value;
}

void AMenuPlayerController::SetConfigStrParam(bool bIsTrue, FString Section, FString Key)
{
	if (!GetConfigStrParam(Section, Key).IsEmpty())
	{
		// Write the modified value back to the config file
        if (bIsTrue)
        {
            FString TrueStr = TEXT("True");
            GConfig->SetString(
            *Section,
            *Key, 
            *TrueStr, 
            GEngineIni);
        }
        else
        {
            FString FalseStr = TEXT("False");
            GConfig->SetString(
            *Section,
            *Key, 
            *FalseStr, 
            GEngineIni);
        }

		GConfig->Flush(false, GEngineIni);
	}
}

void AMenuPlayerController::StartJoinProcess(const FString& JoinSecret)
{
    LobbyServerInfo.GameSessionId = JoinSecret;
    CreateSessionFromJoin();
}

void AMenuPlayerController::CreateSessionFromJoin()
{
	FTournamentParams TourParams;
	IGeneralInterface::Execute_UpdateTournamentParams(UGameplayStatics::GetGameInstance(this), TourParams);

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI == nullptr) return;

    const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
    HTTPRequestObj = NewObject<URequestsObject>(this);
    if (HTTPRequestObj)
    {
		if (!HTTPRequestObj->OnRequestResponseDelegate.IsBound())
		{
			HTTPRequestObj->OnRequestResponseDelegate.AddDynamic(this, &AMenuPlayerController::OnCreateSessionFromJoin);
		}
        FKeyValue Params;

        UE_LOG(LogTemp, Warning, TEXT("StartGettingServerInfoLobby GameSessionId = %s"), *LobbyServerInfo.GameSessionId);
        Params.SetStringField(TEXT("type"), TEXT("free"));
        Params.SetStringField(TEXT("tournamentId"), TEXT(""));
        Params.SetStringField(TEXT("region"), LobbyServerInfo.Region);
        Params.SetStringField(TEXT("playerName"), Ids.LocalGameLiftID);
        Params.SetStringField(TEXT("gameSessionId"), LobbyServerInfo.GameSessionId);
        Params.SetStringField(TEXT("teamGUID"), LobbyServerInfo.TeamGUID);
        Params.SetIntegerField(TEXT("numLobbyPlayers"), 1);
        Params.SetBoolField(TEXT("isFromLobby"), true);
        Params.AddHeader(TEXT("clientToken"), Ids.GameAccessJWT);
        HTTPRequestObj->MakeRequest(Params, TEXT("gamelift/createSession"), ERequestType::ERT_POST, EURLType::EUT_Main);
    }
}

void AMenuPlayerController::OnCreateSessionFromJoin(FString ResponseStr, FString ResponseURL)
{
    if (ResponseURL.Contains(TEXT("gamelift/createSession"), ESearchCase::IgnoreCase))
    {
        ParseLobbyMatch(ResponseStr);
    }
}

void AMenuPlayerController::ParseLobbyMatch(FString DataStr)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (ShooterGI == nullptr) return;
    FLobbyInfo NewLobbyInfo;
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DataStr);

    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("result")))
        {
            TSharedPtr<FJsonObject> ResultData = JsonObject->GetObjectField(TEXT("result"));
            if (ResultData->HasField(TEXT("serverInfo")))
            {
                TSharedPtr<FJsonObject> ServerData = ResultData->GetObjectField(TEXT("serverInfo"));
                if (ServerData->HasField(TEXT("status")))
                {
                    FString ServerStatus = ServerData->GetStringField(TEXT("status"));
					//if (HasAuthority()) MulticastUpdateTravelStatus(ServerStatus);

                    if (ServerStatus.Equals("Completed"))
                    {
						FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
						ShooterGI->TargetGameSessionId = ServerData->GetStringField(TEXT("gameSessionId"));
						Ids.PlayerSessionID = ServerData->GetStringField(TEXT("playerSessionId"));
						IGeneralInterface::Execute_UpdatePlayerIds(UGameplayStatics::GetGameInstance(this), Ids);
						
                        LobbyServerInfo.IpAddress = ServerData->GetStringField(TEXT("ip"));
                        LobbyServerInfo.Port = ServerData->GetIntegerField(TEXT("port"));
                        LobbyServerInfo.GameSessionId = ServerData->GetStringField(TEXT("gameSessionId"));
                        FString Mode = ServerData->GetStringField(TEXT("mode"));
                        if (Mode.Equals(TEXT("solo")))
                        {
                            ShooterGI->SelectedGameType = EGameModeType::EGMT_freeSolo;
                            LobbyServerInfo.GameType = EGameModeType::EGMT_freeSolo;
                            ShooterGI->bTeamMode = false;
                        }
                        else if (Mode.Equals(TEXT("team")))
                        {
                            ShooterGI->SelectedGameType = EGameModeType::EGMT_freeTeam;
                            LobbyServerInfo.GameType = EGameModeType::EGMT_freeTeam;
                            ShooterGI->bTeamMode = true;
                        }
                        JoinGame();
                    }
                    else if (ServerStatus.Equals("Failed"))
                    {
						LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Creating game session failed")));
                    }
                }
                else
                {
                    /*
                    if (HasAuthority())
                    {
                        bStartingMatchFromLobby = false;
                        MulticastUpdateTravelStatus(TEXT("Failed"));
                    }
                    */
                }
            }
        }
    }
}

void AMenuPlayerController::JoinGame()
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (ShooterGI)
	{
		ShooterGI->SetNetDriver(false);
		FString TravelCommand = FString::Printf(TEXT("open %s:%i"), *LobbyServerInfo.IpAddress, LobbyServerInfo.Port);
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Joining game: %s"), *TravelCommand));

		ShooterGI->LobbyServerInfo = LobbyServerInfo;
		ShooterGI->GameType = LobbyServerInfo.GameType;
		ConsoleCommand(TravelCommand);
	}
}

