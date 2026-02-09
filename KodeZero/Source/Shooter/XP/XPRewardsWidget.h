// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shooter/StructTypes/UnlockTypes.h"
#include "XPRewardsWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class USoundCue;
class UCanvasPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRewardContinue);

UCLASS()
class SHOOTER_API UXPRewardsWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    // Widget bindings
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TitleText;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* DescriptionText;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LevelUpTxt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* UnlockedTitleTxt;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UImage* PreviewImage;  // For 3D render or icon

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (BindWidget))
    UCanvasPanel* Canvas;  // For 3D render or icon
    
    UPROPERTY(meta = (BindWidget))
    UButton* ContinueButton;
    
    // Optional: For particle effects overlay
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ParticleEffectImage;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* NiagaraSystem;
    
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    UDataTable* UnlocksDataTable;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    TSubclassOf<class AXPRewardPreview> PreviewActorClass;  // Your custom preview actor with scene capture
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    UMaterialInterface* PreviewRenderMaterial;  // Material with render texture for 3D preview
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnRewardContinue OnContinueClicked;
    
    // Main API
    UFUNCTION(BlueprintCallable, Category = "Reward")
    void SetUnlockReward(FName UnlockID, int32 CompletedLevel, int32 Quantity, bool bIsInventory);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 LevelCompleted;
    
    UFUNCTION(BlueprintCallable, Category = "Reward")
    void SetUnlockRewardDirect(const FUnlockData& UnlockData);

    UFUNCTION(BlueprintCallable, Category = "Reward")
    void OnShakeAnimation();

    inline static const TMap<EUnlockType, FString> UnlockTypeNames = {
        {EUnlockType::CharacterSkin, TEXT("Character Skin")},
        {EUnlockType::Emote, TEXT("Emote/Dance")},
        {EUnlockType::GameMode, TEXT("Game Mode")},
        {EUnlockType::Weapon, TEXT("Weapon")},
        {EUnlockType::Badge, TEXT("Badge")},
        {EUnlockType::Asset, TEXT("Asset")}
        // Add all your types here
    };

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    FString GetUnlockTypeDisplayName(EUnlockType Value);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USoundCue* CloseSound;


private:
    UPROPERTY()
    FUnlockData CurrentUnlockData;
    
    UPROPERTY()
    AActor* SpawnedPreviewActor;
    
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicPreviewMaterial;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* OpenAnim;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* LevelAnim;

    UPROPERTY()
    FName CurrentUnlockID;

    int32 AssetAmount = 0;
    bool bIsAnAsset = false;
    
    UFUNCTION()
    void OnContinueButtonClicked();
    
    UFUNCTION(BlueprintCallable, Category = "Reward")
    void SetupPreview();
    void Setup3DPreview();
    void SetupIconPreview();
    void CleanupPreview();
    void PlayUnlockAnimation();
	
};
