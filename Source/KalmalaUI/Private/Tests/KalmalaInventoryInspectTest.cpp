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
    Panel->SetRows({}, 100, 0);
    TestEqual(TEXT("Empty owner data has no selection"), Panel->GetSelectedItem(), NAME_None);
    Panel->Navigate(EKeys::Left);
    TestEqual(TEXT("Empty navigation is bounded"), Panel->GetSelectedItem(), NAME_None);
    return true;
}
#endif
