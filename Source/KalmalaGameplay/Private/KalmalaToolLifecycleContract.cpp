#include "KalmalaToolLifecycleContract.h"

#include "KalmalaBiomeContentContract.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaToolCatalogue.h"

namespace
{
    EKalmalaToolKind ParseToolKind(const FString& Value)
    {
        if (Value == TEXT("ReedKnife")) return EKalmalaToolKind::StoneKnife;
        if (Value == TEXT("FieldHatchet")) return EKalmalaToolKind::StoneAxe;
        if (Value == TEXT("StoneKnife")) return EKalmalaToolKind::StoneKnife;
        if (Value == TEXT("StoneAxe")) return EKalmalaToolKind::StoneAxe;
        if (Value == TEXT("StonePick")) return EKalmalaToolKind::StonePick;
        if (Value == TEXT("StoneSpear")) return EKalmalaToolKind::StoneSpear;
        if (Value == TEXT("BronzeAxe")) return EKalmalaToolKind::BronzeAxe;
        if (Value == TEXT("BronzeKnife")) return EKalmalaToolKind::BronzeKnife;
        if (Value == TEXT("BronzePick")) return EKalmalaToolKind::BronzePick;
        if (Value == TEXT("BronzeSpear")) return EKalmalaToolKind::BronzeSpear;
        if (Value == TEXT("IronAxe")) return EKalmalaToolKind::IronAxe;
        if (Value == TEXT("IronKnife")) return EKalmalaToolKind::IronKnife;
        if (Value == TEXT("IronPick")) return EKalmalaToolKind::IronPick;
        if (Value == TEXT("IronSpear")) return EKalmalaToolKind::IronSpear;
        if (Value == TEXT("ConstructionHammer")) return EKalmalaToolKind::ConstructionHammer;
        if (Value == TEXT("StoneMace")) return EKalmalaToolKind::StoneMace;
        if (Value == TEXT("BronzeMace")) return EKalmalaToolKind::BronzeMace;
        if (Value == TEXT("IronMace")) return EKalmalaToolKind::IronMace;
        return EKalmalaToolKind::None;
    }

    EKalmalaToolAction ParseToolAction(const FString& Value)
    {
        if (Value == TEXT("Gathering")) return EKalmalaToolAction::Gathering;
        if (Value == TEXT("Woodcutting")) return EKalmalaToolAction::Woodcutting;
        if (Value == TEXT("Mining")) return EKalmalaToolAction::Mining;
        return EKalmalaToolAction::None;
    }

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

    FKalmalaToolDefinition ConvertProfile(const FKalmalaToolProfileData& Profile, const UKalmalaToolCatalogue& Catalogue)
    {
        FKalmalaToolDefinition Definition;
        Definition.ToolId = Profile.ToolId;
        Definition.Kind = ParseToolKind(Profile.ToolKind);
        Definition.Action = ParseToolAction(Profile.Action);
        Definition.RequiredSkill = ParseToolSkill(Profile.RequiredSkill);
        Definition.MinimumSkillLevel = Profile.MinimumSkillLevel;
        Definition.Attacks = Catalogue.GetAttacks(Profile.ToolId);
        for (int32 ToolLevel = 1; ToolLevel <= FKalmalaToolLifecycleContract::MaxToolLevel; ++ToolLevel)
        {
            FKalmalaToolLevelStats Level;
            if (!Catalogue.GetLevelStats(Profile.ToolId, ToolLevel, Level)) return FKalmalaToolDefinition();
            Definition.LevelStats.Add(Level);
            if (ToolLevel == 1)
            {
                Definition.MaxDurability = Level.MaxDurability;
                Definition.DamageBonuses = Level.DamageBonuses;
            }
        }
        return Definition;
    }

