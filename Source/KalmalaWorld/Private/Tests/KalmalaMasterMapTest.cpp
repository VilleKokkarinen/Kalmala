#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMasterMap.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaGenerationPreview.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMasterMapTest, "Kalmala.World.Regional.MasterMap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaMasterMapTest::RunTest(const FString& Parameters)
{
    using M = FKalmalaMasterMap;
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig A{418}, B{419};
    const auto Before = FKalmalaGenerationPreview::Get();
    auto Forged = Before;
    Forged.MasterSeed = 9;
    TestFalse(TEXT("Interactive editor/game cannot install preview tuning"), FKalmalaGenerationPreview::Set(Forged));
    TestEqual(TEXT("Rejected preview tuning preserves master seed"), FKalmalaGenerationPreview::Get().MasterSeed, Before.MasterSeed);
    const auto Crop = M::Crop(A), Repeat = M::Crop(A), Other = M::Crop(B);
    TestTrue(TEXT("Same identity repeats crop"), Crop.Center == Repeat.Center && Crop.Rotation == Repeat.Rotation);
    TestTrue(TEXT("Game seed changes crop and rotation"), Crop.Center != Other.Center && Crop.Rotation != Other.Rotation);
    TestFalse(TEXT("Production master maps omit streams"), G::AreStreamsEnabled(A));
    int32 MaskVariation = 0, MasterVariation = 0, Counts[7] = {};
    double MaxJump = 0;
    for (const auto C : {A, B})
    {
        const auto Transform = M::Crop(C);
        for (int32 I = 0; I < 768; ++I)
        {
            const double Angle = I * 2.399963229728653;
            const FVector2D P = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle))
                * (FKalmalaWorldBounds::Radius * FMath::Sqrt((I + 0.5) / 768.0));
            const FVector2D Atlas = M::ToMasterPosition(Transform, P);
            TestTrue(TEXT("Rotated crop fits master atlas"), FMath::Abs(Atlas.X) <= M::HalfExtent && FMath::Abs(Atlas.Y) <= M::HalfExtent);
            TestTrue(TEXT("Rotation preserves scale"), FMath::IsNearlyEqual((Atlas - Transform.Center).Size(), P.Size(), 0.001));
            const double Mask = M::Sample(C, P);
            TestEqual(TEXT("Game mask samples the independent master map"), Mask, M::SampleMaster(Atlas));
            MaskVariation += (M::Sample(A, P) > 0) != (M::Sample(B, P) > 0);
            MasterVariation += (M::SampleMaster(Atlas, M::MasterSeed + 1) > 0) != (Mask > 0);
            const auto R = G::Sample(C, P);
            TestEqual(TEXT("Fast biome sampler agrees with full terrain/hydrology"), G::SampleBiome(FKalmalaWorldFieldSampler::Sample(C, P)), R.Biome);
            ++Counts[R.Biome];
            TestEqual(TEXT("Production has no stream carving"), R.StreamWeight, 0.f);
            TestEqual(TEXT("Atlas alone determines Ocean label"), R.Biome == 6, Mask <= 0);
            TestTrue(TEXT("Terrain respects atlas sea-level sign"), Mask > 0 ? R.Height > 0 : R.Height <= 0);
            TestTrue(TEXT("Meadows stop at 4 km"), R.Biome != 0 || P.Size() <= T::MeadowsMaximum);
            TestTrue(TEXT("Lakes respect starter exclusion"), R.Biome != 1 || (P.Size() >= T::LakesMinimum && P.Size() <= T::LakesMaximum));
            TestTrue(TEXT("Forest respects 0.75 km minimum"), R.Biome != 2 || P.Size() >= T::ElderwoodMinimum);
            TestTrue(TEXT("Mire respects 2 km minimum"), R.Biome != 3 || (P.Size() >= T::MireMinimum && P.Size() <= T::MireMaximum));
            TestTrue(TEXT("Tundra respects 4 km minimum"), R.Biome != 4 || P.Size() >= T::TundraMinimum);
            float Sum = 0;
            for (float W : R.Weights) { TestTrue(TEXT("Finite bounded weights"), FMath::IsFinite(W) && W >= 0 && W <= 1); Sum += W; }
            TestTrue(TEXT("Weights normalized"), FMath::IsNearlyEqual(Sum, 1.f, .0001f));
            if (I % 32 == 0)
            {
                const auto Again = G::Sample(C, P);
                TestEqual(TEXT("Same seed reproduces height"), R.Height, Again.Height);
                for (int32 W = 0; W < 7; ++W) TestEqual(TEXT("Same seed reproduces weights"), R.Weights[W], Again.Weights[W]);
                MaxJump = FMath::Max(MaxJump, double(FMath::Abs(R.Height - G::Sample(C, P + FVector2D(.1, 0)).Height)));
            }
        }
        // Exact thresholds and both sides in multiple directions, including
        // synthetic peaks to prove mountain precedence cannot invade the start.
        for (double Radius : {0.0, 34999.9, 35000.0, 35000.1, 74999.9, 75000.0, 75000.1,
            299999.9, 300000.0, 300000.1, 399999.9, 400000.0, 400000.1})
        for (int32 I = 0; I < 8; ++I)
        {
            const FVector2D P = FVector2D(FMath::Cos(I * PI / 4), FMath::Sin(I * PI / 4)) * Radius;
            auto F = FKalmalaWorldFieldSampler::Sample(C, P);
            F.Elevation = .9f;
            const auto R = G::Sample(F);
            if (Radius <= T::StarterRadius) TestEqual(TEXT("Starter peaks still Meadows"), R.Biome, uint8(0));
            if (Radius <= T::StarterRadius) TestEqual(TEXT("Starter has no lake influence"), R.BasinWeight, 0.f);
            if (Radius < T::ElderwoodMinimum) TestEqual(TEXT("No ineligible forest blend"), R.Weights[2], 0.f);
            if (Radius < T::MireMinimum) TestEqual(TEXT("No ineligible mire blend"), R.Weights[3], 0.f);
            if (Radius < T::TundraMinimum) TestEqual(TEXT("No ineligible tundra blend"), R.Weights[4], 0.f);
            if (Radius > T::MeadowsMaximum) TestEqual(TEXT("No Meadows beyond limit"), R.Weights[0], 0.f);
            const auto Left = G::Sample(C, P - P.GetSafeNormal() * .1);
            const auto Right = G::Sample(C, P + P.GetSafeNormal() * .1);
            TestTrue(TEXT("Distance gates do not step terrain"), FMath::Abs(Left.Height - Right.Height) < 2);
            for (int32 W = 0; W < 7; ++W)
                TestTrue(TEXT("Distance gates keep continuous weights"), FMath::Abs(Left.Weights[W] - Right.Weights[W]) < .005);
        }
        const FVector2D Start(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(C).GetLocation());
        const auto StartRegion = G::Sample(C, Start);
        TestTrue(TEXT("Start stays in protected radius"), Start.Size() <= T::StarterRadius);
        TestEqual(TEXT("Start is Meadows"), StartRegion.Biome, uint8(0));
        TestTrue(TEXT("Start is dry terrain"), !StartRegion.bHasWater && StartRegion.Height >= 40);
    }
    TestTrue(TEXT("Different crops change coastlines"), MaskVariation > 100);
    const FKalmalaWorldGenerationConfig DefaultWorld{10323456789ull};
    const FVector2D DefaultStart(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(DefaultWorld).GetLocation());
    const auto DefaultRegion = G::Sample(DefaultWorld, DefaultStart);
    TestTrue(TEXT("Normal launch has a dry central Meadows start"), DefaultStart.Size() <= T::StarterRadius
        && DefaultRegion.Biome == 0 && DefaultRegion.Height >= 40 && !DefaultRegion.bHasWater);
    TestTrue(TEXT("Independent master seed changes land"), MasterVariation > 100);
    TestTrue(TEXT("Terrain remains locally continuous"), MaxJump < 2);
    for (int32 I = 0; I < 7; ++I) AddInfo(FString::Printf(TEXT("Master map biome %d samples=%d"), I, Counts[I]));
    for (int32 I = 0; I < 7; ++I) TestTrue(TEXT("World-scale palette coverage"), Counts[I] > 0);
    return true;
}
#endif
