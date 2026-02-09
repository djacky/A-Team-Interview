// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shooter/StructTypes/PlayerProgress.h"
#include "Shooter/EnumTypes/WeaponType.h"
#include "GameplayTagContainer.h"
#include "XPComboComponent.generated.h"


USTRUCT(BlueprintType)
struct FXPConfigRow : public FTableRowBase {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsAbility = false;  // Flag: true for ability rows
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float BaseXP = 0.f;  // For actions (damage/kill)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Exponent = 1.f;  // For actions
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Multiplier = 1.f;  // Event mult (e.g., kill=2)
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float AbilityMultiplier = 1.f;  // For abilities (e.g., grapple=1.5)
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTER_API UXPComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UXPComboComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintCallable, Category="XP")
	void OnAction(FName ActionType, float DamageOrValue = 0.f);  // Called on damage/kill
    UFUNCTION(BlueprintCallable, Category="XP")
	void StartAbility(const FName& AbilityName);
    UFUNCTION(BlueprintCallable, Category="XP")
	void EndAbility(const FName& AbilityName);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TotalXP)
	int32 TotalXP;  // For progression

	UFUNCTION()
	void OnRep_TotalXP();

    UPROPERTY(EditDefaultsOnly)
	UDataTable* XPConfigTable;
    UPROPERTY(EditDefaultsOnly)
	float ResetTime = 4.f;  // Seconds
	
	void SetHUD(class AShooterCrosshairHUD* HUD);

	UFUNCTION()
	void RetrySetHUD();

	UPROPERTY()
	class AShooterCrosshairHUD* ShooterHUD;

	UPROPERTY(Replicated)
	FPlayerProgress PlayerProgress = FPlayerProgress();

    int32 GetMetricValue(EProgressMetric Metric) const;
    void SetMetricValue(EProgressMetric Metric, int32 NewValue);
    void AddToMetric(EProgressMetric Metric, int32 Delta);

	void AddAttackMetric(const UDamageType* DamageType, class AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);

private:
    TSet<FName> ActiveAbilities;
    TMap<FName, float> CachedAbilityMultipliers;  // Loaded from table
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentSequenceScore, meta = (AllowPrivateAccess = "true"))
	int32 CurrentSequenceScore;

	UFUNCTION()
	void OnRep_CurrentSequenceScore();

    int32 ComboCount;
    float TimerRemaining;

	void ResetCombo();

	void ProcessAR(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessSMG(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessPistol(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessCyberPistol(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessSniper(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessGL(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessGraviton(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessDroneMissile(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessDronePulsar(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessCyberPunch(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
	void ProcessRush(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags);
};
