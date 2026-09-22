#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCombatComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCombatLoopTest, "Kalmala.Gameplay.Combat.BasicAttack.AuthorityAndCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCombatLoopTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Client role cannot begin an attack"), UKalmalaCombatComponent::IsAttackRequestAllowed(false, true, 1, true, true));
    TestFalse(TEXT("Zero sequence is rejected"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 0, true, true));
    TestFalse(TEXT("Repeated sequence is rejected"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, false, 2, true, true));
    TestFalse(TEXT("Busy attack cannot begin another action"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 3, false, true));
    TestFalse(TEXT("Server cannot accept a missing target"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 4, true, false));
    TestTrue(TEXT("Validated server intent begins one committed action"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 5, true, true));
    TestTrue(TEXT("Combat recovery remains a positive committed window"), UKalmalaCombatComponent::GetRecoverySeconds() > 0.0f);
    TestEqual(TEXT("M5 combat recovery uses the tuned 0.36-second window"), UKalmalaCombatComponent::GetRecoverySeconds(), 0.36f);
    TestFalse(TEXT("Clients cannot apply wildlife combat damage"), AKalmalaWildlifeSpawn::IsDefeatAllowed(false, false));
    TestNotEqual(TEXT("Hit feedback is distinguishable from defeat"), uint8(EKalmalaCombatFeedback::Hit), uint8(EKalmalaCombatFeedback::Defeat));
    TestNotEqual(TEXT("Unavailable feedback is distinguishable from hit"), uint8(EKalmalaCombatFeedback::Unavailable), uint8(EKalmalaCombatFeedback::Hit));
    return true;
}
#endif
