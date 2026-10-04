#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCatalogueRowsWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaToolPresentationTest,
    "Kalmala.UI.Inventory.ToolPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaToolPresentationTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Intact tool has separate level, condition and ready lines"),
        UKalmalaCatalogueRowsWidget::BuildToolDetail(2, 40, 40), FString(TEXT("Level 2\nCondition 40/40\nREADY")));
    TestEqual(TEXT("Partial condition is explicitly damaged"),
        UKalmalaCatalogueRowsWidget::BuildToolDetail(1, 17, 40), FString(TEXT("Level 1\nCondition 17/40\nDAMAGED")));
    TestEqual(TEXT("Zero condition is explicitly broken, not ready"),
        UKalmalaCatalogueRowsWidget::BuildToolDetail(1, 0, 40), FString(TEXT("Level 1\nCondition 0/40\nBROKEN")));
    for (const auto Values : { FIntVector(0, 40, 40), FIntVector(1, -1, 40), FIntVector(1, 41, 40), FIntVector(1, 0, 0) })
        TestEqual(TEXT("Missing or invalid state does not fabricate a usable tool"),
            UKalmalaCatalogueRowsWidget::BuildToolDetail(Values.X, Values.Y, Values.Z), FString(TEXT("Tool state unavailable")));
    return true;
}
#endif
