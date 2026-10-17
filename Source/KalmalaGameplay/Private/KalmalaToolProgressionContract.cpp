#include "KalmalaToolProgressionContract.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "EngineUtils.h"

namespace
{
    EKalmalaSkill ParseProgressionSkill(const FString& Value)
    {
        if (Value == TEXT("Gathering")) return EKalmalaSkill::Gathering;
        if (Value == TEXT("Woodcutting")) return EKalmalaSkill::Woodcutting;
        if (Value == TEXT("Mining")) return EKalmalaSkill::Mining;
        if (Value == TEXT("Crafting")) return EKalmalaSkill::Crafting;
        if (Value == TEXT("Cooking")) return EKalmalaSkill::Cooking;
        if (Value == TEXT("Survival")) return EKalmalaSkill::Survival;
        return EKalmalaSkill::None;
    }

    EKalmalaToolStationKind ParseToolStation(const FString& Value)
    {
        if (Value == TEXT("Workbench")) return EKalmalaToolStationKind::Workbench;
        if (Value == TEXT("Forge")) return EKalmalaToolStationKind::Forge;
        return EKalmalaToolStationKind::None;
    }


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
            TArray<FKalmalaToolProgressionEntry> LoadedEntries;
            const UKalmalaToolCatalogue* Catalogue = UKalmalaToolCatalogue::Get();
            const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
            if (!Catalogue || !Items) return LoadedEntries;
            LoadedEntries.Reserve(Catalogue->Recipes.Num());
            for (const FKalmalaToolRecipeData& Recipe : Catalogue->Recipes)
            {
                FKalmalaToolProgressionEntry& Entry = LoadedEntries.AddDefaulted_GetRef();
                Entry.ItemId = Recipe.ItemId;
                Entry.TargetToolLevel = Recipe.Level;
                Entry.RequiredStation = ParseToolStation(Recipe.RequiredStation);
                Entry.RequiredStationLevel = Recipe.RequiredStationLevel;
                Entry.RequiredSkill = ParseProgressionSkill(Recipe.RequiredSkill);
                Entry.RequiredSkillLevel = Recipe.RequiredSkillLevel;
                Entry.MaterialCosts.Reserve(Recipe.Ingredients.Num());
                for (const FKalmalaToolRecipeIngredientData& Ingredient : Recipe.Ingredients)
                {
                    Entry.MaterialCosts.Add({Ingredient.ItemId, Ingredient.Quantity});
                }
            }
            for (const FKalmalaToolRecipeData& Upgrade : Catalogue->UpgradeCosts)
            {
                const FKalmalaToolProfileData* Profile = Catalogue->FindProfile(Upgrade.ItemId);
                if (!Profile)
                {
                    const FKalmalaItemDefinition* GearItem = Items->FindItem(Upgrade.ItemId);
                    if (GearItem && !GearItem->GearSlot.IsEmpty()) continue;
                    return TArray<FKalmalaToolProgressionEntry>();
                }
                const int32 StationTargetCap = FKalmalaToolProgressionContract::MaxStationToolLevel;
                for (int32 CurrentLevel = 1; CurrentLevel < StationTargetCap; ++CurrentLevel)
                {
                    TArray<FKalmalaToolRecipeIngredientData> Costs;
                    if (!UKalmalaToolCatalogue::ScaleUpgradeCosts(Upgrade.Ingredients, CurrentLevel, Costs))
                        return TArray<FKalmalaToolProgressionEntry>();
                    FKalmalaToolProgressionEntry& Entry = LoadedEntries.AddDefaulted_GetRef();
                    Entry.ItemId = Upgrade.ItemId;
                    Entry.PreviousItemId = Upgrade.ItemId;
                    Entry.PreviousToolLevel = CurrentLevel;
                    Entry.TargetToolLevel = CurrentLevel + 1;
                    Entry.RequiredStation = ParseToolStation(Upgrade.RequiredStation);
                    Entry.RequiredStationLevel = Entry.TargetToolLevel;
                    for (const FKalmalaToolRecipeIngredientData& Cost : Costs)
                        Entry.MaterialCosts.Add({Cost.ItemId, Cost.Quantity});
                }
            }
            TMap<FName, int32> RecipeOrder;
            for (int32 Index = 0; Index < Catalogue->Recipes.Num(); ++Index)
                RecipeOrder.Add(Catalogue->Recipes[Index].ItemId, Index);
            LoadedEntries.Sort([&RecipeOrder](const FKalmalaToolProgressionEntry& A, const FKalmalaToolProgressionEntry& B)
            {
                const int32 AOrder = RecipeOrder.FindChecked(A.ItemId);
                const int32 BOrder = RecipeOrder.FindChecked(B.ItemId);
                return AOrder == BOrder ? A.TargetToolLevel < B.TargetToolLevel : AOrder < BOrder;
            });
            return LoadedEntries;
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

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(const FName ItemId)
{
    if (ItemId.IsNone()) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ItemId](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ItemId == ItemId;
    });
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(
    const FName ItemId, const int32 TargetToolLevel)
{
    if (ItemId.IsNone() || TargetToolLevel < 1 || TargetToolLevel > MaxStationToolLevel) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ItemId, TargetToolLevel](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ItemId == ItemId && Entry.TargetToolLevel == TargetToolLevel;
    });
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindNextEntry(
    const FName ItemId, const int32 CurrentToolLevel)
{
    if (ItemId.IsNone() || CurrentToolLevel < 0 || CurrentToolLevel >= MaxStationToolLevel) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ItemId, CurrentToolLevel](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ItemId == ItemId && Entry.PreviousItemId == (CurrentToolLevel == 0 ? NAME_None : ItemId)
            && Entry.PreviousToolLevel == CurrentToolLevel;
    });
}

