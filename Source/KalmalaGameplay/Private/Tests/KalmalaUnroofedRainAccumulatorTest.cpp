#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaUnroofedRainAccumulatorTest,
    "Kalmala.Gameplay.Exposure.RainAccumulatorCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaUnroofedRainAccumulatorTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Rain accumulator fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Rain accumulator fixture GameMode created"), Mode)) return false;

    const auto SpawnCharacter = [World]()
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        return World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
            FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    };

    APlayerController* LiveController = World->SpawnActor<APlayerController>();
    AKalmalaCharacter* LiveCharacter = SpawnCharacter();
    if (!TestTrue(TEXT("Live player fixture is fully possessed"), IsValid(LiveController) && IsValid(LiveCharacter)))
    {
        return false;
    }
    World->AddController(LiveController);
    LiveController->Possess(LiveCharacter);
    if (!TestTrue(TEXT("Live player possession succeeded"), LiveCharacter->GetController() == LiveController))
    {
        return false;
    }

    const TWeakObjectPtr<AKalmalaCharacter> LiveCharacterKey(LiveCharacter);
    constexpr float LiveAccumulatedRainSeconds = 4.0f;
    Mode->UnroofedRainSecondsByCharacter.Add(LiveCharacterKey, LiveAccumulatedRainSeconds);

    bool bAllDisconnectedKeysWereInvalidated = true;
    bool bMapStayedBounded = true;
    constexpr int32 DisconnectCycles = 32;
    for (int32 Cycle = 0; Cycle < DisconnectCycles; ++Cycle)
    {
        APlayerController* DisconnectedController = World->SpawnActor<APlayerController>();
        AKalmalaCharacter* DisconnectedCharacter = SpawnCharacter();
        if (!IsValid(DisconnectedController) || !IsValid(DisconnectedCharacter))
        {
            AddError(FString::Printf(TEXT("Disconnect fixture %d could not be spawned"), Cycle));
            return false;
        }

        World->AddController(DisconnectedController);
        DisconnectedController->Possess(DisconnectedCharacter);
        const TWeakObjectPtr<AKalmalaCharacter> DisconnectedCharacterKey(DisconnectedCharacter);
        Mode->UnroofedRainSecondsByCharacter.Add(DisconnectedCharacterKey, static_cast<float>(Cycle + 1));

        DisconnectedController->UnPossess();
        DisconnectedCharacter->Destroy();
        DisconnectedController->Destroy();
        Mode->UpdatePlayerExposure(1.0f);

        bAllDisconnectedKeysWereInvalidated &= !DisconnectedCharacterKey.IsValid();
        bMapStayedBounded &= Mode->UnroofedRainSecondsByCharacter.Num() == 1;
    }

    TestTrue(TEXT("Repeated disconnected pawns leave expired weak keys"), bAllDisconnectedKeysWereInvalidated);
    TestTrue(TEXT("Pruning each disconnected pawn keeps the accumulator map bounded"), bMapStayedBounded);
    TestTrue(TEXT("The live character's accumulator remains present"),
        Mode->UnroofedRainSecondsByCharacter.Contains(LiveCharacterKey));
    const float* PreservedLiveSeconds = Mode->UnroofedRainSecondsByCharacter.Find(LiveCharacterKey);
    TestNotNull(TEXT("The live character's accumulated rain time remains readable"), PreservedLiveSeconds);
    if (PreservedLiveSeconds != nullptr)
    {
        TestTrue(TEXT("Pruning leaves live-character Wet accumulation unchanged"),
            FMath::IsNearlyEqual(*PreservedLiveSeconds, LiveAccumulatedRainSeconds));
    }

    return true;
}

#endif
