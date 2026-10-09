#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryInspectTest, "Kalmala.UI.Inventory.InspectionNavigation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaInventoryInspectTest::RunTest(const FString& Parameters)
{
    auto* Panel = NewObject<UKalmalaInventoryInspectWidget>(); Panel->Initialize();
    Panel->TakeWidget();
    TestNotNull(TEXT("Slate construction creates inspection root before first owner refresh"), Panel->GetRootWidget());
    TArray<FKalmalaCatalogueRow> Rows = {{TEXT("Wood"), TEXT("Wood"), TEXT("Count 7"), false},
        {TEXT("FieldHatchet"), TEXT("Field Hatchet"), TEXT("Condition 17/40"), true}};
    Panel->SetRows(Rows, 150, 1);
    TestTrue(TEXT("Inspection is focusable within modal"), Panel->IsFocusable());
    TestEqual(TEXT("First supplied slot selected"), Panel->GetSelectedItem(), FName(TEXT("Wood")));
    TestTrue(TEXT("Keyboard selects next"), Panel->Navigate(EKeys::Right));
    TestEqual(TEXT("Tool selected"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    TestTrue(TEXT("Controller wraps next"), Panel->Navigate(EKeys::Gamepad_DPad_Down));
    TestEqual(TEXT("Wrap returns first"), Panel->GetSelectedItem(), FName(TEXT("Wood")));
    Panel->Navigate(EKeys::Gamepad_DPad_Left);
    TestEqual(TEXT("Controller wraps previous"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    TestFalse(TEXT("Inspection does not dispatch a use action"), Panel->Navigate(EKeys::Gamepad_FaceButton_Bottom));
    Swap(Rows[0], Rows[1]); Panel->SetRows(Rows, 150, 1);
    TestEqual(TEXT("Refresh preserves canonical selection"), Panel->GetSelectedItem(), FName(TEXT("FieldHatchet")));
    TArray<UWidget*> Widgets; Panel->WidgetTree->GetAllWidgets(Widgets);
    bool bToolPanel = false;
    int32 InitialImageCount = 0;
    bool bInitialImagesLoaded = true;
    FString InitialCardText;
    for (auto* Widget : Widgets)
    {
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Widget))
        {
            ++InitialImageCount;
            bInitialImagesLoaded &= Icon->HasCatalogueTexture();
        }
        if (const auto* Text = Cast<UTextBlock>(Widget)) InitialCardText += Text->GetText().ToString();
        if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
        {
            TArray<UWidget*> Details; Detail->WidgetTree->GetAllWidgets(Details);
            for (auto* Child : Details)
                if (auto* Text = Cast<UTextBlock>(Child))
                    bToolPanel |= Text->GetText().ToString().Contains(TEXT("Condition 17/40"))
                        && Text->GetText().ToString().Contains(TEXT("matching Repair button"));
        }
    }
    TestEqual(TEXT("Owner inventory and carried-tool cards retain one icon each"), InitialImageCount, 2);
    TestTrue(TEXT("Known owner inventory/tool images use imported textures"), bInitialImagesLoaded);
    TestTrue(TEXT("Inventory count and tool condition remain readable beside images"),
        InitialCardText.Contains(TEXT("Count 7")) && InitialCardText.Contains(TEXT("Condition 17/40")));
    TestTrue(TEXT("Selected tool detail includes supplied condition and existing guidance"), bToolPanel);
    auto DetailText = [](UKalmalaInventoryInspectWidget* Owner)
    {
        FString Result;
        TArray<UWidget*> Children; Owner->WidgetTree->GetAllWidgets(Children);
        for (auto* Child : Children)
            if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Child))
            {
                TArray<UWidget*> Texts; Detail->WidgetTree->GetAllWidgets(Texts);
                for (auto* Entry : Texts)
                    if (auto* Text = Cast<UTextBlock>(Entry)) Result += Text->GetText().ToString();
            }
        return Result;
    };
    Rows = {{TEXT("RoastedFieldMeat"), TEXT("Roasted field meat"), TEXT("Count 2"), false},
        {TEXT("Wood"), TEXT("Wood"), TEXT("Count 7"), false}};
    Panel->SetRows(Rows, 150, 1);
    TestEqual(TEXT("Removed selected tool falls back to valid slot"), Panel->GetSelectedItem(), FName(TEXT("RoastedFieldMeat")));
    TestTrue(TEXT("Meal uses existing Eat guidance"), DetailText(Panel).Contains(TEXT("matching Eat button")));
    Rows[0].Detail = TEXT("Count 1"); Panel->SetRows(Rows, 150, 1);
    TestTrue(TEXT("Consumption refreshes selected count"), DetailText(Panel).Contains(TEXT("Count 1")));
    TestFalse(TEXT("Consumption removes previous count"), DetailText(Panel).Contains(TEXT("Count 2")));
    Rows.RemoveAt(0); Panel->SetRows(Rows, 150, 1);
    TestEqual(TEXT("Last meal consumed selects remaining material"), Panel->GetSelectedItem(), FName(TEXT("Wood")));
    TestFalse(TEXT("Removed meal has no stale action guidance"), DetailText(Panel).Contains(TEXT("matching Eat button")));
    auto* OtherOwner = NewObject<UKalmalaInventoryInspectWidget>(); OtherOwner->Initialize();
    OtherOwner->SetRows({{TEXT("Stone"), TEXT("Stone"), TEXT("Count 93"), false}}, 100, 0);
    TestFalse(TEXT("First owner does not acquire other owner count"), DetailText(Panel).Contains(TEXT("Count 93")));
    TestFalse(TEXT("Second owner does not acquire first owner count"), DetailText(OtherOwner).Contains(TEXT("Count 7")));
    TestEqual(TEXT("Second owner selection is independent"), OtherOwner->GetSelectedItem(), FName(TEXT("Stone")));
    auto* ChestView = NewObject<UKalmalaInventoryInspectWidget>();
    ChestView->Initialize(); ChestView->TakeWidget();
    ChestView->SetRows({{TEXT("Fibre"), TEXT("Reed Fibre"), TEXT("Count 4"), false},
        {TEXT("Wood"), TEXT("Wood"), TEXT("Count 7"), false}}, 100, 0,
        TEXT("This chest"), TEXT("This chest is empty."), false, 2);
    TArray<UWidget*> ChestWidgets; ChestView->WidgetTree->GetAllWidgets(ChestWidgets);
    int32 ChestIconCount = 0;
    bool bChestIconsLoaded = true;
    FString ChestCardText;
    int32 MaxChestColumn = -1;
    for (UWidget* Child : ChestWidgets)
    {
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Child))
        {
            ++ChestIconCount;
            bChestIconsLoaded &= Icon->HasCatalogueTexture();
        }
        if (const auto* Text = Cast<UTextBlock>(Child)) ChestCardText += Text->GetText().ToString();
        if (const auto* Grid = Cast<UGridPanel>(Child))
            for (int32 Index = 0; Index < Grid->GetChildrenCount(); ++Index)
                if (const auto* Slot = Cast<UGridSlot>(Grid->GetChildAt(Index)->Slot))
                    MaxChestColumn = FMath::Max(MaxChestColumn, Slot->GetColumn());
    }
    TestEqual(TEXT("Chest selector renders one canonical image per owner-visible stack"), ChestIconCount, 2);
    TestTrue(TEXT("Chest selector loads both item images"), bChestIconsLoaded);
    TestEqual(TEXT("Chest selector uses two columns for readable image/count cards"), MaxChestColumn, 1);
    TestTrue(TEXT("Chest selector keeps both item counts beside their images"),
        ChestCardText.Contains(TEXT("Count 4")) && ChestCardText.Contains(TEXT("Count 7")));
    TestTrue(TEXT("Chest selector shows the reviewed Reed Fibre item name"), ChestCardText.Contains(TEXT("Reed Fibre")));
    auto* UnknownView = NewObject<UKalmalaInventoryInspectWidget>();
    UnknownView->Initialize(); UnknownView->TakeWidget();
    UnknownView->SetRows({{TEXT("Forged"), TEXT("Unknown item"), TEXT("Count 1"), false}}, 100, 0);
    TArray<UWidget*> UnknownWidgets; UnknownView->WidgetTree->GetAllWidgets(UnknownWidgets);
    int32 UnknownIconCount = 0;
    for (UWidget* Child : UnknownWidgets)
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Child))
        {
            ++UnknownIconCount;
            TestFalse(TEXT("Unknown item keeps the vector fallback without loading a texture"), Icon->HasCatalogueTexture());
        }
    TestEqual(TEXT("Unknown identity retains its count card and fallback icon widget"), UnknownIconCount, 1);
    Panel->SetRows({}, 100, 0);
    TestEqual(TEXT("Empty owner data has no selection"), Panel->GetSelectedItem(), NAME_None);
    Panel->Navigate(EKeys::Left);
    TestEqual(TEXT("Empty navigation is bounded"), Panel->GetSelectedItem(), NAME_None);
    return true;
}
#endif
