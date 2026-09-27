#include "KalmalaToolProgressionContract.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaConstructionActor.h"
#include "EngineUtils.h"

namespace
{
    const TArray<FKalmalaStationAttachmentDefinition>& StationAttachmentDefinitions()
    {
        static const TArray<FKalmalaStationAttachmentDefinition> Definitions = {
            {TEXT("WorkbenchToolRackKit"), TEXT("WorkbenchKit")},
            {TEXT("ForgeAnvilKit"), TEXT("ForgeKit")}
        };
        return Definitions;
    }

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

const TArray<FKalmalaStationAttachmentDefinition>& FKalmalaToolProgressionContract::GetAttachmentDefinitions()
{
    return StationAttachmentDefinitions();
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(const FName ToolId)
{
    if (ToolId.IsNone()) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ToolId](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ToolId == ToolId;
    });
}

const FKalmalaStationAttachmentDefinition* FKalmalaToolProgressionContract::FindAttachment(const FName KitId)
{
    if (KitId.IsNone()) return nullptr;
    return StationAttachmentDefinitions().FindByPredicate([KitId](const FKalmalaStationAttachmentDefinition& Definition)
    {
        return Definition.KitId == KitId;
    });
}

bool FKalmalaToolProgressionContract::IsStationAttachmentKit(const FName KitId)
{
    return FindAttachment(KitId) != nullptr;
}

FName FKalmalaToolProgressionContract::GetAttachmentStationKit(const FName AttachmentKitId)
{
    const FKalmalaStationAttachmentDefinition* Definition = FindAttachment(AttachmentKitId);
    return Definition ? Definition->StationKitId : NAME_None;
}

