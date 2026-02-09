// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shooter/EnumTypes/RarityType.h"
#include "Shooter/Misc/Interfaces/ItemInterface.h"
#include "Shooter/EnumTypes/ItemTypeMod.h"
#include "Item.generated.h"



UENUM(BlueprintType)
enum class EItemState : uint8
{
	EIS_Pickup UMETA(DisplayName = "Pickup"),
	EIS_Pickup2 UMETA(DisplayName = "Pickup2"),
	EIS_EquipInterping UMETA(DisplayName = "EquipInterping"),
	EIS_PickedUp UMETA(DisplayName = "PickedUp"),
	EIS_Equipped UMETA(DisplayName = "Equipped"),
	EIS_Falling UMETA(DisplayName = "Falling"),
	EIS_Unequipped UMETA(DisplayName = "Unequipped"),

	EIS_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FItemRarityTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor GlowColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LightColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor DarkColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumberOfStars = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* IconBackground = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CustomDepthStencil = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RarityDamagePercentage = 0.f;

};


UCLASS()
class SHOOTER_API AItem : public AActor, public IItemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//OnSphereOverlap and OnSphereEndOverlap have to be UFUNCTIONS
	// Called when overlapping AreaSphere
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult);

	// Called when end overlapping AreaSphere
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Sets the ActiveStars array of bools based on rarity
	void SetActiveStars();

	// Sets properties of the Item's components based on State
	// Function is made virtual so that we can override it in the Ammo class
	virtual void SetItemProperties();

	// Called when the ItemInterpTimer is finished
	void FinishInterping();

	// Handles item interpolation when in the EquipInterpingState
	void ItemInterp(float DeltaTime);

	// Get interp location based on the item type
	FVector GetInterpLocation();

	virtual void InitializeCustomDepth();

	// C++ version of ConstructionScript
	virtual void OnConstruction(const FTransform& Transform) override;

	void ResetPulseTimer();
	void UpdatePulse();

	virtual void OnEquipped();

	UFUNCTION(BlueprintCallable)
	virtual int32 GetAmmo() const { return 0; }

	UFUNCTION(BlueprintCallable)
	virtual int32 GetBoostItemAmount() const { return 0; }

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void StopFalling(); 
	bool bFalling = false;
	bool bInAir = false;

	UPROPERTY()
	FTimerHandle VelocityCheckTimer;

	UFUNCTION()
	void CheckIfSettled();

	UPROPERTY()
	FTimerHandle ThrowItemTimer;
	float ThrowItemTime = 0.7f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	// Skeletal mesh for the item
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ItemMesh;

	// Line trace collides with box to show HUD widgets
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* CollisionBox;

	// Popup widget for when the player looks at the item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* PickupWidget;

	// Enables Item tracing when overlapped
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* AreaSphere;

	// Name which appears on the Pickup Widget
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FString ItemName = "Default";

	// Item count (ammo, etc...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 ItemCount = 0;

	// Item rarity - detemines the number of stars in Pickup Widget
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ItemRarity, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	EItemRarity ItemRarity = EItemRarity::EIR_Common;

	UFUNCTION()
	void OnRep_ItemRarity();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	TArray<bool> ActiveStars;

	// State of the item
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_ItemState, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemState ItemState = EItemState::EIS_Pickup2;

	// The curve asset to use for the item's Z location when interping
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UCurveFloat* ItemZCurve;

	// Starting location when interping begins
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FVector ItemInterpStartLocation{0.7f};

	// Target interp location in front of the camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	FVector CameraTargetLocation{0.f};
	
	// True when interping
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	bool bInterping = false;

	// Plays when we start interping
	UPROPERTY()
	FTimerHandle ItemInterpTimer;

	// Pointer to the character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class AShooterCharacter* Character;
	
	// Duration of the curve and timer
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float ZCurveTime = 0.7f;

	// X and Y for the Item while interping in the EquipInterping state
	float ItemInterpX = 0.f;
	float ItemInterpY = 0.f;

	// Initial Yaw offset between the camera and the interping item
	float InterpInitialYawOffset = 0.f;

	// Curve used to scale the item when interping
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* ItemScaleCurve;

	// Sound played when you pick up Item
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* PickupSound;

	// Sound played when you equip a Item
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class USoundCue* EquipSound;
	
	// Enum for the type of item this Item is
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	EItemType ItemType = EItemType::EIT_MAX;
	
	// Index of the interp location this item is interping to
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 InterpLocIndex = 0;

	// Item Mesh material index
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	int32 MaterialIndex = 0;

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	// Material instance used with the Dynamic Material Instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	UMaterialInstance* MaterialInstance;

	// Curve to drive the dynamic material parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	class UCurveVector* PulseCurve;

	UPROPERTY()
	FTimerHandle PulseTimer;

	// Time for the Pulse Timer
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float PulseCurveTime = 5.f;

	// Material parameter in M_SMG_Mat_Inst
	UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float GlowAmount = 150.f;

	// Material parameter in M_SMG_Mat_Inst
	UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float FresnelExponent = 3.f;

	// Material parameter in M_SMG_Mat_Inst
	UPROPERTY(VisibleAnywhere, Category = "Item Properties", meta = (AllowPrivateAccess = "true"))
	float FresnelReflectFraction = 4.f;

	// Inventory item icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconItem;

	// Inventory ammo icon
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconAmmo;

	// Slot in the inventory array
	UPROPERTY(VisibleAnywhere, Replicated, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	int32 SlotIndex = 0;

	// Displays text on the PickupWidget to eith pick up or swap
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Inventory, meta = (AllowPrivateAccess = "true"))
	bool bSwapText = false;

	// Item rarity data table
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	class UDataTable* ItemRarityDataTable;

	// Color in the glow material
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor GlowColor;

	// Light Color in the pickup widget
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor LightColor;

	// Dark Color in the pickup widget
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	FLinearColor DarkColor;

	// Number of stars in the pickup widget
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	int32 NumberOfStars;

	// Icon Background in the inventory
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	UTexture2D* IconBackground;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rarity, meta = (AllowPrivateAccess = "true"))
	float RarityDamagePercentage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Crosshair, meta = (AllowPrivateAccess = "true"))
	UTexture2D* GeneralCrosshair;

public:

	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }	
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE UBoxComponent* GetCollisionBox() const { return CollisionBox; }
	FORCEINLINE EItemState GetItemState() const { return ItemState; }
	void SetItemState(EItemState State);
	FORCEINLINE USkeletalMeshComponent* GetItemMesh() const { return ItemMesh; }
	FORCEINLINE EItemRarity GetItemRarity() const { return ItemRarity; }
	FORCEINLINE void SetItemRarity(EItemRarity Rarity) { ItemRarity = Rarity; }
	FORCEINLINE float GetRarityDamagePercentage() const { return RarityDamagePercentage; }
	
	FORCEINLINE USoundCue* GetPickupSound() const { return PickupSound; }
	FORCEINLINE void SetPickupSound(USoundCue* SoundPickup) { PickupSound = SoundPickup; }

	FORCEINLINE USoundCue* GetEquipSound() const { return EquipSound; }
	FORCEINLINE void SetEquipSound(USoundCue* SoundEquip) { EquipSound = SoundEquip; }

	FORCEINLINE int32 GetItemCount() const { return ItemCount; }
	FORCEINLINE int32 GetSlotIndex() const { return SlotIndex; }
	FORCEINLINE EItemType GetItemType() const { return ItemType; }
	FORCEINLINE void SetSlotIndex(int32 Index) { SlotIndex = Index; }
	FORCEINLINE void SetInventoryFull(bool bFull) { bSwapText = bFull; }
	FORCEINLINE void SetItemName(FString Name) { ItemName = Name; }
	FORCEINLINE void SetItemCount(int32 Count) { ItemCount = Count; }
	FORCEINLINE void SetItemType(EItemType InType) { ItemType = InType; }

	FORCEINLINE UMaterialInstance* GetMaterialInstance() const { return MaterialInstance; }
	FORCEINLINE void SetMaterialInstance(UMaterialInstance* MaterialInt) { MaterialInstance = MaterialInt;}

	FORCEINLINE UMaterialInstanceDynamic* GetDynamicMaterialInstance() const { return DynamicMaterialInstance; }
	FORCEINLINE void SetDynamicMaterialInstance(UMaterialInstanceDynamic* Dynamic) { DynamicMaterialInstance = Dynamic; }

	FORCEINLINE int32 GetMaterialIndex() const { return MaterialIndex; }
	FORCEINLINE void SetMaterialIndex(int32 Index) { MaterialIndex = Index; }

	FORCEINLINE FLinearColor GetGlowColor() const { return GlowColor; }
	FORCEINLINE void SetGlowColor(FLinearColor Color) { GlowColor = Color; }

	// Set item icon for the inventory
	FORCEINLINE void SetIconItem(UTexture2D* Icon) { IconItem = Icon; }
	// Set ammo icon for the pickup widget
	FORCEINLINE void SetIconAmmo(UTexture2D* Icon) { IconAmmo = Icon; }
	// Set item icon for the inventory
	FORCEINLINE void SetIconBackground(UTexture2D* Background) { IconBackground = Background; }

	FORCEINLINE UTexture2D* GetGeneralCrosshair() const { return GeneralCrosshair; }

	// Called from the AShootercharacter class
	void StartItemCurve(AShooterCharacter* Char, bool bIsSwappingCurrentItem = false);

	virtual void EnableCustomDepth();
	virtual void DisableCustomDepth();

	void EnableGlowMaterial();
	void DisableGlowMaterial();
	void UpdateItem();
	void StartPulseTimer();

	virtual void OnItemPickedUp(AShooterCharacter* InShooter) override;
	virtual void OnEquipItem(AShooterCharacter* InShooter, UAnimInstance* InAnimInstance) override;
	virtual void OnDropItem(AShooterCharacter* InShooter, FTransform CharacterTransform, bool bPlayerElimed) override;
	virtual bool IsAnItem() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsAnNFT = false;

	virtual UStaticMeshComponent* GetNFTMesh();

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateSelectKey(const FText &SelectKeyName);

	// The number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, decremented in ClientUpdateAmmo.
	int32 Sequence = 0;

	bool bFallingFromContainer = false;

protected:
	UFUNCTION()
	virtual void OnRep_ItemState();

	//UPROPERTY()
	//class UMissileMovementComponent* MissileMovementComponent;

};
