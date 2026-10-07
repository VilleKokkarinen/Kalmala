#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
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
        {TEXT("Stone"), TEXT("Stone"), TEXT("× 2"), false},
        {TEXT("ReedKnife"), TEXT("Reed Knife"), TEXT("Level 1\nCondition 17/40\nDAMAGED\nFree repair at a visible Workbench or Forge."), true}
    };
    OwnerA->SetInventoryRowsForVerification(OwnerARows, 100, 0);
    OwnerB->SetInventoryRowsForVerification({
        {TEXT("Stone"), TEXT("Stone"), TEXT("× 93"), false},
        {TEXT("BronzeAxe"), TEXT("Bronze Axe"), TEXT("Level 1\nCondition 8/55\nBROKEN"), true}
    }, 100, 0);

    TestEqual(TEXT("The first owner selects its first visible pack item"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestTrue(TEXT("Selected item detail includes the first owner's visible count"), DetailText(OwnerA).Contains(TEXT("× 7")));
    TestFalse(TEXT("The first owner's detail excludes the second owner's count"), DetailText(OwnerA).Contains(TEXT("× 93")));
    TestTrue(TEXT("The second owner detail includes its own visible count"), DetailText(OwnerB).Contains(TEXT("× 93")));
    TestFalse(TEXT("The second owner's detail excludes the first owner's count"), DetailText(OwnerB).Contains(TEXT("× 7")));

    UKalmalaCatalogueRowsWidget* OwnerARowsView = nullptr;
    TArray<UWidget*> OwnerAWidgets;
    OwnerA->WidgetTree->GetAllWidgets(OwnerAWidgets);
    for (UWidget* Widget : OwnerAWidgets)
        if (auto* RowsView = Cast<UKalmalaCatalogueRowsWidget>(Widget)) OwnerARowsView = RowsView;
    TestNotNull(TEXT("Inventory creates the shared pack and tool rows view"), OwnerARowsView);
    if (OwnerARowsView)
    {
        TestEqual(TEXT("Tool row remains separate from the sixteen pack slots"), OwnerARowsView->GetCarriedToolCount(), 1);
        TestEqual(TEXT("Tool does not consume a pack slot"), OwnerARowsView->GetFilledSlotCount(), 2);
        TestEqual(TEXT("Pack grid retains its empty cells"), OwnerARowsView->GetEmptySlotCount(), 14);
    }

    OwnerA->StepSelectionForVerification(1);
    TestEqual(TEXT("Next selects the second owner-supplied item"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Stone")));
    TestTrue(TEXT("Selection updates the shared detail component"), DetailText(OwnerA).Contains(TEXT("× 2")));

    OwnerA->StepSelectionForVerification(1);
    TestEqual(TEXT("Selection reaches carried equipment after the pack rows"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("ReedKnife")));
    TestTrue(TEXT("Selected tool detail shows its owner-visible level and condition"), DetailText(OwnerA).Contains(TEXT("Condition 17/40")));
    TestFalse(TEXT("First owner's tool detail excludes the second owner's condition"), DetailText(OwnerA).Contains(TEXT("Condition 8/55")));
    TestTrue(TEXT("Second owner's tool detail keeps its own condition"), DetailText(OwnerB).Contains(TEXT("Condition 8/55")));
    TestFalse(TEXT("Second owner's tool detail excludes the first owner's condition"), DetailText(OwnerB).Contains(TEXT("Condition 17/40")));

    OwnerA->SetInventoryRowsForVerification({{TEXT("Wood"), TEXT("Wood"), TEXT("× 4"), false}}, 100, 0);
    TestEqual(TEXT("Removed selected tool falls back to the first remaining owner row"),
        OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestTrue(TEXT("Fallback replaces the removed tool detail"), DetailText(OwnerA).Contains(TEXT("× 4")));
    TestFalse(TEXT("Fallback does not retain the removed tool condition"), DetailText(OwnerA).Contains(TEXT("Condition 17/40")));

    OwnerA->SetInventoryRowsForVerification({}, 100, 0);
    TestEqual(TEXT("Empty owner pack clears selection"), OwnerA->GetSelectedItemForVerification(), NAME_None);
    TArray<UWidget*> Widgets;
    OwnerA->WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
        if (const auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
            TestEqual(TEXT("Empty pack hides the stale detail panel"), Detail->GetVisibility(), ESlateVisibility::Collapsed);

    return true;
}
#endif
