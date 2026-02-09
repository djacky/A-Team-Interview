// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ShooterGameMode.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJsonNumPlayersCompletedDelegate, int32, SessionNumPlayers);

UCLASS()
class SHOOTER_API AShooterGameMode : public AGameMode
{
	GENERATED_BODY()

protected:
	//UPROPERTY()
	//class AShooterGameState* ShooterGS;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	//virtual void OnMatchStateSet() override;
	virtual void PostLogin(APlayerController* PlayerController);
	virtual void Logout(AController* PController);
	class AShooterGameState* GetShooter_GS();

	FTransform GetUnhitMissileLocation();

	UFUNCTION()
	void UpdateMinNumPlayers();

	//UFUNCTION()
	void StartPlayerPossess(APlayerController* PlayerController);

	virtual void PostSeamlessTravel() override;

	UFUNCTION(BlueprintCallable)
	void InitializeGameStateFuncs(bool bIsTeamMode);

	UFUNCTION()
	void StartMatchTimer();
	bool bStartMatchTimer = false;

	void SetWarningState();

public:
	AShooterGameMode();
	void PlayerEliminated(class AShooterCharacter* ElimmedCharacter,
		class AShooterPlayerController* VictimController, AShooterPlayerController* ShooterAttackerController,
		class AShooterPlayerState* VictimPlayerState, AShooterPlayerState* AttackerPlayerState);
	void AddPlayerToList(class AShooterCharacter* PlayerToAdd);
	
	UPROPERTY(VisibleAnywhere)
	TArray<AShooterCharacter*> PlayerList;

	int32 MaxLevelPlayers = 30;

	UPROPERTY()
	TArray<AShooterPlayerController*> ControllersLoggedIn;

	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);

	UFUNCTION()
	void UpdateHUDParams(AController* ElimmedController);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void ReportNewCharacterSpawnPoint(ATargetPoint* InTargetPoint);

	//UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	//void ReportNewMissileSystemPoint(AMissleSystem* InMissleSystem);

	UPROPERTY(BlueprintReadWrite)
	bool CharacterSpawnPointsFinishedReporting = false;
	
	UPROPERTY(BlueprintReadWrite)
	bool MissileSystemPointsFinishedReporting = false;

	UPROPERTY(BlueprintReadWrite)
	TArray<AActor*> CharacterSpawnPoints;

	UPROPERTY(BlueprintReadWrite)
	TArray<AActor*> MissileHitPoints;

	UFUNCTION(BlueprintCallable)
	void GetSessionNumPlayers(FString MatchmakerData);

	UPROPERTY(BlueprintAssignable)
	FJsonNumPlayersCompletedDelegate OnGetSessionPlayersCompleted;

	UFUNCTION(BlueprintImplementableEvent)
	void DenyNewPlayers();

	UFUNCTION(BlueprintImplementableEvent)
	void OnGetNumSessionPlayers(int32 SessionNumPlayers);

	int32 MaxLoggedInPlayers = 0;

	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
	FORCEINLINE int32 GetNumMissilesToPrespawn() const { return NumMissilesToPrespawn; }
	FORCEINLINE float GetWarningCountdownTime() const { return WarningCountdownTime; }
	FORCEINLINE class UShooterGameInstance* GetShooterGI() const { return ShooterGI; }

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 850.f;

	UPROPERTY(EditDefaultsOnly)
	float WarningStartTime = 45.f;

	float LevelStartingTime = 0.f;

	bool bMatchEnded = false;
	bool bBattleStarted = false;

	UPROPERTY(VisibleAnywhere)
	TArray<FVector> SpawnLocationPoints;

	UFUNCTION(BlueprintImplementableEvent)
	void ActivateAWSSession();

	void UpdateMissileHits();

private:
	UPROPERTY(EditAnywhere, Category = CharacterSpawn);
	TSubclassOf<class ATargetPoint> CharacterSpawnPointsClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = CharacterSpawn, meta = (AllowPrivateAccess = "true"));
	int32 NumLoggedInPlayers = 0;

	UPROPERTY()
	FTimerHandle MinPlayersTimer;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterSpawn, meta = (AllowPrivateAccess = "true"));
	int32 MinNumPlayersToStart = 25;

	float CountdownTime = 0.f;
	float WarningCountdownTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vars", meta = (AllowPrivateAccess = "true"));
	UShooterGameInstance* ShooterGI;

	void OnLogout(AController* PController);

	void InitializeCharSpawnLocations();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);

	int32 MissileHits = 0;
	int32 NumMissilesToPrespawn = 6;
};
