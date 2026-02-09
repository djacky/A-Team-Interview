// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterCrosshairHUD.h"
#include "Shooter/Items/Weapons/Weapon.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "ShooterCharacter.h"
#include "Helicopter.h"
#include "Shooter/Items/Weapons/HelicopterWeapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/CharacterOverlay.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/ElimAnnoucement.h"
#include "Widgets/ElimAnnoucementSmall.h"
#include "Widgets/GeneralAnnouncement.h"
#include "Widgets/EpicNotifys.h"
#include "XP/XPUpdate.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"

AShooterCrosshairHUD::AShooterCrosshairHUD()
{

}
void AShooterCrosshairHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterCrosshairHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    

}

void AShooterCrosshairHUD::DrawHUD()
{
	Super::DrawHUD();
    DynamicCrosshair(GetWorld()->GetDeltaSeconds());
}

void AShooterCrosshairHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void AShooterCrosshairHUD::ShowXPUpdate(const FString& Title, int32 XP, bool bDestroy)
{
	if (!XPClass) return;
    OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (!XPWidget)
	{
        if (bDestroy) return;
		XPWidget = CreateWidget<UXPUpdate>(OwningPlayer, XPClass);
		if (XPWidget)
		{
			XPWidget->StartNotify(this, XP);
		}
	}
	else
	{
        XPWidget->SetTitle(Title);
		if (!bDestroy)
		{
			XPWidget->SetXP(XP);
		}
		else
		{
			XPWidget->DestroyWidget();
		}
	}
}

void AShooterCrosshairHUD::AddEpicNotify(const FString& NotifyType, const FLinearColor& WidgetColor,
    const FString& FirstTitleTxt, const FString& SecondTitleTxt, const FString& NotifyTxt)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && EpicNotifyClass)
	{
        if (!EpicNotifyWidget)
        {
            EpicNotifyWidget = CreateWidget<UEpicNotifys>(OwningPlayer, EpicNotifyClass);
            if (EpicNotifyWidget) EpicNotifyWidget->StartNotify(this, WidgetColor, FirstTitleTxt, SecondTitleTxt, NotifyTxt);
        }
        else
        {
            EpicNotifyWidget->Reset();
            EpicNotifyWidget->StartNotify(this, WidgetColor, FirstTitleTxt, SecondTitleTxt, NotifyTxt);
        }
    }
}

void AShooterCrosshairHUD::AddGeneralAnnouncement(FString Announcement, FString Color, float AddDelay, float RemoveTimeFactor)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && GeneralAnnouncementClass)
	{
        UGeneralAnnouncement* GenAnnouncementWidget = CreateWidget<UGeneralAnnouncement>(OwningPlayer, GeneralAnnouncementClass);
        if (GenAnnouncementWidget)
        {
            GenAnnouncementWidget->SetAnnouncementText(Announcement, Color, AddDelay);
            GenAnnouncementWidget->AddToViewport();
			for (UGeneralAnnouncement* Msg : GeneralMessages)
			{
				if (Msg && Msg->AnnouncementBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementBox);
					if (CanvasSlot)
					{
						FVector2D Position = CanvasSlot->GetPosition();
						FVector2D NewPosition(
							CanvasSlot->GetPosition().X,
							Position.Y - CanvasSlot->GetSize().Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
            }
			GeneralMessages.Add(GenAnnouncementWidget);

			FTimerHandle GeneralMsgTimer;
			FTimerDelegate GeneralMsgDelegate;
			GeneralMsgDelegate.BindUFunction(this, FName("GeneralAnnouncementTimerFinished"), GenAnnouncementWidget);
			GetWorldTimerManager().SetTimer(
				GeneralMsgTimer,
				GeneralMsgDelegate,
				RemoveTimeFactor * ElimAnnouncementTime,
				false
			);
        }
    }
}

void AShooterCrosshairHUD::GeneralAnnouncementTimerFinished(UGeneralAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
        if (GeneralMessages.Contains(MsgToRemove)) GeneralMessages.Remove(MsgToRemove);
		MsgToRemove->RemoveFromParent();
	}
}

