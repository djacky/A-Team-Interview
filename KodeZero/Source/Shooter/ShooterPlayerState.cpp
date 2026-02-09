// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterGameState.h"
#include "GameMode/ShooterGameMode.h"
#include "Helicopter.h"
#include "Shooter/ShooterGameInstance.h"
#include "Json.h"
#include "Shooter/Misc/Requests/RequestsObject.h"
#include "Globals.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "Shooter/Widgets/PauseMenu.h"
#include "GameFramework/GameSession.h"
#include "Shooter/StructTypes/TournamentStruct.h"
#include "Shooter/Misc/CharacterGlobalFuncs.h"
#include "Shooter/Misc/XPComboComponent.h"


AShooterPlayerState::AShooterPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MyPlayerConnectionType(EShooterPlayerConnectionType::Player)
{
	TeamID = FGenericTeamId::NoTeam;
	MySquadID = INDEX_NONE;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	XPComponent = CreateDefaultSubobject<UXPComboComponent>(TEXT("XP_Component"));
	XPComponent->SetIsReplicated(true);
	//XPComponent->SetIsReplicated(true);
}

void AShooterPlayerState::BeginPlay()
{
	Super::BeginPlay();

	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;

	if (HasAuthority())
	{
		if (ShooterGI && ShooterGI->bTeamMode)
		{
			if (GetOwningController() && GetOwningController()->IsLocalPlayerController())
			{
				PlayerLocationUpdateTime = 0.1f;
			}
			else
			{
				PlayerLocationUpdateTime = 0.75f;
				MinNetUpdateFrequency = 1 / PlayerLocationUpdateTime;
				NetUpdateFrequency = 2.f * MinNetUpdateFrequency;
			}
			StartLocationTimer();
		}
		
		if (!HasLocalNetOwner() && ShooterGI->GameType == EGameModeType::EGMT_Lobby)
		{
			StartStatInfoUpdate();
		}
	}
	else
	{
		if (ShooterGI && ShooterGI->bTeamMode && !ShooterGI->bPracticeMode && !IsABot())
		{
			if (!HasLocalNetOwner())
			{
				auto OtherShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
				if (OtherShooterPC)
				{
					SetTeamWidgets(OtherShooterPC);
				}
			}
		}
	}

	if (ShooterGI->IsWithEditor() && HasLocalNetOwner() && !IsABot())
	{
		const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
		ServerSetPlayerId(Ids.PlayerID);
	}

	#if WITH_CLIENT_ONLY
		if (ShooterGI && !ShooterGI->IsWithEditor() &&
			(GetNetMode() == NM_Client || GetNetMode() == NM_ListenServer) &&
			!IsPracticeMode() &&
			!IsABot() &&
			HasLocalNetOwner())
		{
			const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
			ShooterGI->VivoxInit();
			StartVivox3DUpdate();
			ServerSetPlayerId(Ids.PlayerID);
		}
	#endif

	if (!IsABot() && HasLocalNetOwner()) StartStream();
	SetHUDReference();
}

void AShooterPlayerState::SetHUDReference()
{
	AShooterPlayerController* PC = Cast<AShooterPlayerController>(GetPlayerController());
	if (PC && PC->IsLocalController())
	{
		if (XPComponent)
		{
			XPComponent->SetHUD(PC->ShooterHUD);
		}
	}
}

void AShooterPlayerState::StartStatInfoUpdate()
{
	if (!PlayerGameStats.ShooterMeshName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartStatInfoUpdate"));
		AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
		if (ShooterGameState) ShooterGameState->GetPings();
		MulticastPlayerStat(PlayerGameStats, true);
	}
	else
	{
		FTimerHandle StatUpdateTimer;
		GetWorldTimerManager().SetTimer(StatUpdateTimer, this, &AShooterPlayerState::StartStatInfoUpdate, 0.5f);
	}
}

