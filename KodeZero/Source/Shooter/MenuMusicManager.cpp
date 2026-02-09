// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuMusicManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AMenuMusicManager::AMenuMusicManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicComponent"));
	SetRootComponent(AudioComponent);
	AudioComponent->OnAudioFinished.AddDynamic(this, &AMenuMusicManager::PlayNextSong);
}

// Called when the game starts or when spawned
void AMenuMusicManager::BeginPlay()
{
	Super::BeginPlay();
}

/*
// Called every frame
void AMenuMusicManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
*/

void AMenuMusicManager::PlayNextSong()
{
	if (SongsArray.Num() == 0) return;

	if (CurrentHandle.IsValid())
	{
		CurrentHandle->ReleaseHandle();
		CurrentHandle.Reset();
	}

	if (!AudioComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("AudioComponent is null."));
		return;
	}

	if (!AudioComponent->OnAudioFinished.IsBound())
	{
		AudioComponent->OnAudioFinished.AddDynamic(this, &AMenuMusicManager::PlayNextSong);
	}

	// Choose a random song
	int32 Index = FMath::RandRange(0, SongsArray.Num() - 1);
	CurrentSoftSong = SongsArray[Index];

	if (CurrentSoftSong.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected song at index %d is null."), Index);
		return;
	}

	// Async load the song
	if (IsValidLowLevel() && !IsEngineExitRequested() && GEngine)
	{
		CurrentHandle = StreamableManager.RequestAsyncLoad(
			CurrentSoftSong.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &AMenuMusicManager::OnSongLoaded)
		);
	}
}

void AMenuMusicManager::OnSongLoaded()
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
	AudioComponent->VolumeMultiplier = 0.25f;
	AudioComponent->FadeIn(3.f);
}

void AMenuMusicManager::StopPlayingMusic()
{
	if (AudioComponent && AudioComponent->IsActive())
	{
		AudioComponent->OnAudioFinished.RemoveDynamic(this, &AMenuMusicManager::PlayNextSong);
		AudioComponent->Stop();
		AudioComponent->Deactivate();

		if (CurrentHandle.IsValid())
		{
			CurrentHandle->ReleaseHandle();
			CurrentHandle.Reset();
		}
	}
}


