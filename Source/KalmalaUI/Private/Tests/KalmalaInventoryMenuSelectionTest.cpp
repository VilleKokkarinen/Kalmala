#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "KalmalaThemedButton.h"
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

    UKalmalaInventoryMenuWidget* BrowsingMenu = MakeMenu();
    TestNotNull(TEXT("Inventory browsing menu initializes"), BrowsingMenu);
    if (BrowsingMenu)
    {
        const TArray<FKalmalaCatalogueRow> BrowseRows = {
            {TEXT("Wood"), TEXT("Wood"), TEXT("Count 7"), false},
            {TEXT("Stone"), TEXT("Stone"), TEXT("Count 3"), false},
            {TEXT("FieldHatchet"), TEXT("Field hatchet"), TEXT("Condition 17/40"), true}};
        auto VisibleIdsAre = [BrowsingMenu](const TArray<FName>& Expected)
        {
            return BrowsingMenu->GetVisibleItemIdsForVerification() == Expected;
        };
        BrowsingMenu->SetInventoryRowsForVerification(BrowseRows, 150, 1);
        TestTrue(TEXT("Search, category, sort and clear controls remain focusable at enlarged high contrast"),
            BrowsingMenu->HasBrowseFocusTargetsForVerification());
        TestTrue(TEXT("Owner order initially preserves pack/tool source order"),
            VisibleIdsAre(TArray<FName>{FName(TEXT("Wood")), FName(TEXT("Stone")), FName(TEXT("FieldHatchet"))}));

        int32 ScrollBoxCount = 0;
        UEditableTextBox* SearchBox = nullptr;
        UKalmalaThemedButton* CategoryButton = nullptr;
        UKalmalaThemedButton* SortButton = nullptr;
        UKalmalaThemedButton* ClearSearchButton = nullptr;
        FString BrowseLabels;
        TArray<UWidget*> BrowseWidgets;
        BrowsingMenu->WidgetTree->GetAllWidgets(BrowseWidgets);
        for (UWidget* Widget : BrowseWidgets)
        {
            if (Cast<UScrollBox>(Widget)) ++ScrollBoxCount;
            if (auto* Edit = Cast<UEditableTextBox>(Widget)) SearchBox = Edit;
            if (const auto* Text = Cast<UTextBlock>(Widget)) BrowseLabels += Text->GetText().ToString();
            if (auto* Button = Cast<UKalmalaThemedButton>(Widget))
            {
                const auto* Label = Cast<UTextBlock>(Button->GetContent());
                if (Label && Label->GetText().ToString().StartsWith(TEXT("Category:"))) CategoryButton = Button;
                else if (Label && Label->GetText().ToString().StartsWith(TEXT("Sort:"))) SortButton = Button;
                else if (Label && Label->GetText().ToString() == TEXT("Clear search")) ClearSearchButton = Button;
            }
        }
        TestTrue(TEXT("Menu and item area retain scroll fallbacks for viewport/text growth"), ScrollBoxCount >= 2);
        BrowsingMenu->SetViewportSizeForVerification(FVector2D(480.0f, 320.0f));
        TestEqual(TEXT("Inventory menu width leaves a viewport margin after resize"),
            BrowsingMenu->GetPanelSizeForVerification().X, 448.0f);
        TestEqual(TEXT("Inventory menu height leaves room for outer scrolling after resize"),
            BrowsingMenu->GetPanelSizeForVerification().Y, 288.0f);
        BrowsingMenu->SetViewportSizeForVerification(FVector2D(1024.0f, 768.0f));
        TestEqual(TEXT("Inventory menu returns to its normal width after resize"),
            BrowsingMenu->GetPanelSizeForVerification().X, 640.0f);
        TestEqual(TEXT("Inventory menu returns to its normal height after resize"),
            BrowsingMenu->GetPanelSizeForVerification().Y, 560.0f);
        TestNotNull(TEXT("Inventory has an editable search control"), SearchBox);
        TestTrue(TEXT("Browse controls have visible category and sort labels"),
            BrowseLabels.Contains(TEXT("Category: All")) && BrowseLabels.Contains(TEXT("Sort: Owner order")));
        if (CategoryButton)
        {
            CategoryButton->OnClicked.Broadcast();
            TestEqual(TEXT("Category button cycles to Items"), BrowsingMenu->GetInventoryCategoryForVerification(), 1);
            CategoryButton->OnClicked.Broadcast();
            TestEqual(TEXT("Category button cycles to Carried tools"), BrowsingMenu->GetInventoryCategoryForVerification(), 2);
            CategoryButton->OnClicked.Broadcast();
            TestEqual(TEXT("Category button wraps to All"), BrowsingMenu->GetInventoryCategoryForVerification(), 0);
        }
        if (SortButton)
        {
            SortButton->OnClicked.Broadcast();
            TestEqual(TEXT("Sort button cycles to Name"), BrowsingMenu->GetInventorySortForVerification(), 1);
            SortButton->OnClicked.Broadcast();
            TestEqual(TEXT("Sort button cycles to Category/name"), BrowsingMenu->GetInventorySortForVerification(), 2);
            SortButton->OnClicked.Broadcast();
            TestEqual(TEXT("Sort button wraps to owner order"), BrowsingMenu->GetInventorySortForVerification(), 0);
        }
        if (SearchBox)
        {
            SearchBox->OnTextChanged.Broadcast(FText::FromString(TEXT("stone")));
            TestTrue(TEXT("Editable text-change delegate updates visible search results"),
                VisibleIdsAre(TArray<FName>{FName(TEXT("Stone"))}));
            SearchBox->OnTextChanged.Broadcast(FText::GetEmpty());
        }

        BrowsingMenu->SetInventoryBrowseForVerification(TEXT("  HATCHET  "), 2, 1);
        TestTrue(TEXT("Search trims and matches visible names without case sensitivity"),
            VisibleIdsAre(TArray<FName>{FName(TEXT("FieldHatchet"))}));
        TestEqual(TEXT("Search trims outer whitespace"), BrowsingMenu->GetInventorySearchForVerification(), FString(TEXT("HATCHET")));
        TestEqual(TEXT("Carried-tool category is retained for this menu session"),
            BrowsingMenu->GetInventoryCategoryForVerification(), 2);
        BrowsingMenu->SetInventoryBrowseForVerification(TEXT(""), 1, 0);
        TestTrue(TEXT("Items category excludes carried tools"),
            VisibleIdsAre(TArray<FName>{FName(TEXT("Wood")), FName(TEXT("Stone"))}));
        BrowsingMenu->SetInventoryBrowseForVerification(FString::ChrN(80, TCHAR('a')), 0, 0);
        TestEqual(TEXT("Search input is bounded to 64 characters"), BrowsingMenu->GetInventorySearchForVerification().Len(), 64);

        BrowsingMenu->SetInventoryBrowseForVerification(TEXT("Condition 17"), 0, 0);
        TestEqual(TEXT("Search does not match private condition/details"), BrowsingMenu->GetVisibleItemIdsForVerification().Num(), 0);
        TestEqual(TEXT("No-results state clears selection"), BrowsingMenu->GetSelectedItemForVerification(), NAME_None);
        TestFalse(TEXT("No-results state hides stale item detail"),
            DetailText(BrowsingMenu).Contains(TEXT("Condition 17/40")));
        bool bNoResultsGuidance = false;
        BrowsingMenu->WidgetTree->GetAllWidgets(BrowseWidgets);
        for (UWidget* Widget : BrowseWidgets)
            if (const auto* Text = Cast<UTextBlock>(Widget); Text && Text->GetText().ToString().Contains(TEXT("No results"))) bNoResultsGuidance = true;
        TestTrue(TEXT("No-results state provides search recovery guidance"), bNoResultsGuidance);
        TestTrue(TEXT("No-results controller navigation is handled safely"),
            BrowsingMenu->NavigateForVerification(EKeys::Gamepad_DPad_Right));
        TestEqual(TEXT("No-results controller navigation selects nothing"), BrowsingMenu->GetSelectedItemForVerification(), NAME_None);

        BrowsingMenu->SetInventoryBrowseForVerification(TEXT(""), 0, 1);
        TestTrue(TEXT("Name sort uses deterministic display-name order"),
            VisibleIdsAre(TArray<FName>{FName(TEXT("Stone")), FName(TEXT("Wood")), FName(TEXT("FieldHatchet"))}));
        BrowsingMenu->SetInventoryBrowseForVerification(TEXT(""), 0, 2);
        TestTrue(TEXT("Category/name sort keeps items ahead of carried tools"),
            VisibleIdsAre(TArray<FName>{FName(TEXT("Stone")), FName(TEXT("Wood")), FName(TEXT("FieldHatchet"))}));
        TestTrue(TEXT("Keyboard category cycle is routed"), BrowsingMenu->NavigateForVerification(EKeys::PageUp));
        TestEqual(TEXT("Keyboard category cycle reaches Items"), BrowsingMenu->GetInventoryCategoryForVerification(), 1);
        TestTrue(TEXT("Controller shoulder sort cycle is routed"), BrowsingMenu->NavigateForVerification(EKeys::Gamepad_RightShoulder));
        TestEqual(TEXT("Controller shoulder changes the session sort mode"), BrowsingMenu->GetInventorySortForVerification(), 0);

        BrowsingMenu->SetInventoryBrowseForVerification(TEXT(""), 0, 0);
        TestTrue(TEXT("D-pad changes the selected canonical inventory item"),
            BrowsingMenu->NavigateForVerification(EKeys::Gamepad_DPad_Right));
        TestTrue(TEXT("D-pad can move to the third owner-order item"),
            BrowsingMenu->NavigateForVerification(EKeys::Gamepad_DPad_Right));
        TestEqual(TEXT("Session selection tracks the canonical item"),
            BrowsingMenu->GetSelectedItemForVerification(), FName(TEXT("FieldHatchet")));
        BrowsingMenu->SetInventoryBrowseForVerification(TEXT("o"), 0, 0);
        BrowsingMenu->SetInventoryScrollOffsetForVerification(72.0f);
        BrowsingMenu->SetInventoryRowsForVerification({
            {TEXT("Wood"), TEXT("Wood"), TEXT("Count 6"), false},
            {TEXT("Stone"), TEXT("Stone"), TEXT("Count 3"), false}}, 150, 1);
        TestEqual(TEXT("Removed selected item falls back to the first remaining row"),
            BrowsingMenu->GetSelectedItemForVerification(), FName(TEXT("Wood")));
        TestEqual(TEXT("Browse query remains local to this menu session"), BrowsingMenu->GetInventorySearchForVerification(), FString(TEXT("o")));
        TestEqual(TEXT("Scroll position is retained when owner rows refresh"),
            BrowsingMenu->GetInventoryScrollOffsetForVerification(), 72.0f);
        BrowsingMenu->SetInventoryBrowseForVerification(TEXT("secret reward"), 0, 0);
        TestEqual(TEXT("Search does not reveal absent catalogue entries"), BrowsingMenu->GetVisibleItemIdsForVerification().Num(), 0);
        TestNotNull(TEXT("Clear search control remains available after no results"), ClearSearchButton);
        if (ClearSearchButton) ClearSearchButton->OnClicked.Broadcast();
        TestEqual(TEXT("Clearing no-results restores the session selection"),
            BrowsingMenu->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    }

    UKalmalaInventoryMenuWidget* FoodMenu = MakeMenu();
    TestNotNull(TEXT("Food inventory menu initializes"), FoodMenu);
    if (FoodMenu)
    {
        FoodMenu->SetInventoryRowsForVerification({
            {TEXT("HearthBroth"), TEXT("Hearth Broth"), TEXT("× 1"), false}
        }, 100, 0);
        TestTrue(TEXT("Supported food detail shows its actual Steady Meal effect"),
            DetailText(FoodMenu).Contains(TEXT("10% lower stamina use for 120 seconds")));

        UKalmalaThemedButton* EatButton = nullptr;
        FString MenuText;
        TArray<UWidget*> FoodWidgets;
        FoodMenu->WidgetTree->GetAllWidgets(FoodWidgets);
        for (UWidget* Widget : FoodWidgets)
        {
            if (const auto* Text = Cast<UTextBlock>(Widget)) MenuText += Text->GetText().ToString();
            if (auto* Button = Cast<UKalmalaThemedButton>(Widget))
            {
                const auto* Label = Cast<UTextBlock>(Button->GetContent());
                if (Label && Label->GetText().ToString() == TEXT("Eat one serving")) EatButton = Button;
            }
        }
        TestNotNull(TEXT("Supported food exposes the Eat action"), EatButton);
        if (EatButton)
        {
            TestEqual(TEXT("Eat stays unavailable without live owner data"),
                EatButton->GetVisibility(), ESlateVisibility::Visible);
            TestFalse(TEXT("Eat stays disabled without the local owner components"), EatButton->GetIsEnabled());
        }
        TestTrue(TEXT("Unavailable owner food action explains the missing data"),
            MenuText.Contains(TEXT("Food use is unavailable while owner data is loading.")));
    }

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