void AShooterPlayerState::ServerSetPlayerId_Implementation(const FString &UserPlayerID)
{
	PlayerGameStats.PlayerId = UserPlayerID;
	if (ShooterGI && !IsPracticeMode())
	{
		//if (XPComponent) XPComponent->PlayerProgress = ShooterGI->CurrentPlayerProgress;
		switch (ShooterGI->GameType)
		{
		case EGameModeType::EGMT_tournament:
		case EGameModeType::EGMT_tournamentTeam:
		{
			if (ShooterGI->TournamentPlayerIds.Num() > 0)
			{
				if (!ShooterGI->TournamentPlayerIds.Contains(UserPlayerID))
				{
					UWorld* World = GetWorld();
					auto ShooterController = GetShooterPlayerController();
					if (World && ShooterController)
					{
						ShooterController->RemovePlayerSession();
						World->GetAuthGameMode()->GameSession->KickPlayer(ShooterController, FText::FromString(TEXT("You are not registered for this tournament")));
					}
				}
			}
		}
		default:
			break;
		}
	}
}

void AShooterPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (ShooterGI)
	{
		if (!HasLocalNetOwner() && ShooterGI->bTeamMode && !IsABot())
		{
			auto OtherShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
			if (OtherShooterPC)
			{
				OtherShooterPC->OnPlayerLeftGame(this);
			}
		}
        if (HasLocalNetOwner() && !IsABot() && ShooterGI->WebSocket && ShooterGI->WebSocket->IsConnected())
        {
            ShooterGI->WebSocket->Close();
        }
		if (HasAuthority())
		{
			if (ShooterGI->GameType == EGameModeType::EGMT_Lobby) UpdateLobbyIdArray(false, PlayerGameStats.PlayerId);
			MulticastPlayerStat(PlayerGameStats, false);
		}
	}
}

void AShooterPlayerState::ClientStartVoiceChat_Implementation()
{
	#if WITH_CLIENT_ONLY
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
		if (ShooterGI && !ShooterGI->IsWithEditor() && !IsPracticeMode())
		{
			ShooterGI->VivoxInit();
			StartVivox3DUpdate();
		}
	#endif
}

void AShooterPlayerState::StartVivox3DUpdate()
{
	#if WITH_CLIENT_ONLY
		if (ShooterGI && RootSceneComponent)
		{
			ShooterGI->Update3DPosition(GetPawn());
			FTimerHandle Update3DPositionTimer;
			GetWorldTimerManager().SetTimer(Update3DPositionTimer, this, &AShooterPlayerState::StartVivox3DUpdate, 0.2f);
		}
	#endif
}

void AShooterPlayerState::OnClientJoined()
{
	if (IsPracticeMode()) return;

	const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
	SetPlayerName(Ids.PlayerName);
	PlayerGameStats.PlayerName = Ids.PlayerName;
	ServerOnClientJoined(Ids);
}

void AShooterPlayerState::ServerOnClientJoined_Implementation(FPlayerID ClientPlayerIds)
{
	SetPlayerName(ClientPlayerIds.PlayerName);
	PlayerGameStats.PlayerName = ClientPlayerIds.PlayerName;
	if (GetWorld())
	{
		AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
		if (ShooterGameState)
		{
			ShooterGameState->NotifyPlayerJoined(this, GetPlayerName());
		}
	}

}

void AShooterPlayerState::ServerTriggerMap_Implementation(bool bTriggerMap)
{
	bMapTriggered = bTriggerMap;
	//bMapTriggered = true;
	if (bMapTriggered)
	{
		GetShooterLocation();
	}
}

// This is called in the PlayerController
void AShooterPlayerState::StartGetLocation()
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (!ShooterGI)
	{
		FTimerHandle InvalidTimer;
		GetWorldTimerManager().SetTimer(InvalidTimer, this, &AShooterPlayerState::StartGetLocation, 0.1f);
	}

	if (!ShooterGI->bTeamMode)
	{
		PlayerLocationUpdateTime = 0.1f;
		GetShooterLocation();
	}
	else if (ShooterGI->bTeamMode)
	{
		if (!HasAuthority())
		{
			PlayerLocationUpdateTime = 0.1f;
			GetShooterLocation();
		}
	}
}

void AShooterPlayerState::StartLocationTimer()
{
	FTimerHandle PlayerLocationTimer;
	GetWorldTimerManager().SetTimer(PlayerLocationTimer, this, &AShooterPlayerState::GetShooterLocation, PlayerLocationUpdateTime);
}

void AShooterPlayerState::GetShooterLocation()
{
	if (IsABot()) return;
    if (APawn* CurrentPawn = GetPawn())
    {
        if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(CurrentPawn))
        {
            PlayerLocation = ShooterInterface->Execute_GetPawnLocation(CurrentPawn);
			PlayerRotationYaw = ShooterInterface->Execute_GetPawnRotationYaw(CurrentPawn);
        }
    }
	StartLocationTimer();
}

