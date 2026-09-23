#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWeatherActivityWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "KalmalaWeatherState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWeatherActivityWidgetTest, "Kalmala.UI.WeatherActivity.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWeatherActivityWidgetTest::RunTest(const FString& Parameters)
{
    const FString Calm = UKalmalaWeatherActivityWidget::BuildActivityLabel(EKalmalaWeatherActivityLevel::Calm);
    const FString Active = UKalmalaWeatherActivityWidget::BuildActivityLabel(EKalmalaWeatherActivityLevel::Active);
    const FString HighlyActive = UKalmalaWeatherActivityWidget::BuildActivityLabel(EKalmalaWeatherActivityLevel::HighlyActive);
    TestEqual(TEXT("Calm uses a labelled circle marker"), Calm, FString(TEXT("○ CALM")));
    TestEqual(TEXT("Active uses a labelled diamond marker"), Active, FString(TEXT("◇ ACTIVE")));
    TestEqual(TEXT("Highly Active uses a labelled triangle marker"), HighlyActive, FString(TEXT("▲ HIGHLY ACTIVE")));
    TSet<FString> Labels;
    Labels.Add(Calm);
    Labels.Add(Active);
    Labels.Add(HighlyActive);
    TestEqual(TEXT("Each tier has a distinct explicit text and shape marker"), Labels.Num(), 3);
    TestEqual(TEXT("An unknown replicated enum fails visibly"),
        UKalmalaWeatherActivityWidget::BuildActivityLabel(static_cast<EKalmalaWeatherActivityLevel>(255)),
        FString(TEXT("? WEATHER UNKNOWN")));

    UKalmalaWeatherActivityWidget* Widget = NewObject<UKalmalaWeatherActivityWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestTrue(TEXT("Weather activity stays anchored at the top-right"), Slot.Anchors == FAnchors(1.0f, 0.0f));
    TestEqual(TEXT("Weather activity aligns to the minimap's right edge"), Slot.Alignment, FVector2D(1.0f, 0.0f));
    TestEqual(TEXT("Weather activity sits 12 UI units below the minimap"), Slot.Offsets.Top, 244.0f);
    TestEqual(TEXT("Weather activity keeps a compact fixed width"), Slot.Offsets.Right, 252.0f);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
