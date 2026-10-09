#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSupportSelectionSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSupportSelectionWidgetTest,
    "Kalmala.UI.SupportSelection.LocalHudCue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSupportSelectionWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("No selection has an explicit text fallback"),
        UKalmalaSupportSelectionWidget::BuildSelectedText(INDEX_NONE, false), FString(TEXT("No support selected")));
    TestEqual(TEXT("Learned selection is named in text"),
        UKalmalaSupportSelectionWidget::BuildSelectedText(0, true), FString(TEXT("Selected: Mending · learned")));
    TestEqual(TEXT("Unlearned selection state stays explicit"),
        UKalmalaSupportSelectionWidget::BuildSelectedText(3, false), FString(TEXT("Selected: Deer Call · not learned")));
    TestEqual(TEXT("Invalid selection cannot invent an effect name"),
        UKalmalaSupportSelectionWidget::BuildSelectedText(99, true), FString(TEXT("No support selected")));
    return true;
}

#endif
