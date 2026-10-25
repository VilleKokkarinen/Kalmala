#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerDiscoverySaveGame.h"

#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

namespace
{
    FKalmalaPlayerToolSaveRecord MakeToolRecord(const FName ItemId, const int32 ToolLevel, const int32 Condition)
    {
        FKalmalaPlayerToolSaveRecord Record;
        Record.ItemId = ItemId;
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
    "Kalmala.Gameplay.Discovery.Schema3CurrentFormat",
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
    TestTrue(TEXT("Schema 3 accepts a canonical first-wave discovery"), Current->AddDiscovery(PoiId));
    TestTrue(TEXT("A second world scroll location keeps its own discovery identity"),
        Current->AddDiscovery(TEXT("Scroll:1:mending:2,1:1")));
    TestTrue(TEXT("Schema 3 accepts a canonical M9 land claim"), Current->AddM9Claim(M9RillstoneId));
    TestTrue(TEXT("Schema 3 accepts a second independent M9 candidate"), Current->AddM9Claim(M9GrainId));
    TestTrue(TEXT("Schema 3 accepts an allowlisted learned effect"), Current->AddLearnedEffect(TEXT("Effect:mending")));
    TestTrue(TEXT("A distinct scroll discovery reuses an existing learned entitlement"),
        Current->EnsureLearnedEffect(TEXT("Effect:mending")));
    TestEqual(TEXT("Reusing an entitlement does not add another effect record"), Current->GetLearnedEffectIds().Num(), 1);
    TestFalse(TEXT("Reusing an unknown learned effect remains rejected"), Current->EnsureLearnedEffect(TEXT("Effect:unknown")));
    TestTrue(TEXT("Schema 3 accepts the canonical Stone Knife ID"), Current->AddToolRecord(MakeToolRecord(TEXT("StoneKnife"), 1, 9)));
    TestTrue(TEXT("Schema 3 accepts the authored Bronze Axe level and condition"), Current->AddToolRecord(MakeToolRecord(TEXT("BronzeAxe"), 1, 0)));
    TestTrue(TEXT("Schema 3 accepts the authored Iron Axe level and condition"), Current->AddToolRecord(MakeToolRecord(TEXT("IronAxe"), 2, 20)));
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
    TestTrue(TEXT("Schema 3 player facts serialize in memory"), Serialize(Current, CurrentBytes));
    UKalmalaPlayerDiscoverySaveGameV2* Reloaded =
        Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (TestNotNull(TEXT("Schema 3 player facts reload with their type"), Reloaded))
    {
        TestTrue(TEXT("Reloaded facts carry exact seed, revision, scope, and owner"), Reloaded->MatchesPlayer(World, PlayerIdentity));
        TestTrue(TEXT("First-wave and M9 discoveries both survive round-trip"),
            Reloaded->HasDiscovery(PoiId) && Reloaded->HasDiscovery(TEXT("Scroll:1:mending:2,1:1"))
            && Reloaded->HasDiscovery(M9RillstoneId) && Reloaded->HasDiscovery(M9GrainId));
        TestTrue(TEXT("Learned effects and carried tool progression survive round-trip"),
            Reloaded->HasLearnedEffect(TEXT("Effect:mending")) && Reloaded->GetToolRecords().Num() == 4
            && Reloaded->GetToolRecords()[1].ItemId == TEXT("BronzeAxe")
            && Reloaded->GetToolRecords()[2].ToolLevel == 2);
    }

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
    for (int32 Index = 0; Index < UKalmalaPlayerDiscoverySaveGameV2::MaxDiscoveries; ++Index)
    {
        TestTrue(TEXT("Legacy discovery records fit within their cap"), FullDiscoveries->AddDiscovery(MakeDiscoveryId(Index)));
    }
    TestFalse(TEXT("Legacy discovery cap plus one is rejected"), FullDiscoveries->AddDiscovery(MakeDiscoveryId(200)));
    TestEqual(TEXT("Rejected discovery leaves the capped candidate unchanged"),
        FullDiscoveries->GetDiscoveryIds().Num(), UKalmalaPlayerDiscoverySaveGameV2::MaxDiscoveries);

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
        FullTools->AddToolRecord(MakeToolRecord(AllTools[0].ItemId, AllTools[0].ToolLevel, AllTools[0].Condition)));
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
