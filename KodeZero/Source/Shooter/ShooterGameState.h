// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Components/TimelineComponent.h"
// #include "Messages/ShooterVerbMessage.h"
#include "Shooter/EnumTypes/MatchState.h"
#include "ShooterTeamAgentInterface.h"
//#include "Shooter/Items/Weapons/SpawnWeapon.h"
#include "Http.h"
#include "ShooterGameInstance.h"
#include "Shooter/StructTypes/StatsStruct.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "Shooter/EnumTypes/RarityType.h"
#include "Shooter/EnumTypes/BoostType.h"
#include "Shooter/EnumTypes/AmmoType.h"
#include "Shooter/StructTypes/LobbyStruct.h"
#include "ShooterGameState.generated.h"


class AShooterCharacter;
class AShooterPlayerState;
class AShooterPlayerController;

DECLARE_MULTICAST_DELEGATE_OneParam(FShooterOnPlayerArrayAddedDelegate, int32 AddIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterOnPlayerArrayAddedDynamicDelegate, int32, AddIndex);

DECLARE_MULTICAST_DELEGATE_OneParam(FShooterOnPlayerArrayRemovedDelegate, const AShooterPlayerState* RemovedShooterPlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterOnPlayerArrayRemovedDynamicDelegate, const AShooterPlayerState*, RemovedShooterPlayerState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDroneChanged, TArray<class AItemContainer*>, Drones);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDroneDestroyed, bool, bDroneDestroyed);
/**
 * AShooterGameState
 *
 */

USTRUCT(BlueprintType)
struct FPlayerControllerScores
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AShooterPlayerController* PlayerController = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PlayerScore = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlayerId = TEXT("");

};

USTRUCT(BlueprintType)
struct FTeamPlayerStates
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TeamID = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AShooterPlayerState*> TeamArray = TArray<AShooterPlayerState*>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AShooterPlayerController*> TeamControllerArray = TArray<AShooterPlayerController*>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FPlayerControllerScores> ScoreControllers = TArray<FPlayerControllerScores>();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TeamAvgScore = 0.f;

	void AddScoreController(AShooterPlayerController* InController, float InScore, const FString& InId)
	{
		FPlayerControllerScores ScoreController;
		ScoreController.PlayerController = InController;
		ScoreController.PlayerScore = InScore;
		ScoreController.PlayerId = InId;
		ScoreControllers.Add(ScoreController);
	}

    bool operator==(const FTeamPlayerStates& Other) const {
        return TeamID == Other.TeamID && TeamArray == Other.TeamArray;
    }
};

USTRUCT(BlueprintType)
struct FPlayerStatistics
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NumKills = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NumDeaths = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NumHits = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float NumMiss = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 NumGamesPlayed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageDealt = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DamageTaken = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float TotalMinutesPlayed = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Rating = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlayerName = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString PlayerId = TEXT("");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AShooterPlayerState* OwnerPlayerState = nullptr;
};


/* Struct that contains the debris spawn locations, 
the box collision scale for the RandomDebris actor,
and the box collision Z rotation for the RandomDebris actor.
*/
USTRUCT(BlueprintType)
struct FLevelDebris
{
	GENERATED_BODY() // needed for structs

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FVector> DebrisSpawnLocations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FVector2D> ScaleArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> RotationArray;

};

struct FScoreCalcContext
{
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FPlayerGameStats*> PlayerStatsArray = TArray<FPlayerGameStats*>();
    float MaxKDR = 0.f;
    float MaxDTR = 0.f;
    float MaxTimePlayed = 0.f;
    float MaxCombatFactor = 1.f;
	float MaxDamageDealt = 0.f;
	bool bUseTimeFactor = false;
	bool bIncludeAIInScore = true;
};

UCLASS(Config = Game)
class SHOOTER_API AShooterGameState : public AGameState
{
	GENERATED_BODY()

public:
	AShooterGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void Tick(float DeltaTime) override;

	float GetServerFPS() const { return ServerFPS; }

	/** Array of all ShooterPlayerStates, maintained on both server and clients (ShooterPlayerStates are always relevant) */
	UPROPERTY(Transient, BlueprintReadOnly, Category = GameState)
	TArray<TObjectPtr<AShooterPlayerState>> ShooterPlayerArray;

