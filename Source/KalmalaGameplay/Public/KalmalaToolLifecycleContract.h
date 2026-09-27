#pragma once

#include "CoreMinimal.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaToolLifecycleContract.generated.h"

/** Client-selectable action categories; the server still validates the source and outcome. */
enum class EKalmalaToolAction : uint8
{
    None,
    Gathering,
    Woodcutting,
    Mining
};

/** Tool kinds; these are not pack-item or saved-data IDs. */
enum class EKalmalaToolKind : uint8
{
    None,
    ReedKnife,
    FieldHatchet,
    StonePick,
    BronzeAxe,
    IronAxe
};

/** M9 harvesting tiers; station level is validated only by later craft/upgrade transactions. */
enum class EKalmalaToolTier : uint8
{
    None,
    Bronze,
    Iron
};

struct KALMALAGAMEPLAY_API FKalmalaToolDefinition
{
    FName ToolId = NAME_None;
    EKalmalaToolKind Kind = EKalmalaToolKind::None;
    EKalmalaToolAction Action = EKalmalaToolAction::None;
    EKalmalaSkill RequiredSkill = EKalmalaSkill::None;
    int32 MinimumSkillLevel = 1;
    int32 MaxDurability = 0;
    int32 DurabilityCost = 1;
};

USTRUCT(BlueprintType)
struct KALMALAGAMEPLAY_API FKalmalaToolState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools")
    FName ToolId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools")
    int32 Durability = 0;

    /** Authored tool progression, independent of the owner's skill ledger. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tools", meta = (ClampMin = "1"))
    int32 ToolLevel = 1;
};

/** Server-derived trace and node facts; clients cannot author these values. */
struct KALMALAGAMEPLAY_API FKalmalaToolServerContext
{
    bool bServerAuthority = false;
    bool bTraceHit = false;
    bool bSameWorld = false;
    bool bNodeAvailable = false;
    float TraceDistance = -1.0f;
    float MaximumRange = 0.0f;
};

/** Server-selected source/action/skill/reward tuple for one validated node. */
struct KALMALAGAMEPLAY_API FKalmalaToolServerSelection
{
    FName SourceId = NAME_None;
    EKalmalaToolAction Action = EKalmalaToolAction::None;
    EKalmalaToolKind RequiredTool = EKalmalaToolKind::None;
    EKalmalaSkill RequiredSkill = EKalmalaSkill::None;
    FName RewardItemId = NAME_None;
    int32 RewardQuantity = 0;
    EKalmalaToolTier MinimumToolTier = EKalmalaToolTier::None;
};

/**
 * Bounded server-side rules for the first tool-gathering slice. It validates
 * server-derived node facts and client-selected tool/action intent, then
 * returns the fixed catalogue reward without accepting a client outcome.
 */
class KALMALAGAMEPLAY_API FKalmalaToolLifecycleContract
{
public:
    static constexpr int32 MaxFirstWaveToolDefinitions = 3;
    static constexpr int32 MaxCarriedToolRecords = 5;
    static constexpr float DefaultMaximumRange = 250.0f;

    static const TArray<FKalmalaToolDefinition>& GetDefinitions();
    static const TArray<FKalmalaToolDefinition>& GetTieredAxeDefinitions();
    static TArray<FKalmalaToolState> BuildInitialCarriedTools();
    static const FKalmalaToolDefinition* FindDefinition(FName ToolId);
    static bool IsKnownAction(EKalmalaToolAction Action);
    static bool IsKnownTool(EKalmalaToolKind Tool);
    static bool IsKnownToolTier(EKalmalaToolTier Tier);
    static EKalmalaToolTier GetToolTier(EKalmalaToolKind Tool);
    static bool BuildServerSelection(FName ServerSourceId, FKalmalaToolServerSelection& OutSelection);
    static bool IsToolSuitableForSelection(const FKalmalaToolDefinition& Definition, const FKalmalaToolServerSelection& Selection);
    static const FKalmalaToolDefinition* FindMinimumQualifiedTool(const FKalmalaToolServerSelection& Selection);
    static bool IsUseAllowed(
        const FKalmalaToolServerContext& Context,
        FName ClientToolId,
        EKalmalaToolAction ClientAction,
        const FKalmalaToolState& ToolState,
        const FKalmalaSkillState& SkillState,
        const FKalmalaToolServerSelection& ServerSelection);
    static bool ApplyServerUse(
        FKalmalaToolState& ToolState,
        const FKalmalaToolServerContext& Context,
        FName ClientToolId,
        EKalmalaToolAction ClientAction,
        const FKalmalaSkillState& SkillState,
        const FKalmalaToolServerSelection& ServerSelection,
        FName& OutRewardItemId,
        int32& OutRewardQuantity);
    /** Build a no-cost full-repair outcome from authoritative current condition. */
    static bool BuildServerFreeRepair(
        bool bServerAuthority,
        bool bAtValidatedRepairStation,
        const FKalmalaToolState& ToolState,
        FKalmalaToolState& OutRepairedState);
};
