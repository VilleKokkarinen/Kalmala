#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWorldMapWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldMapExplorationSaveGame.h"
#include "KalmalaWorldMapPinsSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldMapWidgetTest, "Kalmala.UI.WorldMap.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldMapWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Expanded-map zoom clamps at its minimum"), UKalmalaWorldMapWidget::ClampMapZoom(100.0f, 2500.0f, 50000.0f), 2500.0f);
    TestEqual(TEXT("Expanded-map zoom clamps at its maximum"), UKalmalaWorldMapWidget::ClampMapZoom(100000.0f, 2500.0f, 50000.0f), 50000.0f);
    TestEqual(TEXT("Expanded-map zoom preserves valid values"), UKalmalaWorldMapWidget::ClampMapZoom(18000.0f, 2500.0f, 50000.0f), 18000.0f);
    TestEqual(TEXT("Owning player maps to the centre of a recentered map"),
        UKalmalaWorldMapWidget::WorldToMapNormalized(FVector2D(1000.0f, -500.0f), FVector2D(1000.0f, -500.0f), FVector2D(8000.0f, 4500.0f)), FVector2D(0.5f, 0.5f));
    TestEqual(TEXT("Map marker follows only its world-space offset"),
        UKalmalaWorldMapWidget::WorldToMapNormalized(FVector2D(9000.0f, 4000.0f), FVector2D(1000.0f, -500.0f), FVector2D(8000.0f, 4500.0f)), FVector2D(1.0f, 1.0f));
    TestTrue(TEXT("Owning marker faces actor yaw in map space"),
        UKalmalaWorldMapWidget::GetFacingDirection(90.0f).Equals(FVector2D(0.0f, 1.0f), KINDA_SMALL_NUMBER));
    TestEqual(TEXT("Expanded map uses a bounded local grid at close zoom"), UKalmalaWorldMapWidget::ChooseGridSpacing(2500.0f), 2500.0f);
    TestEqual(TEXT("Expanded map uses a broad local grid at maximum zoom"), UKalmalaWorldMapWidget::ChooseGridSpacing(50000.0f), 10000.0f);
    TestEqual(TEXT("Pin labels trim unsupported characters and respect the maximum length"),
        UKalmalaWorldMapWidget::SanitizePinLabel(TEXT("  Ember! cairn @ north  ")), FString(TEXT("Ember cairn  north")));
    TestTrue(TEXT("Pin labels accept bounded original player text"), UKalmalaWorldMapWidget::IsValidPinLabel(TEXT("Ember cairn")));
    TestFalse(TEXT("Pin labels reject empty text"), UKalmalaWorldMapWidget::IsValidPinLabel(TEXT("")));
    TestFalse(TEXT("Pin labels reject unsanitized text"), UKalmalaWorldMapWidget::IsValidPinLabel(TEXT("Ember!")));
    UKalmalaWorldMapWidget* PinWidget = NewObject<UKalmalaWorldMapWidget>();
    PinWidget->BeginPinPlacement(FVector2D(1250.0f, -750.0f));
    PinWidget->PendingPinLabel = TEXT("  Lantern! ridge  ");
    PinWidget->PendingPinStyle = EKalmalaWorldMapPinStyle::Lantern;
    PinWidget->CommitPinPlacement();
    TestEqual(TEXT("Local pin placement commits one personal marker"), PinWidget->LocalPins.Num(), 1);
    if (!PinWidget->LocalPins.IsEmpty())
    {
        TestEqual(TEXT("Local pin placement keeps its selected finite style"), PinWidget->LocalPins[0].Style, EKalmalaWorldMapPinStyle::Lantern);
        TestEqual(TEXT("Local pin placement stores only a sanitized label"), PinWidget->LocalPins[0].Label, FString(TEXT("Lantern ridge")));
        TestEqual(TEXT("Local pin placement preserves the clicked world coordinate"), PinWidget->LocalPins[0].WorldLocation, FVector2D(1250.0f, -750.0f));
    }
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
    UKalmalaWorldMapPinsSaveGame* Pins = NewObject<UKalmalaWorldMapPinsSaveGame>();
    Pins->InitializeForWorld(Config);
    TestTrue(TEXT("Personal pins accept bounded local presentation data"), Pins->SetPins(PinWidget->LocalPins));
    TArray<uint8> SerializedPins;
    TestTrue(TEXT("Personal pins serialize independently in memory"), UGameplayStatics::SaveGameToMemory(Pins, SerializedPins));
    UKalmalaWorldMapPinsSaveGame* ReloadedPins = Cast<UKalmalaWorldMapPinsSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedPins));
    TestNotNull(TEXT("Personal pins reload from their own save schema"), ReloadedPins);
    if (ReloadedPins != nullptr)
    {
        TestTrue(TEXT("Reloaded personal pins retain immutable world identity"), ReloadedPins->MatchesWorld(Config));
        TestEqual(TEXT("Reloaded personal pins retain label and local coordinate"), ReloadedPins->GetPins()[0].Label, FString(TEXT("Lantern ridge")));
        FKalmalaWorldGenerationConfig MismatchedPinConfig = Config;
        ++MismatchedPinConfig.WorldSeed;
        TestFalse(TEXT("Personal pins reject a mismatched immutable identity"), ReloadedPins->MatchesWorld(MismatchedPinConfig));
    }
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
    const TArray<FColor> RevealEdgePixels = UKalmalaWorldMapWidget::BuildFogPixels(FVector2D(6500.0f, 0.0f), FVector2D(100.0f, 1.0f),
        FVector2D::ZeroVector, FIntPoint(3, 1));
    TestEqual(TEXT("Reveal edge keeps the exact radius clear"), RevealEdgePixels[1].A, uint8(0));
    TestEqual(TEXT("Reveal edge occludes the first remote sample"), RevealEdgePixels[2],
        UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::Unexplored));
    const TArray<FColor> FogPixels = UKalmalaWorldMapWidget::BuildFogPixels(FVector2D::ZeroVector, FVector2D(10000.0f, 5000.0f),
        FVector2D::ZeroVector, FIntPoint(9, 5));
    TestEqual(TEXT("Fog covers every local map sample"), FogPixels.Num(), 45);
    TestEqual(TEXT("Fog clears only the owning-pawn sample"), FogPixels[22].A, uint8(0));
    TestEqual(TEXT("Fog fully hides remote map samples"), FogPixels[0].A, uint8(255));
    UKalmalaWorldMapExplorationSaveGame* Exploration = NewObject<UKalmalaWorldMapExplorationSaveGame>();
    Exploration->InitializeForWorld(Config);
    TestTrue(TEXT("Personal coverage records an owning-player reveal"), Exploration->RecordReveal(FVector2D::ZeroVector, 6500.0f));
    TestTrue(TEXT("Personal coverage retains a previously revealed local cell"), Exploration->IsExplored(FVector2D(500.0f, 500.0f)));
    TestFalse(TEXT("Personal coverage rejects remote unexplored cells"), Exploration->IsExplored(FVector2D(20000.0f, 0.0f)));
    const TArray<FColor> RememberedFogPixels = UKalmalaWorldMapWidget::BuildFogPixels(FVector2D::ZeroVector, FVector2D(20000.0f, 20000.0f),
        FVector2D(-100000.0f, 0.0f), FIntPoint(3, 3), Exploration);
    TestEqual(TEXT("Personal map memory has a translucent sea-glass treatment"), RememberedFogPixels[4],
        UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::RememberedPersonal));
    TestTrue(TEXT("Reserved shared-map treatment remains visually distinct from personal memory"),
        UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::RememberedPersonal)
        != UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::ReservedShared));
    TestEqual(TEXT("No shared exploration is materialized by the local map"), RememberedFogPixels[0],
        UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::Unexplored));
    TArray<uint8> SerializedExploration;
    TestTrue(TEXT("Personal coverage serializes independently in memory"), UGameplayStatics::SaveGameToMemory(Exploration, SerializedExploration));
    UKalmalaWorldMapExplorationSaveGame* ReloadedExploration = Cast<UKalmalaWorldMapExplorationSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedExploration));
    TestNotNull(TEXT("Personal coverage reloads from its own save schema"), ReloadedExploration);
    if (ReloadedExploration != nullptr)
    {
        TestTrue(TEXT("Reloaded personal coverage retains the immutable identity"), ReloadedExploration->MatchesWorld(Config));
        const TArray<FColor> ReloadedFogPixels = UKalmalaWorldMapWidget::BuildFogPixels(FVector2D::ZeroVector, FVector2D(20000.0f, 20000.0f),
            FVector2D(-100000.0f, 0.0f), FIntPoint(3, 3), ReloadedExploration);
        TestEqual(TEXT("Restarted personal coverage restores only the remembered local cell"), ReloadedFogPixels[4],
            UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::RememberedPersonal));
        TestEqual(TEXT("Restarted personal coverage keeps remote terrain opaque"), ReloadedFogPixels[0],
            UKalmalaWorldMapWidget::GetFogTreatmentColor(EKalmalaWorldMapFogTreatment::Unexplored));
        FKalmalaWorldGenerationConfig MismatchedConfig = Config;
        ++MismatchedConfig.WorldSeed;
        TestFalse(TEXT("Personal coverage rejects a mismatched immutable identity"), ReloadedExploration->MatchesWorld(MismatchedConfig));
    }
    UKalmalaWorldMapWidget* Widget = NewObject<UKalmalaWorldMapWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestEqual(TEXT("Expanded-map viewport slot stretches horizontally"), Slot.Anchors.Minimum.X, 0.0);
    TestEqual(TEXT("Expanded-map viewport slot stretches vertically"), Slot.Anchors.Maximum.Y, 1.0);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
