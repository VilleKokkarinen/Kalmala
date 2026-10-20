#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveWriteCandidateTest,
    "Kalmala.Gameplay.Construction.Schema4WriteCandidate",
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

    UKalmalaConstructionSaveGameV2* Existing = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    Existing->InitializeForWorld(World);
    TestTrue(TEXT("Current schema contains an existing floor"), Existing->AddRecord(MakeRecord(TEXT("floor-01"), TEXT("FloorKit"))));
    TArray<uint8> ExistingBefore;
    TestTrue(TEXT("Current slot serializes before candidate creation"), Serialize(Existing, ExistingBefore));

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    TestTrue(TEXT("A current schema slot builds a new write candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(Existing, World, GetTransientPackage(), Candidate));
    if (TestNotNull(TEXT("Candidate is separate from the existing save"), Candidate))
    {
        TestTrue(TEXT("Candidate carries exact world identity"), Candidate->MatchesWorld(World));
        TestEqual(TEXT("Candidate preserves existing constructions"), Candidate->GetRecords().Num(), 1);
        TestTrue(TEXT("First write accepts a validated server attachment"),
            Candidate->AddStationAttachmentRecord(MakeRecord(TEXT("workbench-rack-01"), TEXT("WorkbenchToolRackKit"))));
        TestTrue(TEXT("Complete candidate revalidates"), Candidate->MatchesWorld(World));
        TestEqual(TEXT("Candidate retains existing facts plus the new attachment"), Candidate->GetRecords().Num(), 2);

        TArray<uint8> CandidateBytes;
        TestTrue(TEXT("Schema-4 write candidate serializes"), Serialize(Candidate, CandidateBytes));
        UKalmalaConstructionSaveGameV2* Reloaded = Cast<UKalmalaConstructionSaveGameV2>(
            UGameplayStatics::LoadGameFromMemory(CandidateBytes));
        if (TestNotNull(TEXT("Write candidate reloads as schema 4"), Reloaded))
        {
            TestTrue(TEXT("Reloaded candidate remains valid for the same world"), Reloaded->MatchesWorld(World));
            TestEqual(TEXT("Reloaded candidate preserves existing and new records"), Reloaded->GetRecords().Num(), 2);
        }
    }
    TestEqual(TEXT("The source save was not changed by candidate edits"), Existing->GetRecords().Num(), 1);

    TArray<uint8> ExistingAfter;
    TestTrue(TEXT("Existing source still serializes after candidate creation"), Serialize(Existing, ExistingAfter));
    TestTrue(TEXT("Candidate creation leaves the existing save bytes unchanged"), ExistingBefore == ExistingAfter);

    UKalmalaConstructionSaveGameV2* NewWorldCandidate = nullptr;
    TestTrue(TEXT("A new world builds an empty schema-4 candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(nullptr, World, GetTransientPackage(), NewWorldCandidate));
    if (TestNotNull(TEXT("Empty candidate is available for its first construction"), NewWorldCandidate))
    {
        TestTrue(TEXT("Empty candidate matches the requested world"), NewWorldCandidate->MatchesWorld(World));
        TestEqual(TEXT("New schema-4 world begins without invented constructions"), NewWorldCandidate->GetRecords().Num(), 0);
    }

    FKalmalaWorldGenerationConfig OtherWorld = World;
    ++OtherWorld.WorldSeed;
    UKalmalaConstructionSaveGameV2* RejectedCandidate = nullptr;
    TestFalse(TEXT("Mismatched current save cannot build a replacement candidate"),
        UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(Existing, OtherWorld, GetTransientPackage(), RejectedCandidate));
    TestNull(TEXT("Rejected mismatched candidate exposes no replacement"), RejectedCandidate);

    return true;
}
#endif
