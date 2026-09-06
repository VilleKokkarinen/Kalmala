#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerMovementTest, "Kalmala.Gameplay.Movement.SprintSavedMoves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlayerMovementTest::RunTest(const FString& Parameters)
{
    FSavedMove_Kalmala Move;
    Move.Clear();
    Move.bSavedSprint = true;
    Move.bPressedJump = true;
    const uint8 Flags = Move.GetCompressedFlags();
    TestTrue(TEXT("Sprint intent is encoded in the custom move flag"), (Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
    TestTrue(TEXT("Jump retains its built-in network flag alongside sprint"), (Flags & FSavedMove_Character::FLAG_JumpPressed) != 0);
    auto* Movement = NewObject<UKalmalaCharacterMovementComponent>();
    Movement->UpdateFromCompressedFlags(Flags);
    TestTrue(TEXT("Server movement decodes sprint intent"), Movement->IsSprintRequested());
    Movement->UpdateFromCompressedFlags(0);
    TestFalse(TEXT("Release is decoded from the next move"), Movement->IsSprintRequested());
    FSavedMovePtr Released(new FSavedMove_Kalmala());
    Released->Clear();
    TestFalse(TEXT("Sprint and release moves cannot be combined and lose the transition"), Move.CanCombineWith(Released, nullptr, 0.125f));
    Move.Clear();
    TestFalse(TEXT("Recycled saved moves cannot keep a stale sprint flag"), Move.bSavedSprint);
    TestEqual(TEXT("Generated ocean swim uses a dedicated custom movement mode"), UKalmalaCharacterMovementComponent::GeneratedOceanSwimmingMode, static_cast<uint8>(1));
    TestFalse(TEXT("Shallow shore does not enter swimming"), UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(99.9f));
    TestTrue(TEXT("Deep shared sea enters swimming"), UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(100.0f));
    TestFalse(TEXT("Shore hysteresis retains swimming at 75 cm"), UKalmalaCharacterMovementComponent::ShouldReturnToLand(75.0f));
    TestTrue(TEXT("Shore hysteresis returns to land below 75 cm"), UKalmalaCharacterMovementComponent::ShouldReturnToLand(74.9f));
    return true;
}
#endif
