#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaM9ExplorationRewardCatalogue.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9ExplorationRewardCatalogueTest,
    "Kalmala.Gameplay.M9.ExplorationRewardCatalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9ExplorationRewardCatalogueTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const TArray<FKalmalaM9ExplorationRewardDefinition>& Definitions = FKalmalaM9ExplorationRewardCatalogue::GetDefinitions();
    TestTrue(TEXT("The M9 land reward catalogue is canonical and bounded"), FKalmalaM9ExplorationRewardCatalogue::IsValid(Items));
    TestEqual(TEXT("The approved land slate contains exactly two candidates"), Definitions.Num(), FKalmalaM9ExplorationRewardCatalogue::MaxDefinitions);

    TSet<FName> CandidateIds;
    TSet<FName> PresentationIds;
    TSet<FName> RewardIds;
    for (const FKalmalaM9ExplorationRewardDefinition& Definition : Definitions)
    {
        TestTrue(TEXT("Each candidate is optional and has a catalogue-backed small reward"),
            FKalmalaM9ExplorationRewardCatalogue::IsValidDefinition(Definition, Items));
        TestFalse(TEXT("Each candidate has a canonical identity"), Definition.CandidateId.IsNone());
        TestFalse(TEXT("Each candidate has an original presentation identity"), Definition.PresentationId.IsNone());
        TestTrue(TEXT("Each candidate describes the observed feature"), !Definition.ObservationText.IsEmpty());
        TestTrue(TEXT("Each reward stays within the approved quantity bound"), Definition.RewardQuantity > 0 && Definition.RewardQuantity <= 2);
        CandidateIds.Add(Definition.CandidateId);
        PresentationIds.Add(Definition.PresentationId);
        RewardIds.Add(Definition.RewardItemId);

        const FString StableId = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(Definition.CandidateId, FIntPoint(-12, 7));
        TestTrue(TEXT("The stable identity satisfies the sparse-ID format"), FKalmalaM7SparseDelta::IsValidStableId(StableId));
        TestEqual(TEXT("The same candidate and cell reproduce its sparse identity"), StableId,
            FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(Definition.CandidateId, FIntPoint(-12, 7)));
        TestNotEqual(TEXT("A different cell has a distinct sparse identity"), StableId,
            FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(Definition.CandidateId, FIntPoint(-11, 7)));

        TArray<FKalmalaInventoryStack> RewardStacks;
        FString Reason;
        const TArray<FKalmalaInventoryStack> EmptyStacks;
        TestTrue(TEXT("Each immutable reward builds through the owner inventory grant contract"),
            UKalmalaInventoryComponent::BuildGrant(EmptyStacks, Definition.RewardItemId, Definition.RewardQuantity, RewardStacks, Reason));
        TestEqual(TEXT("The grant contains the exact approved amount"), RewardStacks.Num(), 1);
        if (!RewardStacks.IsEmpty())
        {
            TestEqual(TEXT("The grant contains the canonical item"), RewardStacks[0].ItemId, Definition.RewardItemId);
            TestEqual(TEXT("The grant contains the canonical quantity"), RewardStacks[0].Quantity, Definition.RewardQuantity);
        }
    }
    TestEqual(TEXT("Candidate identities are unique"), CandidateIds.Num(), Definitions.Num());
    TestEqual(TEXT("Presentation identities are unique"), PresentationIds.Num(), Definitions.Num());
    TestEqual(TEXT("The discoveries offer distinct existing materials"), RewardIds.Num(), Definitions.Num());
    TestNull(TEXT("An unknown candidate fails closed"), FKalmalaM9ExplorationRewardCatalogue::FindDefinition(TEXT("forged-land-cache")));
    TestTrue(TEXT("An unknown candidate cannot produce a stable claim ID"),
        FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(TEXT("forged-land-cache"), FIntPoint::ZeroValue).IsEmpty());

    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;
    TMap<FName, FKalmalaM9ExplorationRewardDescriptor> FoundByCandidate;
    for (int32 Y = -160; Y <= 160 && FoundByCandidate.Num() < Definitions.Num(); ++Y)
    {
        for (int32 X = -160; X <= 160 && FoundByCandidate.Num() < Definitions.Num(); ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            const FVector2D KeyCenter = (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(World, KeyCenter));
            if (KeyBiome != EKalmalaBiome::ShimmeringLakes && KeyBiome != EKalmalaBiome::ThunderMountains) continue;

            const TArray<FKalmalaM9ExplorationRewardDescriptor> First =
                FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(World, SpatialKey);
            const TArray<FKalmalaM9ExplorationRewardDescriptor> Repeat =
                FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(World, SpatialKey);
            TestTrue(TEXT("Each biome cell produces at most one candidate"), First.Num() <= 1);
            TestEqual(TEXT("Placement repeats with the same world identity"), Repeat.Num(), First.Num());
            if (First.IsEmpty()) continue;

            const FKalmalaM9ExplorationRewardDescriptor& Descriptor = First[0];
            TestEqual(TEXT("Repeated placement preserves its candidate kind"), Repeat[0].CandidateId, Descriptor.CandidateId);
            TestTrue(TEXT("The server descriptor is current for this world and key"),
                FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(World, Descriptor));
            TestTrue(TEXT("The placement remains in the supported world"),
                FKalmalaWorldBounds::Contains(World, FVector2D(Descriptor.Location)));
            TestFalse(TEXT("The placement stays on dry land"), FKalmalaOceanSampler::Sample(World, FVector2D(Descriptor.Location)).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(World, FVector2D(Descriptor.Location)));
            TestTrue(TEXT("The placement is on an ordinary walkable surface"),
                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(World, FVector2D(Descriptor.Location)).Z
                    >= FKalmalaM9ExplorationRewardCatalogue::MinimumWalkableNormalZ);
            TestEqual(TEXT("The candidate appears in its approved biome"),
                FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(World, FVector2D(Descriptor.Location))),
                Descriptor.CandidateId == TEXT("lakes-rillworn-marker") ? EKalmalaBiome::ShimmeringLakes : EKalmalaBiome::ThunderMountains);
            TestTrue(TEXT("The key and candidate kind produce a valid sparse identity"),
                FKalmalaM7SparseDelta::IsValidStableId(
                    FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(Descriptor.CandidateId, Descriptor.SpatialKey)));

            FKalmalaM9ExplorationRewardDescriptor Forged = Descriptor;
            Forged.Location.X += 500.0f;
            TestFalse(TEXT("A forged candidate location is rejected"),
                FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(World, Forged));
            Forged = Descriptor;
            Forged.CandidateId = TEXT("forged-land-cache");
            TestFalse(TEXT("A forged candidate ID is rejected"),
                FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(World, Forged));

            if (Descriptor.CandidateId == TEXT("lakes-rillworn-marker"))
            {
                const FVector2D Position(Descriptor.Location);
                const bool bLakeNearby =
                    FKalmalaShimmeringLakeSampler::IsWater(World, Position + FVector2D(FKalmalaM9ExplorationRewardCatalogue::ShoreProbeDistanceCm, 0.0f))
                    || FKalmalaShimmeringLakeSampler::IsWater(World, Position - FVector2D(FKalmalaM9ExplorationRewardCatalogue::ShoreProbeDistanceCm, 0.0f))
                    || FKalmalaShimmeringLakeSampler::IsWater(World, Position + FVector2D(0.0f, FKalmalaM9ExplorationRewardCatalogue::ShoreProbeDistanceCm))
                    || FKalmalaShimmeringLakeSampler::IsWater(World, Position - FVector2D(0.0f, FKalmalaM9ExplorationRewardCatalogue::ShoreProbeDistanceCm));
                TestTrue(TEXT("The Rillstone is placed beside visible lake water"), bLakeNearby);
            }
            else
            {
                TestTrue(TEXT("Leeward Grain is placed on the broad high-ground shelf"),
                    FKalmalaWorldFieldSampler::Sample(World, FVector2D(Descriptor.Location)).Elevation >= 0.80f);
            }
            FoundByCandidate.Add(Descriptor.CandidateId, Descriptor);
        }
    }
    TestEqual(TEXT("The deterministic bounded scan reaches both approved candidates"), FoundByCandidate.Num(), Definitions.Num());

    for (const FKalmalaM9ExplorationRewardDefinition& Definition : Definitions)
    {
        const FKalmalaM9ExplorationRewardDescriptor* Descriptor = FoundByCandidate.Find(Definition.CandidateId);
        if (Descriptor == nullptr) continue;
        FKalmalaWorldGenerationConfig OtherWorld = World;
        ++OtherWorld.WorldSeed;
        const TArray<FKalmalaM9ExplorationRewardDescriptor> OtherWorldDescriptors =
            FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(OtherWorld, Descriptor->SpatialKey);
        const bool bSameLocation = OtherWorldDescriptors.ContainsByPredicate([Descriptor](const FKalmalaM9ExplorationRewardDescriptor& Other)
        {
            return Other.CandidateId == Descriptor->CandidateId && Other.Location.Equals(Descriptor->Location, 1.0f);
        });
        TestFalse(TEXT("A different world seed does not reproduce the same placement"), bSameLocation);
    }
    return true;
}

#endif
