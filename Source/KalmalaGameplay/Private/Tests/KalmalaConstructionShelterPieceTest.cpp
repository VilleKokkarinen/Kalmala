#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionShelterPieceTest, "Kalmala.Gameplay.Construction.ShelterPieces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionShelterPieceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Floor uses a broad low collision footprint"), AKalmalaConstructionActor::GetCollisionExtent(TEXT("FloorKit")), FVector(120, 120, 12));
    TestEqual(TEXT("Windbreak uses a thin upright collision footprint"), AKalmalaConstructionActor::GetCollisionExtent(TEXT("WallKit")), FVector(120, 12, 110));
    TestEqual(TEXT("Roof uses a broad overhead collision footprint"), AKalmalaConstructionActor::GetCollisionExtent(TEXT("RoofKit")), FVector(132, 132, 16));
    TestTrue(TEXT("Only the accepted windbreak kit declares shelter eligibility"), AKalmalaConstructionActor::IsShelterKit(TEXT("WallKit")));
    TestTrue(TEXT("Only the accepted roof kit declares shelter eligibility"), AKalmalaConstructionActor::IsShelterKit(TEXT("RoofKit")));
    TestFalse(TEXT("Floors do not claim weather shelter"), AKalmalaConstructionActor::IsShelterKit(TEXT("FloorKit")));
    TestFalse(TEXT("Unknown construction cannot claim weather shelter"), AKalmalaConstructionActor::IsShelterKit(TEXT("UnknownKit")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionGeometryTest, "Kalmala.Gameplay.Construction.GeometryAlignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionGeometryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient geometry test world"), World)) return false;
    for (const FName Kit : {FName(TEXT("FloorKit")), FName(TEXT("WallKit")), FName(TEXT("RoofKit"))})
    {
        auto* Piece = World->SpawnActor<AKalmalaConstructionActor>();
        if (!TestNotNull(TEXT("Spawn server construction"), Piece)) continue;
        Piece->InitializeFromServer(Kit, TEXT("geometry-test"));
        const auto* Box = Piece->FindComponentByClass<UBoxComponent>();
        const auto* Mesh = Piece->FindComponentByClass<UProceduralMeshComponent>();
        if (TestNotNull(TEXT("Collision component"), Box) && TestNotNull(TEXT("Presentation component"), Mesh))
        {
            const FBox VisualBounds = Mesh->CalcBounds(Mesh->GetComponentTransform()).GetBox();
            const FBox CollisionBounds = Box->CalcBounds(Box->GetComponentTransform()).GetBox();
            TestTrue(*FString::Printf(TEXT("%s visible geometry and blocking collision have matching centres: visual=%s collision=%s"),
                *Kit.ToString(), *VisualBounds.GetCenter().ToString(), *CollisionBounds.GetCenter().ToString()),
                VisualBounds.GetCenter().Equals(CollisionBounds.GetCenter(), 0.1));
            TestTrue(TEXT("Collision covers the visible shelter piece extent"),
                VisualBounds.GetExtent().Equals(CollisionBounds.GetExtent(), 0.1));
            TestEqual(TEXT("Construction blocks pawn movement"), Box->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
            TestEqual(TEXT("Construction blocks server shelter traces"), Box->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
        }
        Piece->Destroy();
    }
    World->DestroyWorld(false);
    return true;
}
#endif
