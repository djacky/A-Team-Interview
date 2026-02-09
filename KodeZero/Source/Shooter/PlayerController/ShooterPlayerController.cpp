// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Shooter/ShooterCrosshairHUD.h"
#include "Shooter/Widgets/CharacterOverlay.h"
#include "Components/Image.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/Widgets/ReturnToMainMenu.h"
#include "Shooter/Widgets/PauseMenu.h"
#include "Shooter/Widgets/LobbyStatusWidget.h"
#include "Shooter/Widgets/ShowDamageIndicator.h"
#include "Shooter/ShooterCharacter.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Shooter/ShooterGameState.h"
#include "Shooter/GameMode/ShooterGameMode.h"
#include "Shooter/Widgets/MissileWarning.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpectatorPawn.h"
#include "Shooter/Widgets/StartMatchWidget.h"
#include "Shooter/MusicManager.h"
#include "Components/InputComponent.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Shooter/ShooterGameInstance.h"
#include "Shooter/Helicopter.h"
#include "Shooter/Widgets/HelicopterHealthBar.h"
#include "Shooter/Widgets/HelicopterInventory.h"
#include "Components/AudioComponent.h"
#include "Shooter/Widgets/TutorialWidget.h"
#include "Shooter/AI/ShooterAI.h"
#include "Shooter/Widgets/ShowTopPlayers.h"
#include "Shooter/Widgets/OnStillLoading.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "Shooter/Widgets/MapWidget.h"
#include "Shooter/AI/ShooterAI.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Shooter/Widgets/TeamMemberContainer.h"
#include "Shooter/Widgets/LastPlayerLocation.h"
#include "Globals.h"
#include "Shooter/Misc/Requests/RequestsObject.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LevelScriptActor.h"
#include "Shooter/Misc/XPComboComponent.h"
//#include "Shooter/PlayerController/TempInitialCamera.h"

AShooterPlayerController::AShooterPlayerController()
{
	MissileDamageBoxSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MissileBoxSound"));
	MissileDamageBoxSoundComponent->SetupAttachment(GetRootComponent());
	MissileDamageBoxSoundComponent->Deactivate();
	MissileDamageBoxSoundComponent->VolumeMultiplier = 2.f;
}

void AShooterPlayerController::BeginPlay()
{
    Super::BeginPlay();
	//SetCharacter();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(ShooterContext, 0);
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Bind the Move action
		EnhancedInputComponent->BindAction(PushToTalkAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::TalkButtonPressed);
		EnhancedInputComponent->BindAction(PushToTalkAction, ETriggerEvent::Completed, this, &AShooterPlayerController::TalkButtonReleased);
		EnhancedInputComponent->BindAction(ToggleMapAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::ToggleMapPressed);
	}

	if (IsLocalPlayerController())
	{
		FTimerHandle MemoryCheckTimer;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(MemoryCheckTimer, this, &AShooterPlayerController::StartCheckingMemory, 10.0f, true);
		}
	}

	//FTimerHandle MemoryLogTimerHandle;
	//GetWorld()->GetTimerManager().SetTimer(MemoryLogTimerHandle, this, &AShooterPlayerController::LogMemoryUsage, 5.0f, false);

}

void AShooterPlayerController::StartCheckingMemory()
{
    FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
	//UE_LOG(LogTemp, Warning, TEXT("Virtual Memory = %llu"), MemoryStats.AvailableVirtual);

    const uint64 CriticalThreshold = 512 * 1024 * 1024; // Example: 512 MB free
    if (MemoryStats.AvailableVirtual < CriticalThreshold)
    {
        UE_LOG(LogTemp, Warning, TEXT("Low virtual memory detected: %llu MB free."),
               MemoryStats.AvailableVirtual / (1024 * 1024));
		UWorld* World = GetWorld();
		if (GEngine && World)
		{
			GEngine->Exec(World, TEXT("sg.TextureQuality 1"));
			GEngine->Exec(World, TEXT("sg.ShadowQuality 0"));
			GEngine->Exec(World, TEXT("sg.PostProcessQuality 0"));
			GEngine->Exec(World, TEXT("sg.AntiAliasingQuality 0"));
		}
    }
}

void AShooterPlayerController::LogMemoryUsage()
{
    if (GEngine)
    {
        // Execute the memreport command
        //GEngine->Exec(GetWorld(), TEXT("memreport"));
		GEngine->Exec(GetWorld(), TEXT("stat memory"));
    }
}

void AShooterPlayerController::CheckStillLoadingWidget()
{
	if (!IsPracticeMode() && OnStillLoadingWidget)
	{
		OnStillLoadingWidget->RemoveFromParent();
		OnStillLoadingWidget = nullptr;
	}
}

void AShooterPlayerController::OpenStillLoadingWidget()
{
	if (!IsPracticeMode() && OnStillLoadingClass)
	{
		OnStillLoadingWidget = CreateWidget<UOnStillLoading>(this, OnStillLoadingClass);
		if (OnStillLoadingWidget)
		{
			OnStillLoadingWidget->AddToViewport();
		}
	}
}

void AShooterPlayerController::ClientSetCharacter_Implementation(float InMatchTime, float InLevelStartingTime, float InWarningTime)
{
	OpenStillLoadingWidget();
	MatchTime = InMatchTime;
	WarningMatchTime = InWarningTime;
	LevelStartingTime = InLevelStartingTime;
	SetCharacter();
	UpdateLumenParams();

    HTTPRequestObj = NewObject<URequestsObject>(this);
    if (HTTPRequestObj) HTTPRequestObj->OnRequestResponseDelegate.AddDynamic(this, &AShooterPlayerController::OnResponseReceived);
}

void AShooterPlayerController::SetCharacter()
{
	if (IsLocalPlayerController())
	{
		//AShooterGameState* ShooterGS = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));

		// Only call this when local, otherwise the server will call a Server RPC locally
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;

		if (ShooterGI)
		{
			if (ShooterGI->bPracticeMode && bMatchEnded) return;
			USkeletalMesh* SelectSkelMesh = ShooterGI->GetSelectedSkeletalMesh();

			if (SelectedTargetCharacterClass.Get() == nullptr)
			{
				// This only resolves to the defaults for when we did not select anything in the MainMenu selection screen
				SelectedTargetCharacterClass = ShooterGI->GetDefaultSelectedCharacter().LoadSynchronous();
			}
			if (SelectSkelMesh == nullptr)
			{
				SelectSkelMesh = ShooterGI->GetDefaultSelectedSkeletalMesh().LoadSynchronous();
			}

			UE_LOG(LogTemp, Display, TEXT("GamePC - Client calling ServerSetCharacter now!"));
			ServerSetCharacter(SelectedTargetCharacterClass, SelectSkelMesh);
		}
	}
}

void AShooterPlayerController::ServerSetCharacter_Implementation(TSubclassOf<AShooterCharacter> TargetCharacterClass, USkeletalMesh* TargetSkelMesh)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	ShooterGM = ShooterGM == nullptr ? Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)) : ShooterGM;
	
	if (ShooterGI && ShooterGM)
	{
    	FActorSpawnParameters SpawnParams;
    	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		if (TargetCharacterClass.Get() == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("GamePC - (ShouldNeverHappen) - Server did not receive a valid Character Selection from the client, resolving to server's default char"));

			TargetCharacterClass = ShooterGI->GetDefaultSelectedCharacter().LoadSynchronous();
		}
		if (TargetSkelMesh == nullptr)
		{
			TargetSkelMesh = ShooterGI->GetDefaultSelectedSkeletalMesh().LoadSynchronous();
		}

		if (TargetCharacterClass.Get() != nullptr && TargetSkelMesh != nullptr && GetWorld())
		{
			FRotator SpawnRotation{0.f, 0.f, 0.f};
			SpawnRotation.Yaw = FMath::RandRange(0, 360);
			int32 MaxIndex = ShooterGM->SpawnLocationPoints.Num() > 0 ? ShooterGM->SpawnLocationPoints.Num() - 1 : 0;
			int32 SpawnSelectionIndex = FMath::RandRange(0, MaxIndex);
			if (ShooterGM->SpawnLocationPoints.Num() > 0 && ShooterGM->SpawnLocationPoints.IsValidIndex(SpawnSelectionIndex))
			{
				//UE_LOG(LogTemp, Warning, TEXT("CharacterSpawnPoints Num > 0"));
				//UE_LOG(LogTemp, Warning, TEXT("Spawning Character at a spawn point"));
				if (AShooterCharacter* ShooterChar = GetWorld()->SpawnActor<AShooterCharacter>(TargetCharacterClass, ShooterGM->SpawnLocationPoints[SpawnSelectionIndex], SpawnRotation, SpawnParams))
				{
					UE_LOG(LogTemp, Warning, TEXT("GamePC - Server successfully responded to player selection with Character: %s"), *ShooterChar->GetName());
					ShooterChar->SetTargetSkeletalMeshOverride(TargetSkelMesh);
					ShooterChar->OnRep_TargetSkeletalMeshOverride(TargetSkelMesh);
					Possess(ShooterChar);
					MainCharacter = ShooterChar;
				}
			}
			else
			{
				FVector SpawnStartLocation = FVector{-2451.88f,1578.8f,238.5f};
				for (TActorIterator<APlayerStart> ActorIt(GetWorld()); ActorIt; ++ActorIt)
				{
					SpawnStartLocation = ActorIt->GetActorLocation();
					break;
				}
				if (AShooterCharacter* ShooterChar = GetWorld()->SpawnActor<AShooterCharacter>(TargetCharacterClass, SpawnStartLocation, SpawnRotation, SpawnParams))
				{
					UE_LOG(LogTemp, Warning, TEXT("GamePC - Server successfully responded to player selection with Character: %s"), *ShooterChar->GetName());
					ShooterChar->SetTargetSkeletalMeshOverride(TargetSkelMesh);
					ShooterChar->OnRep_TargetSkeletalMeshOverride(TargetSkelMesh);
					Possess(ShooterChar);
					MainCharacter = ShooterChar;
				}
			}
		}
	}
}

