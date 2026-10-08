#include "KalmalaStationContextWidget.h"

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetTree.h"

void UKalmalaStationContextWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    if (!WidgetTree) return;

    Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetPadding(FMargin(18.0f));
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
    StationTitle = WidgetTree->ConstructWidget<UTextBlock>();
    SectionTitle = WidgetTree->ConstructWidget<UTextBlock>();
    Header->AddChildToHorizontalBox(StationTitle)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Header->AddChildToHorizontalBox(SectionTitle)->SetPadding(FMargin(12.0f, 0.0f));
    CloseButton = WidgetTree->ConstructWidget<UButton>();
    auto* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>();
    CloseLabel->SetText(FText::FromString(TEXT("Close")));
    CloseButton->SetContent(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &ThisClass::CloseClicked);
    Header->AddChildToHorizontalBox(CloseButton);
    Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

    ContentBox = WidgetTree->ConstructWidget<USizeBox>();
    Column->AddChildToVerticalBox(ContentBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Background->SetContent(Column);
    WidgetTree->RootWidget = Background;
    ApplyTheme();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaStationContextWidget::ApplyTheme()
{
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    LastTextScalePercent = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrastMode = UKalmalaSettingsWidget::GetContrastMode();
    Theme.ApplyMenu(*WidgetTree, StationTitle, LastTextScalePercent, LastContrastMode);
    if (Background) Theme.ApplyPanel(*Background, LastContrastMode, &Theme.BuildPanelImage);
    if (CloseButton)
        Theme.ApplyButton(*CloseButton, LastContrastMode, UKalmalaSettingsWidget::IsReducedMotionEnabled());
}

void UKalmalaStationContextWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (bOpen && ServiceContent && SectionTitle)
    {
        const FString CurrentSection = ServiceContent->GetStationContextSection();
        if (SectionTitle->GetText().ToString() != CurrentSection)
            SectionTitle->SetText(FText::FromString(CurrentSection));
    }
    if (LastTextScalePercent != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrastMode != UKalmalaSettingsWidget::GetContrastMode()) ApplyTheme();
}

bool UKalmalaStationContextWidget::OpenForStation(AKalmalaConstructionActor* Station,
    const FString& Section, UKalmalaCraftingWidget* InServiceContent)
{
    if (bOpen) Close();
    APlayerController* PC = GetOwningPlayer();
    AKalmalaCharacter* Character = PC ? Cast<AKalmalaCharacter>(PC->GetPawn()) : nullptr;
    if (!PC || !PC->IsLocalController() || !Character || !IsValid(Station) || !InServiceContent
        || Section.TrimStartAndEnd().IsEmpty() || PC->IsMoveInputIgnored() || PC->IsLookInputIgnored()
        || !ContentBox) return false;
    if (!InServiceContent->OpenInStationContext(Station, Section) || !InServiceContent->IsStationContextValid())
    {
        InServiceContent->Close();
        return false;
    }

    ContextStation = Station;
    ContextOwnerPawn = Character;
    ContextKit = InServiceContent->GetStationContextKit();
    ContextConstructionId = InServiceContent->GetStationContextConstructionId();
    ServiceContent = InServiceContent;
    ServiceContent->RemoveFromParent();
    ContentBox->SetContent(ServiceContent);

    const FKalmalaItemDefinition* StationItem = UKalmalaItemCatalogue::Get()->FindItem(ContextKit);
    StationTitle->SetText(FText::FromString(StationItem ? StationItem->DisplayName : ContextKit.ToString()));
    SectionTitle->SetText(FText::FromString(Section.TrimStartAndEnd()));

    bPreviousMoveInputIgnored = PC->IsMoveInputIgnored();
    bPreviousLookInputIgnored = PC->IsLookInputIgnored();
    bPreviousCursorVisible = PC->bShowMouseCursor;
    PC->SetIgnoreMoveInput(true);
    PC->SetIgnoreLookInput(true);
    PC->bShowMouseCursor = true;
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    PC->GetViewportSize(ViewportWidth, ViewportHeight);
    const float Scale = FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    const float Width = FMath::Max(360.0f, FMath::Min(1060.0f, ViewportWidth / Scale - 32.0f));
    const float Height = FMath::Max(360.0f, FMath::Min(900.0f, ViewportHeight / Scale - 32.0f));
    ContentBox->SetWidthOverride(FMath::Max(320.0f, Width - 40.0f));
    ContentBox->SetHeightOverride(FMath::Max(300.0f, Height - 68.0f));
    SetDesiredSizeInViewport(FVector2D(Width, Height));
    SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
    SetPositionInViewport(FVector2D(ViewportWidth * 0.5f, ViewportHeight * 0.5f), true);
    SetVisibility(ESlateVisibility::Visible);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(ServiceContent->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
    ServiceContent->SetKeyboardFocus();
    bOpen = true;
    return true;
}

bool UKalmalaStationContextWidget::IsTargetValid() const
{
    if (!bOpen || !ServiceContent) return false;
    const AKalmalaConstructionActor* Station = ContextStation.Get();
    const APlayerController* PC = GetOwningPlayer();
    const AKalmalaCharacter* Character = PC ? Cast<AKalmalaCharacter>(PC->GetPawn()) : nullptr;
    return IsValid(Station) && PC && Character && Character == ContextOwnerPawn.Get()
        && Station == ServiceContent->GetStationContextActor()
        && ServiceContent->GetStationContextKit() == ContextKit
        && ServiceContent->GetStationContextConstructionId() == ContextConstructionId
        && ServiceContent->IsStationContextValid();
}

void UKalmalaStationContextWidget::Close()
{
    if (!bOpen) return;
    bOpen = false;
    if (ServiceContent)
    {
        ServiceContent->Close();
        ServiceContent->RemoveFromParent();
        ServiceContent->AddToPlayerScreen(160);
    }
    if (ContentBox) ContentBox->SetContent(nullptr);
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (!bPreviousMoveInputIgnored) PC->SetIgnoreMoveInput(false);
        if (!bPreviousLookInputIgnored) PC->SetIgnoreLookInput(false);
        PC->bShowMouseCursor = bPreviousCursorVisible;
        PC->SetInputMode(FInputModeGameOnly());
    }
    ContextStation.Reset();
    ContextOwnerPawn.Reset();
    ContextKit = NAME_None;
    ContextConstructionId.Reset();
    ServiceContent = nullptr;
}

void UKalmalaStationContextWidget::CloseClicked()
{
    Close();
}

FReply UKalmalaStationContextWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right)
    {
        Close();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
