#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaOceanSampler.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSamplerTest, "Kalmala.World.Water.OceanDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSamplerTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    FVector2D Origin(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation());
    // Locate a physical master-map coastline, then inspect its collision lattice.
    bool bFoundCoast = false;
    for (int32 Y = -80; Y <= 80 && !bFoundCoast; ++Y)
    for (int32 X = -80; X < 80 && !bFoundCoast; ++X)
    {
        const FVector2D P(X * 10000.0, Y * 10000.0);
        if ((FKalmalaTerrainHeightSampler::SampleHeight(Config, P) < 0) !=
            (FKalmalaTerrainHeightSampler::SampleHeight(Config, P + FVector2D(10000, 0)) < 0))
        { Origin = P + FVector2D(5000, 0); bFoundCoast = true; }
    }
    TestTrue(TEXT("Master crop contains a coast fixture"), bFoundCoast);
    FKalmalaWorldGenerationConfig Other = Config;
    Other.WorldSeed = 999;
    const FVector2D OtherOrigin(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Other).GetLocation());
    int32 Wet = 0, Dry = 0, Coastal = 0, Different = 0;
    for (int32 Y = -64; Y < 64; ++Y)
    {
        for (int32 X = -64; X < 64; ++X)
        {
            const FVector2D Base = Origin + FVector2D(X, Y) * 125.0;
            for (int32 Upper = 0; Upper < 2; ++Upper)
            {
                FKalmalaWaterMeshVertex V[3];
                V[0].Position = Base + FVector2D(125, 0);
                V[1].Position = Base + FVector2D(0, 125);
                V[2].Position = Base + (Upper ? FVector2D(125, 125) : FVector2D::ZeroVector);
                for (auto& Vertex : V) Vertex.TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, Vertex.Position);
                const FVector2D Centre = (V[0].Position + V[1].Position + V[2].Position) / 3.0;
                const auto Sample = FKalmalaOceanSampler::Sample(Config, Centre);
                const float Expected = (V[0].TerrainHeight + V[1].TerrainHeight + V[2].TerrainHeight) / 3.0f;
                TestEqual(TEXT("Depth uses collision triangle plane, including negative coordinates and both diagonals"), Sample.TerrainHeight, Expected, 0.001f);
                TestEqual(TEXT("Repeated peer identity reproduces depth"), Sample.WaterDepth, FKalmalaOceanSampler::Sample(Config, Centre).WaterDepth);
                TestEqual(TEXT("Adjacent patch origins share the same sea query"), Sample.TerrainHeight,
                    FKalmalaOceanSampler::Sample(Config, Centre, Origin + FVector2D(3000, -3000)).TerrainHeight, 0.001f);
                Sample.IsWater() ? ++Wet : ++Dry;
                if (!FMath::IsNearlyEqual(Sample.TerrainHeight, FKalmalaOceanSampler::Sample(Other, Centre, OtherOrigin).TerrainHeight, 0.1f)) ++Different;
                const float Min = FMath::Min3(V[0].TerrainHeight, V[1].TerrainHeight, V[2].TerrainHeight);
                const float Max = FMath::Max3(V[0].TerrainHeight, V[1].TerrainHeight, V[2].TerrainHeight);
                if (Min < 0 && Max > 0)
                {
                    ++Coastal;
                    FKalmalaWaterMesh Mesh;
                    FKalmalaWaterSurfaceMesh::AppendTriangle(V[0], V[1], V[2], false, false, Mesh);
                    TestFalse(TEXT("Mixed coastal triangle renders water"), Mesh.Vertices.IsEmpty());
                    for (const FVector& Point : Mesh.Vertices)
                    {
                        const auto Edge = FKalmalaOceanSampler::Sample(Config, FVector2D(Point));
                        TestTrue(TEXT("Clipped sea vertices never cover dry terrain"), Edge.TerrainHeight <= 0.001f);
                        TestEqual(TEXT("Rendered sea depth agrees with query"), Edge.WaterDepth, FMath::Max(0.0f, -Edge.TerrainHeight), 0.001f);
                    }
                }
            }
        }
    }
    TestTrue(TEXT("Fixture covers sea floor, dry land and physical coasts"), Wet > 0 && Dry > 0 && Coastal > 0);
    TestTrue(TEXT("Different seed changes terrain beneath sea"), Different > 0);
    TestTrue(TEXT("Current seeded world supplies a valid query"), FKalmalaOceanSampler::Sample(Config, Origin).bIsValid);
    TestFalse(TEXT("Nonfinite positions are rejected"), FKalmalaOceanSampler::Sample(Config,
        FVector2D(std::numeric_limits<double>::quiet_NaN(), 0)).bIsValid);
    AddInfo(FString::Printf(TEXT("Ocean fixture: wet=%d dry=%d coastal=%d different=%d"), Wet, Dry, Coastal, Different));
    return true;
}
#endif
