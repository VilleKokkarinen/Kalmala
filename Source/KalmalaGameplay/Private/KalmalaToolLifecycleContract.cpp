#include "KalmalaToolLifecycleContract.h"

#include "KalmalaBiomeContentContract.h"
#include "KalmalaItemCatalogue.h"

namespace
{
    const TArray<FKalmalaToolDefinition>& ToolDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions =
        {
            { TEXT("ReedKnife"), EKalmalaToolKind::ReedKnife, EKalmalaToolAction::Gathering, EKalmalaSkill::Gathering, 1, 16, 1 },
            { TEXT("FieldHatchet"), EKalmalaToolKind::FieldHatchet, EKalmalaToolAction::Woodcutting, EKalmalaSkill::Woodcutting, 1, 24, 1 },
            { TEXT("StonePick"), EKalmalaToolKind::StonePick, EKalmalaToolAction::Mining, EKalmalaSkill::Mining, 1, 20, 1 }
        };
        return Definitions;
    }

    const TArray<FKalmalaToolDefinition>& TieredAxeDefinitions()
    {
        static const TArray<FKalmalaToolDefinition> Definitions = []
        {
            FKalmalaToolDefinition Bronze;
            Bronze.ToolId = TEXT("BronzeAxe");
            Bronze.Kind = EKalmalaToolKind::BronzeAxe;
            Bronze.Action = EKalmalaToolAction::Woodcutting;
            Bronze.RequiredSkill = EKalmalaSkill::Woodcutting;
            Bronze.MinimumSkillLevel = 1;
            Bronze.MaxDurability = 24;
            Bronze.DurabilityCost = 1;

            FKalmalaToolDefinition Iron;
            Iron.ToolId = TEXT("IronAxe");
            Iron.Kind = EKalmalaToolKind::IronAxe;
            Iron.Action = EKalmalaToolAction::Woodcutting;
            Iron.RequiredSkill = EKalmalaSkill::Woodcutting;
            Iron.MinimumSkillLevel = 1;
            Iron.MaxDurability = 24;
            Iron.DurabilityCost = 1;
            return TArray<FKalmalaToolDefinition>{Bronze, Iron};
        }();
        return Definitions;
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

const TArray<FKalmalaToolDefinition>& FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()
{
    return TieredAxeDefinitions();
}

const FKalmalaToolDefinition* FKalmalaToolLifecycleContract::FindDefinition(const FName ToolId)
{
    const FKalmalaToolDefinition* Definition = ToolDefinitions().FindByPredicate([ToolId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == ToolId;
    });
    if (Definition != nullptr) return Definition;
    return TieredAxeDefinitions().FindByPredicate([ToolId](const FKalmalaToolDefinition& Candidate)
    {
        return Candidate.ToolId == ToolId;
    });
}

bool FKalmalaToolLifecycleContract::IsKnownAction(const EKalmalaToolAction Action)
{
    return Action >= EKalmalaToolAction::Gathering && Action <= EKalmalaToolAction::Mining;
}

bool FKalmalaToolLifecycleContract::IsKnownTool(const EKalmalaToolKind Tool)
{
    return Tool >= EKalmalaToolKind::ReedKnife && Tool <= EKalmalaToolKind::IronAxe;
}

bool FKalmalaToolLifecycleContract::IsKnownToolTier(const EKalmalaToolTier Tier)
{
    return Tier >= EKalmalaToolTier::Bronze && Tier <= EKalmalaToolTier::Iron;
}

EKalmalaToolTier FKalmalaToolLifecycleContract::GetToolTier(const EKalmalaToolKind Tool)
{
    switch (Tool)
    {
    case EKalmalaToolKind::BronzeAxe: return EKalmalaToolTier::Bronze;
    case EKalmalaToolKind::IronAxe: return EKalmalaToolTier::Iron;
    default: return EKalmalaToolTier::None;
    }
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
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::FieldHatchet, EKalmalaSkill::Woodcutting, TEXT("Wood"), 1 };
    }
    else if (ServerSourceId == TEXT("lakes-reed-cluster") || ServerSourceId == TEXT("tundra-frostmoss"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Gathering, EKalmalaToolKind::ReedKnife, EKalmalaSkill::Gathering, TEXT("Fibre"), 1 };
    }
    else if (ServerSourceId == TEXT("mire-bog-iron") || ServerSourceId == TEXT("mountains-slate-vein"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Mining, EKalmalaToolKind::StonePick, EKalmalaSkill::Mining, TEXT("Stone"), 1 };
    }
    else if (ServerSourceId == TEXT("meadows-birch-trunk"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaSkill::Woodcutting, TEXT("Lightwood"), 1, EKalmalaToolTier::Bronze };
    }
    else if (ServerSourceId == TEXT("elderwood-ironheart-trunk"))
    {
        OutSelection = { ServerSourceId, EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaSkill::Woodcutting, TEXT("Densewood"), 1, EKalmalaToolTier::Iron };
    }

    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    return IsSelectionShapeValid(OutSelection)
        && Catalogue != nullptr
        && Catalogue->IsValidStack(OutSelection.RewardItemId, OutSelection.RewardQuantity);
}

bool FKalmalaToolLifecycleContract::IsToolSuitableForSelection(
    const FKalmalaToolDefinition& Definition,
    const FKalmalaToolServerSelection& Selection)
{
    if (Definition.Action != Selection.Action) return false;
    if (Selection.MinimumToolTier == EKalmalaToolTier::None)
    {
        return Definition.Kind == Selection.RequiredTool;
    }
    return IsKnownToolTier(Selection.MinimumToolTier)
        && IsKnownToolTier(GetToolTier(Definition.Kind))
        && GetToolTier(Definition.Kind) >= Selection.MinimumToolTier;
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
    for (const FKalmalaToolDefinition& Definition : GetTieredAxeDefinitions()) ConsiderDefinition(Definition);
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
        || !IsToolSuitableForSelection(*Definition, ServerSelection)
        || Definition->RequiredSkill != ServerSelection.RequiredSkill
        || ToolState.ToolId != ClientToolId
        || ToolState.Durability <= 0 || ToolState.Durability > Definition->MaxDurability
        || !SkillState.IsValid() || SkillState.Skill != ServerSelection.RequiredSkill
        || SkillState.Level < Definition->MinimumSkillLevel) return false;

    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
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
