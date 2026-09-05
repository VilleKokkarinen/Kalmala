#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaMinimapViewModel.h"
#include "KalmalaMinimapWidget.h"

#include "KalmalaWorldGenerationConfig.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMinimapViewModelTest, "Kalmala.UI.Minimap.LocalPresentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMinimapViewModelTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;

    const TArray<FKalmalaMinimapTerrainSample> FirstSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D(1250.0f, -750.0f), 5000.0f, 5);
    const TArray<FKalmalaMinimapTerrainSample> RepeatedSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D(1250.0f, -750.0f), 5000.0f, 5);

    TestEqual(TEXT("The local presentation builds the requested terrain grid"), FirstSamples.Num(), 25);
    TestEqual(TEXT("Repeated local presentation derives the same sample count"), RepeatedSamples.Num(), FirstSamples.Num());
    TestTrue(TEXT("The grid contains a centred owning-player map coordinate"), FirstSamples[12].MapPosition.IsNearlyZero());
    for (int32 Index = 0; Index < FirstSamples.Num(); ++Index)
    {
        TestEqual(TEXT("Repeated local terrain heights match"), FirstSamples[Index].TerrainHeight, RepeatedSamples[Index].TerrainHeight);
        TestEqual(TEXT("Repeated local water treatment matches"), FirstSamples[Index].bIsWater, RepeatedSamples[Index].bIsWater);
        TestTrue(TEXT("Map coordinates remain normalized for circular clipping"), FirstSamples[Index].MapPosition.GetAbsMax() <= 1.0f);
    }

    TestEqual(TEXT("Invalid local presentation input produces no samples"), UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D::ZeroVector, 0.0f, 5).Num(), 0);
    TestTrue(TEXT("Circular minimap includes its centred player marker"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D::ZeroVector));
    TestTrue(TEXT("Circular minimap includes positions on its edge"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 0.0f)));
    TestFalse(TEXT("Circular minimap rejects square-grid corners"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 1.0f)));
    TestEqual(TEXT("Minimap zoom clamps at its minimum"), UKalmalaMinimapWidget::ClampZoom(500.0f, 2500.0f, 10000.0f), 2500.0f);
    TestEqual(TEXT("Minimap zoom clamps at its maximum"), UKalmalaMinimapWidget::ClampZoom(15000.0f, 2500.0f, 10000.0f), 10000.0f);
    TestEqual(TEXT("Minimap zoom retains values inside its bounds"), UKalmalaMinimapWidget::ClampZoom(6000.0f, 2500.0f, 10000.0f), 6000.0f);
    TestFalse(TEXT("A modal UI retains mouse-wheel ownership"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(false));
    TestTrue(TEXT("Minimap accepts mouse-wheel input without a modal UI"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(true));
    return true;
}

#endif
