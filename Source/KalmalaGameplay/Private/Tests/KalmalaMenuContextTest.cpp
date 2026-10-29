#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMenuContext.h"
#include "KalmalaGameMode.h"
#include "KalmalaCharacter.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMenuContextTest, "Kalmala.Gameplay.MenuContext",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaMenuContextTest::RunTest(const FString& Parameters)
{
    AKalmalaMenuGameMode* Menu = GetMutableDefault<AKalmalaMenuGameMode>();
    const AKalmalaGameMode* Gameplay = GetDefault<AKalmalaGameMode>();
    TestTrue(TEXT("Menu uses its distinct controller"), Menu->PlayerControllerClass == AKalmalaMenuPlayerController::StaticClass());
    TestTrue(TEXT("Menu has no pawn, spectator or HUD class"),
        Menu->DefaultPawnClass == nullptr && Menu->SpectatorClass == nullptr && Menu->HUDClass == nullptr);
    TestTrue(TEXT("Both menu default spawn paths refuse a pawn"),
        Menu->SpawnDefaultPawnFor_Implementation(nullptr, nullptr) == nullptr
        && Menu->SpawnDefaultPawnAtTransform_Implementation(nullptr, FTransform::Identity) == nullptr);
    TestTrue(TEXT("Gameplay retains its character and is not a menu context"),
        Gameplay->DefaultPawnClass == AKalmalaCharacter::StaticClass()
        && Gameplay->PlayerControllerClass != AKalmalaMenuPlayerController::StaticClass());
    return true;
}
#endif
