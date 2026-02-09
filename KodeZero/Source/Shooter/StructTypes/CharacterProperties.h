#pragma once

#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "CharacterProperties.generated.h"


USTRUCT(BlueprintType)
struct FCharacterProperties : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ShooterMeshName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* HandCombatMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* EmoteMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ElimMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* ChargeAttackMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* HitMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* WinMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SwingHLocation = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SwingLoLocation = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SwingRLocation = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector SwingLLocation = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* OpponentElimSound = nullptr;

};