    TArray<FKalmalaToolDefinition> BuildDefinitionsForSet(const TCHAR* ToolSet)
    {
        TArray<FKalmalaToolDefinition> Definitions;
        const UKalmalaToolCatalogue* Catalogue = UKalmalaToolCatalogue::Get();
        if (!Catalogue) return Definitions;
        for (const FKalmalaToolProfileData& Profile : Catalogue->Profiles)
        {
            if (Profile.ToolSet == ToolSet) Definitions.Add(ConvertProfile(Profile, *Catalogue));
        }
        return Definitions;
    }

    const TArray<FKalmalaToolDefinition>& ToolDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions = BuildDefinitionsForSet(TEXT("Starter"));
        return Definitions;
    }

    const TArray<FKalmalaToolDefinition>& TieredToolDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions = BuildDefinitionsForSet(TEXT("Crafted"));
        return Definitions;
    }

    const FKalmalaToolDefinition& ConstructionHammerDefinition()
    {
        static const FKalmalaToolDefinition Definition = []
        {
            const UKalmalaToolCatalogue* Catalogue = UKalmalaToolCatalogue::Get();
            const FKalmalaToolProfileData* Profile = Catalogue
                ? Catalogue->FindProfile(TEXT("ConstructionHammer")) : nullptr;
            return Profile ? ConvertProfile(*Profile, *Catalogue) : FKalmalaToolDefinition();
        }();
        return Definition;
    }

    bool IsSelectionShapeValid(const FKalmalaToolServerSelection& Selection)
    {
        const bool bExactTool = FKalmalaToolLifecycleContract::IsKnownTool(Selection.RequiredTool)
            && Selection.MinimumToolTier == EKalmalaToolTier::None;
        const bool bTieredTool = Selection.RequiredTool == EKalmalaToolKind::None
            && FKalmalaToolLifecycleContract::IsKnownToolTier(Selection.MinimumToolTier);
        return !Selection.SourceId.IsNone()
            && FKalmalaToolLifecycleContract::IsKnownAction(Selection.Action)
            && (bExactTool || bTieredTool)
            && FKalmalaSkillProgressionContract::IsKnownSkill(Selection.RequiredSkill)
            && !Selection.RewardItemId.IsNone()
            && Selection.RewardQuantity == 1;
    }
}

const TArray<FKalmalaToolDefinition>& FKalmalaToolLifecycleContract::GetDefinitions()
{
    return ToolDefinitions();
}

const TArray<FKalmalaToolDefinition>& FKalmalaToolLifecycleContract::GetTieredToolDefinitions()
{
    return TieredToolDefinitions();
}

const FKalmalaToolDefinition& FKalmalaToolLifecycleContract::GetConstructionHammerDefinition()
{
    return ConstructionHammerDefinition();
}

TArray<FKalmalaToolState> FKalmalaToolLifecycleContract::BuildInitialCarriedTools()
{
    TArray<FKalmalaToolState> CarriedTools;
    CarriedTools.Reserve(FMath::Min(GetDefinitions().Num() + 1, MaxCarriedToolRecords));
    for (const FKalmalaToolDefinition& Definition : GetDefinitions())
    {
        if (CarriedTools.Num() >= MaxCarriedToolRecords || Definition.ToolId.IsNone()
            || Definition.MaxDurability <= 0) break;

        FKalmalaToolState State;
        State.ToolId = Definition.ToolId;
        State.ToolLevel = 1;
        State.Durability = Definition.MaxDurability;
        CarriedTools.Add(State);
    }
    if (CarriedTools.Num() < MaxCarriedToolRecords)
    {
        FKalmalaToolState HammerState;
        HammerState.ToolId = ConstructionHammerDefinition().ToolId;
        HammerState.ToolLevel = 1;
        HammerState.Durability = ConstructionHammerDefinition().MaxDurability;
        CarriedTools.Add(HammerState);
    }
    return CarriedTools;
}

FName FKalmalaToolLifecycleContract::CanonicalizeToolId(FName ToolId)
{
    if (ToolId == TEXT("ReedKnife")) return TEXT("StoneKnife");
    if (ToolId == TEXT("FieldHatchet")) return TEXT("StoneAxe");
    return ToolId;
}

