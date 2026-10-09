#include "KalmalaInventorySubsystem.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UnrealClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaSettingsWidget.h"
#include "GameFramework/InputSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
void ApplyInventoryVisualTestSettings()
{
#if !UE_BUILD_SHIPPING
    static bool bApplied = false;
    if (bApplied || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest"))) return;
    bApplied = true;
    int32 TextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperTextScale="), TextScale))
        UKalmalaSettingsWidget::SetTextScalePercent(TextScale);
    if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperContrast="), Contrast))
        UKalmalaSettingsWidget::SetContrastMode(Contrast);
#endif
}

}

void UKalmalaInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    Background = WidgetTree->ConstructWidget<UBorder>();
    const FKalmalaUITheme& InitialTheme = FKalmalaUITheme::Get();
    InitialTheme.ApplyPanel(*Background, UKalmalaSettingsWidget::GetContrastMode(), &InitialTheme.InventoryPanelImage);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    PackText = WidgetTree->ConstructWidget<UTextBlock>();
    FKalmalaUITheme::Get().ApplyText(*PackText, FKalmalaUITheme::Get().BodySize + 1, false,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    PackText->SetAutoWrapText(true);
    // Explicit width also makes prepass height include wrapped lines before
    // the viewport panel is resized for a font/accessibility change.
    PackText->SetWrapTextAt(340 - 2 * FKalmalaUITheme::Get().PaddingX);
    CatalogueScroll = WidgetTree->ConstructWidget<UScrollBox>();
    FKalmalaUITheme::Get().ApplyScroll(*CatalogueScroll);
    auto* PackContent = WidgetTree->ConstructWidget<UVerticalBox>();
    PackContent->AddChild(PackText);
    CatalogueRows = WidgetTree->ConstructWidget<UKalmalaCatalogueRowsWidget>();
    PackContent->AddChild(CatalogueRows); CatalogueScroll->AddChild(PackContent);
    Content->AddChildToVerticalBox(CatalogueScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Background->SetContent(Content);
    WidgetTree->RootWidget = Background;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaInventoryWidget::SetPackText(const FString& Text)
{
    if (PackText && PackText->GetText().ToString() != Text) PackText->SetText(FText::FromString(Text));
}

void UKalmalaInventoryWidget::SetPackTextAccessibility(const int32 TextScalePercent, const int32 ContrastMode)
{
    const int32 BoundedTextScale = UKalmalaSettingsWidget::ClampTextScale(TextScalePercent);
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode);
    if (LastTextScalePercent == BoundedTextScale && LastContrastMode == BoundedContrast) return;

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    if (Background)
    {
        Theme.ApplyPanel(*Background, BoundedContrast, &Theme.InventoryPanelImage);
    }
    if (PackText)
    {
        Theme.ApplyText(*PackText, Theme.BodySize + 1, false, BoundedTextScale, BoundedContrast);
    }
    LastContrastMode = BoundedContrast;
    LastTextScalePercent = BoundedTextScale;
    LastContrastMode = BoundedContrast;
}

void UKalmalaInventoryWidget::SetCatalogueRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast)
{
    if (CatalogueRows) CatalogueRows->SetRows(Rows, UKalmalaInventoryComponent::MaxSlots, TextScale, Contrast);
#if !UE_BUILD_SHIPPING
    // Isolated visual review of the existing scrolled equipment region, without changing owner state.
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaEquipmentView")))
    {
        if (PackText) PackText->SetVisibility(ESlateVisibility::Collapsed);
        if (CatalogueScroll) CatalogueScroll->ScrollToEnd();
    }
#endif
}

FString UKalmalaInventoryWidget::GetCatalogueGridSummary() const
{
    const bool bScrollable = CatalogueScroll && CatalogueScroll->GetScrollOffsetOfEnd() > 1.0f;
    return CatalogueRows
        ? FString::Printf(TEXT("PackSlots=%d Filled=%d Empty=%d CarriedTools=%d Scrollable=%d"),
            CatalogueRows->GetSlotCapacity(), CatalogueRows->GetFilledSlotCount(),
            CatalogueRows->GetEmptySlotCount(), CatalogueRows->GetCarriedToolCount(), bScrollable)
        : FString::Printf(TEXT("PackSlots=0 Filled=0 Empty=0 CarriedTools=0 Scrollable=%d"), bScrollable);
}

float UKalmalaInventoryWidget::GetRequiredPanelHeight() const
{
    return Background ? Background->GetDesiredSize().Y : 0;
}