void AShooterCrosshairHUD::AddElimAnnouncement(FString Attacker, FString Victim, bool bTeamMode)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && ElimAnnouncementClass)
	{
		UElimAnnoucement* ElimAnnouncementWidget = CreateWidget<UElimAnnoucement>(OwningPlayer, ElimAnnouncementClass);
		if (ElimAnnouncementWidget)
		{
            bTeamMode ? ElimAnnouncementWidget->SetTeamElimAnnouncementText(Attacker, Victim) : ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			//ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			ElimAnnouncementWidget->AddToViewport();

			for (UElimAnnoucement* Msg : ElimMessages)
			{
				if (Msg && Msg->AnnouncementBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg->AnnouncementBox);
					if (CanvasSlot)
					{
						FVector2D Position = CanvasSlot->GetPosition();
						FVector2D NewPosition(
							CanvasSlot->GetPosition().X,
							Position.Y - CanvasSlot->GetSize().Y
						);
						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			ElimMessages.Add(ElimAnnouncementWidget);

			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget);
			GetWorldTimerManager().SetTimer(
				ElimMsgTimer,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false
			);
		}
	}
}

void AShooterCrosshairHUD::ElimAnnouncementTimerFinished(UElimAnnoucement* MsgToRemove)
{
	if (MsgToRemove)
	{
		MsgToRemove->RemoveFromParent();
	}
}

void AShooterCrosshairHUD::AddElimAnnouncementSmall(const FString& Attacker, const FString& Victim, EShooterDamageType ShooterDT, bool bTeamMode)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (OwningPlayer && ElimAnnouncementSmallClass)
	{
        /*
        if (ElimMessagesSmall.Num() >= 4)
        {
            ElimMessagesSmall[0]->RemoveFromParent();
            ElimMessagesSmall.RemoveAt(0);
        }
        */
        
		UElimAnnoucementSmall* ElimAnnouncementWidget = CreateWidget<UElimAnnoucementSmall>(OwningPlayer, ElimAnnouncementSmallClass);
		if (ElimAnnouncementWidget)
		{
            ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim, ShooterDT);
			//ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			//ElimAnnouncementWidget->AddToViewport();

            UCanvasPanelSlot* CanvasSlot = nullptr;
            if (ElimAnnouncementWidget && CharacterOverlay && CharacterOverlay->MainCanvas)
            {
                CanvasSlot = CharacterOverlay->MainCanvas->AddChildToCanvas(ElimAnnouncementWidget);
                if (CanvasSlot)
                {
                    CanvasSlot->SetAnchors(FAnchors(0.f, 1.f)); 
                    CanvasSlot->SetAlignment(FVector2D(0.f, 1.f));
                    CanvasSlot->SetAutoSize(false);
                    CanvasSlot->SetSize(FVector2D(290.f, 42.f));
                    CanvasSlot->SetPosition(FVector2D(40.f, -170.f)); // Offset
                }
            }

			for (UElimAnnoucementSmall* Msg : ElimMessagesSmall)
			{
				if (Msg)
				{
					UCanvasPanelSlot* OldCanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Msg);
					if (OldCanvasSlot)
					{
						FVector2D Position = OldCanvasSlot->GetPosition();
						FVector2D NewPosition(
							OldCanvasSlot->GetPosition().X,
							Position.Y - OldCanvasSlot->GetSize().Y
						);
						OldCanvasSlot->SetPosition(NewPosition);
					}
				}
			}

			ElimMessagesSmall.Add(ElimAnnouncementWidget);

			FTimerHandle ElimMsgTimer;
			FTimerDelegate ElimMsgDelegate;
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementSmallTimerFinished"), ElimAnnouncementWidget);
			GetWorldTimerManager().SetTimer(
				ElimMsgTimer,
				ElimMsgDelegate,
				3.f * ElimAnnouncementTime,
				false
			);
		}
	}
}

void AShooterCrosshairHUD::ElimAnnouncementSmallTimerFinished(UElimAnnoucementSmall* MsgToRemove)
{
	if (MsgToRemove)
	{
		//MsgToRemove->RemoveFromParent();
        MsgToRemove->PlayFinishAnimation();
	}
}