void AShooterPlayerState::ClientGetShooterLocation_Implementation(FVector Loc, float YawValue)
{
	PlayerLocation = Loc;
	PlayerRotationYaw = YawValue;
}

void AShooterPlayerState::ClientSetVoiceVolume_Implementation(AShooterPlayerState* NewPlayerState)
{
	//if (NewPlayerState) SetVoiceVolume(NewPlayerState);
	if (NewPlayerState) SetVolumeOnNewClient(NewPlayerState);
}

void AShooterPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

}

void AShooterPlayerState::Reset()
{
	Super::Reset();
}

void AShooterPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
}

void AShooterPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

void AShooterPlayerState::OnDeactivated()
{
	bool bDestroyDeactivatedPlayerState = false;

	switch (GetPlayerConnectionType())
	{
	case EShooterPlayerConnectionType::Player:
	case EShooterPlayerConnectionType::InactivePlayer:
		//@TODO: Ask the experience if we should destroy disconnecting players immediately or leave them around
		// (e.g., for long running servers where they might build up if lots of players cycle through)
		bDestroyDeactivatedPlayerState = true;
		break;
	default:
		bDestroyDeactivatedPlayerState = true;
		break;
	}

	SetPlayerConnectionType(EShooterPlayerConnectionType::InactivePlayer);

	if (bDestroyDeactivatedPlayerState)
	{
		Destroy();
	}
}

void AShooterPlayerState::OnReactivated()
{
	if (GetPlayerConnectionType() == EShooterPlayerConnectionType::InactivePlayer)
	{
		SetPlayerConnectionType(EShooterPlayerConnectionType::Player);
	}
}

void AShooterPlayerState::SetAIProperties(const EEnemyType& InAIType)
{
	bIsAI = true;
	EnemyReplicationData.EnemyType = InAIType;

	FEnemyData* EnemyRow = UCharacterGlobalFuncs::GetEnemyDataRow(EnemyReplicationData.EnemyType);
	if (EnemyRow)
	{
		int32 MeshVariantIndex = FMath::RandRange(0, EnemyRow->MeshVariants.Num() - 1);
		PlayerGameStats.Attributes.IconImage = InAIType == EEnemyType::EET_Default ? 
			AIIconImages[FMath::RandRange(0, AIIconImages.Num() - 1)] : EnemyRow->MeshVariants[MeshVariantIndex].IconImage;
		EnemyReplicationData.SkeletalMesh = EnemyRow->MeshVariants[MeshVariantIndex].SkeletalMesh;
	}

	if (GetWorld())
	{
		AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>();
		if (ShooterGameState)
		{
			int32 RandomNameIndex = FMath::RandRange(0, ShooterGameState->AINameArray.Num() - 1);
			if (ShooterGameState->AINameArray.IsValidIndex(RandomNameIndex))
			{
				SetPlayerName(ShooterGameState->AINameArray[RandomNameIndex]);
				ShooterGameState->AINameArray.RemoveAt(RandomNameIndex);
			}
			else
			{
				SetPlayerName(ShooterGameState->AINameArray[FMath::RandRange(0, ShooterGameState->AINameArray.Num() - 1)]);
			}
		}
	}
	PlayerGameStats.PlayerName = GetPlayerName();
	PlayerGameStats.ShooterMeshName = PlayerGameStats.PlayerName;
}

EEnemyType AShooterPlayerState::GetRandomEnemyTypeExcludingDefaults()
{
    UEnum* EnumPtr = StaticEnum<EEnemyType>();
    if (!EnumPtr) return EEnemyType::EET_Default; // fallback

    TArray<EEnemyType> ValidValues;

    // Iterate over enum entries, excluding EET_Default and EET_MAX
    for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i) // -1 to skip EET_MAX
    {
        EEnemyType Value = static_cast<EEnemyType>(EnumPtr->GetValueByIndex(i));
        
        if (Value != EEnemyType::EET_Default)
        {
            ValidValues.Add(Value);
        }
    }

    if (ValidValues.Num() > 0)
    {
        return ValidValues[FMath::RandRange(0, ValidValues.Num() - 1)];
    }

    return EEnemyType::EET_Default; // fallback
}

