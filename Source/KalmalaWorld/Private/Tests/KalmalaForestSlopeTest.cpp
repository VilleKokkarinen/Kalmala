#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaForestLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestSlopeGateTest,
    "Kalmala.World.ForestLayout.MeadowsSlopeGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestSlopeGateTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Flat surface accepts Meadow trees"),
        FKalmalaForestLayout::IsMeadowSurfaceSlopeSuitable(1.0f));
    TestTrue(TEXT("The existing gentle-slope boundary is inclusive"),
        FKalmalaForestLayout::IsMeadowSurfaceSlopeSuitable(0.88f));
    TestFalse(TEXT("Steeper final surfaces reject Meadow trees"),
        FKalmalaForestLayout::IsMeadowSurfaceSlopeSuitable(0.879f));
    return true;
}

#endif