int32 FKalmalaToolProgressionContract::GetMaximumToolLevel(const FName ItemId)
{
    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(ItemId);
    if (!Definition) return 0;
    int32 MaximumLevel = 1;
    for (const FKalmalaToolLevelStats& Stats : Definition->LevelStats)
        MaximumLevel = FMath::Max(MaximumLevel, Stats.ToolLevel);
    // Legacy Iron Axe level two remains readable with level-one-equivalent stats.
    if (Definition->ItemId == TEXT("IronAxe")) MaximumLevel = FMath::Max(MaximumLevel, 2);
    return FMath::Min(MaximumLevel, FKalmalaToolLifecycleContract::MaxToolLevel);
}

bool FKalmalaToolProgressionContract::IsValidToolLevel(const FName ItemId, const int32 ToolLevel)
{
    if (ItemId.IsNone() || ToolLevel < 1 || ToolLevel > FKalmalaToolLifecycleContract::MaxToolLevel) return false;
    // Item validity is independent of station recipes, including levels 11–20.
    return FKalmalaToolLifecycleContract::GetMaxDurability(ItemId, ToolLevel) > 0;
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
    const UKalmalaItemCatalogue* ItemCatalogue = UKalmalaItemCatalogue::Get();
    const TArray<FKalmalaStationAttachmentDefinition>& Attachments = StationAttachmentDefinitions();
    if (Entries.IsEmpty() || Entries.Num() > MaxToolProgressionEntries || Attachments.Num() != 2 || ItemCatalogue == nullptr
        || !ItemCatalogue->IsValidCatalogue()) return false;

    TSet<FName> SeenAttachmentIds;
    for (const FKalmalaStationAttachmentDefinition& Attachment : Attachments)
    {
        if (Attachment.KitId.IsNone() || SeenAttachmentIds.Contains(Attachment.KitId)
            || GetBaseStationLevel(Attachment.StationKitId) != 1
            || !ItemCatalogue->IsValidStack(Attachment.KitId, 1)) return false;
        SeenAttachmentIds.Add(Attachment.KitId);
    }

    TSet<FString> SeenEntryIds;
    for (const FKalmalaToolProgressionEntry& Entry : Entries)
    {
        const FKalmalaToolDefinition* Tool = FKalmalaToolLifecycleContract::FindDefinition(Entry.ItemId);
        const FString EntryId = FString::Printf(TEXT("%s:%d"), *Entry.ItemId.ToString(), Entry.TargetToolLevel);
        if (Entry.ItemId.IsNone() || SeenEntryIds.Contains(EntryId) || Tool == nullptr
            || FKalmalaToolLifecycleContract::GetToolTier(Tool->Kind) == EKalmalaToolTier::None
            || Entry.TargetToolLevel <= 0 || Entry.TargetToolLevel > MaxStationToolLevel
            || Entry.RequiredStationLevel != Entry.TargetToolLevel
            || (Entry.RequiredStation != EKalmalaToolStationKind::Workbench
                && Entry.RequiredStation != EKalmalaToolStationKind::Forge)
            || Entry.RequiredSkill != EKalmalaSkill::None || Entry.RequiredSkillLevel != 0
            || Entry.MaterialCosts.IsEmpty() || Entry.MaterialCosts.Num() > 4)
        {
            return false;
        }

        SeenEntryIds.Add(EntryId);
        TSet<FName> SeenCostIds;
        for (const FKalmalaToolMaterialCost& Cost : Entry.MaterialCosts)
        {
            if (SeenCostIds.Contains(Cost.ItemId) || !ItemCatalogue->FindItem(Cost.ItemId) || Cost.Quantity <= 0)
                return false;
            SeenCostIds.Add(Cost.ItemId);
        }

        if (Entry.PreviousItemId.IsNone())
        {
            if (Entry.PreviousToolLevel != 0 || Entry.TargetToolLevel != 1) return false;
        }
        else
        {
            const FKalmalaToolProgressionEntry* Previous = FindEntry(Entry.PreviousItemId, Entry.PreviousToolLevel);
            if (Entry.PreviousItemId != Entry.ItemId || Previous == nullptr
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
    const FKalmalaSkillProgressionLedger* /*UnusedSkills*/,
    const FName SelectedStationKit,
    const int32 EffectiveStationLevel,
    const FName ItemId,
    const TArray<FKalmalaToolState>& ExistingTools,
    const TArray<FKalmalaInventoryStack>& ExistingInventory,
    TArray<FKalmalaToolState>& OutTools,
    TArray<FKalmalaInventoryStack>& OutInventory,
    FString& Reason,
    const FGuid MainHandGuid)
{
    Reason = TEXT("Server authority required");
    if (!bServerAuthority) return false;
    Reason = TEXT("Tool progression catalogue unavailable");
    if (!IsCatalogueValid()) return false;

    Reason = TEXT("Carried item state is invalid");
    const int32 HandCount = MainHandGuid.IsValid() && ExistingTools.ContainsByPredicate(
        [MainHandGuid](const FKalmalaToolState& Tool) { return Tool.InstanceGuid == MainHandGuid; }) ? 1 : 0;
    if (ExistingTools.IsEmpty() || ExistingTools.Num() > UKalmalaInventoryComponent::MaxSlots
        || (MainHandGuid.IsValid() && HandCount == 0)
        || ExistingInventory.Num() + ExistingTools.Num() - HandCount > UKalmalaInventoryComponent::MaxSlots)
        return false;
    for (const FKalmalaToolState& State : ExistingTools)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(State.ItemId);
        const int32 MaxDurability = FKalmalaToolLifecycleContract::GetMaxDurability(State.ItemId, State.ToolLevel);
        if (!Definition || !IsValidToolLevel(State.ItemId, State.ToolLevel)
            || State.ToolLevel > GetMaximumToolLevel(State.ItemId)
            || MaxDurability <= 0 || State.Durability < 0 || State.Durability > MaxDurability)
        {
            return false;
        }
    }

    const FKalmalaToolState* ExistingTarget = ExistingTools.FindByPredicate([ItemId](const FKalmalaToolState& State)
    {
        return State.ItemId == ItemId;
    });
    const int32 CurrentToolLevel = ExistingTarget ? ExistingTarget->ToolLevel : 0;
    const FKalmalaToolProfileData* Profile = UKalmalaToolCatalogue::Get()->FindProfile(ItemId);
    const bool bRepeatableStarterAcquisition = Profile && Profile->ToolSet == TEXT("Starter");
    const FKalmalaToolProgressionEntry* Entry = bRepeatableStarterAcquisition
        ? FindEntry(ItemId, 1) : FindNextEntry(ItemId, CurrentToolLevel);
    Reason = TEXT("No same-item upgrade is available for that tool level");
    if (!Entry) return false;

    const FName RequiredStationKit = GetStationKit(Entry->RequiredStation);
    Reason = TEXT("A matching Workbench or Forge is required");
    if (RequiredStationKit.IsNone() || SelectedStationKit != RequiredStationKit) return false;
    Reason = FString::Printf(TEXT("%s level %d is required"),
        Entry->RequiredStation == EKalmalaToolStationKind::Workbench ? TEXT("Workbench") : TEXT("Forge"),
        Entry->RequiredStationLevel);
    if (EffectiveStationLevel != Entry->RequiredStationLevel) return false;


    int32 PreviousIndex = INDEX_NONE;
    if (!Entry->PreviousItemId.IsNone())
    {
        PreviousIndex = ExistingTools.IndexOfByPredicate([Entry](const FKalmalaToolState& State)
        {
            return State.ItemId == Entry->PreviousItemId && State.ToolLevel == Entry->PreviousToolLevel;
        });
        Reason = TEXT("The required previous tool and level are not carried");
        if (PreviousIndex == INDEX_NONE) return false;
    }
    else if (Entry->TargetToolLevel != 1)
    {
        Reason = TEXT("The first tool progression step must target level one");
        return false;
    }

    const FKalmalaToolDefinition* OutputDefinition = FKalmalaToolLifecycleContract::FindDefinition(Entry->ItemId);
    Reason = TEXT("Tool definition is invalid");
    if (!OutputDefinition) return false;

    TArray<FKalmalaToolState> CandidateTools = ExistingTools;
    if (PreviousIndex != INDEX_NONE)
    {
        // A same-item upgrade preserves the inventory cell and current condition.
        CandidateTools[PreviousIndex].ToolLevel = Entry->TargetToolLevel;
    }
    else
    {
        FKalmalaToolState& OutputState = CandidateTools.AddDefaulted_GetRef();
        OutputState.ItemId = Entry->ItemId;
        OutputState.ToolLevel = Entry->TargetToolLevel;
        OutputState.InstanceGuid = FGuid::NewGuid();
        OutputState.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(
            OutputState.ItemId, OutputState.ToolLevel);
    }

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
    if (CandidateTools.Num() > UKalmalaInventoryComponent::MaxSlots
        || CandidateTools.Num() + CandidateInventory.Num() - HandCount > UKalmalaInventoryComponent::MaxSlots)
    {
        Reason = TEXT("Inventory is full");
        return false;
    }

    OutTools = MoveTemp(CandidateTools);
    OutInventory = MoveTemp(CandidateInventory);
    if (Entry->PreviousItemId.IsNone())
    {
        Reason = FString::Printf(TEXT("Crafted %s at level %d"), *Entry->ItemId.ToString(), Entry->TargetToolLevel);
    }
    else
    {
        Reason = FString::Printf(TEXT("Upgraded %s to level %d"), *Entry->ItemId.ToString(), Entry->TargetToolLevel);
    }
    return true;
}

bool FKalmalaToolProgressionContract::BuildServerUpgradeFromCharacter(
    const AKalmalaCharacter* ServerCharacter,
    const FName SelectedStationKit,
    const int32 EffectiveStationLevel,
    const FName ItemId,
    const TArray<FKalmalaToolState>& ExistingTools,
    const TArray<FKalmalaInventoryStack>& ExistingInventory,
    TArray<FKalmalaToolState>& OutTools,
    TArray<FKalmalaInventoryStack>& OutInventory,
    FString& Reason)
{
    const bool bServerAuthority = IsValid(ServerCharacter) && ServerCharacter->HasAuthority();
    const auto* Inventory = bServerAuthority ? ServerCharacter->GetInventoryComponent() : nullptr;
    return BuildServerUpgrade(bServerAuthority, nullptr, SelectedStationKit,
        EffectiveStationLevel, ItemId, ExistingTools, ExistingInventory,
        OutTools, OutInventory, Reason, Inventory ? Inventory->GetMainHandGuid() : FGuid());
}