const FKalmalaToolDefinition* FKalmalaToolLifecycleContract::FindDefinition(const FName ToolId)
{
    const FName CanonicalId = CanonicalizeToolId(ToolId);
    const FKalmalaToolDefinition* Definition = ToolDefinitions().FindByPredicate([CanonicalId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == CanonicalId;
    });
    if (Definition != nullptr) return Definition;
    Definition = TieredToolDefinitions().FindByPredicate([CanonicalId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == CanonicalId;
    });
    if (Definition != nullptr) return Definition;
    return CanonicalId == ConstructionHammerDefinition().ToolId ? &ConstructionHammerDefinition() : nullptr;
}

int32 FKalmalaToolLifecycleContract::GetMaxDurability(const FName ToolId, const int32 ToolLevel)
{
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    if (Definition == nullptr || ToolLevel < 1 || ToolLevel > MaxToolLevel) return 0;
    const FKalmalaToolLevelStats* LevelStats = Definition->LevelStats.FindByPredicate([ToolLevel](const auto& Stats)
    {
        return Stats.ToolLevel == ToolLevel;
    });
    if (LevelStats) return LevelStats->MaxDurability;
    // Older saves can contain Iron Axe level two even though it has no authored upgrade recipe.
    return CanonicalizeToolId(ToolId) == TEXT("IronAxe") && ToolLevel == 2 ? Definition->MaxDurability : 0;
}

FKalmalaDamageChannels FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(const FName ToolId, const int32 ToolLevel)
{
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    if (!Definition || GetMaxDurability(ToolId, ToolLevel) <= 0) return {};
    const FKalmalaToolLevelStats* Stats = Definition->LevelStats.FindByPredicate(
        [ToolLevel](const FKalmalaToolLevelStats& Level) { return Level.ToolLevel == ToolLevel; });
    return Stats ? Stats->DamageBonuses : Definition->DamageBonuses;
}

FKalmalaDamageChannels FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
    const FName ToolId, const int32 ToolLevel, const FName AttackId)
{
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    if (!Definition || GetMaxDurability(ToolId, ToolLevel) <= 0) return {};
    const FKalmalaWeaponAttackData* Attack = Definition->Attacks.FindByPredicate(
        [AttackId](const FKalmalaWeaponAttackData& Entry) { return Entry.AttackId == AttackId; });
    return Attack ? GetWeaponDamageBonuses(ToolId, ToolLevel).Added(Attack->DamageBonuses) : FKalmalaDamageChannels();
}

float FKalmalaToolLifecycleContract::GetSlashDamage(const FName ToolId, const int32 ToolLevel)
{
    return GetWeaponDamageBonuses(ToolId, ToolLevel).Slash;
}

float FKalmalaToolLifecycleContract::GetWeaponDamage(const FName ToolId, const int32 ToolLevel)
{
    return GetWeaponDamageBonuses(ToolId, ToolLevel).Total();
}

bool FKalmalaToolLifecycleContract::IsKnownAction(const EKalmalaToolAction Action)
{
    return Action >= EKalmalaToolAction::Gathering && Action <= EKalmalaToolAction::Mining;
}

bool FKalmalaToolLifecycleContract::IsKnownTool(const EKalmalaToolKind Tool)
{
    return Tool >= EKalmalaToolKind::StoneKnife && Tool <= EKalmalaToolKind::IronMace;
}

bool FKalmalaToolLifecycleContract::IsKnownToolTier(const EKalmalaToolTier Tier)
{
    return Tier >= EKalmalaToolTier::Stone && Tier <= EKalmalaToolTier::Iron;
}

