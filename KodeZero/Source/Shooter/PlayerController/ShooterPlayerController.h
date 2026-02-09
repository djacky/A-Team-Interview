// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterTeamAgentInterface.h"
#include "Shooter/EnumTypes/MatchState.h"
#include "Shooter/ShooterPlayerState.h"
#include "InputActionValue.h"
#include "Shooter/StructTypes/LobbyStruct.h"
#include "Engine/StreamableManager.h"
#include "Shooter/Misc/Interfaces/ShooterInterface.h"
#include "ShooterPlayerController.generated.h"


class AShooterPlayerState;
class APawn;
class AShooterGameState;

/**
 * 
 */

class UInputMappingContext;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRespawnDelegate, bool, bRespawned);
UCLASS()
class SHOOTER_API AShooterPlayerController : public APlayerController, public IShooterTeamAgentInterface, public IShooterInterface
{
	GENERATED_BODY()

public:
	AShooterPlayerController();

	void HandleMatchHasStarted();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD, meta = (AllowPrivateAccess = "true"))
	class AShooterCrosshairHUD* ShooterHUD = nullptr;

	void SetInventoryVisibility(bool bIsVisible);

protected:

	UPROPERTY(EditAnywhere, Category = Input)
	UInputMappingContext* ShooterContext;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* PushToTalkAction;

	UPROPERTY(EditAnywhere, Category = Input)
	UInputAction* ToggleMapAction;

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// Ping warnings
	void HighPingWarning(int32 PingAmount);
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

	UFUNCTION()
	void StartOnPossess(class AShooterCharacter* ShooterChar);

	UFUNCTION()
	void PlayElimmedCharSound(AShooterCharacter* DamageCauserCharacter);

	UFUNCTION(Server, Reliable)
	void ServerSetCharacter(TSubclassOf<AShooterCharacter> CharacterClass, USkeletalMesh* TargetSkelMesh);

	UFUNCTION()
	void StartHeliPossess(AHelicopter* PossessedHelicopter);

	void StartMatchLogic(AShooterCharacter* ShooterChar);

	UFUNCTION(Server, Reliable)
	void ServerSetOldStats(AShooterPlayerState* ShooterPS);

	void SetTextureStreamingEnabled(FString StreamingValue);

	float ClientServerDelta = 0.f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync(float DeltaTime);

	void TalkButtonPressed(const FInputActionValue& Value);
	void TalkButtonReleased(const FInputActionValue& Value);
	void ToggleMapPressed(const FInputActionValue& Value);

	UFUNCTION()
	void OnResponseReceived(FString ResponseStr, FString ResponseURL);

	UFUNCTION()
	void StartCheckingMemory();

private:
	// Reference to the Overall HUD Overlay BlueprintClass
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> HUDOverlayClass;

	float HighPingRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;

	float PingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float CheckPingFrequency = 10.f;

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 100.f;

	UPROPERTY()
	FTimerHandle PlayElimmedSoundTimer;
	UPROPERTY(EditDefaultsOnly)
	float PlayElimmedSoundDelay = 1.f;

	UPROPERTY()
	class UShooterGameInstance* ShooterGI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Class Variables", meta = (AllowPrivateAccess = "true"))
	class UShooterSaveGame* SaveGame;

	UPROPERTY()
	class URequestsObject* HTTPRequestObj;
	
