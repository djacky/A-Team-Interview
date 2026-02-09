// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterGlobalFuncs.h"
#include "../ShooterPlayerState.h"
#include "../PlayerController/ShooterPlayerController.h"
#include "../ShooterCharacter.h"
#include "Shooter/Misc/Interfaces/WeaponHitInterface.h"
#include "XPComboComponent.h"

void UCharacterGlobalFuncs::GetShooterReferences(
    AShooterPlayerController*& VictimController, AShooterPlayerController*& AttackerController,
    AShooterPlayerState*& VictimPS, AShooterPlayerState*& AttackerPS,
    AActor* VictimActor, AActor* AttackerActor, AController* InstigatorCon)
{
    // If Attacker is a missile system, treat Victim as the source
    if (AttackerActor && AttackerActor->GetName().Contains(TEXT("MissleSystem")))
    {
        AttackerActor = VictimActor;
    }

    // Victim references
    if (APawn* VictimPawn = Cast<APawn>(VictimActor))
    {
        // Get effective controller: current if possessed, fallback to Owner if unpossessed
        AController* EffectiveVictimCon = VictimPawn->GetController();
        if (!EffectiveVictimCon)
        {
            if (auto VictimShooter = Cast<AShooterCharacter>(VictimPawn))
            {
                EffectiveVictimCon = VictimShooter->LastController;
            }
        }

        if (EffectiveVictimCon)
        {
            // Attempt to set player-specific controller (null for AIs)
            VictimController = Cast<AShooterPlayerController>(EffectiveVictimCon);
            // Set player state for both players and AIs (assuming AShooterPlayerState is set on the controller's PlayerState for AIs)
            VictimPS = Cast<AShooterPlayerState>(EffectiveVictimCon->PlayerState);
        }
    }

    // Attacker references from instigator
    if (InstigatorCon)
    {
        AttackerController = Cast<AShooterPlayerController>(InstigatorCon);
        AttackerPS = Cast<AShooterPlayerState>(InstigatorCon->PlayerState);
        //UE_LOG(LogTemp, Warning, TEXT("AttackerPawn 1: %s"), *InstigatorCon->GetName());
        //if (AttackerController) UE_LOG(LogTemp, Warning, TEXT("AttackerPawn 2: %s"), *AttackerController->GetName());
        //if (AttackerPS) UE_LOG(LogTemp, Warning, TEXT("AttackerPawn 3: %s"), *AttackerPS->GetName());
    }
}

void UCharacterGlobalFuncs::UpdateDamageStat(AActor* DamagedCharacter, AActor* DamageCauserCharacter, 
	AShooterPlayerState* DamagedCharPS, AShooterPlayerState* DamageCauserCharPS, float Damage, float Health, const UDamageType* DamageType)
{
    if (DamagedCharPS)
    {
        if (DamageCauserCharPS && DamagedCharPS != DamageCauserCharPS) DamagedCharPS->OnEnterCombat();
        DamagedCharPS->PlayerGameStats.DamageTaken += Damage;
        if (Health <= 1e-4f)
        {
            DamagedCharPS->RegisterDeath();
        }
    }

    if (DamageCauserCharPS && DamagedCharPS != DamageCauserCharPS)
    {
        DamageCauserCharPS->OnEnterCombat();
        ApplyDamage(DamageCauserCharPS, DamagedCharPS, Damage);
        DamageCauserCharPS->PlayerGameStats.ShotsHit++;
        if (Health <= 1e-4f)
        {
            HandleKill(DamageCauserCharPS, DamagedCharPS);
            SetAttackMethodMetric(DamageCauserCharacter, DamageCauserCharPS, DamagedCharPS, DamageType);
        }
    }
}

void UCharacterGlobalFuncs::SetAttackMethodMetric(AActor* DamageCauser, AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS, const UDamageType* DamageType)
{
    if (AttackerPS && AttackerPS->XPComponent && !AttackerPS->IsABot() && VictimPS)
    {
        FGameplayTagContainer AttackerStateTags;
        AActor* InstigatorActor = AttackerPS->GetPawn();
		if (IWeaponHitInterface* HitInterface = Cast<IWeaponHitInterface>(InstigatorActor))
		{
			AttackerStateTags = HitInterface->Execute_GetStateTags(InstigatorActor);
		}
        AttackerPS->XPComponent->AddAttackMetric(DamageType, VictimPS, AttackerStateTags);
    }
}

void UCharacterGlobalFuncs::ApplyDamage(AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS, float DamageAmount)
{
    if (!AttackerPS) return;

    FPlayerGameStats& AttackerStats = AttackerPS->PlayerGameStats;
    AttackerStats.DamageDealt += DamageAmount;

    // Find existing entry or add new
    FVictimStats* Existing = AttackerStats.VictimInteractions.FindByPredicate([VictimPS](const FVictimStats& Entry) {
        return Entry.Victim == VictimPS;
    });

    if (Existing)
    {
        Existing->DamageDealt += DamageAmount;
    }
    else
    {
        FVictimStats NewEntry;
        NewEntry.Victim = VictimPS;
        NewEntry.DamageDealt = DamageAmount;
        NewEntry.Kills = 0;  // Will increment separately
        AttackerStats.VictimInteractions.Add(NewEntry);
    }

    if (AttackerPS->XPComponent)
    {
        AttackerPS->XPComponent->OnAction(FName("Damage"), DamageAmount);
    }
}

void UCharacterGlobalFuncs::HandleKill(AShooterPlayerState* AttackerPS, AShooterPlayerState* VictimPS)
{
    if (!AttackerPS) return;

    FPlayerGameStats& KillerStats = AttackerPS->PlayerGameStats;
    AttackerPS->RegisterKill(VictimPS);

    FVictimStats* Existing = KillerStats.VictimInteractions.FindByPredicate([VictimPS](const FVictimStats& Entry) {
        return Entry.Victim == VictimPS;
    });

    if (Existing)
    {
        Existing->Kills += 1;
    }
    else
    {
        // Rare: Kill without prior damage? Still add
        FVictimStats NewEntry;
        NewEntry.Victim = VictimPS;
        NewEntry.DamageDealt = 0.f;  // Or any final damage
        NewEntry.Kills = 1;
        KillerStats.VictimInteractions.Add(NewEntry);
    }
    if (AttackerPS->XPComponent)
    {
        AttackerPS->XPComponent->OnAction(FName("Kill"));
    }
}

FEnemyData* UCharacterGlobalFuncs::GetEnemyDataRow(EEnemyType EnemyType)
{
    const FString EnemyTablePath(TEXT("DataTable'/Game/_Game/DataTables/AIEnemyDataTable_DT.AIEnemyDataTable_DT'"));
    UDataTable* EnemyTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *EnemyTablePath));
    if (!EnemyTableObject)
    {
        return nullptr;
    }

    switch (EnemyType)
    {
        case EEnemyType::EET_TwinBlast:
            return EnemyTableObject->FindRow<FEnemyData>(FName("TwinBlast"), TEXT(""));
        case EEnemyType::EET_Howitzer:
            return EnemyTableObject->FindRow<FEnemyData>(FName("Howitzer"), TEXT(""));
        case EEnemyType::EET_Default:
            return EnemyTableObject->FindRow<FEnemyData>(FName("Default"), TEXT(""));
        // Add other EnemyTypes here as needed
        default:
            return nullptr;
    }
}
