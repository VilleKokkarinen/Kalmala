#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWaterSurfaceMeshTest, "Kalmala.World.Water.ClippedSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWaterSurfaceMeshTest::RunTest(const FString& Parameters)
{
    auto Vertex = [](double X, double Y, float Height, float Humidity = 0.68f)
    {
        FKalmalaWaterMeshVertex V;
        V.Position = FVector2D(X, Y);
        V.TerrainHeight = Height;
        V.WaterLevel = 400.0f;
        V.BasinWeight = 1.0f;
        V.Fields = { 0.35f, Humidity, 0.5f, 0.5f };
        return V;
    };
    FKalmalaWaterMesh Ocean;
    FKalmalaWaterSurfaceMesh::AppendTriangle(Vertex(0, 0, -10), Vertex(0, 100, 10), Vertex(100, 0, -10), false, false, Ocean);
    double Area = 0;
    for (int32 I = 0; I < Ocean.Vertices.Num(); I += 3)
    {
        const FVector Cross = FVector::CrossProduct(Ocean.Vertices[I + 1] - Ocean.Vertices[I], Ocean.Vertices[I + 2] - Ocean.Vertices[I]);
        TestTrue(TEXT("Water keeps terrain triangle winding"), Cross.Z < 0);
        Area += FMath::Abs(Cross.Z) * 0.5;
    }
    TestEqual(TEXT("Partly submerged triangle keeps precisely its wet area"), Area, 3750.0, 0.01);
    for (const FVector& V : Ocean.Vertices) TestEqual(TEXT("Sea water remains level"), V.Z, 0.0);

    FKalmalaWaterMesh Lake, Shore;
    // Once a basin is accepted, humidity edges must not cut its water surface.
    const auto A = Vertex(0, 0, 300, 0.6f);
    const auto B = Vertex(0, 100, 300, 0.8f);
    const auto C = Vertex(100, 0, 300, 0.8f);
    FKalmalaWaterSurfaceMesh::AppendTriangle(A, B, C, true, false, Lake);
    FKalmalaWaterSurfaceMesh::AppendTriangle(A, B, C, true, true, Shore);
    TestEqual(TEXT("Accepted basin keeps its whole triangle across internal biome edges"), Lake.Vertices.Num(), 3);
    TestTrue(TEXT("A humidity boundary cannot create an exposed water edge"), Lake.Vertices.Contains(FVector(A.Position, 400.0f)));
    TestTrue(TEXT("Deep biome edges do not get floating shore frames"), Shore.Triangles.IsEmpty());
    for (const FVector& V : Lake.Vertices) TestEqual(TEXT("Lake water remains at its existing level"), V.Z, 400.0);

    FKalmalaWaterMesh Shallow;
    FKalmalaWaterSurfaceMesh::AppendTriangle(Vertex(0, 0, 410), Vertex(0, 100, 390), Vertex(100, 0, 390), true, true, Shallow);
    TestTrue(TEXT("Physical terrain intersection receives shoreline treatment"), !Shallow.Triangles.IsEmpty());

    FKalmalaWaterMesh BasinOnly, RiverSurface;
    auto RiverA = Vertex(0, 0, -80.0f), RiverB = Vertex(0, 100, -80.0f), RiverC = Vertex(100, 0, -80.0f);
    RiverA.WaterLevel = RiverB.WaterLevel = RiverC.WaterLevel = 0.0f;
    RiverA.BasinWeight = RiverB.BasinWeight = RiverC.BasinWeight = 0.0f;
    FKalmalaWaterSurfaceMesh::AppendTriangle(RiverA, RiverB, RiverC, true, false, BasinOnly);
    FKalmalaWaterSurfaceMesh::AppendTriangle(RiverA, RiverB, RiverC, false, false, RiverSurface);
    TestTrue(TEXT("A sea-level river cannot create a lake sheet without basin support"), BasinOnly.Triangles.IsEmpty());
    TestTrue(TEXT("The same submerged river remains in the sea-level surface mesh"), !RiverSurface.Triangles.IsEmpty());
    for (const FVector& V : RiverSurface.Vertices) TestEqual(TEXT("Sea-level river surface remains at zero"), V.Z, 0.0f);

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    TestFalse(TEXT("Analytic basin policy remains disabled"), FKalmalaShimmeringLakeSampler::bEnabled);
    const FVector2D FormerBasinSample(100830.5f, -143038.6f);
    TestFalse(TEXT("Disabled basin cannot report Shimmering Lakes water"), FKalmalaShimmeringLakeSampler::IsWater(Config, FormerBasinSample));
    TestFalse(TEXT("Disabled basin cannot report basin containment"), FKalmalaLakeBasin::Contains(Config, FormerBasinSample, FVector2D::ZeroVector));
    TestFalse(TEXT("Disabled basin cannot report visible lake water"), FKalmalaLakeBasin::IsVisibleWater(Config, FormerBasinSample));
    const auto DisabledLakeMesh = FKalmalaWaterSurfaceMesh::BuildPatch(Config, FormerBasinSample, true);
    TestTrue(TEXT("Disabled analytic basins produce no production lake patch"), DisabledLakeMesh.Triangles.IsEmpty());
    int32 PositiveInlandSurfaceVertices = 0;
    for (int32 Y = -3; Y <= 3; ++Y)
    {
        const FVector2D Centre(0, Y * 3000.0);
        const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(Config, Centre, true);
        const auto Repeat = FKalmalaWaterSurfaceMesh::BuildPatch(Config, Centre, true);
        TestTrue(TEXT("Repeated seed gives identical clipped mesh"), Left.Vertices == Repeat.Vertices && Left.Triangles == Repeat.Triangles);
        for (const FVector& V : Left.Vertices) PositiveInlandSurfaceVertices += V.Z > 0.01f;
    }
    TestEqual(TEXT("Disabled analytic basins produce no elevated inland water surface"), PositiveInlandSurfaceVertices, 0);
    TArray<FIntPoint> Wet;
    auto Bowl = [](FIntPoint P) { return TPair<float, bool>(P.X*P.X + P.Y*P.Y < 25 ? 300.0f : 500.0f, P == FIntPoint(0,0)); };
    TestTrue(TEXT("Enclosed depression containing a lake seed fills"), FKalmalaLakeBasin::Find(FIntPoint(0,0), Bowl, Wet));
    TestTrue(TEXT("Water extends past biome boundaries to physical banks"), Wet.Contains(FIntPoint(3,0)));
    TestFalse(TEXT("A channel to sea level rejects the entire suspended sheet"), FKalmalaLakeBasin::Find(FIntPoint(0,0), [&](FIntPoint P)
    {
        if (P.Y == 0 && P.X >= 0) return TPair<float,bool>(P.X >= 8 ? 0.0f : 300.0f, P.X == 0);
        return Bowl(P);
    }, Wet));
    TestFalse(TEXT("Unbounded water is omitted instead of cut at a streaming edge"), FKalmalaLakeBasin::Find(FIntPoint(0,0), [](FIntPoint P) { return TPair<float,bool>(300.0f, true); }, Wet, 128));
    TestFalse(TEXT("Unseeded depression does not become a lake"), FKalmalaLakeBasin::Find(FIntPoint(0,0), [&](FIntPoint P) { return TPair<float,bool>(Bowl(P).Key, false); }, Wet));
    return true;
}
#endif
