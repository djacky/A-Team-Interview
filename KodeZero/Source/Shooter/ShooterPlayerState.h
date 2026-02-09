// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterTeamAgentInterface.h"
#include "Http.h"
#include "Shooter/EnumTypes/ShooterDamageType.h"
#include "Shooter/EnumTypes/ProgressMetric.h"
#include "Shooter/StructTypes/StatsStruct.h"
#include "Shooter/StructTypes/EnemyTypeStruct.h"
#include "ShooterPlayerState.generated.h"


class AShooterPlayerController;

/** Defines the types of client connected */
UENUM()
enum class EShooterPlayerConnectionType : uint8
{
	// An active player
	Player = 0,

	// Spectator connected to a running game
	LiveSpectator,

	// Spectating a demo recording offline
	ReplaySpectator,

	// A deactivated player (disconnected)
	InactivePlayer
};

/**
 * AShooterPlayerState
 *
 *	Base player state class used by this project.
 */



UCLASS(Config = Game)
class SHOOTER_API AShooterPlayerState : public APlayerState, public IShooterTeamAgentInterface
{
	GENERATED_BODY()

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice Chat", meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootSceneComponent;

protected:
	virtual void BeginPlay() override;
	void StartLocationTimer();
	void GetShooterLocation();

	UFUNCTION(Client, Unreliable)
	void ClientGetShooterLocation(FVector Loc, float YawValue);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void StartStream();

	UFUNCTION()
	void SetHUDReference();

	//virtual void Tick(float DeltaTime) override;

public:
	//FORCEINLINE UEmbeddedVoiceChatGroup* GetEmbeddedVoiceChatGroup() const { return ChatGroupComponent; }
	//FORCEINLINE UAudioComponent* GetVoiceChatAudioComponent() const { return VoiceChatAudioComponent; }

	AShooterPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Shooter|PlayerState")
	AShooterPlayerController* GetShooterPlayerController() const;

	UPROPERTY()
	AShooterPlayerController* LocalPlayerController;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

	//~INLTeamAgentInterface interface
	UFUNCTION(BlueprintCallable)
	void BPSetGenericTeamId(const FGenericTeamId& NewTeamID);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	UFUNCTION(BlueprintCallable)
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnShooterTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of INLTeamAgentInterface interface

	void SetPlayerConnectionType(EShooterPlayerConnectionType NewType);
	EShooterPlayerConnectionType GetPlayerConnectionType() const { return MyPlayerConnectionType; }

	/** Returns the Squad ID of the squad the player belongs to. */
	UFUNCTION(BlueprintCallable)
		int32 GetSquadId() const
	{
		return MySquadID;
	}

	/** Returns the Team ID of the team the player belongs to. */
	UFUNCTION(BlueprintCallable)
		int32 GetTeamId() const
	{
		return GenericTeamIdToInteger(TeamID);
	}

	void SetSquadID(int32 NewSquadID);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bPlayerIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FVector PlayerLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float PlayerRotationYaw;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	int32 RemainingSpawns = 1000;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Stats")
	FPlayerGameStats PlayerGameStats;

	int32 UpdateRespawns();

	UPROPERTY(Replicated)
	bool bMapTriggered = false;

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ServerTriggerMap(bool bTriggerMap);

	UFUNCTION(Server, Reliable)
	void ServerSetPlayerId(const FString &UserPlayerID);

	bool bHasBeenElimmed = false;

	bool bGotTeamID = false;

	//bool bMatchStarted = false;

	UPROPERTY()
	FString ServerPlayerName;

	UPROPERTY(Replicated)
	FString ShooterMeshName;

	UPROPERTY()
	float CharacterMaxHealth = 100.f;

	UPROPERTY()
	float CharacterMaxShield = 100.f;

	UPROPERTY(ReplicatedUsing=OnRep_CharacterHealth)
	float CharacterHealth = -1.f;

	UFUNCTION()
	void OnRep_CharacterHealth();

	UPROPERTY(ReplicatedUsing=OnRep_CharacterShield)
	float CharacterShield = -1.f;

	UFUNCTION()
	void OnRep_CharacterShield();

	void UpdateAttributeValues(float Value, bool bIsCharShield);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UXPComboComponent* XPComponent;

	void AddToXPMetric(EProgressMetric Metric, int32 Delta);