bool FKalmalaToolProgressionContract::IsCatalogueValid()
{
    const TArray<FKalmalaToolProgressionEntry>& Entries = ToolProgressionEntries();
    const UKalmalaItemCatalogue* ItemCatalogue = GetDefault<UKalmalaItemCatalogue>();
    const TArray<FKalmalaStationAttachmentDefinition>& Attachments = StationAttachmentDefinitions();
    if (Entries.Num() != MaxAxeProgressionEntries || Attachments.Num() != 2 || ItemCatalogue == nullptr
        || !ItemCatalogue->IsValidCatalogue()) return false;

    TSet<FName> SeenAttachmentIds;
    for (const FKalmalaStationAttachmentDefinition& Attachment : Attachments)
    {
        if (Attachment.KitId.IsNone() || SeenAttachmentIds.Contains(Attachment.KitId)
            || GetBaseStationLevel(Attachment.StationKitId) != 1
            || !ItemCatalogue->IsValidStack(Attachment.KitId, 1)) return false;
        SeenAttachmentIds.Add(Attachment.KitId);
    }

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

FName FKalmalaToolProgressionContract::GetStationKit(const EKalmalaToolStationKind Station)
{
    switch (Station)
    {
    case EKalmalaToolStationKind::Workbench: return TEXT("WorkbenchKit");
    case EKalmalaToolStationKind::Forge: return TEXT("ForgeKit");
    default: return NAME_None;
    }
}

int32 FKalmalaToolProgressionContract::GetBaseStationLevel(const FName KitId)
{
    return KitId == TEXT("WorkbenchKit") || KitId == TEXT("ForgeKit") ? 1 : 0;
}

int32 FKalmalaToolProgressionContract::DeriveEffectiveStationLevel(
    const FName StationKitId,
    const TArray<FKalmalaStationAttachmentCandidate>& Attachments)
{
    const int32 BaseLevel = GetBaseStationLevel(StationKitId);
    if (BaseLevel == 0) return 0;

    for (const FKalmalaStationAttachmentCandidate& Candidate : Attachments)
    {
        const FKalmalaStationAttachmentDefinition* Definition = FindAttachment(Candidate.KitId);
        if (Definition && Definition->StationKitId == StationKitId && Candidate.bSameWorld
            && Candidate.bInitialized && FMath::IsFinite(Candidate.DistanceToStationCm)
            && Candidate.DistanceToStationCm >= 0.0f
            && Candidate.DistanceToStationCm <= MaxAttachmentDistanceCm)
        {
            return FMath::Min(MaxStationLevel, BaseLevel + 1);
        }
    }
    return BaseLevel;
}

int32 FKalmalaToolProgressionContract::GetEffectiveStationLevel(const AKalmalaConstructionActor* Station)
{
    if (!IsValid(Station) || Station->GetConstructionId().IsEmpty()
        || GetBaseStationLevel(Station->GetConstructionKit()) == 0
        || Station->GetActorLocation().ContainsNaN()) return 0;
    UWorld* World = Station->GetWorld();
    if (!World) return 0;

    TArray<FKalmalaStationAttachmentCandidate> Attachments;
    for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
    {
        const AKalmalaConstructionActor* CandidateActor = *It;
        if (!IsValid(CandidateActor) || CandidateActor->GetWorld() != World
            || (Station->HasAuthority() && !CandidateActor->HasAuthority())) continue;

        FKalmalaStationAttachmentCandidate& Candidate = Attachments.AddDefaulted_GetRef();
        Candidate.KitId = CandidateActor->GetConstructionKit();
        Candidate.bSameWorld = CandidateActor->GetWorld() == World;
        Candidate.bInitialized = !CandidateActor->GetConstructionId().IsEmpty()
            && !CandidateActor->GetActorLocation().ContainsNaN();
        if (Candidate.bInitialized)
        {
            const float DistanceSquared = FVector::DistSquared(
                Station->GetActorLocation(), CandidateActor->GetActorLocation());
            Candidate.DistanceToStationCm = FMath::Sqrt(DistanceSquared);
        }
    }
    return DeriveEffectiveStationLevel(Station->GetConstructionKit(), Attachments);
}

bool FKalmalaToolProgressionContract::CanPlaceAttachment(
    const FName AttachmentKitId,
    const FName StationKitId,
    const float DistanceToStationCm,
    const bool bStationUsable,
    const bool bAlreadyUpgraded,
    FString& Reason)
{
    const FKalmalaStationAttachmentDefinition* Definition = FindAttachment(AttachmentKitId);
    Reason = TEXT("Unknown station attachment");
    if (!Definition) return false;

    const TCHAR* StationName = Definition->StationKitId == TEXT("WorkbenchKit")
        ? TEXT("Workbench") : TEXT("Forge");
    Reason = TEXT("This attachment does not match that station");
    if (Definition->StationKitId != StationKitId) return false;
    Reason = FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"), StationName);
    if (!bStationUsable) return false;
    Reason = FString::Printf(TEXT("Place the attachment within %.2f m of the %s"),
        MaxAttachmentDistanceCm / 100.0f, StationName);
    if (!FMath::IsFinite(DistanceToStationCm) || DistanceToStationCm < 0.0f
        || DistanceToStationCm > MaxAttachmentDistanceCm) return false;
    Reason = TEXT("That station already has an attachment");
    if (bAlreadyUpgraded) return false;

    Reason = Definition->KitId == TEXT("WorkbenchToolRackKit")
        ? TEXT("Place the paid tool rack beside the Workbench to raise it to level 2")
        : TEXT("Place the paid anvil beside the Forge to raise it to level 2");
    return true;
}

bool FKalmalaToolProgressionContract::BuildServerUpgrade(
    const bool bServerAuthority,
    const FName SelectedStationKit,
    const int32 EffectiveStationLevel,
    const FName ToolId,
    const TArray<FKalmalaToolState>& ExistingTools,
    const TArray<FKalmalaInventoryStack>& ExistingInventory,
    TArray<FKalmalaToolState>& OutTools,
    TArray<FKalmalaInventoryStack>& OutInventory,
    FString& Reason)
{
    Reason = TEXT("Server authority required");
    if (!bServerAuthority) return false;
    Reason = TEXT("Tool progression catalogue unavailable");
    if (!IsCatalogueValid()) return false;

    const FKalmalaToolProgressionEntry* Entry = FindEntry(ToolId);
    Reason = TEXT("Unknown tool progression");
    if (!Entry) return false;
    const FName RequiredStationKit = GetStationKit(Entry->RequiredStation);
    Reason = TEXT("A matching Workbench or Forge is required");
    if (RequiredStationKit.IsNone() || SelectedStationKit != RequiredStationKit) return false;
    Reason = FString::Printf(TEXT("%s level %d is required"),
        Entry->RequiredStation == EKalmalaToolStationKind::Workbench ? TEXT("Workbench") : TEXT("Forge"),
        Entry->RequiredStationLevel);
    if (EffectiveStationLevel != Entry->RequiredStationLevel) return false;

    Reason = TEXT("Carried tool inventory is invalid or full");
    if (ExistingTools.IsEmpty() || ExistingTools.Num() > FKalmalaToolLifecycleContract::MaxCarriedToolRecords)
        return false;
    TSet<FName> SeenTools;
    for (const FKalmalaToolState& State : ExistingTools)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(State.ToolId);
        const FKalmalaToolProgressionEntry* ToolProgression = FindEntry(State.ToolId);
        const int32 MaximumLevel = ToolProgression ? ToolProgression->TargetToolLevel : 1;
        if (!Definition || SeenTools.Contains(State.ToolId) || State.ToolLevel < 1 || State.ToolLevel > MaximumLevel
            || State.Durability < 0 || State.Durability > Definition->MaxDurability)
        {
            return false;
        }
        SeenTools.Add(State.ToolId);
    }

    Reason = TEXT("That tool is already carried");
    if (SeenTools.Contains(Entry->ToolId)) return false;

    int32 PreviousIndex = INDEX_NONE;
    if (!Entry->PreviousToolId.IsNone())
    {
        PreviousIndex = ExistingTools.IndexOfByPredicate([Entry](const FKalmalaToolState& State)
        {
            return State.ToolId == Entry->PreviousToolId && State.ToolLevel == Entry->PreviousToolLevel;
        });
        Reason = TEXT("The required previous tool and level are not carried");
        if (PreviousIndex == INDEX_NONE) return false;
    }
    else if (Entry->TargetToolLevel != 1)
    {
        Reason = TEXT("The first tool progression step must target level one");
        return false;
    }

    const FKalmalaToolDefinition* OutputDefinition = FKalmalaToolLifecycleContract::FindDefinition(Entry->ToolId);
    Reason = TEXT("Tool definition is invalid");
    if (!OutputDefinition || (PreviousIndex == INDEX_NONE
        && ExistingTools.Num() >= FKalmalaToolLifecycleContract::MaxCarriedToolRecords)) return false;

    TArray<FKalmalaToolState> CandidateTools = ExistingTools;
    if (PreviousIndex != INDEX_NONE) CandidateTools.RemoveAt(PreviousIndex);
    FKalmalaToolState& OutputState = CandidateTools.AddDefaulted_GetRef();
    OutputState.ToolId = Entry->ToolId;
    OutputState.ToolLevel = Entry->TargetToolLevel;
    OutputState.Durability = OutputDefinition->MaxDurability;

    TArray<FKalmalaInventoryStack> Costs;
    Costs.Reserve(Entry->MaterialCosts.Num());
    for (const FKalmalaToolMaterialCost& Material : Entry->MaterialCosts)
    {
        FKalmalaInventoryStack& Cost = Costs.AddDefaulted_GetRef();
        Cost.ItemId = Material.ItemId;
        Cost.Quantity = Material.Quantity;
    }
    TArray<FKalmalaInventoryStack> CandidateInventory;
    if (!UKalmalaInventoryComponent::BuildExchange(
        ExistingInventory, Costs, NAME_None, 0, CandidateInventory, Reason))
    {
        return false;
    }

    OutTools = MoveTemp(CandidateTools);
    OutInventory = MoveTemp(CandidateInventory);
    Reason = FString::Printf(TEXT("Crafted %s at level %d"),
        *Entry->ToolId.ToString(), Entry->TargetToolLevel);
    return true;
}
