#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeContentContract.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaBiomeContentContractTest,
    "Kalmala.World.M7.BiomeContentContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaBiomeContentContractTest::RunTest(const FString& Parameters)
{
    const TArray<EKalmalaBiome> FirstWaveBiomes =
    {
        EKalmalaBiome::Meadows,
        EKalmalaBiome::ShimmeringLakes,
        EKalmalaBiome::Elderwood,
        EKalmalaBiome::MossyMire,
        EKalmalaBiome::FreezingTundra,
        EKalmalaBiome::ThunderMountains
    };

    TSet<FName> GatheringIds;
    TSet<FName> GatheringPresentationIds;
    TSet<FName> NicheIds;
    TSet<FName> DiscoveryIds;
    TSet<FName> DiscoveryPresentationIds;
    for (const EKalmalaBiome Biome : FirstWaveBiomes)
    {
        const FKalmalaBiomeContentDefinition Definition = FKalmalaBiomeContentContract::GetDefinition(Biome);
        TestTrue(TEXT("Every first-wave biome has a bounded valid content definition"), FKalmalaBiomeContentContract::IsValidFirstWaveDefinition(Definition));
        TestTrue(TEXT("Every first-wave rare source remains explicitly optional"), FKalmalaBiomeContentContract::IsOptionalRareDiscoverySource(Biome));
        TestEqual(TEXT("Gathering source lookup is stable"), Definition.GatheringSourceId, FKalmalaBiomeContentContract::GetServerContentId(Biome, EKalmalaBiomeContentKind::GatheringSource));
        TestFalse(TEXT("Every first-wave gathering source has a presentation identity"), Definition.GatheringPresentationId.IsNone());
        TestEqual(TEXT("Gathering presentation lookup is stable"), Definition.GatheringPresentationId, FKalmalaBiomeContentContract::GetGatheringPresentationId(Definition.GatheringSourceId));
        TestTrue(TEXT("Every first-wave gathering source is catalogue-valid"), FKalmalaBiomeContentContract::IsValidGatheringSourceId(Definition.GatheringSourceId));
        TestEqual(TEXT("Creature niche lookup is stable"), Definition.CreatureNicheId, FKalmalaBiomeContentContract::GetServerContentId(Biome, EKalmalaBiomeContentKind::CreatureNiche));
        TestEqual(TEXT("Rare discovery lookup is stable"), Definition.RareDiscoverySourceId, FKalmalaBiomeContentContract::GetServerContentId(Biome, EKalmalaBiomeContentKind::RareDiscovery));
        TestFalse(TEXT("Every first-wave rare source has a presentation identity"), Definition.RareDiscoveryPresentationId.IsNone());
        TestEqual(TEXT("Rare discovery presentation lookup is stable"), Definition.RareDiscoveryPresentationId, FKalmalaBiomeContentContract::GetRareDiscoveryPresentationId(Definition.RareDiscoverySourceId));
        TestTrue(TEXT("Every first-wave rare source is catalogue-valid"), FKalmalaBiomeContentContract::IsValidRareDiscoverySourceId(Definition.RareDiscoverySourceId));
        GatheringIds.Add(Definition.GatheringSourceId);
        GatheringPresentationIds.Add(Definition.GatheringPresentationId);
        NicheIds.Add(Definition.CreatureNicheId);
        DiscoveryIds.Add(Definition.RareDiscoverySourceId);
        DiscoveryPresentationIds.Add(Definition.RareDiscoveryPresentationId);
    }
    TestEqual(TEXT("First-wave gathering sources remain one-per-biome"), GatheringIds.Num(), FirstWaveBiomes.Num());
    TestEqual(TEXT("First-wave gathering presentations remain one-per-biome"), GatheringPresentationIds.Num(), FirstWaveBiomes.Num());
    TestEqual(TEXT("First-wave creature niches remain one-per-biome"), NicheIds.Num(), FirstWaveBiomes.Num());
    TestEqual(TEXT("First-wave rare sources remain one-per-biome"), DiscoveryIds.Num(), FirstWaveBiomes.Num());
    TestEqual(TEXT("First-wave rare presentations remain one-per-biome"), DiscoveryPresentationIds.Num(), FirstWaveBiomes.Num());

    const FKalmalaBiomeContentDefinition Ocean = FKalmalaBiomeContentContract::GetDefinition(EKalmalaBiome::Ocean);
    TestFalse(TEXT("Ocean does not receive land-biome content identities"), FKalmalaBiomeContentContract::IsValidFirstWaveDefinition(Ocean));
    TestFalse(TEXT("Ocean cannot receive a gathering presentation identity"), !Ocean.GatheringPresentationId.IsNone());
    TestFalse(TEXT("Unknown gathering source fails closed"), FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("forged-source")));
    TestTrue(TEXT("Meadow Aspen trunk is a valid gathering source"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("meadows-aspen-trunk")));
    TestEqual(TEXT("Meadow Aspen resolves its own presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("meadows-aspen-trunk")), FName(TEXT("aspen-trunk-harvest")));
    TestTrue(TEXT("Wild Meadow carrots have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("meadows-wild-carrot-patch")));
    TestEqual(TEXT("Wild carrots use their original procedural patch presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("meadows-wild-carrot-patch")), FName(TEXT("carrot-patch-harvest")));
    TestTrue(TEXT("Wild carrots are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("meadows-wild-carrot-patch")));
    TestFalse(TEXT("Unknown sources are not hand-gatherable"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("forged-source")));
    TestFalse(TEXT("Ocean cannot become an optional land rare source"), FKalmalaBiomeContentContract::IsOptionalRareDiscoverySource(EKalmalaBiome::Ocean));
    TestFalse(TEXT("Unknown rare source fails closed"), FKalmalaBiomeContentContract::IsValidRareDiscoverySourceId(TEXT("forged-discovery")));
    TestTrue(TEXT("Rare discoveries are explicitly optional"), Ocean.RareDiscoverySourceId.IsNone() && !Ocean.bRareDiscoveryOptional);

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    bool bSawGatheringDescriptor = false;
    bool bSawWildCarrotDescriptor = false;
    bool bSawCreatureDescriptor = false;
    bool bSawRareDiscoveryDescriptor = false;
    for (int32 Y = -4; Y <= 4; ++Y)
    {
        for (int32 X = -4; X <= 4; ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Wildlife })
            {
                for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind))
                {
                    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Spawn.Location)));
                    if (Spawn.ContentId == TEXT("meadows-wild-carrot-patch"))
                    {
                        TestEqual(TEXT("Wild carrot source is only selected for Meadows"), Biome, EKalmalaBiome::Meadows);
                        TestTrue(TEXT("Wild carrot source is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                        const FVector2D CarrotPosition(Spawn.Location.X, Spawn.Location.Y);
                        TestFalse(TEXT("Wild carrot patch stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, CarrotPosition).IsWater());
                        TestFalse(TEXT("Wild carrot patch stays out of lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, CarrotPosition));
                        TestTrue(TEXT("Wild carrot patch uses safe terrain slope"),
                            FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, CarrotPosition).Z >= 0.88f);
                        const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
                        const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        int32 SameKeyCarrotCount = 0;
                        for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
                            if (Candidate.ContentId == Spawn.ContentId) ++SameKeyCarrotCount;
                        TestEqual(TEXT("At most one wild carrot patch occupies each spatial key"), SameKeyCarrotCount, 1);
                        TestTrue(TEXT("Wild carrot placement and identity repeat for the same seed and key"),
                            RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
                            {
                                return Candidate.ContentId == Spawn.ContentId
                                    && Candidate.Location.Equals(Spawn.Location, 0.01f)
                                    && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
                            }));
                        bSawWildCarrotDescriptor = true;
                        continue;
                    }
                    const EKalmalaBiomeContentKind ContentKind = Kind == EKalmalaWorldPopulationKind::HarvestNode
                        ? EKalmalaBiomeContentKind::GatheringSource
                        : EKalmalaBiomeContentKind::CreatureNiche;
                    const FName Expected = FKalmalaBiomeContentContract::GetServerContentId(Biome, ContentKind);
                    if (!Expected.IsNone())
                    {
                        TestEqual(TEXT("Generated population carries the server-selected biome identity"), Spawn.ContentId, Expected);
                        bSawGatheringDescriptor |= Kind == EKalmalaWorldPopulationKind::HarvestNode;
                        bSawCreatureDescriptor |= Kind == EKalmalaWorldPopulationKind::Wildlife;
                    }
                }
            }
            for (const FKalmalaWorldDiscoveryDescriptor& Descriptor : FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(Config, SpatialKey, EKalmalaWorldDiscoveryKind::PointOfInterest))
            {
                const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Descriptor.Location)));
                const FName Expected = FKalmalaBiomeContentContract::GetServerContentId(Biome, EKalmalaBiomeContentKind::RareDiscovery);
                if (!Expected.IsNone())
                {
                    TestEqual(TEXT("Generated optional discoveries carry the server-selected rare source"), Descriptor.DefinitionId, Expected.ToString());
                    bSawRareDiscoveryDescriptor = true;
                }
            }
        }
    }
    TestTrue(TEXT("The deterministic neighborhood exposes a gathering identity"), bSawGatheringDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A0 wild carrot source"), bSawWildCarrotDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes a creature niche identity"), bSawCreatureDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes an optional rare identity"), bSawRareDiscoveryDescriptor);
    return true;
}

#endif
