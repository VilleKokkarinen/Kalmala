#include "KalmalaToolProgressionContract.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaToolLifecycleContract.h"

namespace
{
    const TArray<FKalmalaToolProgressionEntry>& ToolProgressionEntries()
    {
        static const TArray<FKalmalaToolProgressionEntry> Entries = []
        {
            FKalmalaToolProgressionEntry BronzeAxe;
            BronzeAxe.ToolId = TEXT("BronzeAxe");
            BronzeAxe.TargetToolLevel = 1;
            BronzeAxe.RequiredStation = EKalmalaToolStationKind::Workbench;
            BronzeAxe.RequiredStationLevel = 1;
            BronzeAxe.MaterialCosts = {
                {TEXT("Wood"), 4},
                {TEXT("Stone"), 3},
                {TEXT("Fibre"), 2}
            };

            FKalmalaToolProgressionEntry IronAxe;
            IronAxe.ToolId = TEXT("IronAxe");
            IronAxe.TargetToolLevel = 2;
            IronAxe.PreviousToolId = TEXT("BronzeAxe");
            IronAxe.PreviousToolLevel = 1;
            IronAxe.RequiredStation = EKalmalaToolStationKind::Forge;
            IronAxe.RequiredStationLevel = 2;
            IronAxe.MaterialCosts = {
                {TEXT("Lightwood"), 3},
                {TEXT("PeatAmber"), 2},
                {TEXT("Stone"), 4},
                {TEXT("Fibre"), 2}
            };

            return TArray<FKalmalaToolProgressionEntry>{BronzeAxe, IronAxe};
        }();
        return Entries;
    }
}

const TArray<FKalmalaToolProgressionEntry>& FKalmalaToolProgressionContract::GetEntries()
{
    return ToolProgressionEntries();
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(const FName ToolId)
{
    if (ToolId.IsNone()) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ToolId](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ToolId == ToolId;
    });
}

bool FKalmalaToolProgressionContract::IsCatalogueValid()
{
    const TArray<FKalmalaToolProgressionEntry>& Entries = ToolProgressionEntries();
    const UKalmalaItemCatalogue* ItemCatalogue = GetDefault<UKalmalaItemCatalogue>();
    if (Entries.Num() != MaxAxeProgressionEntries || ItemCatalogue == nullptr
        || !ItemCatalogue->IsValidCatalogue()) return false;

    TSet<FName> SeenToolIds;
    for (const FKalmalaToolProgressionEntry& Entry : Entries)
    {
        const FKalmalaToolDefinition* Tool = FKalmalaToolLifecycleContract::FindDefinition(Entry.ToolId);
        if (Entry.ToolId.IsNone() || SeenToolIds.Contains(Entry.ToolId) || Tool == nullptr
            || FKalmalaToolLifecycleContract::GetToolTier(Tool->Kind) == EKalmalaToolTier::None
            || Entry.TargetToolLevel <= 0 || Entry.RequiredStationLevel != Entry.TargetToolLevel
            || (Entry.RequiredStation != EKalmalaToolStationKind::Workbench
                && Entry.RequiredStation != EKalmalaToolStationKind::Forge)
            || Entry.MaterialCosts.IsEmpty() || Entry.MaterialCosts.Num() > 4)
        {
            return false;
        }

        SeenToolIds.Add(Entry.ToolId);
        TSet<FName> SeenCostIds;
        for (const FKalmalaToolMaterialCost& Cost : Entry.MaterialCosts)
        {
            if (SeenCostIds.Contains(Cost.ItemId) || !ItemCatalogue->IsValidStack(Cost.ItemId, Cost.Quantity))
                return false;
            SeenCostIds.Add(Cost.ItemId);
        }

        if (Entry.PreviousToolId.IsNone())
        {
            if (Entry.PreviousToolLevel != 0 || Entry.TargetToolLevel != 1) return false;
        }
        else
        {
            const FKalmalaToolProgressionEntry* Previous = FindEntry(Entry.PreviousToolId);
            if (Previous == nullptr || Entry.PreviousToolLevel != Previous->TargetToolLevel
                || Entry.TargetToolLevel != Entry.PreviousToolLevel + 1)
            {
                return false;
            }
        }
    }

    return true;
}
