#pragma once

#include "CoreMinimal.h"
#include "UnlockTypes.generated.h"

// UnlockTypes.h

UENUM(BlueprintType)
enum class EUnlockType : uint8
{
    CharacterSkin UMETA(DisplayName = "Character Skin"),
    Emote UMETA(DisplayName = "Emote/Dance"),
    GameMode UMETA(DisplayName = "Game Mode"),
    Weapon UMETA(DisplayName = "Weapon"),
    Badge UMETA(DisplayName = "Badge"),
    Asset UMETA(DisplayName = "Asset"),
    MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FUnlockData : public FTableRowBase
{
    GENERATED_BODY()

    // Basic info
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText DisplayName = FText();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText Description = FText();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    EUnlockType UnlockType = EUnlockType::MAX;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    TSoftObjectPtr<UTexture2D> IconTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    int32 XPCost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    float CryptoCost = 0;

    // 0 for Leo, 1 for Dekker, 2 for Phase, 3 for Maverick
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::CharacterSkin", EditConditionHides))
    int32 CharacterID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::CharacterSkin", EditConditionHides))
    int32 SortOrder = 0;
    
    // Type-specific assets (only one should be set based on UnlockType)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::CharacterSkin", EditConditionHides))
    TSoftObjectPtr<USkeletalMesh> CharacterMesh = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::CharacterSkin || UnlockType == EUnlockType::Emote", EditConditionHides))
    TSoftClassPtr<UAnimInstance> AnimBlueprintClass= nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::Emote", EditConditionHides))
    TSoftObjectPtr<UAnimMontage> EmoteMontage= nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::GameMode", EditConditionHides))
    FName GameModeID = FName();  // Reference to game mode (e.g., "Deathmatch", "CaptureTheFlag")
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets", meta = (EditCondition = "UnlockType == EUnlockType::Badge || UnlockType == EUnlockType::Asset", EditConditionHides))
    TSoftObjectPtr<UStaticMesh> PreviewMesh = nullptr;  // Optional: for non-character items
    
    // Preview settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FVector PreviewLocationOffset = FVector(0, 0, 0);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    FRotator PreviewRotation = FRotator(0, 0, 0);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    float PreviewScale = 1.0f;
    
    // Rarity/special effects
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FLinearColor RarityColor = FLinearColor::White;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    bool bShowParticleEffect = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    TSoftObjectPtr<class UNiagaraSystem> UnlockNiagaraEffect = nullptr;
};