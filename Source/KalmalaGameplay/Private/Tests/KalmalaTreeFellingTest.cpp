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
    TestEqual(TEXT("Overflow never skips the fallen trunk"), Stage, uint8(1));
    TestEqual(TEXT("Trunk has fresh health"), Health, 60.0f);
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
