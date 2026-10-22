#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDodgeRollContractTest,
    "Kalmala.Gameplay.Movement.DodgeRollAuthorityAndCost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDodgeRollContractTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("A valid living grounded server pawn can request a roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, false, false,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Clients cannot authorize a roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(false, true, true, false, false,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Defeated pawns cannot roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, false, true, false, false,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Airborne pawns cannot start a roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, false, false, false,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Attached skiff occupants cannot roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, true, false,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("An active roll cannot be replayed"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, false, true,
            0.0f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Cooldown prevents repeat rolls"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, false, false,
            0.01f, 100.0f, AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Insufficient stamina rejects the roll"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, false, false,
            0.0f, AKalmalaCharacter::DodgeRollStaminaCost - 0.1f,
            AKalmalaCharacter::DodgeRollStaminaCost));
    TestFalse(TEXT("Malformed stamina fails closed"),
        AKalmalaCharacter::IsDodgeRollRequestAllowed(true, true, true, false, false,
            0.0f, std::numeric_limits<float>::quiet_NaN(), AKalmalaCharacter::DodgeRollStaminaCost));
    TestEqual(TEXT("The accepted roll briefly breaks swarm contact"),
        AKalmalaCharacter::DodgeRollMosquitoRepelSeconds, 2.0f);
    return true;
}

#endif
