#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "KalmalaGameMode.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeComparisonFixtureTest,
    "Kalmala.Gameplay.World.BiomeComparisonFixtureContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaBiomeComparisonFixtureTest::RunTest(const FString& Parameters)
{
    static const TCHAR* SampleNames[] = {
        TEXT("Ground"), TEXT("Rock"), TEXT("Wood"), TEXT("Canopy"), TEXT("Creature"), TEXT("Construction")
    };
    const auto MakeFixtureNames = [](const TCHAR* Site)
    {
        TArray<FString> Names;
        for (const TCHAR* Sample : SampleNames)
        {
            Names.Add(FString::Printf(TEXT("L03Smooth_%s_%s"), Site, Sample));
            Names.Add(FString::Printf(TEXT("L03Band_%s_%s"), Site, Sample));
        }
        return Names;
    };

    const TArray<FString> CompleteForest = MakeFixtureNames(TEXT("ForestShade"));
    TestTrue(TEXT("A complete site has all six exact smooth and banded identities"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), CompleteForest));

    TArray<FString> CompleteMap = CompleteForest;
    CompleteMap.Append(MakeFixtureNames(TEXT("MeadowsDaylight")));
    CompleteMap.Append(MakeFixtureNames(TEXT("OpenWater")));
    CompleteMap.Append(MakeFixtureNames(TEXT("OvercastRain")));
    TestTrue(TEXT("The complete four-site lighting map satisfies the fixture manifest"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), CompleteMap));
    TestFalse(TEXT("Complete pairs from another site cannot satisfy the requested site"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"),
            MakeFixtureNames(TEXT("OpenWater"))));

    TArray<FString> MissingPair = CompleteForest;
    MissingPair.RemoveSingle(TEXT("L03Smooth_ForestShade_Rock"));
    MissingPair.RemoveSingle(TEXT("L03Band_ForestShade_Rock"));
    TestFalse(TEXT("A site missing one smooth/banded pair is rejected"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), MissingPair));

    TArray<FString> MisnamedPair = CompleteForest;
    MisnamedPair.RemoveSingle(TEXT("L03Smooth_ForestShade_Creature"));
    MisnamedPair.RemoveSingle(TEXT("L03Band_ForestShade_Creature"));
    MisnamedPair.Add(TEXT("L03Smooth_ForestShade_CreatureProxy"));
    MisnamedPair.Add(TEXT("L03Band_ForestShade_CreatureProxy"));
    TestFalse(TEXT("A misnamed pair cannot satisfy the exact site manifest"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), MisnamedPair));

    TArray<FString> UnknownVariant = CompleteForest;
    UnknownVariant.Add(TEXT("L03Toon_ForestShade_Ground"));
    TestFalse(TEXT("A tagged actor with an unrecognized variant prefix is rejected"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), UnknownVariant));

    TArray<FString> UnknownClass = CompleteForest;
    UnknownClass.Add(TEXT("L03Smooth_ForestShade_Metal"));
    TestFalse(TEXT("A tagged actor with an unrecognized sample identity is rejected"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), UnknownClass));

    TArray<FString> DuplicateIdentity = CompleteForest;
    DuplicateIdentity.Add(TEXT("L03Smooth_ForestShade_Ground"));
    TestFalse(TEXT("Duplicate tagged identities are rejected"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("ForestShade"), DuplicateIdentity));

    TestTrue(TEXT("FireDusk resolves to its shared MeadowsDaylight board"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("FireDusk"), CompleteMap));
    TestFalse(TEXT("An unsupported requested site is rejected"),
        AKalmalaGameMode::HasCompleteBiomeBaselineComparisonFixture(TEXT("UnknownSite"), CompleteForest));
    return true;
}

#endif
