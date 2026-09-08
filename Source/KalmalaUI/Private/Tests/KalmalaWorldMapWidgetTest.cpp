#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWorldMapWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "KalmalaWorldGenerationConfig.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldMapWidgetTest, "Kalmala.UI.WorldMap.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldMapWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Expanded-map zoom clamps at its minimum"), UKalmalaWorldMapWidget::ClampMapZoom(100.0f, 2500.0f, 50000.0f), 2500.0f);
    TestEqual(TEXT("Expanded-map zoom clamps at its maximum"), UKalmalaWorldMapWidget::ClampMapZoom(100000.0f, 2500.0f, 50000.0f), 50000.0f);
    TestEqual(TEXT("Expanded-map zoom preserves valid values"), UKalmalaWorldMapWidget::ClampMapZoom(18000.0f, 2500.0f, 50000.0f), 18000.0f);
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;
    const FIntPoint Dimensions(9, 5);
    const TArray<FKalmalaMinimapTerrainSample> RectangularSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(
        Config, FVector2D(1000.0f, -500.0f), FVector2D(8000.0f, 4500.0f), Dimensions);
    TestEqual(TEXT("Expanded map creates every aspect-aware sample"), RectangularSamples.Num(), Dimensions.X * Dimensions.Y);
    TestTrue(TEXT("Expanded map keeps its centre sample at the requested world centre"), RectangularSamples[22].MapPosition.IsNearlyZero());
    const TArray<FColor> RectangularPixels = FKalmalaMinimapRaster::BuildPixels(RectangularSamples, Dimensions);
    TestEqual(TEXT("Expanded map raster preserves rectangular dimensions"), RectangularPixels.Num(), RectangularSamples.Num());
    TestEqual(TEXT("Expanded map has no circular alpha cutout"), RectangularPixels[0].A, uint8(255));
    UKalmalaWorldMapWidget* Widget = NewObject<UKalmalaWorldMapWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestEqual(TEXT("Expanded-map viewport slot stretches horizontally"), Slot.Anchors.Minimum.X, 0.0);
    TestEqual(TEXT("Expanded-map viewport slot stretches vertically"), Slot.Anchors.Maximum.Y, 1.0);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
