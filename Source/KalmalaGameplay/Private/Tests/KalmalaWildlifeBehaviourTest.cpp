#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeBehaviourTest, "Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeBehaviourTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot select wildlife behaviour"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(false, false, EKalmalaWildlifeBehaviour::Idle, EKalmalaWildlifeBehaviour::Flee));
    TestFalse(TEXT("Defeated wildlife cannot resume behaviour"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, true, EKalmalaWildlifeBehaviour::Return, EKalmalaWildlifeBehaviour::Idle));
    TestTrue(TEXT("Server damage can enter bounded flee"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Idle, EKalmalaWildlifeBehaviour::Flee));
    TestTrue(TEXT("Server flee progresses to investigate"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Flee, EKalmalaWildlifeBehaviour::Investigate));
    TestTrue(TEXT("Server investigate progresses to return"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Investigate, EKalmalaWildlifeBehaviour::Return));
    TestTrue(TEXT("Server return settles at idle"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Return, EKalmalaWildlifeBehaviour::Idle));
    TestFalse(TEXT("The cycle cannot skip directly from flee to idle"), AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(true, false, EKalmalaWildlifeBehaviour::Flee, EKalmalaWildlifeBehaviour::Idle));
    TestEqual(TEXT("Archetype selection stays deterministic for one descriptor seed"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(6)), uint8(EKalmalaWildlifeArchetype::Boar));
    TestEqual(TEXT("Existing Mireling descriptor seed stays a Mireling"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(3308806006119996599ull)), uint8(EKalmalaWildlifeArchetype::Mireling));
    TestEqual(TEXT("A non-boar descriptor deterministically selects deer"), uint8(AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(1)), uint8(EKalmalaWildlifeArchetype::Deer));
    TestFalse(TEXT("A client cannot begin a territorial charge"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(false, false, true, 100.0f));
    TestFalse(TEXT("A defeated boar cannot begin a territorial charge"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, true, true, 100.0f));
    TestFalse(TEXT("A distant player cannot provoke the resting area"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, false, true, 501.0f));
    TestTrue(TEXT("A server-confirmed nearby player can provoke the resting area"), AKalmalaWildlifeSpawn::IsBoarChargeAllowed(true, false, true, 500.0f));
    return true;
}
#endif
