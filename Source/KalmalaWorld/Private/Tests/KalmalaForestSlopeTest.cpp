#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaForestLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestSlopeGateTest,
    "Kalmala.World.ForestLayout.SharedSlopeGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestSlopeGateTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Flat surface accepts forest trees"),
        FKalmalaForestLayout::IsForestSurfaceSlopeSuitable(1.0f));
    TestTrue(TEXT("The shared gentle-slope boundary is inclusive"),
        FKalmalaForestLayout::IsForestSurfaceSlopeSuitable(0.88f));
    TestFalse(TEXT("Steeper final surfaces reject forest trees"),
        FKalmalaForestLayout::IsForestSurfaceSlopeSuitable(0.879f));
    return true;
}

#endif
