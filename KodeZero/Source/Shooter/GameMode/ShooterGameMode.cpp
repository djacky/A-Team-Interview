// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameMode.h"
#include "Shooter/ShooterCharacter.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/ShooterGameState.h"
#include "Shooter/ShooterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#include "Shooter/ShooterCrosshairHUD.h"
#include "Shooter/Widgets/CharacterOverlay.h"
#include "MissleSystem.h"
#include "Components/BoxComponent.h"
#include "Shooter/ShooterGameInstance.h"
#include "Json.h"
#include "Shooter/Helicopter.h"
#include "Shooter/Misc/Requests/RequestsObject.h"
#include "Shooter/Misc/GameSettings.h"


AShooterGameMode::AShooterGameMode()
{

}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
    InitializeCharSpawnLocations();
    //ShooterGS = GetWorld()->GetGameState<AShooterGameState>();
    //LevelStartingTime = GetWorld()->GetTimeSeconds();
    //UE_LOG(LogTemp, Display, TEXT("On GameMode BeginPlay")); 
    //GetWorldTimerManager().SetTimer(MinPlayersTimer, this, &AShooterGameMode::UpdateMinNumPlayers, 15.f);
}

void AShooterGameMode::Tick(float DeltaTime)
{
    CountdownTime = MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
    
    if (CountdownTime <= 0.f && !bMatchEnded && bStartMatchTimer)
    {
        bMatchEnded = true;
        AShooterGameState* ShooterGS = GetShooter_GS();
        //ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
        if (ShooterGS)
        {
            ShooterGS->EndMatch();
        }
    }
    if (!bBattleStarted)
    {
        WarningCountdownTime = WarningStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
    }
}

void AShooterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    // This is called before BeginPlay and Tick
    Super::InitGame(MapName, Options, ErrorMessage);
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI)
    {
        if (UGameSettings* Settings = ShooterGI->GetGameSettings_Implementation())
        {
            MatchTime = Settings->MatchTime;
        }
    }
}

void AShooterGameMode::UpdateMinNumPlayers()
{
    MinNumPlayersToStart = FMath::FloorToInt32(MinNumPlayersToStart / 2.f);
    if (MinNumPlayersToStart > 1)
    {
        GetWorldTimerManager().SetTimer(MinPlayersTimer, this, &AShooterGameMode::UpdateMinNumPlayers, 15.f);
    }
    else
    {
        MinNumPlayersToStart = 1;
    }
}

void AShooterGameMode::InitializeGameStateFuncs(bool bIsTeamMode)
{
    AShooterGameState* ShooterGS = GetShooter_GS();
    if (ShooterGS) ShooterGS->StartServerFuncs(bIsTeamMode);
}

void AShooterGameMode::StartMatchTimer()
{
    bStartMatchTimer = true;
}

