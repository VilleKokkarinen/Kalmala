#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaItemCatalogue.h"
#include "Dom/JsonObject.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaItemCatalogueTest, "Kalmala.Gameplay.Inventory.Catalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaItemCatalogueTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Versioned JSON loads a valid item catalogue"), Catalogue->IsValidCatalogue());
    TestTrue(TEXT("Versioned JSON loads a valid recipe catalogue"), Recipes->IsValidCatalogue());
    FString JsonText;
    TestTrue(TEXT("The catalogue JSON is available to verify its external identifiers"),
        FFileHelper::LoadFileToString(JsonText, *(FPaths::ProjectContentDir() / TEXT("Data/GameCatalogues.json"))));
    const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonText);
    TSharedPtr<FJsonObject> JsonRoot;
    const bool bParsedJson = FJsonSerializer::Deserialize(JsonReader, JsonRoot) && JsonRoot.IsValid();
    TestTrue(TEXT("The catalogue JSON parses for completeness verification"), bParsedJson);
    const TArray<TSharedPtr<FJsonValue>>* JsonItems = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* JsonRecipes = nullptr;
    const bool bHasCatalogueArrays = bParsedJson
        && JsonRoot->TryGetArrayField(TEXT("items"), JsonItems)
        && JsonRoot->TryGetArrayField(TEXT("recipes"), JsonRecipes)
        && JsonItems != nullptr && JsonRecipes != nullptr;
    TestTrue(TEXT("The JSON has item and recipe arrays"), bHasCatalogueArrays);
    if (bHasCatalogueArrays)
    {
        TestEqual(TEXT("Every JSON item definition is loaded"), Catalogue->Items.Num(), JsonItems->Num());
        TestEqual(TEXT("Every JSON recipe definition is loaded"), Recipes->Recipes.Num(), JsonRecipes->Num());
    }
    TestFalse(TEXT("The catalogue JSON has no Kit substring in any property or value"), JsonText.Contains(TEXT("Kit"), ESearchCase::IgnoreCase));
    const FKalmalaRecipe* Campfire = Recipes->Find(TEXT("Campfire"));
    if (TestNotNull(TEXT("Campfire construction recipe resolves"), Campfire))
    {
        TestEqual(TEXT("Legacy HearthRing output becomes a separate buildable descriptor"),
            Campfire->BuildableOutput, FName(TEXT("CampfireKit")));
        TestTrue(TEXT("Construction descriptor leaves the inventory output empty"), Campfire->Output.IsNone());
        TestEqual(TEXT("The construction recipe uses its player-facing Campfire name"),
            Campfire->DisplayName, FString(TEXT("Campfire")));
        TestNull(TEXT("Campfire is absent from the normal inventory catalogue"),
            Catalogue->FindItem(TEXT("CampfireKit")));
        TestNull(TEXT("Legacy HearthRing remains only as a recipe/build alias"),
            Catalogue->FindItem(TEXT("HearthRing")));

        TArray<FKalmalaInventoryStack> ScaledCosts = {{TEXT("Iron"), 1}};
        int32 ScaledOutputCount = 77;
        TestFalse(TEXT("Direct Campfire construction cannot scale into an inventory output"),
            UKalmalaRecipeCatalogue::Scale(*Campfire, 1, ScaledCosts, ScaledOutputCount));
        TestEqual(TEXT("Rejected direct output leaves existing costs unchanged"), ScaledCosts.Num(), 1);
        TestEqual(TEXT("Rejected direct output leaves the output count unchanged"), ScaledOutputCount, 77);

        TestTrue(TEXT("Campfire descriptor validates without an inventory item definition"),
            Recipes->IsValidCatalogue(Catalogue));
    }
    for (const TCHAR* RemovedField : { TEXT("OutputTool"), TEXT("bRequiresCampfire"), TEXT("AlternateStation"), TEXT("RequiredSkillLevel") })
        TestFalse(FString::Printf(TEXT("Recipe data omits removed field %s"), RemovedField), JsonText.Contains(RemovedField));
    TestTrue(TEXT("Station requirements use JSON arrays"), JsonText.Contains(TEXT("\"RequiredStation\": [")));
    for (const FName Id : { FName(TEXT("Fuel")), FName(TEXT("ConstructionSupply")), FName(TEXT("RaisedStorage")) })
    {
        TestNull(TEXT("Removed item is not loaded"), Catalogue->FindItem(Id));
        TestNull(TEXT("Removed item recipe is not loaded"), Recipes->Find(Id));
    }
    for (const FKalmalaItemDefinition& Item : Catalogue->Items)
    {
        TestFalse(FString::Printf(TEXT("%s has a player-facing name without Kit"), *Item.ItemId.ToString()),
            Item.DisplayName.Contains(TEXT("kit"), ESearchCase::IgnoreCase));
        TestFalse(FString::Printf(TEXT("%s has a bounded description"), *Item.ItemId.ToString()),
            Item.Description.TrimStartAndEnd().IsEmpty() || Item.Description.Len() > 180);
    }
    const FName CropIds[] = {
        FName(TEXT("Carrot")), FName(TEXT("Potato")), FName(TEXT("Rutabaga")), FName(TEXT("Onion"))
    };
    for (const FName CropId : CropIds)
    {
        const FName SeedId(*FString::Printf(TEXT("%sSeed"), *CropId.ToString()));
        const FKalmalaItemDefinition* Crop = Catalogue->FindItem(CropId);
        const FKalmalaItemDefinition* Seed = Catalogue->FindItem(SeedId);
        TestNotNull(FString::Printf(TEXT("Crop item %s is defined"), *CropId.ToString()), Crop);
        TestNotNull(FString::Printf(TEXT("Matching seed item %s is defined"), *SeedId.ToString()), Seed);
        if (Seed)
        {
            TestTrue(FString::Printf(TEXT("%s seed description identifies its crop"), *SeedId.ToString()),
                Seed->Description.Contains(CropId.ToString(), ESearchCase::IgnoreCase));
        }
    }

    const TPair<FName, FName> CurrentFoodRecipes[] = {
        {TEXT("CookedBoarMeatRecipe"), TEXT("CookedBoarMeat")},
        {TEXT("CookedDeerMeatRecipe"), TEXT("CookedDeerMeat")},
        {TEXT("MeatStewRecipe"), TEXT("MeatStew")},
        {TEXT("RootVegetableSoupRecipe"), TEXT("RootVegetableSoup")},
        {TEXT("RoastedRootVegetablesRecipe"), TEXT("RoastedRootVegetables")},
        {TEXT("DeerRootRoastRecipe"), TEXT("DeerRootRoast")}
    };
    for (const TPair<FName, FName>& Entry : CurrentFoodRecipes)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Entry.Key);
        if (!TestNotNull(FString::Printf(TEXT("Current food recipe %s exists"), *Entry.Key.ToString()), Recipe)) continue;
        TestEqual(TEXT("Current food recipe yields its stable output ID"), Recipe->Output, Entry.Value);
        TestEqual(TEXT("Current food recipe batch is bounded at five"), Recipe->MaxBatch, 5);
        TestTrue(TEXT("Current food recipe has no carried-tool requirement"), Recipe->RequiredTool.IsNone());

        const FName StationId = Entry.Key == FName(TEXT("CookedBoarMeatRecipe")) || Entry.Key == FName(TEXT("CookedDeerMeatRecipe"))
            ? FName(TEXT("CookingRackKit"))
            : Entry.Key == FName(TEXT("MeatStewRecipe")) || Entry.Key == FName(TEXT("RootVegetableSoupRecipe"))
                ? FName(TEXT("CauldronKit")) : FName(TEXT("FryingPanKit"));
        TestTrue(TEXT("Current food recipe resolves to its required cooking station"), Recipe->RequiredStation.Contains(StationId));

        const FKalmalaItemDefinition* Output = Catalogue->FindItem(Entry.Value);
        if (TestNotNull(TEXT("Current food recipe output item is defined"), Output))
            TestEqual(TEXT("Prepared food stacks are bounded at twenty"), Output->MaxStack, 20);
    }
    for (const FName RemovedRecipe : {
        FName(TEXT("RoastBoarMeat")), FName(TEXT("RoastDeerMeat")),
        FName(TEXT("SimmerBoarBroth")), FName(TEXT("SimmerDeerBroth")),
        FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat")) })
        TestNull(TEXT("Retired food recipe is absent from schema four"), Recipes->Find(RemovedRecipe));
    for (const FName RemovedItem : {
        FName(TEXT("RoastedFieldMeat")), FName(TEXT("SmokedFieldMeat")),
        FName(TEXT("SmokeFrame")), FName(TEXT("SmokeFrameKit")) })
        TestNull(TEXT("Retired food or station item is absent from schema four"), Catalogue->FindItem(RemovedItem));

    const FKalmalaItemDefinition* HearthBroth = Catalogue->FindItem(TEXT("HearthBroth"));
    if (TestNotNull(TEXT("Hearth broth remains a current catalogue item"), HearthBroth))
        TestEqual(TEXT("Hearth broth stack remains bounded at twenty"), HearthBroth->MaxStack, 20);
    TestNull(TEXT("Hearth broth has no production recipe"),
        Recipes->Recipes.FindByPredicate([](const FKalmalaRecipe& Recipe) { return Recipe.Output == TEXT("HearthBroth"); }));

    for (const FName Id : { FName(TEXT("Iron")), FName(TEXT("FryingPanKit")) })
        TestNotNull(FString::Printf(TEXT("New crafting item %s is defined"), *Id.ToString()), Catalogue->FindItem(Id));
    const FKalmalaRecipe* FryingPanRecipe = Recipes->Find(TEXT("FryingPanRecipe"));
    TestNotNull(TEXT("Frying pan forge recipe is defined"), FryingPanRecipe);
    if (FryingPanRecipe)
    {
        TestEqual(TEXT("Frying pan forge recipe outputs the placeable station"), FryingPanRecipe->Output, FName(TEXT("FryingPanKit")));
        TestTrue(TEXT("Frying pan is made at a Forge"), FryingPanRecipe->RequiredStation.Contains(TEXT("ForgeKit")));
        TestTrue(TEXT("Frying pan output is no longer a carried tool"), FryingPanRecipe->RequiredTool.IsNone());
    }
    for (const FKalmalaRecipe& Recipe : Recipes->Recipes)
    {
        TestFalse(FString::Printf(TEXT("%s has a player-facing recipe name without Kit"), *Recipe.RecipeId.ToString()),
            Recipe.DisplayName.Contains(TEXT("kit"), ESearchCase::IgnoreCase));
    }
    const TPair<FName, FName> LegacyAliases[] = {
        {TEXT("HearthRing"), TEXT("CampfireKit")}, {TEXT("Workbench"), TEXT("WorkbenchKit")},
        {TEXT("Forge"), TEXT("ForgeKit")}, {TEXT("WorkbenchToolRack"), TEXT("WorkbenchToolRackKit")},
        {TEXT("ForgeAnvil"), TEXT("ForgeAnvilKit")}, {TEXT("GrindingStone"), TEXT("GrindingStoneKit")},
        {TEXT("Storage"), TEXT("StorageKit")},
        {TEXT("CookingRack"), TEXT("CookingRackKit")}, {TEXT("Cauldron"), TEXT("CauldronKit")},
        {TEXT("FryingPan"), TEXT("FryingPanKit")},
        {TEXT("Floor"), TEXT("FloorKit")}, {TEXT("Wall"), TEXT("WallKit")}, {TEXT("Roof"), TEXT("RoofKit")}
    };
    for (const TPair<FName, FName>& Alias : LegacyAliases)
    {
        if (Catalogue->FindItem(Alias.Value))
        {
            TestNotNull(FString::Printf(TEXT("Clean catalogue ID %s resolves to its stable runtime item"), *Alias.Key.ToString()),
                Catalogue->FindItem(Alias.Value));
            TestNull(FString::Printf(TEXT("Clean catalogue ID %s is translated before runtime lookup"), *Alias.Key.ToString()),
                Catalogue->FindItem(Alias.Key));
        }
    }
    const FKalmalaRecipe* GrindingStone = Recipes->Find(TEXT("GrindingStone"));
    TestNotNull(TEXT("Grinding Stone recipe loads"), GrindingStone);
    if (GrindingStone)
    {
        TestEqual(TEXT("Clean output identity maps to the existing construction identity"), GrindingStone->Output, FName(TEXT("GrindingStoneKit")));
        TestTrue(TEXT("Clean station identity maps to the existing station identity"), GrindingStone->RequiredStation.Contains(TEXT("WorkbenchKit")));
    }
    for (const FName Id : {FName(TEXT("Wood")), FName(TEXT("Stone")), FName(TEXT("Fibre")),
        FName(TEXT("Lightwood")), FName(TEXT("Densewood")), FName(TEXT("Coal"))})
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
