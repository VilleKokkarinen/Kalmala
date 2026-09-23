#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaRecipeCatalogue.h"
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
        TestEqual(TEXT("Roasting batch remains bounded"), Recipe->MaxBatch, 5);
        TestEqual(TEXT("Roasting yields only the approved food item"), Recipe->Output, FName(TEXT("RoastedFieldMeat")));
    }
    const FKalmalaItemDefinition* Food = Items->FindItem(TEXT("RoastedFieldMeat"));
    TestNotNull(TEXT("Prepared meat has a catalogue-bounded stack"), Food);
    if (Food) TestEqual(TEXT("Prepared meat stack ceiling is twenty"), Food->MaxStack, 20);

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
    if (!Crafting || !Inventory || !Status)
    {
        AddError(TEXT("Food processing pawn components are incomplete"));
        World->DestroyWorld(false);
        return false;
    }

    TestTrue(TEXT("Fixture provides one boar meat and two deer meat"),
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1) && Inventory->TryGrantFromServer(TEXT("DeerMeat"), 2));
    FString Reason;
    TestFalse(TEXT("Roasting rejects a missing fire"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Missing-fire failure preserves raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 1);

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
    TestTrue(TEXT("Lit-hearth roast commits through the recipe exchange"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestEqual(TEXT("Accepted roast consumes exactly one raw meat"), Inventory->GetQuantity(TEXT("BoarMeat")), 0);
    TestEqual(TEXT("Accepted roast creates exactly one cooked meat"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 1);

    TestFalse(TEXT("Over-bound cooking batch rejects"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 6, Reason));
    TestEqual(TEXT("Invalid batch preserves all raw ingredients"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    Fire->AdvanceFromServer(1.0f, 0.1f, 0.0f);
    TestFalse(TEXT("Smouldering fire has no cooking heat"), Fire->IsLit());
    TestFalse(TEXT("Rain-smouldered fire rejects cooking"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 2, Reason));
    TestEqual(TEXT("Heat failure leaves batch ingredients untouched"), Inventory->GetQuantity(TEXT("DeerMeat")), 2);
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Dry fire recovers its existing heat state"), Fire->IsLit());
    TestTrue(TEXT("Two-item batch uses bounded atomic recipe scaling"), Crafting->CraftFromServer(TEXT("RoastDeerMeat"), 2, Reason));
    TestEqual(TEXT("Batch consumes both raw pieces"), Inventory->GetQuantity(TEXT("DeerMeat")), 0);
    TestEqual(TEXT("Batch outputs two cooked pieces"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 3);

    TestTrue(TEXT("One prepared meal consumes exactly one food item"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Accepted consumption removes one cooked meat"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    TestEqual(TEXT("Server applies the fixed meal duration"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestEqual(TEXT("Meal modifier affects authoritative stamina cost"), Status->CalculateStaminaCost(20.0f), 18.0f);
    TestFalse(TEXT("A duplicate active meal is rejected"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Duplicate rejection does not consume more food"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    TestEqual(TEXT("Duplicate rejection does not extend the meal"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestFalse(TEXT("Forged food IDs are rejected"), Crafting->ConsumeFoodFromServer(TEXT("Wood"), Reason));
    TestEqual(TEXT("Forged food rejection leaves inventory alone"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 2);
    TestTrue(TEXT("Owner feedback explains the active benefit and wait rule"), Crafting->GetFoodText().Contains(TEXT("cannot stack or replace")));

    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestFalse(TEXT("Meal expires on server status time"), Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
    TestTrue(TEXT("A new meal is allowed after expiry"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Post-expiry meal consumes once"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), 1);

    const int32 FoodBeforeClientAttempt = Inventory->GetQuantity(TEXT("RoastedFieldMeat"));
    const float MealBeforeClientAttempt = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot invoke the server cooking transaction"), Crafting->CraftFromServer(TEXT("RoastBoarMeat"), 1, Reason));
    TestFalse(TEXT("Client cannot invoke the server consumption transaction"), Crafting->ConsumeFoodFromServer(TEXT("RoastedFieldMeat"), Reason));
    TestEqual(TEXT("Client requests do not mutate food inventory"), Inventory->GetQuantity(TEXT("RoastedFieldMeat")), FoodBeforeClientAttempt);
    TestEqual(TEXT("Client requests do not mutate the active effect"), Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId), MealBeforeClientAttempt);

    World->DestroyWorld(false);
    return true;
}

#endif