FString UKalmalaInventoryWidget::BuildPreparedFoodDetails(const bool bHasPreparedFood, const float MealSecondsRemaining)
{
    if (!bHasPreparedFood) return FString();

    const int32 BenefitPercent = FMath::RoundToInt((1.0f - UKalmalaPlayerStatusComponent::SteadyMealStaminaUseMultiplier) * 100.0f);
    FString Details = FString::Printf(
        TEXT("◇ PREPARED FOOD · One serving grants Steady Meal: stamina cost −%d%% for %.0f s. Meal effects do not stack or replace."),
        BenefitPercent, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);

    if (FMath::IsFinite(MealSecondsRemaining) && MealSecondsRemaining > 0.0f)
    {
        const int32 RemainingSeconds = FMath::CeilToInt(FMath::Clamp(MealSecondsRemaining,
            0.0f, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds));
        Details += FString::Printf(TEXT("\n◆ ACTIVE MEAL · stamina cost −%d%% · %d s remaining · wait for expiry before another meal."),
            BenefitPercent, RemainingSeconds);
    }
    return Details;
}

void UKalmalaInventorySubsystem::SetCraftingMenuSuppressed(const bool bSuppressed)
{
    bCraftingMenuSuppressed = bSuppressed;
    if (Widget)
        Widget->SetVisibility(bSuppressed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

bool UKalmalaInventorySubsystem::IsCraftingMenuSuppressed() const
{
    return bCraftingMenuSuppressed && (!Widget || Widget->GetVisibility() == ESlateVisibility::Collapsed);
}

void UKalmalaInventorySubsystem::Tick(float DeltaTime)
{
    ApplyInventoryVisualTestSettings();
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    APlayerController* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (Widget && Widget->GetOwningPlayer() != Controller)
    {
        Widget->RemoveFromParent();
        Widget = nullptr;
        bVerified = false;
    }
    if (!Controller || !Controller->IsLocalController()) return;
    if (!Widget)
    {
        Widget = CreateWidget<UKalmalaInventoryWidget>(Controller);
        if (!Widget) return;
        Widget->SetDesiredSizeInViewport(FVector2D(340, 480));
        Widget->SetPositionInViewport(FVector2D(24, 24), false);
        Widget->AddToPlayerScreen(40);
        SetCraftingMenuSuppressed(bCraftingMenuSuppressed);
    }
    APawn* Pawn = Controller->GetPawn();
    const UKalmalaInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    FString CraftKey = TEXT("Unbound");
    for (const auto& Mapping : GetDefault<UInputSettings>()->GetActionMappings())
        if (Mapping.ActionName == TEXT("CraftMenu") && !Mapping.Key.IsGamepadKey()) { CraftKey=Mapping.Key.GetDisplayName().ToString(); break; }
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn);
    FString Text;
    Text += TEXT("Pack | Build/craft: ") + CraftKey + TEXT("\n");
    bool bHasPreparedFood = false;
    TArray<FKalmalaCatalogueRow> Catalogue;
    if (!Inventory) Text += TEXT("Waiting for player");
    else if (Inventory->GetStacks().IsEmpty()) Text += TEXT("Empty");
    else
    {
        for (const auto& Stack : Inventory->GetStacks())
        {
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Stack.ItemId);
            Catalogue.Add({ Stack.ItemId,
                Item ? Item->DisplayName : Stack.ItemId.ToString(),
                FString::Printf(TEXT("× %d"), Stack.Quantity), false });
            bHasPreparedFood |= Stack.Quantity > 0 && UKalmalaPlayerStatusComponent::IsKnownFoodItem(Stack.ItemId);
        }
    }
    if (Character)
        for (const auto& Tool : Character->GetCarriedToolInventory())
        {
            FString Name = Tool.ToolId.ToString();
            Name.ReplaceInline(TEXT("ReedKnife"), TEXT("Reed knife"));
            Name.ReplaceInline(TEXT("FieldHatchet"), TEXT("Field hatchet"));
            Name.ReplaceInline(TEXT("StonePick"), TEXT("Stone pick"));
            Name.ReplaceInline(TEXT("BronzeAxe"), TEXT("Bronze axe"));
            Name.ReplaceInline(TEXT("IronAxe"), TEXT("Iron axe"));
            Name.ReplaceInline(TEXT("ConstructionHammer"), TEXT("Construction hammer"));
            const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(Tool.ToolId);
            Catalogue.Add({ Tool.ToolId, Name,
                UKalmalaCatalogueRowsWidget::BuildToolDetail(
                    Tool.ToolLevel, Tool.Durability, Definition ? Definition->MaxDurability : 0), true });
        }
    if (bHasPreparedFood)
    {
        Text += UKalmalaInventoryWidget::BuildPreparedFoodDetails(true, 0.0f);
    }
    Widget->SetPackText(Text);
    const int32 TextScalePercent = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    Widget->SetPackTextAccessibility(TextScalePercent, UKalmalaSettingsWidget::GetContrastMode());
    Widget->SetCatalogueRows(Catalogue, TextScalePercent, UKalmalaSettingsWidget::GetContrastMode());
    Widget->ForceLayoutPrepass();
    int32 TextLineCount = 1;
    for (const TCHAR CurrentChar : Text) if (CurrentChar == TEXT('\n')) ++TextLineCount;
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    if (APlayerController* PlayerController = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr)
    {
        PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
    }
    const float ViewportScale = FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(Widget));
    const float PanelWidth = ViewportWidth > 0
        ? FMath::Clamp(ViewportWidth / ViewportScale - 32.0f, 280.0f, 340.0f) : 340.0f;
    const float AvailableHeight = ViewportHeight > 0 ? ViewportHeight / ViewportScale - 32.0f : 560.0f;
    const float DesiredHeight = FMath::Max(Widget->GetRequiredPanelHeight(),
        60.0f + FMath::RoundToInt(TextLineCount * 22.0f * TextScalePercent / 100.0f));
    Widget->SetDesiredSizeInViewport(FVector2D(PanelWidth, FMath::Clamp(DesiredHeight, 180.0f, FMath::Max(180.0f, AvailableHeight))));
