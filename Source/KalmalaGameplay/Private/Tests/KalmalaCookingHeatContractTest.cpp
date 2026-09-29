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
    if (!CookRecipe) CookRecipe = Recipes->Find(TEXT("RoastBoarMeat"));
    const FKalmalaRecipe* BrothRecipe = Recipes->Find(TEXT("SimmerBoarBroth"));
    if (!TestNotNull(TEXT("Cooked boar recipe exists"), CookRecipe)
        || !TestNotNull(TEXT("Broth recipe exists"), BrothRecipe)) return false;

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
    CheckMeatOnlyRecipeCost(*BrothRecipe, TEXT("BoarMeat"));
    TestTrue(TEXT("Cooked boar resolves to the Cooking Rack"), CookRecipe->RequiredStation.Contains(TEXT("CookingRackKit")));
    TestTrue(TEXT("Broth resolves to the Cauldron"), BrothRecipe->RequiredStation.Contains(TEXT("CauldronKit")));
    TestEqual(TEXT("Cooked boar keeps its bounded batch"), CookRecipe->MaxBatch, 5);
    TestEqual(TEXT("Broth keeps its bounded batch"), BrothRecipe->MaxBatch, 3);

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

    TestTrue(TEXT("Fixture grants meat and hearth fuel"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 2) && Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    FString Reason;
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
    TestTrue(TEXT("Recipe detail explains time-based fire fuel"),
        Crafting->GetRecipeDescription(CookRecipe->RecipeId).Contains(TEXT("fuel burns at the normal rate")));

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
    TestTrue(TEXT("Fixture grants broth input"), Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1));
    TestFalse(TEXT("Cauldron rejects a fire that only heats another station"),
        Crafting->CraftFromServer(BrothRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Cauldron heat rejection preserves meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);

    Fire->SetActorLocation(Cauldron->GetActorLocation() - FVector(0.0f, 0.0f, 40.0f));
    const float FuelBeforeBroth = Fire->GetFuelSeconds();
    TestTrue(TEXT("A lit fire at the cauldron allows broth"), Crafting->CraftFromServer(BrothRecipe->RecipeId, 1, Reason));
    TestEqual(TEXT("Accepted broth consumes its meat input"), Inventory->GetQuantity(TEXT("BoarMeat")), 0);
    TestEqual(TEXT("Accepted broth creates one serving"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);
    TestEqual(TEXT("Accepted broth does not spend raw fuel per serving"), Inventory->GetQuantity(TEXT("Wood")), 1);
    TestEqual(TEXT("Accepted broth does not debit the lit fire"), Fire->GetFuelSeconds(), FuelBeforeBroth);

    Rack->Destroy();
    Cauldron->Destroy();
    Fire->Destroy();
    World->DestroyWorld(false);
    return true;
}

#endif
