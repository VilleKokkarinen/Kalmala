#pragma once

#include "CoreMinimal.h"

/** Buildable station family required by an authored tool recipe. */
enum class EKalmalaToolStationKind : uint8
{
    None,
    Workbench,
    Forge
};

/** One catalogue item and quantity in a server-authored tool recipe. */
struct KALMALAGAMEPLAY_API FKalmalaToolMaterialCost
{
    FName ItemId = NAME_None;
    int32 Quantity = 0;
};

/** Static progression data; it does not itself grant or upgrade a carried tool. */
struct KALMALAGAMEPLAY_API FKalmalaToolProgressionEntry
{
    FName ToolId = NAME_None;
    int32 TargetToolLevel = 0;
    FName PreviousToolId = NAME_None;
    int32 PreviousToolLevel = 0;
    EKalmalaToolStationKind RequiredStation = EKalmalaToolStationKind::None;
    int32 RequiredStationLevel = 0;
    TArray<FKalmalaToolMaterialCost> MaterialCosts;
};

/** Bounded authored progression catalogue for M9's two tiered axes. */
class KALMALAGAMEPLAY_API FKalmalaToolProgressionContract
{
public:
    static constexpr int32 MaxAxeProgressionEntries = 2;

    static const TArray<FKalmalaToolProgressionEntry>& GetEntries();
    static const FKalmalaToolProgressionEntry* FindEntry(FName ToolId);
    static bool IsCatalogueValid();
};
