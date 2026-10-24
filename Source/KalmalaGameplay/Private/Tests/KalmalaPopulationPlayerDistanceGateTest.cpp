#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationPlayerDistanceGateTest,
    "Kalmala.Gameplay.Population.PlayerDistanceGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationPlayerDistanceGateTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population distance fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population distance fixture GameMode created"), Mode)) return false;

    struct FPlayerFixture
    {
        APlayerController* Controller = nullptr;
        APlayerState* PlayerState = nullptr;
        AKalmalaCharacter* Character = nullptr;
    };
    const auto AddPlayer = [World](const int32 PlayerId, const FVector& Location)
    {
        FPlayerFixture Fixture;
        Fixture.Controller = World->SpawnActor<APlayerController>();
        Fixture.PlayerState = World->SpawnActor<APlayerState>();
        if (Fixture.PlayerState != nullptr)
        {
            Fixture.PlayerState->SetPlayerId(PlayerId);
            if (Fixture.Controller != nullptr) Fixture.Controller->SetPlayerState(Fixture.PlayerState);
        }
        FActorSpawnParameters PawnParameters;
        PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Fixture.Character = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
            Location, FRotator::ZeroRotator, PawnParameters);
        if (Fixture.Controller != nullptr && Fixture.Character != nullptr)
        {
            Fixture.Controller->Possess(Fixture.Character);
        }
        if (Fixture.Controller != nullptr) World->AddController(Fixture.Controller);
        return Fixture;
    };

    const FPlayerFixture SelectingPlayer = AddPlayer(1, FVector::ZeroVector);
    const FPlayerFixture DistantPlayer = AddPlayer(2, FVector(10000.0, 0.0, 0.0));
    if (!TestTrue(TEXT("Both living player fixtures are possessed"),
        IsValid(SelectingPlayer.Controller) && IsValid(SelectingPlayer.PlayerState)
        && IsValid(SelectingPlayer.Character) && IsValid(DistantPlayer.Controller)
        && IsValid(DistantPlayer.PlayerState) && IsValid(DistantPlayer.Character)))
    {
        return false;
    }

    const TArray<AKalmalaCharacter*> LivingPlayers = Mode->GetLivingPlayerPawnsForPopulation();
    if (!TestTrue(TEXT("The authoritative source includes both current-world players"),
        LivingPlayers.Num() == 2 && LivingPlayers.Contains(SelectingPlayer.Character)
        && LivingPlayers.Contains(DistantPlayer.Character)))
    {
        return false;
    }
    TestTrue(TEXT("The second player is outside the selecting player's 96 m neighborhood"),
        FVector::DistSquared2D(SelectingPlayer.Character->GetActorLocation(),
            DistantPlayer.Character->GetActorLocation())
            > FKalmalaMonsterSpawnTuning::PlayerKeyIntersectionRadiusCm
                * FKalmalaMonsterSpawnTuning::PlayerKeyIntersectionRadiusCm);

    FKalmalaWorldPopulationSpawn NearSecondPlayer;
    NearSecondPlayer.Kind = EKalmalaWorldPopulationKind::Wildlife;
    NearSecondPlayer.SpatialKey = FIntPoint(1, 0);
    NearSecondPlayer.SpawnSeed = 9001;
    NearSecondPlayer.Location = FVector(6500.0, 0.0, 0.0);
    NearSecondPlayer.ContentId = FName(TEXT("meadow-creature"));
    TestTrue(TEXT("The candidate passes the unchanged inclusive S08 ring selector"),
        Mode->IsDescriptorInSpawnRing(NearSecondPlayer.Location, LivingPlayers));

    FKalmalaPopulationSchedulerKeyWork Work;
    Work.SpatialKey = NearSecondPlayer.SpatialKey;
    Work.Candidates.Add(NearSecondPlayer);
    Mode->ApplyPopulationPlayerDistanceGate(Work, LivingPlayers);
    TestTrue(TEXT("A candidate 35 m from the second, distant player is rejected"), Work.Candidates.IsEmpty());

    FKalmalaWorldPopulationSpawn ExactBoundary = NearSecondPlayer;
    ExactBoundary.Location = FVector(6400.0, 0.0, 0.0);
    Work.Candidates.Add(ExactBoundary);
    Mode->ApplyPopulationPlayerDistanceGate(Work, LivingPlayers);
    TestTrue(TEXT("Exactly 36 m from every player passes the S09 exclusion gate"),
        Work.Candidates.Num() == 1 && Work.Candidates[0].SpawnSeed == ExactBoundary.SpawnSeed);
    return true;
}

#endif
