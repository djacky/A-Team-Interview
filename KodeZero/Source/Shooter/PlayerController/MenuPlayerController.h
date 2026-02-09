// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Shooter/StructTypes/LobbyStruct.h"
#include "MenuPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	//virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Music")
	TSubclassOf<class AMenuMusicManager> MusicClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Class Variables", meta = (AllowPrivateAccess = "true"))
	AMenuMusicManager* MenuMusicManager;
	

	UFUNCTION(BlueprintCallable)
	int32 GetLumen();
	UFUNCTION(BlueprintCallable)
	void SetLumen(bool bIsLumen);

	UFUNCTION(BlueprintCallable)
	FString GetConfigStrParam(FString Section, FString Key);
	UFUNCTION(BlueprintCallable)
	void SetConfigStrParam(bool bIsTrue, FString Section, FString Key);

	void CreateSessionFromJoin();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FLobbyInfo LobbyServerInfo;

	UPROPERTY()
	class URequestsObject* HTTPRequestObj;

	UFUNCTION()
	void OnCreateSessionFromJoin(FString ResponseStr, FString ResponseURL);

	void ParseLobbyMatch(FString DataStr);

	UFUNCTION(BlueprintCallable)
	void StartJoinProcess(const FString& JoinSecret);

	void JoinGame();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Class Variables", meta = (AllowPrivateAccess = "true"))
	class UShooterGameInstance* ShooterGI;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Class Variables", meta = (AllowPrivateAccess = "true"))
	class UShooterSaveGame* SaveGame;

	void InitializeMenu();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UCanvasPanel* MainCanvas = nullptr;
	
};

