#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "Misc/AutomationTest.h"

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
#endif
