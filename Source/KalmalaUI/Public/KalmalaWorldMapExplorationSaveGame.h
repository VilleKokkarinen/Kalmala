#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldMapExplorationSaveGame.generated.h"

/**
 * Bounded, local-only map coverage. This deliberately has its own schema and
 * slot from server-owned generated-world deltas.
 */
UCLASS()
class KALMALAUI_API UKalmalaWorldMapExplorationSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CoverageSchemaVersion = 1;
    static constexpr float CoverageCellSize = 500.0f;
    static constexpr int32 MaxExploredCells = 8192;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const;
    bool RecordReveal(FVector2D WorldLocation, float RevealRadius);
    bool IsExplored(FVector2D WorldLocation) const;
    int32 GetExploredCellCount() const { return ExploredCells.Num(); }

private:
    static FIntPoint ToCoverageCell(FVector2D WorldLocation);
    static FVector2D GetCoverageCellCentre(FIntPoint Cell);

    UPROPERTY(SaveGame)
    int32 SchemaVersion = CoverageSchemaVersion;

    UPROPERTY(SaveGame)
    FKalmalaWorldGenerationConfig WorldConfig;

    /** Chronological order supports bounded oldest-first eviction. */
    UPROPERTY(SaveGame)
    TArray<FIntPoint> ExploredCells;
};