void AShooterPlayerController::StartTutorial()
{
    if (!IsPracticeMode() || PracticeTutorialClass == nullptr) return;
	//bTutorialStarted = true;
	PracticeTutorialWidget = CreateWidget<UTutorialWidget>(this, PracticeTutorialClass);
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (PracticeTutorialWidget && ShooterHUD)
	{
		PracticeTutorialWidget->TutorialSetup(ShooterHUD);
	}
}

void AShooterPlayerController::ServerSetOldStats_Implementation(AShooterPlayerState* ShooterPS)
{
	if (ShooterPS)
	{
		const FString ServerPlayerName = ShooterPS->GetPlayerName();
		//const FString ServerPlayerName = TEXT("Ack");
		//GetShooterPlayerState()->GetPlayerStats(ServerPlayerName);
	}
}

void AShooterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
	//SetHUDMatchTime(DeltaTime);
	InitInvalidHUD();

	CheckPing(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, FString::Printf(TEXT("FPS =  %f"), 1.f / GetWorld()->DeltaTimeSeconds), false, FVector2D(0.8f, 4.2f));
	//}
	
}

void AShooterPlayerController::CreateStartMatchWidget()
{
	if (StartMatchClass)
	{
		if (StartMatchWidget == nullptr && IsLocalPlayerController())
		{
			StartMatchWidget = CreateWidget<UStartMatchWidget>(this, StartMatchClass);
			if (StartMatchWidget)
			{
				StartMatchWidget->AddToViewport();
				StartMatchWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void AShooterPlayerController::HandleMatchHasStarted()
{
	//if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD)
	{
		if (ShooterHUD->CharacterOverlay == nullptr) 
		{
			ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
			ShooterHUD->AddCharacterOverlay();
			if (ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->BP_LargeMap &&
				ShooterGI && ShooterGI->GameType == EGameModeType::EGMT_Lobby)
			{
				ShooterHUD->CharacterOverlay->BP_LargeMap->RemoveFromParent();
			}
		}
    }
}

bool AShooterPlayerController::IsPracticeMode()
{
    ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
    if (ShooterGI)
    {
        return ShooterGI->bPracticeMode;
    }
    return false;
}

void AShooterPlayerController::InitInvalidHUD()
{
	if (CharacterOverlay == nullptr)
	{
		if (ShooterHUD && ShooterHUD->CharacterOverlay)
		{
			CharacterOverlay = ShooterHUD->CharacterOverlay;
		}
	}
}

// Updates HUD when character respawns
void AShooterPlayerController::UpdateHUDonRespawn()
{   
    bRespawnSwitch = !bRespawnSwitch;
    OnRep_RespawnSwitch();
}

void AShooterPlayerController::OnRep_RespawnSwitch()
{
	// Set shooter character for respawned player for HUD and all BP widgets
	if (IsLocalPlayerController())
	{
		ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn());
		if (ShooterHUD && ShooterChar)
		{
			ShooterHUD->SetShooterCharacter(ShooterChar);
		}
		RespawnDelegate.Broadcast(true);
	} 
}

void AShooterPlayerController::AddMissileHitWidget(FVector MissileLocation, bool bPlayAnimation)
{
	if (MissileWarningWidget)
	{
		ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		MissileWarning = CreateWidget<UMissileWarning>(this, MissileWarningWidget);
		if (MissileWarning && MissileWarning->WarningImage && ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->BP_LargeMap)
		{
			MissileWarning->StartMissileWarning(MissileLocation, bPlayAnimation, ShooterHUD->CharacterOverlay->BP_LargeMap);
			MissileWidgetArray.Add(MissileWarning);
			//UE_LOG(LogTemp, Warning, TEXT("Changing Opacity = %f"), OpacityValue);
		}
	}
}

void AShooterPlayerController::ClientBroadcastMissileWarning_Implementation(FVector MissileLocation)
{
	AddMissileHitWidget(MissileLocation, true);

	if (GetPawn() == nullptr) return;
	FVector DamageBoxScale = 100.f * FVector(416.f, 320.0f, 512.0f); //This is the DamageBoxMesh scale values of the missile (scaled by 100 because the static mesh for this is a 100cm cube)
	FVector PawnLocation = GetPawn()->GetActorLocation();

	if (PawnLocation.X >= MissileLocation.X - (DamageBoxScale.X / 2) &&
		PawnLocation.X <= MissileLocation.X + (DamageBoxScale.X / 2) &&
		PawnLocation.Y >= MissileLocation.Y - (DamageBoxScale.Y / 2) &&
		PawnLocation.Y <= MissileLocation.Y + (DamageBoxScale.Y / 2))
	{
		if (MissileWarningSound) UGameplayStatics::PlaySound2D(this, MissileWarningSound);
	}
	else
	{
		GeneralMessage(TEXT("Missile Launched"), TEXT("Red"), 0.f);
		if (MissileOutsideWarningSound) UGameplayStatics::PlaySound2D(this, MissileOutsideWarningSound);
	}
}

void AShooterPlayerController::ClientStopMissileWarning_Implementation()
{
	if (MissileWarning)
	{
		MissileWarning->StopMissileWarning();
	}
}

void AShooterPlayerController::ClientBroadcastPlayersAlive_Implementation(EGameMatchState State, int32 PlayersLeft, int32 MaxPlayers)
{
	LocalPlayersStillAlive = PlayersLeft;
	if (State == EGameMatchState::EMS_Idle)
	{
		CreateStartMatchWidget();
		//StartMatchWidget = StartMatchWidget == nullptr ? CreateWidget<UStartMatchWidget>(this, StartMatchClass) : StartMatchWidget;
		if (StartMatchWidget)
		{
			StartMatchWidget->UpdatePlayersLoggedIn(PlayersLeft, MaxPlayers);
		}
	}
	else if (State == EGameMatchState::EMS_Start)
	{
		ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		bool bShowPlayersCond = ShooterHUD && 
					ShooterHUD->CharacterOverlay && 
					ShooterHUD->CharacterOverlay->NumPlayersAlive;
		if (bShowPlayersCond)
		{
			// Ok to take PlayerArray().Num() here because this is just set right when begin battle starts
			FString RemainingPlayersStr = FString::Printf(TEXT("Cyberpunks: %i/%i"), PlayersLeft, MaxPlayers);
			ShooterHUD->CharacterOverlay->NumPlayersAlive->SetText(FText::FromString(RemainingPlayersStr));
		}
	}
}

void AShooterPlayerController::ClientUpdateHUDNumOfKills_Implementation(const FPlayerGameStats& InStats, AShooterCharacter* AttackerCharacter)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD)
	{
		//UE_LOG(LogTemp, Warning, TEXT("ClientUpdateHUDNumOfKills"));
		bool bShowNumOfKills = ShooterHUD->CharacterOverlay && 
			ShooterHUD->CharacterOverlay->NumKills &&
			ShooterHUD->CharacterOverlay->NumOfKillsAnim;
		if (bShowNumOfKills)
		{
			//UE_LOG(LogTemp, Warning, TEXT("ClientUpdateHUDNumOfKills Check Ok"));
			FString NumOfKillsStr = FString::Printf(TEXT("Number of Kills: %i"), InStats.NumOfKills);
			ShooterHUD->CharacterOverlay->NumKills->SetText(FText::FromString(NumOfKillsStr));
			ShooterHUD->CharacterOverlay->PlayAnimation(ShooterHUD->CharacterOverlay->NumOfKillsAnim);
		}

		if (InStats.KillStreak == 15)
		{
			FString StreakStr = FString::Printf(TEXT("Is this even fair?"), InStats.KillStreak);
			ShooterHUD->AddEpicNotify(TEXT("KillStreak"), FLinearColor::Red, TEXT("God"), TEXT("Mode"), StreakStr);
			if (EpicNotifySounds.IsValidIndex(4)) UGameplayStatics::PlaySound2D(this, EpicNotifySounds[4]);
		}
		else if (InStats.KillStreak == 10)
		{
			FString StreakStr = FString::Printf(TEXT("%i Kills!"), InStats.KillStreak);
			ShooterHUD->AddEpicNotify(TEXT("KillStreak"), FLinearColor::Yellow, TEXT("No"), TEXT("Mercy!!"), StreakStr);
			if (EpicNotifySounds.IsValidIndex(3)) UGameplayStatics::PlaySound2D(this, EpicNotifySounds[3]);
		}
		else if (InStats.KillStreak == 7)
		{
			FString StreakStr = FString::Printf(TEXT("They can't stop you"));
			ShooterHUD->AddEpicNotify(TEXT("KillStreak"), FLinearColor::Green, TEXT("Frag"), TEXT("Fury!"), StreakStr);
			if (EpicNotifySounds.IsValidIndex(2)) UGameplayStatics::PlaySound2D(this, EpicNotifySounds[2]);
		}
		else if (InStats.KillStreak == 5)
		{
			FString StreakStr = FString::Printf(TEXT("%i Eliminated"), InStats.KillStreak);
			ShooterHUD->AddEpicNotify(TEXT("KillStreak"), FLinearColor::White, TEXT("Net"), TEXT("Slayer"), StreakStr);
			if (EpicNotifySounds.IsValidIndex(1)) UGameplayStatics::PlaySound2D(this, EpicNotifySounds[1]);
		}
		else if (InStats.KillStreak == 3)
		{
			FString StreakStr = FString::Printf(TEXT("Keep Going!"));
			ShooterHUD->AddEpicNotify(TEXT("KillStreak"), FLinearColor::White, TEXT("Hot"), TEXT("Streak"), StreakStr);
			if (EpicNotifySounds.IsValidIndex(0)) UGameplayStatics::PlaySound2D(this, EpicNotifySounds[0]);
		}
	}

	if (AttackerCharacter != nullptr)
	{
		FTimerDelegate PlayElimmedSoundDel;
		PlayElimmedSoundDel.BindUFunction(this, FName("PlayElimmedCharSound"), AttackerCharacter);
		GetWorld()->GetTimerManager().SetTimer(PlayElimmedSoundTimer, PlayElimmedSoundDel, PlayElimmedSoundDelay, false);
	}
}

void AShooterPlayerController::GeneralMessage(FString Message, FString Color, float AddDelay, float RemoveTimeFactor)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD)
	{
		ShooterHUD->AddGeneralAnnouncement(Message, Color, AddDelay, RemoveTimeFactor);
	}
}

