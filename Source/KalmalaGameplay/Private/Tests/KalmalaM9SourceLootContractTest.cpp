#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeContentContract.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9SourceLootContractTest,
    "Kalmala.Gameplay.M9.SecondWaveHarvestContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9SourceLootContractTest::RunTest(const FString& Parameters)
{
    struct FExpectedSource
    {
        FName SourceId;
        FName PresentationId;
        FName ItemId;
        int32 MaxStack;
        EKalmalaToolAction Action;
        EKalmalaToolKind Tool;
        EKalmalaToolTier MinimumTier;
    };
    const FExpectedSource Sources[] =
    {
        { TEXT("meadows-birch-trunk"), TEXT("birch-trunk-harvest"), TEXT("Lightwood"), 50,
            EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaToolTier::Bronze },
        { TEXT("elderwood-ironheart-trunk"), TEXT("ironheart-trunk-harvest"), TEXT("Densewood"), 50,
            EKalmalaToolAction::Woodcutting, EKalmalaToolKind::None, EKalmalaToolTier::Iron },
        { TEXT("mire-peat-amber-seam"), TEXT("peat-amber-seam"), TEXT("PeatAmber"), 40,
            EKalmalaToolAction::Mining, EKalmalaToolKind::StonePick, EKalmalaToolTier::None },
        { TEXT("tundra-frost-salt-deposit"), TEXT("tundra-salt-crystals"), TEXT("FrostSalt"), 40,
            EKalmalaToolAction::Mining, EKalmalaToolKind::StonePick, EKalmalaToolTier::None }
    };

    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    const FString SpawnId = TEXT("1/-3/4/123456");
    for (const FExpectedSource& Expected : Sources)
    {
        FName PrimaryItemId;
        FKalmalaToolServerSelection Selection;
        TestTrue(TEXT("Approved source belongs to the M9 allowlist"),
            FKalmalaM9SourceLootContract::IsM9Source(Expected.SourceId));
        TestTrue(TEXT("Approved source resolves its primary item"),
            FKalmalaM9SourceLootContract::GetPrimaryItemId(Expected.SourceId, PrimaryItemId));
        TestEqual(TEXT("Source resolves its approved item"), PrimaryItemId, Expected.ItemId);
        TestEqual(TEXT("Source resolves its approved presentation"),
            FKalmalaBiomeContentContract::GetGatheringPresentationId(Expected.SourceId), Expected.PresentationId);
        TestTrue(TEXT("Server harvest transaction builds the source selection"),
            FKalmalaToolLifecycleContract::BuildServerSelection(Expected.SourceId, Selection));
        TestEqual(TEXT("Server selection returns the approved item"), Selection.RewardItemId, Expected.ItemId);
        TestTrue(TEXT("Server selection uses the source action"), Selection.Action == Expected.Action);
        TestTrue(TEXT("Server selection uses the existing tool"), Selection.RequiredTool == Expected.Tool);
        TestTrue(TEXT("Source tool gate matches its catalogue rule"), Selection.MinimumToolTier == Expected.MinimumTier);

        const FKalmalaItemDefinition* Item = Catalogue != nullptr ? Catalogue->FindItem(Expected.ItemId) : nullptr;
        TestNotNull(TEXT("Approved material exists in the item catalogue"), Item);
        if (Item != nullptr)
        {
            TestEqual(TEXT("Approved material keeps its bounded stack cap"), Item->MaxStack, Expected.MaxStack);
            TestTrue(TEXT("One- and two-unit yields fit an empty catalogue stack"),
                Catalogue->IsValidStack(Expected.ItemId, 1) && Catalogue->IsValidStack(Expected.ItemId, 2));

            const TArray<FKalmalaInventoryStack> NearFullPack = { { Expected.ItemId, Expected.MaxStack - 1 } };
            TArray<FKalmalaInventoryStack> Candidate;
            FString Reason;
            TestTrue(TEXT("Guaranteed one-unit yield fits the last stack slot"),
                UKalmalaInventoryComponent::BuildGrant(NearFullPack, Expected.ItemId, 1, Candidate, Reason));
            Candidate.Reset();
            Reason.Reset();
            TestFalse(TEXT("Bonus yield is rejected when the complete two-unit grant will not fit"),
                UKalmalaInventoryComponent::BuildGrant(NearFullPack, Expected.ItemId, 2, Candidate, Reason));
            TestEqual(TEXT("Failed preflight leaves the owner's live snapshot untouched"),
                NearFullPack[0].Quantity, Expected.MaxStack - 1);
        }

        int32 Quantity = 0;
        TestTrue(TEXT("Approved source derives a bounded deterministic yield"),
            FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(WorldConfig, Expected.SourceId, SpawnId, Quantity));
        TestTrue(TEXT("Source yield is exactly one or two"),
            Quantity >= FKalmalaM9SourceLootContract::PrimaryYield && Quantity <= FKalmalaM9SourceLootContract::MaximumYield);
        int32 RepeatQuantity = 0;
        TestTrue(TEXT("Repeated same-world source query succeeds"),
            FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(WorldConfig, Expected.SourceId, SpawnId, RepeatQuantity));
        TestEqual(TEXT("Same world and population identity reproduce loot"), RepeatQuantity, Quantity);

        const FString ResourceId = FKalmalaM9SourceLootContract::BuildResourceDepletionId(Expected.SourceId, SpawnId);
        TestTrue(TEXT("Source depletion uses an admissible sparse resource identity"),
            ResourceId.StartsWith(TEXT("resource:m9:v1:")) && FKalmalaM7SparseDelta::IsValidStableId(ResourceId));
        TestTrue(TEXT("Source-specific sparse identity contains the population spawn key"), ResourceId.EndsWith(SpawnId));
    }

    TestEqual(TEXT("FNV-1a implementation matches the published 64-bit test vector"),
        FKalmalaM9SourceLootContract::HashAscii(TEXT("hello")), 0xa430d84680aabd0bull);
    FName ForgedItemId;
    TestFalse(TEXT("Unknown sources cannot choose an item"),
        FKalmalaM9SourceLootContract::GetPrimaryItemId(TEXT("forged-source"), ForgedItemId));
    TestTrue(TEXT("Unknown source lookup clears its output item"), ForgedItemId.IsNone());

    bool bSawGuaranteedOnly = false;
    bool bSawBonus = false;
    for (uint64 Seed = 0; Seed < 128 && !(bSawGuaranteedOnly && bSawBonus); ++Seed)
    {
        WorldConfig.WorldSeed = Seed;
        int32 Quantity = 0;
        if (FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(WorldConfig,
            TEXT("mire-peat-amber-seam"), SpawnId, Quantity))
        {
            bSawGuaranteedOnly |= Quantity == 1;
            bSawBonus |= Quantity == 2;
        }
    }
    TestTrue(TEXT("Bounded FNV outcome table exercises both no-bonus and bonus cases"), bSawGuaranteedOnly && bSawBonus);

    int32 RejectedQuantity = 7;
    TestFalse(TEXT("Forged source cannot derive a reward"), FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
        WorldConfig, TEXT("forged-source"), SpawnId, RejectedQuantity));
    TestEqual(TEXT("Rejected source clears reward quantity"), RejectedQuantity, 0);
    TestTrue(TEXT("Empty spawn identity cannot derive sparse depletion"),
        FKalmalaM9SourceLootContract::BuildResourceDepletionId(Sources[0].SourceId, FString()).IsEmpty());
    return true;
}

#endif