void AShooterGameMode::PlayerEliminated(class AShooterCharacter* ElimmedCharacter,
    AShooterPlayerController* VictimController, AShooterPlayerController* ShooterAttackerController,
    AShooterPlayerState* VictimPlayerState, AShooterPlayerState* AttackerPlayerState)
{
    if (!VictimPlayerState) return;
    int32 RespawnsLeft = VictimPlayerState->UpdateRespawns();
    if (ElimmedCharacter)
    {
        ElimmedCharacter->Elim(AttackerPlayerState, RespawnsLeft);
    }
    
    if (AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>())
    {
        if (!AttackerPlayerState) return;
        if (VictimController)
        {
            VictimController->ClientElimAnnouncement(AttackerPlayerState, VictimPlayerState, false);
            if (ShooterAttackerController)
            {
                ShooterAttackerController->ClientUpdateHUDNumOfKills(AttackerPlayerState->PlayerGameStats, ShooterAttackerController->MainCharacter);
                if (ShooterAttackerController != VictimController) ShooterAttackerController->ClientElimAnnouncement(AttackerPlayerState, VictimPlayerState, false);
                if (AttackerPlayerState->PlayerGameStats.KillStreak == 3)
                {
                    AttackerPlayerState->AddToXPMetric(EProgressMetric::EPM_Get3KillStreak, 1);
                }
                else if (AttackerPlayerState->PlayerGameStats.KillStreak == 5)
                {
                    AttackerPlayerState->AddToXPMetric(EProgressMetric::EPM_Get5KillStreak, 1);
                }
            }
        }

        for (auto ShooterController : ShooterGameState->ShooterControllerArray)
        {
            if (ShooterController)
            {
                ShooterController->ClientElimAnnouncementSmall(AttackerPlayerState, VictimPlayerState, AttackerPlayerState->DamageType, false);
            }
        }
    }
    
    /*
    if (ShooterGS && VictimController && ShooterAttackerController)
    {
        for (auto ShooterController : ShooterGS->ShooterControllerArray)
        {
            //AShooterPlayerController* ShooterController = Cast<AShooterPlayerController>(PS->GetPlayerController());
            //if (ShooterController && ShooterController != ShooterAttackerController && ShooterController != VictimController)
            if (ShooterController && ShooterController != ShooterAttackerController && ShooterController != VictimController)
            {
                ShooterController->ClientElimAnnouncementSmall(AttackerPlayerState, VictimPlayerState, AttackerPlayerState->DamageType, false);
            }
        }
    }
    */

    /*
    // code used for team mode before
    if (ShooterGS && ShooterGS->bIsTeamMode && GetWorld())
    {
        ShooterGS = ShooterGS == nullptr ? GetWorld()->GetGameState<AShooterGameState>() : ShooterGS;
        if (ShooterGS)
        {
            TArray<AShooterPlayerState*> AliveTeam = ShooterGS->GetAliveTeamMembers(VictimPlayerState->GetGenericTeamId());
            if (AliveTeam.Num() > 0)
            {
                for (auto TeamMemberPS : AliveTeam)
                {
                    AShooterPlayerController* TeamMemberController = TeamMemberPS->GetShooterPlayerController();
                    if (TeamMemberController && TeamMemberController != VictimController)
                    {
                        TeamMemberController->ClientElimAnnouncement(AttackerPlayerState, VictimPlayerState, ShooterGS->bIsTeamMode);
                    }
                }
            }
        }
    }
    */

    /*
    // Show elim announcements to all players
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AShooterPlayerController* ShooterPlayer = Cast<AShooterPlayerController>(*It);
        if (ShooterPlayer && AttackerPlayerState && VictimPlayerState)
        {
            ShooterPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
        }
    }
    */
}

void AShooterGameMode::AddPlayerToList(AShooterCharacter* PlayerToAdd)
{
    PlayerList.Add(PlayerToAdd);
}


void AShooterGameMode::InitializeCharSpawnLocations()
{
    if (CharacterSpawnPointsClass)
    {
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), CharacterSpawnPointsClass, CharacterSpawnPoints);
        for (auto SpawnPoint : CharacterSpawnPoints)
        {
            //UE_LOG(LogTemp, Warning, TEXT("Adding Spawn Point: %s"), *SpawnPoint->GetName());
            if (SpawnPoint) SpawnLocationPoints.Add(SpawnPoint->GetActorLocation());
        }
    }
}

void AShooterGameMode::PostLogin(APlayerController* PlayerController)
{
    Super::PostLogin(PlayerController);

    NumLoggedInPlayers += 1;
    if (NumLoggedInPlayers == 1 && LevelStartingTime == 0.f && !bStartMatchTimer)
    {
        // Start the game timer
        LevelStartingTime = GetWorld()->GetTimeSeconds();
        bStartMatchTimer = true;

        FTimerHandle WarningStateTimer;
        GetWorldTimerManager().SetTimer(WarningStateTimer, this, &AShooterGameMode::SetWarningState, WarningStartTime);
        ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
        AShooterGameState* ShooterGS = GetShooter_GS();
        if (ShooterGI && ShooterGI->GameType != EGameModeType::EGMT_Lobby && !ShooterGI->bPracticeMode && ShooterGS) 
        {
            ShooterGS->StartServerFuncs(ShooterGI->bTeamMode);
        }
    }
    if (NumLoggedInPlayers > MaxLoggedInPlayers) MaxLoggedInPlayers = NumLoggedInPlayers;
    StartPlayerPossess(PlayerController);
}

