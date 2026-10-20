#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlacementPreview.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveGameTest, "Kalmala.Gameplay.Construction.SaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveGameTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World; World.WorldSeed = 418; auto* Save = NewObject<UKalmalaConstructionSaveGame>(); Save->InitializeForWorld(World);
    FKalmalaConstructionSaveRecord Record; Record.ConstructionId = TEXT("camp-0001"); Record.KitId = TEXT("FloorKit"); Record.Transform = FTransform(FVector(100, 200, 300));
    TestTrue(TEXT("Construction save accepts a bounded valid record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-rack"); Record.KitId = TEXT("CookingRackKit");
    TestTrue(TEXT("Cooking rack uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-cauldron"); Record.KitId = TEXT("CauldronKit");
    TestTrue(TEXT("Cauldron uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-frying-pan"); Record.KitId = TEXT("FryingPanKit");
    TestTrue(TEXT("Placeable frying pan uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-charcoal-kiln"); Record.KitId = TEXT("CharcoalKilnKit");
    TestTrue(TEXT("Charcoal Kiln uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("session-attachment"); Record.KitId = TEXT("WorkbenchToolRackKit");
    TestFalse(TEXT("M9 station attachments remain outside the existing save schema"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    TestTrue(TEXT("Station attachment session policy is explicit"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("WorkbenchToolRackKit")));
    Record.ConstructionId = TEXT("camp-0001"); Record.KitId = TEXT("FloorKit");
    Record.Transform.SetLocation(FVector(NAN, 0, 0));
    TestFalse(TEXT("Construction save rejects non-finite transforms"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    Record.Transform = FTransform(FVector(100, 200, 300));
    TestFalse(TEXT("Construction save rejects duplicate stable IDs"), Save->AddRecord(Record));
    TestTrue(TEXT("Construction save can roll back a newly added record"), Save->RemoveRecord(TEXT("camp-0001")));
    TestTrue(TEXT("Construction save can re-add a rolled-back record"), Save->AddRecord(Record));
    for (int32 Index = Save->GetRecords().Num() + 1; Index <= UKalmalaConstructionSaveGame::MaxRecords; ++Index)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveGameV2Test,
    "Kalmala.Gameplay.Construction.Schema3CurrentFormat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveGameV2Test::RunTest(const FString& Parameters)
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
    const auto Serialize = [](USaveGame* Save, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();
        return UGameplayStatics::SaveGameToMemory(Save, OutBytes);
    };

    UKalmalaConstructionSaveGameV2* Save = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    Save->InitializeForWorld(World);
    const FKalmalaConstructionSaveRecord Floor = MakeRecord(TEXT("camp-floor-01"), TEXT("FloorKit"));
    const FKalmalaConstructionSaveRecord Rack = MakeRecord(TEXT("camp-rack-01"), TEXT("WorkbenchToolRackKit"));
    const FKalmalaConstructionSaveRecord Campfire = MakeRecord(TEXT("camp-fire-01"), TEXT("CampfireKit"));
    FKalmalaConstructionSaveRecord Kiln = MakeRecord(TEXT("camp-kiln-01"), TEXT("CharcoalKilnKit"));
    Kiln.CharcoalKiln.InputWood = 2;
    Kiln.CharcoalKiln.OutputCoal = 1;
    Kiln.CharcoalKiln.JobRemainingSeconds = 12.5f;
    TestTrue(TEXT("Schema 3 accepts an established camp record"), Save->AddRecord(Floor));
    TestTrue(TEXT("Schema 3 accepts a bounded station attachment"), Save->AddStationAttachmentRecord(Rack));
    TestTrue(TEXT("Schema 3 retains the stable Campfire construction identity"), Save->AddRecord(Campfire));
    TestTrue(TEXT("Schema 3 saves kiln input, output, and remaining active seconds"), Save->AddRecord(Kiln));
    TestFalse(TEXT("Schema 3 rejects a duplicate stable construction ID across record kinds"),
        Save->AddStationAttachmentRecord(MakeRecord(TEXT("camp-floor-01"), TEXT("ForgeAnvilKit"))));
    TestFalse(TEXT("Schema 3 rejects unsupported construction identities"),
        Save->AddRecord(MakeRecord(TEXT("camp-unknown-01"), TEXT("UnknownConstructionKit"))));
    TestEqual(TEXT("Rejected schema-3 additions leave the live candidate unchanged"), Save->GetRecords().Num(), 4);

    TArray<uint8> CurrentBytes;
    TestTrue(TEXT("Schema 3 construction records serialize in memory"), Serialize(Save, CurrentBytes));
    UKalmalaConstructionSaveGameV2* Reloaded =
        Cast<UKalmalaConstructionSaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (TestNotNull(TEXT("Schema 3 construction records reload with their type"), Reloaded))
    {
        TestTrue(TEXT("Reloaded container carries exact world, revision, and world scope"), Reloaded->MatchesWorld(World));
        TestEqual(TEXT("Existing construction, Campfire, kiln, and attachment records round-trip together"), Reloaded->GetRecords().Num(), 4);
        TestTrue(TEXT("Kiln inventory and job timer survive reload"), Reloaded->GetRecords().ContainsByPredicate(
            [](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-kiln-01") && Record.KitId == TEXT("CharcoalKilnKit")
                    && Record.CharcoalKiln.InputWood == 2 && Record.CharcoalKiln.OutputCoal == 1
                    && FMath::IsNearlyEqual(Record.CharcoalKiln.JobRemainingSeconds, 12.5f);
            }));
        TestTrue(TEXT("Reloaded container retains the Campfire construction ID"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-fire-01") && Record.KitId == TEXT("CampfireKit");
            }));
        TestTrue(TEXT("Reloaded container retains its station attachment"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.KitId == TEXT("WorkbenchToolRackKit");
            }));
    }

    UKalmalaConstructionSaveGameV2* IdentityChecks = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    const FKalmalaM7SaveIdentity ExpectedIdentity = IdentityChecks->Identity;
    IdentityChecks->Identity.WorldSeed++;
    TestFalse(TEXT("Schema 3 rejects a mismatched identity seed"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.GeneratorRevision--;
    TestFalse(TEXT("Schema 3 rejects a mismatched generator revision"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.Scope = EKalmalaM7SaveScope::Player;
    IdentityChecks->Identity.OwnerIdentity = TEXT("provider:other");
    TestFalse(TEXT("Schema 3 rejects a mismatched persistence scope"), IdentityChecks->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->SchemaVersion = 0;
    TestFalse(TEXT("Current loads reject unsupported schema zero"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent->SchemaVersion = UKalmalaConstructionSaveGameV2::SchemaVersionValue + 1;
    TestFalse(TEXT("Current loads reject future schema versions"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[0].Transform.SetLocation(FVector(0.0f, NAN, 0.0f));
    TestFalse(TEXT("Current loads reject malformed transforms"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[0].ConstructionId.Reset();
    TestFalse(TEXT("Current loads reject malformed construction identities"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[3].CharcoalKiln.OutputCoal = FKalmalaCharcoalKilnState::OutputCapacity;
    InvalidCurrent->Records[3].CharcoalKiln.JobRemainingSeconds = 1.0f;
    TestFalse(TEXT("Current loads reject a kiln job when its output is full"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    const FKalmalaConstructionSaveRecord DuplicateCurrentRecord = InvalidCurrent->Records[0];
    InvalidCurrent->Records.Add(DuplicateCurrentRecord);
    TestFalse(TEXT("Current loads reject duplicate construction identities"), InvalidCurrent->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* OverCapCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    for (int32 Index = 0; Index < UKalmalaConstructionSaveGameV2::MaxRecords; ++Index)
    {
        OverCapCurrent->Records.Add(MakeRecord(*FString::Printf(TEXT("current-%03d"), Index), TEXT("FloorKit")));
    }
    TestFalse(TEXT("Current loads reject combined construction records above the total cap"), OverCapCurrent->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* AttachmentCap = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    AttachmentCap->InitializeForWorld(World);
    for (int32 Index = 0; Index < UKalmalaConstructionSaveGameV2::MaxStationAttachments; ++Index)
    {
        const FName KitId = Index % 2 == 0 ? FName(TEXT("WorkbenchToolRackKit")) : FName(TEXT("ForgeAnvilKit"));
        TestTrue(TEXT("Station attachment remains within its accepted cap"),
            AttachmentCap->AddStationAttachmentRecord(MakeRecord(*FString::Printf(TEXT("attachment-%02d"), Index), KitId)));
    }
    TestFalse(TEXT("Station attachment cap plus one is rejected"),
        AttachmentCap->AddStationAttachmentRecord(MakeRecord(TEXT("attachment-over-cap"), TEXT("ForgeAnvilKit"))));
    TestEqual(TEXT("Rejected attachment does not change the candidate"),
        AttachmentCap->GetRecords().Num(), UKalmalaConstructionSaveGameV2::MaxStationAttachments);

    FKalmalaCharcoalKilnState Process;
    for (int32 Index = 0; Index < FKalmalaCharcoalKilnState::InputCapacity; ++Index)
        TestTrue(TEXT("Kiln accepts Wood up to its input capacity"), Process.TryAddWood());
    TestFalse(TEXT("Kiln input rejects Wood above capacity"), Process.TryAddWood());
    TestTrue(TEXT("The first active second reserves one Wood and starts a job"), Process.Advance(1.0f));
    TestEqual(TEXT("Active job reserves exactly one Wood"), Process.InputWood, 9);
    TestTrue(TEXT("The job stores its remaining active-game time"), FMath::IsNearlyEqual(Process.JobRemainingSeconds, 29.0f));
    TestTrue(TEXT("The remaining 29 active seconds complete one job"), Process.Advance(29.0f));
    TestEqual(TEXT("A completed job produces exactly one Coal"), Process.OutputCoal, 1);
    FKalmalaCharcoalKilnState FullOutput;
    FullOutput.InputWood = 1;
    FullOutput.OutputCoal = FKalmalaCharcoalKilnState::OutputCapacity;
    TestFalse(TEXT("A full output pauses the queued input"), FullOutput.Advance(5.0f));
    TestEqual(TEXT("Output-full pause retains queued Wood"), FullOutput.InputWood, 1);
    return true;
}
#endif