void AShooterCrosshairHUD::DynamicCrosshair(float DeltaTime)
{
    if (ShooterCharacter == nullptr)
    {
        ShooterCharacter = Cast<AShooterCharacter>(GetOwningPawn());
    }

    if (ShooterCharacter && ShooterCharacter->OwnedPawn && HelicopterActor == nullptr)
    {
        HelicopterActor = Cast<AHelicopter>(GetOwningPlayerController()->GetPawn());
    }

    

    if (ShooterCharacter && ShooterCharacter->OwnedPawn == nullptr)
    {
        //ViewportSize = ShooterCharacter->GetViewportVector();
        ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);

        FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
        CrosshairLocation.Y -= 0.f;
        FVector2D CrosshairBaseCenter(CrosshairLocation.X - 32.f, CrosshairLocation.Y - 32.f);
        
        MoveFactor = CalculateSpreadFactor(DeltaTime);
        
        // Pick a random position within the crosshair
        const FVector2D RandomShotPosition{ UKismetMathLibrary::RandomFloatInRange(CrosshairLocation.X - MoveFactor, CrosshairLocation.X + MoveFactor),
            UKismetMathLibrary::RandomFloatInRange(CrosshairLocation.Y - MoveFactor, CrosshairLocation.Y + MoveFactor) };
        
        FVector CrosshairWorldPosition;
        FVector CrosshairWorldDirection;
        bool bScreenToWorld;

        // Get world position and direction of crosshairs
        if (ShooterCharacter->GetTraceHitItem() || (ShooterCharacter->GetEquippedWeapon() && ShooterCharacter->GetEquippedWeapon()->GetWeaponType() == EWeaponType::EWT_Shotgun))
        {
            bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
            GetOwningPlayerController(),
            CrosshairLocation,
            CrosshairWorldPosition,
            CrosshairWorldDirection);
        }
        else
        {
            bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
            GetOwningPlayerController(),
            RandomShotPosition,
            CrosshairWorldPosition,
            CrosshairWorldDirection);
        }

        if (!ShooterCharacter->bIsAI) ShooterCharacter->SetRandomShotInfo(bScreenToWorld, CrosshairWorldPosition, CrosshairWorldDirection);

        if (!ShooterCharacter->GetEquippedItem())
        {
            // Waiting for replication of weapon and item
        }
        else
        {
            if (ShooterCharacter->GetEquippedItem()->GetItemType() != EItemType::EIT_Weapon)
            {
                DrawCrosshair(ShooterCharacter->GetEquippedItem()->GetGeneralCrosshair(),
                    CrosshairBaseCenter.X, CrosshairBaseCenter.Y);
            }
            else
            {
                if (ShooterCharacter->GetEquippedWeapon())
                {
                    if (ShooterCharacter->GetEquippedWeapon()->GetCrosshairLeft())
                    {
                        DrawCrosshair(ShooterCharacter->GetEquippedWeapon()->GetCrosshairLeft(), 
                            CrosshairBaseCenter.X - MoveFactor, CrosshairBaseCenter.Y);
                    }
                    if (ShooterCharacter->GetEquippedWeapon()->GetCrosshairRight())
                    {
                        DrawCrosshair(ShooterCharacter->GetEquippedWeapon()->GetCrosshairRight(), 
                            CrosshairBaseCenter.X + MoveFactor, CrosshairBaseCenter.Y);
                    }
                    if (ShooterCharacter->GetEquippedWeapon()->GetCrosshairTop())
                    {
                        DrawCrosshair(ShooterCharacter->GetEquippedWeapon()->GetCrosshairTop(),
                            CrosshairBaseCenter.X, CrosshairBaseCenter.Y - MoveFactor);
                    }
                    if (ShooterCharacter->GetEquippedWeapon()->GetCrosshairBottom())
                    {
                        DrawCrosshair(ShooterCharacter->GetEquippedWeapon()->GetCrosshairBottom(),
                            CrosshairBaseCenter.X, CrosshairBaseCenter.Y + MoveFactor);
                    }
                    if (ShooterCharacter->GetEquippedWeapon()->GetCrosshairMiddle())
                    {
                        DrawCrosshair(ShooterCharacter->GetEquippedWeapon()->GetCrosshairMiddle(),
                            CrosshairBaseCenter.X, CrosshairBaseCenter.Y);
                    }
                }
            }
        }
    }
    else if (HelicopterActor && ShooterCharacter->OwnedPawn)
    {
        //ViewportSize = ShooterCharacter->GetViewportVector();
        ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);

        FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
        CrosshairLocation.Y -= 0.f;
        FVector2D CrosshairBaseCenter(CrosshairLocation.X - 32.f, CrosshairLocation.Y - 32.f);
        
        //MoveFactor = CalculateSpreadFactorHelicopter(DeltaTime);
        
        FVector CrosshairWorldPosition;
        FVector CrosshairWorldDirection;
        bool bScreenToWorld;

        bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        GetOwningPlayerController(),
        CrosshairLocation,
        CrosshairWorldPosition,
        CrosshairWorldDirection);

        HelicopterActor->SetRandomShotInfo(bScreenToWorld, CrosshairWorldPosition, CrosshairWorldDirection);

        if (HelicopterActor && HelicopterActor->GetEquippedWeapon())
        {
            DrawCrosshair(HelicopterActor->GetEquippedWeapon()->CrosshairMiddle,
                CrosshairBaseCenter.X, CrosshairBaseCenter.Y);
        }
    }
}

