#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaHomeWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaHomeWidgetTest, "Kalmala.UI.Home.LayoutAndActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHomeWidgetTest::RunTest(const FString& Parameters)
{
    const EKalmalaHomeAction Expected[] = { EKalmalaHomeAction::Play, EKalmalaHomeAction::Options,
        EKalmalaHomeAction::SaveInformation, EKalmalaHomeAction::Credits, EKalmalaHomeAction::Exit };
    const TCHAR* Labels[] = { TEXT("Play"), TEXT("Options"), TEXT("Save information"), TEXT("Credits"), TEXT("Exit") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index)
    {
        EKalmalaHomeAction Action = EKalmalaHomeAction::Exit;
        TestTrue(TEXT("Each action is in the planned order"), UKalmalaHomeWidget::TryGetAction(Index, Action)
            && Action == Expected[Index] && FString(UKalmalaHomeWidget::GetActionLabel(Action)) == Labels[Index]);
    }
    EKalmalaHomeAction Action = EKalmalaHomeAction::Play;
    TestFalse(TEXT("Out-of-range action is rejected"), UKalmalaHomeWidget::TryGetAction(5, Action));
    const float Widths[] = {1280.0f, 1920.0f};
    for (const float Width : Widths)
    {
        const float ColumnWidth = UKalmalaHomeWidget::ActionColumnWidth(Width);
        TestTrue(TEXT("Action column respects its 420 width and 32-unit margins"),
            ColumnWidth == 420.0f && ColumnWidth + 64.0f <= Width);
    }
    TestEqual(TEXT("Small viewports bound the action column"), UKalmalaHomeWidget::ActionColumnWidth(300.0f), 236.0f);
    return true;
}
#endif
