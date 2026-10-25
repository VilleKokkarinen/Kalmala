#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaForestLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestFactorPatchinessTest,
    "Kalmala.World.ForestFactor.PatchinessAndDensity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestFactorPatchinessTest::RunTest(const FString& Parameters)
{
    const float FloraProbability = 0.64f;
    const float LowFactorProbability = FKalmalaForestLayout::CombineForestAcceptanceProbability(
        FloraProbability, 0.1f);
    const float HighFactorProbability = FKalmalaForestLayout::CombineForestAcceptanceProbability(
        FloraProbability, 0.9f);
    TestEqual(TEXT("Zero forest factor creates a complete clearing"),
        FKalmalaForestLayout::CombineForestAcceptanceProbability(FloraProbability, 0.0f), 0.0f);
    TestEqual(TEXT("Forest factor one preserves the existing Flora probability"),
        FKalmalaForestLayout::CombineForestAcceptanceProbability(FloraProbability, 1.0f), FloraProbability);
    TestTrue(TEXT("Lower forest factor lowers forest admission probability"),
        LowFactorProbability < HighFactorProbability);
    TestTrue(TEXT("Combined probability never exceeds Flora suitability"),
        HighFactorProbability <= FloraProbability);
    const float TundraProbability = FKalmalaForestLayout::CombineTundraAcceptanceProbability(
        FloraProbability, 0.9f);
    TestEqual(TEXT("Tundra reuses Flora and forest factor with the provisional 0.85 density multiplier"),
        TundraProbability, FloraProbability * 0.9f * 0.85f);
    TestTrue(TEXT("Tundra remains below comparable Elderwood acceptance at full factor"),
        FKalmalaForestLayout::CombineTundraAcceptanceProbability(FloraProbability, 1.0f)
            < FloraProbability);

    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D PatchCenter(123000.0, -75000.0);
    const double Threshold = FKalmalaForestLayout::SampleForestCandidateThreshold(Config, PatchCenter, 7);
    TestTrue(TEXT("Candidate threshold is a unit interval sample"), Threshold >= 0.0 && Threshold < 1.0);
    TestEqual(TEXT("Same world, patch and original ordinal reproduce the threshold"),
        FKalmalaForestLayout::SampleForestCandidateThreshold(Config, PatchCenter, 7), Threshold);
    TestTrue(TEXT("Different original ordinal has an independent threshold"),
        FMath::Abs(FKalmalaForestLayout::SampleForestCandidateThreshold(Config, PatchCenter, 8) - Threshold) > 1.0e-6);
    TestTrue(TEXT("Adjacent patch has an independent threshold"),
        FMath::Abs(FKalmalaForestLayout::SampleForestCandidateThreshold(
            Config, PatchCenter + FVector2D(3000.0, 0.0), 7) - Threshold) > 1.0e-6);
    TestEqual(TEXT("Threshold identity normalizes signed zero like the persistent ID"),
        FKalmalaForestLayout::SampleForestCandidateThreshold(Config, FVector2D(-0.0, 0.0), 7),
        FKalmalaForestLayout::SampleForestCandidateThreshold(Config, FVector2D(0.0, 0.0), 7));

    const FVector2D SeamLeft(1499.5, 8700.0);
    const FVector2D SeamRight(1500.5, 8700.0);
    const float LeftFactor = FKalmalaForestLayout::SampleForestFactor(Config, SeamLeft);
    const float RightFactor = FKalmalaForestLayout::SampleForestFactor(Config, SeamRight);
    TestTrue(TEXT("Forest factor remains normalized"),
        LeftFactor >= 0.0f && LeftFactor <= 1.0f && RightFactor >= 0.0f && RightFactor <= 1.0f);
    TestTrue(TEXT("Forest factor remains continuous across a 30 m patch boundary"),
        FMath::Abs(LeftFactor - RightFactor) < 0.01f);
    TestEqual(TEXT("Absolute-world forest factor reproduces exactly"),
        FKalmalaForestLayout::SampleForestFactor(Config, SeamLeft), LeftFactor);
    return true;
}

#endif