float AShooterCrosshairHUD::CalculateSpreadFactor(float DeltaTime)
{
    bAiming = ShooterCharacter->GetAiming();
    bFiringBullet = ShooterCharacter->GetBulletFire();
    float CrosshairSpreadMax = 16.f;

    // Calculate CrossairAimFactor
    if (bAiming) //Checks if character is aiming
    {
        // Shrink the crosshairs when aiming
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.6f, DeltaTime, 20.f);
    }
    else // character is not aiming
    {
        // Set crossairs back to normal when not aiming
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 20.f);
    }

    // True 0.05 seconds after firing
    if (bFiringBullet)
    {
        CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
    }
    else
    {
        CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
    }

    // Calculate CrossairInAirFactor
    if (ShooterCharacter->GetCharacterMovement()->IsFalling()) //Checks if character is in the air or not
    {
        // Spread the crosshairs slowly while in air
        if (ShooterCharacter->GetStartFlying())
        {
            CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 1.f, DeltaTime, 2.25f);
        }
        else
        {
            CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
        }
        
    }
    else // Character is on the ground
    {
        // Shrink the crosshairs rapidly when on the ground
        CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
    }

    // Calculate CrossairInAirFactor
    FVector2D WalkSpeedRange{0.f, 600.f};
    FVector2D VelocityMultiplierRange{0.f, 1.f};
    FVector Velocity{ShooterCharacter->GetVelocity()};
    Velocity.Z = 0.f;
    //CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
    CrosshairVelocityFactor = Velocity.Size() / 600.f;
    CrosshairSpreadMultiplier = 0.5f + 
        CrosshairVelocityFactor + 
        CrosshairInAirFactor +
        CrosshairAimFactor +
        CrosshairShootingFactor;
    
    return CrosshairSpreadMultiplier * CrosshairSpreadMax;
}

float AShooterCrosshairHUD::CalculateSpreadFactorHelicopter(float DeltaTime)
{
    float bAimingHeli = HelicopterActor->GetAiming();

    float CrosshairSpreadMax = 16.f;

    // Calculate CrossairAimFactor
    if (bAimingHeli) //Checks if character is aiming
    {
        // Shrink the crosshairs when aiming
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.6f, DeltaTime, 20.f);
    }
    else // character is not aiming
    {
        // Set crossairs back to normal when not aiming
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 20.f);
    }

    FVector Velocity{HelicopterActor->GetVelocity()};
    Velocity.Z = 0.f;
    //CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());
    CrosshairVelocityFactor = Velocity.Size() / 600.f;
    CrosshairSpreadMultiplier = 0.5f + 
        CrosshairAimFactor;
    
    return CrosshairSpreadMultiplier * CrosshairSpreadMax;
}


void AShooterCrosshairHUD::DrawCrosshair(UTexture2D* Crosshair, float x, float y)
{
    DrawTexture
    (
        Crosshair,
        x,
        y,
        64.f,
        64.f,
        0.f,
        0.f,
        1.f,
        1.f
    );

}