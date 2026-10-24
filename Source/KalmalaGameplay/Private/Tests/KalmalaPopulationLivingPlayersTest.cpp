#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationLivingPlayersTest,
    "Kalmala.Gameplay.Population.LivingPlayerSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationLivingPlayersTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population player fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population player fixture GameMode created"), Mode)) return false;

    struct FPlayerFixture
    {
        APlayerController* Controller = nullptr;
        APlayerState* PlayerState = nullptr;
        AKalmalaCharacter* Character = nullptr;
    };
    const auto AddPlayer = [World](const int32 PlayerId, const bool bPossess = true,
        const bool bCreatePlayerState = true)
    {
        FPlayerFixture Fixture;
        Fixture.Controller = World->SpawnActor<APlayerController>();
        Fixture.Character = World->SpawnActor<AKalmalaCharacter>();
        if (bCreatePlayerState)
        {
            Fixture.PlayerState = World->SpawnActor<APlayerState>();
            if (Fixture.PlayerState != nullptr)
            {
                Fixture.PlayerState->SetPlayerId(PlayerId);
                if (Fixture.Controller != nullptr) Fixture.Controller->SetPlayerState(Fixture.PlayerState);
            }
        }
        if (bPossess && Fixture.Controller != nullptr && Fixture.Character != nullptr)
        {
            Fixture.Controller->Possess(Fixture.Character);
        }
        if (Fixture.Controller != nullptr) World->AddController(Fixture.Controller);
        return Fixture;
    };

    const FPlayerFixture HigherIdLivingPlayer = AddPlayer(20);
    const FPlayerFixture LowerIdLivingPlayer = AddPlayer(10);
    const FPlayerFixture DeadPlayer = AddPlayer(30);
    const FPlayerFixture Spectator = AddPlayer(40);
    const FPlayerFixture OnlySpectator = AddPlayer(45);
    const FPlayerFixture DisconnectedPlayer = AddPlayer(50);
    const FPlayerFixture UnpossessedPlayer = AddPlayer(60, false);
    const FPlayerFixture MissingPlayerState = AddPlayer(70, true, false);
    if (!TestTrue(TEXT("Living, dead, spectator, disconnected and invalid fixtures were created"),
        IsValid(HigherIdLivingPlayer.Controller) && IsValid(HigherIdLivingPlayer.PlayerState)
        && IsValid(HigherIdLivingPlayer.Character) && IsValid(LowerIdLivingPlayer.Controller)
        && IsValid(LowerIdLivingPlayer.PlayerState) && IsValid(LowerIdLivingPlayer.Character)
        && IsValid(DeadPlayer.Character) && IsValid(Spectator.PlayerState)
        && IsValid(OnlySpectator.PlayerState) && IsValid(DisconnectedPlayer.PlayerState) && IsValid(UnpossessedPlayer.Controller)
        && IsValid(MissingPlayerState.Character)))
    {
        return false;
    }

    FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(
        AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Character health is available for a dead-player fixture"), HealthProperty))
    {
        return false;
    }
    *HealthProperty->ContainerPtrToValuePtr<float>(DeadPlayer.Character) = 1.0f;
    Spectator.PlayerState->SetIsSpectator(true);
    OnlySpectator.PlayerState->SetIsOnlyASpectator(true);
    DisconnectedPlayer.PlayerState->SetIsInactive(true);

    const TArray<AKalmalaCharacter*> LivingPlayers = Mode->GetLivingPlayerPawnsForPopulation();
    TestEqual(TEXT("Only living, connected, possessed players are returned"), LivingPlayers.Num(), 2);
    if (LivingPlayers.Num() == 2)
    {
        TestTrue(TEXT("Player IDs determine stable population-list order"),
            LivingPlayers[0] == LowerIdLivingPlayer.Character
            && LivingPlayers[1] == HigherIdLivingPlayer.Character);
    }
    TestFalse(TEXT("Dead pawn is excluded"), LivingPlayers.Contains(DeadPlayer.Character));
    TestFalse(TEXT("Spectator pawn is excluded"), LivingPlayers.Contains(Spectator.Character));
    TestFalse(TEXT("Only-spectator pawn is excluded"), LivingPlayers.Contains(OnlySpectator.Character));
    TestFalse(TEXT("Inactive disconnected player is excluded"), LivingPlayers.Contains(DisconnectedPlayer.Character));
    TestFalse(TEXT("Unpossessed pawn is excluded"), LivingPlayers.Contains(UnpossessedPlayer.Character));
    TestFalse(TEXT("Controller without a PlayerState is excluded"), LivingPlayers.Contains(MissingPlayerState.Character));
    return true;
}

#endif
