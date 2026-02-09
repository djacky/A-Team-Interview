// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "XPUpdate.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UXPUpdate : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void StartNotify(class AShooterCrosshairHUD* ShooterHUD, int32 XP);

	UPROPERTY()
	AShooterCrosshairHUD* HUD;

	void SetXP(int32 InXP);
	void SetTitle(const FString& Title);
	void DestroyWidget();

private:
	UPROPERTY()
	FWidgetAnimationDynamicEvent EndDelegate;

	UFUNCTION()
	void AnimationFinished();

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* CircleAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* StartAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* EndAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* XPAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* XPLogoAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* TitleAnim;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* XPTxt;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TitleTxt;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sounds", meta = (AllowPrivateAccess = "true"))
	class USoundCue* XPAddSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sounds", meta = (AllowPrivateAccess = "true"))
	USoundCue* XPEndSound;
	
};
