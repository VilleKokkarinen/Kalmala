#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryGridWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryGridWidgetTest, "Kalmala.UI.Inventory.NumberedHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaInventoryGridWidgetTest::RunTest(const FString& Parameters)
{
    TArray<FName> Slots;
    Slots.Init(NAME_None, 40);
    TestEqual(TEXT("Inventory always retains forty cells"), UKalmalaInventoryGridWidget::VisibleSlots(Slots, false).Num(), 40);
    TestTrue(TEXT("Empty hotbar hides every cell"), UKalmalaInventoryGridWidget::VisibleSlots(Slots, true).IsEmpty());
    Slots[0] = TEXT("StoneKnife"); Slots[3] = TEXT("HearthBroth"); Slots[9] = TEXT("StoneAxe"); Slots[10] = TEXT("Wood");
    const auto Visible = UKalmalaInventoryGridWidget::VisibleSlots(Slots, true);
    TestTrue(TEXT("Hotbar omits empty cells and lower rows without renumbering"), Visible == TArray<int32>({0, 3, 9}));
    TestEqual(TEXT("First cell is key one"), UKalmalaInventoryGridWidget::SlotLabel(0), FString(TEXT("1")));
    TestEqual(TEXT("Tenth cell is key zero, after nine"), UKalmalaInventoryGridWidget::SlotLabel(9), FString(TEXT("0")));
    TestTrue(TEXT("Lower rows have no hotbar number"), UKalmalaInventoryGridWidget::SlotLabel(10).IsEmpty());
    Slots[3] = NAME_None;
    TestTrue(TEXT("Consumed hotbar item removes only its HUD cell"),
        UKalmalaInventoryGridWidget::VisibleSlots(Slots, true) == TArray<int32>({0, 9}));
    Slots[0] = NAME_None;
    TestTrue(TEXT("A tool moved into Main Hand leaves no numbered HUD alias"),
        UKalmalaInventoryGridWidget::VisibleSlots(Slots, true) == TArray<int32>({9}));
    Slots[1] = TEXT("StoneKnife");
    TestTrue(TEXT("A returned tool uses its current pack cell number"),
        UKalmalaInventoryGridWidget::VisibleSlots(Slots, true) == TArray<int32>({1, 9}));
    TestEqual(TEXT("Returned tool in cell two is labelled two"), UKalmalaInventoryGridWidget::SlotLabel(1), FString(TEXT("2")));
    return true;
}
#endif