	UPROPERTY(BlueprintAssignable, Meta = (DisplayName = "On Player Array Added"))
	FShooterOnPlayerArrayAddedDynamicDelegate BP_OnPlayerArrayAddedDelegate;

	UPROPERTY(BlueprintAssignable, Meta = (DisplayName = "On Player Array Removed"))
	FShooterOnPlayerArrayRemovedDynamicDelegate BP_OnPlayerArrayRemovedDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", Replicated, meta = (AllowPrivateAccess = "true"))
	EGameMatchState GameMatchState = EGameMatchState::EMS_Idle;

	UPROPERTY(VisibleAnywhere, Replicated)
	int32 GameMaxPlayers = 0;

	UPROPERTY(VisibleAnywhere)
	int32 NumDronesToSpawn = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_DroneActor, Category = "Drone", meta = (AllowPrivateAccess = "true"))
	TArray<class AItemContainer*> DroneActor;

	UFUNCTION()
	void OnRep_DroneActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = GameState, meta = (AllowPrivateAccess = "true"))
	int32 PlayersStillAlive = 0;

	//UPROPERTY(BlueprintReadOnly)
	//class ASpawnWeapon* SpawnWeaponInst;

	UFUNCTION(BlueprintCallable)
	void SpawnGroundContainers();

	UFUNCTION(BlueprintCallable)
	void SpawnGroundedItems();
	void SpawnItemsAfterAIKill(FTransform AITransform);

	void EndMatch();
	bool PracticeModeEndMatch();

	int32 GetPlayersStillAlive();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	bool bInitializedItemMaps = false;

	UPROPERTY()
	FOnDroneChanged DroneDelegate;

	UPROPERTY()
	FOnDroneDestroyed DroneDestroyedDelegate;

	void DroneDestroyed(AItemContainer* DestroyedDrone);

	UPROPERTY()
	FTimerHandle DroneLocTimer;

	UFUNCTION(BlueprintCallable)
	void SetWarningState();

	TArray<FPlayerGameStats> GetRankedPlayerScores();

	void CollectAndPrepareStats(UWorld* World, FScoreCalcContext& Context);
	void ComputeBaseScores(FScoreCalcContext& Context);
	void ComputeValorBoosts(FScoreCalcContext& Context);
	void ComputeCombatFactors(FScoreCalcContext& Context);
	void ComputeGraphScores(FScoreCalcContext& Context);
	void ApplyFinalScores(FScoreCalcContext& Context);
	TArray<FPlayerGameStats> GetSortedStatValues(FScoreCalcContext& Context);

	void SetEndMatchMetrics();

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    //UDataTable* LevelRequirementsTable;

	UPROPERTY(VisibleAnywhere)
	TArray<FVector_NetQuantize> AllTeleportLocations;

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	//UFUNCTION(BlueprintCallable, Category = LevelBlueprint)
	void FireMissle();
	void SpawnMissileAsync(const FVector& SpawnLocation, const FRotator& TargetRotation);
	void OnMissileClassLoaded(FVector SpawnLocation, FRotator TargetRotation);

	UFUNCTION()
	void MissileStartWarning();

	void DroneTimer();

    void InitializeMissileHitLocations();
	void InitializeTeleportLocations();
	//void InitializeFireEffectLocations();
    //void SetupFireEffectArrays();

	void MissileStart();
	void DroneStart();

	void StartMissileWarning();
	void StopMissileWarning();
	//UFUNCTION()
	//void UpdateMissileWarning(float OpacityValue);

	//UFUNCTION(Server, Reliable)
	//void InitMaterialInstances();

	UPROPERTY(Replicated)
		float ServerFPS;

	void GetContainerSplineActor();

	void CheckStartMatch(AShooterPlayerState* ShooterPS);

	void StartBattle();

	// Delegate fired when both base and Shooter player arrays added.
	FShooterOnPlayerArrayAddedDelegate OnPlayerArrayAddedDelegate;

	// Delegate fired when both base and Shooter player arrays removed.
	FShooterOnPlayerArrayRemovedDelegate OnPlayerArrayRemovedDelegate;

	UFUNCTION()
	void OnNewPlayerState(AShooterPlayerState* ShooterPS, UWorld* World);
	void GameStateExpiration();

	void CheckForWinner(int32 PlayersAlive);
	void CheckForWinningTeam();

	// Constuct the array TeamInfoArray which has elements of type FTeamPlayerStates (defined above)
	void ConstructTeamArray();

	// Prevent new players from joining the session
	void SetSessionMaxPlayers();

	UFUNCTION(BlueprintImplementableEvent)
	void SetMaxPlayers(int32 MaxPlayers);

	UFUNCTION(BlueprintCallable)
	void SpawnHelicopters();

	/* Take the AActor array that is passed in this function, and randomlu shuffle it.*/
	void ShuffleArray(TArray<AActor*>& myArray);
	void ShuffleIntArray(TArray<int32>& myArray);

	/* Function which spawns random debris in certain places in the map.
	The locations in this function are set to the places where buildings were removed.
	*/
	void SpawnDebris();

	/* Returns the index from the missile array location to fire.
	The MissileStartLocation and MissileEndLocation arrays are ordered such that
	the first 4 elements are the corner locations, the next 6 elements are the side locations,
	and the last 2 elements are the middle locations. */
	int32 GetMissileLocationIndex();

	void UpdateAIPatrolPoints();

	/* Checks to see if a container exists at the container target point in the map.
	If a container is not there, it will spawn one in the CheckContainersAndSpawn function. 
	Will be useful when there are many players. */
	void StartCheckContainersTimer();

	UFUNCTION()
	void CheckContainersAndSpawn();

	void PostPlayerStats();

	void SetVoiceVolume(AShooterPlayerState* NewShooterPS);

	void SoloEndMatch(const TArray<FPlayerGameStats> &PlayerStatsArray);
	void TeamEndMatch(const TArray<FPlayerGameStats> &PlayerStatsArray);

	void DelayedTournamentCheck(const TArray<FPlayerGameStats> &PlayerStatsArray);
	UFUNCTION()
	void TournamentCheck(const TArray<FPlayerGameStats> &PlayerStatsArray);
	void TeamTournamentCheck(const TArray<FTeamPlayerStates> &TeamStatsArray);

	void ParseTournamentData(FString DataStr);
	void ParsePostedStats(FString DataStr);
