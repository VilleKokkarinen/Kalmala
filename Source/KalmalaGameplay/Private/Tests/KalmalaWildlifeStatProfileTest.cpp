#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeStatProfileTest,
    "Kalmala.Gameplay.Wildlife.Stats.DeathAdder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeStatProfileTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Death Adder health matches the reviewed profile"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::DeathAdder), 125.0f);
    TestEqual(TEXT("Death Adder armor matches the reviewed profile"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::DeathAdder), 20.0f);

    const FKalmalaDamageChannels Attack =
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::DeathAdder);
    TestEqual(TEXT("Death Adder uses reviewed Stab damage"), Attack.Stab, 75.0f);
    TestEqual(TEXT("Death Adder leaves Crush inactive"), Attack.Crush, 0.0f);
    TestEqual(TEXT("Death Adder attack total matches its profile"),
        AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype::DeathAdder), 75.0f);
    return true;
}
#endif
