#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaHarvestNode.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaTreeFellingTest, "Kalmala.Gameplay.Harvest.TreeFelling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaTreeFellingTest::RunTest(const FString& Parameters)
{
    uint8 Stage;
    float Health;
    TestTrue(TEXT("Partial axe hit damages the standing tree"), AKalmalaHarvestNode::AdvanceTreeHit(0, 100, 15, Stage, Health));
    TestEqual(TEXT("Standing stage remains"), Stage, uint8(0));
    TestEqual(TEXT("Damage accumulates"), Health, 85.0f);
    TestTrue(TEXT("Large hit fells the tree"), AKalmalaHarvestNode::AdvanceTreeHit(0, 85, 1000, Stage, Health));
    TestEqual(TEXT("Overflow enters the fall transition"), Stage, uint8(3));
    TestEqual(TEXT("Trunk has fresh health"), Health, 60.0f);
    TestFalse(TEXT("An overlapping hit during the fall is rejected"),
        AKalmalaHarvestNode::AdvanceTreeHit(Stage, Health, 1000, Stage, Health));
    TestEqual(TEXT("Overlap leaves the fall phase intact"), Stage, uint8(3));
    TestEqual(TEXT("Overlap leaves the fresh trunk health intact"), Health, 60.0f);
    TestEqual(TEXT("Late relevance reconstructs half the elapsed fall"),
        AKalmalaHarvestNode::SampleTreeFallProgress(100.0, 100.0 + AKalmalaHarvestNode::TreeFallSeconds * 0.5), 0.5f);
    TestEqual(TEXT("A peer clock before the start clamps to zero"),
        AKalmalaHarvestNode::SampleTreeFallProgress(100, 99), 0.0f);
    TestEqual(TEXT("An elapsed fall clamps to the settled pose"),
        AKalmalaHarvestNode::SampleTreeFallProgress(100, 105), 1.0f);
    TestEqual(TEXT("Invalid clock input is rejected"),
        AKalmalaHarvestNode::SampleTreeFallProgress(100, std::numeric_limits<double>::infinity()), 0.0f);
    Stage = 1; // Only the authoritative timer may settle the actor.
    TestTrue(TEXT("Trunk hit creates dropped wood"), AKalmalaHarvestNode::AdvanceTreeHit(Stage, Health, 60, Stage, Health));
    TestEqual(TEXT("Wood-drop stage"), Stage, uint8(2));
    TestFalse(TEXT("Dropped wood cannot receive further damage"), AKalmalaHarvestNode::AdvanceTreeHit(Stage, Health, 10, Stage, Health));
    TestFalse(TEXT("Zero damage cannot spend a hit"), AKalmalaHarvestNode::AdvanceTreeHit(0, 100, 0, Stage, Health));
    TestFalse(TEXT("Negative damage rejected"), AKalmalaHarvestNode::AdvanceTreeHit(0, 100, -1, Stage, Health));
    TestFalse(TEXT("Nonfinite damage rejected"), AKalmalaHarvestNode::AdvanceTreeHit(0, 100, std::numeric_limits<float>::infinity(), Stage, Health));
    TestFalse(TEXT("Nonfinite health rejected"), AKalmalaHarvestNode::AdvanceTreeHit(0, std::numeric_limits<float>::quiet_NaN(), 10, Stage, Health));
    return true;
}
#endif
