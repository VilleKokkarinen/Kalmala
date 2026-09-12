#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRegionalGeneration.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaFiniteWorldTest, "Kalmala.World.Regional.FiniteWorld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaFiniteWorldTest::RunTest(const FString& Parameters)
{
    using B = FKalmalaWorldBounds;
    using G = FKalmalaRegionalGeneration;
    const FKalmalaWorldGenerationConfig C{418};
    TestTrue(TEXT("Origin is inside"), B::Contains(C, FVector2D::ZeroVector));
    TestTrue(TEXT("16 km radius is inclusive"), B::Contains(C, FVector2D(B::Radius, 0)));
    TestFalse(TEXT("Beyond the radius is outside"), B::Contains(C, FVector2D(B::Radius + 1, 0)));
    TestFalse(TEXT("Square corners are outside"), B::Contains(C, FVector2D(B::Radius * .8)));
    TestFalse(TEXT("Outside patches are rejected"), B::IntersectsPatch(C, FVector2D(B::Radius + 2000, 0), 1500));
    TestTrue(TEXT("Boundary patches are retained for clipping"), B::IntersectsPatch(C, FVector2D(B::Radius + 1000, 0), 1500));
    for (double Angle : {0.0, 0.5, 2.0, 3.5, 5.0})
    {
        const FVector2D P = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (B::Radius + 10000);
        TestTrue(TEXT("Movement clamp retains capsule clearance in every direction"),
            FMath::IsNearlyEqual(B::Constrain(C, P, 44).Size(), B::Radius - 44, .001));
    }
    TArray<FVector> V = {FVector(B::Radius - 100, -100, 10), FVector(B::Radius - 100, 100, 20), FVector(B::Radius + 100, 0, 30)};
    TArray<int32> I = {0, 1, 2};
    B::ClipMesh(C, FVector2D::ZeroVector, V, I);
    TestEqual(TEXT("Crossing triangle clips to a quad"), I.Num(), 6);
    for (auto P : V) TestTrue(TEXT("Clipped collision stays inside circle"), FVector2D(P).Size() <= B::Radius + .001);
    const auto Water = FKalmalaWaterSurfaceMesh::BuildPatch(C, FVector2D(B::Radius, 0), false);
    for (auto P : Water.Vertices) TestTrue(TEXT("Sea mesh stays inside circle"), (FVector2D(P) + FVector2D(B::Radius, 0)).Size() <= B::Radius + .001);
    for (auto Kind : {EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard})
        TestTrue(TEXT("No population beyond edge"), FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(C, FIntPoint(300, 300), Kind).IsEmpty());
    TestFalse(TEXT("Small streams disabled in current worlds"), G::AreStreamsEnabled(C));
    int32 Rivers = 0;
    for (int32 Y = -4; Y <= 4; ++Y) for (int32 X = -4; X <= 4; ++X)
    {
        for (const auto& S : G::GetHydrology(C, FIntPoint(X, Y))) { TestFalse(TEXT("No stream segments"), S.bStream); ++Rivers; }
    }
    TestTrue(TEXT("Large rivers still generated"), Rivers > 0);
    // Distribution checks use actual dominant classifications, not the weighting helper.
    for (uint64 Seed : {418ull, 419ull})
    {
        double Tier[2] = {}; int32 Land[2] = {};
        const FKalmalaWorldGenerationConfig Config{Seed};
        for (int32 Ring = 0; Ring < 2; ++Ring) for (int32 N = 0; N < 192; ++N)
        {
            const double Angle = N * 2.399963229728653;
            const double Radius = B::Radius * (Ring == 0 ? .02 + .13 * N / 192.0 : .70 + .28 * N / 192.0);
            const FVector2D P(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
            const auto A = G::Sample(Config, P), Repeat = G::Sample(Config, P);
            TestEqual(TEXT("Same identity reproduces terrain"), A.Height, Repeat.Height);
            TestEqual(TEXT("No stream carving"), A.StreamWeight, 0.0f);
            if (A.Biome != 1 && A.Biome != 6) { Tier[Ring] += A.Biome; ++Land[Ring]; }
        }
        const double Inner = Tier[0] / FMath::Max(1, Land[0]), Outer = Tier[1] / FMath::Max(1, Land[1]);
        AddInfo(FString::Printf(TEXT("Seed %llu: inner mean tier %.3f, outer %.3f, land samples %d/%d"), Seed, Inner, Outer, Land[0], Land[1]));
        TestTrue(TEXT("Outer land actually favours higher tiers"), Outer > Inner + .5);
    }
    return true;
}
#endif
