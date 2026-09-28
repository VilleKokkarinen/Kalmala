#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeContentContract.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9AxeHarvestGateTest,
    "Kalmala.Gameplay.M9.AxeHarvestGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9AxeHarvestGateTest::RunTest(const FString& Parameters)
{
    struct FExpectedGate
    {
        FName SourceId;
        FName PresentationId;
        FName RewardId;
        EKalmalaToolTier MinimumTier;
        FName MinimumToolId;
    };
    const FExpectedGate Gates[] =
    {
        { TEXT("meadows-birch-trunk"), TEXT("birch-trunk-harvest"), TEXT("Lightwood"), EKalmalaToolTier::Bronze, TEXT("BronzeAxe") },
        { TEXT("elderwood-ironheart-trunk"), TEXT("ironheart-trunk-harvest"), TEXT("Densewood"), EKalmalaToolTier::Iron, TEXT("IronAxe") }
    };

    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    for (const FExpectedGate& Gate : Gates)
    {
        FKalmalaToolServerSelection Selection;
        TestTrue(TEXT("Canonical M9 trunk source has a server selection"),
            FKalmalaToolLifecycleContract::BuildServerSelection(Gate.SourceId, Selection));
        TestTrue(TEXT("Canonical M9 source remains allowlisted"),
            FKalmalaBiomeContentContract::IsValidGatheringSourceId(Gate.SourceId));
        TestEqual(TEXT("Source resolves its presentation identity"),
            FKalmalaBiomeContentContract::GetGatheringPresentationId(Gate.SourceId), Gate.PresentationId);
        TestEqual(TEXT("Source resolves only its canonical reward"), Selection.RewardItemId, Gate.RewardId);
        TestTrue(TEXT("Wood reward is catalogue-valid"), Catalogue->IsValidStack(Gate.RewardId, 1));
        TestTrue(TEXT("Server selection carries its minimum axe tier"), Selection.MinimumToolTier == Gate.MinimumTier);
        const FKalmalaToolDefinition* MinimumTool = FKalmalaToolLifecycleContract::FindMinimumQualifiedTool(Selection);
        TestNotNull(TEXT("Minimum required axe resolves"), MinimumTool);
        if (MinimumTool != nullptr)
        {
            TestEqual(TEXT("Minimum axe identity matches the gate"), MinimumTool->ToolId, Gate.MinimumToolId);
        }
    }

    FKalmalaToolServerSelection LightwoodSelection;
    FKalmalaToolLifecycleContract::BuildServerSelection(Gates[0].SourceId, LightwoodSelection);
    FKalmalaToolServerSelection DensewoodSelection;
    FKalmalaToolLifecycleContract::BuildServerSelection(Gates[1].SourceId, DensewoodSelection);

    FKalmalaSkillProgressionLedger Ledger;
    Ledger.Initialize();
    const FKalmalaSkillState* Woodcutting = Ledger.Find(EKalmalaSkill::Woodcutting);
    TestNotNull(TEXT("Server ledger contains Woodcutting"), Woodcutting);
    if (Woodcutting == nullptr) return true;

    FKalmalaToolServerContext Context;
    Context.bServerAuthority = true;
    Context.bTraceHit = true;
    Context.bSameWorld = true;
    Context.bNodeAvailable = true;
    Context.TraceDistance = 200.0f;
    Context.MaximumRange = FKalmalaToolLifecycleContract::DefaultMaximumRange;

    FName RewardId = NAME_None;
    int32 RewardQuantity = 0;
    FKalmalaToolState ToolState{TEXT("BronzeAxe"), 5};
    TestTrue(TEXT("Bronze Axe accepts Lightwood field harvest without a station check"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("BronzeAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, LightwoodSelection, RewardId, RewardQuantity));
    TestEqual(TEXT("Accepted Lightwood use spends one condition"), ToolState.Durability, 4);
    TestEqual(TEXT("Accepted Lightwood grant stays bounded to one"), RewardQuantity, 1);
    TestEqual(TEXT("Accepted Lightwood grant is server-selected"), RewardId, FName(TEXT("Lightwood")));

    ToolState = {TEXT("BronzeAxe"), 5};
    RewardId = TEXT("StaleReward");
    RewardQuantity = 99;
    TestFalse(TEXT("Bronze Axe rejects the Iron-tier Densewood source"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("BronzeAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, DensewoodSelection, RewardId, RewardQuantity));
    TestTrue(TEXT("Lower-tier rejection preserves condition and clears reward"),
        ToolState.Durability == 5 && RewardId.IsNone() && RewardQuantity == 0);

    ToolState = {TEXT("IronAxe"), 5};
    TestTrue(TEXT("Iron Axe satisfies Bronze-or-better Lightwood gate"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("IronAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, LightwoodSelection, RewardId, RewardQuantity));
    ToolState = {TEXT("IronAxe"), 5};
    TestTrue(TEXT("Iron Axe satisfies Densewood gate"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("IronAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, DensewoodSelection, RewardId, RewardQuantity));

    ToolState = {TEXT("BronzeAxe"), -1};
    RewardId = TEXT("StaleReward");
    RewardQuantity = 99;
    TestFalse(TEXT("Uncrafted Bronze Axe cannot be used in the field"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("BronzeAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, LightwoodSelection, RewardId, RewardQuantity));
    TestTrue(TEXT("Uncrafted axe rejection preserves its sentinel and clears reward"),
        ToolState.Durability == -1 && RewardId.IsNone() && RewardQuantity == 0);

    Context.bServerAuthority = false;
    ToolState = {TEXT("IronAxe"), 5};
    TestFalse(TEXT("Client cannot authorize a Densewood harvest"),
        FKalmalaToolLifecycleContract::ApplyServerUse(ToolState, Context, TEXT("IronAxe"),
            EKalmalaToolAction::Woodcutting, *Woodcutting, DensewoodSelection, RewardId, RewardQuantity));
    TestEqual(TEXT("Rejected client request preserves Iron Axe condition"), ToolState.Durability, 5);
    return true;
}

#endif
