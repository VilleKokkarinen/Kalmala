#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaFoodProcessingTest,
    "Kalmala.Gameplay.Food.CampfireProcessing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaFoodProcessingTest::RunTest(const FString& Parameters)
{
    const auto* Recipes = GetDefault<UKalmalaRecipeCatalogue>();
    const auto* Items = GetDefault<UKalmalaItemCatalogue>();
    TestTrue(TEXT("Food recipes remain in the validated server catalogue"), Recipes->IsValidCatalogue());
    for (const FName RecipeId : { FName(TEXT("RoastBoarMeat")), FName(TEXT("RoastDeerMeat")) })
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(RecipeId);
        if (!TestNotNull(TEXT("Configured roasting recipe exists"), Recipe)) continue;
        TestTrue(TEXT("Roasting requires a lit hearth rather than an assembly station"), Recipe->bRequiresLitCampfire);
        TestFalse(TEXT("Roasting is not unlocked by a workbench"), Recipe->bRequiresCampfire);
        TestEqual(TEXT("Roasting requires the server-owned cooking rack kit"), Recipe->RequiredStationKit, FName(TEXT("CookingRackKit")));
        TestEqual(TEXT("Roasting batch remains bounded"), Recipe->MaxBatch, 5);
        TestEqual(TEXT("Roasting yields only the approved food item"), Recipe->Output, FName(TEXT("RoastedFieldMeat")));
    }
    for (const FName RecipeId : { FName(TEXT("SimmerBoarBroth")), FName(TEXT("SimmerDeerBroth")) })
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(RecipeId);
        if (!TestNotNull(TEXT("Configured cauldron recipe exists"), Recipe)) continue;
        TestTrue(TEXT("Broth requires a lit hearth"), Recipe->bRequiresLitCampfire);
        TestFalse(TEXT("Broth does not accept a generic assembly station"), Recipe->bRequiresCampfire);
        TestEqual(TEXT("Broth requires the server-owned cauldron"), Recipe->RequiredStationKit, FName(TEXT("CauldronKit")));
        TestEqual(TEXT("Cauldron batch is capped at three servings"), Recipe->MaxBatch, 3);
        TestEqual(TEXT("Broth yields only the approved food item"), Recipe->Output, FName(TEXT("HearthBroth")));
        const FKalmalaInventoryStack* FuelCost = Recipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Ingredient) { return Ingredient.ItemId == TEXT("Fuel"); });
        TestNotNull(TEXT("Each serving has an explicit extra fuel cost"), FuelCost);
        if (FuelCost) TestEqual(TEXT("Each broth serving consumes one extra fuel bundle"), FuelCost->Quantity, 1);
    }
    for (const FName RecipeId : { FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat")) })
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(RecipeId);
        if (!TestNotNull(TEXT("Configured smoke-frame recipe exists"), Recipe)) continue;
        TestTrue(TEXT("Smoking requires a lit hearth with heat"), Recipe->bRequiresLitCampfire);
        TestFalse(TEXT("Smoking does not accept a generic assembly station"), Recipe->bRequiresCampfire);
        TestEqual(TEXT("Smoking requires the server-owned frame"), Recipe->RequiredStationKit, FName(TEXT("SmokeFrameKit")));
        TestEqual(TEXT("Smoke batches are capped at three servings"), Recipe->MaxBatch, 3);
        TestEqual(TEXT("Smoking yields only the approved food item"), Recipe->Output, FName(TEXT("SmokedFieldMeat")));
        const FKalmalaInventoryStack* FuelCost = Recipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Ingredient) { return Ingredient.ItemId == TEXT("Fuel"); });
        TestNotNull(TEXT("Each smoked serving has an explicit extra fuel cost"), FuelCost);
        if (FuelCost) TestEqual(TEXT("Each smoked serving consumes one extra fuel bundle"), FuelCost->Quantity, 1);
    }
    const FKalmalaItemDefinition* Food = Items->FindItem(TEXT("RoastedFieldMeat"));
    TestNotNull(TEXT("Prepared meat has a catalogue-bounded stack"), Food);
    if (Food) TestEqual(TEXT("Prepared meat stack ceiling is twenty"), Food->MaxStack, 20);
    const FKalmalaItemDefinition* Broth = Items->FindItem(TEXT("HearthBroth"));
    TestNotNull(TEXT("Prepared broth has a catalogue-bounded stack"), Broth);
    if (Broth) TestEqual(TEXT("Broth stack ceiling is twenty"), Broth->MaxStack, 20);
    const FKalmalaItemDefinition* SmokedMeat = Items->FindItem(TEXT("SmokedFieldMeat"));
    TestNotNull(TEXT("Smoked meat has a catalogue-bounded stack"), SmokedMeat);
    if (SmokedMeat) TestEqual(TEXT("Smoked meat stack ceiling is twenty"), SmokedMeat->MaxStack, 20);

    const UFunction* ConsumeIntent = UKalmalaCraftingComponent::StaticClass()->FindFunctionByName(TEXT("ServerConsumeFood"));
    if (TestNotNull(TEXT("Food use uses an explicit server RPC"), ConsumeIntent))
    {
        TestTrue(TEXT("Food use RPC is server-only and reliable"), ConsumeIntent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        int32 ParametersFound = 0;
        for (TFieldIterator<FProperty> It(ConsumeIntent); It; ++It)
        {
            if (!It->HasAnyPropertyFlags(CPF_Parm)) continue;
            ++ParametersFound;
            TestEqual(TEXT("Client supplies only a food identity"), It->GetName(), FString(TEXT("FoodItemId")));
        }
        TestEqual(TEXT("Food intent has exactly one input"), ParametersFound, 1);
    }

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Food processing test world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!Pawn || !Controller)
    {
        AddError(TEXT("Food processing pawn or controller spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    UKalmalaCraftingComponent* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaInventoryComponent* Inventory = Pawn->GetInventoryComponent();
    UKalmalaPlayerStatusComponent* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    UKalmalaSkillProgressionComponent* Progression = Pawn->GetSkillProgressionComponent();
    if (!Crafting || !Inventory || !Status || !Progression)
    {
        AddError(TEXT("Food processing pawn components are incomplete"));
        World->DestroyWorld(false);
        return false;
    }
    Progression->BeginPlay();
    auto GetCookingExperience = [Progression]()
    {
        const FKalmalaSkillState* State = Progression->GetServerLedger().Find(EKalmalaSkill::Cooking);
        return State ? State->Experience : -1;
    };
    TestEqual(TEXT("A fresh server pawn begins with no Cooking experience"), GetCookingExperience(), 0);

    TestTrue(TEXT("Fixture provides one boar meat and two deer meat"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1) && Inventory->TryGrantFromServer(TEXT("DeerMeat"), 2));
    FString Reason;
    TestFalse(TEXT("Roasting rejects a missing rack"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Missing-rack failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);
    TestEqual(TEXT("Rejected preparation awards no Cooking experience"), GetCookingExperience(), 0);
    TestTrue(TEXT("Unavailable recipe names its required rack"), Crafting->GetRecipeAvailability(TEXT("RoastBoarMeat")).Contains(TEXT("Cooking rack")));

    AKalmalaConstructionActor* Rack = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Transient cooking rack fixture spawned"), Rack))
    {
        World->DestroyWorld(false);
        return false;
    }
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));
    Rack->InitializeFromServer(TEXT("CookingRackKit"), TEXT("FoodProcessingRack"));
    TestTrue(TEXT("Placed rack passes the authoritative visibility and access check"), Rack->CanUse(Pawn));
    TestFalse(TEXT("Roasting rejects a rack without a hot hearth"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Missing-heat failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);

    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f));
    TestFalse(TEXT("Distant cooking rack rejects processing"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Distant-rack failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));

    AKalmalaCampfire* Fire = World->SpawnActor<AKalmalaCampfire>();
    if (!TestNotNull(TEXT("Transient campfire fixture spawned"), Fire))
    {
        World->DestroyWorld(false);
        return false;
    }
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(120.0f, 0.0f, 0.0f));
    Fire->InitializePaidFromServer(Pawn);
    TestFalse(TEXT("An unlit fuelled fire cannot process food"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Unlit-fire failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);

    Fire->Interact_Implementation(Pawn);
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Dry fire supplies authoritative heat"), Fire->IsLit() && Fire->GetEffectiveWarmth() > 0.0f);

    AKalmalaConstructionActor* Cauldron = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Transient hearth cauldron fixture spawned"), Cauldron))
    {
        World->DestroyWorld(false);
        return false;
    }
    Cauldron->SetActorLocation(Pawn->GetActorLocation() + FVector(-120.0f, 0.0f, 0.0f));
    Cauldron->InitializeFromServer(TEXT("CauldronKit"), TEXT("FoodProcessingCauldron"));
    TestTrue(TEXT("Placed cauldron passes the authoritative visibility and access check"), Cauldron->CanUse(Pawn));
    TestTrue(TEXT("Cauldron fixture receives raw meat"), Inventory->TryGrantFromServer(TEXT("BoarMeat"), 2));
    const int32 BoarMeatBeforeCauldron = Inventory->GetQuantity(TEXT("BoarMeat"));
    const int32 FuelBeforeShortage = Inventory->GetQuantity(TEXT("Fuel"));
    if (FuelBeforeShortage > 0) Inventory->TryConsumeFromServer(TEXT("Fuel"), FuelBeforeShortage);
    TestFalse(TEXT("Cauldron rejects a batch without its extra fuel"), Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 1, Reason));
    TestEqual(TEXT("Fuel shortage preserves cauldron ingredients"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeCauldron);
    TestEqual(TEXT("Fuel shortage consumes no fuel"), Inventory->GetQuantity(TEXT("Fuel")), 0);
    Cauldron->SetActorLocation(Pawn->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f));
    Inventory->TryGrantFromServer(TEXT("Fuel"), 1);
    TestFalse(TEXT("Cauldron processing rejects a distant station"), Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 1, Reason));
    TestEqual(TEXT("Distant station rejection preserves meat and fuel"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeCauldron);
    TestEqual(TEXT("Distant station rejection preserves fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    Cauldron->SetActorLocation(Pawn->GetActorLocation() + FVector(-120.0f, 0.0f, 0.0f));
    TestFalse(TEXT("Cauldron rejects a batch above its three-serving cap"), Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 4, Reason));
    TestEqual(TEXT("Over-bound cauldron batch preserves all ingredients"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeCauldron);
    TestTrue(TEXT("Fixture can fill the broth output stack"), Inventory->TryGrantFromServer(TEXT("HearthBroth"), Broth->MaxStack));
    TestFalse(TEXT("Full broth output stack rejects cauldron processing"), Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 1, Reason));
    TestEqual(TEXT("Output-cap rejection preserves cauldron ingredients"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeCauldron);
    TestEqual(TEXT("Output-cap rejection preserves station fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    TestTrue(TEXT("Test fixture clears its full output stack"), Inventory->TryConsumeFromServer(TEXT("HearthBroth"), Broth->MaxStack));
    const float HearthFuelBeforeBroth = Fire->GetFuelSeconds();
    const int32 CookingExperienceBeforeBroth = GetCookingExperience();
    TestTrue(TEXT("Cauldron commits meat and station fuel atomically"), Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 1, Reason));
    TestEqual(TEXT("Accepted broth consumes one raw serving"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeCauldron - 1);
    TestEqual(TEXT("Accepted broth consumes its explicit fuel cost"), Inventory->GetQuantity(TEXT("Fuel")), 0);
    TestEqual(TEXT("Cauldron processing adds no separate hearth debit"), Fire->GetFuelSeconds(), HearthFuelBeforeBroth);
    TestEqual(TEXT("Accepted broth creates one prepared serving"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);
    TestEqual(TEXT("Accepted broth awards one Cooking transaction amount"), GetCookingExperience(), CookingExperienceBeforeBroth + 10);
    TestTrue(TEXT("Recipe text states the extra fuel rule"), Crafting->GetRecipeDescription(TEXT("SimmerBoarBroth")).Contains(TEXT("fuel cost")));

    AKalmalaConstructionActor* SmokeFrame = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Transient smoke frame fixture spawned"), SmokeFrame))
    {
        World->DestroyWorld(false);
        return false;
    }
    SmokeFrame->SetActorLocation(Pawn->GetActorLocation() + FVector(120.0f, 120.0f, 0.0f));
    SmokeFrame->InitializeFromServer(TEXT("SmokeFrameKit"), TEXT("FoodProcessingSmokeFrame"));
    TestTrue(TEXT("Placed smoke frame passes the authoritative visibility and access check"), SmokeFrame->CanUse(Pawn));
    const int32 BoarMeatBeforeSmoking = Inventory->GetQuantity(TEXT("BoarMeat"));
    TestTrue(TEXT("Smoke recipe detail states its level-two unlock"),
        Crafting->GetRecipeDescription(TEXT("SmokeBoarMeat")).Contains(TEXT("Cooking level 2")));
    TestTrue(TEXT("Locked smoke recipe explains the current Cooking level and path to unlock"),
        Crafting->GetRecipeAvailability(TEXT("SmokeBoarMeat")).Contains(TEXT("Cooking level 2 (current level 1)"))
        && Crafting->GetRecipeAvailability(TEXT("SmokeBoarMeat")).Contains(TEXT("prepare food")));
    TestFalse(TEXT("Server rejects locked smoking before any inventory exchange"),
        Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 1, Reason));
    TestTrue(TEXT("Locked recipe result names Cooking and the required level"),
        Reason.Contains(TEXT("Cooking level 2")) && Reason.Contains(TEXT("current level 1")));
    TestEqual(TEXT("Locked smoking preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking);
    TestEqual(TEXT("Locked smoking preserves its fuel input"), Inventory->GetQuantity(TEXT("Fuel")), 0);
    TestEqual(TEXT("Locked smoking preserves its output stack"), Inventory->GetQuantity(TEXT("SmokedFieldMeat")), 0);
    TestEqual(TEXT("Locked smoking awards no Cooking experience"), GetCookingExperience(), 10);

    TestTrue(TEXT("Fixture prepares materials for nine accepted cooking actions"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 9) && Inventory->TryGrantFromServer(TEXT("Fuel"), 9));
    for (int32 Action = 0; Action < 9; ++Action)
        TestTrue(TEXT("Accepted broth preparation advances Cooking toward level two"),
            Crafting->CraftFromServer(TEXT("SimmerBoarBroth"), 1, Reason));
    TestEqual(TEXT("Ten accepted preparation requests reach Cooking level two"), GetCookingExperience(),
        FKalmalaSkillProgressionContract::ExperiencePerLevel);
    TestTrue(TEXT("Cooking level two opens the optional smoke recipe"),
        !Crafting->GetRecipeAvailability(TEXT("SmokeBoarMeat")).Contains(TEXT("Cooking level 2")));

    TestFalse(TEXT("Smoke frame rejects a batch without its extra fuel"), Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 1, Reason));
    TestEqual(TEXT("Fuel shortage preserves smoke-frame ingredients"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking);
    TestEqual(TEXT("Fuel shortage consumes no ember bundle"), Inventory->GetQuantity(TEXT("Fuel")), 0);
    TestTrue(TEXT("Fixture grants one smoke-frame fuel bundle"), Inventory->TryGrantFromServer(TEXT("Fuel"), 1));
    SmokeFrame->SetActorLocation(Pawn->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f));
    TestFalse(TEXT("Smoke frame rejects processing from beyond access range"), Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 1, Reason));
    TestEqual(TEXT("Distant smoke frame preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking);
    TestEqual(TEXT("Distant smoke frame preserves fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    SmokeFrame->SetActorLocation(Pawn->GetActorLocation() + FVector(120.0f, 120.0f, 0.0f));
    TestFalse(TEXT("Smoke frame rejects a batch above three servings"), Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 4, Reason));
    TestEqual(TEXT("Over-bound smoking batch preserves meat"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking);
    TestTrue(TEXT("Fixture can fill the smoked-meat output stack"), Inventory->TryGrantFromServer(TEXT("SmokedFieldMeat"), SmokedMeat->MaxStack));
    TestFalse(TEXT("Full smoked-meat output stack rejects smoking"), Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 1, Reason));
    TestEqual(TEXT("Output-cap rejection preserves smoking ingredients"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking);
    TestEqual(TEXT("Output-cap rejection preserves smoke-frame fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    TestTrue(TEXT("Fixture clears its full smoked-meat stack"), Inventory->TryConsumeFromServer(TEXT("SmokedFieldMeat"), SmokedMeat->MaxStack));
    const float HearthFuelBeforeSmoking = Fire->GetFuelSeconds();
    const int32 CookingExperienceBeforeSmoking = GetCookingExperience();
    TestTrue(TEXT("Smoke frame commits meat and its station fuel atomically"), Crafting->CraftFromServer(TEXT("SmokeBoarMeat"), 1, Reason));
    TestEqual(TEXT("Accepted smoking consumes one raw serving"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeSmoking - 1);
    TestEqual(TEXT("Accepted smoking consumes its explicit ember bundle"), Inventory->GetQuantity(TEXT("Fuel")), 0);
    TestEqual(TEXT("Smoke processing adds no separate hearth debit"), Fire->GetFuelSeconds(), HearthFuelBeforeSmoking);
    TestEqual(TEXT("Accepted smoking creates one prepared serving"), Inventory->GetQuantity(TEXT("SmokedFieldMeat")), 1);
    TestEqual(TEXT("Accepted smoking awards one Cooking transaction amount"), GetCookingExperience(), CookingExperienceBeforeSmoking + 10);
    TestTrue(TEXT("Recipe text explains smoke-frame fuel and heat"), Crafting->GetRecipeDescription(TEXT("SmokeBoarMeat")).Contains(TEXT("fuel cost")));
    TestTrue(TEXT("Food panel names the available smoked serving"), Crafting->GetFoodText().Contains(TEXT("Smoked field meat: 1 available")));

    const int32 BoarMeatBeforeRack = Inventory->GetQuantity(TEXT("BoarMeat"));
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, -240.0f, 0.0f));
    TestFalse(TEXT("A player-near fire cannot heat a rack more than 250 cm away"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Distant-hearth failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeRack);
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));
    TestTrue(TEXT("Recipe text explains rack access and hearth fuel"),
        Crafting->GetRecipeDescription(TEXT("RoastBoarMeat")).Contains(TEXT("normal rate")));
    const int32 CookingExperienceBeforeRoast = GetCookingExperience();
    TestTrue(TEXT("Lit-hearth roast commits through the recipe exchange"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Accepted roast consumes exactly one raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), BoarMeatBeforeRack - 1);
    TestEqual(TEXT("Accepted roast creates exactly one cooked meat"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 1);
    TestEqual(TEXT("Accepted roast awards one Cooking transaction amount"), GetCookingExperience(), CookingExperienceBeforeRoast + 10);

    TestFalse(TEXT("Over-bound cooking batch rejects"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 6, Reason));
    TestEqual(TEXT("Invalid batch preserves all raw ingredients"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    Fire->AdvanceFromServer(1.0f, 0.1f, 0.0f);
    TestFalse(TEXT("Smouldering fire has no cooking heat"), Fire->IsLit());
    TestFalse(TEXT("Rain-smouldered fire rejects cooking"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 2, Reason));
    TestEqual(TEXT("Heat failure leaves batch ingredients untouched"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    TestTrue(TEXT("Heat rejection fixture carries the station fuel cost"), Inventory->TryGrantFromServer(TEXT("Fuel"), 1));
    TestFalse(TEXT("Rain-smouldered fire rejects smoke-frame processing"), Crafting->CraftFromServer(TEXT("SmokeDeerMeat"), 1, Reason));
    TestEqual(TEXT("Missing smoke-frame heat preserves meat"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    TestEqual(TEXT("Missing smoke-frame heat preserves fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    TestFalse(TEXT("Rain-smouldered fire also rejects cauldron processing"), Crafting->CraftFromServer(TEXT("SimmerDeerBroth"), 1, Reason));
    TestEqual(TEXT("Missing cauldron heat preserves meat"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    TestEqual(TEXT("Missing cauldron heat preserves station fuel"), Inventory->GetQuantity(TEXT("Fuel")), 1);
    TestTrue(TEXT("Test fixture removes its unused station fuel"), Inventory->TryConsumeFromServer(TEXT("Fuel"), 1));
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Dry fire recovers its existing heat state"), Fire->IsLit());
    const int32 CookingExperienceBeforeBatch = GetCookingExperience();
    TestTrue(TEXT("Two-item batch uses bounded atomic recipe scaling"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 2, Reason));
    TestEqual(TEXT("Batch consumes both raw pieces"), Inventory->GetQuantity(TEXT("DeerMeat")), 0);
    TestEqual(TEXT("Batch outputs two cooked pieces"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 3);
    TestEqual(TEXT("One accepted batch awards once regardless of servings"), GetCookingExperience(), CookingExperienceBeforeBatch + 10);

    TestTrue(TEXT("One prepared meal consumes exactly one food item"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Accepted consumption removes one cooked meat"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    TestEqual(TEXT("Server applies the fixed meal duration"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestEqual(TEXT("Meal modifier affects authoritative stamina cost"), Status->CalculateStaminaCost(20.0f), 18.0f);
    Status->AdvanceFromServer(30.0f);
    const float MealRemainingBeforeDuplicate = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    TestFalse(TEXT("A duplicate active meal is rejected"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Duplicate rejection does not consume more food"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    TestEqual(TEXT("Duplicate rejection does not extend the meal"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        MealRemainingBeforeDuplicate);
    const int32 BrothBeforeReplacementAttempt = Inventory->GetQuantity(TEXT("HearthBroth"));
    TestFalse(TEXT("An alternate meal cannot replace an active meal"), Crafting->ConsumeFoodFromServer(TEXT("HearthBroth"), Reason));
    TestEqual(TEXT("Replacement rejection preserves alternate food"), Inventory->GetQuantity(TEXT("HearthBroth")), BrothBeforeReplacementAttempt);
    TestEqual(TEXT("Replacement rejection preserves the active timer"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        MealRemainingBeforeDuplicate);
    TestFalse(TEXT("Forged food IDs are rejected"), Crafting->ConsumeFoodFromServer(TEXT("Wood"), Reason));
    TestEqual(TEXT("Forged food rejection leaves inventory alone"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    const FString ActiveMealText = Crafting->GetFoodText();
    TestTrue(TEXT("Owner feedback names the active benefit and wait rule"), ActiveMealText.Contains(TEXT("10% lower")) && ActiveMealText.Contains(TEXT("Wait for expiry")));
    TestTrue(TEXT("Owner feedback keeps every food count visible during a meal"), ActiveMealText.Contains(TEXT("Roasted field meat 2"))
        && ActiveMealText.Contains(TEXT("Hearth broth 10")) && ActiveMealText.Contains(TEXT("Smoked field meat 1")));

    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestFalse(TEXT("Meal expires on server status time"), Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
    TestTrue(TEXT("A new meal is allowed after expiry"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Post-expiry meal consumes once"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 1);
    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Hearth broth is an approved alternate meal"), Crafting->ConsumeFoodFromServer(TEXT("HearthBroth"), Reason));
    TestEqual(TEXT("Broth consumption removes one serving"), Inventory->GetQuantity(TEXT("HearthBroth")), 9);
    TestEqual(TEXT("Broth grants the existing bounded meal duration"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Smoked field meat is an approved alternate meal"), Crafting->ConsumeFoodFromServer(TEXT("SmokedFieldMeat"), Reason));
    TestEqual(TEXT("Smoked-meat consumption removes one serving"), Inventory->GetQuantity(TEXT("SmokedFieldMeat")), 0);
    TestEqual(TEXT("Smoked meat grants the existing bounded meal duration"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);

    const int32 FoodBeforeClientAttempt = Inventory->GetQuantity(TEXT("RoastedFieldMeat"));
    const int32 CookingExperienceBeforeClientAttempt = GetCookingExperience();
    const float MealBeforeClientAttempt = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot invoke the server cooking transaction"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestFalse(TEXT("Client cannot invoke the server consumption transaction"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Client requests do not mutate food inventory"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), FoodBeforeClientAttempt);
    TestEqual(TEXT("Client requests cannot award Cooking experience"), GetCookingExperience(), CookingExperienceBeforeClientAttempt);
    TestEqual(TEXT("Client requests do not mutate the active effect"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId), MealBeforeClientAttempt);

    Rack->Destroy();
    Cauldron->Destroy();
    SmokeFrame->Destroy();
    Fire->Destroy();
    World->DestroyWorld(false);
    return true;
}

#endif