#if !UE_BUILD_SHIPPING
    if (!bVerified && Inventory && Inventory->GetQuantity(TEXT("Wood")) == 7
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest")))
    {
        bVerified = true;
        UE_LOG(LogTemp, Display, TEXT("Inventory presentation: Owner=1 Wood=7 ReadOnly=%d"), !Widget->IsFocusable());
        UE_LOG(LogTemp, Display, TEXT("Inventory grid: %s ReadOnly=%d"),
            *Widget->GetCatalogueGridSummary(), !Widget->IsFocusable());
    }

    if (bVerified && GridCaptureStage < 4
        && FParse::Value(FCommandLine::Get(), TEXT("KalmalaInventoryCapture="), GridCaptureBasePath))
    {
        const int32 CaptureTextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
        const int32 CaptureContrast = UKalmalaSettingsWidget::ClampContrastMode(UKalmalaSettingsWidget::GetContrastMode());
        if (GridCaptureStage <= 1)
        {
            const TArray<FKalmalaCatalogueRow> EmptyRows;
            Widget->SetCatalogueRows(EmptyRows, CaptureTextScale, CaptureContrast);
            Widget->ForceLayoutPrepass();
            if (GridCaptureStage == 0)
            {
                GridCaptureStage = 1;
                GridCaptureWait = 0.0f;
                UE_LOG(LogTemp, Display, TEXT("Inventory grid fixture: State=Empty %s"),
                    *Widget->GetCatalogueGridSummary());
            }
            GridCaptureWait += DeltaTime;
            if (GridCaptureWait >= (FParse::Param(FCommandLine::Get(), TEXT("KalmalaEquipmentView")) ? 5.0f : 3.0f))
            {
                FScreenshotRequest::RequestScreenshot(GridCaptureBasePath + TEXT("-empty.png"), true, false);
                GridCaptureStage = 2;
                GridCaptureWait = 0.0f;
            }
        }
        else if (GridCaptureStage <= 3)
        {
            Widget->SetCatalogueRows(Catalogue, CaptureTextScale, CaptureContrast);
            Widget->ForceLayoutPrepass();
            if (GridCaptureStage == 2)
            {
                GridCaptureStage = 3;
                GridCaptureWait = 0.0f;
                UE_LOG(LogTemp, Display, TEXT("Inventory grid fixture: State=Filled %s"),
                    *Widget->GetCatalogueGridSummary());
            }
            GridCaptureWait += DeltaTime;
            if (GridCaptureWait >= (FParse::Param(FCommandLine::Get(), TEXT("KalmalaEquipmentView")) ? 5.0f : 3.0f))
            {
                FScreenshotRequest::RequestScreenshot(GridCaptureBasePath + TEXT("-filled.png"), true, false);
                GridCaptureStage = 4;
            }
        }
    }
#endif
}

void UKalmalaInventorySubsystem::Deinitialize()
{
    if (Widget) Widget->RemoveFromParent();
    Widget = nullptr;
    bVerified = false;
    Super::Deinitialize();
}