AShooterPlayerController* AShooterPlayerState::GetShooterPlayerController() const
{
	return Cast<AShooterPlayerController>(GetOwner());
}

void AShooterPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
    if (IsABot()) bIsAI = true;
}

void AShooterPlayerState::SetPlayerConnectionType(EShooterPlayerConnectionType NewType)
{
	//MARK_PROPERTY_DIRTY_FROM_NAME(GetClass(), MyPlayerConnectionType, this);
	MyPlayerConnectionType = NewType;
}

void AShooterPlayerState::SetSquadID(int32 NewSquadId)
{
	if (HasAuthority())
	{
		//MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);

		MySquadID = NewSquadId;
	}
}

int32 AShooterPlayerState::UpdateRespawns()
{
	RemainingSpawns -= 1;
	bHasBeenElimmed = true;
	if (RemainingSpawns == 0)
	{
		bPlayerIsDead = true;
	}
	return RemainingSpawns;
}

void AShooterPlayerState::BPSetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	SetGenericTeamId(NewTeamID);
}

void AShooterPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (HasAuthority())
	{
		const FGenericTeamId OldTeamID = TeamID;

		//MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, TeamID, this);
		TeamID = NewTeamID;
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
	}
}

FGenericTeamId AShooterPlayerState::GetGenericTeamId() const
{
	return TeamID;
}

FOnShooterTeamIndexChangedDelegate* AShooterPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AShooterPlayerState::OnRep_TeamID(FGenericTeamId OldTeamID)
{
	ConditionalBroadcastTeamChanged(this, OldTeamID, TeamID);
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	
	if (ShooterGI && ShooterGI->bTeamMode && !IsABot())
	{
		bGotTeamID = true;
		auto ShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
		if (ShooterPC)
		{
			ShooterPC->BroadcastTeamID();
		}
	}
}

void AShooterPlayerState::OnRep_MySquadID()
{
	//@TODO: Let the squad subsystem know (once that exists)
}

void AShooterPlayerState::SetTeamWidgets(AShooterPlayerController* TeamMemberController)
{
	if (TeamMemberController == nullptr) return;

	auto TeamMemberPS = Cast<AShooterPlayerState>(TeamMemberController->PlayerState);
	if (TeamMemberPS && TeamMemberController->ShooterHUD && TeamMemberController->GetMatchStarted() && 
		TeamMemberPS->bGotTeamID && this->bGotTeamID && !this->ShooterMeshName.IsEmpty())
	{
		if (TeamMemberPS->GetGenericTeamId() == this->GetGenericTeamId())
		{
			TeamMemberController->AddTeamMemberInContainer(this);
		}
	}
	else
	{
		FTimerDelegate TeamWidgetsDel;
		FTimerHandle TeamWidgetsTimer;
		TeamWidgetsDel.BindUFunction(this, FName("SetTeamWidgets"), TeamMemberController);
		GetWorld()->GetTimerManager().SetTimer(TeamWidgetsTimer, TeamWidgetsDel, 1.f, false);
	}
}

void AShooterPlayerState::OnRep_CharacterHealth()
{
	UpdateAttributeValues(CharacterHealth, false);
}

void AShooterPlayerState::OnRep_CharacterShield()
{
	UpdateAttributeValues(CharacterShield, true);
}

// Value here is Value/MaxValue (like Health/MaxHealth)
void AShooterPlayerState::UpdateAttributeValues(float Value, bool bIsCharShield)
{
	LocalPlayerController = LocalPlayerController == nullptr ? Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController()) : LocalPlayerController;
	//AShooterPlayerController* LocalPlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (LocalPlayerController && this) LocalPlayerController->SetTeamMemberAttributes(this, Value, bIsCharShield);
}

bool AShooterPlayerState::IsPracticeMode()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
    if (ShooterGI)
    {
        return ShooterGI->bPracticeMode;
    }
    return false;
}

void AShooterPlayerState::StartStream()
{
	if (GetPlayerName().Equals(TEXT("ackyshacky")))
	{
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
		if (ShooterGI)
		{
			ShooterGI->WebSocketConnection_Implementation(true);
			return;
		}
	}
}

void AShooterPlayerState::UpdateLobbyIdArray(bool bIsReady, const FString& ID)
{
	if (AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>())
	{
		ShooterGameState->UpdateLobbyIdArray(bIsReady, ID);
	}
}