public:

	UFUNCTION(BlueprintCallable)
	void LogMemoryUsage();

	FORCEINLINE bool GetMatchStarted() const { return bMatchStarted; }

	void RemoveHeliHUDElements();

	UFUNCTION(BlueprintCallable, Category = "Shooter|PlayerController")
	AShooterPlayerState* GetShooterPlayerState() const;
	AShooterGameState* GetShooter_GS() const;

	//~AActor interface
	virtual void PreInitializeComponents() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//UFUNCTION(Client, Reliable)
	void OnPlayerLeftGame(APlayerState* PlayerLeftPS);

	//~AController interface
	virtual void OnUnPossess() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void PlayerTick(float DeltaTime) override;
	//~End of APlayerController interface

	//~ACommonPlayerController interface
	virtual void OnPossess(APawn* InPawn) override;
	//~End of ACommonPlayerController interface

	//~IShooterTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnShooterTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of IShooterTeamAgentInterface interface
	void BroadcastElim(APlayerState* Attacker, APlayerState* Victim, bool bTeamMode);
	void ShowReturnToMainMenu(bool bWinner = false, FPlayerGameStats PlayerGameStats = FPlayerGameStats(), int32 PlayersLeft = 0, int32 RespawnsLeft = 0);
	void ShowPauseMenu(bool bGamePaused, const TArray<FPlayerGameStats>& PlayerStatsArray);
	void UpdateLobbyStatus(const FString& ServerStatus);
	void ManageInventory(bool bManageInventory);
	UFUNCTION(BlueprintCallable)
	void CameraShake(float ScaleValue = 1.f);
	
	UFUNCTION(Client, Reliable)
	void ClientShowAttackerLocation(AActor* AttackerActor, AActor* DamagedActor, float DamageAmount);
	void ShowAttackerLocationMissed(AActor* AttackerActor, FVector ImpactPoint);

	UFUNCTION(Client, Reliable)
	void ClientBroadcastPlayersAlive(EGameMatchState State, int32 PlayersLeft, int32 MaxPlayers);
	UFUNCTION(Client, Reliable)
	void ClientBroadcastMissileWarning(FVector MissileLocation);
	UFUNCTION(Client, Reliable)
	void ClientStopMissileWarning();
	UFUNCTION(Client, Reliable)
	void ClientUpdateHUDNumOfKills(const FPlayerGameStats& InStats, AShooterCharacter* AttackerCharacter);
	void SetHUDCountdownTime(float CountdownTime);
	void OnMatchStateSet(EGameMatchState State);
	void InitInvalidHUD();
	void AddMissileHitWidget(FVector MissileLocation, bool bPlayAnimation);

	//UFUNCTION(Client, Reliable)
	void CreateStartMatchWidget();

	void UpdateLoggedInPlayers(int32 NumPlayersLoggedIn);

	void GeneralMessage(FString Message, FString Color, float AddDelay = 0.f, float RemoveTimeFactor = 1.4f);

	void UpdateHUDonRespawn();

	UPROPERTY(BlueprintAssignable, Category = Delegates,  meta = (AllowPrivateAccess = "true"))
	FRespawnDelegate RespawnDelegate;

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncement(class APlayerState* Attacker, APlayerState* Victim, bool bTeamMode);

	UFUNCTION(Client, Reliable)
	void ClientElimAnnouncementSmall(class AShooterPlayerState* Attacker, AShooterPlayerState* Victim, EShooterDamageType ShooterDT, bool bTeamMode);

	UFUNCTION(Client, Unreliable)
	void ClientAIElimAnnouncement(AShooterPlayerState* AttackerPS, EShooterDamageType ShooterDT, const FString& AIName);

	UPROPERTY()
	class UPauseMenu* PauseMenuWidget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Map, meta = (AllowPrivateAccess = "true"))
	class ULastPlayerLocation* LastPlayerLocationWidget = nullptr;

	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> PauseMenuHandle;

	void OnPauseMenuClassLoaded(bool bGamePaused, const TArray<FPlayerGameStats>& PlayerStatsArray);

	void ResetPauseMenuHandle();

	void SetLastPlayerLocation(const FVector& LastPlayerLocation);

	UPROPERTY()
	class ULobbyStatusWidget* LobbyStatusWidget;

	UPROPERTY()
	class UOnStillLoading* OnStillLoadingWidget;
	void CheckStillLoadingWidget();
	void OpenStillLoadingWidget();

	UPROPERTY()
	class UTutorialWidget* PracticeTutorialWidget;

	UPROPERTY()
	class UShowDamageLocation* DamageLocationWidget;

	//UPROPERTY()
	//class UShowDamageIndicator* DamageIndicatorWidget;

	UFUNCTION(Client, Reliable)
	void ClientUpdateHUD(bool bHeliHUD);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (AllowPrivateAccess = "true"))
	class AMusicManager* MusicActor;

	void PlayMissileDamageBoxSound(bool bInDamageBox);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile Properties", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* MissileDamageBoxSoundComponent;

	void SetCharacter();

	UFUNCTION(Client, Reliable)
	void ClientSetCharacter(float InMatchTime, float InLevelStartingTime, float InWarningTime);

	UFUNCTION(Client, Reliable)
	void ClientNotifyNewPlayer(const FString& InPlayerName);

	UFUNCTION(BlueprintCallable)
	bool IsPracticeMode();

	void StartTutorial();

	UFUNCTION()
	void OnRep_MatchState();

	void PracticeRestart();

	UPROPERTY()
	class UReturnToMainMenu* ReturnToMainMenu;

	void DestroyReturnToMainMenu();

	UPROPERTY()
	class UShowTopPlayers* TopPlayersWidget;

	void UpdateAICount(int32 AICount);

	void ShowAICountBox(bool bShowBox);

	UFUNCTION(Server, Reliable)
	void ServerUpdateHUDonRespawn(bool bMatchHasStarted, const FString &UserPlayerId);

	void DestroyEOSGameSession();

	UFUNCTION(BlueprintImplementableEvent)
	void DestroyEOSSession();

	UFUNCTION(BlueprintImplementableEvent)
	void RemovePlayerSession();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateDiscordPresence(const FString& State, int32 NumPlayers, const FString& GameSessionId, const FString& PartyId);

	UFUNCTION(Client, Reliable)
	void ClientShowTopPlayers(const TArray<FPlayerGameStats>& InPlayerStatsArray, bool bIsWinner, const TArray<FString>& WinnerNames, EGameModeType GameModeType);

	UFUNCTION(Client, Reliable)
	void ClientShowPrizeAmount(float PercentWon, float AmountWon, const FString &Network, const FString &TourName);

	UFUNCTION()
	void UpdateWidgetXP(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks, const FPlayerProgress& DeltaProgress);

	void UpdateProgressStats(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks);

	UFUNCTION(Client, Reliable)
	void ClientUpdateXP(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks, const FPlayerProgress& DeltaProgress);

	UFUNCTION(Client, Reliable)
	void ClientRespawn(const FVector& LastPlayerLocation);

	void SetHUDTime();

	virtual float GetServerTime(); // Synced with server world clock

	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	float SingleTripTime = 0.f;

	void OnGameEndForWinner();

	UFUNCTION(Client, Reliable)
	void ClientStartSetViewToAI(FVector AIWinnerLocation, FRotator AIWinnerRotation);

	//UFUNCTION(Client, Reliable)
	void BroadcastTeamID();

	UFUNCTION(Client, Reliable)
	void ClientStopVivox();

	UPROPERTY()
	bool bMatchEnded = false;

	void AddTeamMemberInContainer(AShooterPlayerState* AddShooterPS);
	void SetTeamMemberAttributes(AShooterPlayerState* HitPlayer, float NewHealth, bool bIsShield);

	UFUNCTION(Client, Reliable)
	void ClientSetTeamWidgetValues(AShooterPlayerState* OwnerPS, float Value, float MaxValue, bool bIsCharShield);

	UPROPERTY()
	AShooterCharacter* MainCharacter;

	void GetWeaponData(const FString &NFT_ID);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FText> KeyNameMap;

	UPROPERTY()
	FString PlayerId;

	void UpdateLobbyReady(const TArray<FString>& LobbyIdArray);

	void SendPlayerFromLobby(const FLobbyInfo& LobbyServerInfo);

	UFUNCTION(Server, Reliable)
	void ServerUpdateLobbyIdArray(const FString& ClientPlayerId);

