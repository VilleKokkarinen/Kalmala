#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMenuContext.h"
#include "KalmalaGameMode.h"
#include "KalmalaCharacter.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMenuContextTest, "Kalmala.Gameplay.MenuContext.WorldSeedSelection",
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
    TestTrue(TEXT("The authoritative local menu controller may process a seed selection"),
        AKalmalaMenuPlayerController::CanProcessWorldSeedSelectionRequestForTests(true, true, true));
    TestFalse(TEXT("A remote controller cannot process a seed selection"),
        AKalmalaMenuPlayerController::CanProcessWorldSeedSelectionRequestForTests(true, false, true));
    TestFalse(TEXT("A non-authority controller cannot process a seed selection"),
        AKalmalaMenuPlayerController::CanProcessWorldSeedSelectionRequestForTests(false, true, true));
    TestFalse(TEXT("A controller without a world cannot process a seed selection"),
        AKalmalaMenuPlayerController::CanProcessWorldSeedSelectionRequestForTests(true, true, false));
    TestTrue(TEXT("Gameplay retains its character and is not a menu context"),
        Gameplay->DefaultPawnClass == AKalmalaCharacter::StaticClass()
        && Gameplay->PlayerControllerClass != AKalmalaMenuPlayerController::StaticClass());
    return true;
}
#endif
