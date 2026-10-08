#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaIconWidget.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaItemDetailTest, "Kalmala.UI.Inventory.ItemDetail",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaItemDetailTest::RunTest(const FString& Parameters)
{
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    for (const auto& Item : Catalogue->Items)
    {
        TestEqual(TEXT("Uses canonical description and supplied owner state"),
            UKalmalaItemDetailWidget::DescribeItem(Item.ItemId, TEXT("Condition 17/40")),
            Item.Description + TEXT("\n\nCondition 17/40"));
        TestEqual(TEXT("Absent state does not fabricate statistics or actions"),
            UKalmalaItemDetailWidget::DescribeItem(Item.ItemId, TEXT("")), Item.Description);
    }
    TestEqual(TEXT("Unknown identity has explicit fallback"),
        UKalmalaItemDetailWidget::DescribeItem(TEXT("UnknownItem"), TEXT("")), FString(TEXT("Description unavailable.")));
    auto* Panel = NewObject<UKalmalaItemDetailWidget>();
    Panel->Initialize();
    Panel->SetItem(TEXT("Wood"), TEXT("Wood"), TEXT("x 7"), 150, 1);
    TestNotNull(TEXT("Shared panel creates its widget tree"), Panel->GetRootWidget());
    TArray<UWidget*> Widgets;
    Panel->WidgetTree->GetAllWidgets(Widgets);
    bool bFoundDetails = false;
    bool bFoundLoadedItemImage = false;
    for (auto* Widget : Widgets)
    {
        if (const auto* Text = Cast<UTextBlock>(Widget))
            bFoundDetails |= Text->GetText().ToString() == UKalmalaItemDetailWidget::DescribeItem(TEXT("Wood"), TEXT("x 7"));
        if (const auto* Icon = Cast<UKalmalaIconWidget>(Widget))
            bFoundLoadedItemImage |= Icon->HasCatalogueTexture();
    }
    TestTrue(TEXT("High-contrast scaled panel binds current owner-visible data"), bFoundDetails);
    TestTrue(TEXT("Selected item detail shows its imported catalogue image"), bFoundLoadedItemImage);
    return true;
}
#endif