void AShooterPlayerController::ClientNotifyNewPlayer_Implementation(const FString& InPlayerName)
{
	FString PlayerMessage = InPlayerName + TEXT( " has joined...");
	GeneralMessage(PlayerMessage, TEXT("Purple"), 0.f);
}

void AShooterPlayerController::SetTextureStreamingEnabled(FString StreamingValue)
{
	FString Value;
	if (GConfig && GConfig->GetString(
		TEXT("/Script/Engine.RendererSettings"), 
		TEXT("r.TextureStreaming"), 
		Value, 
		GEngineIni))
	{
		// Write the modified value back to the config file
		GConfig->SetString(
		TEXT("/Script/Engine.RendererSettings"),
		TEXT("r.TextureStreaming"), 
		*StreamingValue, 
		GEngineIni);

		GConfig->Flush(false, GEngineIni);
	}
}

void AShooterPlayerController::PlayElimmedCharSound(AShooterCharacter* DamageCauserCharacter)
{
	if (DamageCauserCharacter->RandomCharElimmedSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), DamageCauserCharacter->RandomCharElimmedSound);
	}
}

void AShooterPlayerController::PlayMissileDamageBoxSound(bool bInDamageBox)
{
	if (bInDamageBox)
	{
		if (MissileDamageBoxSoundComponent)
		{
			if (!MissileDamageBoxSoundComponent->IsActive()) MissileDamageBoxSoundComponent->Activate();
			MissileDamageBoxSoundComponent->FadeIn(1.f);
		}
	}
	else
	{
		if (MissileDamageBoxSoundComponent)
		{
			MissileDamageBoxSoundComponent->FadeOut(2.f, 0.f);
		}
	}
}

void AShooterPlayerController::SetHUDCountdownTime(float CountdownTime)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	bool bHUDValid = ShooterHUD &&
		ShooterHUD->CharacterOverlay &&
		ShooterHUD->CharacterOverlay->Timer;
	if (bHUDValid && ShooterGI)
	{
		if (CountdownTime < 0.f || (ShooterGI->bPracticeMode && ShooterGI->GameAccessCheck.bIsFullVersion) || ShooterGI->GameType == EGameModeType::EGMT_Lobby)
		{
			ShooterHUD->CharacterOverlay->Timer->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("Time left: %02d:%02d"), Minutes, Seconds);
		ShooterHUD->CharacterOverlay->Timer->SetText(FText::FromString(CountdownText));
	}
}

void AShooterPlayerController::SetHUDTime()
{
	float TimeLeft, WarningTimeLeft = 0.f;
	TimeLeft = MatchTime - GetServerTime() + LevelStartingTime;
	WarningTimeLeft = WarningMatchTime - GetServerTime() + LevelStartingTime;

	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	
	if (HasAuthority())
	{
		ShooterGM = ShooterGM == nullptr ? Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this)) : ShooterGM;
		if (ShooterGM)
		{
			SecondsLeft = FMath::CeilToInt(ShooterGM->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		SetHUDCountdownTime(TimeLeft);
		if (StartMatchWidget) StartMatchWidget->SetTimer(WarningTimeLeft);
	}

	CountdownInt = SecondsLeft;
}

float AShooterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void AShooterPlayerController::CheckPing(float DeltaTime)
{
	if (HasAuthority()) return;
	HighPingRunningTime += DeltaTime;

	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<AShooterPlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
			{
				HighPingWarning(int32(PlayerState->GetCompressedPing() * 4));
				PingAnimationRunningTime = 0.f;
				//ServerReportPingStatus(true);
			}
			else
			{
				//ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		ShooterHUD && ShooterHUD->CharacterOverlay &&
		ShooterHUD->CharacterOverlay->HighPingAnimation &&
		ShooterHUD->CharacterOverlay->IsAnimationPlaying(ShooterHUD->CharacterOverlay->HighPingAnimation);

	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void AShooterPlayerController::HighPingWarning(int32 PingAmount)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	bool bHUDValid = ShooterHUD &&
		ShooterHUD->CharacterOverlay &&
		ShooterHUD->CharacterOverlay->HighPingImage &&
		ShooterHUD->CharacterOverlay->HighPingAnimation &&
		ShooterHUD->CharacterOverlay->PingAmount;
	if (bHUDValid)
	{
		FString PintAmountText = FString::Printf(TEXT("Ping: %i ms"), PingAmount);
		ShooterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		ShooterHUD->CharacterOverlay->PingAmount->SetText(FText::FromString(PintAmountText));
		ShooterHUD->CharacterOverlay->PingAmount->SetVisibility(ESlateVisibility::Visible);
		ShooterHUD->CharacterOverlay->PlayAnimation(
			ShooterHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5);
	}
}

void AShooterPlayerController::StopHighPingWarning()
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	bool bHUDValid = ShooterHUD &&
		ShooterHUD->CharacterOverlay &&
		ShooterHUD->CharacterOverlay->HighPingImage &&
		ShooterHUD->CharacterOverlay->HighPingAnimation &&
		ShooterHUD->CharacterOverlay->PingAmount;
	if (bHUDValid)
	{
		ShooterHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		ShooterHUD->CharacterOverlay->PingAmount->SetVisibility(ESlateVisibility::Hidden);
		if (ShooterHUD->CharacterOverlay->IsAnimationPlaying(ShooterHUD->CharacterOverlay->HighPingAnimation))
		{
			ShooterHUD->CharacterOverlay->StopAnimation(ShooterHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void AShooterPlayerController::ClientAIElimAnnouncement_Implementation(AShooterPlayerState* AttackerPS, EShooterDamageType ShooterDT, const FString& AIName)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	
	if (ShooterHUD && Self && !bMatchEnded)
	{
		if (Self == AttackerPS)
		{
			ShooterHUD->AddElimAnnouncement("You", AIName, false);
		}
		ShooterHUD->AddElimAnnouncementSmall(AttackerPS->GetPlayerName(), AIName, ShooterDT, false);
	}
}

void AShooterPlayerController::ClientElimAnnouncementSmall_Implementation(AShooterPlayerState* Attacker, AShooterPlayerState* Victim, EShooterDamageType ShooterDT, bool bTeamMode)
{
    if (Attacker && Victim && !bMatchEnded)
    {
		//UE_LOG(LogTemp, Warning, TEXT("ClientElimAnnouncement Check Ok"));
        ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		if (ShooterHUD)
        {
			ShooterHUD->AddElimAnnouncementSmall(Attacker->GetPlayerName(), Victim->GetPlayerName(), ShooterDT, bTeamMode);
		}
	}
}

void AShooterPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim, bool bTeamMode)
{
    ClientElimAnnouncement(Attacker, Victim, bTeamMode);
}

void AShooterPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim, bool bTeamMode)
{
    APlayerState* Self = GetPlayerState<APlayerState>();
	//UE_LOG(LogTemp, Warning, TEXT("ClientElimAnnouncement"));
    if (Attacker && Victim && Self)
    {
		//UE_LOG(LogTemp, Warning, TEXT("ClientElimAnnouncement Check Ok"));
        ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		
        if (ShooterHUD)
        {
			if (!bTeamMode)
			{
				//UE_LOG(LogTemp, Warning, TEXT("ClientElimAnnouncement Hud ok"));
				if (Attacker == Self && Victim != Self)
				{
					ShooterHUD->AddElimAnnouncement("You", Victim->GetPlayerName(), bTeamMode);
					return;
				}
				if (Victim == Self && Attacker != Self)
				{
					ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "you", bTeamMode);
					return;
				}
				if (Attacker == Victim && Attacker == Self)
				{
					ShooterHUD->AddElimAnnouncement("You", "yourself", bTeamMode);
					return;
				}
				if (Attacker == Victim && Attacker != Self)
				{
					ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves", bTeamMode);
					return;
				}
				ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName(), bTeamMode);
			}
			else
			{
				ShooterHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName(), bTeamMode);
			}
        }
		
    }
}

void AShooterPlayerController::ClientShowAttackerLocation_Implementation(AActor* AttackerActor, AActor* DamagedActor, float DamageAmount)
{
    if (DamageIndicatorClass == nullptr) return;

	//UE_LOG(LogTemp, Warning, TEXT("ClientShowAttackerLocation"));
	UShowDamageIndicator* DamageIndicatorWidget = CreateWidget<UShowDamageIndicator>(this, DamageIndicatorClass);
	if (DamageIndicatorWidget)
	{
		DamageIndicatorWidget->ShowDamageWidget(AttackerActor, DamagedActor, DamageAmount);
	}
}

void AShooterPlayerController::ShowAttackerLocationMissed(AActor* AttackerActor, FVector ImpactPoint)
{
    if (DamageIndicatorClass == nullptr) return;

	UShowDamageIndicator* WarningIndicatorWidget = CreateWidget<UShowDamageIndicator>(this, DamageIndicatorClass);
	if (WarningIndicatorWidget)
	{
		WarningIndicatorWidget->ShowWarningWidget(AttackerActor, GetPawn(), ImpactPoint);
	}
}

void AShooterPlayerController::ManageInventory(bool bManageInventory)
{
    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD && ShooterHUD->CharacterOverlay)
    {
		if (bManageInventory)
		{
			FInputModeGameAndUI InputModeData;
			SetInputMode(InputModeData);
			SetShowMouseCursor(true);
			if (ShooterHUD->CharacterOverlay->InventoryBorderAnim && 
				ShooterHUD->CharacterOverlay->InventoryBorder &&
				ShooterHUD->CharacterOverlay->InventoryBorder && ManageInventorySound)
			{
				UGameplayStatics::PlaySound2D(this, ManageInventorySound);
				ShooterHUD->CharacterOverlay->PlayAnimation(ShooterHUD->CharacterOverlay->InventoryBorderAnim, 0.f, 0);
				ShooterHUD->CharacterOverlay->InventoryBorder->SetVisibility(ESlateVisibility::Visible);
				ShooterHUD->CharacterOverlay->ReleaseItemScreen->SetVisibility(ESlateVisibility::Visible);
				ShooterHUD->CharacterOverlay->PlayAnimation(ShooterHUD->CharacterOverlay->ReleaseItemScreenAnim, 0.f, 1);
			}
		}
		else
		{
			FInputModeGameOnly InputModeData;
			SetInputMode(InputModeData);
			SetShowMouseCursor(false);
			if (ShooterHUD->CharacterOverlay->InventoryBorderAnim && 
				ShooterHUD->CharacterOverlay->IsAnimationPlaying(ShooterHUD->CharacterOverlay->InventoryBorderAnim) && 
				ShooterHUD->CharacterOverlay->InventoryBorder)
			{
				ShooterHUD->CharacterOverlay->StopAnimation(ShooterHUD->CharacterOverlay->InventoryBorderAnim);
				ShooterHUD->CharacterOverlay->InventoryBorder->SetVisibility(ESlateVisibility::Hidden);
				ShooterHUD->CharacterOverlay->ReverseAnimation(ShooterHUD->CharacterOverlay->ReleaseItemScreenAnim);
				ShooterHUD->CharacterOverlay->ReleaseItemScreen->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void AShooterPlayerController::ShowPauseMenu(bool bGamePaused, const TArray<FPlayerGameStats>& PlayerStatsArray)
{
	if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
	{
		PauseMenuHandle = StreamableManager.RequestAsyncLoad(
			PauseMenuClass.ToSoftObjectPath(),
            FStreamableDelegate::CreateLambda([this, bGamePaused, PlayerStatsArray]()
            {
                this->OnPauseMenuClassLoaded(bGamePaused, PlayerStatsArray);
            })
		);
	}
	else
	{
		OnPauseMenuClassLoaded(bGamePaused, PlayerStatsArray);
	}
}

void AShooterPlayerController::OnPauseMenuClassLoaded(bool bGamePaused, const TArray<FPlayerGameStats>& PlayerStatsArray)
{
	TSubclassOf<UUserWidget> LoadedClass = PauseMenuClass.Get();
	if (!LoadedClass) return;

	if (bGamePaused)
	{
		if (ShooterHUD && ShooterHUD->CharacterOverlay)
			ShooterHUD->CharacterOverlay->ShowHUD(false);

		if (!PauseMenuWidget)
		{
			PauseMenuWidget = CreateWidget<UPauseMenu>(this, LoadedClass);
			ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;

			if (PauseMenuWidget && ShooterGI)
			{
				PauseMenuWidget->PauseMenuSetup(PlayerStatsArray, IsPracticeMode(), ShooterGI->bTeamMode, HasAuthority());
			}
		}
	}
	else
	{
		if (PauseMenuWidget)
		{
			PauseMenuWidget->ResumeButtonClicked();
		}
	}
}

void AShooterPlayerController::ResetPauseMenuHandle()
{
	if (PauseMenuHandle.IsValid())
	{
		PauseMenuHandle->ReleaseHandle();
		PauseMenuHandle.Reset();
	}
}

void AShooterPlayerController::UpdateLobbyReady(const TArray<FString>& LobbyIdArray)
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->UpdateLobbyReady(LobbyIdArray);
	}
}

void AShooterPlayerController::SendPlayerFromLobby(const FLobbyInfo& LobbyServerInfo)
{
	ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
	if (ShooterGI)
	{
		OpenStillLoadingWidget();
		DestroyEOSGameSession();
		ShooterGI->SetNetDriver(false);
		FString TravelCommand = FString::Printf(TEXT("open %s:%i"), *LobbyServerInfo.IpAddress, LobbyServerInfo.Port);
		UE_LOG(LogTemp, Warning, TEXT("SendPlayerToMatch: %s"), *TravelCommand);
		const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
		ServerUpdateLobbyIdArray(Ids.PlayerID);
		ShooterGI->LobbyServerInfo = LobbyServerInfo;
		ShooterGI->GameType = LobbyServerInfo.GameType;
		switch (LobbyServerInfo.GameType)
		{
			case EGameModeType::EGMT_freeSolo:
				ShooterGI->bTeamMode = false;
				break;
			case EGameModeType::EGMT_freeTeam:
				ShooterGI->bTeamMode = true;
				break;
			default:
				ShooterGI->bTeamMode = false;
				break;
		}
		ConsoleCommand(TravelCommand);
		//GetGameInstance()->ReturnToMainMenu();
	}
}

void AShooterPlayerController::ServerUpdateLobbyIdArray_Implementation(const FString& ClientPlayerId)
{
	if (AShooterGameState* ShooterGS = GetShooter_GS())
	{
		ShooterGS->UpdateLobbyIdArray(false, ClientPlayerId);
	}
}

// This is only being used in practice mode
void AShooterPlayerController::ShowReturnToMainMenu(bool bWinner, FPlayerGameStats PlayerGameStats, int32 PlayersLeft, int32 RespawnsLeft)
{
    if (ReturnToMainMenuWidget == nullptr) return;
    if (ReturnToMainMenu == nullptr)
    {
        ReturnToMainMenu = CreateWidget<UReturnToMainMenu>(this, ReturnToMainMenuWidget);
        if (ReturnToMainMenu)
        {
			AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
			if (bMatchEnded)
			{
				FString PlayerStatStr = FString::Printf(TEXT("Game Over %s!"), *ShooterPlayerState->GetPlayerName());
				FString PlayerPlacementStr = FString::Printf(TEXT("Nice job...Now unlock the full version for PvP action!"));
				ReturnToMainMenu->MenuSetup(PlayerStatStr, PlayerPlacementStr, PlayerGameStats, this, RespawnsLeft);
				if (WinnerSound) UGameplayStatics::PlaySound2D(this, WinnerSound);
			}
			else if (bWinner && ShooterPlayerState)
			{
				FString PlayerStatStr = FString::Printf(TEXT("Nice! You won %s!"), *ShooterPlayerState->GetPlayerName());
				FString PlayerPlacementStr = FString::Printf(TEXT("Finished first place!"));
				ReturnToMainMenu->MenuSetup(PlayerStatStr, PlayerPlacementStr, PlayerGameStats, this, RespawnsLeft);
			}
			else if (!bWinner && ShooterPlayerState)
			{
				AShooterGameState* ShooterGS = GetShooter_GS();
				FString PlayerStatStr = FString::Printf(TEXT("You Lost %s..."), *ShooterPlayerState->GetPlayerName());
				FString PlayerPlacementStr = FString::Printf(TEXT("Unknown Placement..."));
				if (ShooterGS)
				{
					PlayerPlacementStr = FString::Printf(TEXT("Finished in position: %i"), PlayersLeft);
				}
				ReturnToMainMenu->MenuSetup(PlayerStatStr, PlayerPlacementStr, PlayerGameStats, this, RespawnsLeft);
			}
			else
			{
				FString PlayerStatStr = FString::Printf(TEXT("Unknown status..."));
				FString PlayerPlacementStr = FString::Printf(TEXT("Unknown placement..."));
				ReturnToMainMenu->MenuSetup(PlayerStatStr, PlayerPlacementStr, PlayerGameStats, this, RespawnsLeft);
			}
        }
    }

    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD && ShooterHUD->CharacterOverlay)
    {
        ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Hidden);
        ShooterHUD->CharacterOverlay->WBP_StatsBar->SetVisibility(ESlateVisibility::Hidden);
        //ShooterHUD->CharacterOverlay->BP_MiniMap->RemoveFromParent();
        if (ShooterHUD->CharacterOverlay->BP_LargeMap) ShooterHUD->CharacterOverlay->BP_LargeMap->SetVisibility(ESlateVisibility::Hidden);
		//ShowAICountBox(false);
        ShooterHUD->bShowHUD = false;
    }
}

