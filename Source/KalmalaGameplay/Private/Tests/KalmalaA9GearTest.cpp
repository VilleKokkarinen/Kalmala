#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

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
        {TEXT("OpalHelm"), TEXT("StarOpal")}};
    for (const auto& Entry : Cases)
    {
        const auto* Recipe = UKalmalaRecipeCatalogue::Get()->Find(Entry.Key);
        if (!TestNotNull(Entry.Key.ToString() + TEXT(" has an executable inventory recipe"), Recipe)) return false;
        TestTrue(TEXT("One Forge recipe, no batch duplication"), Recipe->RequiredStation == TArray<FName>{TEXT("Forge")}
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
#endif
