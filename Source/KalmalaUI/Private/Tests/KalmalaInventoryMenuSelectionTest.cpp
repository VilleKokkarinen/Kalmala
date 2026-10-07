#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryMenuSelectionTest,
    "Kalmala.UI.InventoryMenu.Selection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryMenuSelectionTest::RunTest(const FString& Parameters)
{
    auto MakeMenu = []()
    {
        auto* Menu = NewObject<UKalmalaInventoryMenuWidget>();
        Menu->Initialize();
        return Menu;
    };
    auto DetailText = [](UKalmalaInventoryMenuWidget* Menu)
    {
        FString Text;
        TArray<UWidget*> Widgets;
        Menu->WidgetTree->GetAllWidgets(Widgets);
        for (UWidget* Widget : Widgets)
        {
            if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
            {
                TArray<UWidget*> DetailWidgets;
                Detail->WidgetTree->GetAllWidgets(DetailWidgets);
                for (UWidget* DetailWidget : DetailWidgets)
                    if (const auto* Block = Cast<UTextBlock>(DetailWidget)) Text += Block->GetText().ToString();
            }
        }
        return Text;
    };

    UKalmalaInventoryMenuWidget* OwnerA = MakeMenu();
    UKalmalaInventoryMenuWidget* OwnerB = MakeMenu();
    TestNotNull(TEXT("First owner inventory menu initializes"), OwnerA);
    TestNotNull(TEXT("Second owner inventory menu initializes"), OwnerB);
    if (!OwnerA || !OwnerB) return false;

    const TArray<FKalmalaCatalogueRow> OwnerARows = {
        {TEXT("Wood"), TEXT("Wood"), TEXT("× 7"), false},
        {TEXT("Stone"), TEXT("Stone"), TEXT("× 2"), false}
    };
    OwnerA->SetPackRowsForVerification(OwnerARows, 100, 0);
    OwnerB->SetPackRowsForVerification({{TEXT("Stone"), TEXT("Stone"), TEXT("× 93"), false}}, 100, 0);

    TestEqual(TEXT("The first owner selects its first visible pack item"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestTrue(TEXT("Selected item detail includes the first owner's visible count"), DetailText(OwnerA).Contains(TEXT("× 7")));
    TestFalse(TEXT("The first owner's detail excludes the second owner's count"), DetailText(OwnerA).Contains(TEXT("× 93")));
    TestTrue(TEXT("The second owner detail includes its own visible count"), DetailText(OwnerB).Contains(TEXT("× 93")));
    TestFalse(TEXT("The second owner's detail excludes the first owner's count"), DetailText(OwnerB).Contains(TEXT("× 7")));

    OwnerA->StepSelectionForVerification(1);
    TestEqual(TEXT("Next selects the second owner-supplied item"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Stone")));
    TestTrue(TEXT("Selection updates the shared detail component"), DetailText(OwnerA).Contains(TEXT("× 2")));

    OwnerA->SetPackRowsForVerification({{TEXT("Wood"), TEXT("Wood"), TEXT("× 4"), false}}, 100, 0);
    TestEqual(TEXT("Removed selected item falls back to the first remaining item"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestTrue(TEXT("Fallback replaces the removed item's detail"), DetailText(OwnerA).Contains(TEXT("× 4")));
    TestFalse(TEXT("Fallback does not retain the removed item's count"), DetailText(OwnerA).Contains(TEXT("× 2")));

    OwnerA->SetPackRowsForVerification({}, 100, 0);
    TestEqual(TEXT("Empty owner pack clears selection"), OwnerA->GetSelectedItemForVerification(), NAME_None);
    TArray<UWidget*> Widgets;
    OwnerA->WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (const auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
            TestEqual(TEXT("Empty pack hides the stale detail panel"), Detail->GetVisibility(), ESlateVisibility::Collapsed);

    return true;
}
#endif
