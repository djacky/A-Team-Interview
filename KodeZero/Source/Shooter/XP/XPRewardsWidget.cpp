// Fill out your copyright notice in the Description page of Project Settings.


#include "XPRewardsWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/DataTable.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "XPRewardPreview.h"
#include "Shooter/Misc/Interfaces/PreviewActorInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Shooter/ShooterGameInstance.h"
#include "Sound/SoundCue.h"
#include "Components/CanvasPanelSlot.h"
#if WITH_CLIENT_ONLY
	#include "NiagaraUIRenderer/Public/NiagaraSystemWidget.h"
#endif


void UXPRewardsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    if (ContinueButton)
    {
        ContinueButton->OnClicked.AddDynamic(this, &UXPRewardsWidget::OnContinueButtonClicked);
    }
    if (OpenAnim)
    {
        PlayAnimation(OpenAnim, 0);
    }
}

void UXPRewardsWidget::NativeDestruct()
{
    CleanupPreview();
    Super::NativeDestruct();
}

void UXPRewardsWidget::OnShakeAnimation()
{
    #if WITH_CLIENT_ONLY
        UNiagaraSystemWidget* NiagaraWidget = NewObject<UNiagaraSystemWidget>(this, UNiagaraSystemWidget::StaticClass());
        if (NiagaraWidget && Canvas && NiagaraSystem)
        {
            NiagaraWidget->AutoActivate = false;
            NiagaraWidget->NiagaraSystemReference = NiagaraSystem;
            UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(NiagaraWidget);
            if (CanvasSlot)
            {
                CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f)); // Anchor is set in the center
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // Position from widget center
                CanvasSlot->SetAutoSize(false);
                CanvasSlot->SetSize(FVector2D(650.f, 650.f));
                CanvasSlot->SetPosition(FVector2D(0.f, -100.f)); // Offset
            }
            NiagaraWidget->ActivateSystem(true);
        }
    #endif
}

void UXPRewardsWidget::SetUnlockReward(FName UnlockID, int32 CompletedLevel, int32 Quantity, bool bIsInventory)
{
    if (LevelAnim) PlayAnimation(LevelAnim, 0);
    LevelCompleted = CompletedLevel;
    CurrentUnlockID = UnlockID;
    AssetAmount = Quantity;
    bIsAnAsset = bIsInventory;

    if (LevelUpTxt) LevelUpTxt->SetText(FText::AsNumber(LevelCompleted - 1));
    if (!UnlocksDataTable)
    {
        UE_LOG(LogTemp, Error, TEXT("UnlocksDataTable not set on RewardUnlockWidget"));
        return;
    }
    
    FString ContextString;
    FUnlockData* UnlockDataPtr = UnlocksDataTable->FindRow<FUnlockData>(UnlockID, ContextString);
    
    if (!UnlockDataPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("Unlock ID '%s' not found in UnlocksDataTable"), *UnlockID.ToString());
        return;
    }
    
    SetUnlockRewardDirect(*UnlockDataPtr);
}

void UXPRewardsWidget::SetUnlockRewardDirect(const FUnlockData& UnlockData)
{
    CurrentUnlockData = UnlockData;
    // Set text
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(TEXT("Level Up!")));
    }
    
    FString UnlockTypeName = GetUnlockTypeDisplayName(CurrentUnlockData.UnlockType);
    FString DescriptionStr = FString::Printf(TEXT("You've unlocked the \"%s\" %s! One step closer to being a true Kode master..."), *CurrentUnlockData.DisplayName.ToString(), *UnlockTypeName.ToLower());
    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(DescriptionStr));
    }

    FString SuccessStr = FString::Printf(TEXT("%s Unlocked!"), *UnlockTypeName);
    if (UnlockedTitleTxt)
    {
        UnlockedTitleTxt->SetText(FText::FromString(SuccessStr));
    }

    
    // Setup preview
    SetupPreview();
}

void UXPRewardsWidget::SetupPreview()
{
    CleanupPreview();
    
    if (UShooterGameInstance* ShooterGI = Cast<UShooterGameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        switch (CurrentUnlockData.UnlockType)
        {
            case EUnlockType::CharacterSkin:
            case EUnlockType::Emote:
                ShooterGI->CurrentPlayerProgress.UnlockedRewards.Add(CurrentUnlockID);
            case EUnlockType::Asset:
                if (bIsAnAsset)
                {
                    ShooterGI->UpdateCurrentProgressInventory_Implementation(CurrentUnlockID, AssetAmount, true);
                }
                Setup3DPreview();
                break;
                
            case EUnlockType::GameMode:
            case EUnlockType::Badge:
            case EUnlockType::Weapon:
            default:
                SetupIconPreview();
                break;
        }
    }

}

void UXPRewardsWidget::Setup3DPreview()
{
    UWorld* World = GetWorld();
    if (!World || !PreviewImage || !PreviewActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot setup 3D preview: Missing World, PreviewImage, or PreviewActorClass"));
        SetupIconPreview(); // Fallback to icon
        return;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    // Spawn at a location far from gameplay (e.g., under the map)
    FVector SpawnLocation = FVector(0, 0, -10000);
    FRotator SpawnRotation = FRotator::ZeroRotator;
    
    SpawnedPreviewActor = World->SpawnActor<AXPRewardPreview>(PreviewActorClass, SpawnLocation, SpawnRotation, SpawnParams);
    
    if (SpawnedPreviewActor)
    {
        if (IPreviewActorInterface* PreviewInterface = Cast<IPreviewActorInterface>(SpawnedPreviewActor))
        {
            PreviewInterface->Execute_SetPreviewContent(SpawnedPreviewActor, CurrentUnlockData);
        }
    }
}

void UXPRewardsWidget::SetupIconPreview()
{
    if (PreviewImage && CurrentUnlockData.IconTexture)
    {
        PreviewImage->SetBrushFromTexture(CurrentUnlockData.IconTexture.LoadSynchronous());
    }
    else if (PreviewImage)
    {
        // Set a default/placeholder texture
        PreviewImage->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
    }
}

void UXPRewardsWidget::CleanupPreview()
{
    if (SpawnedPreviewActor)
    {
        SpawnedPreviewActor->Destroy();
        SpawnedPreviewActor = nullptr;
    }
    
    DynamicPreviewMaterial = nullptr;
}

void UXPRewardsWidget::PlayUnlockAnimation()
{
    // Play a Blueprint widget animation if it exists
    
    // Optional: Play particle effects
    if (ParticleEffectImage && !CurrentUnlockData.UnlockNiagaraEffect.IsNull())
    {
        // You could set a particle texture/animation here
        // Or handle particles in Blueprint
    }
}

void UXPRewardsWidget::OnContinueButtonClicked()
{
    CleanupPreview();
    if (CloseSound)
    {
        UGameplayStatics::PlaySound2D(this, CloseSound);
    }
    // Broadcast event
    OnContinueClicked.Broadcast();
    
    // Remove from parent
    RemoveFromParent();
}

FString UXPRewardsWidget::GetUnlockTypeDisplayName(EUnlockType Value)
{
    if (UnlockTypeNames.Contains(Value))
    {
        return UnlockTypeNames[Value];
    }
    return TEXT("item");
}

