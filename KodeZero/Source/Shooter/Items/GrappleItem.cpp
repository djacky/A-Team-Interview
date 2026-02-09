
#include "GrappleItem.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Shooter/ShooterCharacter.h"
#include "NiagaraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"



AGrappleItem::AGrappleItem()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Helicopter, ECollisionResponse::ECR_Block);
	CollisionBox->SetNotifyRigidBodyCollision(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(CollisionBox);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
	ProjectileMovementComponent->InitialSpeed = 3500.f;
	ProjectileMovementComponent->MaxSpeed = 3500.f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.5f;
	ProjectileMovementComponent->bShouldBounce = false;
	//ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &AGrappleItem::OnProjectileStop);
	
    // Assuming TrailComponent is created here:
    TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
    TrailComponent->SetupAttachment(RootComponent);
    TrailComponent->SetIsReplicated(true); // Replicate the component for state sync
	TrailComponent->SetAutoActivate(true);

}

void AGrappleItem::BeginPlay()
{
	Super::BeginPlay();
	if (CollisionBox)
	{
		CollisionBox->IgnoreActorWhenMoving(GetInstigator(), true);
		CollisionBox->OnComponentHit.AddDynamic(this, &AGrappleItem::OnHit);
	}

	GetWorldTimerManager().SetTimer(TimeoutHandle, this, &AGrappleItem::OnTimeout, 8.f, false);
}

void AGrappleItem::OnTimeout()
{
    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately();
    }

    if (bIsPredicted)
    {
        DeactivateTrail();
    }
    else if (HasAuthority())
    {
        DeactivateTrail();
    }
}

void AGrappleItem::DeactivateTrail()
{
    if (TrailComponent)
    {
        TrailComponent->Deactivate();
    }
    if (HasAuthority())
    {
        bTrailDeactivated = true;
    }
	if (ItemMesh) ItemMesh->SetVisibility(false);

    // Start destruction timer for authoritative and predicted items
    if (HasAuthority() || bIsPredicted)
    {
        GetWorldTimerManager().SetTimer(DestroyHandle, this, &AGrappleItem::DelayedDestroy, FadeTime, false);
    }
}

void AGrappleItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGrappleItem::OnRep_bTrailDeactivated()
{
    if (bTrailDeactivated && TrailComponent)
    {
        TrailComponent->Deactivate();
    }
}

void AGrappleItem::SetIsPredicted(bool Predicted)
{
    bIsPredicted = Predicted;
    if (bIsPredicted)
    {
        SetReplicates(false);
    }
}

void AGrappleItem::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//UE_LOG(LogTemp, Warning, TEXT("OnProjectileStop"));
    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately();
    }
    SetActorLocation(Hit.ImpactPoint);
	GetWorldTimerManager().ClearTimer(TimeoutHandle);

    AShooterCharacter* Char = Cast<AShooterCharacter>(GetOwner());
    if (Char)
    {
		DeactivateTrail();
        if (Char->GetHoverState() == EHoverState::EHS_HoverRush || Char->GetHoverState() == EHoverState::EHS_HoverStart || Char->bUsingItem)
        {
            // Cancel: don't start grapple
            return;
        }

        if (bIsPredicted)
        {
            Char->StartPredictedGrapple(Hit.ImpactPoint);
        }
        else if (HasAuthority())
        {
            Char->StartGrapple(Hit.ImpactPoint);
        }
    }
}

void AGrappleItem::SetVelocity(const FVector& Direction)
{
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = Direction * ProjectileMovementComponent->InitialSpeed;
	}
}

bool AGrappleItem::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
    if (!Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation))
    {
        return false;
    }

    const AController* ViewerController = Cast<AController>(RealViewer);
    if (!ViewerController)
    {
        return true; // Default to relevant if not a controller (e.g., for AI or other viewers)
    }

    const AController* InstController = GetInstigatorController();
    if (InstController && ViewerController == InstController)
    {
        return false; // Skip for the instigator's controller (owning client)
    }

    return true;
}

void AGrappleItem::DelayedDestroy()
{
    Destroy();
}

void AGrappleItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGrappleItem, bTrailDeactivated);
}