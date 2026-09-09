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
    Config.GeneratorRevision = 4;
    const TArray<FColor> Tile = UKalmalaWorldMapWidget::BuildTilePixels(Config, FIntPoint(3, -2));
    const TArray<FColor> RepeatedTile = UKalmalaWorldMapWidget::BuildTilePixels(Config, FIntPoint(3, -2));
    const TArray<FColor> AdjacentTile = UKalmalaWorldMapWidget::BuildTilePixels(Config, FIntPoint(4, -2));
    TestEqual(TEXT("World-space tile has its bounded sample count"), Tile.Num(), 33 * 33);
    TestTrue(TEXT("World-space tile reproduces for the same identity"), Tile == RepeatedTile);
    bool bSeamMatches = Tile.Num() == 33 * 33 && AdjacentTile.Num() == 33 * 33;
    for (int32 Y = 0; bSeamMatches && Y < 33; ++Y) bSeamMatches = Tile[Y * 33 + 32] == AdjacentTile[Y * 33];
    TestTrue(TEXT("Adjacent world-space tiles share their edge samples"), bSeamMatches);
    Config.WorldSeed = 419;
    TestTrue(TEXT("Different identities vary tile presentation"), Tile != UKalmalaWorldMapWidget::BuildTilePixels(Config, FIntPoint(3, -2)));
    const TArray<FIntPoint> MaximumZoomTiles = UKalmalaWorldMapWidget::BuildPrioritizedTileCoordinates(FVector2D::ZeroVector, FVector2D(94000.0f, 50000.0f));
    TestEqual(TEXT("Expanded map caps max-zoom tile requests"), MaximumZoomTiles.Num(), 64);
    TestTrue(TEXT("Expanded map prioritizes the player-centre tile at max zoom"), MaximumZoomTiles.Contains(FIntPoint(0, 0)));
    TestTrue(TEXT("Owning pawn opens only its bounded local reveal radius"),
        UKalmalaWorldMapWidget::IsWithinLocalRevealRadius(FVector2D(6400.0f, 0.0f), FVector2D::ZeroVector, 6500.0f));
    TestFalse(TEXT("Remote terrain remains outside the local reveal radius"),
        UKalmalaWorldMapWidget::IsWithinLocalRevealRadius(FVector2D(6501.0f, 0.0f), FVector2D::ZeroVector, 6500.0f));
    const TArray<FColor> FogPixels = UKalmalaWorldMapWidget::BuildFogPixels(FVector2D::ZeroVector, FVector2D(10000.0f, 5000.0f),
        FVector2D::ZeroVector, FIntPoint(9, 5));
    TestEqual(TEXT("Fog covers every local map sample"), FogPixels.Num(), 45);
    TestEqual(TEXT("Fog clears only the owning-pawn sample"), FogPixels[22].A, uint8(0));
    TestEqual(TEXT("Fog fully hides remote map samples"), FogPixels[0].A, uint8(255));
    UKalmalaWorldMapWidget* Widget = NewObject<UKalmalaWorldMapWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestEqual(TEXT("Expanded-map viewport slot stretches horizontally"), Slot.Anchors.Minimum.X, 0.0);
    TestEqual(TEXT("Expanded-map viewport slot stretches vertically"), Slot.Anchors.Maximum.Y, 1.0);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
