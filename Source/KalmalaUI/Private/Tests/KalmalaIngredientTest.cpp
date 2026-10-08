#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaIngredientWidget.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaIconWidget.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaIngredientTest, "Kalmala.UI.Crafting.IngredientCounts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaIngredientTest::RunTest(const FString& Parameters)
{
    auto* Widget = NewObject<UKalmalaIngredientWidget>();
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient authoritative world"), World)) return false;
    auto* Owner = NewObject<UKalmalaInventoryComponent>(World->SpawnActor<AActor>());
    auto* Peer = NewObject<UKalmalaInventoryComponent>(World->SpawnActor<AActor>());
    TArray<FKalmalaInventoryStack> Costs;
    FKalmalaInventoryStack Cost; Cost.ItemId = TEXT("Wood"); Cost.Quantity = 6; Costs.Add(Cost);
    TestTrue(TEXT("Owner grant accepted"), Owner->TryGrantFromServer(TEXT("Wood"), 9));
    TestTrue(TEXT("Separate owner grant accepted"), Peer->TryGrantFromServer(TEXT("Wood"), 2));
    Widget->SetIngredients(Costs, Owner, 100, 0);
    TestTrue(TEXT("Owned/required and sufficient text"), Widget->GetPresentationText().Contains(TEXT("owned 9 / required 6 — Enough")));
    TArray<UWidget*> IngredientWidgets;
    Widget->WidgetTree->GetAllWidgets(IngredientWidgets);
    int32 IngredientImageCount = 0;
    bool bIngredientImageLoaded = false;
    for (UWidget* Child : IngredientWidgets)
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Child))
        {
            ++IngredientImageCount;
            bIngredientImageLoaded |= Icon->HasCatalogueTexture();
        }
    TestEqual(TEXT("Ingredient row retains one image beside readable owned/required counts"), IngredientImageCount, 1);
    TestTrue(TEXT("Ingredient image uses the imported canonical texture"), bIngredientImageLoaded);
    TestTrue(TEXT("Accepted consumption"), Owner->TryConsumeFromServer(TEXT("Wood"), 5));
    Widget->SetIngredients(Costs, Owner, 150, 1);
    TestTrue(TEXT("Refresh follows consumption and missing state"), Widget->GetPresentationText().Contains(TEXT("owned 4 / required 6 — Missing")));
    Widget->SetIngredients(Costs, Peer, 150, 1);
    TestTrue(TEXT("Only supplied owner count shown"), Widget->GetPresentationText().Contains(TEXT("owned 2 / required 6 — Missing")));
    TestEqual(TEXT("Presentation does not spend owner inventory"), Owner->GetQuantity(TEXT("Wood")), 4);
    TestEqual(TEXT("Presentation does not spend peer inventory"), Peer->GetQuantity(TEXT("Wood")), 2);
    Widget->SetIngredients(Costs, nullptr, 100, 0);
    TestTrue(TEXT("Missing pack is pending, not false zero"), Widget->GetPresentationText().Contains(TEXT("owned pending / required 6")));
    Widget->SetIngredients({}, Owner, 100, 0);
    TestTrue(TEXT("No selection clears stale costs"), Widget->GetPresentationText().IsEmpty());
    for (const auto& Recipe : UKalmalaRecipeCatalogue::Get()->Recipes)
    {
        Costs = Recipe.Ingredients;
        if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.Output))
        {
            FString Reason;
            TestTrue(TEXT("Direct build resolves real raw costs"), UKalmalaRecipeCatalogue::BuildDirectMaterialCost(Recipe.Output, Costs, Reason));
        }
        for (const auto& Ingredient : Costs)
        {
            EKalmalaIcon Kind; int32 Variant;
            TestTrue(TEXT("Every current ingredient reuses canonical icon"), UKalmalaIconWidget::FindCatalogueIcon(Ingredient.ItemId, Kind, Variant));
        }
        Widget->SetIngredients(Costs, Owner, 150, 1);
        TestEqual(TEXT("Recipe refresh leaves materials untouched"), Owner->GetQuantity(TEXT("Wood")), 4);
    }
    World->DestroyWorld(false);
    return true;
}
#endif
