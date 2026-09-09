#include "KalmalaWorldMapExplorationSaveGame.h"

void UKalmalaWorldMapExplorationSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    SchemaVersion = CoverageSchemaVersion;
    WorldConfig = InWorldConfig;
    ExploredCells.Reset();
}

bool UKalmalaWorldMapExplorationSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const
{
    return SchemaVersion == CoverageSchemaVersion && WorldConfig == InWorldConfig;
}

FIntPoint UKalmalaWorldMapExplorationSaveGame::ToCoverageCell(const FVector2D WorldLocation)
{
    return FIntPoint(FMath::FloorToInt(WorldLocation.X / CoverageCellSize), FMath::FloorToInt(WorldLocation.Y / CoverageCellSize));
}

FVector2D UKalmalaWorldMapExplorationSaveGame::GetCoverageCellCentre(const FIntPoint Cell)
{
    return (FVector2D(Cell) + FVector2D(0.5f)) * CoverageCellSize;
}

bool UKalmalaWorldMapExplorationSaveGame::RecordReveal(const FVector2D WorldLocation, const float RevealRadius)
{
    if (!FMath::IsFinite(WorldLocation.X) || !FMath::IsFinite(WorldLocation.Y) || RevealRadius <= 0.0f) return false;
    const FIntPoint CentreCell = ToCoverageCell(WorldLocation);
    const int32 RadiusInCells = FMath::CeilToInt(RevealRadius / CoverageCellSize);
    bool bChanged = false;
    for (int32 Y = CentreCell.Y - RadiusInCells; Y <= CentreCell.Y + RadiusInCells; ++Y)
    {
        for (int32 X = CentreCell.X - RadiusInCells; X <= CentreCell.X + RadiusInCells; ++X)
        {
            const FIntPoint Cell(X, Y);
            if (FVector2D::DistSquared(GetCoverageCellCentre(Cell), WorldLocation) > FMath::Square(RevealRadius) || ExploredCells.Contains(Cell)) continue;
            ExploredCells.Add(Cell);
            bChanged = true;
        }
    }
    while (ExploredCells.Num() > MaxExploredCells)
    {
        ExploredCells.RemoveAt(0);
        bChanged = true;
    }
    return bChanged;
}

bool UKalmalaWorldMapExplorationSaveGame::IsExplored(const FVector2D WorldLocation) const
{
    return FMath::IsFinite(WorldLocation.X) && FMath::IsFinite(WorldLocation.Y) && ExploredCells.Contains(ToCoverageCell(WorldLocation));
}