private:

	// Projectile missle class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TSoftClassPtr<class AProjectile> MissleClass;

	// Drone container class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AItemContainer> ContainerClass;

	//UPROPERTY(EditAnywhere)
	//TSubclassOf<class ASpawnWeapon> SpawnItemClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AWeapon> WeaponClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABoostItem> BoostClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AAmmo> AmmoClass;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class AShooterCharacter> ShooterCharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AShooterPlayerController> ShooterCharacterController;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ATargetPoint> GContainerTargetPointClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AGroundContainer> GContainerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ATargetPoint> HelicopterTargetPointClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AHelicopter> HelicopterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class ATargetPoint> ShooterAITargetPointClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AShooterAI> ShooterAIClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TMap<EEnemyType, TSubclassOf<ACharacter>> AICharacterMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AAICharacterBase> EnemyAIClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AShooterAIController> ShooterAIControllerClass;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	//TArray<AShooterAI*> AIArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (AllowPrivateAccess = "true"))
	TSoftClassPtr<class ARandomDebris> RandomDebrisClass;

	// Store random missle hit location in array (so that missles don't hit the same place)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> RandomMissleHitLocations;

	// Store random integers in array (so that missles don't hit the same place)
	UPROPERTY()
	TArray<int32> RandomIntArray;

	UPROPERTY()
	FTimerHandle MissleSpawnTimer;

	// Time to wait until missle is launched
	float MissleSpawnDelay = 55.f;
	float FireMissileTime = 15.f;
	//float MissleSpawnDelay = 125.f;

	bool bMultiplierEventFired = false;
	float MultiplierEventTriggerTime;

	UFUNCTION()
	void ShortenMapWithMissiles();

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", ReplicatedUsing=OnRep_MissileWarning, meta = (AllowPrivateAccess = "true"))
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", Replicated, meta = (AllowPrivateAccess = "true"))
	bool bMissileWarning = false;

	int32 MinPlayersToStart = 1;

	int32 TeamCount = 0;
	int32 TeamIdVal = 0;

	int8 NumMissileCorners = 4;
	int8 NumMissileSides = 6;
	int8 NumMissileMiddle = 2;

	int32 NumAIToSpawn = 32;

	UPROPERTY()
	class UShooterGameInstance* ShooterGI;
	UPROPERTY()
	class URequestsObject* HTTPRequestObj;

	UFUNCTION()
	void PostResponseReceived(FString ResponseStr, FString ResponseURL);

	class AShooterSpectatorPawn* SpawnSpectator(AShooterPlayerController* WinnerController);

	UPROPERTY()
	FVector MissileBoxScale = 100.f * FVector(416.f, 320.0f, 512.0f);

	//UFUNCTION()
	//void OnRep_MissileWarning();

	/*
	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicMinimapMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MinimapMaterialInstance;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicLargeMapMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* LargeMapMaterialInstance;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Missle Properties")
	UTimelineComponent* MissileTimeline;
	FOnTimelineFloat MissileTrack;

	UPROPERTY(EditAnywhere, Category = "Missle Properties")
	UCurveFloat* MissileCurve;
	*/

	UPROPERTY(VisibleAnywhere)
	TArray<EBoostType> BoostIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EWeaponType> WeaponIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EItemRarity> RarityIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<EAmmoType> AmmoIndexArray;

	UPROPERTY(VisibleAnywhere)
	TArray<int32> BoostProbabliltyArray;
	UPROPERTY(VisibleAnywhere)
    TArray<int32> WeaponProbabliltyArray;
	UPROPERTY(VisibleAnywhere)
    TArray<int32> RarityProbabliltyArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FString Handicap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Elim, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class AShooterSpectatorPawn> ShooterSpectatorClass;

	EEnemyType GetRandomEnemyType();