void AShooterGameMode::StartPlayerPossess(APlayerController* PlayerController)
{
    AShooterGameState* ShooterGS = GetShooter_GS();
    auto ShooterController = Cast<AShooterPlayerController>(PlayerController);
    if (ShooterController && ShooterGS)
    {
        //ShooterGS->GameMatchState = EGameMatchState::EMS_Start;
        ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
        if (ShooterGI && (ShooterGI->bPracticeMode || ShooterGI->GameType == EGameModeType::EGMT_Lobby))
        {
            ShooterGS->GameMatchState = EGameMatchState::EMS_Start;
            ShooterController->OnMatchStateSet(EGameMatchState::EMS_Start);
        }
        else
        {
            //UE_LOG(LogTemp, Warning, TEXT("StartPlayerPossess: %i"), ShooterGS->GameMatchState);
            ShooterController->OnMatchStateSet(ShooterGS->GameMatchState);
        }
        
        ShooterController->ClientSetCharacter(MatchTime, LevelStartingTime, WarningStartTime);
        //PlayerController->HasClientLoadedCurrentWorld();
    }
}

void AShooterGameMode::Logout(AController* PController)
{
    Super::Logout(PController);

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    NumLoggedInPlayers -= 1;
    AShooterGameState* ShooterGS = GetShooter_GS();
    if (NumLoggedInPlayers <= 0 && ShooterGI && (ShooterGI->GameType != EGameModeType::EGMT_tournament && ShooterGI->GameType != EGameModeType::EGMT_tournamentTeam && ShooterGI->GameType != EGameModeType::EGMT_Lobby))
    {
        if (ShooterGS && !bMatchEnded)
        {
            ShooterGS->QuitServer(true, 5.f, true);
        }
    }
    if (ShooterGS && PController && PController->PlayerState)
    {
        if (AShooterPlayerState* LeavingPlayerState = Cast<AShooterPlayerState>(PController->PlayerState))
        {
            ShooterGS->ShooterPlayerArray.Remove(LeavingPlayerState);
        }
        if (AShooterPlayerController* LeavingController = Cast<AShooterPlayerController>(PController))
        {
            ShooterGS->ShooterControllerArray.Remove(LeavingController);
        }
    }
    OnLogout(PController);
}

void AShooterGameMode::SetWarningState()
{
    AShooterGameState* ShooterGS = GetShooter_GS();
    if (ShooterGS)
    {
        ShooterGS->SetWarningState();
    }
}

// This is called after a successful seamless travel in the NEW game mode. But not being used right now (because we are just traveling to the default server map)
void AShooterGameMode::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();
    
    UE_LOG(LogTemp, Display, TEXT("OnPostSeamlessTravel"));
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI)
    {
        if (!ShooterGI->ServerGameSessionId.IsEmpty())
        {
            UE_LOG(LogTemp, Warning, TEXT("GameMode GameSessionId: %s"), *ShooterGI->ServerGameSessionId);
            //ShooterGI->PostUpdateSession(ShooterGI->ServerGameSessionId, "addSessionId");
            ShooterGI->SeamlessTravelOk();
        }
    }
}

void AShooterGameMode::OnLogout(AController* PController)
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI && ShooterGI->bPracticeMode) return;

    if (PController)
    {
        auto LeavingShooterController = Cast<AShooterPlayerController>(PController);
        if (LeavingShooterController)
        {
            if (LeavingShooterController->MainCharacter) LeavingShooterController->MainCharacter->Destroy();
            LeavingShooterController->ClientStopVivox();
        }
    }
}

void AShooterGameMode::UpdateMissileHits()
{
    ++MissileHits;
    if (NumMissilesToPrespawn != 0 && MissileHits == NumMissilesToPrespawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateMissileHits Activating"));
        ActivateAWSSession();
    }
}

AShooterGameState* AShooterGameMode::GetShooter_GS()
{
    if (GetWorld())
    {
        AShooterGameState* ShooterGS = GetWorld()->GetGameState<AShooterGameState>();
        if (ShooterGS) return ShooterGS;
    }
	return nullptr;
}

void AShooterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
        FTransform RespawnTransform = GetUnhitMissileLocation();
        RestartPlayerAtTransform(ElimmedController, RespawnTransform);

        FTimerDelegate UpdateHUDDel;
        FTimerHandle UpdateHUDTimer;
        UpdateHUDDel.BindUFunction(this, FName("UpdateHUDParams"), ElimmedController);
        GetWorld()->GetTimerManager().SetTimer(UpdateHUDTimer, UpdateHUDDel, 0.5f, false);
	}
}