EKalmalaToolTier FKalmalaToolLifecycleContract::GetToolTier(const EKalmalaToolKind Tool)
{
    const UKalmalaToolCatalogue* Catalogue = UKalmalaToolCatalogue::Get();
    if (Catalogue)
    {
        for (const FKalmalaToolProfileData& Profile : Catalogue->Profiles)
        {
            if (ParseToolKind(Profile.ToolKind) == Tool
                && (Profile.Action == TEXT("Woodcutting") || Profile.Action == TEXT("Mining")))
            {
                return static_cast<EKalmalaToolTier>(Profile.MaterialTier);
            }
        }
    }

    switch (Tool)
    {
    case EKalmalaToolKind::StoneKnife:
    case EKalmalaToolKind::StoneMace:
    case EKalmalaToolKind::StoneSpear: return EKalmalaToolTier::Stone;
    case EKalmalaToolKind::BronzeKnife:
    case EKalmalaToolKind::BronzeMace:
    case EKalmalaToolKind::BronzeSpear: return EKalmalaToolTier::Bronze;
    case EKalmalaToolKind::IronKnife:
    case EKalmalaToolKind::IronMace:
    case EKalmalaToolKind::IronSpear: return EKalmalaToolTier::Iron;
    default: return EKalmalaToolTier::None;
    }
}

EKalmalaToolTier FKalmalaToolLifecycleContract::GetToolTier(
    const EKalmalaToolKind Tool, const int32)
{
    // A tool's material tier is fixed by its profile; progression level only affects item stats.
    return GetToolTier(Tool);
}

bool FKalmalaToolLifecycleContract::BuildServerSelection(
    const FName ServerSourceId,
    FKalmalaToolServerSelection& OutSelection)
{
    OutSelection = {};
    if (ServerSourceId.IsNone()
        || !FKalmalaBiomeContentContract::IsValidGatheringSourceId(ServerSourceId)) return false;

    if (ServerSourceId == TEXT("meadows-birch-bark") || ServerSourceId == TEXT("elderwood-resinwood"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None,
            EKalmalaSkill::Woodcutting, TEXT("Wood"), 1, EKalmalaToolTier::Stone };
    }
    else if (ServerSourceId == TEXT("lakes-reed-cluster") || ServerSourceId == TEXT("tundra-frostmoss"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Gathering, EKalmalaToolKind::None,
            EKalmalaSkill::Gathering, TEXT("Fibre"), 1, EKalmalaToolTier::Stone };
    }
    else if (ServerSourceId == TEXT("mire-bog-iron"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Mining, EKalmalaToolKind::None,
            EKalmalaSkill::Mining, TEXT("Iron"), 1, EKalmalaToolTier::Bronze };
    }
    else if (ServerSourceId == TEXT("mountains-slate-vein"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Mining, EKalmalaToolKind::None,
            EKalmalaSkill::Mining, TEXT("Stone"), 1, EKalmalaToolTier::Stone };
    }
    else if (ServerSourceId == TEXT("meadows-birch-trunk"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaSkill::Woodcutting, TEXT("Lightwood"), 1, EKalmalaToolTier::Bronze };
    }
    else if (ServerSourceId == TEXT("elderwood-ironheart-trunk"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaSkill::Woodcutting, TEXT("Densewood"), 1, EKalmalaToolTier::Iron };
    }
    else if (ServerSourceId == TEXT("mire-peat-amber-seam") || ServerSourceId == TEXT("tundra-frost-salt-deposit"))
    {
        FName RewardItemId;
        if (!FKalmalaM9SourceLootContract::GetPrimaryItemId(ServerSourceId, RewardItemId)) return false;
        OutSelection = { ServerSourceId, EKalmalaToolAction::Mining, EKalmalaToolKind::None,
            EKalmalaSkill::Mining, RewardItemId, 1, EKalmalaToolTier::Stone };
    }

    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    return IsSelectionShapeValid(OutSelection)
        && Catalogue != nullptr
        && Catalogue->IsValidStack(OutSelection.RewardItemId, OutSelection.RewardQuantity);
}

