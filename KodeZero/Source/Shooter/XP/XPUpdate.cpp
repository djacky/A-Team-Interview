// Fill out your copyright notice in the Description page of Project Settings.


#include "XPUpdate.h"
#include "Shooter/Widgets/CharacterOverlay.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Shooter/ShooterCrosshairHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

void UXPUpdate::StartNotify(AShooterCrosshairHUD* ShooterHUD, int32 XP)
{
    HUD = ShooterHUD;
    if (ShooterHUD && ShooterHUD->CharacterOverlay && ShooterHUD->CharacterOverlay->MainCanvas)
    {
        UCanvasPanelSlot* CanvasSlot = ShooterHUD->CharacterOverlay->MainCanvas->AddChildToCanvas(this);
        if (CanvasSlot)
        {
            CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // Anchor is set in the center
            CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // Position from widget center
            CanvasSlot->SetAutoSize(false);
            CanvasSlot->SetSize(FVector2D(240.f, 225.f));
            CanvasSlot->SetPosition(FVector2D(-830.f, 0.f)); // Offset
        }
    }
    if (StartAnim) PlayAnimation(StartAnim);
    //if (CircleAnim) PlayAnimation(CircleAnim, 0.f, 0);
    if (XPLogoAnim) PlayAnimation(XPLogoAnim, 0.f, 0);
    SetXP(XP);
    AddToViewport();
}

void UXPUpdate::SetXP(int32 InXP)
{
    if (XPTxt)
    {
        if (XPAnim) PlayAnimation(XPAnim);
        XPTxt->SetText(FText::AsNumber(InXP));

        UWorld* World = GetWorld();
        if (World && XPAddSound)
        {
            UGameplayStatics::PlaySound2D(World, XPAddSound);
        }
    }
}

void UXPUpdate::SetTitle(const FString& Title)
{
    if (TitleTxt)
    {
        TitleTxt->SetText(FText::FromString(Title));
    }
}

void UXPUpdate::DestroyWidget()
{
    if (EndAnim)
    {
        EndDelegate.BindDynamic(this, &UXPUpdate::AnimationFinished);
        BindToAnimationFinished(EndAnim, EndDelegate);
        PlayAnimation(EndAnim);
        if (TitleAnim) PlayAnimation(TitleAnim, 0.f, 0);

        UWorld* World = GetWorld();
        if (World && XPEndSound)
        {
            UGameplayStatics::PlaySound2D(World, XPEndSound);
        }
    }
}

void UXPUpdate::AnimationFinished()
{
    if (HUD)
    {
        HUD->XPWidget = nullptr;
        RemoveFromParent();
    }
}