void AShooterPlayerController::UpdateLobbyStatus(const FString& ServerStatus)
{
    if (LobbyStatusClass == nullptr) return;

	if (LobbyStatusWidget == nullptr)
	{
		LobbyStatusWidget = CreateWidget<ULobbyStatusWidget>(this, LobbyStatusClass);
		if (LobbyStatusWidget)
		{
			LobbyStatusWidget->UpdateStatus(ServerStatus);
			LobbyStatusWidget->AddToViewport();
		}
	}
	else
	{
		LobbyStatusWidget->UpdateStatus(ServerStatus);
		LobbyStatusWidget->AddToViewport();
	}
}

void AShooterPlayerController::SetInventoryVisibility(bool bIsVisible)
{
    if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->BP_InventoryBar)
    {
		if (bIsVisible)
		{
			ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AShooterPlayerController::DestroyReturnToMainMenu()
{
	if (ReturnToMainMenu)
	{
		ReturnToMainMenu->RemoveFromParent();
		ReturnToMainMenu = nullptr;
	}
}

void AShooterPlayerController::PracticeRestart()
{
	SetCharacter();
	OnRep_MatchState();
	//ServerUpdateHUDonRespawn();
	//UpdateHUDonRespawn();
	ReturnToMainMenu = nullptr;

    ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD && ShooterHUD->CharacterOverlay)
    {
        //ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Visible);
        ShooterHUD->CharacterOverlay->WBP_StatsBar->SetVisibility(ESlateVisibility::Visible);
        //ShooterHUD->CharacterOverlay->BP_MiniMap->RemoveFromParent();
        //ShooterHUD->CharacterOverlay->BP_LargeMap->SetVisibility(ESlateVisibility::Visible);
        ShooterHUD->bShowHUD = true;
    }
	if (IsPracticeMode())
	{
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS)
		{
			ShooterGS->CheckPracticeContainers();
			ShooterGS->StopAISearch();
			ShooterGS->DestroyAIs();
			UpdateAICount(0);
		}
	}
}

void AShooterPlayerController::ServerUpdateHUDonRespawn_Implementation(bool bMatchHasStarted, const FString &UserPlayerId)
{
	if (bMatchHasStarted)
	{
		UpdateHUDonRespawn();
	}
	else
	{
		// Only call this once when a player first enters match. This will then be updated with the UpdateElimmedPlayerCount function in GameState
		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS) ClientSetHUDPlayersAtStart(ShooterGS->GetPlayersStillAlive());
		PlayerId = UserPlayerId;
	}
}

void AShooterPlayerController::ClientRespawn_Implementation(const FVector& LastPlayerLocation)
{
	PracticeRestart();
	SetLastPlayerLocation(LastPlayerLocation);
}

void AShooterPlayerController::SetLastPlayerLocation(const FVector& LastPlayerLocation)
{
	if (!LastPlayerLocationClass) return;
	if (!LastPlayerLocationWidget)
	{
		LastPlayerLocationWidget = CreateWidget<ULastPlayerLocation>(this, LastPlayerLocationClass);
		if (LastPlayerLocationWidget && ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer && ShooterHUD->CharacterOverlay->BP_LargeMap)
		{
			LastPlayerLocationWidget->SetLastPlayerLocation(LastPlayerLocation, ShooterHUD->CharacterOverlay->BP_LargeMap);
		}
	}
	else
	{
		LastPlayerLocationWidget->UpdateLastPlayerLocation(LastPlayerLocation);
	}
}

void AShooterPlayerController::UpdateAICount(int32 AICount)
{
	if (IsPracticeMode() && ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->NumAIAliveText)
	{
		FString RemainingAIStr = FString::Printf(TEXT("%i/%i"), AICount, 10);
		ShooterHUD->CharacterOverlay->NumAIAliveText->SetText(FText::FromString(RemainingAIStr));
	}
}

