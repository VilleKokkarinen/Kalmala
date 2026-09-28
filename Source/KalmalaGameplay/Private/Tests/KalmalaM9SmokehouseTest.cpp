#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaShelterSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9SmokehouseTest,
    "Kalmala.Gameplay.M9.Smokehouse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9SmokehouseTest::RunTest(const FString& Parameters)
{
    const auto* Items = UKalmalaItemCatalogue::Get();
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Smokehouse item and recipe catalogues validate"), Items->IsValidCatalogue() && Recipes->IsValidCatalogue());
    const FKalmalaItemDefinition* Kit = Items->FindItem(TEXT("SmokehouseKit"));
    if (TestNotNull(TEXT("Smokehouse has a canonical kit item"), Kit))
        TestEqual(TEXT("Smokehouse kit remains bounded"), Kit->MaxStack, 5);

    const FKalmalaRecipe* BuildRecipe = Recipes->Find(TEXT("Smokehouse"));
    if (TestNotNull(TEXT("Smokehouse has a canonical build recipe"), BuildRecipe))
    {
        TestEqual(TEXT("Visible same-world Workbench is the build station"), BuildRecipe->RequiredStationKit, FName(TEXT("WorkbenchKit")));
        TestEqual(TEXT("Build recipe produces one smokehouse kit"), BuildRecipe->Output, FName(TEXT("SmokehouseKit")));
        TestEqual(TEXT("Build request is limited to one kit"), BuildRecipe->MaxBatch, 1);
        TestFalse(TEXT("Assembly does not award Cooking experience"), BuildRecipe->ExperienceAward > 0);
        const auto HasIngredient = [BuildRecipe](const FName ItemId, const int32 Quantity)
        {
            const auto* Ingredient = BuildRecipe->Ingredients.FindByPredicate(
                [ItemId](const FKalmalaInventoryStack& Entry) { return Entry.ItemId == ItemId; });
            return Ingredient && Ingredient->Quantity == Quantity;
        };
        TestTrue(TEXT("Smokehouse costs three Densewood"), HasIngredient(TEXT("Densewood"), 3));
        TestTrue(TEXT("Smokehouse costs two construction supplies"), HasIngredient(TEXT("ConstructionSupply"), 2));
        TestTrue(TEXT("Smokehouse costs two Fibre"), HasIngredient(TEXT("Fibre"), 2));
    }

    for (const FName RecipeId : {FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat"))})
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(RecipeId);
        if (!TestNotNull(TEXT("Existing smoke recipe remains enabled"), Recipe)) continue;
        TestEqual(TEXT("Smoke frame remains the original station"), Recipe->RequiredStationKit, FName(TEXT("SmokeFrameKit")));
        TestEqual(TEXT("Smokehouse is an alternate station"), Recipe->AlternateStationKit, FName(TEXT("SmokehouseKit")));
        TestEqual(TEXT("Smoke recipe Cooking level gate remains two"), Recipe->RequiredSkillLevel, 2);
        TestEqual(TEXT("Smoke recipe keeps its one-Fuel serving cost"), Recipe->Ingredients.Last().ItemId, FName(TEXT("Fuel")));
    }

    TestTrue(TEXT("Smokehouse uses the existing placement preview"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("SmokehouseKit")));
    TestTrue(TEXT("Smokehouse stays out of schema-one construction saves"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("SmokehouseKit")));
    TestTrue(TEXT("Smokehouse is selected as a crafting station"), AKalmalaConstructionActor::IsCraftingStationKit(TEXT("SmokehouseKit")));
    TestEqual(TEXT("Smokehouse placement uses its compact ground footprint"),
        AKalmalaConstructionActor::GetCollisionExtent(TEXT("SmokehouseKit")), FVector(24, 24, 54));
    TestEqual(TEXT("Construction save schema remains version one"), UKalmalaConstructionSaveGame::CurrentSchemaVersion, 1);
    FKalmalaConstructionSaveRecord SaveRecord;
    SaveRecord.ConstructionId = TEXT("session-smokehouse");
    SaveRecord.KitId = TEXT("SmokehouseKit");
    SaveRecord.Transform = FTransform(FVector(100, 200, 300));
    TestFalse(TEXT("Schema one rejects the session-only Smokehouse"), UKalmalaConstructionSaveGame::IsValidRecord(SaveRecord));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Smokehouse test world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    AActor* Occupant = World->SpawnActor<AActor>();
    AKalmalaConstructionActor* Smokehouse = World->SpawnActor<AKalmalaConstructionActor>();
    if (!Occupant || !Smokehouse)
    {
        AddError(TEXT("Smokehouse roof fixture spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    auto* OccupantRoot = NewObject<USceneComponent>(Occupant);
    Occupant->SetRootComponent(OccupantRoot);
    Occupant->AddInstanceComponent(OccupantRoot);
    OccupantRoot->RegisterComponent();
    Smokehouse->InitializeFromServer(TEXT("SmokehouseKit"), TEXT("session-smokehouse-roof"));
    Smokehouse->SetActorLocation(FVector(0, 60, 56));
    TestTrue(TEXT("Smokehouse carries the existing shelter roof tag"), Smokehouse->ActorHasTag(TEXT("KalmalaShelterRoof")));
    TestTrue(TEXT("Roofed smokehouse contributes shelter through the existing server sampler"),
        FKalmalaShelterSampler::Sample(World, Occupant, 0.0f, 0).bHasRoof);
    TestFalse(TEXT("Roof query collision does not block hearth placement under the overhang"),
        World->OverlapBlockingTestByChannel(FVector(120, 0, 56), FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeSphere(54.0f)));

    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!Pawn || !Controller)
    {
        AddError(TEXT("Smokehouse recipe pawn or controller spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    UKalmalaCraftingComponent* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaInventoryComponent* Inventory = Pawn->GetInventoryComponent();
    if (!Crafting || !Inventory)
    {
        AddError(TEXT("Smokehouse recipe pawn components are incomplete"));
        World->DestroyWorld(false);
        return false;
    }
    TestTrue(TEXT("Fixture receives the authored Smokehouse materials"),
        Inventory->TryGrantFromServer(TEXT("Densewood"), 3)
            && Inventory->TryGrantFromServer(TEXT("ConstructionSupply"), 2)
            && Inventory->TryGrantFromServer(TEXT("Fibre"), 2));
    FString Reason;
    TestFalse(TEXT("Smokehouse assembly rejects a missing Workbench"), Crafting->CraftFromServer(TEXT("Smokehouse"), 1, Reason));
    TestEqual(TEXT("Missing Workbench preserves Densewood"), Inventory->GetQuantity(TEXT("Densewood")), 3);

    AKalmalaConstructionActor* Workbench = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Smokehouse Workbench fixture spawned"), Workbench))
    {
        World->DestroyWorld(false);
        return false;
    }
    Workbench->SetActorLocation(Pawn->GetActorLocation() + FVector(150, 0, 0));
    Workbench->InitializeFromServer(TEXT("WorkbenchKit"), TEXT("smokehouse-workbench"));
    TestTrue(TEXT("Visible same-world Workbench passes the station range check"), Workbench->CanUse(Pawn));
    TestFalse(TEXT("Smokehouse build recipe rejects an over-bound batch"), Crafting->CraftFromServer(TEXT("Smokehouse"), 2, Reason));
    TestEqual(TEXT("Invalid Smokehouse batch preserves all build materials"), Inventory->GetQuantity(TEXT("Fibre")), 2);
    TestTrue(TEXT("Visible Workbench accepts the paid Smokehouse recipe"), Crafting->CraftFromServer(TEXT("Smokehouse"), 1, Reason));
    TestEqual(TEXT("Accepted Smokehouse recipe pays Densewood"), Inventory->GetQuantity(TEXT("Densewood")), 0);
    TestEqual(TEXT("Accepted Smokehouse recipe pays construction supplies"), Inventory->GetQuantity(TEXT("ConstructionSupply")), 0);
    TestEqual(TEXT("Accepted Smokehouse recipe pays Fibre"), Inventory->GetQuantity(TEXT("Fibre")), 0);
    TestEqual(TEXT("Accepted Smokehouse recipe creates one kit"), Inventory->GetQuantity(TEXT("SmokehouseKit")), 1);
    TestTrue(TEXT("Owner-facing recipe text explains the Workbench and session gate"),
        Crafting->GetRecipeDescription(TEXT("Smokehouse")).Contains(TEXT("Joiner's bench"))
            && Crafting->GetRecipeDescription(TEXT("Smokehouse")).Contains(TEXT("server session")));

    World->DestroyWorld(false);
    return true;
}

#endif
