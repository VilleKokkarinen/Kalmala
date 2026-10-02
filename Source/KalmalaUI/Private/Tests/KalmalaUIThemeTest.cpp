#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaUITheme.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaUIThemeTest, "Kalmala.UI.Theme.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaUIThemeTest::RunTest(const FString& Parameters)
{
    FConfigFile Config;
    Config.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nBodySize=20\nHeadingSize=12\n"
        "EmphasisSize=24\nPaddingX=16\nText=(R=0.4,G=0.5,B=0.6,A=1)\n"
        "HighContrastPanel=(R=1,G=1,B=1,A=0)\n"));
    const FKalmalaUITheme Theme = FKalmalaUITheme::FromConfig(Config);
    UTextBlock* Status = NewObject<UTextBlock>();
    UTextBlock* Weather = NewObject<UTextBlock>();
    UBorder* Panel = NewObject<UBorder>();
    Theme.ApplyText(*Status, Theme.BodySize, false, 125, 0);
    Theme.ApplyText(*Weather, Theme.EmphasisSize, false, 125, 0);
    TestEqual(TEXT("Status respects theme and local scale"), Status->GetFont().Size, 25.0f);
    TestEqual(TEXT("Weather respects the same theme and local scale"), Weather->GetFont().Size, 30.0f);
    TestTrue(TEXT("Shared text colour propagates"), Status->GetColorAndOpacity().GetSpecifiedColor().Equals(Theme.Text)
        && Weather->GetColorAndOpacity().GetSpecifiedColor().Equals(Theme.Text));
    Theme.ApplyText(*Status, Theme.BodySize, false, 150, 1);
    Theme.ApplyPanel(*Panel, 1);
    TestTrue(TEXT("Local contrast overrides decorative text"),
        Status->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
    TestTrue(TEXT("Contrast cannot be weakened by theme"),
        Panel->GetBrushColor().Equals(FLinearColor(0, 0, 0, 0.98f)));
    TestEqual(TEXT("Shared panel padding propagates"), Panel->GetPadding().Left, 16.0f);

    FConfigFile Invalid;
    Invalid.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nBodySize=999\nHeadingSize=garbage\n"
        "EmphasisSize=-2\nPaddingX=-1\nPaddingY=nan\nRowSpacing=999\n"
        "Text=(R=2,G=0,B=0,A=1)\nPanel=garbage\n"));
    const FKalmalaUITheme Fallback = FKalmalaUITheme::FromConfig(Invalid);
    const FKalmalaUITheme Defaults;
    TestEqual(TEXT("Out-of-range font falls back"), Fallback.BodySize, Defaults.BodySize);
    TestEqual(TEXT("Malformed font falls back"), Fallback.HeadingSize, Defaults.HeadingSize);
    TestEqual(TEXT("Negative font falls back"), Fallback.EmphasisSize, Defaults.EmphasisSize);
    TestEqual(TEXT("Negative padding falls back"), Fallback.PaddingX, Defaults.PaddingX);
    TestEqual(TEXT("Non-finite padding falls back"), Fallback.PaddingY, Defaults.PaddingY);
    TestEqual(TEXT("Excessive spacing falls back"), Fallback.RowSpacing, Defaults.RowSpacing);
    TestTrue(TEXT("Malformed and out-of-range colours fall back"),
        Fallback.Panel.Equals(Defaults.Panel) && Fallback.Text.Equals(Defaults.Text));
    TestEqual(TEXT("Missing file uses body defaults"), FKalmalaUITheme::FromConfig(FConfigFile()).BodySize, Defaults.BodySize);
    TestEqual(TEXT("Production theme loads from config hierarchy"), FKalmalaUITheme::Get().BodySize, 13);
    return true;
}
#endif
