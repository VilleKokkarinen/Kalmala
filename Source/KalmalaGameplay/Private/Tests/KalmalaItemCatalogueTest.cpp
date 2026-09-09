#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaItemCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaItemCatalogueTest, "Kalmala.Gameplay.Inventory.Catalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaItemCatalogueTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    TestTrue(TEXT("Project config loads a valid catalogue"), Catalogue->IsValidCatalogue());
    for (const FName Id : {FName(TEXT("Wood")), FName(TEXT("Stone")), FName(TEXT("Fibre")),
        FName(TEXT("Fuel")), FName(TEXT("ConstructionSupply"))})
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Id);
        if (!TestNotNull(TEXT("Required camp material exists in config"), Item)) { continue; }
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
    Invalid->Items.Reset();
    TestFalse(TEXT("Missing configuration fails closed"), Invalid->IsValidCatalogue());
    return true;
}
#endif
