#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "Blueprint/WidgetTree.h"
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
        {TEXT("FieldHatchet"), TEXT("Field hatchet"), TEXT("Condition 17/40"), true}};
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
    for (auto* Widget : Widgets)
        if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
        {
            TArray<UWidget*> Details; Detail->WidgetTree->GetAllWidgets(Details);
            for (auto* Child : Details)
                if (auto* Text = Cast<UTextBlock>(Child))
                    bToolPanel |= Text->GetText().ToString().Contains(TEXT("Condition 17/40"))
                        && Text->GetText().ToString().Contains(TEXT("matching Repair button"));
        }
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
    Panel->SetRows({}, 100, 0);
    TestEqual(TEXT("Empty owner data has no selection"), Panel->GetSelectedItem(), NAME_None);
    Panel->Navigate(EKeys::Left);
    TestEqual(TEXT("Empty navigation is bounded"), Panel->GetSelectedItem(), NAME_None);
    return true;
}
#endif
