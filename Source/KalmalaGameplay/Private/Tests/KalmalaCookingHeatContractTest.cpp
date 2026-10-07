#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCookingHeatContractTest,
    "Kalmala.Gameplay.Food.CookingStationHeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCookingHeatContractTest::RunTest(const FString& Parameters)
{
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    TestTrue(TEXT("Recipe and item catalogues validate"), Recipes->IsValidCatalogue() && Items->IsValidCatalogue());

    const FKalmalaRecipe* CookRecipe = Recipes->Find(TEXT("CookedBoarMeatRecipe"));
    const FKalmalaRecipe* MeatStewRecipe = Recipes->Find(TEXT("MeatStewRecipe"));
    const FKalmalaRecipe* RootSoupRecipe = Recipes->Find(TEXT("RootVegetableSoupRecipe"));
    const FKalmalaRecipe* FryingPanRecipe = Recipes->Find(TEXT("FryingPanRecipe"));
    const FKalmalaRecipe* RoastedRootsRecipe = Recipes->Find(TEXT("RoastedRootVegetablesRecipe"));
    const FKalmalaRecipe* DeerRootRoastRecipe = Recipes->Find(TEXT("DeerRootRoastRecipe"));
    if (!TestNotNull(TEXT("Cooked boar recipe exists"), CookRecipe)
        || !TestNotNull(TEXT("Meat stew recipe exists"), MeatStewRecipe)
        || !TestNotNull(TEXT("Root soup recipe exists"), RootSoupRecipe)
        || !TestNotNull(TEXT("Frying pan recipe exists"), FryingPanRecipe)
        || !TestNotNull(TEXT("Roasted roots recipe exists"), RoastedRootsRecipe)
        || !TestNotNull(TEXT("Deer root roast recipe exists"), DeerRootRoastRecipe)) return false;

    const auto CheckMeatOnlyRecipeCost = [this](const FKalmalaRecipe& Recipe, const FName MeatId)
    {
        TestEqual(TEXT("Food recipe lists only its meat ingredient"), Recipe.Ingredients.Num(), 1);
        if (Recipe.Ingredients.Num() == 1)
        {
            TestEqual(TEXT("Food recipe consumes the matching meat"), Recipe.Ingredients[0].ItemId, MeatId);
            TestEqual(TEXT("Food recipe consumes one meat per serving"), Recipe.Ingredients[0].Quantity, 1);
        }
    };
    CheckMeatOnlyRecipeCost(*CookRecipe, TEXT("BoarMeat"));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Cooking heat test world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!Pawn || !Controller)
    {
        AddError(TEXT("Cooking heat test pawn or controller spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);

    UKalmalaCraftingComponent* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaInventoryComponent* Inventory = Pawn->GetInventoryComponent();
    UKalmalaSkillProgressionComponent* Progression = Pawn->GetSkillProgressionComponent();
    if (!Crafting || !Inventory || !Progression)
    {
        AddError(TEXT("Cooking heat test pawn components are incomplete"));
        World->DestroyWorld(false);
        return false;
    }
    Progression->BeginPlay();
    auto GetCookingExperience = [Progression]()
    {
        const FKalmalaSkillState* State = Progression->GetServerLedger().Find(EKalmalaSkill::Cooking);
        return State ? State->Experience : -1;
    };
    FString Reason;

    TestTrue(TEXT("Fixture grants iron and root ingredients"),
        Inventory->TryGrantFromServer(TEXT("Iron"), 5)
        && Inventory->TryGrantFromServer(TEXT("Carrot"), 2)
        && Inventory->TryGrantFromServer(TEXT("Potato"), 2)
        && Inventory->TryGrantFromServer(TEXT("Onion"), 3)
        && Inventory->TryGrantFromServer(TEXT("Rutabaga"), 2));
    TestFalse(TEXT("Frying pan crafting rejects a missing Forge"),
        Crafting->CraftFromServer(FryingPanRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Missing Forge preserves iron"), Inventory->GetQuantity(TEXT("Iron")), 5);
    TestFalse(TEXT("Pan cooking rejects when the placeable pan station is missing"),
        Crafting->CraftFromServer(RoastedRootsRecipe->RecipeId, 1, Reason));
    TestTrue(TEXT("Missing pan feedback names the required station"), Crafting->GetRecipeAvailability(RoastedRootsRecipe->RecipeId).Contains(TEXT("Frying pan")));
    TestEqual(TEXT("Missing pan preserves vegetable inputs"), Inventory->GetQuantity(TEXT("Carrot")), 2);

    AKalmalaConstructionActor* Forge = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Forge fixture spawned"), Forge))
    {
        World->DestroyWorld(false);
        return false;
    }
    Forge->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, -200.0f, 0.0f));
    Forge->InitializeFromServer(TEXT("ForgeKit"), TEXT("CookingHeatForge"));
    TestTrue(TEXT("Five iron make a placeable frying pan at the Forge"),
        Crafting->CraftFromServer(FryingPanRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Pan crafting consumes five iron"), Inventory->GetQuantity(TEXT("Iron")), 0);
    TestEqual(TEXT("Forge recipe produces one placeable pan"), Inventory->GetQuantity(TEXT("FryingPanKit")), 1);

    AKalmalaConstructionActor* Pan = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Frying pan fixture spawned"), Pan))
    {
        Forge->Destroy();
        World->DestroyWorld(false);
        return false;
    }
    Pan->SetActorLocation(Pawn->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f));
    Pan->InitializeFromServer(TEXT("FryingPanKit"), TEXT("CookingHeatPan"));
    TestTrue(TEXT("Placing the pan consumes its carried kit"), Inventory->TryConsumeFromServer(TEXT("FryingPanKit"), 1));
    TestFalse(TEXT("Pan cooking rejects a missing lit hearth"),
        Crafting->CraftFromServer(RoastedRootsRecipe->RecipeId, 1, Reason));
    TestTrue(TEXT("Pan recipe detail explains nearby heat"),
        Crafting->GetRecipeAvailability(RoastedRootsRecipe->RecipeId).Contains(TEXT("lit hearth")));
    TestEqual(TEXT("Missing-pan-heat rejection preserves ingredients"), Inventory->GetQuantity(TEXT("Carrot")), 2);
    TestEqual(TEXT("Missing-pan-heat rejection preserves the placed pan"), Inventory->GetQuantity(TEXT("FryingPanKit")), 0);

    TestTrue(TEXT("Fixture grants meat and hearth fuel"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 2) && Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("Cooking rejects a missing rack"), Crafting->CraftFromServer(CookRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Missing-rack rejection preserves meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 2);
    TestEqual(TEXT("Missing-rack rejection preserves fuel"), Inventory->GetQuantity(TEXT("Wood")), 1);

    AKalmalaConstructionActor* Rack = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Cooking Rack fixture spawned"), Rack))
    {
        World->DestroyWorld(false);
        return false;
    }
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));
    Rack->InitializeFromServer(TEXT("CookingRackKit"), TEXT("CookingHeatRack"));
    TestFalse(TEXT("Cooking rejects a rack without a nearby fire"), Crafting->CraftFromServer(CookRecipe->RecipeId, 1, Reason));
    TestTrue(TEXT("Rack feedback explains the missing heat"), Crafting->GetRecipeAvailability(CookRecipe->RecipeId).Contains(TEXT("lit hearth")));
    TestEqual(TEXT("Missing-heat rejection preserves meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 2);
    TestEqual(TEXT("Missing-heat rejection preserves fuel"), Inventory->GetQuantity(TEXT("Wood")), 1);
    TestEqual(TEXT("Rejected cooking awards no experience"), GetCookingExperience(), 0);

    AKalmalaCampfire* Fire = World->SpawnActor<AKalmalaCampfire>();
    if (!TestNotNull(TEXT("Campfire fixture spawned"), Fire))
    {
        Rack->Destroy();
        World->DestroyWorld(false);
        return false;
    }
    Fire->SetActorLocation(Rack->GetActorLocation() - FVector(0.0f, 0.0f, 80.0f));
    Fire->InitializePaidFromServer(Pawn);
    TestFalse(TEXT("An unlit fire under the rack cannot cook"), Crafting->CraftFromServer(CookRecipe->RecipeId, 1, Reason));
    Fire->Interact_Implementation(Pawn);
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("The server lights a dry, fuelled hearth"), Fire->IsLit() && Fire->GetEffectiveWarmth() > 0.0f);

    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(225.0f, 0.0f, -80.0f));
    TestFalse(TEXT("A lit fire away from the rack does not provide cooking heat"), Crafting->CraftFromServer(CookRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Distant-fire rejection preserves meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 2);
    TestEqual(TEXT("Distant-fire rejection preserves fuel"), Inventory->GetQuantity(TEXT("Wood")), 1);

    Fire->SetActorLocation(Rack->GetActorLocation() - FVector(0.0f, 0.0f, 80.0f));
    const float FuelBeforeCooking = Fire->GetFuelSeconds();
    const int32 ExperienceBeforeCooking = GetCookingExperience();
    TestTrue(TEXT("A lit fire at the rack allows a bounded batch"), Crafting->CraftFromServer(CookRecipe->RecipeId, 2, Reason));
    TestEqual(TEXT("Accepted cooking consumes both meat servings"), Inventory->GetQuantity(TEXT("BoarMeat")), 0);
    TestEqual(TEXT("Accepted cooking produces both servings"), Inventory->GetQuantity(CookRecipe->Output), 2);
    TestEqual(TEXT("Accepted cooking does not consume an extra fuel item"), Inventory->GetQuantity(TEXT("Wood")), 1);
    TestEqual(TEXT("Accepted cooking does not debit fire fuel per serving"), Fire->GetFuelSeconds(), FuelBeforeCooking);
    TestEqual(TEXT("Accepted batch awards Cooking experience once"), GetCookingExperience(), ExperienceBeforeCooking + 10);
    const FKalmalaItemDefinition* CookedMeat = Items->FindItem(CookRecipe->Output);
    TestTrue(TEXT("Recipe detail uses the result description once"),
        CookedMeat && Crafting->GetRecipeDescription(CookRecipe->RecipeId) == CookedMeat->Description);

    const float FuelBeforeRoots = Fire->GetFuelSeconds();
    const int32 ExperienceBeforeRoots = GetCookingExperience();
    TestTrue(TEXT("A lit hearth allows frying pan root vegetables"),
        Crafting->CraftFromServer(RoastedRootsRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Roasted roots consume one of each listed ingredient"), Inventory->GetQuantity(TEXT("Carrot")), 1);
    TestEqual(TEXT("Roasted roots leave the soup potato"), Inventory->GetQuantity(TEXT("Potato")), 1);
    TestEqual(TEXT("Roasted roots leave the soup onion and both roast rutabagas"), Inventory->GetQuantity(TEXT("Onion")), 2);
    TestEqual(TEXT("Roasted roots create one dish"), Inventory->GetQuantity(TEXT("RoastedRootVegetables")), 1);
    TestEqual(TEXT("Roasted roots leave the placed pan intact"), Inventory->GetQuantity(TEXT("FryingPanKit")), 0);
    TestEqual(TEXT("Pan cooking adds no serving fuel debit"), Fire->GetFuelSeconds(), FuelBeforeRoots);
    TestEqual(TEXT("Roasted roots award Cooking experience after acceptance"), GetCookingExperience(), ExperienceBeforeRoots + 10);

    Fire->AdvanceFromServer(2.0f, 0.0f, 0.0f);
    TestEqual(TEXT("The fire consumes fuel by elapsed server time"), Fire->GetFuelSeconds(), FuelBeforeCooking - 2.0f);

    AKalmalaConstructionActor* Cauldron = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Cauldron fixture spawned"), Cauldron))
    {
        Rack->Destroy();
        Fire->Destroy();
        World->DestroyWorld(false);
        return false;
    }
    Cauldron->SetActorLocation(Pawn->GetActorLocation() + FVector(-240.0f, 0.0f, 0.0f));
    Cauldron->InitializeFromServer(TEXT("CauldronKit"), TEXT("CookingHeatCauldron"));
    TestFalse(TEXT("Cauldron rejects a fire that only heats another station"),
        Crafting->CraftFromServer(RootSoupRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Cauldron heat rejection preserves root ingredients"), Inventory->GetQuantity(TEXT("Rutabaga")), 2);

    Fire->SetActorLocation(Cauldron->GetActorLocation() - FVector(0.0f, 0.0f, 40.0f));
    const float FuelBeforeSoup = Fire->GetFuelSeconds();
    TestTrue(TEXT("A lit fire at the cauldron allows root soup"), Crafting->CraftFromServer(RootSoupRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Accepted soup consumes its root ingredients"), Inventory->GetQuantity(TEXT("Rutabaga")), 1);
    TestEqual(TEXT("Accepted soup creates one serving"), Inventory->GetQuantity(TEXT("RootVegetableSoup")), 1);
    TestEqual(TEXT("Accepted soup does not spend raw fuel per serving"), Inventory->GetQuantity(TEXT("Wood")), 1);
    TestEqual(TEXT("Accepted soup does not debit the lit fire"), Fire->GetFuelSeconds(), FuelBeforeSoup);

    TestTrue(TEXT("Fixture grants listed stew ingredients"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1)
        && Inventory->TryGrantFromServer(TEXT("DeerMeat"), 1)
        && Inventory->TryGrantFromServer(TEXT("Carrot"), 2)
        && Inventory->TryGrantFromServer(TEXT("Potato"), 2));
    const float FuelBeforeStew = Fire->GetFuelSeconds();
    const int32 ExperienceBeforeStew = GetCookingExperience();
    TestTrue(TEXT("A lit fire at the cauldron allows meat stew"), Crafting->CraftFromServer(MeatStewRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Stew consumes the listed boar meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 0);
    TestEqual(TEXT("Stew consumes the listed deer meat"), Inventory->GetQuantity(TEXT("DeerMeat")), 0);
    TestEqual(TEXT("Stew consumes both listed carrots"), Inventory->GetQuantity(TEXT("Carrot")), 0);
    TestEqual(TEXT("Stew consumes both listed potatoes"), Inventory->GetQuantity(TEXT("Potato")), 0);
    TestEqual(TEXT("Stew creates one catalogue serving"), Inventory->GetQuantity(TEXT("MeatStew")), 1);
    TestEqual(TEXT("Stew does not add a serving fuel debit"), Fire->GetFuelSeconds(), FuelBeforeStew);
    TestEqual(TEXT("Stew awards Cooking experience after acceptance"), GetCookingExperience(), ExperienceBeforeStew + 10);

    TestTrue(TEXT("Fixture grants deer roast ingredients"), Inventory->TryGrantFromServer(TEXT("DeerMeat"), 1));
    Fire->SetActorLocation(Pan->GetActorLocation() - FVector(0.0f, 0.0f, 40.0f));
    const float FuelBeforeDeerRoast = Fire->GetFuelSeconds();
    TestTrue(TEXT("A nearby lit fire allows deer and rutabaga roast"),
        Crafting->CraftFromServer(DeerRootRoastRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Deer roast consumes the listed meat"), Inventory->GetQuantity(TEXT("DeerMeat")), 0);
    TestEqual(TEXT("Deer roast creates one dish"), Inventory->GetQuantity(TEXT("DeerRootRoast")), 1);
    TestEqual(TEXT("Deer roast leaves the placed pan intact"), Inventory->GetQuantity(TEXT("FryingPanKit")), 0);
    TestEqual(TEXT("Deer roast adds no serving fuel debit"), Fire->GetFuelSeconds(), FuelBeforeDeerRoast);

    Rack->Interact_Implementation(Pawn);
    TestEqual(TEXT("Interacting with the rack identifies its recipe station for the owner"),
        Crafting->GetLastInteractedCookingStationKit(), FName(TEXT("CookingRackKit")));
    TestEqual(TEXT("Each accepted cooking-station interaction triggers one GUI open"),
        Crafting->GetCookingStationInteractionSerial(), 1u);

    Rack->Destroy();
    Cauldron->Destroy();
    Pan->Destroy();
    Forge->Destroy();
    Fire->Destroy();
    World->DestroyWorld(false);
    return true;
}

#endif