void AShooterGameMode::UpdateHUDParams(AController* ElimmedController)
{
    auto ShooterPC = Cast<AShooterPlayerController>(ElimmedController);
    if (ShooterPC)
    {
        ShooterPC->UpdateHUDonRespawn();
    }
    //AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
}

FTransform AShooterGameMode::GetUnhitMissileLocation()
{
    if (!CharacterSpawnPointsFinishedReporting || !MissileSystemPointsFinishedReporting)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSpawnPoints and/or MissileSystemPoints are still reporting, proceeding anyway.."));
    }

    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI && ShooterGI->bPracticeMode && CharacterSpawnPoints.Num() > 0)
    {
        int32 SpawnSelectionIndex = FMath::RandRange(0, CharacterSpawnPoints.Num() - 1);
        return CharacterSpawnPoints[SpawnSelectionIndex]->GetTransform();
    }

    if (CharacterSpawnPointsClass)
    {
        TArray<FTransform> SpawnPointTransforms;

        TArray<AActor*> CharacterSpawnPointsMissileOverlapped;

        if (MissileHitPoints.Num() > 0 && CharacterSpawnPoints.Num() > 0)
        {
            for (int32 k = 0; k < MissileHitPoints.Num(); k++)
            {
                auto Missile = Cast<AMissleSystem>(MissileHitPoints[k]);

                if (Missile && Missile->GetDamageBox())
                {
                    for (int32 i = 0; i < CharacterSpawnPoints.Num(); i++)
                    {
                        bool bRespawnInMissileBox = Missile->GetDamageBox()->Bounds.GetBox().IsInside(CharacterSpawnPoints[i]->GetActorLocation());
                        if (bRespawnInMissileBox)
                        {
                            CharacterSpawnPointsMissileOverlapped.Add(CharacterSpawnPoints[i]);
                        }
                    }
                }
            }
        }
        else if (MissileHitPoints.Num() == 0 && CharacterSpawnPoints.Num() > 0)
        {
            int32 SpawnSelectionIndex = FMath::RandRange(0, CharacterSpawnPoints.Num() - 1);
            return CharacterSpawnPoints[SpawnSelectionIndex]->GetTransform();
        }
        else
        {
            FTransform OutTransform;
            OutTransform.SetLocation(FVector{0.f, 0.f, 200.f});
            return OutTransform;

        }

        for (int32 i = 0; i < CharacterSpawnPoints.Num(); i++)
        {
            if (!CharacterSpawnPointsMissileOverlapped.Contains(CharacterSpawnPoints[i]))
            {
                SpawnPointTransforms.Add(CharacterSpawnPoints[i]->GetTransform());
            }
        }
        
        if (SpawnPointTransforms.Num() > 0)
        {
            int32 SpawnSelectionIndex = FMath::RandRange(0, SpawnPointTransforms.Num() - 1);
            return SpawnPointTransforms[SpawnSelectionIndex];
        }
        else
        {
            int32 SpawnSelectionIndex = FMath::RandRange(0, CharacterSpawnPoints.Num() - 1);
            return CharacterSpawnPoints[SpawnSelectionIndex]->GetTransform();
        }
    }
    return FTransform();
}

void AShooterGameMode::GetSessionNumPlayers(FString MatchmakerData)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(MatchmakerData);
    if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
    {
        // Access the "teams" array
        if (!JsonObject->HasField(TEXT("teams")))
        {
            UE_LOG(LogTemp, Warning, TEXT("MatchmakerData in GetSessionNumPlayers() does not have a teams field. Check the MatchmakerData string and see what fields it has."));
        }
        TArray<TSharedPtr<FJsonValue>> TeamsArray = JsonObject->GetArrayField(TEXT("teams"));
        if (TeamsArray.Num() > 0)
        {
            // Access the first team object
            TSharedPtr<FJsonObject> FirstTeamObject = TeamsArray[0]->AsObject();
            if (FirstTeamObject.IsValid())
            {
                // Access the "players" array within the first team object
                TArray<TSharedPtr<FJsonValue>> PlayersArray = FirstTeamObject->GetArrayField(TEXT("players"));
                OnGetNumSessionPlayers(PlayersArray.Num());
                //OnGetSessionPlayersCompleted.Broadcast(PlayersArray.Num());
            }
        }
    }
}