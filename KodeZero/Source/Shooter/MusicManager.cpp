// Fill out your copyright notice in the Description page of Project Settings.


#include "MusicManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AMusicManager::AMusicManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	SetRootComponent(AudioComponent);
	AudioComponent->SetAutoActivate(false);
	AudioComponent->OnAudioFinished.AddDynamic(this, &AMusicManager::PlayNextSong);

	BackgroundAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("BackgroundAudio"));
	BackgroundAudioComponent->SetupAttachment(AudioComponent);
	BackgroundAudioComponent->SetAutoActivate(false);
}

// Called when the game starts or when spawned
void AMusicManager::BeginPlay()
{
	Super::BeginPlay();
}

/*
// Called every frame
void AMusicManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
*/

void AMusicManager::PlayBackgroundMusic()
{
	if (!BackgroundAudioComponent) return;

	if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
	{
		ResetStreamHandle();
		// Load asynchronously
		CurrentHandle = StreamableManager.RequestAsyncLoad(
			BackgroundMusicCue.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &AMusicManager::OnBackgroundMusicLoaded)
		);
	}
	else
	{
		OnBackgroundMusicLoaded(); // Already loaded
	}
}

void AMusicManager::OnBackgroundMusicLoaded()
{
	if (!BackgroundAudioComponent) return;

	USoundCue* LoadedCue = BackgroundMusicCue.Get();
	if (!LoadedCue) return;

	BackgroundAudioComponent->SetSound(LoadedCue);
	BackgroundAudioComponent->FadeIn(3.f);
	bBackgroundMusicPlaying = true;
}

void AMusicManager::StopBackgroundMusic()
{
	if (BackgroundAudioComponent)
	{
		BackgroundAudioComponent->Stop();
		BackgroundAudioComponent->Deactivate();
		bBackgroundMusicPlaying = false;
		ResetStreamHandle();
	}
}

void AMusicManager::PlayNextSong()
{
	if (SongsArray.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SongsArray is empty."));
		return;
	}

	ResetStreamHandle();

	if (!AudioComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("AudioComponent is null."));
		return;
	}

	if (!AudioComponent->OnAudioFinished.IsBound())
	{
		AudioComponent->OnAudioFinished.AddDynamic(this, &AMusicManager::PlayNextSong);
	}

	AudioComponent->Activate();

	// Choose a random song
	int32 Index = FMath::RandRange(0, SongsArray.Num() - 1);
	CurrentSoftSong = SongsArray[Index];

	if (CurrentSoftSong.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected song at index %d is null."), Index);
		return;
	}

	if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
	{
		// Async load the song
		CurrentHandle = StreamableManager.RequestAsyncLoad(
			CurrentSoftSong.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &AMusicManager::OnSongLoaded)
		);
	}
}

void AMusicManager::OnSongLoaded()
{
	// Ensure it's still valid after load
	USoundBase* LoadedSong = CurrentSoftSong.Get();
	if (!LoadedSong || !AudioComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load or assign song."));
		return;
	}

	// Assign and play with fade
	AudioComponent->SetSound(LoadedSong);
	AudioComponent->VolumeMultiplier = 0.15f;
	AudioComponent->FadeIn(3.f);

	bMusicPlaying = true;
}

void AMusicManager::ResetStreamHandle()
{
	if (CurrentHandle.IsValid())
	{
		CurrentHandle->ReleaseHandle();
		CurrentHandle.Reset();
	}
}

void AMusicManager::StopPlayingMusic()
{
	if (AudioComponent)
	{
		AudioComponent->OnAudioFinished.RemoveDynamic(this, &AMusicManager::PlayNextSong);
		AudioComponent->Stop();
		AudioComponent->Deactivate();
		bMusicPlaying = false;
	}
}


