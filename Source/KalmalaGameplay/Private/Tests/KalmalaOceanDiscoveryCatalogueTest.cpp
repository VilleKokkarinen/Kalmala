#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaOceanDiscoveryCatalogue.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaOceanDiscoveryCatalogueTest,
    "Kalmala.Gameplay.OceanTravel.DiscoveryCatalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaOceanDiscoveryCatalogueTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const TArray<FKalmalaOceanDiscoveryDefinition>& Definitions = FKalmalaOceanDiscoveryCatalogue::GetDefinitions();
    TestTrue(TEXT("The sea catalogue has the bounded original first wave"), FKalmalaOceanDiscoveryCatalogue::IsValid(Items));
    TestEqual(TEXT("The first sea catalogue remains small"), Definitions.Num(), FKalmalaOceanDiscoveryCatalogue::MaxDefinitions);

    TSet<FName> DiscoveryIds;
    TSet<FName> PresentationIds;
    TSet<FName> RewardIds;
    for (const FKalmalaOceanDiscoveryDefinition& Definition : Definitions)
    {
        TestTrue(TEXT("Each definition is canonical, optional, and has a valid bounded item reward"),
            FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(Definition, Items));
        TestFalse(TEXT("Each discovery has a stable source ID"), Definition.DiscoveryId.IsNone());
        TestFalse(TEXT("Each discovery has a stable presentation ID"), Definition.PresentationId.IsNone());
        TestTrue(TEXT("Each reward exists in the server item catalogue"), Items->IsValidStack(Definition.RewardItemId, Definition.RewardQuantity));
        TestFalse(TEXT("Rewards stay within the conservative first-wave quantity bound"), Definition.RewardQuantity > 3);
        DiscoveryIds.Add(Definition.DiscoveryId);
        PresentationIds.Add(Definition.PresentationId);
        RewardIds.Add(Definition.RewardItemId);

        const FString StableId = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Definition.DiscoveryId, FIntPoint(-12, 7));
        TestTrue(TEXT("Stable identity uses the existing sparse-ID character and length contract"), FKalmalaM7SparseDelta::IsValidStableId(StableId));
        TestEqual(TEXT("The same discovery and spatial key reproduce the same sparse identity"),
            StableId, FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Definition.DiscoveryId, FIntPoint(-12, 7)));
        TestNotEqual(TEXT("A different spatial key receives a distinct sparse identity"),
            StableId, FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Definition.DiscoveryId, FIntPoint(-11, 7)));
    }

    TestEqual(TEXT("Discovery kinds have unique stable IDs"), DiscoveryIds.Num(), Definitions.Num());
    TestEqual(TEXT("Discovery presentations have unique IDs"), PresentationIds.Num(), Definitions.Num());
    TestEqual(TEXT("Each optional discovery offers a distinct resource reward"), RewardIds.Num(), Definitions.Num());
    TestNull(TEXT("Unknown discovery IDs fail closed"), FKalmalaOceanDiscoveryCatalogue::FindDefinition(TEXT("forged-ocean-cache")));
    TestTrue(TEXT("Unknown IDs cannot produce sparse claim identities"),
        FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(TEXT("forged-ocean-cache"), FIntPoint(0, 0)).IsEmpty());

    if (!Definitions.IsEmpty())
    {
        FKalmalaOceanDiscoveryDefinition ForgedReward = Definitions[0];
        ForgedReward.RewardItemId = TEXT("ForgedItem");
        TestFalse(TEXT("A forged reward mapping is rejected"), FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(ForgedReward, Items));

        FKalmalaOceanDiscoveryDefinition Required = Definitions[0];
        Required.bOptional = false;
        TestFalse(TEXT("A required sea discovery is outside this optional catalogue"), FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(Required, Items));

        FKalmalaOceanDiscoveryDefinition OversizedReward = Definitions[0];
        OversizedReward.RewardQuantity = MAX_int32;
        TestFalse(TEXT("An unbounded reward quantity is rejected"), FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(OversizedReward, Items));
    }

    TestTrue(TEXT("Extreme spatial keys still produce bounded stable sparse identities"),
        FKalmalaM7SparseDelta::IsValidStableId(FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(
            Definitions[0].DiscoveryId, FIntPoint(MIN_int32, MAX_int32))));
    return true;
}

#endif
