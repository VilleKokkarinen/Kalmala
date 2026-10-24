#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaMinimapViewModel.h"
#include "KalmalaMinimapWidget.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaWeatherActivityWidget.h"
#include "Blueprint/GameViewportSubsystem.h"

#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldMapWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMinimapViewModelTest, "Kalmala.UI.Minimap.LocalPresentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMinimapViewModelTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
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

    const FVector2D FormerBasinSample(100830.5f, -143038.6f);
    const auto FormerLakeMap = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FormerBasinSample, 125.0f, 3);
    TestEqual(TEXT("Former basin map fixture has a centre sample"), FormerLakeMap.Num(), 9);
    if (FormerLakeMap.Num() == 9)
        TestFalse(TEXT("Minimap keeps the fixed former basin dry under the disabled policy"), FormerLakeMap[4].bIsWater);
    const FVector2D HighlandRiverSample(679237.9f, -16414.3f);
    const auto RiverMap = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, HighlandRiverSample, 125.0f, 3);
    TestEqual(TEXT("Highland river map fixture has a centre sample"), RiverMap.Num(), 9);
    if (RiverMap.Num() == 9)
        TestTrue(TEXT("Minimap still classifies a carved sea-level river as water"), RiverMap[4].bIsWater);

    TestEqual(TEXT("Invalid local presentation input produces no samples"), UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D::ZeroVector, 0.0f, 5).Num(), 0);
    TestTrue(TEXT("Circular minimap includes its centred player marker"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D::ZeroVector));
    TestTrue(TEXT("Circular minimap includes positions on its edge"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 0.0f)));
    TestFalse(TEXT("Circular minimap rejects square-grid corners"), UKalmalaMinimapWidget::IsInsideCircularMap(FVector2D(1.0f, 1.0f)));
    TestTrue(TEXT("Circular minimap remains top-right at 16:9 and 100% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(1920.0f, 1080.0f), 208.0f, 12.0f, 1.0f));
    TestTrue(TEXT("Circular minimap remains top-right at ultrawide and 125% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(3440.0f, 1440.0f), 208.0f, 12.0f, 1.25f));
    TestTrue(TEXT("Circular minimap remains top-right at 4:3 and 75% UI scale"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(1024.0f, 768.0f), 208.0f, 12.0f, 0.75f));
    TestFalse(TEXT("Circular minimap rejects a footprint that cannot fit the viewport"), UKalmalaMinimapWidget::IsTopRightPlacementValid(FVector2D(200.0f, 100.0f), 208.0f, 12.0f, 1.0f));
    TestEqual(TEXT("Minimap zoom clamps at its minimum"), UKalmalaMinimapWidget::ClampZoom(500.0f, 2500.0f, 10000.0f), 2500.0f);
    TestEqual(TEXT("Minimap zoom clamps at its maximum"), UKalmalaMinimapWidget::ClampZoom(15000.0f, 2500.0f, 10000.0f), 10000.0f);
    TestEqual(TEXT("Minimap zoom retains values inside its bounds"), UKalmalaMinimapWidget::ClampZoom(6000.0f, 2500.0f, 10000.0f), 6000.0f);
    TestFalse(TEXT("A modal UI retains mouse-wheel ownership"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(false));
    TestTrue(TEXT("Minimap accepts mouse-wheel input without a modal UI"), UKalmalaMinimapWidget::ShouldAcceptZoomInput(true));

    // Exercise the production setters: the former math-only footprint test
    // missed SetPositionInViewport silently resetting the anchor to top-left.
    UKalmalaMinimapWidget* Widget = NewObject<UKalmalaMinimapWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestTrue(TEXT("Actual viewport slot is anchored top-right"), Slot.Anchors == FAnchors(1.0f, 0.0f));
    TestEqual(TEXT("Actual slot aligns its right edge"), Slot.Alignment, FVector2D(1.0f, 0.0f));
    TestEqual(TEXT("Right margin stays at 12 UI units without double DPI scaling"), Slot.Offsets.Left, -12.0f);
    TestEqual(TEXT("Top margin stays at 12 UI units without double DPI scaling"), Slot.Offsets.Top, 12.0f);
    TestEqual(TEXT("Actual viewport slot has a nonzero fixed width"), Slot.Offsets.Right, 208.0f);
    TestEqual(TEXT("Biome header moves the circular minimap down while retaining its viewport margin"),
        Slot.Offsets.Top + UKalmalaMinimapWidget::BiomeHeaderHeight,
        UKalmalaMinimapWidget::ViewportInset + UKalmalaMinimapWidget::BiomeHeaderHeight);
    TestEqual(TEXT("Minimap slot reserves room for the biome label above the circular map"), Slot.Offsets.Bottom,
        UKalmalaMinimapWidget::DefaultMapDiameter + UKalmalaMinimapWidget::BiomeHeaderHeight);

    UKalmalaWeatherActivityWidget* WeatherWidget = NewObject<UKalmalaWeatherActivityWidget>();
    WeatherWidget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot WeatherSlot = UGameViewportSubsystem::Get()->GetWidgetSlot(WeatherWidget);
    const float MinimapBottom = Slot.Offsets.Top + Slot.Offsets.Bottom;
    TestEqual(TEXT("Weather badge stays below the lowered minimap with its existing clear gap"), WeatherSlot.Offsets.Top - MinimapBottom, 24.0f);
    TestTrue(TEXT("Weather badge remains separated by at least the minimap edge inset"), WeatherSlot.Offsets.Top - MinimapBottom >= 12.0f);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    UGameViewportSubsystem::Get()->RemoveWidget(WeatherWidget);

    const auto DetailedSamples = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, FVector2D::ZeroVector, 5000.0f, 129);
    const auto Pixels = FKalmalaMinimapRaster::BuildPixels(DetailedSamples);
    TestEqual(TEXT("A full texture replaces the sparse dots"), Pixels.Num(), 129 * 129);
    for (int32 Index = 0; Index < Pixels.Num(); ++Index)
    {
        const double Distance = DetailedSamples[Index].MapPosition.Size();
        if (Distance >= 1.0) TestEqual(TEXT("Outside the circle is transparent"), Pixels[Index].A, uint8(0));
        if (Distance < 0.98) TestEqual(TEXT("Inside the circle is filled without gaps"), Pixels[Index].A, uint8(255));
    }
    TSet<FColor> BiomeColours;
    for (int32 BiomeIndex = 0; BiomeIndex <= static_cast<int32>(EKalmalaBiome::Ocean); ++BiomeIndex)
    {
        const auto Biome = static_cast<EKalmalaBiome>(BiomeIndex);
        const FLinearColor First = FKalmalaMinimapRaster::SampleBiomeTexture(Biome, FVector2D(113.0, -291.0));
        BiomeColours.Add(First.ToFColorSRGB());
        TestEqual(TEXT("Biome texture repeats at a fixed world coordinate"), First,
            FKalmalaMinimapRaster::SampleBiomeTexture(Biome, FVector2D(113.0, -291.0)));
        TestNotEqual(TEXT("Every biome has texture detail, not a flat fill"), First,
            FKalmalaMinimapRaster::SampleBiomeTexture(Biome, FVector2D(331.0, 89.0)));
    }
    TestEqual(TEXT("All seven biome swatches are distinct"), BiomeColours.Num(), 7);
    bool bFoundOcean = false;
    for (int32 Y = -40; Y <= 40 && !bFoundOcean; ++Y)
    {
        for (int32 X = -40; X <= 40 && !bFoundOcean; ++X)
        {
            const FVector2D Position(X * 15000.0, Y * 15000.0);
            if (FKalmalaWorldFieldSampler::Sample(Config, Position).Elevation < 0.22f)
            {
                const auto Ocean = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, Position, 100.0f, 3);
                TestTrue(TEXT("Sea-level ocean is drawn as water"), Ocean[4].bIsWater);
                bFoundOcean = true;
            }
        }
    }
    TestTrue(TEXT("Ocean fixture found"), bFoundOcean);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaLiveMapHeadingTest, "Kalmala.UI.WorldMap.LivePlayerHeading",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaLiveMapHeadingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Heading fixture world exists"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Pawn = World->SpawnActor<ACharacter>();
    if (!TestNotNull(TEXT("Local controller exists"), Controller) || !TestNotNull(TEXT("Local pawn exists"), Pawn)) return false;
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Controller->Possess(Pawn);
    TestTrue(TEXT("Fixture uses a local owning controller"), Controller->IsLocalController());
    TestTrue(TEXT("Fixture possesses its character"), Controller->GetPawn() == Pawn);
    auto* Model = NewObject<UKalmalaMinimapViewModel>();
    Model->Initialize(Controller);
    const TArray<float> Headings = { 0, 90, 180, -90 };
    const TArray<FVector2D> Directions = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
    for (int32 Index = 0; Index < Headings.Num(); ++Index)
    {
        TestTrue(TEXT("Fixture character physically rotates"), Pawn->SetActorRotation(FRotator(0, Headings[Index], 0)));
        TestTrue(TEXT("Heading follows rotation without terrain refresh"),
            FMath::IsNearlyEqual(FMath::FindDeltaAngleDegrees(Model->GetPlayerFacingDegrees(), Headings[Index]), 0.0f));
        TestTrue(TEXT("Expanded-map arrow follows live pawn direction"),
            UKalmalaWorldMapWidget::GetFacingDirection(Model->GetPlayerFacingDegrees()).Equals(Directions[Index], 0.001f));
    }
    TestFalse(TEXT("Reading heading never schedules or waits for terrain"), Model->IsReady());
    Controller->UnPossess();
    TestEqual(TEXT("Missing pawn has a safe fallback"), Model->GetPlayerFacingDegrees(), 0.0f);
    Model->Initialize(nullptr);
    TestEqual(TEXT("Missing owner has a safe fallback"), Model->GetPlayerFacingDegrees(), 0.0f);
    return true;
}

#endif