public:
	FORCEINLINE TSubclassOf<AGroundContainer> GetGContainerClass() const { return GContainerClass; }
	FORCEINLINE TSubclassOf<AShooterAI> GetShooterAIClass() const { return ShooterAIClass; }
	FORCEINLINE FString GetHandicap() const { return Handicap; }
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	int32 LocationIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> MissileStartLocation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> MissileEndLocation;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Missle Properties", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> MissileHitLocations;

	UFUNCTION(BlueprintImplementableEvent)
	void InitializeDynamicMaterial(UMaterialInstanceDynamic* DynamicInstance, UMaterialInstanceDynamic* DynamicInstanceLargeMap);

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AGameStateBase interface
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	void KickPlayer(const FString& PlayerToKickId);
	//~End of AGameStateBase interface

	UFUNCTION(BlueprintImplementableEvent)
	void OnCharacterEliminated(AShooterCharacter* EliminatedCharacter);

	void UpdateStats(AShooterPlayerState* InPlayerState);

	UFUNCTION()
	void ClientBeginProcess(AShooterPlayerState* ClientPS);
	void NotifyPlayerJoined(APlayerState* NewPlayerState, FString NewPlayerName);

	void StartLobbyMatch(const FString& Region);

	void UpdateLobbyIdArray(bool bIsReady, const FString& ID);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_LobbyIdArray)
	TArray<FString> LobbyIdArray;

	UFUNCTION()
	void OnRep_LobbyIdArray();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_LobbyServerInfo)
	FLobbyInfo LobbyServerInfo;

	UFUNCTION()
	void OnRep_LobbyServerInfo();

	UFUNCTION()
	void StartGettingServerInfoLobby();
	void ParseLobbyMatch(FString DataStr);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastUpdateTravelStatus(const FString& BackendStatus);

	UPROPERTY()
	FTimerHandle BackupLobbyTravelTimer;

	UPROPERTY()
	bool bStartingMatchFromLobby = false;

	UPROPERTY(Replicated)
	FString HostPlayerId;

	UFUNCTION()
	void SendPlayerFromLobby();

	UFUNCTION()
	void StartSendingServer();

	void SetTeams(AShooterPlayerState* ShooterPS, const FString& GroupId);
	int32 SetNonLobbyTeam();
	void ParsePlayerSessionData(AShooterPlayerState* ShooterPS, const FString& DataStr);
	TMap<FString, int32> LobbyToTeamMap;
	TMap<int32, int8> TeamMemberCount;
	int32 NextAvailableTeamId = 30;
	int32 AITeamId = 1000;

	bool CheckIsValidGameMode();

	/*
	// Send a message that all clients will (probably) get
	// (use only for client notifications like eliminations, server join messages, etc... that can handle being lost)
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "Shooter|GameState")
	void MulticastMessageToClients(const FShooterVerbMessage Message);

	// Send a message that all clients will be guaranteed to get
	// (use only for client notifications that cannot handle being lost)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Shooter|GameState")
	void MulticastReliableMessageToClients(const FShooterVerbMessage Message);
	*/

	// Register with the OnPlayerArrayAddedDelegate delegate.
	void OnPlayerArrayAdded_Register(FShooterOnPlayerArrayAddedDelegate::FDelegate Delegate);

	// Register with the OnPlayerArrayRemovedDelegate delegate.
	void OnPlayerArrayRemoved_Register(FShooterOnPlayerArrayRemovedDelegate::FDelegate Delegate);
	void UpdateElimmedPlayerCount();
	void CheckWinners();

	void GetPings();

	float MissileWarningOpacity = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone", meta = (AllowPrivateAccess = "true"))
	int32 RandomDronePathIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drone", meta = (AllowPrivateAccess = "true"))
	TArray<class AContainerSplinePath*> PathActor;

	// Get an array of AShooterPlayerState's of all alive team members (if in team mode)
	UFUNCTION(BlueprintCallable)
	TArray<AShooterPlayerState*> GetAliveTeamMembers(FGenericTeamId TeamID);

	AShooterPlayerState* GetRandomAlivePlayer();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	TArray<FTeamPlayerStates> TeamInfoArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	TArray<int32> TeamIDArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ShooterControllerArray, Category = "Team", meta = (AllowPrivateAccess = "true"))
	TArray<AShooterPlayerController*> ShooterControllerArray;

	UFUNCTION()
	void OnRep_ShooterControllerArray();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
	FString PartyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> HelicopterTargetPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> GContainerTargetPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> ShooterAITargetPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> MapItemTargetPoints;

    /* Quit the AWS server */
	UFUNCTION(BlueprintImplementableEvent)
	void QuitServer(bool bQuitServer, float DelayTime, bool bOnLogout = false);

	UFUNCTION(BlueprintImplementableEvent)
	void StopBackfill();

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Debris", meta = (AllowPrivateAccess = "true"))
	//bool bSpawnedDebris = false;

	void StartServerFuncs(bool bInTeamMode);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Debris", meta = (AllowPrivateAccess = "true"))
	bool bIsTeamMode = false;

	// Max number of players per team
	int8 MaxTeamMembers = 3;

	void CheckPracticeContainers();

	UPROPERTY(VisibleAnywhere, Category = Stats, meta = (AllowPrivateAccess = "true"))
	//TMap<AShooterPlayerState*, FPlayerStatistics> PlayerStatsMap;
	TArray<FPlayerStatistics> AllPlayerStatsArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> AIPatrolPointsFixed = {
					{-2150.0f,-14720.0f,120.0f}, {-2220.0f,2410.0f,120.0f},
					{-3460.0f,33120.0f,120.0f},{24580.0f,32030.0f,120.0f},
					{24550.0f,13210.0f,120.0f},{31110.0f,-1260.0f,120.0f},
					{66320.0f,13690.0f,120.0f},{39700.0f,35950.0f,120.0f},
					{-40790.0f,-19190.0f,120.0f},{-23450.0f,33040.0f,120.0f},
					{-52560.0f,38400.0f,120.0f}, {-75440.0f,11370.0f,120.0f},
					{-70660.0f,-20380.0f,120.0f},{-52440.0f,-20900.0f,120.0f},
					{59050.0f,-34440.0f,120.0f}, {58330.0f,-2370.0f,120.0f},
					{-45280.0f,-38490.0f,120.0f}, {-2520.0f,14500.0f,120.0f},
					{-22970.0f,14500.0f,120.0f},{24240.0f,-10970.0f,200.0f},
					{62310.0f,33340.0f,120.0f},{-49240.0f,-4720.0f,120.0f},
					{60.0f,23040.0f,120.0f}, {-59960.0f,22650.0f,1150.0f},
					{-58100.0f,5670.0f,370.0f}, {39770.0f,-2330.0f,100.0f},
					{24250.0f,-14660.0f,470.0f}, {42360.0f,-30180.0f,350.0f},
					{40740.0f,23330.0f,1530.0f}, {52890.0f,21980.0f,670.0f}
					};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> AIPatrolPointsFixedPractice = {
					{-13820.0f,11600.0f,50.0f}, {-11880.0f,6950.0f,50.0f},
					{-6880.0f,7450.0f,50.0f},{-3970.0f,740.0f,50.0f},
					{-1010.0f,10740.0f,50.0f},{-1580.0f,16420.0f,50.0f},
					{-1290.0f,14020.0f,50.0f},{90.0f,5230.0f,50.0f}
					};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<FVector> AIPatrolPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	TArray<ACharacter*> AIShooterArray;

	UPROPERTY()
	int32 TargetIndex = 0;

	UFUNCTION(BlueprintCallable)
	void SpawnAIShooters(AShooterPlayerController* InController);

	UFUNCTION()
	void StartAISpawn();

	UFUNCTION(BlueprintCallable)
	void SpawnNewAI(AController* InController, const EEnemyType& AIType);

	UFUNCTION()
	void DestroyAIs();

	void CheckWinnerAfterAIKillTimer();

	void CheckWinnerAfterAIKill();

	UFUNCTION(BlueprintCallable)
	bool IsPracticeMode();

	void InitializeRandomAssetMaps();
	EWeaponType GetRandomWeaponType();
	EItemRarity GetRandomRarityType();
	EBoostType GetRandomBoostType();
	EAmmoType GetRandomAmmoType();

	void StopAISearch();

	UPROPERTY()
	TArray<FString> AINameArray = {TEXT("AuntieFaBot"), TEXT("ZuckOnThis42"), TEXT("Skynet4Prez"), TEXT("TurboKaren69"), TEXT("BezosBeard69"), TEXT("TaxEvaderPrime"),
				TEXT("NPC_MommyIssues"), TEXT("EatTheBotz"), TEXT("RektByMyRoomba"), TEXT("TikTokSyndrome"), TEXT("FlatEarthSniper"), TEXT("CancelledSinceBirth"),
				TEXT("GaslightGrenade"), TEXT("ChadGPT"), TEXT("CybergeddonKaren"), TEXT("TriggerMeElmo"), TEXT("ElonMuskRat"), TEXT("XX_EgoDeath_XX"),
				TEXT("GhostOfZuckerberg"), TEXT("404SniperNotFound"), TEXT("CtrlAltDelMePlz"), TEXT("iHackUrMom"), TEXT("iStealYourCloud"),
				TEXT("WokeAndBroke"), TEXT("TouchGrass.exe"), TEXT("2Fast2Vaxxed"), TEXT("TriggeredTroll99"), TEXT("KarenKiller9000"),
				TEXT("NFTears420"), TEXT("GlutenSniper"), TEXT("5GDidThis"), TEXT("SkynetIntern"), TEXT("CringeLordXOXO"),
				TEXT("CancelMePlz"), TEXT("HackedByYourMom"), TEXT("SoyBot9001"), TEXT("iCantEvenReload"), TEXT("ClickBaitSn1pez")};
	/*TArray<FString> AINameArray = {TEXT("NeoByte"), TEXT("CipherX"), TEXT("QuantumPulse"), TEXT("CyberNexa"), TEXT("SynthSpecter"), TEXT("InfraWave"),
				TEXT("DataDynamo"), TEXT("NanoByte"), TEXT("HoloNova"), TEXT("TechnoShade"), TEXT("NexiSynth"), TEXT("MetaMind"),
				TEXT("CircuitSiren"), TEXT("VoxLink"), TEXT("ByteSpecter"), TEXT("CodeEcho"), TEXT("NRG-X"), TEXT("DigiGhost"),
				TEXT("TechnoPhantom"), TEXT("CipherStrain"), TEXT("SynthSplicer"), TEXT("QuantumQuasar"), TEXT("NeuralNexis"),
				TEXT("ByteWhisper"), TEXT("HoloCrafter"), TEXT("CodeChameleon"), TEXT("PixelProphet"), TEXT("DataDjinn"),
				TEXT("AetherLink"), TEXT("NanoVigil"), TEXT("CyberZenith"), TEXT("EnigmaPulse"), TEXT("InfraNaut"),
				TEXT("Zero-Gravity"), TEXT("EchoStryker"), TEXT("RogueMatrix"), TEXT("TechLynx"), TEXT("PhoenixSynapse")};
	*/
};
