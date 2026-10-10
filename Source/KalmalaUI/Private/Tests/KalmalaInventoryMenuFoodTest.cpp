#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryMenuPreparedFoodDetailsTest,
    "Kalmala.UI.Inventory.PreparedFoodDetails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryMenuPreparedFoodDetailsTest::RunTest(const FString& Parameters)
{
    auto* Menu = NewObject<UKalmalaInventoryMenuWidget>();
    TestNotNull(TEXT("Prepared-food details use the Inventory menu"), Menu);
    if (!Menu) return false;
    Menu->Initialize();
    Menu->SetInventoryRowsForVerification({
        {TEXT("HearthBroth"), TEXT("Hearth Broth"), TEXT("× 1"), false}
    }, 100, 0);
    Menu->SelectItemForVerification(TEXT("HearthBroth"));

    FString DetailText;
    TArray<UWidget*> Widgets;
    Menu->WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
    {
        if (auto* Detail = Cast<UKalmalaItemDetailWidget>(Widget))
        {
            TArray<UWidget*> DetailWidgets;
            Detail->WidgetTree->GetAllWidgets(DetailWidgets);
            for (UWidget* DetailWidget : DetailWidgets)
                if (const auto* Block = Cast<UTextBlock>(DetailWidget)) DetailText += Block->GetText().ToString();
        }
    }
    TestTrue(TEXT("Selected food detail explains the real Steady Meal benefit"),
        DetailText.Contains(TEXT("10% lower stamina use for 120 seconds")));

    FString MenuText;
    for (UWidget* Widget : Widgets)
        if (const auto* Block = Cast<UTextBlock>(Widget)) MenuText += Block->GetText().ToString();
    TestTrue(TEXT("The menu exposes the supported Eat action"), MenuText.Contains(TEXT("Eat one serving")));
    TestTrue(TEXT("Food action fails closed while owner data is unavailable"),
        MenuText.Contains(TEXT("Food use is unavailable while owner data is loading.")));
    return true;
}

#endif
