#include "KalmalaNotificationSubsystem.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaUITheme.h"
#include "KalmalaIconWidget.h"
#include "KalmalaSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace
{
FString NoticeText(const FKalmalaSkillNotice& Notice);
const TCHAR* SkillName(EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return TEXT("Gathering");
    case EKalmalaSkill::Woodcutting: return TEXT("Woodcutting");
    case EKalmalaSkill::Mining: return TEXT("Mining");
    case EKalmalaSkill::Crafting: return TEXT("Crafting");
    case EKalmalaSkill::Cooking: return TEXT("Cooking");
    case EKalmalaSkill::Survival: return TEXT("Survival");
    default: return TEXT("Skill");
    }
}
EKalmalaIcon SkillIcon(EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return EKalmalaIcon::Fibre;
    case EKalmalaSkill::Woodcutting: return EKalmalaIcon::Axe;
    case EKalmalaSkill::Mining: return EKalmalaIcon::Pick;
    case EKalmalaSkill::Crafting: return EKalmalaIcon::Hammer;
    case EKalmalaSkill::Cooking: return EKalmalaIcon::Bowl;
    default: return EKalmalaIcon::Shield;
    }
}
FString NoticeText(const FKalmalaSkillNotice& Notice)
{
    if (!Notice.ItemId.IsNone())
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Notice.ItemId);
        return FString::Printf(TEXT("Gained %d %s"), Notice.Quantity, Item ? *Item->DisplayName : *Notice.ItemId.ToString());
    }
    return FString::Printf(TEXT("%s reached level %d"), SkillName(Notice.Skill), Notice.Level);
}
}

void UKalmalaNotificationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPanel();
}

TSharedRef<SWidget> UKalmalaNotificationWidget::RebuildWidget()
{
    BuildPanel();
    return Super::RebuildWidget();
}

void UKalmalaNotificationWidget::BuildPanel()
{
    if (Panel || !WidgetTree) return;
    SetIsFocusable(false);
    Panel = WidgetTree->ConstructWidget<UBorder>();
    Rows = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Rows); WidgetTree->RootWidget = Panel;
}

void UKalmalaNotificationWidget::SetNotices(const TArray<FKalmalaSkillNotice>& Notices, int32 Scale, int32 Contrast)
{
    if (!WidgetTree) Initialize();
    if (!Panel) TakeWidget();
    if (!Panel) return;
    FString Next;
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
        Next += NoticeText(Notices[Index]) + TEXT("\n");
    if (Next == Presentation && Scale == LastScale && Contrast == LastContrast) return;
    Presentation = Next; LastScale = Scale; LastContrast = Contrast;
    Rows->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();
    const FString NoImage; Theme.ApplyPanel(*Panel, Contrast, &NoImage);
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
    {
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Box = WidgetTree->ConstructWidget<USizeBox>();
        Box->SetWidthOverride(28); Box->SetHeightOverride(28);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        EKalmalaIcon Kind = SkillIcon(Notices[Index].Skill); int32 Variant = 0;
        if (!Notices[Index].ItemId.IsNone()) UKalmalaIconWidget::FindCatalogueIcon(Notices[Index].ItemId, Kind, Variant);
        Icon->SetIcon(Kind, Variant);
        Box->SetContent(Icon); Row->AddChild(Box);
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetAutoWrapText(true);
        Text->SetText(FText::FromString(NoticeText(Notices[Index])));
        Theme.ApplyText(*Text, Theme.BodySize, false, Scale, Contrast);
        auto* TextSlot = Row->AddChildToHorizontalBox(Text); TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetPadding(FMargin(Theme.SlotPadding, 0)); Rows->AddChild(Row);
    }
}

void UKalmalaNotificationSubsystem::Tick(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    auto* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    APawn* Pawn = Controller && Controller->IsLocalController() ? Controller->GetPawn() : nullptr;
    if (OwnerPawn.Get() != Pawn)
    {
        Queue.Reset(); OwnerPawn = Pawn;
        if (Widget) Widget->RemoveFromParent(); Widget = nullptr;
    }
    Queue.Tick(DeltaTime);
    const auto* Skills = Pawn ? Pawn->FindComponentByClass<UKalmalaSkillProgressionComponent>() : nullptr;
    if (Skills) Queue.Observe(Skills->GetDetailedProgression(), FKalmalaUITheme::Get().NotificationLifetime);
    const auto* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Inventory) Queue.ObserveGains(Inventory->GetGainReceipts(), FKalmalaUITheme::Get().NotificationLifetime);
    if (!Pawn || !Controller || Queue.GetRows().IsEmpty() || Controller->IsMoveInputIgnored())
    {
        if (Widget) Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    if (!Widget)
    {
        Widget = CreateWidget<UKalmalaNotificationWidget>(Controller);
        if (!Widget) return;
        Widget->AddToPlayerScreen(110);
    }
    int32 Width = 0, Height = 0; Controller->GetViewportSize(Width, Height);
    if (Width <= 48 || Height <= 200)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    const float DPI = FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(Widget));
    Widget->SetAlignmentInViewport(FVector2D(1,1));
    Widget->SetPositionInViewport(FVector2D(Width / DPI - 24, Height / DPI - 160), false);
    const auto& Theme = FKalmalaUITheme::Get();
    const float FontSize = Theme.MakeFont(Theme.BodySize, false, UKalmalaSettingsWidget::GetTextScalePercent()).Size;
    const float PanelHeight = Queue.GetRows().Num() * (FontSize * 2 + 28) + Theme.PaddingY * 2;
    Widget->SetDesiredSizeInViewport(FVector2D(FMath::Max(1.f, FMath::Min(320.f, Width / DPI - 48)), PanelHeight));
    Widget->SetNotices(Queue.GetRows(), UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaNotificationSubsystem::Deinitialize()
{
    if (Widget) Widget->RemoveFromParent(); Widget = nullptr;
    Queue.Reset(); OwnerPawn.Reset(); Super::Deinitialize();
}
