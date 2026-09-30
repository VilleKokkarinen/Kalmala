#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveWriteCandidateTest,
    "Kalmala.Gameplay.Construction.Schema2WriteCandidate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveWriteCandidateTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;

    const auto MakeRecord = [](const TCHAR* Id, const FName KitId)
    {
        FKalmalaConstructionSaveRecord Record;
        Record.ConstructionId = Id;
        Record.KitId = KitId;
        Record.Transform = FTransform(FVector(100.0f, 200.0f, 300.0f));
        return Record;
    };
    const auto Serialize = [](USaveGame* Save, TArray<uint8>& Bytes)
    {
        Bytes.Reset();
        return UGameplayStatics::SaveGameToMemory(Save, Bytes);
    };

    UKalmalaConstructionSaveGame* Legacy = NewObject<UKalmalaConstructionSaveGame>(GetTransientPackage());
    Legacy->InitializeForWorld(World);
    TestTrue(TEXT("Schema 1 contains an existing floor"), Legacy->AddRecord(MakeRecord(TEXT("floor-01"), TEXT("FloorKit"))));
    TestTrue(TEXT("Schema 1 contains an existing workbench"), Legacy->AddRecord(MakeRecord(TEXT("workbench-01"), TEXT("WorkbenchKit"))));
    TArray<uint8> LegacyBefore;
    TestTrue(TEXT("Legacy slot data serializes before candidate creation"), Serialize(Legacy, LegacyBefore));

    UKalmalaConstructionSaveGameV2* MigratedCandidate = nullptr;
    TestTrue(TEXT("A valid schema-1 slot builds a schema-2 write candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(Legacy, World, GetTransientPackage(), MigratedCandidate));
    if (TestNotNull(TEXT("Migration returns a candidate separate from the old save"), MigratedCandidate))
    {
        TestTrue(TEXT("Migrated candidate carries exact world identity"), MigratedCandidate->MatchesWorld(World));
        TestEqual(TEXT("Migrated candidate preserves every legacy construction"), MigratedCandidate->GetRecords().Num(), 2);
        TestTrue(TEXT("First write accepts a validated server attachment"),
            MigratedCandidate->AddStationAttachmentRecord(MakeRecord(TEXT("workbench-rack-01"), TEXT("WorkbenchToolRackKit"))));
        TestTrue(TEXT("Complete first-write candidate revalidates"), MigratedCandidate->MatchesWorld(World));
        TestEqual(TEXT("Candidate retains legacy facts plus the new attachment"), MigratedCandidate->GetRecords().Num(), 3);

        TArray<uint8> CandidateBytes;
        TestTrue(TEXT("Schema-2 first-write candidate serializes"), Serialize(MigratedCandidate, CandidateBytes));
        UKalmalaConstructionSaveGameV2* Reloaded = Cast<UKalmalaConstructionSaveGameV2>(
            UGameplayStatics::LoadGameFromMemory(CandidateBytes));
        if (TestNotNull(TEXT("First-write candidate reloads as schema 2"), Reloaded))
        {
            TestTrue(TEXT("Reloaded candidate remains valid for the same world"), Reloaded->MatchesWorld(World));
            TestEqual(TEXT("Reloaded candidate preserves migrated and new records"), Reloaded->GetRecords().Num(), 3);
        }
    }

    TArray<uint8> LegacyAfter;
    TestTrue(TEXT("Legacy source still serializes after candidate creation"), Serialize(Legacy, LegacyAfter));
    TestTrue(TEXT("First-write construction candidate leaves the old save bytes unchanged"), LegacyBefore == LegacyAfter);

    UKalmalaConstructionSaveGameV2* NewWorldCandidate = nullptr;
    TestTrue(TEXT("A new world builds an empty schema-2 candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(nullptr, World, GetTransientPackage(), NewWorldCandidate));
    if (TestNotNull(TEXT("Empty candidate is available for its first construction"), NewWorldCandidate))
    {
        TestTrue(TEXT("Empty candidate matches the requested world"), NewWorldCandidate->MatchesWorld(World));
        TestEqual(TEXT("New schema-2 world begins without invented constructions"), NewWorldCandidate->GetRecords().Num(), 0);
    }

    FKalmalaWorldGenerationConfig OtherWorld = World;
    ++OtherWorld.WorldSeed;
    UKalmalaConstructionSaveGameV2* RejectedCandidate = nullptr;
    TestFalse(TEXT("Mismatched legacy save cannot build a replacement candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(Legacy, OtherWorld, GetTransientPackage(), RejectedCandidate));
    TestNull(TEXT("Rejected migration exposes no candidate"), RejectedCandidate);

    return true;
}
#endif
