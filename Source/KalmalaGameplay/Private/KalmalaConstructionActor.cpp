#include "KalmalaConstructionActor.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

AKalmalaConstructionActor::AKalmalaConstructionActor()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("ConstructionCollision"));
    Collision->SetBoxExtent(FVector(54, 54, 56));
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
}

void AKalmalaConstructionActor::InitializeFromServer(const FName InKit, const FString& InConstructionId)
{
    if (HasAuthority()) { ConstructionKit = InKit; ConstructionId = InConstructionId.Left(64); }
}

void AKalmalaConstructionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionKit);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionId);
}
