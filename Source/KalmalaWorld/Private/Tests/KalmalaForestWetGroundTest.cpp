#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaForestLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestWetGroundTest,
    "Kalmala.World.Forest.MeadowWetGroundAndIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaForestWetGroundTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Valid dry regional and ocean samples allow placement"),
        FKalmalaForestLayout::IsMeadowGroundDry(false, true, false));
    TestFalse(TEXT("Regional river or basin water rejects placement"),
        FKalmalaForestLayout::IsMeadowGroundDry(true, true, false));
    TestFalse(TEXT("Collision-lattice ocean water rejects placement"),
        FKalmalaForestLayout::IsMeadowGroundDry(false, true, true));
    TestFalse(TEXT("An invalid final ocean sample cannot prove ground is dry"),
        FKalmalaForestLayout::IsMeadowGroundDry(false, false, false));

    const FKalmalaWorldGenerationConfig Config{418ull};
    const FVector2D PatchCenter(3000.0, -6000.0);
    const FString CandidateId = FKalmalaForestLayout::GetPersistentSpawnId(Config, PatchCenter, 7);
    TestFalse(TEXT("Valid candidate has a persistent identity"), CandidateId.IsEmpty());
    TestEqual(TEXT("Filtering preserves the original candidate identity"), CandidateId,
        FKalmalaForestLayout::GetPersistentSpawnId(Config, PatchCenter, 7));
    TestNotEqual(TEXT("Distinct original ordinals keep distinct identities"), CandidateId,
        FKalmalaForestLayout::GetPersistentSpawnId(Config, PatchCenter, 8));
    return true;
}
#endif