void AShooterPlayerState::ServerOnLobbyReady_Implementation(bool bIsReady, const FString& ID, const FString& Name)
{
	UpdateLobbyIdArray(bIsReady, ID);
}

void AShooterPlayerState::StartLobbyMatch(const FString& Region)
{
	if (AShooterGameState* ShooterGameState = GetWorld()->GetGameState<AShooterGameState>())
	{
		ShooterGameState->StartLobbyMatch(Region);
	}
}
void AShooterPlayerState::MulticastPlayerStat_Implementation(const FPlayerGameStats& UserStat, bool bAddingPlayer)
{
	UWorld* World = GetWorld();
	if (World)
	{
		auto LocalShooterPC = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
		if (LocalShooterPC && LocalShooterPC->PauseMenuWidget)
		{
			bAddingPlayer ? 
				LocalShooterPC->PauseMenuWidget->AddPlayerStatWidget(UserStat) : 
				LocalShooterPC->PauseMenuWidget->RemovePlayerStatWidget(UserStat);
		}
	}
}

void AShooterPlayerState::OnEnterCombat()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float CurrentTime = World->GetTimeSeconds();

	// If not already in combat, start a new combat window
	if (!bInCombat)
	{
		bInCombat = true;
		CombatStartTime = CurrentTime;
	}

	// Reset or extend the timer each time combat happens
	World->GetTimerManager().ClearTimer(CombatEndTimerHandle);
	World->GetTimerManager().SetTimer(
		CombatEndTimerHandle,
		this,
		&AShooterPlayerState::HandleCombatEnd,
		3.f,
		false // One-shot, non-looping
	);
}

void AShooterPlayerState::HandleCombatEnd()
{
	if (!bInCombat) return;

	UWorld* World = GetWorld();
	if (!World) return;

	const float CombatEndTime = World->GetTimeSeconds();
	const float Duration = CombatEndTime - CombatStartTime;

	PlayerGameStats.TimeInCombat += Duration;
	bInCombat = false;
}

void AShooterPlayerState::RegisterKill(AShooterPlayerState* VictimPS)
{
    PlayerGameStats.NumOfKills += 1;
    PlayerGameStats.KillStreak++;
	//KillsThisMinute++;
}

void AShooterPlayerState::RegisterDeath()
{
    PlayerGameStats.NumOfDeaths += 1;
	PlayerGameStats.KillStreak = 0;
	//DeathsThisMinute++;
}

void AShooterPlayerState::AddToXPMetric(EProgressMetric Metric, int32 Delta)
{
	if (XPComponent)
	{
		XPComponent->AddToMetric(Metric, Delta);
	}
}

void AShooterPlayerState::SetAbilityStatus(const FName& AbilityName, bool IsActive)
{
	if (XPComponent)
	{
		if (IsActive)
		{
			XPComponent->StartAbility(AbilityName);
		}
		else
		{
			XPComponent->EndAbility(AbilityName);
		}
	}
}

TSharedPtr<FJsonObject> AShooterPlayerState::SerializeProgressToJson()
{
	if (XPComponent)
	{
		FPlayerProgress FinalProgress = XPComponent->PlayerProgress;
		if (TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject()))
		{
			if (TSharedPtr<FJsonObject> MetricsJson = MakeShareable(new FJsonObject()))
			{
				for (const auto& Metric : FinalProgress.CurrentMetrics)
				{
					// Use enum name as key (e.g., "StyleXP") for easy backend parsing
					if (const UEnum* EnumPtr = StaticEnum<EProgressMetric>())
					{
						FString MetricName = EnumPtr->GetNameStringByValue(static_cast<int64>(Metric.Type));
						MetricsJson->SetNumberField(MetricName, Metric.Value);
					}
				}

				JsonObj->SetObjectField(TEXT("metrics"), MetricsJson);
				JsonObj->SetNumberField(TEXT("currentLevel"), FinalProgress.CurrentLevel);
				//JsonObj->SetNumberField(TEXT("matchXP"), PlayerProgress.MatchXP);

				return JsonObj;
			}
		}
	}
    return TSharedPtr<FJsonObject>();
}

