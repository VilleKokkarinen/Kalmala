#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaSelectedResultWidget.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSelectedResultWidgetTest, "Kalmala.UI.Crafting.ResultPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSelectedResultWidgetTest::RunTest(const FString& Parameters)
{
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    TestTrue(TEXT("The result preview is materially larger than recipe-card icons"),
        UKalmalaSelectedResultWidget::GetPreviewIconExtent() >= 64);

    for (const FKalmalaRecipe& Recipe : Recipes)
    {
        EKalmalaIcon Icon = EKalmalaIcon::Unknown;
        int32 Variant = INDEX_NONE;
        TestTrue(*FString::Printf(TEXT("%s output uses its canonical catalogue icon"), *Recipe.Output.ToString()),
            UKalmalaSelectedResultWidget::FindCanonicalIcon(Recipe.Output, Icon, Variant));
        TestTrue(*FString::Printf(TEXT("%s output never resolves to the unknown fallback"), *Recipe.Output.ToString()),
            Icon != EKalmalaIcon::Unknown);
    }

    EKalmalaIcon UnknownIcon = EKalmalaIcon::Unknown;
    int32 UnknownVariant = INDEX_NONE;
    TestFalse(TEXT("Uncatalogued result reports no canonical icon"),
        UKalmalaSelectedResultWidget::FindCanonicalIcon(TEXT("KalmalaMissingPreviewFixture"), UnknownIcon, UnknownVariant));
    TestTrue(TEXT("Uncatalogued result uses the explicit unknown icon fallback"), UnknownIcon == EKalmalaIcon::Unknown);
    return true;
}
#endif
