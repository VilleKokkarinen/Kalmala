#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRegionalGeneration.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRegionalGenerationTest, "Kalmala.World.Regional.Integrated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRegionalGenerationTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr int32 Side = 161;
    constexpr double Step = 2500;
    TArray<uint8> Previous;
    for (uint64 Seed : {418ull, 419ull})
    {
        FKalmalaWorldGenerationConfig C{Seed, 3};
        TArray<uint8> Biomes;
        int32 Counts[7] = {}, RiverCount = 0, StreamCount = 0, Edges = 0, Different = 0;
        double MaxJump = 0, MaxWeightJump = 0;
        FVector2D WaterFixture = FVector2D::ZeroVector;
        float BestBasinDepth = 0;
        bool bWaterFixture = false;
        FVector2D RiverFixture = FVector2D::ZeroVector;
        for (int32 Y = 0; Y < Side; ++Y) for (int32 X = 0; X < Side; ++X)
        {
            const FVector2D P((X - Side / 2) * Step, (Y - Side / 2) * Step);
            auto Fields = FKalmalaWorldFieldSampler::Sample(C, P);
            const auto R = G::Sample(Fields);
            Biomes.Add(R.Biome);
            ++Counts[R.Biome];
            RiverCount += R.RiverWeight > 0;
            if (R.RiverWeight > 0) RiverFixture = P;
            StreamCount += R.StreamWeight > 0;
            if (R.BasinWeight > 0.99f && R.WaterLevel - R.Height > BestBasinDepth)
            { WaterFixture = P; bWaterFixture = true; BestBasinDepth = R.WaterLevel - R.Height; }
            if (!Previous.IsEmpty()) Different += Previous[Biomes.Num() - 1] != R.Biome;
            if (X > 0) Edges += R.Biome != Biomes[Biomes.Num() - 2];
            if (Y > 0) Edges += R.Biome != Biomes[Biomes.Num() - 1 - Side];
            float Total = 0;
            for (float W : R.Weights) { TestTrue(TEXT("Finite normalized biome weight"), FMath::IsFinite(W) && W >= 0 && W <= 1); Total += W; }
            TestTrue(TEXT("Weights sum to one"), FMath::IsNearlyEqual(Total, 1.0f, 0.0001f));
            if (Fields.Elevation < 0.22f) TestEqual(TEXT("Source sea floor stays ocean"), R.Biome, uint8(6));
            if (Fields.Elevation > 0.78f) TestEqual(TEXT("Source peaks stay mountains"), R.Biome, uint8(5));
            if (R.Biome == 1) TestTrue(TEXT("Lake identity requires an enclosed bowl"), R.BasinWeight > 0);
            if (X % 8 == 0 && Y % 8 == 0)
            {
                Fields.Flora = 0;
                const auto Clearing = G::Sample(Fields);
                Fields.Flora = 1;
                const auto Dense = G::Sample(Fields);
                TestEqual(TEXT("Local Flora cannot fragment any regional biome"), Clearing.Biome, Dense.Biome);
                TestEqual(TEXT("Local Flora does not reshape terrain"), Clearing.Height, Dense.Height);
                const auto East = G::Sample(C, P + FVector2D(0.1, 0));
                MaxJump = FMath::Max(MaxJump, double(FMath::Abs(East.Height - R.Height)));
                for (int32 I = 0; I < 7; ++I) MaxWeightJump = FMath::Max(MaxWeightJump, double(FMath::Abs(East.Weights[I] - R.Weights[I])));
                TestEqual(TEXT("Same identity reproduces shaped height"), G::Sample(C, P).Height, R.Height);
            }
        }
        for (int32 I = 0; I < 7; ++I) TestTrue(FString::Printf(TEXT("Seed %llu contains biome %d"), Seed, I), Counts[I] > 5);
        TestTrue(TEXT("Large area contains rivers and streams"), RiverCount > 10 && StreamCount > 10);
        TestTrue(TEXT("Sub-centimetre terrain samples are continuous"), MaxJump < 2);
        TestTrue(TEXT("Biome blends are continuous"), MaxWeightJump < 0.005);
        const double BoundaryDensity = double(Edges) / (2 * Side * (Side - 1));
        TestTrue(TEXT("Biome boundaries do not return to confetti"), BoundaryDensity < 0.18);
        TArray<bool> Visited; Visited.Init(false, Biomes.Num());
        TArray<int32> Areas;
        int32 Small = 0;
        for (int32 Start = 0; Start < Biomes.Num(); ++Start)
        {
            if (Visited[Start]) continue;
            TArray<int32> Queue{Start}; Visited[Start] = true;
            for (int32 Q = 0; Q < Queue.Num(); ++Q)
            {
                const int32 Index = Queue[Q], X = Index % Side, Y = Index / Side;
                const int32 Neighbours[] = { X > 0 ? Index - 1 : -1, X + 1 < Side ? Index + 1 : -1, Y > 0 ? Index - Side : -1, Y + 1 < Side ? Index + Side : -1 };
                for (int32 N : Neighbours) if (N >= 0 && !Visited[N] && Biomes[N] == Biomes[Start]) { Visited[N] = true; Queue.Add(N); }
            }
            Areas.Add(Queue.Num());
            Small += Queue.Num() <= 2;
        }
        Areas.Sort();
        TestTrue(TEXT("Tiny isolated components remain bounded"), Small < 250);
        AddInfo(FString::Printf(TEXT("Regional seed=%llu coverage=%d,%d,%d,%d,%d,%d,%d boundaries=%.4f components=%d medianCells=%d tiny=%d rivers=%d streams=%d maxHeightJump=%.5f"),
            Seed, Counts[0], Counts[1], Counts[2], Counts[3], Counts[4], Counts[5], Counts[6], BoundaryDensity, Areas.Num(), Areas[Areas.Num()/2], Small, RiverCount, StreamCount, MaxJump));
        if (!Previous.IsEmpty()) TestTrue(TEXT("Different seeds meaningfully move regions"), Different > Biomes.Num() / 4);
        Previous = Biomes;

        // The spatial index must return the same curves regardless of cache/query order.
        const FIntPoint Index(int32(FMath::FloorToInt(RiverFixture.X / FKalmalaRegionalTuning::GridCell)), int32(FMath::FloorToInt(RiverFixture.Y / FKalmalaRegionalTuning::GridCell)));
        const auto A = G::GetHydrology(C, Index);
        TestTrue(TEXT("Spline index comparison is nonempty"), !A.IsEmpty());
        G::ClearHydrologyCache();
        G::GetHydrology(C, Index + FIntPoint(1, 0));
        const auto B = G::GetHydrology(C, Index);
        TestEqual(TEXT("Spline rebuild count"), A.Num(), B.Num());
        for (int32 I = 0; I < FMath::Min(A.Num(), B.Num()); ++I)
        {
            TestEqual(TEXT("Spline identity"), A[I].Id, B[I].Id);
            TestEqual(TEXT("Spline start"), A[I].A, B[I].A);
            TestEqual(TEXT("Spline end"), A[I].B, B[I].B);
            TestTrue(TEXT("No broken spline step"), FVector::Dist(A[I].A, A[I].B) < 500);
        }
        for (int32 I = -4; I < 4; ++I)
        {
            const FVector2D P(I * FKalmalaRegionalTuning::GridCell, 7000);
            const auto West = G::Sample(C, P - FVector2D(0.01, 0));
            const auto East = G::Sample(C, P + FVector2D(0.01, 0));
            TestTrue(TEXT("Hydrology GridCell seam is continuous"), FMath::Abs(West.Height - East.Height) < 0.5);
        }
        // Verify the depth query uses both shaped collision-triangle planes.
        for (const FVector2D Origin : { FVector2D(-3000, -6000), FVector2D(3000, 6000) })
        for (const bool Upper : { false, true })
        {
            const FVector2D P = Origin + FVector2D(Upper ? 250.0 / 3 : 125.0 / 3);
            const float H = (G::Sample(C, Origin + FVector2D(125, 0)).Height
                + G::Sample(C, Origin + FVector2D(0, 125)).Height
                + G::Sample(C, Origin + (Upper ? FVector2D(125) : FVector2D::ZeroVector)).Height) / 3;
            const auto Ocean = FKalmalaOceanSampler::Sample(C, P, Origin);
            TestTrue(TEXT("Depth query uses shaped collision triangles"), FMath::IsNearlyEqual(Ocean.TerrainHeight, H, 0.01f));
            TestEqual(TEXT("Equivalent patch origins preserve shaped depth"), Ocean.TerrainHeight, FKalmalaOceanSampler::Sample(C, P, Origin + FVector2D(3000, 0)).TerrainHeight);
        }
        TestTrue(TEXT("Water seam fixture exists"), bWaterFixture);
        if (bWaterFixture)
        {
            const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(C, WaterFixture, true);
            const auto Right = FKalmalaWaterSurfaceMesh::BuildPatch(C, WaterFixture + FVector2D(3000, 0), true);
            int32 Matches = 0;
            for (const FVector& V : Left.Vertices) if (FMath::IsNearlyEqual(V.X, 1500.0, 0.01))
            {
                TestTrue(TEXT("Adjacent water patches share exact physical edge"), Right.Vertices.ContainsByPredicate([&](const FVector& W) { return (V - FVector(3000, 0, 0)).Equals(W, 0.01); }));
                ++Matches;
            }
            AddInfo(FString::Printf(TEXT("Water patch seam vertices=%d"), Matches));
            TestTrue(TEXT("Water seam comparison is nonempty"), Matches > 0);
        }
    }
    return true;
}
#endif
