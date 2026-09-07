#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMinimapViewModel.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMinimapAsyncTest, "Kalmala.UI.Minimap.AsyncRefresh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMinimapAsyncTest::RunTest(const FString& Parameters)
{
    auto* Model = NewObject<UKalmalaMinimapViewModel>();
    const FKalmalaWorldGenerationConfig Config{418, 4};
    const FVector2D Start(1250, -750);
    double MaxRefreshMs = 0;
    auto Refresh = [&](const FKalmalaWorldGenerationConfig& Identity, FVector2D Position)
    {
        const double Begin = FPlatformTime::Seconds();
        Model->RefreshTerrain(Identity, Position);
        MaxRefreshMs = FMath::Max(MaxRefreshMs, (FPlatformTime::Seconds() - Begin) * 1000);
    };
    Refresh(Config, Start);
    TestFalse(TEXT("Initial raster is not built synchronously"), Model->IsReady());
    TestTrue(TEXT("Initial request starts one job"), Model->PendingSamples.IsValid());
    // Waiting is confined to automation; production Refresh only polls readiness.
    Model->PendingSamples.Wait();
    Refresh(Config, Start);
    TestTrue(TEXT("Completed raster is published"), Model->IsReady());
    TestFalse(TEXT("Stationary view does not queue another job"), Model->PendingSamples.IsValid());
    const auto Expected = UKalmalaMinimapViewModel::BuildTerrainSamples(Config, Start, 5000, 129);
    TestEqual(TEXT("Worker preserves full resolution"), Model->TerrainSamples.Num(), Expected.Num());
    for (int32 I = 0; I < Expected.Num(); ++I)
    {
        TestEqual(TEXT("Worker preserves height"), Model->TerrainSamples[I].TerrainHeight, Expected[I].TerrainHeight);
        TestEqual(TEXT("Worker preserves water"), Model->TerrainSamples[I].bIsWater, Expected[I].bIsWater);
        TestEqual(TEXT("Worker preserves colour"), Model->TerrainSamples[I].TerrainColour, Expected[I].TerrainColour);
    }
    Refresh(Config, Start + FVector2D(100, 0));
    for (int32 I = 0; I < 100; ++I) Refresh(Config, Start + FVector2D(200 + I, 0));
    Model->PendingSamples.Wait();
    const uint32 Revision = Model->PresentationRevision;
    Model->SetMapRadius(2500);
    Refresh(Config, Start);
    TestEqual(TEXT("Obsolete zoom result is discarded"), Model->PresentationRevision, Revision);
    TestEqual(TEXT("Latest zoom is queued"), Model->PendingRadius, 2500.0f);
    Model->PendingSamples.Wait();
    const FKalmalaWorldGenerationConfig OtherConfig{419, 4};
    Refresh(OtherConfig, Start);
    TestFalse(TEXT("Old identity is hidden"), Model->IsReady());
    TestEqual(TEXT("Obsolete identity result is discarded"), Model->PresentationRevision, Revision);
    Model->PendingSamples.Wait();
    Refresh(OtherConfig, Start);
    TestTrue(TEXT("New identity is published"), Model->IsReady());
    TestEqual(TEXT("Published identity matches"), Model->LastSeed, OtherConfig.WorldSeed);
    Refresh(OtherConfig, Start + FVector2D(100, 0));
    Model->Initialize(nullptr);
    TestFalse(TEXT("Reinitialization drops pending work without retaining the object"), Model->PendingSamples.IsValid());
    AddInfo(FString::Printf(TEXT("Maximum game-thread refresh cost: %.3f ms"), MaxRefreshMs));
    TestTrue(TEXT("Movement refresh stays below a 50 ms hitch budget"), MaxRefreshMs < 50.0);
    return true;
}
#endif
