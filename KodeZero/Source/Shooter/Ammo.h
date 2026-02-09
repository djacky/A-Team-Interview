// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Shooter/EnumTypes/AmmoType.h"
#include "CoreMinimal.h"
#include "Item.h"
#include "Engine/DataTable.h"
#include "Ammo.generated.h"

USTRUCT(BlueprintType)
struct FAmmoDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* AmmoMeshTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* AmmoTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundCue* PickupSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundCue* EquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* MaterialInstance = nullptr;
};
/**
 * 
 */
UCLASS()
class SHOOTER_API AAmmo : public AItem
{
	GENERATED_BODY()

public:
	AAmmo();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnItemPickedUp(AShooterCharacter* InShooter) override;
	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed) override;

protected:

	virtual void BeginPlay() override;

	// Override of SetItemProperties so we can set AmmoMesh properties
	virtual void SetItemProperties() override;
	
	// Called when overlapping AreaSphere
	UFUNCTION()
	void AmmoSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void StopFalling() override;

private:
	// Mesh for the ammo pickup
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Ammo, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AmmoMesh;

	// The type of ammo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_AmmoType, Category = Ammo, meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType = EAmmoType::EAT_MAX;

	UFUNCTION()
	void OnRep_AmmoType();

	// The icon that shows up in the ammo widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ammo, meta = (AllowPrivateAccess = "true"))
	UTexture2D* AmmoTexture;

	// Overlap sphere for picking up component
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ammo, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AmmoCollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_AmmoState, Category = Ammo, meta = (AllowPrivateAccess = "true"))
	EItemState AmmoState = EItemState::EIS_Pickup;

public:
	
	FORCEINLINE UStaticMeshComponent* GetAmmoMesh() const { return AmmoMesh;}
	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }
	FORCEINLINE void SetAmmoType(EAmmoType AType) { AmmoType = AType; }
	FORCEINLINE USphereComponent* GetAmmoCollisionSphere() const { return AmmoCollisionSphere; }
	void SetAmmoState(EItemState State);
	
	void ThrowAmmo(FTransform CharacterTransform);
	virtual void EnableCustomDepth() override;
	virtual void DisableCustomDepth() override;

	void UpdateAmmo();

	virtual void OnRep_ItemState() override;

private:
	UFUNCTION()
	void OnRep_AmmoState();
};