void AShooterPlayerController::ShowAICountBox(bool bShowBox)
{
	if (IsPracticeMode() && ShooterHUD && ShooterHUD->CharacterOverlay && 
		ShooterHUD->CharacterOverlay->AIShootersBox && ShooterHUD->CharacterOverlay->NumAIAliveText)
	{
		if (bShowBox)
		{
			ShooterHUD->CharacterOverlay->AIShootersBox->SetVisibility(ESlateVisibility::Visible);
			FString RemainingAIStr = FString::Printf(TEXT("%i/%i"), 0, 10);
			ShooterHUD->CharacterOverlay->NumAIAliveText->SetText(FText::FromString(RemainingAIStr));
		}
		else
		{
			ShooterHUD->CharacterOverlay->AIShootersBox->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void AShooterPlayerController::LoadSavedGame()
{
	if (IsLocalPlayerController())
	{
		bool bSaveGameSuccess = false;
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI) ShooterGI->LoadGame(SaveGame, bSaveGameSuccess);
	}
}

void AShooterPlayerController::OnMatchStateSet(EGameMatchState State)
{
	if (MatchState != State)
	{
		MatchState = State;
		OnRep_MatchState();
	}
}

void AShooterPlayerController::OnRep_MatchState()
{
	AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn());
	LoadSavedGame();
	switch (MatchState)
	{
	case EGameMatchState::EMS_Idle:
		CreateStartMatchWidget();
		if (StartMatchWidget)
		{
			StartMatchWidget->StartIdleNotification();
		}
		break;
	case EGameMatchState::EMS_Warn:
		CreateStartMatchWidget();
		if (ShooterChar)
		{
			ShooterChar->StartMatchEffects();
		}
		//StartMatchWidget = StartMatchWidget == nullptr ? CreateWidget<UStartMatchWidget>(this, StartMatchClass) : StartMatchWidget;
		if (StartMatchWidget && ShooterChar && ShooterChar->IsLocallyControlled())
		{
			StartMatchWidget->StartWarningNotification();
		}
		if (ShooterChar && MusicClass && ShooterChar->IsLocallyControlled())
		{
			MusicActor = GetWorld()->SpawnActor<AMusicManager>(MusicClass, ShooterChar->GetTransform());
		}
		#if !WITH_EDITOR
			if (SaveGame)
			{
				if (MusicActor && SaveGame->bMusicGame && !SaveGame->bBackgroundMusic)
				{
					MusicActor->PlayNextSong();
					MusicActor->GetAudioComponent()->AdjustVolume(0.f, SaveGame->MusicGameVolume);
				}
				else if (MusicActor && !SaveGame->bMusicGame && SaveGame->bBackgroundMusic && !IsPracticeMode())
				{
					MusicActor->PlayBackgroundMusic();
					MusicActor->GetBackgroundAudioComponent()->AdjustVolume(0.f, SaveGame->MusicGameBackgroundVolume);
				}
			}
		#endif
		break;
	case EGameMatchState::EMS_Start:
		if (!IsLocalPlayerController()) return;
		if (ShooterChar)
		{
			StartMatchLogic(ShooterChar);
		}
		else
		{
    		FTimerHandle StartMatchTimer;
    		GetWorldTimerManager().SetTimer(StartMatchTimer, this, &AShooterPlayerController::OnRep_MatchState, 0.5f);
		}
		break;
	case EGameMatchState::EMS_Stop:
		if (IShooterInterface* ShooterInterface = Cast<IShooterInterface>(GetPawn()))
		{
			ShooterInterface->Execute_OnMatchEnded(GetPawn(), this);
		}
		if (IsLocalPlayerController()) bMatchEnded = true;
		break;
	}
}

void AShooterPlayerController::StartMatchLogic(AShooterCharacter* ShooterChar)
{
	if (StartMatchWidget) StartMatchWidget->RemoveFromParent();
	if (ShooterChar)
	{
		//AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
		/*
		if (StartMatchWidget)
		{
			StartMatchWidget->StartBattleNotification();
		}
		*/
		
		#if !WITH_EDITOR
			if (!MusicActor)
			{
				if (MusicClass && ShooterChar->IsLocallyControlled())
				{
					MusicActor = GetWorld()->SpawnActor<AMusicManager>(MusicClass, ShooterChar->GetTransform());
				}
			}
		#endif		

		HandleMatchHasStarted();
		StopHighPingWarning();
		ShooterChar->GetHudUpdateAmmoDelegate().Broadcast();
		StartTutorial();

		const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(this));
		ServerUpdateHUDonRespawn(bMatchStarted, Ids.PlayerID);

		//SetTextureStreamingEnabled(TEXT("True"));


		AShooterGameState* ShooterGS = GetShooter_GS();
		if (ShooterGS && ShooterGS->MissileHitLocations.Num() > 0 && !bMatchStarted)
		{
			for (auto HitLocation : ShooterGS->MissileHitLocations)
			{
				AddMissileHitWidget(HitLocation, false);
			}
		}

		if (!bMatchStarted)
		{
			#if !WITH_EDITOR
				if (SaveGame)
				{
					if (MusicActor && !MusicActor->bMusicPlaying && SaveGame->bMusicGame && !SaveGame->bBackgroundMusic)
					{
						MusicActor->PlayNextSong();
						MusicActor->GetAudioComponent()->AdjustVolume(0.f, SaveGame->MusicGameVolume);
					}
					else if (MusicActor && !MusicActor->bBackgroundMusicPlaying && !SaveGame->bMusicGame && SaveGame->bBackgroundMusic && SaveGame->MusicGameBackgroundVolume > 0.f && !IsPracticeMode())
					{
						MusicActor->PlayBackgroundMusic();
						MusicActor->GetBackgroundAudioComponent()->AdjustVolume(0.f, SaveGame->MusicGameBackgroundVolume);
					}
				}
			#endif
		}

		bMatchStarted = true;

		if (IsPracticeMode()) ShooterChar->SetCanTarget(true);
		
		// Not using the old stats anymore here. 
		//ServerSetOldStats(GetShooterPlayerState()); 
	}
}

void AShooterPlayerController::RemoveHeliHUDElements()
{
	if (IsLocalPlayerController())
	{
		if (HelicopterHealthBar) HelicopterHealthBar->RemoveFromParent();
		if (HelicopterInventoryBar) HelicopterInventoryBar->RemoveFromParent();
	}
}

void AShooterPlayerController::ClientSetHUDPlayersAtStart_Implementation(int32 NumPlayersAlive)
{
	AShooterGameState* ShooterGS = GetShooter_GS();

	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	bool bShowPlayersCond = ShooterGS && ShooterHUD && 
			ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->NumPlayersAlive;
	if (bShowPlayersCond)
	{
		// Ok to take PlayerArray().Num() here because this is just set right when begin battle starts
		FString RemainingPlayersStr;
		if (IsPracticeMode())
		{
			RemainingPlayersStr = FString::Printf(TEXT("Cyberpunks: %i"), 1);
		}
		else
		{
			//RemainingPlayersStr = FString::Printf(TEXT("Cyberpunks: %i/%i"), ShooterGS->PlayerArray.Num(), ShooterGS->GameMaxPlayers);
			RemainingPlayersStr = FString::Printf(TEXT("Cyberpunks: %i/%i"), NumPlayersAlive, ShooterGS->GameMaxPlayers);
		}
		ShooterHUD->CharacterOverlay->NumPlayersAlive->SetText(FText::FromString(RemainingPlayersStr));
		ShowAICountBox(true);
	}
}

AShooterPlayerState* AShooterPlayerController::GetShooterPlayerState() const
{
    return CastChecked<AShooterPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

AShooterGameState* AShooterPlayerController::GetShooter_GS() const
{
	UWorld* World = GetWorld();
	if (World) return World->GetGameState<AShooterGameState>();
	return nullptr;
}

void AShooterPlayerController::PreInitializeComponents()
{
    Super::PreInitializeComponents();
}

void AShooterPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
	
	if (IsLocalPlayerController())
	{
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		#if WITH_CLIENT_ONLY
			if (ShooterGI && !ShooterGI->bStoppingVivox) ShooterGI->VivoxStop();
		#endif
	}
    if (GetPawn() && HasAuthority())
    {
        AShooterCharacter* MyChar = Cast<AShooterCharacter>(GetPawn());
        if (MyChar)
        {
            MyChar->DropAllItems();
        }
    }
}

void AShooterPlayerController::OnPlayerLeftGame(APlayerState* PlayerLeftPS)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer && ShooterHUD->CharacterOverlay->BP_LargeMap)
	{
		ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer->OnPlayerLeftGame(PlayerLeftPS);
		ShooterHUD->CharacterOverlay->BP_LargeMap->DestroyTeamMember(PlayerLeftPS);
	}
}

void AShooterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
	
	if (HasAuthority() && !IsPracticeMode())
	{
		AShooterGameState* ShooterGS = GetShooter_GS();
		AShooterPlayerState* ShooterPS = GetShooterPlayerState();
		if (ShooterGS && ShooterPS)
		{
			ShooterGS->NotifyPlayerJoined(ShooterPS, ShooterPS->GetPlayerName());
		}
	}
	
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void AShooterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AShooterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AShooterPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (APawn* CurrentPawn = GetPawn())
    {
        const FRotator MovementRotation(0.0f, GetControlRotation().Yaw, 0.0f);
        const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
        // CurrentPawn->AddMovementInput(MovementDirection, 1.0f);
    }
}

void AShooterPlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
    ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AShooterPlayerController::OnPlayerStateChanged()
{
    // Empty, place for derived classes to implement without having to hook all the other events
}

void AShooterPlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IShooterTeamAgentInterface* PlayerStateTeamInterface = Cast<IShooterTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IShooterTeamAgentInterface* PlayerStateTeamInterface = Cast<IShooterTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}

void AShooterPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShooterPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShooterPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	auto ShooterChar = Cast<AShooterCharacter>(InPawn);
	auto HeliChar = Cast<AHelicopter>(InPawn);

	if (HeliChar)
	{
		ClientUpdateHUD(true);

		FTimerDelegate StartHeliDel;
		FTimerHandle StartHeliTimer;
		StartHeliDel.BindUFunction(this, FName("StartHeliPossess"), HeliChar);
		GetWorld()->GetTimerManager().SetTimer(StartHeliTimer, StartHeliDel, 0.1f, false);

	}
	else if (ShooterChar)
	{
		const USkeletalMeshSocket* RootSocket = ShooterChar->GetMesh()->GetSocketByName("Root");
		if (RootSocket)
		{
			RootSocket->AttachActor(PlayerState, ShooterChar->GetMesh());
		}

		if (ShooterChar->OwnedPawn == nullptr)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Possessing Shooter Character"));
			ShooterChar->SetOnPossess();

			FTimerDelegate IdleModeDel;
			FTimerHandle IdleTimer;
			IdleModeDel.BindUFunction(this, FName("StartOnPossess"), ShooterChar);
			GetWorld()->GetTimerManager().SetTimer(IdleTimer, IdleModeDel, 0.25f, false);
		}
		else
		{
			ClientUpdateHUD(false);
			ShooterChar->OwnedPawn = nullptr;
			if (MatchState == EGameMatchState::EMS_Stop)
			{
				ShooterChar->StopMatch();
			}
		}
	}
}

void AShooterPlayerController::StartHeliPossess(AHelicopter* PossessedHelicopter)
{
	PossessedHelicopter->ServerStartPossess();
}

void AShooterPlayerController::StartOnPossess(AShooterCharacter* ShooterChar)
{
	if (IsPracticeMode()) return;
	if (!HasAuthority()) return;

	AShooterGameState* ShooterGS = GetShooter_GS();
	//AShooterGameMode* ShooterGM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode());
	AShooterPlayerState* ShooterPS = GetShooterPlayerState();

	// If match hasn't started yet, set flying without a timer, and stop flying when enough players have entered the session
	if (ShooterPS && !ShooterPS->bHasBeenElimmed)
	{
		//if (ShooterGS) UE_LOG(LogTemp, Warning, TEXT("StartOnPossess: %i"), ShooterGS->GameMatchState);
		if (ShooterChar)
		{
			if (ShooterGS)
			{
				if (ShooterGS->GameMatchState <= EGameMatchState::EMS_Warn)
				{
					ShooterChar->CheckMatchState();
				}
				else if (ShooterGS->GameMatchState == EGameMatchState::EMS_Start)
				{
					ShooterChar->OnPossessRespawn();
				}
			}
		}
	}
	// otherwise, player temporarily goes into flying mode when they get killed and respawn
	else
	{
		if (ShooterChar)
		{
			ShooterChar->OnPossessRespawn();
		}
	}
}

void AShooterPlayerController::ClientUpdateHUD_Implementation(bool bHeliHUD)
{
	if (HelicopterHealthBarWidget && HelicopterInventoryBarWidget)
	{
		ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
		bool bValidHUD = ShooterHUD && ShooterHUD->CharacterOverlay;
		if (bHeliHUD && bValidHUD)
		{
			HelicopterHealthBar = CreateWidget<UHelicopterHealthBar>(this, HelicopterHealthBarWidget);
			if (HelicopterHealthBar)
			{
				HelicopterHealthBar->HelicopterHealthBar(bHeliHUD);
			}
			HelicopterInventoryBar = CreateWidget<UHelicopterInventory>(this, HelicopterInventoryBarWidget);
			if (HelicopterInventoryBar)
			{
				HelicopterInventoryBar->HelicopterInventoryBar(bHeliHUD);
			}
			ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Hidden);
		}
		else if (!bHeliHUD && bValidHUD)
		{
			if (HelicopterHealthBar)
			{
				HelicopterHealthBar->RemoveFromParent();
			}

			if (HelicopterInventoryBar)
			{
				HelicopterInventoryBar->RemoveFromParent();
			}

			ShooterHUD->SetHelicopter(nullptr);
			
			if (MatchState == EGameMatchState::EMS_Stop)
			{
				auto ShooterChar = Cast<AShooterCharacter>(GetPawn());
				if (ShooterChar) ShooterChar->StopMatch();
			}
			else
			{
				ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void AShooterPlayerController::ClientShowTopPlayers_Implementation(const TArray<FPlayerGameStats>& InPlayerStatsArray, bool bIsWinner, const TArray<FString>& WinnerNames, EGameModeType GameModeType)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (IsLocalPlayerController() && ShooterHUD && ShooterHUD->CharacterOverlay)
	{
		ShooterHUD->CharacterOverlay->BP_InventoryBar->SetVisibility(ESlateVisibility::Hidden);
		ShooterHUD->CharacterOverlay->WBP_StatsBar->SetVisibility(ESlateVisibility::Hidden);
		//ShooterHUD->CharacterOverlay->BP_MiniMap->RemoveFromParent();
		if (ShooterHUD->CharacterOverlay->BP_LargeMap) ShooterHUD->CharacterOverlay->BP_LargeMap->SetVisibility(ESlateVisibility::Hidden);
		ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer->SetVisibility(ESlateVisibility::Hidden);
		//ShowAICountBox(false);
		//ShooterHUD->bShowHUD = false;
		//if (WinnerSound && !bIsWinner) UGameplayStatics::PlaySound2D(this, WinnerSound);

		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI)
		{
			LoadSavedGame();
			if (SaveGame)
			{
				AShooterPlayerState* ShooterPlayerState = GetShooterPlayerState();
				for (auto PlayerStat : InPlayerStatsArray)
				{
					if (ShooterPlayerState && ShooterPlayerState->GetPlayerName() == PlayerStat.PlayerName)
					{
						FPlayerStats CurrentStats;
						if (SaveGame->SavePlayerStats.Contains(PlayerStat.PlayerName))
						{
							CurrentStats = SaveGame->SavePlayerStats[PlayerStat.PlayerName];
						}
						CurrentStats.DamageDealt += PlayerStat.DamageDealt;
						CurrentStats.DamageTaken += PlayerStat.DamageTaken;
						CurrentStats.FireCharacterHit += PlayerStat.ShotsHit;
						CurrentStats.FireCharacterMiss += PlayerStat.ShotsFired - PlayerStat.ShotsHit;
						CurrentStats.NumOfKills += PlayerStat.NumOfKills;
						CurrentStats.NumOfDeaths += PlayerStat.NumOfDeaths;
						SaveGame->SavePlayerStats.Add(PlayerStat.PlayerName, CurrentStats);
						ShooterGI->SaveGame_Implementation(SaveGame);
						break;
					}
				}
			}
			if (TopPlayersClass == nullptr) return;
			if (TopPlayersWidget == nullptr)
			{
				TopPlayersWidget = CreateWidget<UShowTopPlayers>(this, TopPlayersClass);
				if (TopPlayersWidget)
				{
					TopPlayersWidget->StartShowingTopPlayers(InPlayerStatsArray, this, bIsWinner, WinnerNames, GameModeType, ShooterGI->TournamentParams);
				}
			}
			else
			{
				TopPlayersWidget->StartShowingTopPlayers(InPlayerStatsArray, this, bIsWinner, WinnerNames, GameModeType, ShooterGI->TournamentParams);
			}
		}
	}
}

void AShooterPlayerController::ClientShowPrizeAmount_Implementation(float PercentWon, float AmountWon, const FString &Network, const FString &TourName)
{
	if (TopPlayersClass == nullptr) return;
    if (TopPlayersWidget)
	{
		TopPlayersWidget->ShowPrizePool(PercentWon, AmountWon, Network, TourName);
	}
}

void AShooterPlayerController::UpdateProgressStats(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks)
{
	AShooterPlayerState* PS = GetShooterPlayerState();
	if (PS && PS->XPComponent)
	{
		ClientUpdateXP(NewLevel, NewXp, NewUnlocks, PS->XPComponent->PlayerProgress);
	}
}

void AShooterPlayerController::ClientUpdateXP_Implementation(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks, const FPlayerProgress& DeltaProgress)
{
	UpdateWidgetXP(NewLevel, NewXp, NewUnlocks, DeltaProgress);
}

void AShooterPlayerController::UpdateWidgetXP(int32 NewLevel, int32 NewXp, const TArray<FName>& NewUnlocks, const FPlayerProgress& DeltaProgress)
{
	if (TopPlayersWidget && TopPlayersWidget->bXPContainer)
	{
		TopPlayersWidget->UpdateXP(NewLevel, NewXp, NewUnlocks, DeltaProgress);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate XPDel;
		FTimerHandle XPTimer;
		XPDel.BindUFunction(this, FName("UpdateWidgetXP"), NewLevel, NewXp, NewUnlocks, DeltaProgress);
		World->GetTimerManager().SetTimer(XPTimer, XPDel, 0.15f, false);
	}
}

void AShooterPlayerController::OnGameEndForWinner()
{
	AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn());
	if (ShooterChar)
	{
		ShooterChar->WinnerOnRespawnMode();
	}
	else 
	{
		AHelicopter* HeliChar = Cast<AHelicopter>(GetPawn());
		if (HeliChar)
		{
			if (HeliChar->OwnedShooter) HeliChar->OwnedShooter->WinnerOnRespawnMode();
		}
	}
}

