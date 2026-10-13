#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaInventoryGridWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaCraftingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryMenuSelectionTest, "Kalmala.UI.InventoryMenu.Selection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryMenuSelectionTest::RunTest(const FString& Parameters)
{
    auto* OwnerA = NewObject<UKalmalaInventoryMenuWidget>(); OwnerA->Initialize();
    auto* OwnerB = NewObject<UKalmalaInventoryMenuWidget>(); OwnerB->Initialize();
    const TArray<FKalmalaCatalogueRow> Rows = {
        {TEXT("Wood"), TEXT("Wood"), TEXT("× 7"), false},
        {TEXT("StoneKnife"), TEXT("Stone Knife"), TEXT("Level 1\nCondition 17/40"), true}};
    OwnerA->SetInventoryRowsForVerification(Rows, 150, 1);
    OwnerB->SetInventoryRowsForVerification({{TEXT("Stone"), TEXT("Stone"), TEXT("× 93"), false}}, 100, 0);
    const auto DetailText = [](UKalmalaInventoryMenuWidget* Menu)
    {
        FString Text;
        Menu->WidgetTree->ForEachWidget([&](UWidget* Widget) {
            if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
                Detail->WidgetTree->ForEachWidget([&](UWidget* Child) {
                    if (const auto* Block = Cast<UTextBlock>(Child)) Text += Block->GetText().ToString();
                });
        });
        return Text;
    };
    TestEqual(TEXT("Normal gameplay keeps Inventory collapsed until opened"), OwnerA->GetVisibility(), ESlateVisibility::Collapsed);
    TestEqual(TEXT("First owned item is selected"), OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestFalse(TEXT("Item detail stays hidden until an item is focused or hovered"), DetailText(OwnerA).Contains(TEXT("× 7")));
    OwnerA->SelectItemForVerification(TEXT("Wood"));
    TestTrue(TEXT("Owner detail includes its quantity"), DetailText(OwnerA).Contains(TEXT("× 7")));
    TestFalse(TEXT("Owner detail excludes peer contents"), DetailText(OwnerA).Contains(TEXT("× 93")));
    OwnerA->SelectItemForVerification(TEXT("StoneKnife"));
    TestTrue(TEXT("Tool cell selects existing condition and repair detail"), DetailText(OwnerA).Contains(TEXT("Condition 17/40")));
    OwnerA->SetInventoryRowsForVerification(Rows, 150, 1);
    TestEqual(TEXT("Owner refresh preserves selected identity"), OwnerA->GetSelectedItemForVerification(), FName(TEXT("StoneKnife")));
    int32 Grids = 0, Searches = 0;
    bool bArmor = false, bWeight = false;
    OwnerA->WidgetTree->ForEachWidget([&](UWidget* Widget) {
        if (Cast<UKalmalaInventoryGridWidget>(Widget)) ++Grids;
        if (Cast<UEditableTextBox>(Widget)) ++Searches;
        if (const auto* Text = Cast<UTextBlock>(Widget)) {
            bArmor |= Text->GetText().ToString().Contains(TEXT("Armor 0"));
            bWeight |= Text->GetText().ToString().Contains(TEXT("Weight "));
        }
    });
    TestEqual(TEXT("Player inventory mounts exactly one grid"), Grids, 1);
    TestEqual(TEXT("Simple inventory has no search/filter/sort input"), Searches, 0);
    TestFalse(TEXT("Armor summary is absent from Inventory"), bArmor);
    TestFalse(TEXT("Weight summary is absent from Inventory"), bWeight);
    OwnerA->WidgetTree->ForEachWidget([&](UWidget* Widget) {
        TestFalse(TEXT("Crafting is not parented inside the Inventory tree"), Widget->IsA<UKalmalaCraftingWidget>());
        if (const auto* Text = Cast<UTextBlock>(Widget))
            TestFalse(TEXT("Inventory has no title heading"), Text->GetText().ToString() == TEXT("Inventory"));
    });
    OwnerA->SetViewportSizeForVerification(FVector2D(320, 240));
    TestTrue(TEXT("Panel remains inside small viewport"), OwnerA->GetPanelSizeForVerification().X <= 288
        && OwnerA->GetPanelSizeForVerification().Y <= 208);
    OwnerA->SetInventoryRowsForVerification({{TEXT("Wood"), TEXT("Wood"), TEXT("× 4"), false}}, 100, 0);
    TestEqual(TEXT("Removed tool falls back safely"), OwnerA->GetSelectedItemForVerification(), FName(TEXT("Wood")));
    TestFalse(TEXT("Removed tool detail is cleared"), DetailText(OwnerA).Contains(TEXT("Condition 17/40")));
    OwnerA->SetInventoryRowsForVerification({}, 100, 0);
    TestEqual(TEXT("Empty owner pack clears selection"), OwnerA->GetSelectedItemForVerification(), NAME_None);
    OwnerA->WidgetTree->ForEachWidget([&](UWidget* Widget) {
        if (const auto* Grid = Cast<UKalmalaInventoryGridWidget>(Widget))
            TestEqual(TEXT("Empty Inventory retains forty grid cells"), UKalmalaInventoryGridWidget::VisibleSlots({}, false).Num(), 40);
        if (const auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
            TestEqual(TEXT("Empty pack hides the stale detail panel"), Detail->GetVisibility(), ESlateVisibility::Collapsed);
    });
    TestEqual(TEXT("Other owner selection remains independent"), OwnerB->GetSelectedItemForVerification(), FName(TEXT("Stone")));
    return true;
}
#endif
