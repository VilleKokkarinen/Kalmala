#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaTerrainGroundBlend.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaTerrainGroundBlendTest,
    "Kalmala.World.TerrainGroundBlend.SlopeMoistureWeights",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaTerrainGroundBlendTest::RunTest(const FString& Parameters)
{
    const FLinearColor DryFlat = FKalmalaTerrainGroundBlend::SampleWeights(1.0f, 0.2f);
    TestEqual(TEXT("Dry flat ground is grass"), DryFlat.R, 1.0f);
    TestEqual(TEXT("Dry flat ground has no soil"), DryFlat.G, 0.0f);
    TestEqual(TEXT("Dry flat ground has no rock"), DryFlat.B, 0.0f);

    const FLinearColor MoistFlat = FKalmalaTerrainGroundBlend::SampleWeights(1.0f, 0.72f);
    TestEqual(TEXT("Moist flat ground is soil"), MoistFlat.G, 1.0f);
    TestEqual(TEXT("Moist ground reduces grass"), MoistFlat.R, 0.0f);

    const FLinearColor SteepGround = FKalmalaTerrainGroundBlend::SampleWeights(0.70f, 0.55f);
    TestEqual(TEXT("Steep terrain is exposed rock"), SteepGround.B, 1.0f);
    TestEqual(TEXT("Rock displaces grass on steep terrain"), SteepGround.R, 0.0f);

    const FLinearColor Mixed = FKalmalaTerrainGroundBlend::SampleWeights(0.90f, 0.56f);
    TestTrue(TEXT("Slope and moisture produce a normalized blend"),
        FMath::IsNearlyEqual(Mixed.R + Mixed.G + Mixed.B, 1.0f, 1.0e-6f));
    TestTrue(TEXT("Intermediate conditions retain grass, soil and rock"),
        Mixed.R > 0.0f && Mixed.G > 0.0f && Mixed.B > 0.0f);
    TestTrue(TEXT("Increasing moisture shifts flat ground from grass toward soil"),
        FKalmalaTerrainGroundBlend::SampleWeights(1.0f, 0.65f).G
            > FKalmalaTerrainGroundBlend::SampleWeights(1.0f, 0.45f).G);
    TestTrue(TEXT("Steeper final surfaces shift ground toward rock"),
        FKalmalaTerrainGroundBlend::SampleWeights(0.80f, 0.55f).B
            > FKalmalaTerrainGroundBlend::SampleWeights(0.96f, 0.55f).B);
    const FLinearColor Repeated = FKalmalaTerrainGroundBlend::SampleWeights(0.90f, 0.56f);
    TestEqual(TEXT("Repeated sample preserves grass weight"), Repeated.R, Mixed.R);
    TestEqual(TEXT("Repeated sample preserves soil weight"), Repeated.G, Mixed.G);
    TestEqual(TEXT("Repeated sample preserves rock weight"), Repeated.B, Mixed.B);
    return true;
}

#endif
