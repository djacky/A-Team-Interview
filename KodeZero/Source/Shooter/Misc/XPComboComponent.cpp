// Fill out your copyright notice in the Description page of Project Settings.


#include "Misc/XPComboComponent.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/ShooterCrosshairHUD.h"
#include "Shooter/ShooterPlayerState.h"
#include "Shooter/StructTypes/PlayerIdsStruct.h"
#include "Shooter/Misc/Interfaces/GeneralInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Shooter/PlayerController/ShooterPlayerController.h"
#include "Shooter/Items/AllDamageTypes.h"
#include "Shooter/EnumTypes/GameModeType.h"
#include "Shooter/Misc/NativeWeaponTags.h"


UXPComboComponent::UXPComboComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UXPComboComponent::BeginPlay()
{
	Super::BeginPlay();

    ResetCombo();

    // Cache ability multipliers
    if (XPConfigTable) {
        TArray<FXPConfigRow*> Rows;
        XPConfigTable->GetAllRows<FXPConfigRow>(TEXT(""), Rows);
        for (auto Row : Rows) {
            if (Row->bIsAbility) {
                CachedAbilityMultipliers.Add(XPConfigTable->GetRowNames()[Rows.Find(Row)], Row->AbilityMultiplier);
            }
        }
    }
}

void UXPComboComponent::SetHUD(AShooterCrosshairHUD* HUD)
{
    ShooterHUD = HUD;
    if (!ShooterHUD)
    {
        // If cast failed or HUD was null, retry via a separate fetch function
        if (UWorld* World = GetWorld())
        {
            FTimerHandle CheckHUDTimer;
            World->GetTimerManager().SetTimer(CheckHUDTimer, this, &UXPComboComponent::RetrySetHUD, 0.5f, false);
        }
    }
}

void UXPComboComponent::RetrySetHUD()
{
    if (AShooterPlayerState* PS = Cast<AShooterPlayerState>(GetOwner()))
    {
        if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(PS->GetPlayerController()))
        {
            SetHUD(PC->ShooterHUD);
        }
    }
}

void UXPComboComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (TimerRemaining > 0.f) {
        TimerRemaining -= DeltaTime;
        if (TimerRemaining <= 0.f) 
        {
            TotalXP += CurrentSequenceScore;
            UE_LOG(LogTemp, Warning, TEXT("TotalXP: %i | CurrentXP: %i"), TotalXP, CurrentSequenceScore);
			OnRep_TotalXP();
            ResetCombo();
            AddToMetric(EProgressMetric::EPM_StyleXP, CurrentSequenceScore);
        }
    }
}

void UXPComboComponent::StartAbility(const FName& AbilityName) {
    ActiveAbilities.Add(AbilityName);
}

void UXPComboComponent::EndAbility(const FName& AbilityName) {
    ActiveAbilities.Remove(AbilityName);
}

void UXPComboComponent::OnAction(FName ActionType, float DamageOrValue) {

    if (!XPConfigTable || ActiveAbilities.IsEmpty()) return;

    FXPConfigRow* Row = XPConfigTable->FindRow<FXPConfigRow>(ActionType, TEXT(""));
    if (!Row || Row->bIsAbility) return;

    // Calculate ability multiplier product (unchanged)
    float AbilityMult = 1.f;
    for (const FName& Ability : ActiveAbilities) {
        if (float* Mult = CachedAbilityMultipliers.Find(Ability)) {
            AbilityMult *= *Mult;
        }
    }

    ComboCount++;
    float ActionXP = Row->BaseXP * FMath::Pow(0.25f * ComboCount, Row->Exponent) * AbilityMult;

    // Compute final new score in a local var, then set once
    float NewScore = CurrentSequenceScore + ActionXP;
    if (Row->Multiplier > 1.f) {  // e.g., for kills
        NewScore *= Row->Multiplier;
    }
    CurrentSequenceScore = FMath::RoundToInt(NewScore);
	OnRep_CurrentSequenceScore();

    TimerRemaining = ResetTime;
    if (ActiveAbilities.Contains(FName("Grapple")) && ActionType == FName("Kill"))
    {
        AddToMetric(EProgressMetric::EPM_KillDuringGrapple, 1);
    }
}

void UXPComboComponent::ResetCombo() {
    CurrentSequenceScore = 0.f;
    ComboCount = 0;
}

void UXPComboComponent::OnRep_CurrentSequenceScore()
{
	if (CurrentSequenceScore > 0.f)
	{
		if (ShooterHUD)
		{
			ShooterHUD->ShowXPUpdate(TEXT("Style XP"), CurrentSequenceScore, false);
		}
	}
}

void UXPComboComponent::OnRep_TotalXP()
{
	TArray<FString> TitleArray = { TEXT("Sweet!"), TEXT("Nice!"), TEXT("Smooth!") };
	const int32 RandomIndex = FMath::RandRange(0, TitleArray.Num() - 1);
	if (ShooterHUD) ShooterHUD->ShowXPUpdate(TitleArray[RandomIndex], 0, true);
}

int32 UXPComboComponent::GetMetricValue(EProgressMetric Metric) const
{
    const FProgressMetricEntry* Entry = PlayerProgress.CurrentMetrics.FindByPredicate([Metric](const FProgressMetricEntry& E) { return E.Type == Metric; });
    return Entry ? Entry->Value : 0;
}

void UXPComboComponent::SetMetricValue(EProgressMetric Metric, int32 NewValue)
{
    FProgressMetricEntry* Entry = PlayerProgress.CurrentMetrics.FindByPredicate([Metric](const FProgressMetricEntry& E) { return E.Type == Metric; });
    if (Entry)
    {
        Entry->Value = NewValue;
    }
    else
    {
        FProgressMetricEntry NewEntry;
        NewEntry.Type = Metric;
        NewEntry.Value = NewValue;
        PlayerProgress.CurrentMetrics.Add(NewEntry);
    }
}

