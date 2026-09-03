#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "KalmalaGeneratedTerrainTile.h"
#include "KalmalaTerrainPatchLayout.h"
#include "Net/UnrealNetwork.h"

AKalmalaGeneratedTerrainPatch::AKalmalaGeneratedTerrainPatch()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void AKalmalaGeneratedTerrainPatch::Initialize(const FKalmalaWorldGenerationConfig& InConfig, const FVector2D InPatchCenter)
{
    check(HasAuthority());
    WorldGenerationConfig = InConfig;
    PatchCenter = InPatchCenter;
    bIsConfigured = true;
    SpawnCollisionTiles();
    ForceNetUpdate();
}

void AKalmalaGeneratedTerrainPatch::OnRep_GenerationData()
{
    if (bIsConfigured)
    {
        UE_LOG(LogTemp, Display, TEXT("Client received the generated terrain collision patch descriptor."));
    }
}

void AKalmalaGeneratedTerrainPatch::SpawnCollisionTiles()
{
    if (!HasAuthority() || !bIsConfigured || !WorldGenerationConfig.IsValid())
    {
        return;
    }

    for (AKalmalaGeneratedTerrainTile* CollisionTile : CollisionTiles)
    {
        if (CollisionTile != nullptr)
        {
            CollisionTile->Destroy();
        }
    }
    CollisionTiles.Reset();

    const int32 HalfTileCount = FKalmalaTerrainPatchLayout::TilesPerSide / 2;
    for (int32 TileY = -HalfTileCount; TileY <= HalfTileCount; ++TileY)
    {
        for (int32 TileX = -HalfTileCount; TileX <= HalfTileCount; ++TileX)
        {
            const FTransform TileTransform(FRotator::ZeroRotator, FKalmalaTerrainPatchLayout::GetTileCenter(WorldGenerationConfig, PatchCenter, TileX, TileY));
            if (AKalmalaGeneratedTerrainTile* CollisionTile = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainTile>(AKalmalaGeneratedTerrainTile::StaticClass(), TileTransform))
            {
                CollisionTiles.Add(CollisionTile);
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Server spawned %d seed-derived terrain collision tiles."), CollisionTiles.Num());
}

void AKalmalaGeneratedTerrainPatch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, WorldGenerationConfig);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, PatchCenter);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, bIsConfigured);
}
