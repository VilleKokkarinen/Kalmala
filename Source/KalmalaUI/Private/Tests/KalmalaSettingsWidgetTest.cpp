#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSettingsWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSettingsWidgetTest, "Kalmala.UI.Settings.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSettingsWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Render distance quality keeps its lowest supported level"), UKalmalaSettingsWidget::ClampViewDistanceQuality(-1), 0);
    TestEqual(TEXT("Render distance quality preserves supported levels"), UKalmalaSettingsWidget::ClampViewDistanceQuality(2), 2);
    TestEqual(TEXT("Render distance quality clamps to Unreal scalability's highest supported level"), UKalmalaSettingsWidget::ClampViewDistanceQuality(4), 3);
    return true;
}

#endif
