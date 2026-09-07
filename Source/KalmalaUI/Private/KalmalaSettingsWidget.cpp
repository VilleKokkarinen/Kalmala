#include "KalmalaSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr FLinearColor BackgroundColour(0.015f, 0.025f, 0.035f, 0.94f);
    constexpr FLinearColor PanelColour(0.055f, 0.08f, 0.10f, 0.98f);
}

void UKalmalaSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ContentBox != nullptr) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsCanvas"));
    WidgetTree->RootWidget = Canvas;
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackdrop"));
    Backdrop->SetBrushColor(BackgroundColour);
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackdropSlot->SetOffsets(FMargin(0.0f));

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsPanel"));
    Panel->SetBrushColor(PanelColour);
    Panel->SetPadding(FMargin(40.0f));
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PanelSlot->SetSize(FVector2D(620.0f, 580.0f));

    ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContent"));
    Panel->SetContent(ContentBox);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaSettingsWidget::Open(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr) return;
    SetOwningPlayer(InOwningPlayer);
    bMenuOpen = true;
    ShowMainMenu();
    SetVisibility(ESlateVisibility::Visible);
    InOwningPlayer->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    InOwningPlayer->SetInputMode(InputMode);
    InOwningPlayer->SetIgnoreMoveInput(true);
    InOwningPlayer->SetIgnoreLookInput(true);
}

void UKalmalaSettingsWidget::Close()
{
    if (!bMenuOpen) return;
    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->SetShowMouseCursor(false);
        Controller->SetInputMode(FInputModeGameOnly());
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
    }
}

int32 UKalmalaSettingsWidget::ClampViewDistanceQuality(const int32 Quality)
{
    return FMath::Clamp(Quality, 0, 3);
}

UTextBlock* UKalmalaSettingsWidget::AddLabel(UVerticalBox* Parent, const FText& Label, const float FontSize)
{
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.92f, 0.90f)));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = FontSize;
    Text->SetFont(Font);
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Text);
    BoxSlot->SetPadding(FMargin(4.0f, 8.0f));
    return Text;
}

UButton* UKalmalaSettingsWidget::AddButton(UVerticalBox* Parent, const FText& Label, const FName Name)
{
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetJustification(ETextJustify::Center);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = 21;
    Text->SetFont(Font);
    Button->SetContent(Text);
    UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Button);
    BoxSlot->SetPadding(FMargin(4.0f, 7.0f));
    BoxSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    return Button;
}

void UKalmalaSettingsWidget::ShowMainMenu()
{
    ContentBox->ClearChildren();
    AddLabel(ContentBox, FText::FromString(TEXT("KALMALA")), 34.0f)->SetJustification(ETextJustify::Center);
    AddLabel(ContentBox, FText::FromString(TEXT("Settings")), 23.0f)->SetJustification(ETextJustify::Center);
    UButton* Options = AddButton(ContentBox, FText::FromString(TEXT("Options")), TEXT("OptionsButton"));
    Options->OnClicked.AddDynamic(this, &ThisClass::HandleOptionsClicked);
    UButton* Quit = AddButton(ContentBox, FText::FromString(TEXT("Quit")), TEXT("QuitButton"));
    Quit->OnClicked.AddDynamic(this, &ThisClass::HandleQuitClicked);
    AddLabel(ContentBox, FText::FromString(TEXT("Press Esc to return to the game")), 16.0f)->SetJustification(ETextJustify::Center);
}

void UKalmalaSettingsWidget::ShowOptionsMenu()
{
    ContentBox->ClearChildren();
    AddLabel(ContentBox, FText::FromString(TEXT("Options")), 30.0f)->SetJustification(ETextJustify::Center);
    UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    const auto AddTab = [this, Tabs](const FText& Label, FName Name, FScriptDelegate Delegate)
    {
        UButton* Tab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Label); Text->SetJustification(ETextJustify::Center); Tab->SetContent(Text);
        Tab->OnClicked.Add(Delegate);
        UHorizontalBoxSlot* Slot = Tabs->AddChildToHorizontalBox(Tab); Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Slot->SetPadding(FMargin(3.0f));
    };
    FScriptDelegate VideoDelegate; VideoDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleVideoClicked)); AddTab(FText::FromString(TEXT("Video")), TEXT("VideoTab"), VideoDelegate);
    FScriptDelegate AudioDelegate; AudioDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleAudioClicked)); AddTab(FText::FromString(TEXT("Audio")), TEXT("AudioTab"), AudioDelegate);
    FScriptDelegate ControlsDelegate; ControlsDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleControlsClicked)); AddTab(FText::FromString(TEXT("Controls")), TEXT("ControlsTab"), ControlsDelegate);
    FScriptDelegate SettingsDelegate; SettingsDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, HandleSettingsClicked)); AddTab(FText::FromString(TEXT("Settings")), TEXT("SettingsTab"), SettingsDelegate);
    ContentBox->AddChildToVerticalBox(Tabs);
    ShowVideoTab();
}

