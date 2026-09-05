#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWaterSurfaceMesh.h"
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
    // The old four-corner gate discarded this wet band entirely.
    const auto A = Vertex(0, 0, 300, 0.6f);
    const auto B = Vertex(0, 100, 300, 0.8f);
    const auto C = Vertex(100, 0, 300, 0.8f);
    FKalmalaWaterSurfaceMesh::AppendTriangle(A, B, C, true, false, Lake);
    FKalmalaWaterSurfaceMesh::AppendTriangle(A, B, C, true, true, Shore);
    TestTrue(TEXT("Lake crossing dry corners survives continuous clipping"), !Lake.Triangles.IsEmpty());
    TestTrue(TEXT("Deep biome edges do not get floating shore frames"), Shore.Triangles.IsEmpty());
    for (const FVector& V : Lake.Vertices) TestEqual(TEXT("Lake water remains at its existing level"), V.Z, 400.0);

    FKalmalaWaterMesh Shallow;
    FKalmalaWaterSurfaceMesh::AppendTriangle(Vertex(0, 0, 410), Vertex(0, 100, 390), Vertex(100, 0, 390), true, true, Shallow);
    TestTrue(TEXT("Physical terrain intersection receives shoreline treatment"), !Shallow.Triangles.IsEmpty());

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    Config.GeneratorRevision = 1;
    int32 SharedVertices = 0;
    for (int32 Y = -3; Y <= 3; ++Y)
    {
        const FVector2D Centre(0, Y * 3000.0);
        const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(Config, Centre, true);
        const auto Repeat = FKalmalaWaterSurfaceMesh::BuildPatch(Config, Centre, true);
        TestTrue(TEXT("Repeated seed gives identical clipped mesh"), Left.Vertices == Repeat.Vertices && Left.Triangles == Repeat.Triangles);
        const auto Right = FKalmalaWaterSurfaceMesh::BuildPatch(Config, Centre + FVector2D(3000, 0), true);
        for (const FVector& V : Left.Vertices)
        {
            if (!FMath::IsNearlyEqual(V.X, 1500.0, 0.001)) continue;
            ++SharedVertices;
            TestTrue(TEXT("Adjacent patches share identical water edge intersections"), Right.Vertices.ContainsByPredicate([&V](const FVector& R)
                { return FMath::IsNearlyEqual(R.X, -1500.0, 0.001) && FMath::IsNearlyEqual(R.Y, V.Y, 0.01) && R.Z == V.Z; }));
        }
    }
    TestTrue(TEXT("Seed fixture exercises wet patch boundaries"), SharedVertices > 0);
    return true;
}
#endif