private:
	UPROPERTY()
	FOnShooterTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	APlayerState* LastSeenPlayerState;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSoftClassPtr<UUserWidget> PauseMenuClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> LastPlayerLocationClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> LobbyStatusClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> OnStillLoadingClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> PracticeTutorialClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> DamageLocationClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> DamageIndicatorClass;

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> TopPlayersClass;

	bool bReturnToMainMenuOpen = false;

	UPROPERTY(EditAnywhere, Category = "Missle Properties")
	TSubclassOf<UUserWidget> MissileWarningWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	class UMissileWarning* MissileWarning;

	UPROPERTY(EditAnywhere, Category = Helicopter)
	TSubclassOf<UUserWidget> HelicopterHealthBarWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Helicopter, meta = (AllowPrivateAccess = "true"))
	class UHelicopterHealthBar* HelicopterHealthBar;

	UPROPERTY(EditAnywhere, Category = Helicopter)
	TSubclassOf<UUserWidget> HelicopterInventoryBarWidget;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Helicopter, meta = (AllowPrivateAccess = "true"))
	class UHelicopterInventory* HelicopterInventoryBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<UMissileWarning*> MissileWidgetArray;
	UPROPERTY(EditAnywhere, Category = "Missle Properties")
	class USoundCue* MissileWarningSound;
	UPROPERTY(EditAnywhere, Category = "Missle Properties")
	class USoundCue* MissileOutsideWarningSound;
	UPROPERTY(EditAnywhere, Category = HUD)
	USoundCue* ManageInventorySound;

	UPROPERTY(EditAnywhere, Category = Combat)
	USoundBase* WinnerSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = HUD, meta = (AllowPrivateAccess = "true"))
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_MatchState)
	EGameMatchState MatchState = EGameMatchState::EMS_PreIdle;

	UPROPERTY(EditAnywhere, Category = "Start Match")
	TSubclassOf<class UUserWidget> StartMatchClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Start Match", meta = (AllowPrivateAccess = "true"))
	class UStartMatchWidget* StartMatchWidget;

	UPROPERTY(EditAnywhere, Category = "Music")
	TSubclassOf<class AMusicManager> MusicClass;

	UPROPERTY(EditAnywhere, Category = "Character")
	TSubclassOf<AShooterCharacter> SelectedTargetCharacterClass;

	UPROPERTY(EditAnywhere, Category = "Notify")
	TArray<USoundCue*> EpicNotifySounds;

	UPROPERTY(VisibleAnywhere)
	int32 LocalPlayersStillAlive = 0;

	bool bTutorialStarted = false;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarningMatchTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY()
	class AShooterGameMode* ShooterGM;

	UPROPERTY(VisibleAnywhere)
	bool bMatchStarted = false;

protected:

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_RespawnSwitch)
	bool bRespawnSwitch = false;

	UFUNCTION()
	void OnRep_RespawnSwitch();

	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

	UFUNCTION(Client, Reliable)
	void ClientSetHUDPlayersAtStart(int32 NumPlayersAlive);

	UFUNCTION(BlueprintCallable)
	void UpdateLumenParams();
	void LoadSavedGame();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UCameraShakeBase> CameraShakeClass;

	void BroadcastOnPlayerStateChanged();

	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;

	UFUNCTION(BlueprintCallable)
	void SetTeamIds(const FString& PlayerData);

	void OnRewardObtained_Implementation(float CameraScaleVal) override;
	//~End of AController interface
};
