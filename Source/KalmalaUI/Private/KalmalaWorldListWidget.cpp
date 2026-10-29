#include "KalmalaWorldListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaUITheme.h"

namespace
{
    const TCHAR* AvailabilityName(const EKalmalaWorldMetadataAvailability Availability)
    {
        switch (Availability)
        {
        case EKalmalaWorldMetadataAvailability::Available: return TEXT("available");
        case EKalmalaWorldMetadataAvailability::Unavailable: return TEXT("unavailable");
        default: return TEXT("unknown");
        }
    }
}

const TCHAR* UKalmalaWorldListWidget::EmptyOrUnavailableMessage(const EKalmalaWorldMetadataAvailability Availability)
{
    switch (Availability)
    {
    case EKalmalaWorldMetadataAvailability::Available: return TEXT("World listing data is not available in this view yet.");
    case EKalmalaWorldMetadataAvailability::Unavailable: return TEXT("World listing is not supported yet. Existing saves have not been changed.");
    default: return TEXT("World availability could not be determined.");
    }
}

void UKalmalaWorldListWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    Backdrop = WidgetTree->ConstructWidget<UBorder>();
    UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Backdrop);
    BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackgroundSlot->SetOffsets(FMargin(0));
    Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    UCanvasPanelSlot* ScrollSlot = Canvas->AddChildToCanvas(Scroll);
    ScrollSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    ScrollSlot->SetOffsets(FMargin(32));
    Column = WidgetTree->ConstructWidget<USizeBox>();
    Column->SetWidthOverride(420.0f);
    if (UScrollBoxSlot* Slot = Cast<UScrollBoxSlot>(Scroll->AddChild(Column))) Slot->SetHorizontalAlignment(HAlign_Left);
    Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->SetContent(Content);

    ListingAvailability = FKalmalaLocalWorldSaveListing::Enumerate(WorldRecords);
    BuildWorldList();
}

void UKalmalaWorldListWidget::BuildWorldList()
{
    if (!WidgetTree || !Content) return;
    const uint64 PreviouslySelectedSeed = SelectedSeed;
    bShowingDetails = false;
    bHasSelectedWorld = false;
    SelectedSeed = 0;
    Content->ClearChildren();
    WorldPicker = nullptr;
    DetailsText = nullptr;
    NewWorldButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;

    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("Your worlds")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    Message = WidgetTree->ConstructWidget<UTextBlock>();
    if (ListingAvailability == EKalmalaLocalWorldSaveListingAvailability::Unsupported)
    {
        Message->SetText(FText::FromString(TEXT("Local save listing is not supported on this platform. Existing saves have not been changed.")));
    }
    else if (WorldRecords.IsEmpty())
    {
        Message->SetText(FText::FromString(TEXT("No recognized world-scoped save data was found. A world with no saved world data cannot be listed from these slots.")));
    }
    else
    {
        Message->SetText(FText::FromString(FString::Printf(
            TEXT("Found %d seed-based entries from recognized local world save slots. Save completeness and compatibility are not established by slot names."),
            WorldRecords.Num())));
    }
    Message->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Message)->SetPadding(FMargin(0, 0, 0, 16));

    if (ListingAvailability == EKalmalaLocalWorldSaveListingAvailability::Supported && !WorldRecords.IsEmpty())
    {
        UTextBlock* SelectLabel = WidgetTree->ConstructWidget<UTextBlock>();
        SelectLabel->SetText(FText::FromString(TEXT("Choose a world seed")));
        SelectLabel->SetAutoWrapText(true);
        Content->AddChildToVerticalBox(SelectLabel)->SetPadding(FMargin(0, 0, 0, 6));

        WorldPicker = WidgetTree->ConstructWidget<UComboBoxString>();
        int32 SelectedIndex = 0;
        for (int32 Index = 0; Index < WorldRecords.Num(); ++Index)
        {
            const FKalmalaLocalWorldSaveRecord& Record = WorldRecords[Index];
            WorldPicker->AddOption(Record.GetDisplayLabel());
            if (Record.Seed == PreviouslySelectedSeed) SelectedIndex = Index;
        }
        SelectedIndex = FMath::Clamp(SelectedIndex, 0, WorldRecords.Num() - 1);
        WorldPicker->SetSelectedIndex(SelectedIndex);
        SelectedSeed = WorldRecords[SelectedIndex].Seed;
        bHasSelectedWorld = true;
        Content->AddChildToVerticalBox(WorldPicker)->SetPadding(FMargin(0, 0, 0, 10));

        DetailsButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        UTextBlock* DetailsLabel = WidgetTree->ConstructWidget<UTextBlock>();
        DetailsLabel->SetText(FText::FromString(TEXT("World details")));
        DetailsLabel->SetAutoWrapText(true);
        DetailsLabel->SetJustification(ETextJustify::Center);
        DetailsButton->SetContent(DetailsLabel);
        Content->AddChildToVerticalBox(DetailsButton)->SetPadding(FMargin(0, 0, 0, 8));
        DetailsButton->OnClicked.AddDynamic(this, &ThisClass::HandleViewDetails);
        WorldPicker->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleWorldSelectionChanged);
    }

    NewWorldButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* NewWorldLabel = WidgetTree->ConstructWidget<UTextBlock>();
    NewWorldLabel->SetText(FText::FromString(TEXT("New world")));
    NewWorldLabel->SetAutoWrapText(true);
    NewWorldLabel->SetJustification(ETextJustify::Center);
    NewWorldButton->SetContent(NewWorldLabel);
    NewWorldButton->SetIsEnabled(false);
    Content->AddChildToVerticalBox(NewWorldButton)->SetPadding(FMargin(0, 0, 0, 8));

    CreationStatus = WidgetTree->ConstructWidget<UTextBlock>();
    CreationStatus->SetText(FText::FromString(TEXT("World creation is not available yet.")));
    CreationStatus->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(CreationStatus)->SetPadding(FMargin(0, 0, 0, 16));

    BackButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
    BackLabel->SetText(FText::FromString(TEXT("Back")));
    BackLabel->SetAutoWrapText(true);
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->SetContent(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBack);
    Content->AddChildToVerticalBox(BackButton);
    RefreshTheme();
}