/*
void AShooterPlayerState::OnMinuteTick(int32 MinuteMark)
{
    // Store current data
    FStatPoint Point;
    Point.MinuteMark = MinuteMark;
    Point.Kills = KillsThisMinute;
    Point.Deaths = DeathsThisMinute;

    TimeSeriesStats.Add(Point);

    // Reset for next minute
    KillsThisMinute = 0;
    DeathsThisMinute = 0;
    CurrentMinute = MinuteMark;
}
*/

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyPlayerConnectionType, SharedParams)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);
	//DOREPLIFETIME(ThisClass, PlayerGameStats);
	DOREPLIFETIME(ThisClass, bPlayerIsDead);
	DOREPLIFETIME_CONDITION(ThisClass, PlayerLocation, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, PlayerRotationYaw, COND_SkipOwner);
	DOREPLIFETIME(ThisClass, bMapTriggered);
	DOREPLIFETIME(ThisClass, bIsAI);
	DOREPLIFETIME(ThisClass, ShooterMeshName);
	DOREPLIFETIME(ThisClass, CharacterHealth);
	DOREPLIFETIME(ThisClass, CharacterShield);
}

/*
void AShooterPlayerState::ClientPostStats_Implementation(AShooterPlayerState* ServerPlayerState, FPlayerStatistics ServerStats)
{
	ServerPostStats(ServerPlayerState, ServerStats, GetPlayerName());
}

void AShooterPlayerState::ServerPostStats_Implementation(AShooterPlayerState* ServerPlayerState, FPlayerStatistics ServerStats, const FString& ClientPlayerName)
{
	PostPlayerStats(ServerStats, ServerPlayerState, ClientPlayerName);
}

void AShooterPlayerState::ClientPostWinner_Implementation(FPlayerGameStats InPlayerStats)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	if (ShooterGI && !ShooterGI->PlayerInfo.FullName.IsEmpty())
	{
		ServerPostWinner(ShooterGI->PlayerInfo, InPlayerStats);
	}
}

void AShooterPlayerState::ServerPostWinner_Implementation(FPlayerInfo InPlayerInfo, FPlayerGameStats InPlayerStats)
{
	PostWinningPlayerSolo(InPlayerInfo, InPlayerStats);
}

void AShooterPlayerState::PostWinningPlayerSolo(FPlayerInfo InPlayerInfo, FPlayerGameStats InPlayerStats)
{
	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(GetGameInstance()) : ShooterGI;
	ShooterGameMode = ShooterGameMode == nullptr ? Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)) : ShooterGameMode;
	if (ShooterGI && ShooterGameMode)
	{
		FString FileContent = ShooterGI->ReadFileAsString("_Game/MiscFiles/Keys/PrequestKey.txt", false);
		TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
		RequestObj->SetStringField("password", FileContent);
		RequestObj->SetStringField("gameSessionId", ShooterGI->ServerGameSessionId);
		RequestObj->SetStringField("epicGamesName", GetPlayerName());
		RequestObj->SetStringField("twitterName", InPlayerInfo.FullName);
		RequestObj->SetStringField("email", InPlayerInfo.Email);
		RequestObj->SetStringField("walletType", InPlayerInfo.WalletType);
		RequestObj->SetStringField("walletAddress", InPlayerInfo.WalletAddress);
		RequestObj->SetNumberField("numOfKills", InPlayerStats.NumOfKills);
		RequestObj->SetNumberField("damageRatio", (InPlayerStats.DamageDealt / (InPlayerStats.DamageDealt + InPlayerStats.DamageTaken)) * 100.f);
		RequestObj->SetNumberField("maxSessionPlayers", ShooterGameMode->MaxLoggedInPlayers);
		//RequestObj->SetNumberField("numKills", UpdatedPlayerStates.NumKills);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		FJsonSerializer::Serialize(RequestObj, Writer);

		Request->OnProcessRequestComplete().BindUObject(this, &AShooterPlayerState::OnWinningPlayerSoloPostResponseReceived);
		Request->SetURL("https://www.crypto-clash.org/_functions/soloWinners");
		Request->SetVerb("POST");
		Request->SetHeader("Content-Type", "application/json");
		Request->SetContentAsString(RequestBody);
		Request->ProcessRequest();
	}
}

void AShooterPlayerState::OnWinningPlayerSoloPostResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);

	//UE_LOG(LogTemp, Warning, TEXT("POST Response %s"), *Response->GetContentAsString());
}
*/