bool FKalmalaToolLifecycleContract::IsResourceFamilyCompatible(
    const EKalmalaToolKind Kind, const EKalmalaToolAction ResourceAction)
{
    switch (Kind)
    {
    case EKalmalaToolKind::StoneAxe:
    case EKalmalaToolKind::BronzeAxe:
    case EKalmalaToolKind::IronAxe:
        return ResourceAction == EKalmalaToolAction::Woodcutting;
    case EKalmalaToolKind::StonePick:
    case EKalmalaToolKind::BronzePick:
    case EKalmalaToolKind::IronPick:
        return ResourceAction == EKalmalaToolAction::Mining;
    case EKalmalaToolKind::StoneKnife:
    case EKalmalaToolKind::BronzeKnife:
    case EKalmalaToolKind::IronKnife:
        return ResourceAction == EKalmalaToolAction::Gathering;
    default:
        return false;
    }
}

bool FKalmalaToolLifecycleContract::IsToolSuitableForSelection(
    const FKalmalaToolDefinition& Definition,
    const FKalmalaToolServerSelection& Selection)
{
    return IsToolSuitableForSelection(Definition, 1, Selection);
}

bool FKalmalaToolLifecycleContract::IsToolSuitableForSelection(
    const FKalmalaToolDefinition& Definition,
    const int32 ToolLevel,
    const FKalmalaToolServerSelection& Selection)
{
    if (GetMaxDurability(Definition.ToolId, ToolLevel) <= 0) return false;
    if (Definition.Action != Selection.Action
        || !IsResourceFamilyCompatible(Definition.Kind, Selection.Action)) return false;
    if (Selection.MinimumToolTier == EKalmalaToolTier::None)
    {
        return Definition.Kind == Selection.RequiredTool;
    }
    return IsKnownToolTier(Selection.MinimumToolTier)
        && IsKnownToolTier(GetToolTier(Definition.Kind, ToolLevel))
        && GetToolTier(Definition.Kind, ToolLevel) >= Selection.MinimumToolTier;
}

const FKalmalaToolDefinition* FKalmalaToolLifecycleContract::FindMinimumQualifiedTool(
    const FKalmalaToolServerSelection& Selection)
{
    if (!IsSelectionShapeValid(Selection)) return nullptr;
    const FKalmalaToolDefinition* Best = nullptr;
    const auto ConsiderDefinition = [&Selection, &Best](const FKalmalaToolDefinition& Definition)
    {
        if (!IsToolSuitableForSelection(Definition, Selection)) return;
        const EKalmalaToolTier Tier = GetToolTier(Definition.Kind);
        if (Best == nullptr || static_cast<uint8>(Tier) < static_cast<uint8>(GetToolTier(Best->Kind))) Best = &Definition;
    };
    for (const FKalmalaToolDefinition& Definition : GetDefinitions()) ConsiderDefinition(Definition);
    for (const FKalmalaToolDefinition& Definition : GetTieredToolDefinitions()) ConsiderDefinition(Definition);
    return Best;
}

bool FKalmalaToolLifecycleContract::IsUseAllowed(
    const FKalmalaToolServerContext& Context,
    const FName ClientToolId,
    const EKalmalaToolAction ClientAction,
    const FKalmalaToolState& ToolState,
    const FKalmalaSkillState& SkillState,
    const FKalmalaToolServerSelection& ServerSelection)
{
    if (!Context.bServerAuthority || !Context.bTraceHit || !Context.bSameWorld || !Context.bNodeAvailable
        || !FMath::IsFinite(Context.TraceDistance) || !FMath::IsFinite(Context.MaximumRange)
        || Context.TraceDistance < 0.0f || Context.MaximumRange <= 0.0f
        || Context.TraceDistance > Context.MaximumRange
        || !IsSelectionShapeValid(ServerSelection)) return false;

    const FKalmalaToolDefinition* Definition = FindDefinition(ClientToolId);
    if (Definition == nullptr || !IsKnownAction(ClientAction)
        || Definition->Action != ClientAction
        || !IsToolSuitableForSelection(*Definition, ToolState.ToolLevel, ServerSelection)
        || Definition->RequiredSkill != ServerSelection.RequiredSkill
        || CanonicalizeToolId(ToolState.ToolId) != CanonicalizeToolId(ClientToolId)
        || ToolState.Durability <= 0 || ToolState.Durability > GetMaxDurability(ToolState.ToolId, ToolState.ToolLevel)
        || !SkillState.IsValid() || SkillState.Skill != ServerSelection.RequiredSkill
        || SkillState.Level < Definition->MinimumSkillLevel) return false;

    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    return Catalogue != nullptr && Catalogue->IsValidStack(ServerSelection.RewardItemId, ServerSelection.RewardQuantity);
}

