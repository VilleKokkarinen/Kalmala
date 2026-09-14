#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionRainWearTest,
    "Kalmala.Gameplay.Construction.RainWear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaConstructionRainWearTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto Spawn = [&](const TCHAR* Kit, const FVector& Location)
    {
        auto* Piece = World->SpawnActor<AKalmalaConstructionActor>();
        if (Piece) { Piece->InitializeFromServer(Kit, Kit); Piece->SetActorLocation(Location); }
        return Piece;
    };
    int32 Index = 0;
    for (const TCHAR* Kit : {TEXT("FloorKit"), TEXT("WallKit"), TEXT("WorkbenchKit"), TEXT("StorageKit")})
    {
        auto* Piece = Spawn(Kit, FVector(++Index * 1000, 0, 0));
        if (!TestNotNull(TEXT("Eligible construction spawned"), Piece)) continue;
        TestEqual(TEXT("New construction starts full"), Piece->GetHealth(), 100.0f);
        Piece->AdvanceRainWearFromServer(10, 0.5f);
        TestEqual(TEXT("Partial rain applies proportional wear"), Piece->GetHealth(), 99.5f);
        Piece->AdvanceRainWearFromServer(10, 0);
        Piece->AdvanceRainWearFromServer(-10, 1);
        Piece->AdvanceRainWearFromServer(NAN, 1);
        Piece->AdvanceRainWearFromServer(10, NAN);
        TestEqual(TEXT("Dry and invalid inputs cannot damage or heal"), Piece->GetHealth(), 99.5f);
        Piece->SetRole(ROLE_SimulatedProxy);
        Piece->AdvanceRainWearFromServer(1000, 1);
        TestEqual(TEXT("Client-local wear is rejected"), Piece->GetHealth(), 99.5f);
        Piece->SetRole(ROLE_Authority);
        Piece->AdvanceRainWearFromServer(10, 20);
        TestEqual(TEXT("Rain intensity is clamped"), Piece->GetHealth(), 98.5f);
        Piece->AdvanceRainWearFromServer(1000, 1);
        Piece->AdvanceRainWearFromServer(1000, 1);
        TestEqual(TEXT("Rain cannot go below fifty percent"), Piece->GetHealth(), 50.0f);
    }
    auto* Floor = Spawn(TEXT("FloorKit"), FVector::ZeroVector);
    auto* Roof = Spawn(TEXT("RoofKit"), FVector(0, 0, 300));
    if (TestNotNull(TEXT("Protected floor"), Floor) && TestNotNull(TEXT("Roof"), Roof))
    {
        Floor->AdvanceRainWearFromServer(100, 1);
        TestEqual(TEXT("Actual overhead roof collision protects"), Floor->GetHealth(), 100.0f);
        Roof->AdvanceRainWearFromServer(1000, 1);
        TestEqual(TEXT("Exposed roof is rain immune"), Roof->GetHealth(), 100.0f);
        Roof->SetActorLocation(FVector(0, 0, 600));
        Floor->AdvanceRainWearFromServer(10, 1);
        TestEqual(TEXT("Roof beyond probe reach does not protect"), Floor->GetHealth(), 99.0f);
        Roof->SetActorLocation(FVector(0, 0, 300));
        auto* Blocker = Spawn(TEXT("FloorKit"), FVector(0, 0, 180));
        if (TestNotNull(TEXT("Untagged blocking floor"), Blocker))
        {
            Floor->AdvanceRainWearFromServer(10, 1);
            TestEqual(TEXT("First untagged hit is not accepted roof protection"), Floor->GetHealth(), 98.0f);
            Blocker->SetActorLocation(FVector(0, 1000, 180));
            Floor->AdvanceRainWearFromServer(10, 1);
            TestEqual(TEXT("Protection prevents wear without repairing health"), Floor->GetHealth(), 98.0f);
        }
    }
    auto* Unknown = Spawn(TEXT("UnknownKit"), FVector(0, 2000, 0));
    if (Unknown) { Unknown->AdvanceRainWearFromServer(1000, 1); TestEqual(TEXT("Unknown kits do not participate"), Unknown->GetHealth(), 100.0f); }
    World->DestroyWorld(false);
    return true;
}
#endif
