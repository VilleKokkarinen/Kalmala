#include "KalmalaGeneratedTerrainTile.h"

#include "Components/BoxComponent.h"
#include "KalmalaTerrainPatchLayout.h"

AKalmalaGeneratedTerrainTile::AKalmalaGeneratedTerrainTile()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    NetPriority = 10.0f;
    SetNetUpdateFrequency(100.0f);
    SetReplicateMovement(true);

    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Collision->SetBoxExtent(FVector(
        FKalmalaTerrainPatchLayout::TileSize * 0.5f,
        FKalmalaTerrainPatchLayout::TileSize * 0.5f,
        FKalmalaTerrainPatchLayout::CollisionDepth));
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
}

void AKalmalaGeneratedTerrainTile::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Display, TEXT("Client received seed-derived terrain collision tile at %s."), *GetActorLocation().ToCompactString());
    }
}
