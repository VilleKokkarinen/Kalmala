#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeContentContract.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaEnvironmentalExposureSampler.h"
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
    TestTrue(TEXT("All-biome Stone outcrops have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("allbiomes-stone-outcrop")));
    TestEqual(TEXT("All-biome Stone outcrops reuse the slate-rock presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("allbiomes-stone-outcrop")), FName(TEXT("slate-vein")));
    TestTrue(TEXT("Wild Meadow carrots have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("meadows-wild-carrot-patch")));
    TestEqual(TEXT("Wild carrots use their original procedural patch presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("meadows-wild-carrot-patch")), FName(TEXT("carrot-patch-harvest")));
    TestTrue(TEXT("Wild carrots are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("meadows-wild-carrot-patch")));
    TestTrue(TEXT("Wild Meadow potatoes have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("meadows-wild-potato-patch")));
    TestEqual(TEXT("Wild potatoes use their original procedural patch presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("meadows-wild-potato-patch")), FName(TEXT("potato-patch-harvest")));
    TestTrue(TEXT("Wild potatoes are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("meadows-wild-potato-patch")));
    TestTrue(TEXT("Wild Lakes rutabagas have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("lakes-wild-rutabaga-patch")));
    TestEqual(TEXT("Wild rutabagas use their original procedural patch presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("lakes-wild-rutabaga-patch")), FName(TEXT("rutabaga-patch-harvest")));
    TestTrue(TEXT("Wild rutabagas are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("lakes-wild-rutabaga-patch")));
    TestTrue(TEXT("Wild Elderwood onions have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("elderwood-wild-onion-patch")));
    TestEqual(TEXT("Wild onions use their original procedural patch presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("elderwood-wild-onion-patch")), FName(TEXT("onion-patch-harvest")));
    TestTrue(TEXT("Wild onions are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("elderwood-wild-onion-patch")));
    TestTrue(TEXT("Wild chanterelles have a valid gathering presentation"),
        FKalmalaBiomeContentContract::IsValidGatheringSourceId(TEXT("elderwood-wild-chanterelle-patch")));
    TestEqual(TEXT("Wild chanterelles use their dedicated mushroom presentation"),
        FKalmalaBiomeContentContract::GetGatheringPresentationId(TEXT("elderwood-wild-chanterelle-patch")), FName(TEXT("chanterelle-patch-harvest")));
    TestTrue(TEXT("Wild chanterelles are hand-gathered without a tool"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("elderwood-wild-chanterelle-patch")));
    TestFalse(TEXT("Unknown sources are not hand-gatherable"),
        FKalmalaBiomeContentContract::IsHandGatherableSourceId(TEXT("forged-source")));
    TestFalse(TEXT("Ocean cannot become an optional land rare source"), FKalmalaBiomeContentContract::IsOptionalRareDiscoverySource(EKalmalaBiome::Ocean));
    TestFalse(TEXT("Unknown rare source fails closed"), FKalmalaBiomeContentContract::IsValidRareDiscoverySourceId(TEXT("forged-discovery")));
    TestTrue(TEXT("Rare discoveries are explicitly optional"), Ocean.RareDiscoverySourceId.IsNone() && !Ocean.bRareDiscoveryOptional);

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 418;
    bool bSawGatheringDescriptor = false;
    bool bSawWildCarrotDescriptor = false;
    bool bSawWildPotatoDescriptor = false;
    bool bSawWildRutabagaDescriptor = false;
    bool bSawWildOnionDescriptor = false;
    bool bSawWildChanterelleDescriptor = false;
    bool bSawCopperOreDescriptor = false;
    bool bSawTinOreDescriptor = false;
    bool bSawStoneOutcropMeadows = false;
    bool bSawStoneOutcropLakes = false;
    bool bSawStoneOutcropElderwood = false;
    bool bSawStoneOutcropMire = false;
    bool bSawStoneOutcropTundra = false;
    bool bSawStoneOutcropMountains = false;
    bool bSawCreatureDescriptor = false;
    bool bSawRareDiscoveryDescriptor = false;
    const auto VerifyOreSpawn = [this, &Config, &bSawCopperOreDescriptor, &bSawTinOreDescriptor](
        const FKalmalaWorldPopulationSpawn& Spawn, const FName ExpectedSource,
        const EKalmalaBiome ExpectedBiome, const bool bTin)
    {
        const FVector2D Position(Spawn.Location.X, Spawn.Location.Y);
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
        TestEqual(TEXT("Ore node is placed only in its authored biome"), Biome, ExpectedBiome);
        TestFalse(TEXT("Ore node stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, Position).IsWater());
        TestFalse(TEXT("Ore node stays on dry land"), FKalmalaShimmeringLakeSampler::IsWater(Config, Position));
        TestTrue(TEXT("Ore node uses safe terrain slope"),
            FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.88f);
        if (bTin)
        {
            TestTrue(TEXT("Tin gravel stays on a dry shoreline"),
                FKalmalaEnvironmentalExposureSampler::Sample(Config, Position).bIsShoreline);
        }

        const FIntPoint SpatialKey = Spawn.SpatialKey;
        const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
            Config, SpatialKey, EKalmalaWorldPopulationKind::HarvestNode);
        int32 SameKeyOreCount = 0;
        for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
            if (Candidate.ContentId == ExpectedSource) ++SameKeyOreCount;
        TestEqual(TEXT("At most one node of each ore kind occupies a spatial key"), SameKeyOreCount, 1);
        const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        TestTrue(TEXT("Ore node position and sparse identity repeat for the same seed and key"),
            RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
            {
                return Candidate.ContentId == Spawn.ContentId
                    && Candidate.Location.Equals(Spawn.Location, 0.01f)
                    && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
            }));
        if (bTin) bSawTinOreDescriptor = true;
        else bSawCopperOreDescriptor = true;
    };
    const auto VerifyStoneOutcrop = [this, &Config](const FKalmalaWorldPopulationSpawn& Spawn, const EKalmalaBiome ExpectedBiome)
    {
        const FVector2D Position(Spawn.Location.X, Spawn.Location.Y);
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
        TestTrue(TEXT("Stone outcrop appears only in its requested land biome"), Biome == ExpectedBiome);
        TestFalse(TEXT("Stone outcrop stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, Position).IsWater());
        TestFalse(TEXT("Stone outcrop stays out of lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, Position));
        TestTrue(TEXT("Stone outcrop uses safe terrain slope"),
            FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.88f);

        const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
            Config, Spawn.SpatialKey, EKalmalaWorldPopulationKind::HarvestNode);
        int32 SameBiomeStoneCount = 0;
        for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
        {
            if (Candidate.ContentId == TEXT("allbiomes-stone-outcrop")
                && FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(
                    Config, FVector2D(Candidate.Location.X, Candidate.Location.Y))) == ExpectedBiome)
            {
                ++SameBiomeStoneCount;
            }
        }
        TestEqual(TEXT("At most one added Stone outcrop occupies a biome within each key"), SameBiomeStoneCount, 1);

        const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
        {
            if (FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId) continue;
            TestTrue(TEXT("Stone outcrop keeps 250 cm from other harvest descriptors"),
                FVector2D(Candidate.Location.X, Candidate.Location.Y).DistSquared(Position) >= 62500.0f);
        }
        TestTrue(TEXT("Stone outcrop position and identity repeat for the same seed and key"),
            RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
            {
                return Candidate.ContentId == Spawn.ContentId
                    && Candidate.Location.Equals(Spawn.Location, 0.01f)
                    && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
            }));
    };
    const auto HasCompleteSourceCoverage = [&]()
    {
        return bSawGatheringDescriptor && bSawWildCarrotDescriptor && bSawWildPotatoDescriptor
            && bSawWildRutabagaDescriptor && bSawWildOnionDescriptor && bSawWildChanterelleDescriptor
            && bSawCopperOreDescriptor && bSawTinOreDescriptor
            && bSawStoneOutcropMeadows && bSawStoneOutcropLakes && bSawStoneOutcropElderwood
            && bSawStoneOutcropMire && bSawStoneOutcropTundra && bSawStoneOutcropMountains
            && bSawCreatureDescriptor && bSawRareDiscoveryDescriptor;
    };
    // Search deterministic seeds and a broad area so lake shorelines are
    // represented without assuming every biome is near the starter region.
    for (uint64 SearchSeed = 418; SearchSeed < 450 && !HasCompleteSourceCoverage(); ++SearchSeed)
    {
        Config.WorldSeed = SearchSeed;
        for (int32 Y = -16; Y <= 16 && !HasCompleteSourceCoverage(); ++Y)
        {
            for (int32 X = -16; X <= 16 && !HasCompleteSourceCoverage(); ++X)
            {
                const FIntPoint SpatialKey(X, Y);
                for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Wildlife })
                {
                    for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind))
                    {
                        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Spawn.Location)));
                        if (Spawn.ContentId == TEXT("allbiomes-stone-outcrop"))
                        {
                            TestTrue(TEXT("Added Stone outcrop is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            bool* bSawBiomeOutcrop = nullptr;
                            switch (Biome)
                            {
                            case EKalmalaBiome::Meadows: bSawBiomeOutcrop = &bSawStoneOutcropMeadows; break;
                            case EKalmalaBiome::ShimmeringLakes: bSawBiomeOutcrop = &bSawStoneOutcropLakes; break;
                            case EKalmalaBiome::Elderwood: bSawBiomeOutcrop = &bSawStoneOutcropElderwood; break;
                            case EKalmalaBiome::MossyMire: bSawBiomeOutcrop = &bSawStoneOutcropMire; break;
                            case EKalmalaBiome::FreezingTundra: bSawBiomeOutcrop = &bSawStoneOutcropTundra; break;
                            case EKalmalaBiome::ThunderMountains: bSawBiomeOutcrop = &bSawStoneOutcropMountains; break;
                            default: break;
                            }
                            TestNotNull(TEXT("Stone outcrop biome is a supported land biome"), bSawBiomeOutcrop);
                            if (bSawBiomeOutcrop != nullptr && !*bSawBiomeOutcrop)
                            {
                                VerifyStoneOutcrop(Spawn, Biome);
                                *bSawBiomeOutcrop = true;
                            }
                            continue;
                        }
                        if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Spawn.ContentId == TEXT("mountains-slate-vein"))
                            bSawStoneOutcropMountains = true;
                        if (Spawn.ContentId == TEXT("elderwood-copper-vein"))
                        {
                            TestTrue(TEXT("Copper vein is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            VerifyOreSpawn(Spawn, TEXT("elderwood-copper-vein"), EKalmalaBiome::Elderwood, false);
                            continue;
                        }
                        if (Spawn.ContentId == TEXT("lakes-tin-shore-gravel"))
                        {
                            TestTrue(TEXT("Tin gravel is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            VerifyOreSpawn(Spawn, TEXT("lakes-tin-shore-gravel"), EKalmalaBiome::ShimmeringLakes, true);
                            continue;
                        }
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
                        if (Spawn.ContentId == TEXT("meadows-wild-potato-patch"))
                        {
                            TestEqual(TEXT("Wild potato source is only selected for Meadows"), Biome, EKalmalaBiome::Meadows);
                            TestTrue(TEXT("Wild potato source is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            const FVector2D PotatoPosition(Spawn.Location.X, Spawn.Location.Y);
                            TestFalse(TEXT("Wild potato patch stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, PotatoPosition).IsWater());
                            TestFalse(TEXT("Wild potato patch stays out of lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, PotatoPosition));
                            TestTrue(TEXT("Wild potato patch uses safe terrain slope"),
                                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, PotatoPosition).Z >= 0.88f);
                            const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
                            const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                            int32 SameKeyPotatoCount = 0;
                            for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
                                if (Candidate.ContentId == Spawn.ContentId) ++SameKeyPotatoCount;
                            TestEqual(TEXT("At most one wild potato patch occupies each spatial key"), SameKeyPotatoCount, 1);
                            TestTrue(TEXT("Wild potato placement and identity repeat for the same seed and key"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return Candidate.ContentId == Spawn.ContentId
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f)
                                        && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
                                }));
                            TestFalse(TEXT("Wild potato and carrot patches have distinct positions"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return Candidate.ContentId == TEXT("meadows-wild-carrot-patch")
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f);
                                }));
                            bSawWildPotatoDescriptor = true;
                            continue;
                        }
                        if (Spawn.ContentId == TEXT("lakes-wild-rutabaga-patch"))
                        {
                            TestEqual(TEXT("Wild rutabaga source is only selected for Shimmering Lakes"), Biome, EKalmalaBiome::ShimmeringLakes);
                            TestTrue(TEXT("Wild rutabaga source is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            const FVector2D RutabagaPosition(Spawn.Location.X, Spawn.Location.Y);
                            TestFalse(TEXT("Wild rutabaga patch stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, RutabagaPosition).IsWater());
                            TestFalse(TEXT("Wild rutabaga patch occupies dry land"), FKalmalaShimmeringLakeSampler::IsWater(Config, RutabagaPosition));
                            TestTrue(TEXT("Wild rutabaga patch is on a generated lake bank"),
                                FKalmalaEnvironmentalExposureSampler::Sample(Config, RutabagaPosition).bIsShoreline);
                            TestTrue(TEXT("Wild rutabaga patch uses safe terrain slope"),
                                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, RutabagaPosition).Z >= 0.88f);
                            const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
                            const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                            int32 SameKeyRutabagaCount = 0;
                            for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
                                if (Candidate.ContentId == Spawn.ContentId) ++SameKeyRutabagaCount;
                            TestEqual(TEXT("At most one wild rutabaga patch occupies each spatial key"), SameKeyRutabagaCount, 1);
                            TestTrue(TEXT("Wild rutabaga placement and identity repeat for the same seed and key"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return Candidate.ContentId == Spawn.ContentId
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f)
                                        && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
                                }));
                            TestFalse(TEXT("Wild rutabaga patch does not replace a Meadow crop at the same position"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return (Candidate.ContentId == TEXT("meadows-wild-carrot-patch")
                                        || Candidate.ContentId == TEXT("meadows-wild-potato-patch"))
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f);
                                }));
                            bSawWildRutabagaDescriptor = true;
                            continue;
                        }
                        if (Spawn.ContentId == TEXT("elderwood-wild-onion-patch"))
                        {
                            TestEqual(TEXT("Wild onion source is only selected for Elderwood"), Biome, EKalmalaBiome::Elderwood);
                            TestTrue(TEXT("Wild onion source is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            const FVector2D OnionPosition(Spawn.Location.X, Spawn.Location.Y);
                            TestFalse(TEXT("Wild onion patch stays out of ocean water"), FKalmalaOceanSampler::Sample(Config, OnionPosition).IsWater());
                            TestFalse(TEXT("Wild onion patch stays out of lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, OnionPosition));
                            TestTrue(TEXT("Wild onion patch uses safe terrain slope"),
                                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, OnionPosition).Z >= 0.88f);
                            const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
                            const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                            int32 SameKeyOnionCount = 0;
                            for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
                                if (Candidate.ContentId == Spawn.ContentId) ++SameKeyOnionCount;
                            TestEqual(TEXT("At most one wild onion patch occupies each spatial key"), SameKeyOnionCount, 1);
                            TestTrue(TEXT("Wild onion placement and identity repeat for the same seed and key"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return Candidate.ContentId == Spawn.ContentId
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f)
                                        && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
                                }));
                            bSawWildOnionDescriptor = true;
                            continue;
                        }
                        if (Spawn.ContentId == TEXT("elderwood-wild-chanterelle-patch"))
                        {
                            TestEqual(TEXT("Wild chanterelle source is only selected for Elderwood"), Biome, EKalmalaBiome::Elderwood);
                            TestTrue(TEXT("Wild chanterelle source is a harvest node"), Kind == EKalmalaWorldPopulationKind::HarvestNode);
                            const FVector2D ChanterellePosition(Spawn.Location.X, Spawn.Location.Y);
                            TestFalse(TEXT("Wild chanterelles stay out of ocean water"), FKalmalaOceanSampler::Sample(Config, ChanterellePosition).IsWater());
                            TestFalse(TEXT("Wild chanterelles stay out of lake water"), FKalmalaShimmeringLakeSampler::IsWater(Config, ChanterellePosition));
                            TestTrue(TEXT("Wild chanterelles use safe terrain slope"),
                                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, ChanterellePosition).Z >= 0.88f);
                            const auto RepeatedSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, SpatialKey, Kind);
                            const FString PersistentId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                            int32 SameKeyChanterelleCount = 0;
                            for (const FKalmalaWorldPopulationSpawn& Candidate : RepeatedSpawns)
                                if (Candidate.ContentId == Spawn.ContentId) ++SameKeyChanterelleCount;
                            TestEqual(TEXT("At most one wild chanterelle patch occupies each spatial key"), SameKeyChanterelleCount, 1);
                            TestTrue(TEXT("Wild chanterelle placement and identity repeat for the same seed and key"),
                                RepeatedSpawns.ContainsByPredicate([&Spawn, &PersistentId](const FKalmalaWorldPopulationSpawn& Candidate)
                                {
                                    return Candidate.ContentId == Spawn.ContentId
                                        && Candidate.Location.Equals(Spawn.Location, 0.01f)
                                        && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Candidate) == PersistentId;
                                }));
                            bSawWildChanterelleDescriptor = true;
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
    }
    TestTrue(TEXT("The deterministic neighborhood exposes a gathering identity"), bSawGatheringDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A0 wild carrot source"), bSawWildCarrotDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A0 wild potato source"), bSawWildPotatoDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A0 wild rutabaga source"), bSawWildRutabagaDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A0 wild onion source"), bSawWildOnionDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes the A6 wild chanterelle source"), bSawWildChanterelleDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes an Elderwood copper node"), bSawCopperOreDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes a dry Shimmering Lakes tin node"), bSawTinOreDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes a creature niche identity"), bSawCreatureDescriptor);
    TestTrue(TEXT("The deterministic neighborhood exposes an optional rare identity"), bSawRareDiscoveryDescriptor);
    return true;
}

#endif
