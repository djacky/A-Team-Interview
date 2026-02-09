// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
//#include "DiscordLocalPlayerSubsystem.h"
#include "ShooterGameInstanceSubsystem.generated.h"

/**
 * 
 */

#define APPLICATION_ID 1404546595013398618

UCLASS()
class SHOOTER_API UShooterGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	/*
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void UpdatePresence(const FString& State, const FString& Details, const FString& LargeImageKey);

private:

    UPROPERTY()
    UDiscordLocalPlayerSubsystem* Discord;

    void SetupDiscord(ULocalPlayer* LocalPlayer);
    UFUNCTION()
    void CheckForLocalPlayer();

    FTimerHandle CheckPlayerTimer;  // For local player check

    UFUNCTION()
    void OnLogMessage(FString Message, EDiscordLoggingSeverity Severity);

    UFUNCTION()
    void OnStatusChanged(EDiscordClientStatus Status, EDiscordClientError Error, int32 ErrorDetail);

	UFUNCTION()
	void OnRichPresenceUpdated(UDiscordClientResult* Result);
	*/
};
