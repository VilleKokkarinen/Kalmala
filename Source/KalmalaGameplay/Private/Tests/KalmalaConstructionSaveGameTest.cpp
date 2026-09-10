#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveGameTest, "Kalmala.Gameplay.Construction.SaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveGameTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World; World.WorldSeed = 418; World.GeneratorRevision = 4;
    auto* Save = NewObject<UKalmalaConstructionSaveGame>(); Save->InitializeForWorld(World);
    FKalmalaConstructionSaveRecord Record; Record.ConstructionId = TEXT("camp-0001"); Record.KitId = TEXT("FloorKit"); Record.Transform = FTransform(FVector(100, 200, 300));
    TestTrue(TEXT("Construction save accepts a bounded valid record"), Save->AddRecord(Record));
    Record.Transform.SetLocation(FVector(NAN, 0, 0));
    TestFalse(TEXT("Construction save rejects non-finite transforms"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    Record.Transform = FTransform(FVector(100, 200, 300));
    TestFalse(TEXT("Construction save rejects duplicate stable IDs"), Save->AddRecord(Record));
    TestTrue(TEXT("Construction save can roll back a newly added record"), Save->RemoveRecord(TEXT("camp-0001")));
    TestTrue(TEXT("Construction save can re-add a rolled-back record"), Save->AddRecord(Record));
    for (int32 Index = 2; Index <= UKalmalaConstructionSaveGame::MaxRecords; ++Index)
    {
        Record.ConstructionId = FString::Printf(TEXT("camp-%04d"), Index);
        TestTrue(TEXT("Construction save accepts records within its cap"), Save->AddRecord(Record));
    }
    Record.ConstructionId = TEXT("camp-over-cap");
    TestFalse(TEXT("Construction save rejects records beyond its cap"), Save->AddRecord(Record));
    TArray<uint8> Bytes; TestTrue(TEXT("Construction save serializes in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reloaded = Cast<UKalmalaConstructionSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Construction save reloads with its type"), Reloaded))
    {
        TestTrue(TEXT("Construction save retains matching world identity"), Reloaded->MatchesWorld(World));
        TestEqual(TEXT("Construction save retains every bounded stable record"), Reloaded->GetRecords().Num(), UKalmalaConstructionSaveGame::MaxRecords);
    }
    World.WorldSeed = 419;
    TestFalse(TEXT("Construction save rejects a different world identity"), Save->MatchesWorld(World));
    return true;
}
#endif
