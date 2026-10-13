#include "KalmalaToolLifecycleContract.h"

#include "KalmalaBiomeContentContract.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM9SourceLootContract.h"

namespace
{
    const TArray<FKalmalaToolDefinition>& ToolDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions =
        {
            { TEXT("ReedKnife"), EKalmalaToolKind::ReedKnife, EKalmalaToolAction::Gathering, EKalmalaSkill::Gathering, 1, 16, 1,
                EKalmalaWeaponDamageType::Slash, 30.0f },
            { TEXT("FieldHatchet"), EKalmalaToolKind::FieldHatchet, EKalmalaToolAction::Woodcutting, EKalmalaSkill::Woodcutting, 1, 24, 1,
                EKalmalaWeaponDamageType::Slash, 34.0f },
            { TEXT("StonePick"), EKalmalaToolKind::StonePick, EKalmalaToolAction::Mining, EKalmalaSkill::Mining, 1, 20, 1,
                EKalmalaWeaponDamageType::Blunt, 26.0f },
            { TEXT("StoneSpear"), EKalmalaToolKind::StoneSpear, EKalmalaToolAction::None, EKalmalaSkill::None, 1, 36, 1,
                EKalmalaWeaponDamageType::Pierce, 42.0f },
        };
        return Definitions;
    }

    const TArray<FKalmalaToolDefinition>& TieredToolDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions = []
        {
            FKalmalaToolDefinition Bronze;
            Bronze.ToolId = TEXT("BronzeAxe");
            Bronze.Kind = EKalmalaToolKind::BronzeAxe;
            Bronze.Action = EKalmalaToolAction::Woodcutting;
            Bronze.RequiredSkill = EKalmalaSkill::Woodcutting;
            Bronze.MinimumSkillLevel = 1;
            Bronze.MaxDurability = 100;
            Bronze.DurabilityCost = 1;
            Bronze.DamageType = EKalmalaWeaponDamageType::Slash;
            Bronze.CombatDamage = 50.0f;

            FKalmalaToolDefinition BronzeKnife;
            BronzeKnife.ToolId = TEXT("BronzeKnife");
            BronzeKnife.Kind = EKalmalaToolKind::BronzeKnife;
            BronzeKnife.Action = EKalmalaToolAction::Gathering;
            BronzeKnife.RequiredSkill = EKalmalaSkill::Gathering;
            BronzeKnife.MinimumSkillLevel = 1;
            BronzeKnife.MaxDurability = 48;
            BronzeKnife.DurabilityCost = 1;
            BronzeKnife.DamageType = EKalmalaWeaponDamageType::Slash;
            BronzeKnife.CombatDamage = 42.0f;

            FKalmalaToolDefinition BronzePick;
            BronzePick.ToolId = TEXT("BronzePick");
            BronzePick.Kind = EKalmalaToolKind::BronzePick;
            BronzePick.Action = EKalmalaToolAction::Mining;
            BronzePick.RequiredSkill = EKalmalaSkill::Mining;
            BronzePick.MinimumSkillLevel = 1;
            BronzePick.MaxDurability = 90;
            BronzePick.DurabilityCost = 1;
            BronzePick.DamageType = EKalmalaWeaponDamageType::Blunt;
            BronzePick.CombatDamage = 38.0f;

            FKalmalaToolDefinition BronzeSpear;
            BronzeSpear.ToolId = TEXT("BronzeSpear");
            BronzeSpear.Kind = EKalmalaToolKind::BronzeSpear;
            BronzeSpear.Action = EKalmalaToolAction::None;
            BronzeSpear.RequiredSkill = EKalmalaSkill::None;
            BronzeSpear.MinimumSkillLevel = 1;
            BronzeSpear.MaxDurability = 90;
            BronzeSpear.DurabilityCost = 1;
            BronzeSpear.DamageType = EKalmalaWeaponDamageType::Pierce;
            BronzeSpear.CombatDamage = 58.0f;

            FKalmalaToolDefinition Iron;
            Iron.ToolId = TEXT("IronAxe");
            Iron.Kind = EKalmalaToolKind::IronAxe;
            Iron.Action = EKalmalaToolAction::Woodcutting;
            Iron.RequiredSkill = EKalmalaSkill::Woodcutting;
            Iron.MinimumSkillLevel = 1;
            Iron.MaxDurability = 24;
            Iron.DurabilityCost = 1;
            return TArray<FKalmalaToolDefinition>{Bronze, BronzeKnife, BronzePick, BronzeSpear, Iron};
        }();
        return Definitions;
    }

    const FKalmalaToolDefinition& ConstructionHammerDefinition()
    {
        static const FKalmalaToolDefinition Definition = []
        {
            FKalmalaToolDefinition Hammer;
            Hammer.ToolId = TEXT("ConstructionHammer");
            Hammer.Kind = EKalmalaToolKind::ConstructionHammer;
            Hammer.Action = EKalmalaToolAction::None;
            Hammer.RequiredSkill = EKalmalaSkill::None;
            Hammer.MinimumSkillLevel = 1;
            Hammer.MaxDurability = 100;
            Hammer.DurabilityCost = 0;
            return Hammer;
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

const FKalmalaToolDefinition* FKalmalaToolLifecycleContract::FindDefinition(const FName ToolId)
{
    const FKalmalaToolDefinition* Definition = ToolDefinitions().FindByPredicate([ToolId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == ToolId;
    });
    if (Definition != nullptr) return Definition;
    Definition = TieredToolDefinitions().FindByPredicate([ToolId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == ToolId;
    });
    if (Definition != nullptr) return Definition;
    return ToolId == ConstructionHammerDefinition().ToolId ? &ConstructionHammerDefinition() : nullptr;
}

int32 FKalmalaToolLifecycleContract::GetMaxDurability(const FName ToolId, const int32 ToolLevel)
{
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    if (Definition == nullptr || ToolLevel < 1) return 0;
    if (ToolId == TEXT("BronzeAxe"))
    {
        if (ToolLevel == 1) return 100;
        if (ToolLevel == 2) return 150;
        return 0;
    }
    // Older saves may contain an Iron Axe at level 2 from the superseded cross-item upgrade.
    if (ToolId == TEXT("IronAxe")) return ToolLevel <= 2 ? Definition->MaxDurability : 0;
    return ToolLevel == 1 ? Definition->MaxDurability : 0;
}

float FKalmalaToolLifecycleContract::GetSlashDamage(const FName ToolId, const int32 ToolLevel)
{
    if (GetWeaponDamageType(ToolId) != EKalmalaWeaponDamageType::Slash) return 0.0f;
    return GetWeaponDamage(ToolId, ToolLevel);
}

float FKalmalaToolLifecycleContract::GetWeaponDamage(const FName ToolId, const int32 ToolLevel)
{
    if (ToolId.IsNone() || GetMaxDurability(ToolId, ToolLevel) <= 0) return 0.0f;
    if (ToolId == TEXT("BronzeAxe") && ToolLevel == 2) return 55.0f;
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    return Definition ? Definition->CombatDamage : 0.0f;
}

EKalmalaWeaponDamageType FKalmalaToolLifecycleContract::GetWeaponDamageType(const FName ToolId)
{
    const FKalmalaToolDefinition* Definition = FindDefinition(ToolId);
    return Definition ? Definition->DamageType : EKalmalaWeaponDamageType::None;
}

const TCHAR* FKalmalaToolLifecycleContract::GetWeaponDamageTypeLabel(
    const EKalmalaWeaponDamageType DamageType)
{
    switch (DamageType)
    {
    case EKalmalaWeaponDamageType::Slash: return TEXT("Slash");
    case EKalmalaWeaponDamageType::Pierce: return TEXT("Pierce");
    case EKalmalaWeaponDamageType::Blunt: return TEXT("Blunt");
    default: return TEXT("Damage");
    }
}

bool FKalmalaToolLifecycleContract::IsKnownAction(const EKalmalaToolAction Action)
{
    return Action >= EKalmalaToolAction::Gathering && Action <= EKalmalaToolAction::Mining;
}

bool FKalmalaToolLifecycleContract::IsKnownTool(const EKalmalaToolKind Tool)
{
    return Tool >= EKalmalaToolKind::ReedKnife && Tool <= EKalmalaToolKind::ConstructionHammer;
}

bool FKalmalaToolLifecycleContract::IsKnownToolTier(const EKalmalaToolTier Tier)
{
    return Tier >= EKalmalaToolTier::Stone && Tier <= EKalmalaToolTier::Iron;
}

EKalmalaToolTier FKalmalaToolLifecycleContract::GetToolTier(const EKalmalaToolKind Tool)
{
    switch (Tool)
    {
    case EKalmalaToolKind::ReedKnife:
    case EKalmalaToolKind::FieldHatchet:
    case EKalmalaToolKind::StonePick:
    case EKalmalaToolKind::StoneSpear: return EKalmalaToolTier::Stone;
    case EKalmalaToolKind::BronzeAxe:
    case EKalmalaToolKind::BronzeKnife:
    case EKalmalaToolKind::BronzePick:
    case EKalmalaToolKind::BronzeSpear: return EKalmalaToolTier::Bronze;
    case EKalmalaToolKind::IronAxe: return EKalmalaToolTier::Iron;
    default: return EKalmalaToolTier::None;
    }
}

EKalmalaToolTier FKalmalaToolLifecycleContract::GetToolTier(
    const EKalmalaToolKind Tool, const int32)
{
    // A tool's harvesting tier comes from its item identity; progression level only affects item stats.
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
    else if (ServerSourceId == TEXT("mire-bog-iron") || ServerSourceId == TEXT("mountains-slate-vein"))
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
    const bool bTieredAxe = Definition.Kind == EKalmalaToolKind::BronzeAxe
        || Definition.Kind == EKalmalaToolKind::IronAxe;
    if (ToolLevel < 1 || (bTieredAxe ? ToolLevel > 2 : ToolLevel != 1)) return false;
    if (Definition.Action != Selection.Action) return false;
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
        || ToolState.ToolId != ClientToolId
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
        || Definition == nullptr || ToolState.Durability < Definition->DurabilityCost) return false;

    ToolState.Durability -= Definition->DurabilityCost;
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