void UKalmalaWorldListWidget::BuildWorldDetails()
{
    if (!WidgetTree || !Content) return;
    const FKalmalaLocalWorldSaveRecord* Record = FindWorld(SelectedSeed);
    if (!Record) return;
    bShowingDetails = true;
    Content->ClearChildren();
    Message = nullptr;
    CreationStatus = nullptr;
    WorldPicker = nullptr;
    NewWorldButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;

    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("World details")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    DetailsText = WidgetTree->ConstructWidget<UTextBlock>();
    DetailsText->SetText(FText::FromString(DescribeWorld(*Record)));
    DetailsText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(DetailsText)->SetPadding(FMargin(0, 0, 0, 16));

    BackButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
    BackLabel->SetText(FText::FromString(TEXT("Back to worlds")));
    BackLabel->SetAutoWrapText(true);
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->SetContent(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBack);
    Content->AddChildToVerticalBox(BackButton);
    RefreshTheme();
}

void UKalmalaWorldListWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (Column) Column->SetWidthOverride(UKalmalaHomeWidget::ActionColumnWidth(Geometry.GetLocalSize().X));
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaWorldListWidget::RefreshTheme()
{
    const auto& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (Scroll) Theme.ApplyScroll(*Scroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (Message) Theme.ApplyText(*Message, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    if (CreationStatus) Theme.ApplyText(*CreationStatus, Theme.BodySize, false, LastTextScale, LastContrast);
    if (DetailsText) Theme.ApplyText(*DetailsText, Theme.BodySize + 2, false, LastTextScale, LastContrast);
    if (WorldPicker) Theme.ApplyDropdown(*WorldPicker, LastContrast);
    UKalmalaThemedButton* Buttons[] = {NewWorldButton.Get(), DetailsButton.Get(), BackButton.Get()};
    for (UKalmalaThemedButton* Button : Buttons)
    {
        if (!Button) continue;
        Theme.ApplyButton(*Button, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

const FKalmalaLocalWorldSaveRecord* UKalmalaWorldListWidget::FindWorld(const uint64 Seed) const
{
    return WorldRecords.FindByPredicate([Seed](const FKalmalaLocalWorldSaveRecord& Record)
    {
        return Record.Seed == Seed;
    });
}

FString UKalmalaWorldListWidget::DescribeWorld(const FKalmalaLocalWorldSaveRecord& Record) const
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = Record.Seed;
    const FKalmalaWorldDescriptor Descriptor = FKalmalaWorldDescriptorAdapter::Describe(Config);

    TArray<FString> Components;
    if (Record.bHasPopulation) Components.Add(TEXT("generated population"));
    if (Record.bHasConstruction) Components.Add(TEXT("construction"));
    if (Record.bHasStorage) Components.Add(TEXT("storage"));
    if (Record.bHasOceanTravel) Components.Add(TEXT("ocean travel"));
    FString ComponentSummary;
    for (const FString& Component : Components)
    {
        if (!ComponentSummary.IsEmpty()) ComponentSummary += TEXT(", ");
        ComponentSummary += Component;
    }
    if (ComponentSummary.IsEmpty()) ComponentSummary = TEXT("none recognized");

    FString StoredRevision = TEXT("not present in recognized slot names");
    if (Record.bHasConflictingGeneratorRevisions) StoredRevision = TEXT("conflicting values detected");
    else if (Record.bHasStoredGeneratorRevision)
        StoredRevision = FString::Printf(TEXT("%d (from the ocean-travel slot name only)"), Record.StoredGeneratorRevision);

    return FString::Printf(
        TEXT("%s\n\nWorld seed: %llu (%s)\nRecognized world-data slot names: %s\nSaved-world existence: %s; these component slots are not a complete world registry.\nSaved-world compatibility: %s; save contents have not been validated.\nStored generator revision: %s\nCurrent generator revision: %d (not evidence of the saved revision).\nWorld rules: unavailable in the existing menu/save metadata.\n\nThis screen is read-only and does not launch or rewrite a save."),
        *Record.GetDisplayLabel(), Record.Seed, AvailabilityName(Descriptor.SeedAvailability), *ComponentSummary,
        AvailabilityName(Descriptor.SavedWorldExistence), AvailabilityName(Descriptor.SavedWorldCompatibility),
        *StoredRevision, Descriptor.CurrentIdentityRevision);
}

void UKalmalaWorldListWidget::FocusWidget(UWidget* Widget)
{
    if (!Widget || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(Widget->TakeWidget());
    Controller->SetInputMode(InputMode);
    Widget->SetUserFocus(Controller);
    Widget->SetKeyboardFocus();
}

void UKalmalaWorldListWidget::FocusBack()
{
    FocusWidget(BackButton);
}

void UKalmalaWorldListWidget::FocusPrimary()
{
    if (bShowingDetails) FocusBack();
    else if (WorldPicker) FocusWidget(WorldPicker);
    else FocusBack();
}

bool UKalmalaWorldListWidget::ShowWorldDetails(const uint64 Seed)
{
    if (!FindWorld(Seed)) return false;
    SelectedSeed = Seed;
    bHasSelectedWorld = true;
    BuildWorldDetails();
    return bShowingDetails;
}

void UKalmalaWorldListWidget::ShowWorldList()
{
    BuildWorldList();
}

void UKalmalaWorldListWidget::HandleWorldSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectionType;
    const FKalmalaLocalWorldSaveRecord* Record = WorldRecords.FindByPredicate([&SelectedItem](const auto& Candidate)
    {
        return Candidate.GetDisplayLabel() == SelectedItem;
    });
    bHasSelectedWorld = Record != nullptr;
    if (Record) SelectedSeed = Record->Seed;
    if (DetailsButton) DetailsButton->SetIsEnabled(bHasSelectedWorld);
}

void UKalmalaWorldListWidget::HandleViewDetails()
{
    if (bHasSelectedWorld && FindWorld(SelectedSeed)) WorldSelected.Broadcast(SelectedSeed);
}

void UKalmalaWorldListWidget::HandleBack()
{
    BackRequested.Broadcast();
}

FReply UKalmalaWorldListWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        HandleBack();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}
