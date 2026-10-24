#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"
#include "KalmalaGameMode.h"
#include "KalmalaGemstoneContract.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaA9GearRecipesTest, "Kalmala.Gameplay.Inventory.A9GearRecipes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaA9GearRecipesTest::RunTest(const FString& Parameters)
{
    const TPair<FName, FName> Cases[] = {{TEXT("QuartzNecklace"), TEXT("SmokyQuartz")},
        {TEXT("MoonstoneRing"), TEXT("Moonstone")},
        {TEXT("GarnetNecklace"), TEXT("Garnet")},
        {TEXT("JadeRing"), TEXT("GreenJade")},
        {TEXT("AmethystNecklace"), TEXT("Amethyst")},
        {TEXT("AquamarineRing"), TEXT("Aquamarine")},
        {TEXT("TourmalineNecklace"), TEXT("Tourmaline")},
        {TEXT("OpalHelm"), TEXT("StarOpal")},
        {TEXT("DuskRing"), TEXT("DuskDiamond")}};
    for (const auto& Entry : Cases)
    {
        const auto* Recipe = UKalmalaRecipeCatalogue::Get()->Find(Entry.Key);
        if (!TestNotNull(Entry.Key.ToString() + TEXT(" has an executable inventory recipe"), Recipe)) return false;
        TestTrue(TEXT("One Forge recipe, no batch duplication"), Recipe->RequiredStation == TArray<FName>{TEXT("ForgeKit")}
            && Recipe->MaxBatch == 1 && Recipe->OutputCount == 1 && Recipe->Output == Entry.Key);
        TArray<FKalmalaInventoryStack> Costs, After;
        int32 Output = 0;
        FString Reason;
        TestTrue(TEXT("Recipe scales through ordinary crafting"), UKalmalaRecipeCatalogue::Scale(*Recipe, 1, Costs, Output));
        TestTrue(TEXT("Recipe consumes exactly one named gem"), Costs.ContainsByPredicate([&Entry](const auto& Cost)
            { return Cost.ItemId == Entry.Value && Cost.Quantity == 1; }));
        TestTrue(TEXT("Complete paid exchange succeeds"), UKalmalaInventoryComponent::BuildExchange(Costs, Costs, Entry.Key, Output, After, Reason));
        TestTrue(TEXT("Materials consumed, exact non-stackable result"), After.Num() == 1 && After[0].ItemId == Entry.Key
            && After[0].Quantity == 1 && After[0].ItemGuids.Num() == 1);
        TArray<FKalmalaInventoryStack> EquippedPack;
        TArray<FKalmalaEquippedGear> Gear;
        TestTrue(TEXT("Crafted identity equips through the validated transaction"), UKalmalaInventoryComponent::BuildEquipGearTransaction(
            After, {}, After[0].ItemGuids[0], {}, EquippedPack, Gear, Reason));
        TestTrue(TEXT("Equipped item is absent from pack"), EquippedPack.IsEmpty() && Gear.Num() == 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaA9EnemySourcesTest, "Kalmala.Gameplay.Discovery.EnemyGemSources",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaA9EnemySourcesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Authoritative source fixture"), Mode))
    {
        World->DestroyWorld(false);
        return false;
    }
    // A fresh mode has the default seed-zero config; do not BeginPlay or load saves.
    const FKalmalaWorldGenerationConfig Config;
    FString MoonId, DuskId;
    for (int32 Y = -64; Y <= 64 && (MoonId.IsEmpty() || DuskId.IsEmpty()); ++Y)
    {
        for (int32 X = -64; X <= 64 && (MoonId.IsEmpty() || DuskId.IsEmpty()); ++X)
        {
            for (const auto& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                Config, FIntPoint(X, Y), EKalmalaWorldPopulationKind::Wildlife))
            {
                const FString Id = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                // Cheap chance prefilter; the production selector must still authenticate
                // the descriptor, biome and actual species rather than trusting this bool.
                const auto Biome = FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Spawn.Location)));
                for (const bool bBear : {true, false})
                {
                    const FName Expected = FKalmalaGemstoneContract::SelectEnemyGem(bBear, Biome, Spawn.SpawnSeed);
                    if (Expected.IsNone()) continue;
                    const FName Actual = Mode->SelectEnemyGemForSpawn(Id, bBear);
                    if (Actual != Expected) continue; // A different canonical species is correctly excluded.
                    if (bBear && MoonId.IsEmpty()) MoonId = Id;
                    if (!bBear && DuskId.IsEmpty()) DuskId = Id;
                }
            }
        }
    }
    TestFalse(TEXT("Generated Elderwood Moon Bear has a Moonstone success case"), MoonId.IsEmpty());
    TestFalse(TEXT("Generated Mire Mireling has a Dusk Diamond success case"), DuskId.IsEmpty());
    for (const auto& Pair : {TPair<FString, bool>(MoonId, true), TPair<FString, bool>(DuskId, false)})
    {
        if (Pair.Key.IsEmpty()) continue;
        const FName Expected = Pair.Value ? FName(TEXT("Moonstone")) : FName(TEXT("DuskDiamond"));
        TestEqual(TEXT("Canonical source is repeatable"), Mode->SelectEnemyGemForSpawn(Pair.Key, Pair.Value), Expected);
        TestTrue(TEXT("Other species route cannot reuse the identity"), Mode->SelectEnemyGemForSpawn(Pair.Key, !Pair.Value).IsNone());
        TestTrue(TEXT("Forged seed cannot name this generated spawn"), Mode->SelectEnemyGemForSpawn(Pair.Key + TEXT("0"), Pair.Value).IsNone());
    }
    TestTrue(TEXT("Malformed identity fails closed"), Mode->SelectEnemyGemForSpawn(TEXT("not-a-spawn"), true).IsNone());
    TestTrue(TEXT("Wrong population kind fails closed"), Mode->SelectEnemyGemForSpawn(TEXT("0/0/0/0"), false).IsNone());
    World->DestroyWorld(false);
    return true;
}
#endif
