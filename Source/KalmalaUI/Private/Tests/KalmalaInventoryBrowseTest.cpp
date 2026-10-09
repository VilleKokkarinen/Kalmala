#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaIconWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryBrowseTest, "Kalmala.UI.Inventory.LocalBrowsing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaInventoryBrowseTest::RunTest(const FString& Parameters)
{
    auto* Panel = NewObject<UKalmalaInventoryInspectWidget>(); Panel->Initialize(); Panel->TakeWidget();
    const TArray<FKalmalaCatalogueRow> Source = {
        {TEXT("Wood"), TEXT("Wood"), TEXT("Count 7"), false},
        {TEXT("FieldHatchet"), TEXT("Field Hatchet"), TEXT("Condition 17/40"), true},
        {TEXT("Stone"), TEXT("Stone"), TEXT("Count 3"), false},
        {TEXT("Fibre"), TEXT("Reed Fibre"), TEXT("Count 4"), false}};
    Panel->SetRows(Source, 150, 1);
    TArray<UWidget*> InitialWidgets;
    Panel->WidgetTree->GetAllWidgets(InitialWidgets);
    int32 InitialIconCount = 0;
    bool bInitialIconsLoaded = true;
    bool bReedFibreVisible = false;
    for (UWidget* Widget : InitialWidgets)
    {
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Widget))
        {
            ++InitialIconCount;
            bInitialIconsLoaded &= Icon->HasCatalogueTexture();
        }
        if (const auto* Text = Cast<UTextBlock>(Widget))
            bReedFibreVisible |= Text->GetText().ToString().Contains(TEXT("Reed Fibre"));
    }
    EKalmalaIcon FibreIcon = EKalmalaIcon::Fibre;
    int32 FibreIconVariant = INDEX_NONE;
    TestTrue(TEXT("Reviewed Reed Fibre name is visible on its inventory row"), bReedFibreVisible);
    TestTrue(TEXT("Reed Fibre retains the canonical Fibre icon mapping"),
        UKalmalaIconWidget::FindCatalogueIcon(FName(TEXT("Fibre")), FibreIcon, FibreIconVariant));
    TestEqual(TEXT("Every inventory row retains its canonical icon widget"), InitialIconCount, Source.Num());
    TestTrue(TEXT("Inventory rows load their canonical item and tool icons"), bInitialIconsLoaded);
    Panel->Navigate(EKeys::Right);
    Panel->SetSort(1);
    TestEqual(TEXT("Name sorting retains selected canonical identity"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    Panel->Navigate(EKeys::Right);
    TestEqual(TEXT("Name sorting uses the reviewed Reed Fibre display name"), Panel->GetSelectedItem(), FName(TEXT("Fibre")));
    Panel->Navigate(EKeys::Right);
    TestEqual(TEXT("Name sorting navigates from Reed Fibre to Stone"), Panel->GetSelectedItem(), FName(TEXT("Stone")));
    Panel->SetSearch(TEXT("  REED FIBRE  "));
    TestEqual(TEXT("Trimmed search finds the reviewed item display name"), Panel->GetVisibleCount(), 1);
    TestEqual(TEXT("Display-name search retains the canonical item identity"), Panel->GetSelectedItem(), FName(TEXT("Fibre")));
    Panel->SetSearch(TEXT(""));
    Panel->SetCategory(2);
    TestEqual(TEXT("Tools filter contains only supplied tool"), Panel->GetVisibleCount(), 1);
    TestEqual(TEXT("Filtered selection falls back safely"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    Panel->SetSearch(TEXT("  HATCHET  "));
    TestEqual(TEXT("Trimmed case-insensitive display-name search"), Panel->GetVisibleCount(), 1);
    Panel->SetSearch(TEXT("Count 7"));
    TestEqual(TEXT("Search does not match state or quantities"), Panel->GetVisibleCount(), 0);
    TestEqual(TEXT("No results clears selection"), Panel->GetSelectedItem(), NAME_None);
    Panel->Navigate(EKeys::Gamepad_DPad_Right);
    TestEqual(TEXT("No-results controller navigation is bounded"), Panel->GetSelectedItem(), NAME_None);
    TArray<UWidget*> Widgets; Panel->WidgetTree->GetAllWidgets(Widgets);
    bool bNoResults = false; UEditableTextBox* SearchBox = nullptr;
    for (auto* Widget : Widgets)
    {
        if (auto* Text = Cast<UTextBlock>(Widget)) bNoResults |= Text->GetText().ToString().Contains(TEXT("No results"));
        if (auto* Edit = Cast<UEditableTextBox>(Widget)) SearchBox = Edit;
    }
    TestTrue(TEXT("No results explains recovery"), bNoResults);
    TestNotNull(TEXT("Search has actual editable control"), SearchBox);
    if (SearchBox) SearchBox->OnTextChanged.Broadcast(FText::FromString(TEXT("field")));
    TestEqual(TEXT("Actual text-change binding filters supplied rows"), Panel->GetVisibleCount(), 1);
    Panel->SetSearch(TEXT("")); Panel->SetCategory(0); Panel->SetSort(2);
    Panel->Navigate(EKeys::Gamepad_DPad_Right);
    TestEqual(TEXT("Grouped view wraps from tool to first named item"), Panel->GetSelectedItem(), FName(TEXT("Fibre")));
    Panel->SetSort(0); Panel->Navigate(EKeys::Left);
    TestEqual(TEXT("Owner order is unchanged after sorting"), Panel->GetSelectedItem(), FName(TEXT("Stone")));
    TestEqual(TEXT("Caller inventory order remains unchanged"), Source[0].Id, FName(TEXT("Wood")));
    Panel->SetCategory(1);
    TestEqual(TEXT("Items excludes carried tools"), Panel->GetVisibleCount(), 3);
    Panel->Navigate(EKeys::Gamepad_LeftShoulder);
    TestEqual(TEXT("Controller category cycling reaches tools"), Panel->GetVisibleCount(), 1);
    Panel->Navigate(EKeys::PageUp);
    TestEqual(TEXT("Keyboard category cycling wraps to All"), Panel->GetVisibleCount(), 4);
    Panel->Navigate(EKeys::Gamepad_RightShoulder);
    TestEqual(TEXT("Controller sorting retains canonical selection"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    Panel->SetCategory(1);
    Panel->SetRows({{TEXT("Wood"), TEXT("Wood"), TEXT("Count 6"), false}}, 100, 0);
    TestEqual(TEXT("Owner refresh retains filters and excludes removed rows"), Panel->GetVisibleCount(), 1);
    TestEqual(TEXT("Removed selection falls back to remaining row"), Panel->GetSelectedItem(), FName(TEXT("Wood")));
    Panel->SetSearch(TEXT("secret reward"));
    TestEqual(TEXT("Search never loads absent catalogue entries"), Panel->GetVisibleCount(), 0);
    Panel->SetRows({}, 100, 0); Panel->SetSearch(TEXT("")); Panel->SetCategory(0);
    TestEqual(TEXT("Empty owner inventory remains empty"), Panel->GetVisibleCount(), 0);
    return true;
}
#endif
