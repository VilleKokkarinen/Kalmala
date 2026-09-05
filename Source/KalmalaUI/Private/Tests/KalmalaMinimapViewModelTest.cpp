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

    const TArray<FKalmalaMinimapTerrainSample> HostSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D(-3000.0f, 1250.0f), 5000.0f, 9);
    const TArray<FKalmalaMinimapTerrainSample> ClientSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D(4200.0f, -1750.0f), 5000.0f, 9);
    TestEqual(TEXT("Host and client local views use the same replicated world identity"), HostSamples.Num(), ClientSamples.Num());
    TestTrue(TEXT("Host view remains centred on its owning player"), HostSamples[40].MapPosition.IsNearlyZero());
    TestTrue(TEXT("Client view remains centred on its owning player"), ClientSamples[40].MapPosition.IsNearlyZero());
    TestNotEqual(TEXT("Separated players derive distinct local surrounding terrain samples"), HostSamples[0].TerrainHeight, ClientSamples[0].TerrainHeight);

    TestEqual(TEXT("Invalid local presentation input produces no samples"), UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D::ZeroVector, 0.0f, 5).Num(), 0);
    TestTrue(TEXT("Circular minimap includes its centred player marker"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D::ZeroVector));
    TestTrue(TEXT("Circular minimap includes positions on its edge"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 0.0f)));
    TestFalse(TEXT("Circular minimap rejects square-grid corners"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 1.0f)));
    TestTrue(TEXT("Circular minimap remains top-right at 16:9 and 100% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(1920.0f, 1080.0f), 208.0f, 24.0f, 1.0f));
    TestTrue(TEXT("Circular minimap remains top-right at ultrawide and 125% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(3440.0f, 1440.0f), 208.0f, 24.0f, 1.25f));
    TestTrue(TEXT("Circular minimap remains top-right at 4:3 and 75% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(1024.0f, 768.0f), 208.0f, 24.0f, 0.75f));
    TestFalse(TEXT("Circular minimap rejects a footprint that cannot fit the viewport"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(200.0f, 100.0f), 208.0f, 24.0f, 1.0f));
    TestEqual(TEXT("Minimap zoom clamps at its minimum"), UKalmalaMinimapWidget::ClampZoom(500.0f, 2500.0f, 10000.0f), 2500.0f);
    TestEqual(TEXT("Minimap zoom clamps at its maximum"), UKalmalaMinimapWidget::ClampZoom(15000.0f, 2500.0f, 10000.0f), 10000.0f);
    TestEqual(TEXT("Minimap zoom retains values inside its bounds"), UKalmalaMinimapWidget::ClampZoom(6000.0f, 2500.0f, 10000.0f), 6000.0f);
    TestFalse(TEXT("A modal UI retains mouse-wheel ownership"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(false));
    TestTrue(TEXT("Minimap accepts mouse-wheel input without a modal UI"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(true));
    return true;
}

#endif
