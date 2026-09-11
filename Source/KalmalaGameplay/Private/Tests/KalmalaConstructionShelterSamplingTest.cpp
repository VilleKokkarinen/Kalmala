#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaShelterSampler.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionShelterSamplingTest,
    "Kalmala.Gameplay.Construction.ShelterSampling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionShelterSamplingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient collision world"), World)) return false;
    AActor* Occupant = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Shelter probe occupant"), Occupant))
    {
        World->DestroyWorld(false);
        return false;
    }
    auto* Root = NewObject<USceneComponent>(Occupant);
    Occupant->SetRootComponent(Root);
    Occupant->AddInstanceComponent(Root);
    Root->RegisterComponent();
    auto SpawnPiece = [&](FName Kit, const FVector& Position)
    {
        auto* Piece = World->SpawnActor<AKalmalaConstructionActor>();
        if (Piece)
        {
            Piece->InitializeFromServer(Kit, Kit.ToString());
            Piece->SetActorLocation(Position);
        }
        return Piece;
    };
    auto Sample = [&](int32 Wind = 90)
    {
        return FKalmalaShelterSampler::Sample(World, Occupant, 0.0f, Wind);
    };
    TestEqual(TEXT("Open geometry gives no constructed shelter"), Sample().Shelter, 0.0f);
    auto* Roof = SpawnPiece(TEXT("RoofKit"), FVector(0, 0, 300));
    auto* Wall = SpawnPiece(TEXT("WallKit"), FVector(0, -160, 110));
    auto* Floor = SpawnPiece(TEXT("FloorKit"), FVector(0, 0, -24));
    if (TestNotNull(TEXT("Accepted roof"), Roof) && TestNotNull(TEXT("Accepted windbreak"), Wall)
        && TestNotNull(TEXT("Accepted floor"), Floor))
    {
        const auto Covered = Sample();
        TestTrue(TEXT("Actual roof collision intercepts the upward probe"), Covered.bHasRoof);
        TestTrue(TEXT("Actual windbreak collision intercepts the upwind probe"), Covered.bHasWindbreak);
        TestTrue(TEXT("Construction geometry composes eighty percent shelter"), FMath::IsNearlyEqual(Covered.Shelter, 0.8f, 0.0001f));
        const auto ReversedWind = Sample(270);
        TestTrue(TEXT("Wind reversal retains roof protection"), ReversedWind.bHasRoof);
        TestFalse(TEXT("A downwind wall does not protect against reversed wind"), ReversedWind.bHasWindbreak);
        TestTrue(TEXT("Roof alone supplies forty-five percent shelter"), FMath::IsNearlyEqual(ReversedWind.Shelter, 0.45f, 0.0001f));

        Occupant->SetActorLocation(FVector(600, 0, 0));
        TestEqual(TEXT("Leaving the geometry removes constructed shelter"), Sample().Shelter, 0.0f);
        Occupant->SetActorLocation(FVector::ZeroVector);
        Roof->SetActorLocation(FVector(1000, 0, 300));
        Wall->SetActorLocation(FVector(1000, -160, 110));
        Floor->SetActorLocation(FVector(0, 0, 300));
        TestFalse(TEXT("Blocking floor overhead is not a tagged roof"), Sample().bHasRoof);
        Floor->SetActorLocation(FVector(0, -160, 80));
        TestFalse(TEXT("Blocking floor upwind is not a tagged windbreak"), Sample().bHasWindbreak);

        // Exercise physics queries, not merely the configured response enum. Isolate
        // each original solid and sweep a pawn-sized capsule through its face.
        Roof->SetActorLocation(FVector(0, 0, 1000));
        Wall->SetActorLocation(FVector(1000, 0, 1000));
        Floor->SetActorLocation(FVector(2000, 0, 1000));
        for (auto* Piece : {Roof, Wall, Floor})
        {
            const FVector Axis = Piece == Wall ? FVector::YAxisVector : FVector::UpVector;
            const FVector Centre = Piece->GetActorLocation();
            FHitResult Hit;
            const bool Blocked = World->SweepSingleByChannel(Hit, Centre + Axis * 300, Centre - Axis * 300,
                FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(34, 88));
            TestTrue(*FString::Printf(TEXT("%s blocks a pawn capsule sweep with its actual solid"),
                *Piece->GetConstructionKit().ToString()), Blocked && Hit.GetActor() == Piece && !Hit.bStartPenetrating);
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif
