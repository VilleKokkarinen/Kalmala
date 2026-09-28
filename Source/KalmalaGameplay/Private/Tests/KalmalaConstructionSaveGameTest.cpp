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
    Record.ConstructionId = TEXT("session-drying-line"); Record.KitId = TEXT("DryingLineKit");
    TestFalse(TEXT("M9 Drying Lines remain outside the existing save schema"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    TestTrue(TEXT("Drying Line session policy is explicit"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("DryingLineKit")));
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
    "Kalmala.Gameplay.Construction.Schema2Migration",
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
    const FKalmalaConstructionSaveRecord DryingLine = MakeRecord(TEXT("camp-drying-01"), TEXT("DryingLineKit"));
    TestTrue(TEXT("Schema 2 accepts an established camp record"), Save->AddRecord(Floor));
    TestTrue(TEXT("Schema 2 accepts a bounded station attachment"), Save->AddStationAttachmentRecord(Rack));
    TestTrue(TEXT("Schema 2 accepts an approved Drying Line record"), Save->AddDryingLineRecord(DryingLine));
    TestFalse(TEXT("Schema 2 rejects a duplicate stable construction ID across record kinds"),
        Save->AddDryingLineRecord(MakeRecord(TEXT("camp-rack-01"), TEXT("DryingLineKit"))));
    TestFalse(TEXT("Schema 2 rejects unapproved session-only camp records"),
        Save->AddDryingLineRecord(MakeRecord(TEXT("camp-unknown-01"), TEXT("SmokehouseKit"))));
    TestEqual(TEXT("Rejected schema-2 additions leave the live candidate unchanged"), Save->GetRecords().Num(), 3);

    TArray<uint8> CurrentBytes;
    TestTrue(TEXT("Schema 2 construction records serialize in memory"), Serialize(Save, CurrentBytes));
    UKalmalaConstructionSaveGameV2* Reloaded =
        Cast<UKalmalaConstructionSaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (TestNotNull(TEXT("Schema 2 construction records reload with their type"), Reloaded))
    {
        TestTrue(TEXT("Reloaded container carries exact world, revision, and world scope"), Reloaded->MatchesWorld(World));
        TestEqual(TEXT("Existing, attachment, and Drying Line records round-trip together"), Reloaded->GetRecords().Num(), 3);
        TestTrue(TEXT("Reloaded container retains its station attachment"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.KitId == TEXT("WorkbenchToolRackKit");
            }));
        TestTrue(TEXT("Reloaded container retains its Drying Line"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.KitId == TEXT("DryingLineKit");
            }));
    }

    UKalmalaConstructionSaveGame* Legacy = NewObject<UKalmalaConstructionSaveGame>(GetTransientPackage());
    Legacy->InitializeForWorld(World);
    TestTrue(TEXT("Schema 1 fixture stores its first legacy construction"), Legacy->AddRecord(Floor));
    TestTrue(TEXT("Schema 1 fixture stores its second legacy construction"),
        Legacy->AddRecord(MakeRecord(TEXT("camp-workbench-01"), TEXT("WorkbenchKit"))));
    TArray<uint8> LegacyBeforeMigration;
    TestTrue(TEXT("Legacy source serializes before migration"), Serialize(Legacy, LegacyBeforeMigration));
    UKalmalaConstructionSaveGameV2* Migrated = nullptr;
    TestTrue(TEXT("Matching schema 1 data migrates to schema 2"),
        UKalmalaConstructionSaveGameV2::TryMigrateSchema1(Legacy, World, GetTransientPackage(), Migrated));
    if (TestNotNull(TEXT("Migration returns a new candidate without replacing its source"), Migrated))
    {
        TestTrue(TEXT("Migrated facts bind to the current exact world identity"), Migrated->MatchesWorld(World));
        TestEqual(TEXT("Migration preserves every accepted schema 1 construction"), Migrated->GetRecords().Num(), 2);
        TestTrue(TEXT("Migration retains legacy construction identity and kit"),
            Migrated->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-workbench-01") && Record.KitId == TEXT("WorkbenchKit");
            }));
        TestFalse(TEXT("Absent attachments and Drying Lines start empty after migration"),
            Migrated->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.KitId == TEXT("WorkbenchToolRackKit") || Record.KitId == TEXT("ForgeAnvilKit")
                    || Record.KitId == TEXT("DryingLineKit");
            }));
    }
    TArray<uint8> LegacyAfterMigration;
    TestTrue(TEXT("Legacy source still serializes after migration"), Serialize(Legacy, LegacyAfterMigration));
    TestTrue(TEXT("Successful migration builds a candidate without mutating source bytes"),
        LegacyBeforeMigration == LegacyAfterMigration);

    const auto RejectWithoutChangingSource = [&](UKalmalaConstructionSaveGame* Invalid, const TCHAR* Label)
    {
        TArray<uint8> Before;
        TArray<uint8> After;
        TestTrue(FString::Printf(TEXT("%s source serializes before rejection"), Label), Serialize(Invalid, Before));
        UKalmalaConstructionSaveGameV2* Rejected = nullptr;
        TestFalse(FString::Printf(TEXT("%s migration is rejected"), Label),
            UKalmalaConstructionSaveGameV2::TryMigrateSchema1(Invalid, World, GetTransientPackage(), Rejected));
        TestNull(FString::Printf(TEXT("%s produces no replacement candidate"), Label), Rejected);
        TestTrue(FString::Printf(TEXT("%s source serializes after rejection"), Label), Serialize(Invalid, After));
        TestTrue(FString::Printf(TEXT("%s leaves source bytes unchanged"), Label), Before == After);
    };

    FKalmalaWorldGenerationConfig OtherWorld = World;
    ++OtherWorld.WorldSeed;
    UKalmalaConstructionSaveGame* WrongSeedSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    WrongSeedSource->WorldConfig = OtherWorld;
    RejectWithoutChangingSource(WrongSeedSource, TEXT("Wrong-seed"));
    UKalmalaConstructionSaveGame* WrongSchemaSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    WrongSchemaSource->SchemaVersion = 0;
    RejectWithoutChangingSource(WrongSchemaSource, TEXT("Schema-zero"));
    WrongSchemaSource->SchemaVersion = UKalmalaConstructionSaveGameV2::SchemaVersionValue + 1;
    RejectWithoutChangingSource(WrongSchemaSource, TEXT("Future-schema"));

    UKalmalaConstructionSaveGame* MalformedSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    MalformedSource->Records[0].Transform.SetScale3D(FVector(NAN, 1.0f, 1.0f));
    RejectWithoutChangingSource(MalformedSource, TEXT("Malformed-transform"));
    UKalmalaConstructionSaveGame* MalformedIdSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    MalformedIdSource->Records[0].ConstructionId.Reset();
    RejectWithoutChangingSource(MalformedIdSource, TEXT("Malformed-construction-ID"));
    UKalmalaConstructionSaveGame* DuplicateSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    const FKalmalaConstructionSaveRecord DuplicateLegacyRecord = DuplicateSource->Records[0];
    DuplicateSource->Records.Add(DuplicateLegacyRecord);
    RejectWithoutChangingSource(DuplicateSource, TEXT("Duplicate-ID"));
    UKalmalaConstructionSaveGame* M9InLegacySource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    M9InLegacySource->Records.Add(DryingLine);
    RejectWithoutChangingSource(M9InLegacySource, TEXT("Unapproved-schema-one-record"));
    UKalmalaConstructionSaveGame* OverCapSource = DuplicateObject<UKalmalaConstructionSaveGame>(Legacy, GetTransientPackage());
    OverCapSource->Records.Reset();
    for (int32 Index = 0; Index <= UKalmalaConstructionSaveGame::MaxRecords; ++Index)
    {
        OverCapSource->Records.Add(MakeRecord(*FString::Printf(TEXT("legacy-%03d"), Index), TEXT("FloorKit")));
    }
    RejectWithoutChangingSource(OverCapSource, TEXT("Over-cap"));

    UKalmalaConstructionSaveGameV2* WrongSeedCandidate = nullptr;
    TestFalse(TEXT("A valid legacy construction cannot migrate to a different seed"),
        UKalmalaConstructionSaveGameV2::TryMigrateSchema1(Legacy, OtherWorld, GetTransientPackage(), WrongSeedCandidate));
    TestNull(TEXT("Seed mismatch does not create a migration candidate"), WrongSeedCandidate);

    UKalmalaConstructionSaveGameV2* IdentityChecks = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    const FKalmalaM7SaveIdentity ExpectedIdentity = IdentityChecks->Identity;
    IdentityChecks->Identity.WorldSeed++;
    TestFalse(TEXT("Schema 2 rejects a mismatched identity seed"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.GeneratorRevision--;
    TestFalse(TEXT("Schema 2 rejects a mismatched generator revision"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.Scope = EKalmalaM7SaveScope::Player;
    IdentityChecks->Identity.OwnerIdentity = TEXT("provider:other");
    TestFalse(TEXT("Schema 2 rejects a mismatched persistence scope"), IdentityChecks->MatchesWorld(World));

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
    UKalmalaConstructionSaveGameV2* DryingCap = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    DryingCap->InitializeForWorld(World);
    for (int32 Index = 0; Index < UKalmalaConstructionSaveGameV2::MaxDryingLines; ++Index)
    {
        TestTrue(TEXT("Drying Line remains within its accepted cap"),
            DryingCap->AddDryingLineRecord(MakeRecord(*FString::Printf(TEXT("drying-%02d"), Index), TEXT("DryingLineKit"))));
    }
    TestFalse(TEXT("Drying Line cap plus one is rejected"),
        DryingCap->AddDryingLineRecord(MakeRecord(TEXT("drying-over-cap"), TEXT("DryingLineKit"))));
    TestEqual(TEXT("Rejected Drying Line does not change the candidate"),
        DryingCap->GetRecords().Num(), UKalmalaConstructionSaveGameV2::MaxDryingLines);

    return true;
}
#endif
