#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStatComparison.h"
#include "KalmalaUITheme.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStatComparisonTest,
    "Kalmala.UI.Crafting.StatComparisonPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaStatComparisonTest::RunTest(const FString& Parameters)
{
    const FKalmalaStatComparisonLine Crush = FKalmalaStatComparisonLine::FromValues(
        TEXT("crush"), 0, 60, true);
    TestEqual(TEXT("Illustrative crush line uses selected value and signed delta"),
        Crush.Text, FString(TEXT("crush 60 (+60)")));
    TestTrue(TEXT("Crush increase has positive semantic treatment"),
        Crush.Tone == EKalmalaStatComparisonTone::Improved);

    const FKalmalaStatComparisonLine Slash = FKalmalaStatComparisonLine::FromValues(
        TEXT("slash"), 40, 0, true);
    TestEqual(TEXT("Illustrative slash line uses selected value and signed delta"),
        Slash.Text, FString(TEXT("slash 0 (-40)")));
    TestTrue(TEXT("Slash decrease has negative semantic treatment"),
        Slash.Tone == EKalmalaStatComparisonTone::Worsened);

    const FKalmalaStatComparisonLine Equal = FKalmalaStatComparisonLine::FromValues(
        TEXT("condition"), 24, 24, true);
    TestEqual(TEXT("Equal values remain explicit and neutral"),
        Equal.Text, FString(TEXT("condition 24 (0)")));
    TestTrue(TEXT("Equal values use neutral semantic treatment"),
        Equal.Tone == EKalmalaStatComparisonTone::Neutral);

    const FKalmalaStatComparisonLine LowerIsBetter = FKalmalaStatComparisonLine::FromValues(
        TEXT("weight"), 5, 4, false);
    TestEqual(TEXT("Lower-is-better stats retain the numeric signed delta"),
        LowerIsBetter.Text, FString(TEXT("weight 4 (-1)")));
    TestTrue(TEXT("Lower value can be represented as an improvement"),
        LowerIsBetter.Tone == EKalmalaStatComparisonTone::Improved);

    const FKalmalaStatComparisonLine Missing = FKalmalaStatComparisonLine::Unavailable(
        TEXT("slash"), TEXT("items are incompatible"));
    TestEqual(TEXT("Missing comparisons say unavailable without inventing zero"),
        Missing.Text, FString(TEXT("slash comparison unavailable: items are incompatible.")));
    TestFalse(TEXT("Unavailable comparison contains no fabricated numeric value"),
        Missing.Text.Contains(TEXT(" 0 ")));

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    TestTrue(TEXT("Improvement color reuses the established positive theme cue"),
        Crush.ResolveColor(Theme, 0) == Theme.StatusCueStartedColor);
    TestTrue(TEXT("Worsening color reuses the established ended theme cue"),
        Slash.ResolveColor(Theme, 0) == Theme.StatusCueEndedColor);
    TestTrue(TEXT("Unavailable text uses the normal accessible text color"),
        Missing.ResolveColor(Theme, 1) == Theme.TextColor(false, 1));
    return true;
}
#endif