void UKalmalaSettingsWidget::ShowVideoTab()
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, FText::FromString(TEXT("Video")), 24.0f);
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr) return;
    ResolutionChoices = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3840, 2160) };
    const FIntPoint CurrentResolution = Settings->GetScreenResolution();
    ResolutionChoiceIndex = ResolutionChoices.IndexOfByPredicate([CurrentResolution](const FIntPoint& Choice) { return Choice == CurrentResolution; });
    if (ResolutionChoiceIndex == INDEX_NONE) { ResolutionChoices.Insert(CurrentResolution, 0); ResolutionChoiceIndex = 0; }
    UButton* Resolution = AddButton(ContentBox, FText::GetEmpty(), TEXT("ResolutionButton")); Resolution->OnClicked.AddDynamic(this, &ThisClass::HandleResolutionClicked); ResolutionLabel = Cast<UTextBlock>(Resolution->GetContent());
    UButton* VSync = AddButton(ContentBox, FText::GetEmpty(), TEXT("VSyncButton")); VSync->OnClicked.AddDynamic(this, &ThisClass::HandleVSyncClicked); VSyncLabel = Cast<UTextBlock>(VSync->GetContent());
    UButton* WindowMode = AddButton(ContentBox, FText::GetEmpty(), TEXT("WindowModeButton")); WindowMode->OnClicked.AddDynamic(this, &ThisClass::HandleWindowModeClicked); WindowModeLabel = Cast<UTextBlock>(WindowMode->GetContent());
    UButton* ViewDistance = AddButton(ContentBox, FText::GetEmpty(), TEXT("ViewDistanceButton")); ViewDistance->OnClicked.AddDynamic(this, &ThisClass::HandleViewDistanceClicked); ViewDistanceLabel = Cast<UTextBlock>(ViewDistance->GetContent());
    AddLabel(ContentBox, FText::FromString(TEXT("Changes are applied and saved immediately.")), 15.0f)->SetJustification(ETextJustify::Center);
    UpdateVideoLabels();
}

void UKalmalaSettingsWidget::ShowPlaceholderTab(const FText& Title, const FText& Description)
{
    while (ContentBox->GetChildrenCount() > 2) ContentBox->RemoveChildAt(2);
    AddLabel(ContentBox, Title, 24.0f);
    AddLabel(ContentBox, Description, 18.0f);
}

void UKalmalaSettingsWidget::UpdateVideoLabels()
{
    UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
    if (Settings == nullptr) return;
    const FIntPoint Resolution = Settings->GetScreenResolution();
    if (ResolutionLabel) ResolutionLabel->SetText(FText::FromString(FString::Printf(TEXT("Resolution: %d x %d"), Resolution.X, Resolution.Y)));
    if (VSyncLabel) VSyncLabel->SetText(FText::FromString(FString::Printf(TEXT("V-Sync: %s"), Settings->IsVSyncEnabled() ? TEXT("On") : TEXT("Off"))));
    const TCHAR* Mode = Settings->GetFullscreenMode() == EWindowMode::Windowed ? TEXT("Windowed") : Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen ? TEXT("Borderless") : TEXT("Fullscreen");
    if (WindowModeLabel) WindowModeLabel->SetText(FText::FromString(FString::Printf(TEXT("Window Mode: %s"), Mode)));
    if (ViewDistanceLabel) ViewDistanceLabel->SetText(FText::FromString(FString::Printf(TEXT("Render Distance: %d / 3"), ClampViewDistanceQuality(Settings->ScalabilityQuality.ViewDistanceQuality))));
}

void UKalmalaSettingsWidget::ApplyVideoSettings() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ApplySettings(false); Settings->SaveSettings(); UpdateVideoLabels(); } }
void UKalmalaSettingsWidget::HandleOptionsClicked() { ShowOptionsMenu(); }
void UKalmalaSettingsWidget::HandleQuitClicked() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); }
void UKalmalaSettingsWidget::HandleVideoClicked() { ShowVideoTab(); }
void UKalmalaSettingsWidget::HandleAudioClicked() { ShowPlaceholderTab(FText::FromString(TEXT("Audio")), FText::FromString(TEXT("Audio controls will be available here."))); }
void UKalmalaSettingsWidget::HandleControlsClicked() { ShowPlaceholderTab(FText::FromString(TEXT("Controls")), FText::FromString(TEXT("Control remapping will be available here."))); }
void UKalmalaSettingsWidget::HandleSettingsClicked() { ShowPlaceholderTab(FText::FromString(TEXT("Settings")), FText::FromString(TEXT("Gameplay and accessibility settings will be available here."))); }
void UKalmalaSettingsWidget::HandleResolutionClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { ResolutionChoiceIndex = (ResolutionChoiceIndex + 1) % ResolutionChoices.Num(); Settings->SetScreenResolution(ResolutionChoices[ResolutionChoiceIndex]); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleVSyncClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled()); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleWindowModeClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { const EWindowMode::Type Mode = Settings->GetFullscreenMode(); Settings->SetFullscreenMode(Mode == EWindowMode::Fullscreen ? EWindowMode::Windowed : Mode == EWindowMode::Windowed ? EWindowMode::WindowedFullscreen : EWindowMode::Fullscreen); ApplyVideoSettings(); } }
void UKalmalaSettingsWidget::HandleViewDistanceClicked() { if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings()) { Settings->ScalabilityQuality.ViewDistanceQuality = (ClampViewDistanceQuality(Settings->ScalabilityQuality.ViewDistanceQuality) + 1) % 4; ApplyVideoSettings(); } }
