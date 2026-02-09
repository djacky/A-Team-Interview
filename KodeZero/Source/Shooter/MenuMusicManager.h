// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "MenuMusicManager.generated.h"

UCLASS()
class SHOOTER_API AMenuMusicManager : public AActor
{
	GENERATED_BODY()
	
public:	

	AMenuMusicManager();

	UFUNCTION(BlueprintCallable)
	void PlayNextSong();

	UFUNCTION(BlueprintCallable)
	void StopPlayingMusic();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, Category = Music)
	TArray<TSoftObjectPtr<USoundBase>> SongsArray;

	UPROPERTY()
	TSoftObjectPtr<USoundBase> CurrentSoftSong;

	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> CurrentHandle;

	UFUNCTION()
	void OnSongLoaded();

public:	
	//virtual void Tick(float DeltaTime) override;


};