	TSharedPtr<FJsonObject> SerializeProgressToJson();

	void SetAbilityStatus(const FName& AbilityName, bool IsActive);

	// Stat trackers
	bool bHitByMapMissile = false;
	int32 HandShieldHits = 0;
	int32 PlayedEmotes = 0;
	int32 NumGrapples = 0;

	//void PostWinningPlayerSolo(FPlayerInfo InPlayerInfo, FPlayerGameStats InPlayerStats);
	//void OnWinningPlayerSoloPostResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	//UFUNCTION(Client, Reliable)
	//void ClientPostWinner(FPlayerGameStats InPlayerStats);

	//UFUNCTION(Server, Reliable)
	//void ServerPostWinner(FPlayerInfo InPlayerInfo, FPlayerGameStats InPlayerStats);

	//UFUNCTION(Client, Reliable)
	//void ClientPostStats(AShooterPlayerState* ServerPlayerState, FPlayerStatistics ServerStats);

	//UFUNCTION(Server, Reliable)
	//void ServerPostStats(AShooterPlayerState* ServerPlayerState, FPlayerStatistics ServerStats, const FString& ClientPlayerName);

	bool bOldStats = false;

	UPROPERTY()
	TArray<AShooterPlayerController*> DeadPlayerControllers;

	UFUNCTION(BlueprintCallable)
	void StartGetLocation();

	UFUNCTION(BlueprintCallable)
	void SetAIProperties(const EEnemyType& InAIType);
	EEnemyType GetRandomEnemyTypeExcludingDefaults();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "AI", meta = (AllowPrivateAccess = "true"))
	bool bIsAI = false;

	UFUNCTION(BlueprintImplementableEvent)
	void StartVoiceChat();

	UFUNCTION(BlueprintImplementableEvent)
	void SetVolumeOnNewClient(AShooterPlayerState* NewPlayerState);

	UFUNCTION(Client, Reliable)
	void ClientSetVoiceVolume(AShooterPlayerState* NewPlayerState);

	void OnClientJoined();

	UFUNCTION(Server, Reliable)
	void ServerOnClientJoined(FPlayerID ClientPlayerIds);

	UFUNCTION(Client, Reliable)
	void ClientStartVoiceChat();

	UPROPERTY()
	EShooterDamageType DamageType = EShooterDamageType::ESDT_Gun;

	UFUNCTION(Server, Reliable)
	void ServerOnLobbyReady(bool bIsReady, const FString& ID, const FString& Name);

	void StartLobbyMatch(const FString& Region);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayerStat(const FPlayerGameStats& UserStat, bool bAddingPlayer);

	UFUNCTION()
	void StartStatInfoUpdate();

	void UpdateLobbyIdArray(bool bIsReady, const FString& ID);

	// Used with the AICharacherBase class
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FEnemyReplicationData EnemyReplicationData;

	void OnEnterCombat();
	void RegisterKill(AShooterPlayerState* VictimPS);
	void RegisterDeath();

private:

	UFUNCTION()
	void HandleCombatEnd();
	float CombatStartTime = 0.0f;
	bool bInCombat = false;
	int32 KillStreak = 0;

	FTimerHandle CombatEndTimerHandle;

	UPROPERTY(Replicated)
	EShooterPlayerConnectionType MyPlayerConnectionType;

	UPROPERTY()
	FOnShooterTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY(ReplicatedUsing = OnRep_TeamID)
	FGenericTeamId TeamID;

	UPROPERTY(ReplicatedUsing = OnRep_MySquadID)
	int32 MySquadID;

	UPROPERTY()
	class AShooterGameMode* ShooterGameMode;
	//UPROPERTY()
	//class AShooterGameState* ShooterGameState;

	float PlayerLocationUpdateTime = 1.5;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UShooterGameInstance* ShooterGI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Game", meta = (AllowPrivateAccess = "true"))
	class UShooterSaveGame* SaveGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<FSlateBrush> AIIconImages;

private:
	UFUNCTION()
	void OnRep_TeamID(FGenericTeamId OldTeamID);

	UFUNCTION()
	void OnRep_MySquadID();

	UFUNCTION(BlueprintCallable)
	bool IsPracticeMode();

	void StartVivox3DUpdate();

	UFUNCTION()
	void SetTeamWidgets(AShooterPlayerController* TeamMemberController);
};
