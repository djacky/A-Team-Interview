// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "MusicManager.generated.h"

UCLASS()
class SHOOTER_API AMusicManager : public AActor
{
	GENERATED_BODY()
public:	

	AMusicManager();
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAudioComponent* AudioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAudioComponent* BackgroundAudioComponent;

	UPROPERTY(EditAnywhere, Category = Music)
	TArray<TSoftObjectPtr<USoundBase>> SongsArray;

	UPROPERTY()
	TSoftObjectPtr<USoundBase> CurrentSoftSong;

	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> CurrentHandle;

	UPROPERTY(EditAnywhere, Category = "Music")
	TSoftObjectPtr<class USoundCue> BackgroundMusicCue;

	void OnBackgroundMusicLoaded();

	UFUNCTION()
	void OnSongLoaded();

	void ResetStreamHandle();

public:	
	FORCEINLINE UAudioComponent* GetAudioComponent() const { return AudioComponent; }
	FORCEINLINE UAudioComponent* GetBackgroundAudioComponent() const { return BackgroundAudioComponent; }
	//virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void PlayNextSong();

	UFUNCTION(BlueprintCallable)
	void StopPlayingMusic();

	UFUNCTION(BlueprintCallable)
	void PlayBackgroundMusic();

	UFUNCTION(BlueprintCallable)
	void StopBackgroundMusic();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMusicPlaying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bBackgroundMusicPlaying = false;

};
