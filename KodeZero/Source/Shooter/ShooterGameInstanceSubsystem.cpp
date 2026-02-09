// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameInstanceSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

/*
void UShooterGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("Discord Initialize"));

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        ULocalPlayer* LocalPlayer = GameInstance->GetLocalPlayerByIndex(0);
        if (LocalPlayer)
        {
            SetupDiscord(LocalPlayer);
        }
        else
        {
            GameInstance->GetTimerManager().SetTimer(CheckPlayerTimer, this, &UShooterGameInstanceSubsystem::CheckForLocalPlayer, 0.1f, true);
        }
    }
}

void UShooterGameInstanceSubsystem::Deinitialize()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().ClearTimer(CheckPlayerTimer);
    }

    Super::Deinitialize();
}

void UShooterGameInstanceSubsystem::CheckForLocalPlayer()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        ULocalPlayer* LocalPlayer = GameInstance->GetLocalPlayerByIndex(0);
        if (LocalPlayer)
        {
            SetupDiscord(LocalPlayer);
            GameInstance->GetTimerManager().ClearTimer(CheckPlayerTimer);
        }
    }
}

void UShooterGameInstanceSubsystem::SetupDiscord(ULocalPlayer* LocalPlayer)
{
    Discord = ULocalPlayer::GetSubsystem<UDiscordLocalPlayerSubsystem>(LocalPlayer);
    if (Discord && Discord->Client)
    {
        // Set log callback (optional for debugging)
        auto LogCallback = FDiscordClientLogCallback::CreateUObject(this, &UShooterGameInstanceSubsystem::OnLogMessage);
        Discord->Client->AddLogCallback(LogCallback, EDiscordLoggingSeverity::Info);

        // Set status changed delegate (optional, for monitoring)
        FScriptDelegate StatusChanged;
        StatusChanged.BindUFunction(this, "OnStatusChanged");
        Discord->OnStatusChanged.Add(StatusChanged);

        // Set Application ID (key for no-auth RPC)
        Discord->Client->SetApplicationId(APPLICATION_ID);
        UpdatePresence(TEXT("Logging In"), TEXT("About to Log In..."), TEXT(""));

        UE_LOG(LogTemp, Log, TEXT("Discord Subsystem Initialized (No-Auth Mode)"));
    }
}

void UShooterGameInstanceSubsystem::OnLogMessage(FString Message, EDiscordLoggingSeverity Severity)
{
    UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *UEnum::GetValueAsString(Severity), *Message);
}

void UShooterGameInstanceSubsystem::OnStatusChanged(EDiscordClientStatus Status, EDiscordClientError Error, int32 ErrorDetail)
{
    UE_LOG(LogTemp, Log, TEXT("Discord Status: %s (Error: %s, Detail: %d)"), *UEnum::GetValueAsString(Status), *UEnum::GetValueAsString(Error), ErrorDetail);
}

void UShooterGameInstanceSubsystem::UpdatePresence(const FString& State, const FString& Details, const FString& LargeImageKey)
{
    if (!Discord || !Discord->Client) return;

    if (auto Activity = NewObject<UDiscordActivity>())
    {
        Activity->Init();
        Activity->SetType(EDiscordActivityTypes::Playing);
        Activity->SetState(State);    // e.g., "In Lobby"
        Activity->SetDetails(Details); // e.g., "Waiting for Players"
        //Activity->GetAssets()->SetLargeImage(LargeImageKey);  // e.g., "kode_zero_logo"

        // Optional: Timestamps, party for joinable status (but party/joins need auth)
        // Activity->SetStartTimestamp(FDateTime::UtcNow().ToUnixTimestamp());
        // Activity->GetParty()->SetId("unique_party_id");
        // Activity->GetParty()->GetSize()->SetCurrentSize(CurrentPlayers);
        // Activity->GetParty()->GetSize()->SetMaxSize(MaxPlayers);
        // Activity->GetSecrets()->SetJoin("unique_join_secret");  // For join handling (requires auth callbacks)

        Discord->Client->UpdateRichPresence(Activity, FDiscordClientUpdateRichPresenceCallback::CreateUObject(this, &UShooterGameInstanceSubsystem::OnRichPresenceUpdated)); 
    }
}

void UShooterGameInstanceSubsystem::OnRichPresenceUpdated(UDiscordClientResult* Result) 
{
  if (Result->Successful()) {
    UE_LOG(LogTemp, Log, TEXT("Rich Presence updated successfully!"));
  } else {
    UE_LOG(LogTemp, Error, TEXT("Rich Presence update failed"));
  }
}

*/