// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Shooter/EnumTypes/ShooterDamageType.h"
#include "ShooterCrosshairHUD.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AShooterCrosshairHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShooterCrosshairHUD();
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	class AShooterCharacter* ShooterCharacter;
	
	virtual void DrawHUD() override;
	// Get current size of the viewport

public:
	UPROPERTY(BlueprintReadOnly)
	FVector2D ViewportSize;

	void AddCharacterOverlay();

	void AddElimAnnouncement(FString Attacker, FString Victim, bool bTeamMode);
	void AddElimAnnouncementSmall(const FString& Attacker, const FString& Victim, EShooterDamageType ShooterDT, bool bTeamMode);
	void AddGeneralAnnouncement(FString Announcement, FString Color, float AddDelay = 0.f, float RemoveTimeFactor = 1.4f);
	void AddEpicNotify(const FString& NotifyType, const FLinearColor& WidgetColor,
		const FString& FirstTitleTxt, const FString& SecondTitleTxt, const FString& NotifyTxt);

	void ShowXPUpdate(const FString& Title, int32 XP, bool bDestroy = false);

	UPROPERTY()
	class UEpicNotifys* EpicNotifyWidget;

	UPROPERTY()
	class UXPUpdate* XPWidget = nullptr;

private:

	UPROPERTY()
	class APlayerController* OwningPlayer;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUserWidget> XPClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> EpicNotifyClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnoucement> ElimAnnouncementClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnoucementSmall> ElimAnnouncementSmallClass;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGeneralAnnouncement> GeneralAnnouncementClass;

	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 2.5f;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnoucement* MsgToRemove);

	UFUNCTION()
	void ElimAnnouncementSmallTimerFinished(UElimAnnoucementSmall* MsgToRemove);

	UFUNCTION()
	void GeneralAnnouncementTimerFinished(UGeneralAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<UElimAnnoucement*> ElimMessages;

	UPROPERTY()
	TArray<UElimAnnoucementSmall*> ElimMessagesSmall;

	UPROPERTY()
	TArray<UGeneralAnnouncement*> GeneralMessages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairTop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairBottom;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairMiddle;

	// Determines the spread of the crosshairs
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))	
	float CrosshairSpreadMultiplier;

	// Velocity component for crosshars spead
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))	
	float CrosshairVelocityFactor = 0.f;

	// In air component for crosshars spead
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))	
	float CrosshairInAirFactor = 0.f;

	// Aim component for crosshars spead
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))	
	float CrosshairAimFactor = 0.f;

	// Shooting component for crosshars spead
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))	
	float CrosshairShootingFactor = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	float MoveFactor;

	bool bFiringBullet;

	UPROPERTY()
	class AHelicopter* HelicopterActor;

protected:
	void DynamicCrosshair(float DeltaTime);
	void DrawCrosshair(UTexture2D* Crosshair, float x, float y);
	
	// Calculate the amount of spread for the crosshair
	float CalculateSpreadFactor(float DeltaTime);
	float CalculateSpreadFactorHelicopter(float DeltaTime);

public:
	FORCEINLINE float GetMoveFactor() const { return MoveFactor; }
	FORCEINLINE void SetShooterCharacter(AShooterCharacter* Shooter) { ShooterCharacter = Shooter; }
	FORCEINLINE void SetHelicopter(AHelicopter* InHelicopter) { HelicopterActor = InHelicopter; }

	UPROPERTY(EditAnywhere, Category = "Widget HUD")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Widget HUD", meta = (AllowPrivateAccess = "true"))
	class UCharacterOverlay* CharacterOverlay;
};
