// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "XPCounter.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UXPCounter : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable)
    void AnimateFromOldToNew(int32 OldXP, int32 NewXP);

	void UpdateXPText(int32 CurrentXP);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UTextBlock* TicketTxt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UHorizontalBox* TicketBox;

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* XPText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UCurveFloat* AnimationCurve;  // Optional easing

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AnimationDuration = 2.f;  // Longer for dramatic count-up

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class USoundCue* XPChargeStart;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* CoinANim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* XPUpdateAnim;



private:
    int32 StartXP = 0;
    int32 TargetXP = 0;
    float AnimationElapsed = 0.f;
    bool bIsAnimating = false;
    float MinCurveTime = 0.f;
    float MaxCurveTime = 1.f;

    void OnXPAnimFinished();
	
	
};
