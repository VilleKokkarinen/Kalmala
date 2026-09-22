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

    bool IsSelectionShapeValid(const FKalmalaToolServerSelection& Selection)
    {
        return !Selection.SourceId.IsNone()
            && FKalmalaToolLifecycleContract::IsKnownAction(Selection.Action)
            && FKalmalaToolLifecycleContract::IsKnownTool(Selection.RequiredTool)
            && FKalmalaSkillProgressionContract::IsKnownSkill(Selection.RequiredSkill)
            && !Selection.RewardItemId.IsNone()
            && Selection.RewardQuantity == 1;
    }
}

const TArray<FKalmalaToolDefinition>& FKalmalaToolLifecycleContract::GetDefinitions()
{
    return ToolDefinitions();
}

const FKalmalaToolDefinition* FKalmalaToolLifecycleContract::FindDefinition(const FName ToolId)
{
    return ToolDefinitions().FindByPredicate([ToolId](const FKalmalaToolDefinition& Definition)
    {
        return Definition.ToolId == ToolId;
    });
}

bool FKalmalaToolLifecycleContract::IsKnownAction(const EKalmalaToolAction Action)
{
    return Action >= EKalmalaToolAction::Gathering && Action <= EKalmalaToolAction::Mining;
}

bool FKalmalaToolLifecycleContract::IsKnownTool(const EKalmalaToolKind Tool)
{
    return Tool >= EKalmalaToolKind::ReedKnife && Tool <= EKalmalaToolKind::StonePick;
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

    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    return IsSelectionShapeValid(OutSelection)
        && Catalogue != nullptr
        && Catalogue->IsValidStack(OutSelection.RewardItemId, OutSelection.RewardQuantity);
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
        || Definition->Action != ServerSelection.Action
        || Definition->Kind != ServerSelection.RequiredTool
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
