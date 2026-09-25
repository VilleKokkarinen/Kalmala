#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaOceanDiscoveryCatalogue.h"

#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldBounds.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaOceanDiscoveryClaimContractTest,
    "Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaOceanDiscoveryClaimContractTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;

    TMap<FName, FKalmalaOceanDiscoveryDescriptor> FoundByKind;
    for (int32 Y = -64; Y <= 64 && FoundByKind.Num() < FKalmalaOceanDiscoveryCatalogue::MaxDefinitions; ++Y)
    {
        for (int32 X = -64; X <= 64 && FoundByKind.Num() < FKalmalaOceanDiscoveryCatalogue::MaxDefinitions; ++X)
        {
            const FIntPoint SpatialKey(X, Y);
            const TArray<FKalmalaOceanDiscoveryDescriptor> First = FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(World, SpatialKey);
            if (First.IsEmpty())
            {
                continue;
            }

            const TArray<FKalmalaOceanDiscoveryDescriptor> Repeat = FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(World, SpatialKey);
            TestEqual(TEXT("A spatial cell materializes at most one sea discovery"), First.Num(), 1);
            TestEqual(TEXT("The same world and cell reproduce the chosen discovery kind"), Repeat.Num(), First.Num());
            if (Repeat.IsEmpty())
            {
                continue;
            }

            const FKalmalaOceanDiscoveryDescriptor& Descriptor = First[0];
            TestEqual(TEXT("Repeated generation reproduces the stable discovery ID"), Repeat[0].DiscoveryId, Descriptor.DiscoveryId);
            TestTrue(TEXT("The server descriptor is canonical for the seed and cell"), FKalmalaOceanDiscoveryCatalogue::IsCurrentDescriptor(World, Descriptor));
            TestTrue(TEXT("The generated candidate stays in the finite world"), FKalmalaWorldBounds::Contains(World, FVector2D(Descriptor.Location)));
            const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(World, FVector2D(Descriptor.Location));
            TestTrue(TEXT("The generated candidate is in qualifying master-ocean water"), Ocean.IsWater() && Ocean.WaterDepth >= FKalmalaOceanDiscoveryCatalogue::MinimumOceanDepthCm);
            TestTrue(TEXT("The canonical sparse claim ID is valid"), FKalmalaM7SparseDelta::IsValidStableId(
                FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Descriptor.DiscoveryId, Descriptor.SpatialKey)));

            FKalmalaOceanDiscoveryDescriptor Forged = Descriptor;
            Forged.Location.X += 1000.0f;
            TestFalse(TEXT("A client-selected discovery position is rejected"), FKalmalaOceanDiscoveryCatalogue::IsCurrentDescriptor(World, Forged));
            Forged = Descriptor;
            Forged.DiscoveryId = TEXT("ocean-forged-cache");
            TestFalse(TEXT("A forged discovery kind is rejected"), FKalmalaOceanDiscoveryCatalogue::IsCurrentDescriptor(World, Forged));

            FoundByKind.Add(Descriptor.DiscoveryId, Descriptor);
        }
    }

    TestEqual(TEXT("The bounded placement scan can produce every first-wave sea discovery"),
        FoundByKind.Num(), FKalmalaOceanDiscoveryCatalogue::MaxDefinitions);
    if (FoundByKind.IsEmpty())
    {
        return false;
    }

    const auto ClaimIterator = FoundByKind.CreateConstIterator();
    const FKalmalaOceanDiscoveryDescriptor& Claim = ClaimIterator.Value();
    const FString ClaimId = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Claim.DiscoveryId, Claim.SpatialKey);
    const FString PlayerIdentity = TEXT("test-provider:sea-claimant");
    const FKalmalaM7SaveIdentity SaveIdentity = FKalmalaM7SaveIdentity::ForPlayer(World.WorldSeed, PlayerIdentity);
    UKalmalaM7PersistenceSaveGame* Save = NewObject<UKalmalaM7PersistenceSaveGame>();
    Save->Initialize(SaveIdentity);
    FKalmalaM7SparseDelta ClaimDelta;
    ClaimDelta.Kind = EKalmalaM7SparseDeltaKind::DiscoveryClaimed;
    ClaimDelta.StableId = ClaimId;
    TestTrue(TEXT("An authenticated player's first claim is accepted into the versioned sparse ledger"), Save->AddSparseDelta(ClaimDelta));

    const FString ClaimTestSlot = FString::Printf(TEXT("KalmalaOceanClaimContract_%llu"), World.WorldSeed);
    TestTrue(TEXT("The one-time ocean claim writes to an isolated local slot"), UGameplayStatics::SaveGameToSlot(Save, ClaimTestSlot, 0));
    UKalmalaM7PersistenceSaveGame* Reloaded = Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(ClaimTestSlot, 0));
    TestNotNull(TEXT("The versioned player ledger reloads from its local slot"), Reloaded);
    if (Reloaded == nullptr)
    {
        return false;
    }

    TestTrue(TEXT("The claim ledger remains scoped to world seed, generator revision, and player after load"), Reloaded->Matches(SaveIdentity));
    TestTrue(TEXT("The ocean claim remains present after load"), Reloaded->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, ClaimId));
    TestFalse(TEXT("A replayed claim cannot be recorded after load"), Reloaded->AddSparseDelta(ClaimDelta));
    FKalmalaM7SaveIdentity WrongRevision = SaveIdentity;
    ++WrongRevision.GeneratorRevision;
    TestFalse(TEXT("A claim save from another generator revision fails closed"), Reloaded->Matches(WrongRevision));

    UKalmalaM7PersistenceSaveGame* OtherPlayerSave = NewObject<UKalmalaM7PersistenceSaveGame>();
    OtherPlayerSave->Initialize(FKalmalaM7SaveIdentity::ForPlayer(World.WorldSeed, TEXT("test-provider:other-claimant")));
    TestFalse(TEXT("One player's claim does not consume another player's optional discovery"),
        OtherPlayerSave->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, ClaimId));
    TestTrue(TEXT("A separate player can claim the same world discovery"), OtherPlayerSave->AddSparseDelta(ClaimDelta));

    const FKalmalaOceanDiscoveryDefinition* Definition = FKalmalaOceanDiscoveryCatalogue::FindDefinition(Claim.DiscoveryId);
    const TArray<FKalmalaInventoryStack> EmptyStacks;
    TArray<FKalmalaInventoryStack> RewardCandidate;
    FString GrantFailure;
    TestTrue(TEXT("The immutable catalogue reward produces a bounded server inventory grant"), Definition != nullptr
        && UKalmalaInventoryComponent::BuildGrant(EmptyStacks, Definition->RewardItemId, Definition->RewardQuantity, RewardCandidate, GrantFailure));
    if (Definition != nullptr)
    {
        const FKalmalaInventoryStack* GrantedStack = RewardCandidate.FindByPredicate([Definition](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == Definition->RewardItemId;
        });
        TestNotNull(TEXT("The accepted reward contains its canonical item"), GrantedStack);
        if (GrantedStack != nullptr)
        {
            TestEqual(TEXT("The reward grants the catalogue's exact quantity"), GrantedStack->Quantity, Definition->RewardQuantity);
        }

        const FKalmalaItemDefinition* Item = GetDefault<UKalmalaItemCatalogue>()->FindItem(Definition->RewardItemId);
        if (Item != nullptr)
        {
            FKalmalaInventoryStack FullRewardStack;
            FullRewardStack.ItemId = Definition->RewardItemId;
            FullRewardStack.Quantity = Item->MaxStack;
            const TArray<FKalmalaInventoryStack> FullStacks = { FullRewardStack };
            TArray<FKalmalaInventoryStack> RejectedCandidate;
            TestFalse(TEXT("A full reward stack cannot accept the discovery grant"), UKalmalaInventoryComponent::BuildGrant(
                FullStacks, Definition->RewardItemId, Definition->RewardQuantity, RejectedCandidate, GrantFailure));
            TestEqual(TEXT("A rejected full-pack grant leaves its source quantity unchanged"), FullStacks[0].Quantity, Item->MaxStack);
        }
    }
    return true;
}

#endif