bool FKalmalaToolLifecycleContract::ApplyServerUse(
    FKalmalaToolState& ToolState,
    const FKalmalaToolServerContext& Context,
    const FName ClientToolId,
    const EKalmalaToolAction ClientAction,
    const FKalmalaSkillState& SkillState,
    const FKalmalaToolServerSelection& ServerSelection,
    FName& OutRewardItemId,
    int32& OutRewardQuantity)
{
    OutRewardItemId = NAME_None;
    OutRewardQuantity = 0;
    const FKalmalaToolDefinition* Definition = FindDefinition(ClientToolId);
    if (!IsUseAllowed(Context, ClientToolId, ClientAction, ToolState, SkillState, ServerSelection)
        || Definition == nullptr || ToolState.Durability < DurabilityPerUse) return false;

    ToolState.Durability -= DurabilityPerUse;
    ToolState.ToolId = Definition->ToolId;
    OutRewardItemId = ServerSelection.RewardItemId;
    OutRewardQuantity = ServerSelection.RewardQuantity;
    return true;
}

bool FKalmalaToolLifecycleContract::BuildServerFreeRepair(
    const bool bServerAuthority,
    const bool bAtValidatedRepairStation,
    const FKalmalaToolState& ToolState,
    FKalmalaToolState& OutRepairedState)
{
    OutRepairedState = ToolState;
    const int32 MaxDurability = GetMaxDurability(ToolState.ToolId, ToolState.ToolLevel);
    if (!bServerAuthority || !bAtValidatedRepairStation || MaxDurability <= 0
        || ToolState.Durability < 0 || ToolState.Durability >= MaxDurability) return false;

    OutRepairedState.Durability = MaxDurability;
    return true;
}

bool FKalmalaToolLifecycleContract::BuildServerRepairAll(
    const bool bServerAuthority,
    const bool bAtValidatedRepairStation,
    const TArray<FKalmalaToolState>& ToolStates,
    TArray<FKalmalaToolState>& OutRepairedStates,
    int32& OutRepairedCount)
{
    OutRepairedStates = ToolStates;
    OutRepairedCount = 0;
    if (!bServerAuthority || !bAtValidatedRepairStation || ToolStates.Num() > MaxCarriedToolRecords) return false;

    TSet<FName> SeenToolIds;
    for (int32 Index = 0; Index < ToolStates.Num(); ++Index)
    {
        const FKalmalaToolState& ToolState = ToolStates[Index];
        const FKalmalaToolDefinition* Definition = FindDefinition(ToolState.ToolId);
        const int32 MaxDurability = GetMaxDurability(ToolState.ToolId, ToolState.ToolLevel);
        if (!Definition || ToolState.ToolId.IsNone() || SeenToolIds.Contains(ToolState.ToolId)
            || MaxDurability <= 0 || ToolState.Durability < 0 || ToolState.Durability > MaxDurability)
        {
            OutRepairedStates = ToolStates;
            OutRepairedCount = 0;
            return false;
        }
        SeenToolIds.Add(ToolState.ToolId);
        if (ToolState.Durability == MaxDurability) continue;

        FKalmalaToolState RepairedState;
        if (!BuildServerFreeRepair(true, true, ToolState, RepairedState))
        {
            OutRepairedStates = ToolStates;
            OutRepairedCount = 0;
            return false;
        }
        OutRepairedStates[Index] = RepairedState;
        ++OutRepairedCount;
    }
    return true;
}
