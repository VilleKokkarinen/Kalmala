#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldBoundaryMovementTest, "Kalmala.Gameplay.Movement.WorldBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaWorldBoundaryMovementTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient movement world"), World)) return false;
    auto* State = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    World->SetGameState(State);
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (TestNotNull(TEXT("Server character"), Pawn) && TestNotNull(TEXT("Server identity"), State))
    {
        auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
        const double Limit = FKalmalaWorldBounds::Radius - Pawn->GetSimpleCollisionRadius() - 2;
        for (double Angle : {0.0, 0.8, 2.0, 3.5, 5.0})
        {
            const FVector Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0);
            const FVector Tangent(-Outward.Y, Outward.X, 0);
            Pawn->SetActorLocation(Outward * (Limit + 500) + FVector(0, 0, 1000), false, nullptr, ETeleportType::TeleportPhysics);
            Movement->Velocity = Outward * 500 + Tangent * 100 + FVector(0, 0, -30);
            Movement->OnMovementUpdated(.016f, Pawn->GetActorLocation(), Movement->Velocity);
            TestTrue(TEXT("Actual movement clamps server pawn inside circular capsule margin"),
                FMath::IsNearlyEqual(FVector2D(Pawn->GetActorLocation()).Size(), Limit, .01));
            TestTrue(TEXT("Outward velocity removed"), FMath::Abs(FVector::DotProduct(Movement->Velocity, Outward)) < .001);
            TestTrue(TEXT("Tangential travel retained"), FMath::IsNearlyEqual(FVector::DotProduct(Movement->Velocity, Tangent), 100.0, .001));
            TestEqual(TEXT("Vertical movement retained"), Movement->Velocity.Z, -30.0);
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif
