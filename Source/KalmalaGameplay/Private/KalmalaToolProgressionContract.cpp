#include "KalmalaToolProgressionContract.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCharacter.h"
#include "KalmalaSkillProgressionComponent.h"
#include "EngineUtils.h"

namespace
{
    EKalmalaSkill ParseToolSkill(const FString& Value)
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

    const TCHAR* GetSkillName(const EKalmalaSkill Skill)
    {
        switch (Skill)
        {
        case EKalmalaSkill::Gathering: return TEXT("Gathering");
        case EKalmalaSkill::Woodcutting: return TEXT("Woodcutting");
        case EKalmalaSkill::Mining: return TEXT("Mining");
        case EKalmalaSkill::Crafting: return TEXT("Crafting");
        case EKalmalaSkill::Cooking: return TEXT("Cooking");
        case EKalmalaSkill::Survival: return TEXT("Survival");
        default: return TEXT("Skill");
        }
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
            if (!Catalogue) return LoadedEntries;
            LoadedEntries.Reserve(Catalogue->Recipes.Num());
            for (const FKalmalaToolRecipeData& Recipe : Catalogue->Recipes)
            {
                FKalmalaToolProgressionEntry& Entry = LoadedEntries.AddDefaulted_GetRef();
                Entry.ToolId = Recipe.ToolId;
                Entry.TargetToolLevel = Recipe.Level;
                Entry.RequiredStation = ParseToolStation(Recipe.RequiredStation);
                Entry.RequiredStationLevel = Recipe.RequiredStationLevel;
                Entry.RequiredSkill = ParseToolSkill(Recipe.RequiredSkill);
                Entry.RequiredSkillLevel = Recipe.RequiredSkillLevel;
                Entry.MaterialCosts.Reserve(Recipe.Ingredients.Num());
                for (const FKalmalaToolRecipeIngredientData& Ingredient : Recipe.Ingredients)
                {
                    Entry.MaterialCosts.Add({Ingredient.ItemId, Ingredient.Quantity});
                }
            }
            for (const FKalmalaToolRecipeData& Upgrade : Catalogue->UpgradeCosts)
            {
                const FKalmalaToolProfileData* Profile = Catalogue->FindProfile(Upgrade.ToolId);
                if (!Profile) return TArray<FKalmalaToolProgressionEntry>();
                const int32 StationTargetCap = FKalmalaToolProgressionContract::MaxStationToolLevel;
                for (int32 CurrentLevel = 1; CurrentLevel < StationTargetCap; ++CurrentLevel)
                {
                    TArray<FKalmalaToolRecipeIngredientData> Costs;
                    if (!UKalmalaToolCatalogue::ScaleUpgradeCosts(Upgrade.Ingredients, CurrentLevel, Costs))
                        return TArray<FKalmalaToolProgressionEntry>();
                    FKalmalaToolProgressionEntry& Entry = LoadedEntries.AddDefaulted_GetRef();
                    Entry.ToolId = Upgrade.ToolId;
                    Entry.PreviousToolId = Upgrade.ToolId;
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
                RecipeOrder.Add(Catalogue->Recipes[Index].ToolId, Index);
            LoadedEntries.Sort([&RecipeOrder](const FKalmalaToolProgressionEntry& A, const FKalmalaToolProgressionEntry& B)
            {
                const int32 AOrder = RecipeOrder.FindChecked(A.ToolId);
                const int32 BOrder = RecipeOrder.FindChecked(B.ToolId);
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

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(const FName ToolId)
{
    if (ToolId.IsNone()) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ToolId](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ToolId == ToolId;
    });
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindEntry(
    const FName ToolId, const int32 TargetToolLevel)
{
    if (ToolId.IsNone() || TargetToolLevel < 1 || TargetToolLevel > MaxStationToolLevel) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ToolId, TargetToolLevel](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ToolId == ToolId && Entry.TargetToolLevel == TargetToolLevel;
    });
}

const FKalmalaToolProgressionEntry* FKalmalaToolProgressionContract::FindNextEntry(
    const FName ToolId, const int32 CurrentToolLevel)
{
    if (ToolId.IsNone() || CurrentToolLevel < 0 || CurrentToolLevel >= MaxStationToolLevel) return nullptr;
    return ToolProgressionEntries().FindByPredicate([ToolId, CurrentToolLevel](const FKalmalaToolProgressionEntry& Entry)
    {
        return Entry.ToolId == ToolId && Entry.PreviousToolId == (CurrentToolLevel == 0 ? NAME_None : ToolId)
            && Entry.PreviousToolLevel == CurrentToolLevel;
    });
}

int32 FKalmalaToolProgressionContract::GetMaximumToolLevel(const FName ToolId)
{
    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(ToolId);
    if (!Definition) return 0;
    int32 MaximumLevel = 1;
    for (const FKalmalaToolLevelStats& Stats : Definition->LevelStats)
        MaximumLevel = FMath::Max(MaximumLevel, Stats.ToolLevel);
    // Legacy Iron Axe level two remains readable with level-one-equivalent stats.
    if (Definition->ToolId == TEXT("IronAxe")) MaximumLevel = FMath::Max(MaximumLevel, 2);
    return FMath::Min(MaximumLevel, FKalmalaToolLifecycleContract::MaxToolLevel);
}

bool FKalmalaToolProgressionContract::IsValidToolLevel(const FName ToolId, const int32 ToolLevel)
{
    if (ToolId.IsNone() || ToolLevel < 1 || ToolLevel > FKalmalaToolLifecycleContract::MaxToolLevel) return false;
    // Item validity is independent of station recipes, including levels 11–20.
    return FKalmalaToolLifecycleContract::GetMaxDurability(ToolId, ToolLevel) > 0;
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
        const FKalmalaToolDefinition* Tool = FKalmalaToolLifecycleContract::FindDefinition(Entry.ToolId);
        const FString EntryId = FString::Printf(TEXT("%s:%d"), *Entry.ToolId.ToString(), Entry.TargetToolLevel);
        if (Entry.ToolId.IsNone() || SeenEntryIds.Contains(EntryId) || Tool == nullptr
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

        if (Entry.PreviousToolId.IsNone())
        {
            if (Entry.PreviousToolLevel != 0 || Entry.TargetToolLevel != 1) return false;
        }
        else
        {
            const FKalmalaToolProgressionEntry* Previous = FindEntry(Entry.PreviousToolId, Entry.PreviousToolLevel);
            if (Entry.PreviousToolId != Entry.ToolId || Previous == nullptr
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
    const FKalmalaSkillProgressionLedger* ServerSkills,
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

    Reason = TEXT("Carried tool inventory is invalid or full");
    if (ExistingTools.IsEmpty() || ExistingTools.Num() > FKalmalaToolLifecycleContract::MaxCarriedToolRecords)
        return false;
    TSet<FName> SeenTools;
    for (const FKalmalaToolState& State : ExistingTools)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(State.ToolId);
        const int32 MaxDurability = FKalmalaToolLifecycleContract::GetMaxDurability(State.ToolId, State.ToolLevel);
        if (!Definition || SeenTools.Contains(State.ToolId) || !IsValidToolLevel(State.ToolId, State.ToolLevel)
            || State.ToolLevel > GetMaximumToolLevel(State.ToolId)
            || MaxDurability <= 0 || State.Durability < 0 || State.Durability > MaxDurability)
        {
            return false;
        }
        SeenTools.Add(State.ToolId);
    }

    const FKalmalaToolState* ExistingTarget = ExistingTools.FindByPredicate([ToolId](const FKalmalaToolState& State)
    {
        return State.ToolId == ToolId;
    });
    const int32 CurrentToolLevel = ExistingTarget ? ExistingTarget->ToolLevel : 0;
    const FKalmalaToolProgressionEntry* Entry = FindNextEntry(ToolId, CurrentToolLevel);
    Reason = TEXT("No same-item upgrade is available for that tool level");
    if (!Entry) return false;

    const FName RequiredStationKit = GetStationKit(Entry->RequiredStation);
    Reason = TEXT("A matching Workbench or Forge is required");
    if (RequiredStationKit.IsNone() || SelectedStationKit != RequiredStationKit) return false;
    Reason = FString::Printf(TEXT("%s level %d is required"),
        Entry->RequiredStation == EKalmalaToolStationKind::Workbench ? TEXT("Workbench") : TEXT("Forge"),
        Entry->RequiredStationLevel);
    if (EffectiveStationLevel != Entry->RequiredStationLevel) return false;

    if (Entry->RequiredSkill != EKalmalaSkill::None)
    {
        Reason = FString::Printf(TEXT("Requires %s level %d and its second-tier unlock"),
            GetSkillName(Entry->RequiredSkill), Entry->RequiredSkillLevel);
        if (!ServerSkills || !ServerSkills->IsValid()) return false;
        const FKalmalaSkillState* SkillState = ServerSkills->Find(Entry->RequiredSkill);
        const uint8 SecondTierMask = static_cast<uint8>(EKalmalaSkillUnlock::SecondTier);
        if (!SkillState || !SkillState->IsValid() || SkillState->Level < Entry->RequiredSkillLevel
            || (SkillState->UnlockMask & SecondTierMask) == 0) return false;
    }

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
    if (PreviousIndex != INDEX_NONE)
    {
        // A same-item upgrade preserves the inventory cell and current condition.
        CandidateTools[PreviousIndex].ToolLevel = Entry->TargetToolLevel;
    }
    else
    {
        FKalmalaToolState& OutputState = CandidateTools.AddDefaulted_GetRef();
        OutputState.ToolId = Entry->ToolId;
        OutputState.ToolLevel = Entry->TargetToolLevel;
        OutputState.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(
            OutputState.ToolId, OutputState.ToolLevel);
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

    OutTools = MoveTemp(CandidateTools);
    OutInventory = MoveTemp(CandidateInventory);
    if (Entry->PreviousToolId.IsNone())
    {
        Reason = FString::Printf(TEXT("Crafted %s at level %d"), *Entry->ToolId.ToString(), Entry->TargetToolLevel);
    }
    else
    {
        Reason = FString::Printf(TEXT("Upgraded %s to level %d"), *Entry->ToolId.ToString(), Entry->TargetToolLevel);
    }
    return true;
}

bool FKalmalaToolProgressionContract::BuildServerUpgradeFromCharacter(
    const AKalmalaCharacter* ServerCharacter,
    const FName SelectedStationKit,
    const int32 EffectiveStationLevel,
    const FName ToolId,
    const TArray<FKalmalaToolState>& ExistingTools,
    const TArray<FKalmalaInventoryStack>& ExistingInventory,
    TArray<FKalmalaToolState>& OutTools,
    TArray<FKalmalaInventoryStack>& OutInventory,
    FString& Reason)
{
    const bool bServerAuthority = IsValid(ServerCharacter) && ServerCharacter->HasAuthority();
    const UKalmalaSkillProgressionComponent* SkillProgression = bServerAuthority
        ? ServerCharacter->GetSkillProgressionComponent() : nullptr;
    const FKalmalaSkillProgressionLedger* ServerSkills = SkillProgression
        ? &SkillProgression->GetServerLedger() : nullptr;
    return BuildServerUpgrade(bServerAuthority, ServerSkills, SelectedStationKit,
        EffectiveStationLevel, ToolId, ExistingTools, ExistingInventory,
        OutTools, OutInventory, Reason);
}
