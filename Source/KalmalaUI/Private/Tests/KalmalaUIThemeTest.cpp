#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaUITheme.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "UObject/Package.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaUIThemeTest, "Kalmala.UI.Theme.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaUIThemeTest::RunTest(const FString& Parameters)
{
    FConfigFile Config;
    Config.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nBodySize=20\nHeadingSize=12\n"
        "EmphasisSize=24\nPaddingX=16\nText=(R=0.4,G=0.5,B=0.6,A=1)\n"
        "HighContrastPanel=(R=1,G=1,B=1,A=0)\n"), TEXT("ThemeFixture.ini"));
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
        Panel->Background.TintColor.GetSpecifiedColor().Equals(FLinearColor(0, 0, 0, 0.98f)));
    TestEqual(TEXT("Shared panel padding propagates"), Panel->GetPadding().Left, 16.0f);

    FConfigFile Invalid;
    Invalid.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nBodySize=999\nHeadingSize=garbage\n"
        "EmphasisSize=-2\nPaddingX=-1\nPaddingY=nan\nRowSpacing=999\n"
        "Text=(R=2,G=0,B=0,A=1)\nPanel=garbage\n"), TEXT("InvalidThemeFixture.ini"));
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
    FConfigFile Extended;
    Extended.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nIconWidth=80\nIconHeight=60\nSlotPadding=8\n"
        "OutlineSize=2\nBorderWidth=2\nFontFace=Bold\nAnimateScrolling=True\nScrollSpeed=24\n"
        "ButtonHovered=(R=0.3,G=0.4,B=0.5,A=1)\n"), TEXT("ExtendedTheme.ini"));
    const FKalmalaUITheme Extension = FKalmalaUITheme::FromConfig(Extended);
    UButton* Button = NewObject<UButton>();
    USizeBox* Icon = NewObject<USizeBox>();
    UScrollBox* Scroll = NewObject<UScrollBox>();
    Extension.ApplyButton(*Button, 0);
    Extension.ApplyIconSlot(*Icon);
    Extension.ApplyScroll(*Scroll);
    Extension.ApplyText(*Status, Extension.BodySize, false, 100, 0);
    TestEqual(TEXT("Font face propagates"), Status->GetFont().TypefaceFontName, FName(TEXT("Bold")));
    TestEqual(TEXT("Text outline propagates"), Status->GetFont().OutlineSettings.OutlineSize, 2);
    TestEqual(TEXT("Button slot padding propagates"), Button->GetStyle().NormalPadding.Left, 8.0f);
    TestTrue(TEXT("Hover state propagates"), Button->GetStyle().Hovered.TintColor.GetSpecifiedColor().Equals(Extension.ButtonHovered));
    TestEqual(TEXT("Icon width propagates"), Icon->GetWidthOverride(), 80.0f);
    TestEqual(TEXT("Icon height propagates"), Icon->GetHeightOverride(), 60.0f);
    TestTrue(TEXT("Theme enables wheel animation"), Scroll->IsAnimateWheelScrolling());
    TestEqual(TEXT("Animation speed propagates"), Scroll->GetScrollAnimationInterpolationSpeed(), 24.0f);
    Extension.ApplyScroll(*Scroll, true);
    TestFalse(TEXT("Reduced motion overrides animation"), Scroll->IsAnimateWheelScrolling());
    Extension.ApplyButton(*Button, 1);
    TestTrue(TEXT("High contrast outlines buttons"), Button->GetStyle().Normal.OutlineSettings.Color.GetSpecifiedColor().Equals(FLinearColor::White));
    FConfigFile InvalidExtension;
    InvalidExtension.ProcessInputFileContents(TEXT("[Kalmala.UI.Theme]\nIconWidth=999\nIconHeight=nan\n"
        "BorderWidth=-1\nOutlineSize=99\nScrollSpeed=0\nFontFace=unknown\nFontAsset=../font\nPanelImage=C:/image.png\n"), TEXT("InvalidExtension.ini"));
    const FKalmalaUITheme Safe = FKalmalaUITheme::FromConfig(InvalidExtension);
    TestEqual(TEXT("Invalid icon width falls back"), Safe.IconWidth, Defaults.IconWidth);
    TestEqual(TEXT("Nonfinite icon height falls back"), Safe.IconHeight, Defaults.IconHeight);
    TestEqual(TEXT("Invalid border falls back"), Safe.BorderWidth, Defaults.BorderWidth);
    TestEqual(TEXT("Invalid outline falls back"), Safe.OutlineSize, Defaults.OutlineSize);
    TestEqual(TEXT("Invalid animation falls back"), Safe.ScrollSpeed, Defaults.ScrollSpeed);
    TestEqual(TEXT("Invalid font face falls back"), Safe.FontFace, Defaults.FontFace);
    TestTrue(TEXT("Filesystem asset paths rejected"), Safe.FontAsset.IsEmpty() && Safe.PanelImage.IsEmpty());
    FKalmalaUITheme MissingAssets = Extension;
    MissingAssets.FontAsset = TEXT("/Game/MissingThemeFont.MissingThemeFont");
    MissingAssets.PanelImage = TEXT("/Game/MissingThemeImage.MissingThemeImage");
    MissingAssets.ApplyText(*Status, 13, false, 100, 0);
    MissingAssets.ApplyPanel(*Panel, 0);
    TestTrue(TEXT("Missing image keeps geometric panel fallback"), Panel->Background.GetResourceObject() == nullptr);
    TestEqual(TEXT("Missing font retains readable size"), Status->GetFont().Size, 13.0f);
    TestTrue(TEXT("High contrast button retains border"), Button->GetStyle().Normal.OutlineSettings.Width >= 1);
    UPackage* Package = CreatePackage(TEXT("/Game/KalmalaThemeFixture"));
    UTexture2D* Texture = NewObject<UTexture2D>(Package, TEXT("Panel"), RF_Transient);
    MissingAssets.PanelImage = Texture->GetPathName();
    MissingAssets.ApplyPanel(*Panel, 0);
    TestTrue(TEXT("Valid image reference reaches the shared panel"), Panel->Background.GetResourceObject() == Texture);
    MissingAssets.ApplyPanel(*Panel, 1);
    TestTrue(TEXT("Contrast ignores decorative images"), Panel->Background.GetResourceObject() == nullptr);
    MissingAssets.BorderWidth = 0;
    MissingAssets.ApplyButton(*Button, 1);
    TestEqual(TEXT("Contrast keeps border with zero decorative width"), Button->GetStyle().Normal.OutlineSettings.Width, 1.0f);
    TestFalse(TEXT("Contrast retains distinct hover fill"), Button->GetStyle().Normal.TintColor.GetSpecifiedColor()
        .Equals(Button->GetStyle().Hovered.TintColor.GetSpecifiedColor()));
    UFont* OfflineFont = NewObject<UFont>(Package, TEXT("OfflineFont"), RF_Transient);
    OfflineFont->FontCacheType = EFontCacheType::Offline;
    MissingAssets.FontAsset = OfflineFont->GetPathName();
    MissingAssets.ApplyText(*Status, 13, false, 125, 0);
    TestTrue(TEXT("Unsupported bitmap font falls back to composite text"), Status->GetFont().FontObject == nullptr);
    TestEqual(TEXT("Font fallback preserves accessibility scale"), Status->GetFont().Size, 16.0f);
    UWidgetTree* Menu = NewObject<UWidgetTree>();
    UVerticalBox* MenuContent = Menu->ConstructWidget<UVerticalBox>();
    Menu->RootWidget = MenuContent;
    UTextBlock* Title = Menu->ConstructWidget<UTextBlock>();
    UTextBlock* Detail = Menu->ConstructWidget<UTextBlock>();
    UButton* Action = Menu->ConstructWidget<UButton>();
    UTextBlock* ActionLabel = Menu->ConstructWidget<UTextBlock>();
    MenuContent->AddChild(Title);
    MenuContent->AddChild(Detail);
    MenuContent->AddChild(Action);
    Action->SetContent(ActionLabel);
    Detail->SetText(FText::FromString(TEXT("Server-owned cost: 5 Stone")));
    Action->SetIsEnabled(false);
    Extension.ApplyMenu(*Menu, Title, 150, 1);
    TestEqual(TEXT("Menu heading uses semantic theme and accessibility size"), Title->GetFont().Size, 42.0f);
    TestEqual(TEXT("Menu detail uses theme and accessibility size"), Detail->GetFont().Size, 27.0f);
    TestTrue(TEXT("Menu action label respects high contrast"), ActionLabel->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor::White));
    TestFalse(TEXT("Styling preserves unavailable action"), Action->GetIsEnabled());
    TestEqual(TEXT("Styling preserves authoritative detail text"), Detail->GetText().ToString(), FString(TEXT("Server-owned cost: 5 Stone")));
    TestEqual(TEXT("Slate map and UMG use the same font"), Extension.MakeFont(Extension.BodySize + 5, false, 150).Size, Detail->GetFont().Size);
    TestTrue(TEXT("Slate map and UMG use the same contrast surface"), Extension.MakePanelBrush(1).TintColor.GetSpecifiedColor().Equals(Panel->Background.TintColor.GetSpecifiedColor()));
    return true;
}
#endif