void UXPComboComponent::AddToMetric(EProgressMetric Metric, int32 Delta)
{
    SetMetricValue(Metric, GetMetricValue(Metric) + Delta);
}

void UXPComboComponent::AddAttackMetric(const UDamageType* DamageType, AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
	FGameplayTag AttackTag = FGameplayTag::EmptyTag;

    if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
    {
        APawn* AttackerPawn = PS->GetPawn();
        APawn* VictimPawn = VictimPS ? VictimPS->GetPawn() : nullptr;
        if (AttackerPawn && VictimPawn)
        {
            if (AttackerPawn->GetDistanceTo(VictimPawn) > 10000.f)
            {
                AddToMetric(EProgressMetric::EPM_ElimFrom100mPlus, 1);
            }
        }
    }

    if (StateTags.HasTagExact(TAG_State_Character_Crouched) && VictimPS)
    {
        if (!VictimPS->IsABot())
        {
            AddToMetric(EProgressMetric::EPM_ElimPlayerWhileCrouched, 1);
        }
            
    }

    if (StateTags.HasTagExact(TAG_State_Character_InAir) && VictimPS)
    {
        if (VictimPS->IsABot())
        {
            AddToMetric(EProgressMetric::EPM_ElimAiWhileInAir, 1);
        }
            
    }

	if (const UCustomDamageTypeBase* CustomDamageType = Cast<UCustomDamageTypeBase>(DamageType))
	{
		AttackTag = CustomDamageType->DamageTag;

		if (AttackTag == TAG_Attack_Weapon_AR)
		{
            ProcessAR(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_Pistol)
		{
			ProcessPistol(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_SubmachineGun)
		{
			ProcessSMG(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_Sniper)
		{
			ProcessSniper(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_Graviton)
		{
			ProcessGraviton(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_GrenadeLauncher)
		{
			ProcessGL(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Weapon_CyberPistol)
		{
			ProcessCyberPistol(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Character_Rush)
		{
			ProcessRush(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Item_CyberPunch)
		{
			ProcessCyberPunch(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Drone_Missile)
		{
			ProcessDroneMissile(VictimPS, StateTags);
		}
		else if (AttackTag == TAG_Attack_Drone_Pulsar)
		{
			ProcessDronePulsar(VictimPS, StateTags);
		}
	}
}

void UXPComboComponent::ProcessAR(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS && VictimPS->IsABot())
    {
        AddToMetric(EProgressMetric::EPM_ElimAiWithAR, 1);
    }
}
void UXPComboComponent::ProcessSMG(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (!VictimPS) return;
    if (VictimPS->IsABot())
    {
        AddToMetric(EProgressMetric::EPM_ElimAiWithSMG, 1);
    }
}
void UXPComboComponent::ProcessPistol(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS)
    {
        VictimPS->IsABot() ? AddToMetric(EProgressMetric::EPM_ElimAiWithPistol, 1) : AddToMetric(EProgressMetric::EPM_ElimPlayerWithPistol, 1);
    }
}
void UXPComboComponent::ProcessCyberPistol(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS && VictimPS->IsABot() && VictimPS->EnemyReplicationData.EnemyType == EEnemyType::EET_Howitzer)
    {
        AddToMetric(EProgressMetric::EPM_ElimMechWithCyberPistol, 1);
    }
}
void UXPComboComponent::ProcessSniper(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    UE_LOG(LogTemp, Warning, TEXT("ProcessSniper"));
    if (!VictimPS) return;
    !VictimPS->IsABot() ? AddToMetric(EProgressMetric::EPM_ElimPlayerWithSniper, 1) : AddToMetric(EProgressMetric::EPM_ElimAiWithSniper, 1);

}
void UXPComboComponent::ProcessGL(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (!VictimPS) return;
    if (VictimPS->IsABot())
    {
        AddToMetric(EProgressMetric::EPM_ElimAiWithGrenadeLauncher, 1);
    }
}
void UXPComboComponent::ProcessGraviton(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (!VictimPS) return;
    if (VictimPS->IsABot() && VictimPS->EnemyReplicationData.EnemyType == EEnemyType::EET_Howitzer)
    {
        AddToMetric(EProgressMetric::EPM_ElimMechWithGraviton, 1);
    }
    else if (!VictimPS->IsABot())
    {
        AddToMetric(EProgressMetric::EPM_ElimPlayerWithGraviton, 1);
    }
}
void UXPComboComponent::ProcessDroneMissile(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS)
    {
        AddToMetric(EProgressMetric::EPM_ElimWithDroneMissile, 1);
    }
}
void UXPComboComponent::ProcessDronePulsar(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS)
    {
        AddToMetric(EProgressMetric::EPM_ElimWithDronePulser, 1);
    }
}
void UXPComboComponent::ProcessCyberPunch(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (VictimPS)
    {
        AddToMetric(EProgressMetric::EPM_ElimPlayerWithCyberPunch, 1);
    }
}
void UXPComboComponent::ProcessRush(AShooterPlayerState* VictimPS, const FGameplayTagContainer& StateTags)
{
    if (!VictimPS) return;
    if (VictimPS->IsABot())
    {
        AddToMetric(EProgressMetric::EPM_ElimAiWithRushAttack, 1);
    }
}


void UXPComboComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UXPComboComponent, TotalXP, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UXPComboComponent, CurrentSequenceScore, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UXPComboComponent, PlayerProgress, COND_OwnerOnly);
}