#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWorldMapWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldMapWidgetTest, "Kalmala.UI.WorldMap.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldMapWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Expanded-map zoom clamps at its minimum"), UKalmalaWorldMapWidget::ClampMapZoom(100.0f, 2500.0f, 50000.0f), 2500.0f);
    TestEqual(TEXT("Expanded-map zoom clamps at its maximum"), UKalmalaWorldMapWidget::ClampMapZoom(100000.0f, 2500.0f, 50000.0f), 50000.0f);
    TestEqual(TEXT("Expanded-map zoom preserves valid values"), UKalmalaWorldMapWidget::ClampMapZoom(18000.0f, 2500.0f, 50000.0f), 18000.0f);
    UKalmalaWorldMapWidget* Widget = NewObject<UKalmalaWorldMapWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestEqual(TEXT("Expanded-map viewport slot stretches horizontally"), Slot.Anchors.Minimum.X, 0.0);
    TestEqual(TEXT("Expanded-map viewport slot stretches vertically"), Slot.Anchors.Maximum.Y, 1.0);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
