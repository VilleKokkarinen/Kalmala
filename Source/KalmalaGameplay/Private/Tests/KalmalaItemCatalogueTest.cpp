#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaItemCatalogueTest, "Kalmala.Gameplay.Inventory.Catalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaItemCatalogueTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Versioned JSON loads a valid item catalogue"), Catalogue->IsValidCatalogue());
    TestTrue(TEXT("Versioned JSON loads a valid recipe catalogue"), Recipes->IsValidCatalogue());
    TestEqual(TEXT("The JSON item catalogue contains the complete current set"), Catalogue->Items.Num(), 32);
    TestEqual(TEXT("The JSON recipe catalogue contains the complete current set"), Recipes->Recipes.Num(), 23);
    for (const FKalmalaItemDefinition& Item : Catalogue->Items)
    {
        TestFalse(FString::Printf(TEXT("%s has a player-facing name without Kit"), *Item.ItemId.ToString()),
            Item.DisplayName.Contains(TEXT("kit"), ESearchCase::IgnoreCase));
        TestFalse(FString::Printf(TEXT("%s has a bounded description"), *Item.ItemId.ToString()),
            Item.Description.TrimStartAndEnd().IsEmpty() || Item.Description.Len() > 180);
    }
    for (const FKalmalaRecipe& Recipe : Recipes->Recipes)
    {
        TestFalse(FString::Printf(TEXT("%s has a player-facing recipe name without Kit"), *Recipe.RecipeId.ToString()),
            Recipe.DisplayName.Contains(TEXT("kit"), ESearchCase::IgnoreCase));
    }
    for (const FName Id : {FName(TEXT("Wood")), FName(TEXT("Stone")), FName(TEXT("Fibre")),
        FName(TEXT("Fuel")), FName(TEXT("ConstructionSupply"))})
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Id);
        if (!TestNotNull(TEXT("Required camp material exists in JSON"), Item)) { continue; }
        TestTrue(TEXT("Exact stack limit accepted"), Catalogue->IsValidStack(Id, Item->MaxStack));
        TestFalse(TEXT("Over-limit stack rejected"), Catalogue->IsValidStack(Id, Item->MaxStack + 1));
        TestTrue(TEXT("Fill remaining capacity"), Catalogue->CanAddToStack(Id, Item->MaxStack - 1, 1));
        TestFalse(TEXT("Full stack rejects addition"), Catalogue->CanAddToStack(Id, Item->MaxStack, 1));
    }
    TestFalse(TEXT("Unknown ID rejected"), Catalogue->IsValidStack(TEXT("ForgedItem"), 1));
    TestFalse(TEXT("Empty ID rejected"), Catalogue->IsValidStack(NAME_None, 1));
    for (const int32 Quantity : {MIN_int32, -1, 0, MAX_int32})
    {
        TestFalse(TEXT("Malformed quantity rejected"), Catalogue->IsValidStack(TEXT("Wood"), Quantity));
        TestFalse(TEXT("Malformed addition rejected"), Catalogue->CanAddToStack(TEXT("Wood"), 1, Quantity));
    }
    TestFalse(TEXT("Negative existing count rejected"), Catalogue->CanAddToStack(TEXT("Wood"), -1, 1));
    TestFalse(TEXT("Overflow existing count rejected"), Catalogue->CanAddToStack(TEXT("Wood"), MAX_int32, 1));
    TestFalse(TEXT("Unknown addition ID rejected"), Catalogue->CanAddToStack(TEXT("ForgedItem"), 0, 1));
    TestTrue(TEXT("Empty stack may receive known material"), Catalogue->CanAddToStack(TEXT("Wood"), 0, 1));

    UKalmalaItemCatalogue* Invalid = NewObject<UKalmalaItemCatalogue>();
    Invalid->Items = Catalogue->Items;
    if (Invalid->Items.IsEmpty()) { return false; }
    const FKalmalaItemDefinition Duplicate = Invalid->Items[0];
    Invalid->Items.Add(Duplicate);
    TestFalse(TEXT("Duplicate definitions fail closed"), Invalid->IsValidStack(TEXT("Wood"), 1));
    Invalid->Items = Catalogue->Items;
    Invalid->Items[0].MaxStack = MAX_int32;
    TestFalse(TEXT("Unbounded configuration fails closed"), Invalid->IsValidCatalogue());
    Invalid->Items[0].MaxStack = 0;
    TestFalse(TEXT("Zero stack configuration rejected"), Invalid->IsValidCatalogue());
    Invalid->Items = Catalogue->Items;
    Invalid->Items[0].Description.Reset();
    TestFalse(TEXT("Missing item description fails closed"), Invalid->IsValidCatalogue());
    Invalid->Items[0].Description = FString::ChrN(181, TEXT('x'));
    TestFalse(TEXT("Overlong item description fails closed"), Invalid->IsValidCatalogue());
    Invalid->Items.Reset();
    TestFalse(TEXT("Missing configuration fails closed"), Invalid->IsValidCatalogue());
    return true;
}
#endif
