#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerDiscoverySaveGame.h"

#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

namespace
{
    FKalmalaPlayerToolSaveRecord MakeToolRecord(const FName ToolId, const int32 ToolLevel, const int32 Condition)
    {
        FKalmalaPlayerToolSaveRecord Record;
        Record.ToolId = ToolId;
        Record.ToolLevel = ToolLevel;
        Record.Condition = Condition;
        Record.InstanceGuid = FGuid::NewGuid();
        return Record;
    }

    FString MakeDiscoveryId(const int32 Index)
    {
        return FString::Printf(TEXT("Poi:1:cache-%d:%d,%d:%d"), Index, Index - 40, 18 - Index, Index % 8);
    }

    FString MakeM9ClaimId(const FName CandidateId, const int32 Index)
    {
        return FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
            CandidateId, FIntPoint(Index - 32, 17 - Index));
    }

    bool Serialize(USaveGame* Save, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();
        return UGameplayStatics::SaveGameToMemory(Save, OutBytes);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerDiscoverySaveGameV2Test,
    "Kalmala.Gameplay.Discovery.Schema2Migration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlayerDiscoverySaveGameV2Test::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;
    const FString PlayerIdentity(TEXT("provider:player-alpha"));
    const FString PoiId(TEXT("Poi:1:lakes-island-cache:-2,4:1"));
    const FString ScrollId(TEXT("Scroll:1:mending:0,0:0"));
    const FString M9RillstoneId = MakeM9ClaimId(TEXT("lakes-rillworn-marker"), 3);
    const FString M9GrainId = MakeM9ClaimId(TEXT("mountains-leeward-grain"), 5);

    UKalmalaPlayerDiscoverySaveGameV2* Current = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(GetTransientPackage());
    Current->InitializeForPlayer(World, PlayerIdentity);
    TestTrue(TEXT("Schema 2 accepts a canonical first-wave discovery"), Current->AddDiscovery(PoiId));
    TestTrue(TEXT("Schema 2 accepts a canonical M9 land claim"), Current->AddM9Claim(M9RillstoneId));
    TestTrue(TEXT("Schema 2 accepts a second independent M9 candidate"), Current->AddM9Claim(M9GrainId));
    TestTrue(TEXT("Schema 2 accepts an allowlisted learned effect"), Current->AddLearnedEffect(TEXT("Effect:mending")));
    TestTrue(TEXT("Schema 2 accepts an old carried starter ID"), Current->AddToolRecord(MakeToolRecord(TEXT("ReedKnife"), 1, 9)));
    TestTrue(TEXT("Schema 2 accepts the authored Bronze Axe level and condition"), Current->AddToolRecord(MakeToolRecord(TEXT("BronzeAxe"), 1, 0)));
    TestTrue(TEXT("Schema 2 accepts the authored Iron Axe level and condition"), Current->AddToolRecord(MakeToolRecord(TEXT("IronAxe"), 2, 20)));
    TestFalse(TEXT("A duplicate first-wave discovery is rejected without mutation"), Current->AddDiscovery(PoiId));
    TestFalse(TEXT("A duplicate M9 claim is rejected without mutation"), Current->AddM9Claim(M9RillstoneId));
    TestFalse(TEXT("A malformed M9 identity is rejected"), Current->AddM9Claim(TEXT("land-discovery:m9:1:unknown:0,0")));
    TestTrue(TEXT("A second tool of the same type is accepted as a separate item instance"),
        Current->AddToolRecord(MakeToolRecord(TEXT("IronAxe"), 2, 20)));
    TestFalse(TEXT("A repeated instance GUID is rejected"), Current->AddToolRecord(Current->GetToolRecords()[0]));
    TestFalse(TEXT("An unapproved learned effect is rejected"), Current->AddLearnedEffect(TEXT("Effect:unknown")));
    TestEqual(TEXT("Only the duplicate GUID rejection leaves the candidate unchanged"), Current->GetToolRecords().Num(), 4);

    UKalmalaPlayerDiscoverySaveGameV2* ToolReplacement = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(
        Current, GetTransientPackage());
    TArray<FKalmalaPlayerToolSaveRecord> InvalidReplacement = { MakeToolRecord(TEXT("IronAxe"), 21, 20) };
    TestFalse(TEXT("Invalid complete tool replacement is rejected"), ToolReplacement->TryReplaceToolRecords(InvalidReplacement));
    TestEqual(TEXT("Rejected tool replacement preserves the original records"), ToolReplacement->GetToolRecords().Num(), 4);
    TestEqual(TEXT("Rejected tool replacement preserves the original Iron Axe level"),
        ToolReplacement->GetToolRecords()[2].ToolLevel, 2);

    TArray<uint8> CurrentBytes;
    TestTrue(TEXT("Schema 2 player facts serialize in memory"), Serialize(Current, CurrentBytes));
    UKalmalaPlayerDiscoverySaveGameV2* Reloaded =
        Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (TestNotNull(TEXT("Schema 2 player facts reload with their type"), Reloaded))
    {
        TestTrue(TEXT("Reloaded facts carry exact seed, revision, scope, and owner"), Reloaded->MatchesPlayer(World, PlayerIdentity));
        TestTrue(TEXT("First-wave and M9 discoveries both survive round-trip"),
            Reloaded->HasDiscovery(PoiId) && Reloaded->HasDiscovery(M9RillstoneId) && Reloaded->HasDiscovery(M9GrainId));
        TestTrue(TEXT("Learned effects and carried tool progression survive round-trip"),
            Reloaded->HasLearnedEffect(TEXT("Effect:mending")) && Reloaded->GetToolRecords().Num() == 3
            && Reloaded->GetToolRecords()[1].ToolId == TEXT("BronzeAxe")
            && Reloaded->GetToolRecords()[2].ToolLevel == 2);
    }

    UKalmalaPlayerDiscoverySaveGame* Legacy = NewObject<UKalmalaPlayerDiscoverySaveGame>(GetTransientPackage());
    Legacy->InitializeForPlayer(World, PlayerIdentity);
    TestTrue(TEXT("Schema 1 fixture stores an existing point-of-interest claim"), Legacy->AddDiscovery(PoiId));
    TestTrue(TEXT("Schema 1 fixture stores an existing scroll claim"), Legacy->AddDiscovery(ScrollId));
    for (const TCHAR* EffectId : { TEXT("Effect:mending"), TEXT("Effect:hearth-shield"), TEXT("Effect:bears-vigor"), TEXT("Effect:deer-call") })
    {
        TestTrue(TEXT("Schema 1 fixture stores each allowlisted learned effect"), Legacy->AddLearnedEffect(EffectId));
    }
    TArray<uint8> LegacyBeforeMigration;
    TestTrue(TEXT("Schema 1 source serializes before migration"), Serialize(Legacy, LegacyBeforeMigration));
    UKalmalaPlayerDiscoverySaveGameV2* Migrated = nullptr;
    TestTrue(TEXT("Matching schema 1 player facts migrate to a schema 2 candidate"),
        UKalmalaPlayerDiscoverySaveGameV2::TryMigrateSchema1(
            Legacy, World, PlayerIdentity, GetTransientPackage(), Migrated));
    if (TestNotNull(TEXT("Migration returns a distinct candidate"), Migrated))
    {
        TestTrue(TEXT("Migrated facts bind to exact current player identity"), Migrated->MatchesPlayer(World, PlayerIdentity));
        TestTrue(TEXT("Migration preserves the legacy discovery and effect sets"),
            Migrated->HasDiscovery(PoiId) && Migrated->HasDiscovery(ScrollId)
            && Migrated->GetLearnedEffectIds().Num() == UKalmalaPlayerDiscoverySaveGameV2::MaxLearnedEffects);
        TestTrue(TEXT("Schema 1 fields absent from legacy saves begin empty"),
            Migrated->GetM9ClaimIds().IsEmpty() && Migrated->GetToolRecords().IsEmpty());
    }
    TArray<uint8> LegacyAfterMigration;
    TestTrue(TEXT("Schema 1 source still serializes after migration"), Serialize(Legacy, LegacyAfterMigration));
    TestTrue(TEXT("Successful migration leaves the source save bytes unchanged"), LegacyBeforeMigration == LegacyAfterMigration);

    const auto RejectMigrationWithoutChangingSource = [&](UKalmalaPlayerDiscoverySaveGame* Invalid, const TCHAR* Label)
    {
        TArray<uint8> Before;
        TArray<uint8> After;
        TestTrue(FString::Printf(TEXT("%s source serializes before rejection"), Label), Serialize(Invalid, Before));
        UKalmalaPlayerDiscoverySaveGameV2* Rejected = nullptr;
        TestFalse(FString::Printf(TEXT("%s migration is rejected"), Label),
            UKalmalaPlayerDiscoverySaveGameV2::TryMigrateSchema1(
                Invalid, World, PlayerIdentity, GetTransientPackage(), Rejected));
        TestNull(FString::Printf(TEXT("%s produces no candidate"), Label), Rejected);
        TestTrue(FString::Printf(TEXT("%s source serializes after rejection"), Label), Serialize(Invalid, After));
        TestTrue(FString::Printf(TEXT("%s rejection preserves source bytes"), Label), Before == After);
    };

    FKalmalaWorldGenerationConfig OtherWorld = World;
    ++OtherWorld.WorldSeed;
    UKalmalaPlayerDiscoverySaveGameV2* WrongSeedMigration = nullptr;
    TestFalse(TEXT("Schema 1 cannot migrate to a different world seed"),
        UKalmalaPlayerDiscoverySaveGameV2::TryMigrateSchema1(Legacy, OtherWorld, PlayerIdentity, GetTransientPackage(), WrongSeedMigration));
    TestNull(TEXT("World mismatch creates no migration candidate"), WrongSeedMigration);
    UKalmalaPlayerDiscoverySaveGame* WrongSeedSource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    WrongSeedSource->WorldConfig = OtherWorld;
    RejectMigrationWithoutChangingSource(WrongSeedSource, TEXT("Wrong-seed"));
    UKalmalaPlayerDiscoverySaveGame* WrongPlayerSource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    WrongPlayerSource->PlayerIdentity = TEXT("provider:player-beta");
    RejectMigrationWithoutChangingSource(WrongPlayerSource, TEXT("Wrong-player"));
    UKalmalaPlayerDiscoverySaveGame* InvalidOwnerSource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    InvalidOwnerSource->PlayerIdentity = FString::ChrN(FKalmalaM7SaveIdentity::MaxOwnerIdentityLength + 1, TEXT('p'));
    RejectMigrationWithoutChangingSource(InvalidOwnerSource, TEXT("Overlong-player"));
    UKalmalaPlayerDiscoverySaveGame* WrongSchemaSource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    WrongSchemaSource->SchemaVersion = 0;
    RejectMigrationWithoutChangingSource(WrongSchemaSource, TEXT("Schema-zero"));
    WrongSchemaSource->SchemaVersion = UKalmalaPlayerDiscoverySaveGameV2::SchemaVersionValue + 1;
    RejectMigrationWithoutChangingSource(WrongSchemaSource, TEXT("Future-schema"));
    UKalmalaPlayerDiscoverySaveGame* MalformedDiscoverySource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    MalformedDiscoverySource->DiscoveryIds.Add(TEXT("Poi:1:not a stable id"));
    RejectMigrationWithoutChangingSource(MalformedDiscoverySource, TEXT("Malformed-discovery"));
    UKalmalaPlayerDiscoverySaveGame* MalformedEffectSource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    MalformedEffectSource->LearnedEffectIds.Add(TEXT("Effect:unknown"));
    RejectMigrationWithoutChangingSource(MalformedEffectSource, TEXT("Malformed-effect"));
    UKalmalaPlayerDiscoverySaveGame* DuplicateDiscoverySource = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    DuplicateDiscoverySource->DiscoveryIds.Add(PoiId);
    TestEqual(TEXT("Legacy discovery set represents duplicates as one stable fact"), DuplicateDiscoverySource->DiscoveryIds.Num(), Legacy->DiscoveryIds.Num());

    UKalmalaPlayerDiscoverySaveGame* OverCapLegacy = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    OverCapLegacy->DiscoveryIds.Reset();
    for (int32 Index = 0; Index <= UKalmalaPlayerDiscoverySaveGameV2::MaxLegacyDiscoveries; ++Index)
    {
        OverCapLegacy->DiscoveryIds.Add(MakeDiscoveryId(Index));
    }
    RejectMigrationWithoutChangingSource(OverCapLegacy, TEXT("Over-cap-discoveries"));
    UKalmalaPlayerDiscoverySaveGame* OverCapEffects = DuplicateObject<UKalmalaPlayerDiscoverySaveGame>(Legacy, GetTransientPackage());
    OverCapEffects->LearnedEffectIds.Add(TEXT("Effect:invalid-extra"));
    RejectMigrationWithoutChangingSource(OverCapEffects, TEXT("Over-cap-learned-effects"));

    const auto RejectCurrentCandidate = [&](UKalmalaPlayerDiscoverySaveGameV2* Invalid, const TCHAR* Label)
    {
        TestFalse(FString::Printf(TEXT("%s current candidate is rejected"), Label), Invalid->MatchesPlayer(World, PlayerIdentity));
    };
    UKalmalaPlayerDiscoverySaveGameV2* IdentityMismatch = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    IdentityMismatch->Identity.WorldSeed++;
    RejectCurrentCandidate(IdentityMismatch, TEXT("Wrong-seed-identity"));
    IdentityMismatch = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    IdentityMismatch->Identity.GeneratorRevision++;
    RejectCurrentCandidate(IdentityMismatch, TEXT("Wrong-revision"));
    IdentityMismatch = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    IdentityMismatch->Identity.Scope = EKalmalaM7SaveScope::World;
    IdentityMismatch->Identity.OwnerIdentity.Reset();
    RejectCurrentCandidate(IdentityMismatch, TEXT("Wrong-scope"));
    IdentityMismatch = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    IdentityMismatch->Identity.OwnerIdentity = TEXT("provider:player-beta");
    RejectCurrentCandidate(IdentityMismatch, TEXT("Wrong-player-identity"));
    UKalmalaPlayerDiscoverySaveGameV2* InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    InvalidCurrent->SchemaVersion = 0;
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-schema-zero"));
    InvalidCurrent->SchemaVersion = UKalmalaPlayerDiscoverySaveGameV2::SchemaVersionValue + 1;
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-future-schema"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    InvalidCurrent->DiscoveryIds.Add(TEXT("malformed id"));
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-malformed-discovery"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    InvalidCurrent->M9ClaimIds[0] = TEXT("land-discovery:m9:1:unknown:0,0");
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-malformed-claim"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    const FString DuplicateClaimId = InvalidCurrent->M9ClaimIds[0];
    InvalidCurrent->M9ClaimIds.Add(DuplicateClaimId);
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-duplicate-claim"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    const FKalmalaPlayerToolSaveRecord DuplicateToolRecord = InvalidCurrent->ToolRecords[0];
    InvalidCurrent->ToolRecords.Add(DuplicateToolRecord);
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-duplicate-tool"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    InvalidCurrent->ToolRecords[0].Condition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StoneKnife"))->MaxDurability + 1;
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-over-condition"));
    InvalidCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Current, GetTransientPackage());
    InvalidCurrent->ToolRecords[1].ToolLevel = 21;
    RejectCurrentCandidate(InvalidCurrent, TEXT("Current-wrong-tool-level"));

    UKalmalaPlayerDiscoverySaveGameV2* FullDiscoveries = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(GetTransientPackage());
    FullDiscoveries->InitializeForPlayer(World, PlayerIdentity);
    for (int32 Index = 0; Index < UKalmalaPlayerDiscoverySaveGameV2::MaxLegacyDiscoveries; ++Index)
    {
        TestTrue(TEXT("Legacy discovery records fit within their cap"), FullDiscoveries->AddDiscovery(MakeDiscoveryId(Index)));
    }
    TestFalse(TEXT("Legacy discovery cap plus one is rejected"), FullDiscoveries->AddDiscovery(MakeDiscoveryId(200)));
    TestEqual(TEXT("Rejected discovery leaves the capped candidate unchanged"),
        FullDiscoveries->GetDiscoveryIds().Num(), UKalmalaPlayerDiscoverySaveGameV2::MaxLegacyDiscoveries);

    UKalmalaPlayerDiscoverySaveGameV2* FullClaims = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(GetTransientPackage());
    FullClaims->InitializeForPlayer(World, PlayerIdentity);
    for (int32 Index = 0; Index < UKalmalaPlayerDiscoverySaveGameV2::MaxM9Claims; ++Index)
    {
        TestTrue(TEXT("M9 claims fit within their separate cap"),
            FullClaims->AddM9Claim(MakeM9ClaimId(Index % 2 == 0 ? FName(TEXT("lakes-rillworn-marker")) : FName(TEXT("mountains-leeward-grain")), Index)));
    }
    TestFalse(TEXT("M9 claim cap plus one is rejected"), FullClaims->AddM9Claim(MakeM9ClaimId(TEXT("lakes-rillworn-marker"), 100)));
    TestEqual(TEXT("Rejected M9 claim leaves the capped candidate unchanged"),
        FullClaims->GetM9ClaimIds().Num(), UKalmalaPlayerDiscoverySaveGameV2::MaxM9Claims);

    UKalmalaPlayerDiscoverySaveGameV2* FullTools = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(GetTransientPackage());
    FullTools->InitializeForPlayer(World, PlayerIdentity);
    const TArray<FKalmalaPlayerToolSaveRecord> AllTools = {
        MakeToolRecord(TEXT("StoneKnife"), 1, 16), MakeToolRecord(TEXT("StoneAxe"), 1, 24),
        MakeToolRecord(TEXT("StonePick"), 1, 20), MakeToolRecord(TEXT("BronzeAxe"), 1, 24),
        MakeToolRecord(TEXT("IronAxe"), 2, 24), MakeToolRecord(TEXT("ConstructionHammer"), 1, 100)
    };
    for (const FKalmalaPlayerToolSaveRecord& Tool : AllTools)
    {
        TestTrue(TEXT("Carried tool instances are accepted independently by their GUIDs"), FullTools->AddToolRecord(Tool));
    }
    TestTrue(TEXT("A repeated non-hammer item type is accepted with a new GUID"),
        FullTools->AddToolRecord(MakeToolRecord(AllTools[0].ToolId, AllTools[0].ToolLevel, AllTools[0].Condition)));
    TestEqual(TEXT("All seven unique tool instances are retained"), FullTools->GetToolRecords().Num(), 7);
    TestFalse(TEXT("Reusing an existing item GUID is rejected"), FullTools->AddToolRecord(AllTools[0]));

    UKalmalaPlayerDiscoverySaveGameV2* MultipleHammers = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(GetTransientPackage());
    MultipleHammers->InitializeForPlayer(World, PlayerIdentity);
    for (int32 Index = 0; Index < UKalmalaPlayerDiscoverySaveGameV2::MaxInventorySlots; ++Index)
    {
        TestTrue(TEXT("Hammer instances can fill each ordinary inventory slot"),
            MultipleHammers->AddToolRecord(MakeToolRecord(TEXT("ConstructionHammer"), 1, 100)));
    }
    TestEqual(TEXT("All carried hammer copies are retained"), MultipleHammers->GetToolRecords().Num(),
        UKalmalaPlayerDiscoverySaveGameV2::MaxInventorySlots);
    TestTrue(TEXT("A full repeated-hammer inventory still matches its player save"),
        MultipleHammers->MatchesPlayer(World, PlayerIdentity));
    TestFalse(TEXT("A forty-first item cannot exceed the shared inventory capacity"),
        MultipleHammers->AddToolRecord(MakeToolRecord(TEXT("ConstructionHammer"), 1, 100)));
    TArray<FKalmalaPlayerToolSaveRecord> HammerReplacement;
    HammerReplacement.Add(MakeToolRecord(TEXT("ConstructionHammer"), 1, 100));
    HammerReplacement.Add(MakeToolRecord(TEXT("ConstructionHammer"), 1, 50));
    TestTrue(TEXT("Complete carried-tool replacements preserve repeated hammer copies"),
        MultipleHammers->TryReplaceToolRecords(HammerReplacement));
    TArray<uint8> HammerBytes;
    TestTrue(TEXT("Repeated hammer copies serialize in the existing tool-record array"),
        Serialize(MultipleHammers, HammerBytes));
    UKalmalaPlayerDiscoverySaveGameV2* ReloadedHammers =
        Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(HammerBytes));
    if (TestNotNull(TEXT("Repeated hammer copies reload with their save type"), ReloadedHammers))
    {
        TestTrue(TEXT("Repeated hammer copies survive the existing save format"),
            ReloadedHammers->MatchesPlayer(World, PlayerIdentity)
            && ReloadedHammers->GetToolRecords().Num() == 2
            && ReloadedHammers->GetToolRecords()[1].Condition == 50);
    }
    TestFalse(TEXT("Unknown tool IDs are rejected"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(MakeToolRecord(TEXT("UnknownTool"), 1, 0)));
    TestFalse(TEXT("Negative tool condition is rejected"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(MakeToolRecord(TEXT("StoneKnife"), 1, -1)));
    TestTrue(TEXT("Zero-reward items can retain level twenty without station recipes"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(MakeToolRecord(TEXT("IronAxe"), 20, 24)));
    TestTrue(TEXT("Reward-derived level twenty accepts its increased durability"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(MakeToolRecord(TEXT("BronzeAxe"), 20, 1000)));
    TestFalse(TEXT("Item levels above the hard maximum are rejected"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(MakeToolRecord(TEXT("IronAxe"), 21, 24)));

    UKalmalaPlayerDiscoverySaveGameV2* OverCapCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(FullClaims, GetTransientPackage());
    OverCapCurrent->M9ClaimIds.Add(MakeM9ClaimId(TEXT("lakes-rillworn-marker"), 101));
    RejectCurrentCandidate(OverCapCurrent, TEXT("Current-over-cap-claims"));
    OverCapCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(FullDiscoveries, GetTransientPackage());
    OverCapCurrent->DiscoveryIds.Add(MakeDiscoveryId(201));
    RejectCurrentCandidate(OverCapCurrent, TEXT("Current-over-cap-discoveries"));
    OverCapCurrent = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(FullTools, GetTransientPackage());
    const FKalmalaPlayerToolSaveRecord OverCapToolRecord = AllTools[0];
    OverCapCurrent->ToolRecords.Add(OverCapToolRecord);
    RejectCurrentCandidate(OverCapCurrent, TEXT("Current-duplicate-tool-instance-guid"));

    return true;
}
#endif