void AShooterPlayerController::ClientStartSetViewToAI_Implementation(FVector AIWinnerLocation, FRotator AIWinnerRotation)
{
	AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn());
	if (ShooterChar == nullptr)
	{
		// Not moving the camera for the helicopter because it interferes with the camera movement with unpossessing
		AHelicopter* HeliChar = Cast<AHelicopter>(GetPawn());
		if (HeliChar)
		{
			HeliChar->StartSmoothCameraToAIWinner(AIWinnerLocation, AIWinnerRotation);
		}
	}
	else
	{
		ShooterChar->StartSmoothCameraToAIWinner(AIWinnerLocation, AIWinnerRotation);
	}
}

void AShooterPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// You can't set the team ID on a player controller (%s); it's driven by the associated player state
}

FGenericTeamId AShooterPlayerController::GetGenericTeamId() const
{
	if (const IShooterTeamAgentInterface* PSWithTeamInterface = Cast<IShooterTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnShooterTeamIndexChangedDelegate* AShooterPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AShooterPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
	}

	Super::OnUnPossess();
}

void AShooterPlayerController::DestroyEOSGameSession()
{
	DestroyEOSSession();
}

void AShooterPlayerController::BroadcastTeamID()
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
    if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->BP_LargeMap)
    {
		LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Broadcasting new player to map")));
		ShooterHUD->CharacterOverlay->BP_LargeMap->TeamIDDelegate.Broadcast();
	}
}

void AShooterPlayerController::AddTeamMemberInContainer(AShooterPlayerState* AddShooterPS)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer)
	{
		ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer->AddTeamMember(AddShooterPS, AddShooterPS->ShooterMeshName);
	}
}

void AShooterPlayerController::SetTeamMemberAttributes(AShooterPlayerState* HitPlayer, float NewHealth, bool bIsShield)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer)
	{
		ShooterHUD->CharacterOverlay->WBP_TeamMemberContainer->OnValueChange(HitPlayer, NewHealth, bIsShield);
	}
}

void AShooterPlayerController::ClientSetTeamWidgetValues_Implementation(AShooterPlayerState* OwnerPS, float Value, float MaxValue, bool bIsCharShield)
{
	SetTeamMemberAttributes(OwnerPS, Value / MaxValue, bIsCharShield);
}

void AShooterPlayerController::ClientStopVivox_Implementation()
{
	#if WITH_CLIENT_ONLY
		ShooterGI = ShooterGI == nullptr ? Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)) : ShooterGI;
		if (ShooterGI && !ShooterGI->bStoppingVivox) ShooterGI->VivoxStop();
	#endif
}

void AShooterPlayerController::TalkButtonPressed(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		if (IsPracticeMode()) return;
		#if WITH_CLIENT_ONLY
			if (SaveGame && SaveGame->bVoicePushToTalk && SaveGame->bVoiceChat && ShooterGI && ShooterGI->VivoxVoiceClient)
			{
				ShooterGI->VivoxVoiceClient->AudioInputDevices().SetMuted(false);
			}
		#endif
	}
}

void AShooterPlayerController::TalkButtonReleased(const FInputActionValue& Value)
{
	if (!Value.Get<bool>())
	{
		if (IsPracticeMode()) return;
		#if WITH_CLIENT_ONLY
			if (SaveGame && SaveGame->bVoicePushToTalk && SaveGame->bVoiceChat && ShooterGI && ShooterGI->VivoxVoiceClient)
			{
				ShooterGI->VivoxVoiceClient->AudioInputDevices().SetMuted(true);
			}
		#endif
	}
}

void AShooterPlayerController::ToggleMapPressed(const FInputActionValue& Value)
{
	ShooterHUD = ShooterHUD == nullptr ? Cast<AShooterCrosshairHUD>(GetHUD()) : ShooterHUD;
	if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->BP_LargeMap)
	{
		ShooterHUD->CharacterOverlay->BP_LargeMap->ToggleMap();
	}
}

void AShooterPlayerController::GetWeaponData(const FString &NFT_ID)
{
	TMap<FString, FString> WeaponMap; 
	WeaponMap.Add(TEXT(""), NFT_ID); // *** query point
	const FString MainURL = TEXT(""); // *** URL for weapon end point
	if (HTTPRequestObj)
	{
		//HTTPRequestObj->GetRequest(WeaponMap, MainURL);
	}
}

void AShooterPlayerController::OnResponseReceived(FString ResponseStr, FString ResponseURL)
{
	// *** need to change this text to end point function name
	/*
    if (ResponseURL.Contains(TEXT("postStatsDL"), ESearchCase::CaseSensitive))
    {
		if (AShooterCharacter* ShooterChar = Cast<AShooterCharacter>(GetPawn()))
		{
			ShooterChar->ServerSpawnNFT(ResponseStr);
		}
    }
	*/
}

void AShooterPlayerController::CameraShake(float ScaleValue)
{
	if (APlayerCameraManager* CameraManager = PlayerCameraManager)
	{
		if (CameraShakeClass) CameraManager->StartCameraShake(CameraShakeClass.Get(), ScaleValue);
	}
}

void AShooterPlayerController::UpdateLumenParams()
{
	struct FLumenArgs
	{
		bool Lumen = true;
		bool LumenReflection = true;
	};

	FLumenArgs LumenArgs = FLumenArgs();
    // Get the Level Script Actor
    if (ULevel* CurrentLevel = GetWorld()->GetCurrentLevel())
    {
        if (ALevelScriptActor* LevelScriptActor = CurrentLevel->GetLevelScriptActor())
        {
			LoadSavedGame();
			if (SaveGame)
			{
				LumenArgs.Lumen = SaveGame->bLumen;
				LumenArgs.LumenReflection = SaveGame->bLumenReflections;
			}
            FName FunctionName(TEXT("UpdateLumen"));
            UFunction* Function = LevelScriptActor->FindFunction(FunctionName);
            if (Function)
            {
                LevelScriptActor->ProcessEvent(Function, &LumenArgs);
            }
        }
    }
}

void AShooterPlayerController::SetTeamIds(const FString& PlayerData)
{
	if (AShooterGameState* ShooterGS = GetShooter_GS())
	{
		ShooterGS->ParsePlayerSessionData(GetShooterPlayerState(), PlayerData);
	}
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, bRespawnSwitch);
}

void AShooterPlayerController::OnRewardObtained_Implementation(float CameraScaleVal)
{
	CameraShake(CameraScaleVal);
}
