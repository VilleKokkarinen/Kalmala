#include "KalmalaWorldListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
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

    const TCHAR* SaveComponentStatusName(const EKalmalaWorldSaveComponentStatus Status)
    {
        switch (Status)
        {
        case EKalmalaWorldSaveComponentStatus::Accepted: return TEXT("accepted");
        case EKalmalaWorldSaveComponentStatus::Rejected: return TEXT("rejected");
        default: return TEXT("absent");
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

FString UKalmalaWorldListWidget::ReuseConfirmationMessage(const uint64 Seed, const int32 GeneratorRevision)
{
    return FString::Printf(
        TEXT("A recognized ocean-travel slot name lists world seed %llu at generator revision %d. "
             "The save contents have not been validated. This opens details only and will not change saves."),
        Seed, GeneratorRevision);
}

const TCHAR* UKalmalaWorldListWidget::SeedCheckResultMessage(
    const EKalmalaWorldSeedParseResult ParseResult,
    const EKalmalaLocalWorldSaveListingAvailability Availability,
    const EKalmalaWorldSeedReuseDecision Decision)
{
    switch (ParseResult)
    {
    case EKalmalaWorldSeedParseResult::Empty: return TEXT("Enter a decimal world seed.");
    case EKalmalaWorldSeedParseResult::InvalidCharacter:
        return TEXT("Use decimal digits only. Signs, spaces, and extra text are not accepted.");
    case EKalmalaWorldSeedParseResult::OutOfRange:
        return TEXT("That seed is outside the supported uint64 range.");
    case EKalmalaWorldSeedParseResult::Success: break;
    }

    if (Availability == EKalmalaLocalWorldSaveListingAvailability::Unsupported)
        return TEXT("Local world-save listing is unsupported, so this seed cannot be checked safely.");

    switch (Decision)
    {
    case EKalmalaWorldSeedReuseDecision::NoKnownRecord:
        return TEXT("No recognized save slot lists this seed. New world creation is unavailable; nothing was created.");
    case EKalmalaWorldSeedReuseDecision::ConfirmExisting:
        return TEXT("A matching current-revision record is listed, but its confirmation could not be opened. No save was changed.");
    case EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview:
        return TEXT("Existing save names for this seed do not establish one current generator revision. It cannot be reused automatically.");
    }
    return TEXT("The world seed could not be checked safely.");
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

    RefreshWorldRecords();
    BuildWorldList();
}

void UKalmalaWorldListWidget::RefreshWorldRecords()
{
    ListingAvailability = FKalmalaLocalWorldSaveListing::Enumerate(WorldRecords);
}

void UKalmalaWorldListWidget::BuildWorldList()
{
    if (!WidgetTree || !Content) return;
    const uint64 PreviouslySelectedSeed = SelectedSeed;
    bShowingDetails = false;
    bShowingReuseConfirmation = false;
    PendingReuseSeed = 0;
    bHasSelectedWorld = false;
    SelectedSeed = 0;
    Content->ClearChildren();
    WorldPicker = nullptr;
    SeedInput = nullptr;
    SeedInputLabel = nullptr;
    DetailsText = nullptr;
    NewWorldButton = nullptr;
    SeedCheckButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;
    ReuseConfirmButton = nullptr;
    ReuseCancelButton = nullptr;
    bShowingNewWorldSeedEntry = false;

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
    NewWorldButton->SetIsEnabled(ListingAvailability == EKalmalaLocalWorldSaveListingAvailability::Supported);
    Content->AddChildToVerticalBox(NewWorldButton)->SetPadding(FMargin(0, 0, 0, 8));
    NewWorldButton->OnClicked.AddDynamic(this, &ThisClass::HandleNewWorldRequested);

    CreationStatus = WidgetTree->ConstructWidget<UTextBlock>();
    const TCHAR* CreationMessage = ListingAvailability == EKalmalaLocalWorldSaveListingAvailability::Supported
        ? TEXT("World creation is not available yet. New world lets you check for a listed seed and revision to reuse.")
        : TEXT("World creation is unavailable, and local saves cannot be listed to check a seed safely.");
    CreationStatus->SetText(FText::FromString(CreationMessage));
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

void UKalmalaWorldListWidget::BuildNewWorldSeedEntry()
{
    if (!WidgetTree || !Content || !bShowingNewWorldSeedEntry) return;

    bShowingDetails = false;
    bShowingReuseConfirmation = false;
    PendingReuseSeed = 0;
    Content->ClearChildren();
    Message = nullptr;
    SeedInputLabel = nullptr;
    CreationStatus = nullptr;
    DetailsText = nullptr;
    WorldPicker = nullptr;
    SeedInput = nullptr;
    NewWorldButton = nullptr;
    SeedCheckButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;
    ReuseConfirmButton = nullptr;
    ReuseCancelButton = nullptr;

    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("New world")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    Message = WidgetTree->ConstructWidget<UTextBlock>();
    Message->SetText(FText::FromString(TEXT(
        "Enter a decimal seed to check for a listed current generator revision. Matching slot names do not prove a complete world. "
        "Fresh-world creation is unavailable; this screen will not create or change saves.")));
    Message->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Message)->SetPadding(FMargin(0, 0, 0, 16));

    SeedInputLabel = WidgetTree->ConstructWidget<UTextBlock>();
    SeedInputLabel->SetText(FText::FromString(TEXT("World seed")));
    SeedInputLabel->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(SeedInputLabel)->SetPadding(FMargin(0, 0, 0, 6));

    SeedInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    SeedInput->SetHintText(FText::FromString(TEXT("Decimal digits")));
    FEditableTextBoxStyle InputStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    InputStyle.SetFont(FKalmalaUITheme::Get().MakeFont(
        FKalmalaUITheme::Get().BodySize + 4, false, UKalmalaSettingsWidget::GetTextScalePercent()));
    InputStyle.SetForegroundColor(FSlateColor(FKalmalaUITheme::Get().TextColor(
        false, UKalmalaSettingsWidget::GetContrastMode())));
    InputStyle.SetBackgroundColor(FSlateColor(FLinearColor(.02f, .025f, .03f, 1.f)));
    SeedInput->SetWidgetStyle(InputStyle);
    SeedInput->OnTextCommitted.AddDynamic(this, &ThisClass::HandleSeedTextCommitted);
    Content->AddChildToVerticalBox(SeedInput)->SetPadding(FMargin(0, 0, 0, 10));

    CreationStatus = WidgetTree->ConstructWidget<UTextBlock>();
    CreationStatus->SetText(FText::FromString(TEXT("Only an exact listed match can be opened after confirmation.")));
    CreationStatus->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(CreationStatus)->SetPadding(FMargin(0, 0, 0, 12));

    SeedCheckButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* CheckLabel = WidgetTree->ConstructWidget<UTextBlock>();
    CheckLabel->SetText(FText::FromString(TEXT("Check existing seed")));
    CheckLabel->SetAutoWrapText(true);
    CheckLabel->SetJustification(ETextJustify::Center);
    SeedCheckButton->SetContent(CheckLabel);
    SeedCheckButton->OnClicked.AddDynamic(this, &ThisClass::HandleSeedCheck);
    Content->AddChildToVerticalBox(SeedCheckButton)->SetPadding(FMargin(0, 0, 0, 8));

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

void UKalmalaWorldListWidget::BuildWorldDetails()
{
    if (!WidgetTree || !Content) return;
    const FKalmalaLocalWorldSaveRecord* Record = FindWorld(SelectedSeed);
    if (!Record) return;
    bShowingDetails = true;
    bShowingNewWorldSeedEntry = false;
    Content->ClearChildren();
    Message = nullptr;
    CreationStatus = nullptr;
    WorldPicker = nullptr;
    SeedInput = nullptr;
    SeedInputLabel = nullptr;
    NewWorldButton = nullptr;
    SeedCheckButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;
    ReuseConfirmButton = nullptr;
    ReuseCancelButton = nullptr;

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

void UKalmalaWorldListWidget::BuildWorldReuseConfirmation()
{
    if (!WidgetTree || !Content || !bShowingReuseConfirmation) return;

    Content->ClearChildren();
    Message = nullptr;
    CreationStatus = nullptr;
    DetailsText = nullptr;
    WorldPicker = nullptr;
    SeedInput = nullptr;
    SeedInputLabel = nullptr;
    NewWorldButton = nullptr;
    SeedCheckButton = nullptr;
    DetailsButton = nullptr;
    BackButton = nullptr;
    ReuseConfirmButton = nullptr;
    ReuseCancelButton = nullptr;

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = PendingReuseSeed;
    const FKalmalaWorldDescriptor Descriptor = FKalmalaWorldDescriptorAdapter::Describe(Config);

    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("Use existing world?")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    Message = WidgetTree->ConstructWidget<UTextBlock>();
    Message->SetText(FText::FromString(ReuseConfirmationMessage(
        PendingReuseSeed, Descriptor.CurrentIdentityRevision)));
    Message->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Message)->SetPadding(FMargin(0, 0, 0, 16));

    ReuseConfirmButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* ConfirmLabel = WidgetTree->ConstructWidget<UTextBlock>();
    ConfirmLabel->SetText(FText::FromString(TEXT("Open existing world details")));
    ConfirmLabel->SetAutoWrapText(true);
    ConfirmLabel->SetJustification(ETextJustify::Center);
    ReuseConfirmButton->SetContent(ConfirmLabel);
    ReuseConfirmButton->OnClicked.AddDynamic(this, &ThisClass::HandleReuseConfirmed);
    Content->AddChildToVerticalBox(ReuseConfirmButton)->SetPadding(FMargin(0, 0, 0, 8));

    ReuseCancelButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* CancelLabel = WidgetTree->ConstructWidget<UTextBlock>();
    CancelLabel->SetText(FText::FromString(TEXT("Cancel")));
    CancelLabel->SetAutoWrapText(true);
    CancelLabel->SetJustification(ETextJustify::Center);
    ReuseCancelButton->SetContent(CancelLabel);
    ReuseCancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleReuseCancelled);
    Content->AddChildToVerticalBox(ReuseCancelButton);
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
    if (SeedInputLabel) Theme.ApplyText(*SeedInputLabel, Theme.BodySize + 2, false, LastTextScale, LastContrast);
    if (WorldPicker) Theme.ApplyDropdown(*WorldPicker, LastContrast);
    if (SeedInput)
    {
        FEditableTextBoxStyle InputStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
        InputStyle.SetFont(Theme.MakeFont(Theme.BodySize + 4, false, LastTextScale));
        InputStyle.SetForegroundColor(FSlateColor(Theme.TextColor(false, LastContrast)));
        InputStyle.SetBackgroundColor(FSlateColor(LastContrast > 0
            ? FLinearColor::Black : FLinearColor(.02f, .025f, .03f, 1.f)));
        SeedInput->SetWidgetStyle(InputStyle);
    }
    UKalmalaThemedButton* Buttons[] =
    {
        NewWorldButton.Get(), SeedCheckButton.Get(), DetailsButton.Get(), BackButton.Get(),
        ReuseConfirmButton.Get(), ReuseCancelButton.Get()
    };
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
    const FKalmalaWorldSaveInspection SaveChecks = FKalmalaWorldDescriptorAdapter::InspectKnownSeed(Record.Seed);

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
        TEXT("%s\n\nWorld seed: %llu (%s)\nRecognized world-data slot names: %s\nSaved-world existence: %s; these component slots are not a complete world registry.\nSaved-world compatibility: %s; individual component checks do not prove complete-world compatibility.\nComponent payload checks: population %s, construction %s, storage %s, ocean travel %s (current generator revision slot only).\nStored generator revision: %s\nCurrent generator revision: %d (not evidence of the saved revision).\nWorld rules: unavailable in the existing menu/save metadata.\n\nThis screen is read-only. Component checks load known world-scoped saves but do not launch or rewrite them."),
        *Record.GetDisplayLabel(), Record.Seed, AvailabilityName(Descriptor.SeedAvailability), *ComponentSummary,
        AvailabilityName(Descriptor.SavedWorldExistence), AvailabilityName(Descriptor.SavedWorldCompatibility),
        SaveComponentStatusName(SaveChecks.Population), SaveComponentStatusName(SaveChecks.Construction),
        SaveComponentStatusName(SaveChecks.Storage), SaveComponentStatusName(SaveChecks.Ocean),
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
    if (bShowingReuseConfirmation) FocusWidget(ReuseCancelButton);
    else if (bShowingNewWorldSeedEntry) FocusWidget(SeedInput);
    else if (bShowingDetails) FocusBack();
    else if (WorldPicker) FocusWidget(WorldPicker);
    else FocusBack();
}

bool UKalmalaWorldListWidget::ShowWorldDetails(const uint64 Seed)
{
    if (!FindWorld(Seed)) return false;
    bShowingReuseConfirmation = false;
    bShowingNewWorldSeedEntry = false;
    PendingReuseSeed = 0;
    SelectedSeed = Seed;
    bHasSelectedWorld = true;
    BuildWorldDetails();
    return bShowingDetails;
}

bool UKalmalaWorldListWidget::HasExactCurrentReuseCandidate(const uint64 Seed) const
{
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = Seed;
    const FKalmalaWorldDescriptor Descriptor = FKalmalaWorldDescriptorAdapter::Describe(Config);
    return Descriptor.IdentityRevisionAvailability == EKalmalaWorldMetadataAvailability::Available
        && FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            ListingAvailability, WorldRecords, Seed, Descriptor.CurrentIdentityRevision)
            == EKalmalaWorldSeedReuseDecision::ConfirmExisting;
}

bool UKalmalaWorldListWidget::BeginWorldReuseConfirmation(const uint64 Seed)
{
    if (!WidgetTree || !Content || bShowingDetails || bShowingReuseConfirmation) return false;
    RefreshWorldRecords();
    if (!HasExactCurrentReuseCandidate(Seed)) return false;

    PendingReuseSeed = Seed;
    bShowingNewWorldSeedEntry = false;
    bShowingReuseConfirmation = true;
    BuildWorldReuseConfirmation();
    FocusPrimary();
    return true;
}

void UKalmalaWorldListWidget::HandleReuseConfirmed()
{
    if (!bShowingReuseConfirmation) return;
    RefreshWorldRecords();
    if (!HasExactCurrentReuseCandidate(PendingReuseSeed))
    {
        HandleReuseCancelled();
        return;
    }

    const uint64 Seed = PendingReuseSeed;
    if (WorldSelected.IsBound()) WorldSelected.Broadcast(Seed);
    else ShowWorldDetails(Seed);
}

void UKalmalaWorldListWidget::HandleReuseCancelled()
{
    if (!bShowingReuseConfirmation) return;
    bShowingReuseConfirmation = false;
    PendingReuseSeed = 0;
    if (ReturnToWorldsRequested.IsBound()) ReturnToWorldsRequested.Broadcast();
    else
    {
        ShowWorldList();
        FocusPrimary();
    }
}

void UKalmalaWorldListWidget::ShowWorldList()
{
    RefreshWorldRecords();
    BuildWorldList();
}

bool UKalmalaWorldListWidget::ShowNewWorldSeedEntry()
{
    if (!WidgetTree || !Content || ListingAvailability != EKalmalaLocalWorldSaveListingAvailability::Supported)
        return false;
    bShowingNewWorldSeedEntry = true;
    bShowingReuseConfirmation = false;
    PendingReuseSeed = 0;
    BuildNewWorldSeedEntry();
    return SeedInput != nullptr;
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

void UKalmalaWorldListWidget::HandleNewWorldRequested()
{
    if (bShowingDetails || bShowingNewWorldSeedEntry || bShowingReuseConfirmation) return;
    NewWorldRequested.Broadcast();
}

void UKalmalaWorldListWidget::HandleSeedCheck()
{
    if (!bShowingNewWorldSeedEntry || !SeedInput || !CreationStatus) return;
    const auto ShowStatus = [this](const TCHAR* Text)
    {
        CreationStatus->SetText(FText::FromString(Text));
        FocusWidget(SeedInput);
    };

    uint64 Seed = 0;
    const EKalmalaWorldSeedParseResult ParseResult = FKalmalaWorldSeedParser::Parse(
        SeedInput->GetText().ToString(), Seed);
    if (ParseResult != EKalmalaWorldSeedParseResult::Success)
    {
        ShowStatus(SeedCheckResultMessage(
            ParseResult, ListingAvailability, EKalmalaWorldSeedReuseDecision::NoKnownRecord));
        return;
    }

    RefreshWorldRecords();
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = Seed;
    const FKalmalaWorldDescriptor Descriptor = FKalmalaWorldDescriptorAdapter::Describe(Config);
    if (Descriptor.IdentityRevisionAvailability != EKalmalaWorldMetadataAvailability::Available)
    {
        ShowStatus(TEXT("The current generator revision is unavailable, so this seed cannot be checked safely."));
        return;
    }

    EKalmalaWorldSeedReuseDecision Decision = FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
        ListingAvailability, WorldRecords, Seed, Descriptor.CurrentIdentityRevision);
    if (Decision == EKalmalaWorldSeedReuseDecision::ConfirmExisting)
    {
        if (BeginWorldReuseConfirmation(Seed)) return;
        Decision = FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            ListingAvailability, WorldRecords, Seed, Descriptor.CurrentIdentityRevision);
    }

    ShowStatus(SeedCheckResultMessage(ParseResult, ListingAvailability, Decision));
}

void UKalmalaWorldListWidget::HandleSeedTextCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
    (void)Text;
    if (CommitMethod == ETextCommit::OnEnter) HandleSeedCheck();
}

void UKalmalaWorldListWidget::HandleBack()
{
    if (bShowingReuseConfirmation)
    {
        HandleReuseCancelled();
        return;
    }
    if (bShowingNewWorldSeedEntry)
    {
        if (ReturnToWorldsRequested.IsBound()) ReturnToWorldsRequested.Broadcast();
        else
        {
            ShowWorldList();
            FocusPrimary();
        }
        return;
    }
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
