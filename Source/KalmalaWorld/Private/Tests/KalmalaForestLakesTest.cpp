#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaForestLayout.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldFieldSampler.h"
#include "Misc/AutomationTest.h"

namespace
{
    bool FindLakesForestPatch(const FKalmalaWorldGenerationConfig& Config,
        FVector2D& OutCenter, TArray<FKalmalaForestTreeDescriptor>& OutTrees)
    {
        constexpr int32 AngleSteps = 72;
        constexpr double MinimumRadius = 40000.0;
        constexpr double MaximumRadius = 300000.0;
        constexpr double RadiusStep = 5000.0;
        for (double Radius = MinimumRadius; Radius <= MaximumRadius; Radius += RadiusStep)
        {
            for (int32 AngleIndex = 0; AngleIndex < AngleSteps; ++AngleIndex)
            {
                const double Angle = 2.0 * PI * AngleIndex / AngleSteps;
                const FVector2D Center(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
                if (FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, Center)) != EKalmalaBiome::ShimmeringLakes)
                {
                    continue;
                }
                TArray<FKalmalaForestTreeDescriptor> LakesTrees;
                for (const FKalmalaForestTreeDescriptor& Tree : FKalmalaForestLayout::Generate(Config, Center))
                {
                    if (FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Tree.Location)))
                        == EKalmalaBiome::ShimmeringLakes)
                    {
                        LakesTrees.Add(Tree);
                    }
                }
                if (LakesTrees.IsEmpty()) continue;
                OutCenter = Center;
                OutTrees = MoveTemp(LakesTrees);
                return true;
            }
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestLakesShouldersTest,
    "Kalmala.World.Forest.ShimmeringLakesDryShoulders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestLakesShouldersTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{418ull};
    FVector2D PatchCenter = FVector2D::ZeroVector;
    TArray<FKalmalaForestTreeDescriptor> LakesTrees;
    if (!TestTrue(TEXT("Fixture finds a dry Shimmering Lakes Birch descriptor"),
        FindLakesForestPatch(Config, PatchCenter, LakesTrees))) return false;
    TestTrue(TEXT("Lakes tree count stays within the existing 18-candidate cap"), LakesTrees.Num() <= 18);

    constexpr float HalfPatchSize = FKalmalaTerrainPatchLayout::PatchSize * 0.5f;
    const FVector2D GridOrigin = PatchCenter - FVector2D(HalfPatchSize, HalfPatchSize);
    for (const FKalmalaForestTreeDescriptor& Tree : LakesTrees)
    {
        const FVector2D Position(Tree.Location);
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
        TestEqual(TEXT("Lakes forest accepts only Shimmering Lakes classified candidates"),
            FKalmalaBiomeClassifier::Classify(Fields), EKalmalaBiome::ShimmeringLakes);
        TestTrue(TEXT("Lakes trees reuse the established Birch kind"),
            Tree.TreeKind == EKalmalaForestTreeKind::MeadowsBirch);
        TestEqual(TEXT("Lakes descriptor keeps its original stable candidate ID"), Tree.PersistentSpawnId,
            FKalmalaForestLayout::GetPersistentSpawnId(Config, PatchCenter, Tree.CandidateIndex));
        TestTrue(TEXT("Lakes candidates use only the existing lowland ordinals"),
            Tree.CandidateIndex >= 0 && Tree.CandidateIndex < 18);
        const float FloraProbability = FMath::Clamp((Fields.Flora - 0.60f) / 0.24f, 0.20f, 0.92f);
        TestEqual(TEXT("Lakes descriptor retains the sampled absolute-world forest factor"), Tree.ForestFactor,
            FKalmalaForestLayout::SampleForestFactor(Config, Position));
        TestTrue(TEXT("Lakes descriptor passes the stable Flora/factor candidate threshold"),
            FKalmalaForestLayout::SampleForestCandidateThreshold(Config, PatchCenter, Tree.CandidateIndex)
                <= FKalmalaForestLayout::CombineForestAcceptanceProbability(FloraProbability, Tree.ForestFactor));

        const FKalmalaRegionalSample Regional = FKalmalaRegionalGeneration::Sample(Config, Position);
        TestEqual(TEXT("Lakes root uses the shared final regional surface"), Tree.Location.Z, Regional.Height);
        FKalmalaOceanSample Ocean;
        if (!Regional.bHasWater) Ocean = FKalmalaOceanSampler::Sample(Config, Position, GridOrigin);
        TestTrue(TEXT("Final regional and collision-lattice samples prove dry ground"),
            FKalmalaForestLayout::IsForestGroundDry(Regional.bHasWater, Ocean.bIsValid, Ocean.IsWater()));
        TestTrue(TEXT("Final terrain slope is within the shared forest limit"),
            FKalmalaForestLayout::IsForestSurfaceSlopeSuitable(
                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z));
    }

    const TArray<FKalmalaForestTreeDescriptor> RepeatedTrees = FKalmalaForestLayout::Generate(Config, PatchCenter);
    int32 RepeatedLakesCount = 0;
    for (const FKalmalaForestTreeDescriptor& Tree : RepeatedTrees)
    {
        if (FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Tree.Location))) != EKalmalaBiome::ShimmeringLakes)
        {
            continue;
        }
        TestTrue(TEXT("Repeated Lakes generation retains original candidate IDs"),
            LakesTrees.ContainsByPredicate([&Tree](const FKalmalaForestTreeDescriptor& Existing)
            {
                return Existing.CandidateIndex == Tree.CandidateIndex
                    && Existing.PersistentSpawnId == Tree.PersistentSpawnId;
            }));
        ++RepeatedLakesCount;
    }
    TestEqual(TEXT("Repeated generation preserves the Lakes descriptor count"), RepeatedLakesCount, LakesTrees.Num());
    return true;
}

#endif
