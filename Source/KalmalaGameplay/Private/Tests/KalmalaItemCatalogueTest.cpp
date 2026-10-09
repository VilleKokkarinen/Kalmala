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
    struct FExpectedBatchOneCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchOneCopy BatchOneCopy[] = {
        { TEXT("Wood"), TEXT("Wood"), TEXT("Common timber for tools, structures, and camp equipment."), 50 },
        { TEXT("Lightwood"), TEXT("Lightwood"), TEXT("Pale birch timber for stronger tools."), 50 },
        { TEXT("Densewood"), TEXT("Densewood"),
            TEXT("Dark, dense timber from Ironheart trunks that burns as hearth fuel."), 50 },
        { TEXT("Coal"), TEXT("Coal"), TEXT("A dense, black mineral that burns directly as hearth fuel."), 40 },
        { TEXT("Stone"), TEXT("Stone"), TEXT("Add some googly eyes, and it becomes a friendly pet rock."), 40 },
        { TEXT("Iron"), TEXT("Iron"), TEXT("A bar of iron used to make a frying pan."), 50 },
        { TEXT("Fibre"), TEXT("Reed Fibre"),
            TEXT("Plant strands used in simple tools and camp furnishings."), 50 },
        { TEXT("PeatAmber"), TEXT("Peat Amber"), TEXT("Dark peat threaded with amber from the Mire."), 40 },
    };
    for (const FExpectedBatchOneCopy& Expected : BatchOneCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 01 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
    }
    struct FExpectedBatchTwoCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchTwoCopy BatchTwoCopy[] = {
        { TEXT("FrostSalt"), TEXT("Frost Salt"), TEXT("Pale mineral crystals gathered in the tundra."), 40 },
        { TEXT("MirelingAsh"), TEXT("Mireling Ember Ash"), TEXT("Fine, soot-dark ash left by a Mireling."), 25 },
        { TEXT("WorkbenchKit"), TEXT("Workbench"), TEXT("A camp workbench for making furnishings and repairing tools."), 5 },
        { TEXT("ForgeKit"), TEXT("Forge"), TEXT("A stone-and-timber forge for making and repairing advanced tools."), 5 },
        { TEXT("WorkbenchToolRackKit"), TEXT("Workbench Tool Rack"),
            TEXT("A tool rack that raises a Workbench to level 2."), 5 },
        { TEXT("ForgeAnvilKit"), TEXT("Forge Anvil"),
            TEXT("An anvil that lets a Forge support higher-tier tools."), 5 },
        { TEXT("GrindingStoneKit"), TEXT("Grinding Stone"),
            TEXT("A stone wheel for repairing all worn tools at once."), 5 },
    };
    for (const FExpectedBatchTwoCopy& Expected : BatchTwoCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 02 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
    }
    const TPair<FName, FName> BatchTwoRecipeNames[] = {
        { TEXT("Workbench"), TEXT("WorkbenchKit") },
        { TEXT("Forge"), TEXT("ForgeKit") },
        { TEXT("WorkbenchToolRack"), TEXT("WorkbenchToolRackKit") },
        { TEXT("ForgeAnvil"), TEXT("ForgeAnvilKit") },
        { TEXT("GrindingStone"), TEXT("GrindingStoneKit") },
    };
    for (const TPair<FName, FName>& Expected : BatchTwoRecipeNames)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Expected.Key);
        if (TestNotNull(FString::Printf(TEXT("Batch 02 recipe %s remains available"), *Expected.Key.ToString()), Recipe))
        {
            const FKalmalaItemDefinition* Output = Catalogue->FindItem(Expected.Value);
            TestNotNull(FString::Printf(TEXT("Batch 02 recipe %s output item remains available"), *Expected.Key.ToString()), Output);
            TestEqual(FString::Printf(TEXT("Batch 02 recipe %s matches its output name"), *Expected.Key.ToString()),
                Recipe->DisplayName, Output ? Output->DisplayName : FString());
            TestEqual(FString::Printf(TEXT("Batch 02 recipe %s preserves its output identity"), *Expected.Key.ToString()),
                Recipe->Output, Expected.Value);
        }
    }
    struct FExpectedBatchThreeCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchThreeCopy BatchThreeCopy[] = {
        { TEXT("Storage"), TEXT("Chest"), TEXT("A shared chest for storing camp supplies."), 5 },
        { TEXT("CookingRack"), TEXT("Cooking Rack"), TEXT("A rack for roasting boar and deer over a fire."), 5 },
        { TEXT("FryingPan"), TEXT("Frying Pan"), TEXT("An iron pan for cooking over a lit hearth."), 1 },
        { TEXT("Cauldron"), TEXT("Cauldron"), TEXT("A pot for simmering soups over a fire."), 5 },
        { TEXT("Floor"), TEXT("Timber Floor"), TEXT("Timber boards for the floor of a camp shelter."), 10 },
        { TEXT("Wall"), TEXT("Windbreak Wall"), TEXT("A light timber-and-reed wall that blocks wind around camp."), 10 },
        { TEXT("Roof"), TEXT("Reed Roof"), TEXT("A reed roof that shields a small camp from rain."), 10 },
        { TEXT("BoarMeat"), TEXT("Boar Meat"), TEXT("Rich boar meat for roasting over a fire."), 20 },
    };
    for (const FExpectedBatchThreeCopy& Expected : BatchThreeCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 03 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
    }
    const TPair<FName, FName> BatchThreeRecipeNames[] = {
        { TEXT("Storage"), TEXT("Storage") },
        { TEXT("CookingRack"), TEXT("CookingRack") },
        { TEXT("FryingPanRecipe"), TEXT("FryingPan") },
        { TEXT("Cauldron"), TEXT("Cauldron") },
        { TEXT("Floor"), TEXT("Floor") },
        { TEXT("Wall"), TEXT("Wall") },
        { TEXT("Roof"), TEXT("Roof") },
    };
    for (const TPair<FName, FName>& Expected : BatchThreeRecipeNames)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Expected.Key);
        if (TestNotNull(FString::Printf(TEXT("Batch 03 recipe %s remains available"), *Expected.Key.ToString()), Recipe))
        {
            const FKalmalaItemDefinition* Output = Catalogue->FindItem(Expected.Value);
            TestNotNull(FString::Printf(TEXT("Batch 03 recipe %s output item remains available"), *Expected.Key.ToString()), Output);
            TestEqual(FString::Printf(TEXT("Batch 03 recipe %s matches its output name"), *Expected.Key.ToString()),
                Recipe->DisplayName, Output ? Output->DisplayName : FString());
            TestEqual(FString::Printf(TEXT("Batch 03 recipe %s preserves its output identity"), *Expected.Key.ToString()),
                Recipe->Output, Expected.Value);
        }
    }
    struct FExpectedBatchFourCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchFourCopy BatchFourCopy[] = {
        { TEXT("DeerMeat"), TEXT("Deer Meat"), TEXT("Lean deer meat for roasting or stew."), 20 },
        { TEXT("BoarHide"), TEXT("Boar Hide"), TEXT("A coarse, bristled hide taken from a boar."), 20 },
        { TEXT("DeerHide"), TEXT("Deer Hide"), TEXT("A soft hide taken from a deer."), 20 },
        { TEXT("CookedBoarMeat"), TEXT("Cooked Boar Meat"), TEXT("Roasted boar meat, ready to eat."), 20 },
        { TEXT("CookedDeerMeat"), TEXT("Cooked Deer Meat"), TEXT("Roasted deer meat, ready to eat."), 20 },
        { TEXT("HearthBroth"), TEXT("Hearth Broth"), TEXT("A simple savory broth for a camp meal."), 20 },
        { TEXT("MeatStew"), TEXT("Meat Stew"), TEXT("A hearty stew of boar and deer meat with carrots and potatoes."), 20 },
        { TEXT("RootVegetableSoup"), TEXT("Root Vegetable Soup"),
            TEXT("Carrot, potato, rutabaga, and onion simmered in a cauldron."), 20 },
    };
    for (const FExpectedBatchFourCopy& Expected : BatchFourCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 04 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
    }
    const TPair<FName, FName> BatchFourRecipeNames[] = {
        { TEXT("CookedBoarMeatRecipe"), TEXT("CookedBoarMeat") },
        { TEXT("CookedDeerMeatRecipe"), TEXT("CookedDeerMeat") },
        { TEXT("MeatStewRecipe"), TEXT("MeatStew") },
        { TEXT("RootVegetableSoupRecipe"), TEXT("RootVegetableSoup") },
    };
    for (const TPair<FName, FName>& Expected : BatchFourRecipeNames)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Expected.Key);
        if (TestNotNull(FString::Printf(TEXT("Batch 04 recipe %s remains available"), *Expected.Key.ToString()), Recipe))
        {
            const FKalmalaItemDefinition* Output = Catalogue->FindItem(Expected.Value);
            TestNotNull(FString::Printf(TEXT("Batch 04 recipe %s output item remains available"), *Expected.Key.ToString()), Output);
            TestEqual(FString::Printf(TEXT("Batch 04 recipe %s matches its output name"), *Expected.Key.ToString()),
                Recipe->DisplayName, Output ? Output->DisplayName : FString());
            TestEqual(FString::Printf(TEXT("Batch 04 recipe %s preserves its output identity"), *Expected.Key.ToString()),
                Recipe->Output, Expected.Value);
        }
    }
    struct FExpectedBatchFiveCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchFiveCopy BatchFiveCopy[] = {
        { TEXT("RoastedRootVegetables"), TEXT("Roasted Root Vegetables"),
            TEXT("Carrot, potato, and onion roasted in a pan over a lit hearth."), 20 },
        { TEXT("DeerRootRoast"), TEXT("Deer and Rutabaga Roast"),
            TEXT("Deer meat, rutabaga, and onion roasted in a frying pan."), 20 },
        { TEXT("Carrot"), TEXT("Carrot"), TEXT("The original 'orange stick'."), 20 },
        { TEXT("Potato"), TEXT("Potato"), TEXT("A trusty root for stews, soups, and pan roasts."), 20 },
        { TEXT("Rutabaga"), TEXT("Rutabaga"), TEXT("A hardy root with a mild, earthy flavor."), 20 },
        { TEXT("Onion"), TEXT("Onion"), TEXT("Layers upon layers of destruction to the eyes."), 20 },
        { TEXT("CarrotSeed"), TEXT("Carrot Seeds"), TEXT("Seeds from the carrot plant."), 50 },
        { TEXT("PotatoSeed"), TEXT("Potato Seeds"), TEXT("Seeds from the potato plant."), 50 },
    };
    for (const FExpectedBatchFiveCopy& Expected : BatchFiveCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 05 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
    }
    const TPair<FName, FName> BatchFiveRecipeNames[] = {
        { TEXT("RoastedRootVegetablesRecipe"), TEXT("RoastedRootVegetables") },
        { TEXT("DeerRootRoastRecipe"), TEXT("DeerRootRoast") },
    };
    for (const TPair<FName, FName>& Expected : BatchFiveRecipeNames)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Expected.Key);
        if (TestNotNull(FString::Printf(TEXT("Batch 05 recipe %s remains available"), *Expected.Key.ToString()), Recipe))
        {
            const FKalmalaItemDefinition* Output = Catalogue->FindItem(Expected.Value);
            TestNotNull(FString::Printf(TEXT("Batch 05 recipe %s output item remains available"), *Expected.Key.ToString()), Output);
            TestEqual(FString::Printf(TEXT("Batch 05 recipe %s matches its output name"), *Expected.Key.ToString()),
                Recipe->DisplayName, Output ? Output->DisplayName : FString());
            TestEqual(FString::Printf(TEXT("Batch 05 recipe %s preserves its output identity"), *Expected.Key.ToString()),
                Recipe->Output, Expected.Value);
        }
    }
    struct FExpectedBatchSixCopy
    {
        FName ItemId;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        int32 MaxStack;
    };
    const FExpectedBatchSixCopy BatchSixCopy[] = {
        { TEXT("RutabagaSeed"), TEXT("Rutabaga Seeds"), TEXT("Seeds from the rutabaga plant."), 50 },
        { TEXT("OnionSeed"), TEXT("Onion Seeds"), TEXT("Seeds from the onion plant."), 50 },
    };
    for (const FExpectedBatchSixCopy& Expected : BatchSixCopy)
    {
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Expected.ItemId);
        if (TestNotNull(FString::Printf(TEXT("Batch 06 item %s remains in the catalogue"), *Expected.ItemId.ToString()), Item))
        {
            TestEqual(FString::Printf(TEXT("%s has the reviewed display name"), *Expected.ItemId.ToString()),
                Item->DisplayName, FString(Expected.DisplayName));
            TestEqual(FString::Printf(TEXT("%s has the reviewed description"), *Expected.ItemId.ToString()),
                Item->Description, FString(Expected.Description));
            TestEqual(FString::Printf(TEXT("%s keeps its existing stack limit"), *Expected.ItemId.ToString()),
                Item->MaxStack, Expected.MaxStack);
        }
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
    const FKalmalaItemDefinition* WorkbenchItem = Catalogue->FindItem(TEXT("WorkbenchKit"));
    if (TestNotNull(TEXT("Workbench keeps its stable runtime item identity"), WorkbenchItem))
        TestEqual(TEXT("Workbench item uses its current display name"), WorkbenchItem->DisplayName, FString(TEXT("Workbench")));
    const FKalmalaRecipe* WorkbenchRecipe = Recipes->Find(TEXT("Workbench"));
    if (TestNotNull(TEXT("Workbench keeps its stable recipe identity"), WorkbenchRecipe))
    {
        TestEqual(TEXT("Workbench recipe uses its current display name"), WorkbenchRecipe->DisplayName, FString(TEXT("Workbench")));
        TestEqual(TEXT("Workbench recipe still produces its stable item identity"), WorkbenchRecipe->Output, FName(TEXT("WorkbenchKit")));
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
