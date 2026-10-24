#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationSpatialKeyUnionTest,
    "Kalmala.Gameplay.Population.LoadedKeyUnion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationSpatialKeyUnionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population key-union fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population key-union fixture GameMode created"), Mode)) return false;

    TestTrue(TEXT("Negative positions floor into the preceding invisible key"),
        FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(-0.01, -0.01)) == FIntPoint(-1, -1));
    TestTrue(TEXT("An exact negative key boundary belongs to the west/south key"),
        FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(-6400.0, -6400.0)) == FIntPoint(-1, -1));
    TestTrue(TEXT("Coordinates just below a negative boundary floor into the next negative key"),
        FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(-6400.1, -6400.1)) == FIntPoint(-2, -2));

    const FIntPoint TangentLeft(-2, 0);
    const FIntPoint CenterKey(0, 0);
    const FIntPoint SharedKey(1, 0);
    const FIntPoint TangentRight(2, 0);
    const FIntPoint SecondPlayerKey(2, 1);
    const FIntPoint OutsideKey(3, 0);
    const FIntPoint DistantKey(-3, 2);
    for (const FIntPoint& Key : { CenterKey, OutsideKey, TangentRight, SecondPlayerKey,
             SharedKey, DistantKey, TangentLeft })
    {
        Mode->ActivePopulationSpatialKeys.Add(Key);
    }

    TestTrue(TEXT("No players means there are no eligible loaded keys"),
        Mode->GetPopulationSpatialKeyCandidates().IsEmpty());

    struct FPlayerFixture
    {
        APlayerController* Controller = nullptr;
        APlayerState* PlayerState = nullptr;
        AKalmalaCharacter* Character = nullptr;
    };
    const auto AddPlayer = [World](const int32 PlayerId, const FVector Location)
    {
        FPlayerFixture Fixture;
        Fixture.Controller = World->SpawnActor<APlayerController>();
        Fixture.PlayerState = World->SpawnActor<APlayerState>();
        FActorSpawnParameters PawnParameters;
        PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Fixture.Character = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
            Location, FRotator::ZeroRotator, PawnParameters);
        if (Fixture.PlayerState != nullptr)
        {
            Fixture.PlayerState->SetPlayerId(PlayerId);
            if (Fixture.Controller != nullptr) Fixture.Controller->SetPlayerState(Fixture.PlayerState);
        }
        if (Fixture.Controller != nullptr && Fixture.Character != nullptr)
        {
            Fixture.Controller->Possess(Fixture.Character);
            World->AddController(Fixture.Controller);
        }
        return Fixture;
    };

    const FPlayerFixture HigherIdPlayer = AddPlayer(20, FVector(3200.0, 0.0, 0.0));
    const FPlayerFixture LowerIdPlayer = AddPlayer(10, FVector(8000.0, 0.0, 0.0));
    if (!TestTrue(TEXT("Both living player fixtures are fully possessed"),
        IsValid(HigherIdPlayer.Controller) && IsValid(HigherIdPlayer.PlayerState)
        && IsValid(HigherIdPlayer.Character) && HigherIdPlayer.Character->GetController() == HigherIdPlayer.Controller
        && IsValid(LowerIdPlayer.Controller) && IsValid(LowerIdPlayer.PlayerState)
        && IsValid(LowerIdPlayer.Character) && LowerIdPlayer.Character->GetController() == LowerIdPlayer.Controller))
    {
        return false;
    }

    TArray<FIntPoint> ExpectedKeys;
    ExpectedKeys.Add(TangentLeft);
    ExpectedKeys.Add(CenterKey);
    ExpectedKeys.Add(SharedKey);
    ExpectedKeys.Add(TangentRight);
    ExpectedKeys.Add(SecondPlayerKey);

    const TArray<FIntPoint> Candidates = Mode->GetPopulationSpatialKeyCandidates();
    TestTrue(TEXT("Candidates are the unique sorted union of intersecting loaded keys"),
        Candidates == ExpectedKeys);
    TestTrue(TEXT("A cell tangent to the 96 m radius is included"),
        Candidates.Contains(TangentLeft) && Candidates.Contains(TangentRight));
    TestTrue(TEXT("A loaded cell intersecting only the second player's neighborhood is included"),
        Candidates.Contains(SecondPlayerKey));
    TestFalse(TEXT("A loaded cell outside every player neighborhood is excluded"),
        Candidates.Contains(OutsideKey));

    const double KeySizeCm = FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
    const double RadiusCm = FKalmalaMonsterSpawnTuning::PlayerKeyIntersectionRadiusCm;
    const auto DoesCircleIntersectKey = [KeySizeCm, RadiusCm](const FVector& PlayerLocation, const FIntPoint Key)
    {
        const double MinimumX = static_cast<double>(Key.X) * KeySizeCm;
        const double MaximumX = MinimumX + KeySizeCm;
        const double MinimumY = static_cast<double>(Key.Y) * KeySizeCm;
        const double MaximumY = MinimumY + KeySizeCm;
        const double DistanceX = PlayerLocation.X < MinimumX ? MinimumX - PlayerLocation.X
            : PlayerLocation.X > MaximumX ? PlayerLocation.X - MaximumX : 0.0;
        const double DistanceY = PlayerLocation.Y < MinimumY ? MinimumY - PlayerLocation.Y
            : PlayerLocation.Y > MaximumY ? PlayerLocation.Y - MaximumY : 0.0;
        return DistanceX * DistanceX + DistanceY * DistanceY <= RadiusCm * RadiusCm;
    };
    TestTrue(TEXT("Shared loaded key intersects both player neighborhoods"),
        DoesCircleIntersectKey(HigherIdPlayer.Character->GetActorLocation(), SharedKey)
        && DoesCircleIntersectKey(LowerIdPlayer.Character->GetActorLocation(), SharedKey));
    int32 SharedKeyResultCount = 0;
    for (const FIntPoint& Key : Candidates)
    {
        if (Key == SharedKey) ++SharedKeyResultCount;
    }
    TestEqual(TEXT("Overlapping neighborhoods return their shared key once"), SharedKeyResultCount, 1);

    const TArray<FIntPoint> RepeatedCandidates = Mode->GetPopulationSpatialKeyCandidates();
    TestTrue(TEXT("Repeated query retains deterministic key order"), RepeatedCandidates == Candidates);
    TestEqual(TEXT("Candidate query does not materialize or remove population keys"),
        Mode->ActivePopulationSpatialKeys.Num(), 7);
    TestTrue(TEXT("Out-of-radius key remains loaded but is not activated again"),
        Mode->ActivePopulationSpatialKeys.Contains(OutsideKey));
    return true;
}

#endif
