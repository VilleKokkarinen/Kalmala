#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaItemCatalogue.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaToolLifecycleContractTest,
    "Kalmala.Gameplay.Tools.LifecycleContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaToolLifecycleContractTest::RunTest(const FString& Parameters)
{
    const TArray<FName> Sources =
    {
        TEXT("meadows-birch-bark"), TEXT("lakes-reed-cluster"), TEXT("elderwood-resinwood"),
        TEXT("mire-bog-iron"), TEXT("tundra-frostmoss"), TEXT("mountains-slate-vein")
    };
    TSet<FName> Rewards;
    for (const FName Source : Sources)
    {
        FKalmalaToolServerSelection Selection;
        TestTrue(TEXT("Catalogue source produces a server selection"), FKalmalaToolLifecycleContract::BuildServerSelection(Source, Selection));
        TestTrue(TEXT("Server selection reward is catalogue-valid"), GetDefault<UKalmalaItemCatalogue>()->IsValidStack(Selection.RewardItemId, Selection.RewardQuantity));
        Rewards.Add(Selection.RewardItemId);
    }
    TestEqual(TEXT("First-wave selection covers the three existing material rewards"), Rewards.Num(), 3);

    FKalmalaToolServerSelection Selection;
    TestFalse(TEXT("Forged source fails closed"), FKalmalaToolLifecycleContract::BuildServerSelection(TEXT("forged-source"), Selection));

    FKalmalaSkillProgressionLedger Ledger;
    Ledger.Initialize();
    const FKalmalaSkillState* Skill = Ledger.Find(EKalmalaSkill::Woodcutting);
    TestNotNull(TEXT("Server ledger exposes the matching skill"), Skill);
    if (Skill == nullptr) return true;

    TestTrue(TEXT("Meadow bark selects a hatchet and wood reward"), FKalmalaToolLifecycleContract::BuildServerSelection(TEXT("meadows-birch-bark"), Selection));
    FKalmalaToolServerContext Context;
    Context.bServerAuthority = true;
    Context.bTraceHit = true;
    Context.bSameWorld = true;
    Context.bNodeAvailable = true;
    Context.TraceDistance = 250.0f;
    Context.MaximumRange = FKalmalaToolLifecycleContract::DefaultMaximumRange;
    FKalmalaToolState ToolState{TEXT("FieldHatchet"), 2};
    const FKalmalaToolState Before = ToolState;
    FName RewardItemId;
    int32 RewardQuantity = 0;
    TestTrue(TEXT("A valid server trace and selected tool are accepted"), FKalmalaToolLifecycleContract::ApplyServerUse(
        ToolState, Context, TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, RewardItemId, RewardQuantity));
    TestEqual(TEXT("Accepted use spends exactly one durability"), ToolState.Durability, Before.Durability - 1);
    TestEqual(TEXT("Accepted use returns only the server-selected reward"), RewardItemId, FName(TEXT("Wood")));
    TestEqual(TEXT("Accepted use returns one catalogue item"), RewardQuantity, 1);

    const auto ExpectRejectedWithoutMutation = [this, &ToolState, &RewardItemId, &RewardQuantity](
        const TCHAR* Label,
        const FKalmalaToolServerContext& AttemptContext,
        const FName RequestedTool,
        const EKalmalaToolAction RequestedAction,
        const FKalmalaSkillState& AttemptSkill,
        const FKalmalaToolServerSelection& AttemptSelection,
        const int32 Durability)
    {
        ToolState = {TEXT("FieldHatchet"), Durability};
        const FKalmalaToolState BeforeAttempt = ToolState;
        RewardItemId = TEXT("StaleOutput");
        RewardQuantity = 99;
        TestFalse(Label, FKalmalaToolLifecycleContract::ApplyServerUse(
            ToolState, AttemptContext, RequestedTool, RequestedAction, AttemptSkill, AttemptSelection,
            RewardItemId, RewardQuantity));
        TestTrue(*FString::Printf(TEXT("%s leaves tool state unchanged"), Label),
            ToolState.ToolId == BeforeAttempt.ToolId && ToolState.Durability == BeforeAttempt.Durability);
        TestTrue(*FString::Printf(TEXT("%s clears reward output"), Label),
            RewardItemId.IsNone() && RewardQuantity == 0);
    };

    auto InvalidContext = Context;
    InvalidContext.bServerAuthority = false;
    ExpectRejectedWithoutMutation(TEXT("Client authority rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    InvalidContext = Context;
    InvalidContext.bTraceHit = false;
    ExpectRejectedWithoutMutation(TEXT("Missing server trace rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    InvalidContext = Context;
    InvalidContext.bSameWorld = false;
    ExpectRejectedWithoutMutation(TEXT("Cross-world source rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    InvalidContext = Context;
    InvalidContext.bNodeAvailable = false;
    ExpectRejectedWithoutMutation(TEXT("Unavailable node rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    InvalidContext = Context;
    InvalidContext.TraceDistance = 251.0f;
    ExpectRejectedWithoutMutation(TEXT("Out-of-range server trace rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    InvalidContext = Context;
    InvalidContext.TraceDistance = std::numeric_limits<float>::quiet_NaN();
    ExpectRejectedWithoutMutation(TEXT("Non-finite trace distance rejects"), InvalidContext,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    ExpectRejectedWithoutMutation(TEXT("Mismatched tool rejects"), Context,
        TEXT("StonePick"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 2);
    ExpectRejectedWithoutMutation(TEXT("Mismatched action rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Mining, *Skill, Selection, 2);

    const FKalmalaSkillState* WrongSkill = Ledger.Find(EKalmalaSkill::Gathering);
    TestNotNull(TEXT("Ledger exposes a different valid skill for rejection coverage"), WrongSkill);
    if (WrongSkill != nullptr)
    {
        ExpectRejectedWithoutMutation(TEXT("Mismatched skill rejects"), Context,
            TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *WrongSkill, Selection, 2);
    }
    const FKalmalaSkillState InvalidSkill;
    ExpectRejectedWithoutMutation(TEXT("Invalid skill rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, InvalidSkill, Selection, 2);
    ExpectRejectedWithoutMutation(TEXT("Zero condition rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 0);
    ExpectRejectedWithoutMutation(TEXT("Condition above tool maximum rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, 25);
    ExpectRejectedWithoutMutation(TEXT("Negative condition rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, Selection, -1);

    FKalmalaToolServerSelection InvalidSelection = Selection;
    InvalidSelection.RewardItemId = TEXT("ForgedReward");
    ExpectRejectedWithoutMutation(TEXT("Non-catalogue reward rejects"), Context,
        TEXT("FieldHatchet"), EKalmalaToolAction::Woodcutting, *Skill, InvalidSelection, 2);
    return true;
}

#endif
