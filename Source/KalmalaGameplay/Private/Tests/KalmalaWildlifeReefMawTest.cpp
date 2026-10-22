#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeReefMawContractTest,
    "Kalmala.Gameplay.Wildlife.ReefMaw.OceanBiteAndRewardClaim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeReefMawContractTest::RunTest(const FString& Parameters)
{
    int32 ReefMawSlots = 0;
    for (uint64 Seed = 0; Seed < 1000; ++Seed)
    {
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        const bool bEligible = AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            Seed, EKalmalaBiome::Ocean, Archetype);
        if (Seed % 10ull == 9ull)
        {
            TestTrue(TEXT("The rare Ocean slot selects Reef Maw"), bEligible);
            TestEqual(TEXT("Reef Maw occupies the final Ocean slot"), Archetype,
                EKalmalaWildlifeArchetype::ReefMaw);
            ++ReefMawSlots;
        }
        else
        {
            TestFalse(TEXT("The other Ocean slots remain empty"), bEligible);
        }
    }
    TestEqual(TEXT("Reef Maw receives ten percent of Ocean candidates"), ReefMawSlots, 100);

    for (const EKalmalaBiome Biome : { EKalmalaBiome::Meadows, EKalmalaBiome::Elderwood,
             EKalmalaBiome::ShimmeringLakes, EKalmalaBiome::MossyMire,
             EKalmalaBiome::FreezingTundra, EKalmalaBiome::ThunderMountains })
    {
        for (uint64 Seed = 0; Seed < 10; ++Seed)
        {
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::ReefMaw;
            if (AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Seed, Biome, Archetype))
                TestFalse(TEXT("Reef Maw is exclusive to Ocean"), Archetype == EKalmalaWildlifeArchetype::ReefMaw);
        }
    }

    TestEqual(TEXT("Reef Maw retains 180 health"),
        AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype::ReefMaw), 180.0f);
    TestEqual(TEXT("Reef Maw retains 2 armor"),
        AKalmalaWildlifeSpawn::GetArmorForArchetype(EKalmalaWildlifeArchetype::ReefMaw), 2.0f);
    TestEqual(TEXT("Reef Maw uses the reviewed 50 Stab bite"),
        AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(
            EKalmalaWildlifeArchetype::ReefMaw).Stab, 50.0f);
    TestEqual(TEXT("The baseline defeat reward contains three Bone Fragments"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::ReefMaw), 3);
    TestEqual(TEXT("Archetypes without a Bone reward remain unchanged"),
        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype::MosquitoSwarm), 0);

    TestTrue(TEXT("Ocean water at the 150 cm habitat boundary is accepted"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean, false, 150.0f));
    TestFalse(TEXT("Shallower water is rejected"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean, false, 149.99f));
    TestFalse(TEXT("Shimmering Lake water is not Ocean habitat"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean, true, 500.0f));
    TestFalse(TEXT("Land biomes are rejected as Reef Maw habitat"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Elderwood, false, 500.0f));
    TestFalse(TEXT("Non-finite water depth fails closed"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean, false,
            std::numeric_limits<float>::quiet_NaN()));

    TestTrue(TEXT("The server accepts a swimming target at the inclusive 180 cm bite reach"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, true, true, 180.0f));
    TestFalse(TEXT("The bite rejects targets beyond 180 cm"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, true, true, 180.01f));
    TestFalse(TEXT("The bite rejects a target that is not swimming"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, true, false, 100.0f));
    TestFalse(TEXT("Clients cannot author Reef Maw damage"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(false, true, true, 100.0f));
    TestFalse(TEXT("Invalid targets cannot receive the bite"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, false, true, 100.0f));
    TestFalse(TEXT("Negative target distances fail closed"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, true, true, -1.0f));
    TestFalse(TEXT("Non-finite target distances fail closed"),
        AKalmalaWildlifeSpawn::IsReefMawBiteInReach(true, true, true,
            std::numeric_limits<float>::infinity()));
    TestEqual(TEXT("The bite telegraph lasts one second"),
        AKalmalaWildlifeSpawn::ReefMawTelegraphSeconds, 1.0f);
    TestEqual(TEXT("The bite cue window is short"),
        AKalmalaWildlifeSpawn::ReefMawBiteCueSeconds, 0.16f);
    TestEqual(TEXT("The bite recovery lasts 1.2 seconds"),
        AKalmalaWildlifeSpawn::ReefMawRecoverySeconds, 1.2f);
    TestEqual(TEXT("The hard enemy remains within its 25 m leash"),
        AKalmalaWildlifeSpawn::ReefMawSpawnLeashCentimetres, 2500.0f);
    TestFalse(TEXT("Hard enemies cannot spawn inside 150 m of the generated start"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(true, true, 14999.0f));
    TestTrue(TEXT("The hard-enemy start exclusion boundary is inclusive"),
        AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(true, true, 15000.0f));

    const FString SpawnId(TEXT("-8/3/7/reefmaw-712"));
    const FString ClaimId = UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(SpawnId, true);
    FString DecodedSpawnId;
    bool bDecodedHasReefTooth = false;
    TestTrue(TEXT("The durable claim fits the stable-ID limit"), !ClaimId.IsEmpty() && ClaimId.Len() <= 128);
    TestTrue(TEXT("The claim decodes the stable spawn ID"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            ClaimId, DecodedSpawnId, bDecodedHasReefTooth));
    TestEqual(TEXT("The decoded claim retains the defeated spawn ID"), DecodedSpawnId, SpawnId);
    TestTrue(TEXT("The claim stores the successful Reef Tooth roll"), bDecodedHasReefTooth);
    TestTrue(TEXT("Empty spawn IDs are rejected"),
        UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(FString(), true).IsEmpty());
    const FString NoToothSpawnId(TEXT("-8/3/7/reefmaw-713"));
    const FString NoToothClaimId = UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(
        NoToothSpawnId, false);
    bool bDecodedNoTooth = true;
    TestTrue(TEXT("A failed roll has a durable claim encoding"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            NoToothClaimId, DecodedSpawnId, bDecodedNoTooth));
    TestEqual(TEXT("The no-Tooth claim retains its spawn ID"), DecodedSpawnId, NoToothSpawnId);
    TestFalse(TEXT("The claim stores the failed Reef Tooth roll"), bDecodedNoTooth);
    bool bLegacyHasTooth = true;
    TestTrue(TEXT("An older unsuffixed Reef Maw claim remains readable"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            FString(TEXT("reefmaw-reward|")) + SpawnId, DecodedSpawnId, bLegacyHasTooth));
    TestEqual(TEXT("The legacy claim retains the original spawn ID"), DecodedSpawnId, SpawnId);
    TestFalse(TEXT("Legacy claims remain baseline-only"), bLegacyHasTooth);
    TestFalse(TEXT("A Sentinel claim cannot decode as a Reef Maw claim"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            TEXT("thundersentinel-reward|1/2/3"), DecodedSpawnId, bDecodedHasReefTooth));
    TestFalse(TEXT("Unknown Reef Maw claim suffixes are rejected"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            FString(TEXT("reefmaw-reward|")) + SpawnId + TEXT("|unknown"),
            DecodedSpawnId, bDecodedHasReefTooth));

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 431;
    UKalmalaWorldPopulationSaveGame* Population = NewObject<UKalmalaWorldPopulationSaveGame>();
    TestNotNull(TEXT("Reef Maw reward escrow can be created"), Population);
    if (Population == nullptr) return false;
    Population->InitializeForWorld(WorldConfig);
    Population->MarkDefeated(SpawnId);
    TestTrue(TEXT("A credited defeat queues its stored Bone and Reef Tooth outcome for its owner"),
        Population->TryQueuePendingReefMawReward(SpawnId, TEXT("player:test"), true));
    TestTrue(TEXT("Retrying the same owner's claim retains its first roll"),
        Population->TryQueuePendingReefMawReward(SpawnId, TEXT("player:test"), false));
    TestFalse(TEXT("A different player cannot take the pending reward"),
        Population->TryQueuePendingReefMawReward(SpawnId, TEXT("player:other"), true));
    TestEqual(TEXT("Only one Reef Maw reward row is queued"), Population->GetPendingWildlifeRewards().Num(), 1);

    TArray<uint8> Bytes;
    TestTrue(TEXT("The Reef Maw reward claim serializes with the population save"),
        UGameplayStatics::SaveGameToMemory(Population, Bytes));
    UKalmalaWorldPopulationSaveGame* Reloaded =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    TestNotNull(TEXT("The pending Reef Maw claim reloads"), Reloaded);
    if (Reloaded == nullptr || Reloaded->GetPendingWildlifeRewards().IsEmpty()) return false;
    TestTrue(TEXT("Reload retains the defeated spawn identity"), Reloaded->IsDefeated(SpawnId));
    TestEqual(TEXT("Reload retains the stable claim"),
        Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId, ClaimId);
    TestEqual(TEXT("Reload retains the credited owner"),
        Reloaded->GetPendingWildlifeRewards()[0].PlayerIdentity, FString(TEXT("player:test")));

    FString ReloadedSpawnId;
    bool bReloadedHasReefTooth = false;
    TestTrue(TEXT("Reload decodes the stored Reef Tooth outcome"),
        UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            Reloaded->GetPendingWildlifeRewards()[0].PersistentSpawnId,
            ReloadedSpawnId, bReloadedHasReefTooth));
    TestTrue(TEXT("Reload retains the successful Reef Tooth roll"), bReloadedHasReefTooth);

    TArray<FKalmalaInventoryStack> EmptyInventory;
    TArray<FKalmalaInventoryStack> WithBone;
    TArray<FKalmalaInventoryStack> Candidate;
    FString FailureReason;
    TestTrue(TEXT("The three-Bone Reef Maw baseline fits an empty inventory"),
        UKalmalaInventoryComponent::BuildGrant(EmptyInventory, TEXT("BoneFragment"), 3, WithBone, FailureReason));
    TestTrue(TEXT("The successful Reef Tooth roll joins the same reward candidate"),
        UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("ReefTooth"), 1, Candidate, FailureReason));
    const FKalmalaInventoryStack* BoneStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("BoneFragment"); });
    const FKalmalaInventoryStack* ToothStack = Candidate.FindByPredicate(
        [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("ReefTooth"); });
    TestTrue(TEXT("The accepted Reef Maw baseline is exactly three Bone Fragments"),
        BoneStack != nullptr && BoneStack->Quantity == 3);
    TestTrue(TEXT("The successful material roll awards exactly one Reef Tooth"),
        ToothStack != nullptr && ToothStack->Quantity == 1);

    TArray<FKalmalaInventoryStack> FullPack;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (Catalogue != nullptr)
    {
        for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
        {
            if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment")
                || Definition.ItemId == TEXT("ReefTooth") || Definition.MaxStack <= 0)
                continue;
            FKalmalaInventoryStack Stack;
            Stack.ItemId = Definition.ItemId;
            Stack.Quantity = 1;
            FullPack.Add(MoveTemp(Stack));
            if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots) break;
        }
    }
    TestEqual(TEXT("The capacity fixture fills all forty inventory cells"),
        FullPack.Num(), UKalmalaInventoryComponent::MaxSlots);
    if (FullPack.Num() == UKalmalaInventoryComponent::MaxSlots)
    {
        TArray<FKalmalaInventoryStack> OneCellPack = FullPack;
        OneCellPack.RemoveAt(0);
        TArray<FKalmalaInventoryStack> OneCellWithBone;
        TArray<FKalmalaInventoryStack> OneCellCandidate;
        const bool bOneCellBoneFits = UKalmalaInventoryComponent::BuildGrant(
            OneCellPack, TEXT("BoneFragment"), 3, OneCellWithBone, FailureReason);
        const bool bOneCellToothFits = bOneCellBoneFits && UKalmalaInventoryComponent::BuildGrant(
            OneCellWithBone, TEXT("ReefTooth"), 1, OneCellCandidate, FailureReason);
        TestFalse(TEXT("One free cell cannot accept the complete rolled bundle"), bOneCellToothFits);
        TestEqual(TEXT("A capacity-limited Reef Maw claim remains pending"),
            Reloaded->GetPendingWildlifeRewards().Num(), 1);

        TArray<FKalmalaInventoryStack> TwoCellPack = FullPack;
        TwoCellPack.RemoveAt(0, 2);
        TArray<FKalmalaInventoryStack> TwoCellWithBone;
        TArray<FKalmalaInventoryStack> TwoCellCandidate;
        const bool bTwoCellBoneFits = UKalmalaInventoryComponent::BuildGrant(
            TwoCellPack, TEXT("BoneFragment"), 3, TwoCellWithBone, FailureReason);
        const bool bTwoCellToothFits = bTwoCellBoneFits && UKalmalaInventoryComponent::BuildGrant(
            TwoCellWithBone, TEXT("ReefTooth"), 1, TwoCellCandidate, FailureReason);
        TestTrue(TEXT("The complete rolled bundle fits after two cells are freed"), bTwoCellToothFits);
        const FKalmalaInventoryStack* RetryTooth = TwoCellCandidate.FindByPredicate(
            [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("ReefTooth"); });
        TestTrue(TEXT("The retry candidate retains the one-Tooth reward"),
            RetryTooth != nullptr && RetryTooth->Quantity == 1);
    }
    return true;
}

#endif
