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
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    const auto* Items = UKalmalaItemCatalogue::Get();
    TestTrue(TEXT("Food recipes remain in the validated server catalogue"), Recipes->IsValidCatalogue());
    TestTrue(TEXT("Food items remain in the validated server catalogue"), Items->IsValidCatalogue());

    const TPair<FName, FName> RecipeAndOutput[] = {
        {TEXT("CookedBoarMeatRecipe"), TEXT("CookedBoarMeat")},
        {TEXT("CookedDeerMeatRecipe"), TEXT("CookedDeerMeat")},
        {TEXT("MeatStewRecipe"), TEXT("MeatStew")},
        {TEXT("RootVegetableSoupRecipe"), TEXT("RootVegetableSoup")},
        {TEXT("RoastedRootVegetablesRecipe"), TEXT("RoastedRootVegetables")},
        {TEXT("DeerRootRoastRecipe"), TEXT("DeerRootRoast")}
    };
    for (const TPair<FName, FName>& Entry : RecipeAndOutput)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Entry.Key);
        if (!TestNotNull(FString::Printf(TEXT("Current food recipe %s exists"), *Entry.Key.ToString()), Recipe)) continue;
        TestEqual(TEXT("Current recipe yields its stable catalogue food ID"), Recipe->Output, Entry.Value);
        TestEqual(TEXT("Current food recipe batch stays bounded at five"), Recipe->MaxBatch, 5);
        const FKalmalaItemDefinition* Output = Items->FindItem(Entry.Value);
        if (TestNotNull(TEXT("Current recipe output has a catalogue item"), Output))
            TestEqual(TEXT("Prepared food stack remains bounded at twenty"), Output->MaxStack, 20);
    }
    for (const TPair<FName, FName>& Entry : {
        TPair<FName, FName>(TEXT("CookedBoarMeatRecipe"), TEXT("CookingRackKit")),
        TPair<FName, FName>(TEXT("CookedDeerMeatRecipe"), TEXT("CookingRackKit")),
        TPair<FName, FName>(TEXT("MeatStewRecipe"), TEXT("CauldronKit")),
        TPair<FName, FName>(TEXT("RootVegetableSoupRecipe"), TEXT("CauldronKit")),
        TPair<FName, FName>(TEXT("RoastedRootVegetablesRecipe"), TEXT("FryingPanKit")),
        TPair<FName, FName>(TEXT("DeerRootRoastRecipe"), TEXT("FryingPanKit")) })
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Entry.Key);
        if (Recipe) TestTrue(TEXT("Food preparation requires its catalogue station"), Recipe->RequiredStation.Contains(Entry.Value));
    }
    for (const FName RemovedRecipe : {
        FName(TEXT("RoastBoarMeat")), FName(TEXT("RoastDeerMeat")),
        FName(TEXT("SimmerBoarBroth")), FName(TEXT("SimmerDeerBroth")),
        FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat")) })
        TestNull(TEXT("Retired food recipe is absent from schema four"), Recipes->Find(RemovedRecipe));
    for (const FName RemovedItem : {
        FName(TEXT("RoastedFieldMeat")), FName(TEXT("SmokedFieldMeat")),
        FName(TEXT("SmokeFrame")), FName(TEXT("SmokeFrameKit")) })
        TestNull(TEXT("Retired food or station item is absent from schema four"), Items->FindItem(RemovedItem));

    const FKalmalaItemDefinition* HearthBroth = Items->FindItem(TEXT("HearthBroth"));
    if (TestNotNull(TEXT("Hearth broth remains a current catalogue item"), HearthBroth))
        TestEqual(TEXT("Hearth broth stack remains bounded at twenty"), HearthBroth->MaxStack, 20);
    TestNull(TEXT("Hearth broth currently has no production recipe"),
        Recipes->Recipes.FindByPredicate([](const FKalmalaRecipe& Recipe) { return Recipe.Output == TEXT("HearthBroth"); }));

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

    const FName CookedBoarRecipeId(TEXT("CookedBoarMeatRecipe"));
    const FName MeatStewRecipeId(TEXT("MeatStewRecipe"));
    TestTrue(TEXT("Fixture provides current meat ingredients"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 2)
        && Inventory->TryGrantFromServer(TEXT("DeerMeat"), 1));
    FString Reason;
    TestFalse(TEXT("Cooking rejects a missing rack"), Crafting->CraftFromServer(CookedBoarRecipeId, 1, Reason));
    TestEqual(TEXT("Missing-rack rejection preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 2);
    TestEqual(TEXT("Rejected preparation awards no Cooking experience"), GetCookingExperience(), 0);

    AKalmalaConstructionActor* Rack = World->SpawnActor<AKalmalaConstructionActor>();
    AKalmalaCampfire* Fire = World->SpawnActor<AKalmalaCampfire>();
    AKalmalaConstructionActor* Cauldron = World->SpawnActor<AKalmalaConstructionActor>();
    if (!Rack || !Fire || !Cauldron)
    {
        AddError(TEXT("Cooking station or hearth fixture spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    Rack->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));
    Rack->InitializeFromServer(TEXT("CookingRackKit"), TEXT("FoodProcessingRack"));
    Cauldron->SetActorLocation(Pawn->GetActorLocation() + FVector(-120.0f, 0.0f, 0.0f));
    Cauldron->InitializeFromServer(TEXT("CauldronKit"), TEXT("FoodProcessingCauldron"));
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(120.0f, 0.0f, 0.0f));
    Fire->InitializePaidFromServer(Pawn);
    TestTrue(TEXT("Placed rack passes authoritative access"), Rack->CanUse(Pawn));
    TestTrue(TEXT("Placed cauldron passes authoritative access"), Cauldron->CanUse(Pawn));
    TestFalse(TEXT("An unlit fuelled hearth cannot process food"), Crafting->CraftFromServer(CookedBoarRecipeId, 1, Reason));
    TestEqual(TEXT("Unlit-hearth rejection preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 2);
    Fire->Interact_Implementation(Pawn);
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Dry hearth supplies authoritative heat"), Fire->IsLit() && Fire->GetEffectiveWarmth() > 0.0f);

    const float FuelBeforeCooking = Fire->GetFuelSeconds();
    TestTrue(TEXT("Rack cooks the current boar recipe"), Crafting->CraftFromServer(CookedBoarRecipeId, 1, Reason));
    TestEqual(TEXT("Accepted recipe consumes one raw boar meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);
    TestEqual(TEXT("Accepted recipe creates the current output"), Inventory->GetQuantity(TEXT("CookedBoarMeat")), 1);
    TestEqual(TEXT("Accepted recipe awards one Cooking transaction amount"), GetCookingExperience(), 10);
    TestEqual(TEXT("Cooking adds no separate fuel debit"), Fire->GetFuelSeconds(), FuelBeforeCooking);

    TestTrue(TEXT("Fixture provides listed stew vegetables"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1)
        && Inventory->TryGrantFromServer(TEXT("Carrot"), 2)
        && Inventory->TryGrantFromServer(TEXT("Potato"), 2));
    const float FuelBeforeStew = Fire->GetFuelSeconds();
    TestTrue(TEXT("Cauldron prepares the current meat-stew recipe"), Crafting->CraftFromServer(MeatStewRecipeId, 1, Reason));
    TestEqual(TEXT("Stew consumes one boar meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);
    TestEqual(TEXT("Stew consumes one deer meat"), Inventory->GetQuantity(TEXT("DeerMeat")), 0);
    TestEqual(TEXT("Stew consumes both carrots"), Inventory->GetQuantity(TEXT("Carrot")), 0);
    TestEqual(TEXT("Stew consumes both potatoes"), Inventory->GetQuantity(TEXT("Potato")), 0);
    TestEqual(TEXT("Stew creates the current output"), Inventory->GetQuantity(TEXT("MeatStew")), 1);
    TestEqual(TEXT("Each accepted recipe request awards Cooking once"), GetCookingExperience(), 20);
    TestEqual(TEXT("Cauldron recipe adds no separate fuel debit"), Fire->GetFuelSeconds(), FuelBeforeStew);

    TestTrue(TEXT("Hearth broth remains an item in the current catalogue"),
        Inventory->TryGrantFromServer(TEXT("HearthBroth"), 2));
    const int32 ExperienceBeforeMeal = GetCookingExperience();
    TestTrue(TEXT("Current broth item can grant the steady meal"), Crafting->ConsumeFoodFromServer(TEXT("HearthBroth"), Reason));
    TestEqual(TEXT("Meal use consumes one broth serving"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);
    TestEqual(TEXT("Meal grants the fixed server duration"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestEqual(TEXT("Meal use awards no Cooking experience"), GetCookingExperience(), ExperienceBeforeMeal);
    const float MealRemainingBeforeDuplicate = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    TestFalse(TEXT("A duplicate active meal is rejected"), Crafting->ConsumeFoodFromServer(TEXT("HearthBroth"), Reason));
    TestEqual(TEXT("Duplicate rejection preserves the remaining serving"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);
    TestEqual(TEXT("Duplicate rejection preserves the effect timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId), MealRemainingBeforeDuplicate);
    TestFalse(TEXT("A removed food ID cannot be consumed"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Removed-food rejection preserves the current item"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);

    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot invoke the server cooking transaction"), Crafting->CraftFromServer(CookedBoarRecipeId, 1, Reason));
    TestFalse(TEXT("Client cannot invoke the server meal-consumption transaction"), Crafting->ConsumeFoodFromServer(TEXT("HearthBroth"), Reason));
    TestEqual(TEXT("Client requests do not mutate food inventory"), Inventory->GetQuantity(TEXT("HearthBroth")), 1);
    Pawn->SetRole(ROLE_Authority);

    Fire->Destroy();
    Rack->Destroy();
    Cauldron->Destroy();
    World->DestroyWorld(false);
    return true;
}

#endif
