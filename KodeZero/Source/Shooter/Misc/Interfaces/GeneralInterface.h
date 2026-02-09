// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Shooter/StructTypes/PlayerIdsStruct.h"
#include "Shooter/StructTypes/CallbackStruct.h"
#include "Shooter/StructTypes/PlayerProgress.h"
#include "Shooter/EnumTypes/GameModeType.h"
#include "GeneralInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UGeneralInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTER_API IGeneralInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void GetWalletBalance(FPendingRequest Callback, const FString& Network);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void GetMenuSaveGame(const FString& SlotName, bool bCheckCreate, UShooterSaveGame*& SavedGameObj, bool& bSuccess);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void GetBlockchainSaveGame(const FString& SlotName, bool bCheckCreate, UBlockchainSaveGame*& SavedGameObj, bool& bSuccess);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SaveGame(UShooterSaveGame* SavedGameObj);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FPlayerID GetPlayerIds();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetIsTestMode();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetIsLocalServer();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FString GetEncodedString(const FString& InString);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	EGameModeType GetGameType();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetHasReward(const FString& UnlockId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FString GetBlockchainAPI();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	class UGameSettings* GetGameSettings();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool UpdatePlayerIds(const FPlayerID& NewIds);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdateTournamentParams(const FTournamentParams& NewTourParams);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void GetEncryptedKeys(const FString& CryptoKeys, const FString& PublicKey, FString &EncodedKey, FString &EncodedIV, FString &EncodedCipher, bool &bSuccess);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FString GenerateHMAC(const FString& Data, const FString& SecretKey);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FString GetBackendURL();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FPlayerProgress UpdateCurrentProgressInventory(FName RewardKey, int32 Value, bool bIsDelta);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FPlayerProgress UpdateCurrentProgressInventoryOrder();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void UpdateDiscordPresence(const FString& State, const FString& Details, const FString& LargeImageKey);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void WebSocketConnection(bool bIsOpen);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void CopyButtonClicked(const FString &CopyString);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MakeHTTPParams(const FString &Endpoint, const TMap<FString, FString> &QueryParams,
    	const FKeyValue &BodyParams, bool bIncludeCryptoKey, FString &Url, FString &Body);
};
