#include "KalmalaInventoryGridWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaIconWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"

FString UKalmalaInventoryGridWidget::SlotLabel(const int32 Slot)
{
    return Slot >= 0 && Slot < 10 ? FString::FromInt((Slot + 1) % 10) : TEXT("");
}

TArray<int32> UKalmalaInventoryGridWidget::VisibleSlots(const TArray<FName>& Slots, const bool bHotbar)
{
    TArray<int32> Result;
    for (int32 Slot = 0; Slot < (bHotbar ? 10 : 40); ++Slot)
        if (!bHotbar || (Slots.IsValidIndex(Slot) && !Slots[Slot].IsNone())) Result.Add(Slot);
    return Result;
}

void UKalmalaInventoryGridWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
    WidgetTree->RootWidget = Canvas;
    for (int32 Slot = 0; Slot < 40; ++Slot)
    {
        auto* Border = WidgetTree->ConstructWidget<UBorder>();
        Border->SetPadding(FMargin(2.0f));
        Border->SetContent(WidgetTree->ConstructWidget<UBorder>());
        Canvas->AddChildToCanvas(Border)->SetZOrder(0);
        Backgrounds.Add(Border);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        Icon->SetVisibility(ESlateVisibility::Collapsed);
        Canvas->AddChildToCanvas(Icon)->SetZOrder(1);
        Icons.Add(Icon);
    }
}

void UKalmalaInventoryGridWidget::Refresh(UKalmalaInventoryComponent* Inventory, const bool bHotbar, const FName Selected)
{
    if (OwnerInventory != Inventory) CancelMove();
    OwnerInventory = Inventory;
    const bool bModeChanged = bHotbarOnly != bHotbar;
    bHotbarOnly = bHotbar;
    SetIsFocusable(!bHotbar);
    SelectedItem = Selected;
    TArray<FName> Next;
    Next.Init(NAME_None, 40);
    if (Inventory) for (int32 Slot = 0; Slot < 40; ++Slot) Next[Slot] = Inventory->GetSlotItem(Slot);
    if (Next != SlotItems)
    {
        SlotItems = MoveTemp(Next);
        if (Icons.Num() == 40)
            for (int32 Slot = 0; Slot < 40; ++Slot)
                if (!SlotItems[Slot].IsNone()) Icons[Slot]->SetCatalogueIcon(SlotItems[Slot]);
        LastSize = FVector2D::ZeroVector;
    }
    DisplaySlots = VisibleSlots(SlotItems, bHotbar);
    const auto& Theme = FKalmalaUITheme::Get();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    for (int32 Slot = 0; Slot < Backgrounds.Num(); ++Slot)
    {
        Backgrounds[Slot]->SetBrushColor((!SelectedItem.IsNone() && SlotItems[Slot] == SelectedItem)
            || (HasKeyboardFocus() && FocusedSlot == Slot)
            ? Theme.TextColor(true, Contrast) : Theme.BorderColor);
        CastChecked<UBorder>(Backgrounds[Slot]->GetContent())->SetBrushColor(Contrast == 0
            ? Theme.Panel : Theme.HighContrastPanel);
    }
    if (bModeChanged) LastSize = FVector2D::ZeroVector;
    Invalidate(EInvalidateWidget::Paint);
}

void UKalmalaInventoryGridWidget::NativeTick(const FGeometry& G, const float DeltaTime)
{
    Super::NativeTick(G, DeltaTime);
    if (Icons.Num() != 40 || G.GetLocalSize() == LastSize) return;
    LastSize = G.GetLocalSize();
    const float Cell = FMath::Min(LastSize.X / (bHotbarOnly ? FMath::Max(1, DisplaySlots.Num()) : 10),
        LastSize.Y / (bHotbarOnly ? 1 : 4));
    if (Cell <= 2.0f) return;
    for (const auto& Icon : Icons) Icon->SetVisibility(ESlateVisibility::Collapsed);
    for (int32 Slot = 0; Slot < Backgrounds.Num(); ++Slot)
    {
        Backgrounds[Slot]->SetVisibility(bHotbarOnly ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        auto* Placement = CastChecked<UCanvasPanelSlot>(Backgrounds[Slot]->Slot);
        Placement->SetPosition(FVector2D(Slot % 10, Slot / 10) * Cell + FVector2D(1));
        Placement->SetSize(FVector2D(Cell - 2));
    }
    for (int32 Index = 0; Index < DisplaySlots.Num(); ++Index)
    {
        const int32 Slot = DisplaySlots[Index];
        if (SlotItems[Slot].IsNone()) continue;
        auto* Icon = Icons[Slot].Get();
        auto* Placement = CastChecked<UCanvasPanelSlot>(Icon->Slot);
        const FVector2D Position = FVector2D(bHotbarOnly ? Index : Slot % 10, bHotbarOnly ? 0 : Slot / 10) * Cell;
        Placement->SetPosition(Position + FVector2D(Cell * 0.18f, Cell * 0.18f));
        Placement->SetSize(FVector2D(Cell * 0.64f, Cell * 0.64f));
        Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

int32 UKalmalaInventoryGridWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G,
    const FSlateRect& Culling, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, const bool bEnabled) const
{
    const auto& Theme = FKalmalaUITheme::Get();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    const auto* Brush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    const float Cell = FMath::Min(G.GetLocalSize().X / (bHotbarOnly ? FMath::Max(1, DisplaySlots.Num()) : 10),
        G.GetLocalSize().Y / (bHotbarOnly ? 1 : 4));
    Layer = Super::NativePaint(Args, G, Culling, Elements, Layer, Style, bEnabled) + 1;
    FSlateFontInfo Font = Theme.MakeFont(FMath::Clamp(FMath::RoundToInt(Cell * .22f), 8, 16),
        true, UKalmalaSettingsWidget::GetTextScalePercent());
    Font.Size = FMath::Clamp<float>(Font.Size, 8, 20);
    const auto* Character = OwnerInventory ? Cast<AKalmalaCharacter>(OwnerInventory->GetOwner()) : nullptr;
    for (int32 Index = 0; Index < DisplaySlots.Num(); ++Index)
    {
        const int32 Slot = DisplaySlots[Index];
        const FVector2D P = FVector2D(bHotbarOnly ? Index : Slot % 10, bHotbarOnly ? 0 : Slot / 10) * Cell;
        auto Text = [&](const FString& Value, const FVector2D Offset)
        {
            if (Value.IsEmpty()) return;
            const FGeometry TextGeometry = G.MakeChild(FVector2D(Cell), FSlateLayoutTransform(P + Offset));
            const FGeometry ShadowGeometry = G.MakeChild(FVector2D(Cell), FSlateLayoutTransform(P + Offset + FVector2D(1)));
            FSlateDrawElement::MakeText(Elements, Layer, ShadowGeometry.ToPaintGeometry(), Value, Font, ESlateDrawEffect::None,
                FLinearColor::Black);
            FSlateDrawElement::MakeText(Elements, Layer + 1, TextGeometry.ToPaintGeometry(), Value, Font, ESlateDrawEffect::None,
                Theme.TextColor(false, Contrast));
        };
        Text(SlotLabel(Slot), FVector2D(4, 2));
        const FName Id = SlotItems[Slot];
        const int32 Quantity = OwnerInventory ? OwnerInventory->GetQuantity(Id) : 0;
        if (Quantity > 1) Text(FString::FromInt(Quantity), FVector2D(4, Cell * .72f));
        if (Character)
            if (const auto* Tool = FKalmalaToolLifecycleContract::FindDefinition(Id))
            {
                const float Fraction = FMath::Clamp(float(Character->GetToolDurability(Id)) / FMath::Max(1, Tool->MaxDurability), 0.0f, 1.0f);
                FSlateDrawElement::MakeBox(Elements, Layer, G.ToPaintGeometry(FVector2D(Cell - 8, 3),
                    FSlateLayoutTransform(P + FVector2D(4, Cell - 5))), Brush, ESlateDrawEffect::None, FLinearColor(0.3f, 0.08f, 0.06f, 1));
                FSlateDrawElement::MakeBox(Elements, Layer + 1, G.ToPaintGeometry(FVector2D((Cell - 8) * Fraction, 3),
                    FSlateLayoutTransform(P + FVector2D(4, Cell - 5))), Brush, ESlateDrawEffect::None, Theme.TextColor(false, Contrast));
            }
        if (OwnerInventory && Id == OwnerInventory->GetActiveItem() && !Id.IsNone())
            FSlateDrawElement::MakeBox(Elements, Layer + 1, G.ToPaintGeometry(FVector2D(Cell - 8, 2),
                FSlateLayoutTransform(P + FVector2D(4, Cell - 2))), Brush, ESlateDrawEffect::None, Theme.TextColor(true, Contrast));
    }
    return Layer + 2;
}

int32 UKalmalaInventoryGridWidget::SlotAt(const FGeometry& G, const FVector2D ScreenPosition) const
{
    const FVector2D P = G.AbsoluteToLocal(ScreenPosition);
    const float Cell = FMath::Min(G.GetLocalSize().X / 10, G.GetLocalSize().Y / 4);
    if (bHotbarOnly || Cell <= 0 || P.X < 0 || P.Y < 0 || P.X >= Cell * 10 || P.Y >= Cell * 4) return INDEX_NONE;
    return FMath::FloorToInt(P.Y / Cell) * 10 + FMath::FloorToInt(P.X / Cell);
}

void UKalmalaInventoryGridWidget::SelectSlot(const int32 Slot)
{
    if (!SlotItems.IsValidIndex(Slot)) return;
    FocusedSlot = Slot;
    SelectedItem = SlotItems[Slot];
    OnItemSelected.Broadcast(SelectedItem);
    Invalidate(EInvalidateWidget::Paint);
}

void UKalmalaInventoryGridWidget::RequestMove(const int32 Source, const int32 Target, const FName SourceId, const FName TargetId)
{
    if (OwnerInventory && Source != Target && !SourceId.IsNone())
        OwnerInventory->ServerMoveSlot(Source, Target, SourceId, TargetId);
}

FReply UKalmalaInventoryGridWidget::NativeOnMouseButtonDown(const FGeometry& G, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnMouseButtonDown(G, Event);
    const int32 Slot = SlotAt(G, Event.GetScreenSpacePosition());
    if (Slot == INDEX_NONE) return FReply::Unhandled();
    if (MoveSource != INDEX_NONE)
    {
        RequestMove(MoveSource, Slot, MoveItem, SlotItems[Slot]);
        MoveSource = INDEX_NONE;
        SelectSlot(Slot);
        return FReply::Handled();
    }
    SelectSlot(Slot);
    DragSource = Slot;
    DragItem = SlotItems[Slot];
    return FReply::Handled().CaptureMouse(TakeWidget()).SetUserFocus(TakeWidget());
}

FReply UKalmalaInventoryGridWidget::NativeOnMouseButtonUp(const FGeometry& G, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton || DragSource == INDEX_NONE) return FReply::Unhandled();
    const int32 Target = SlotAt(G, Event.GetScreenSpacePosition());
    if (Target != INDEX_NONE) RequestMove(DragSource, Target, DragItem, SlotItems[Target]);
    DragSource = INDEX_NONE;
    return FReply::Handled().ReleaseMouseCapture();
}

void UKalmalaInventoryGridWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    DragSource = INDEX_NONE;
    Super::NativeOnMouseCaptureLost(Event);
}

FReply UKalmalaInventoryGridWidget::NativeOnMouseMove(const FGeometry& G, const FPointerEvent& Event)
{
    const int32 Slot = SlotAt(G, Event.GetScreenSpacePosition());
    const FName Id = SlotItems.IsValidIndex(Slot) ? SlotItems[Slot] : NAME_None;
    const auto* Definition = UKalmalaItemCatalogue::Get()->FindItem(Id);
    SetToolTipText(FText::FromString(Definition ? Definition->DisplayName : Id.IsNone() ? TEXT("Empty slot") : Id.ToString()));
    return Super::NativeOnMouseMove(G, Event);
}

FReply UKalmalaInventoryGridWidget::NativeOnKeyDown(const FGeometry& G, const FKeyEvent& Event)
{
    const FKey Key = Event.GetKey();
    if (bHotbarOnly || SlotItems.Num() != 40) return FReply::Unhandled();
    int32 Step = 0;
    if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left) Step = -1;
    if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right) Step = 1;
    if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) Step = -10;
    if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) Step = 10;
    if (Step) { SelectSlot(FMath::Clamp(FocusedSlot + Step, 0, 39)); return FReply::Handled(); }
    if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        if (MoveSource == INDEX_NONE) { MoveSource = FocusedSlot; MoveItem = SlotItems[FocusedSlot]; }
        else { RequestMove(MoveSource, FocusedSlot, MoveItem, SlotItems[FocusedSlot]); MoveSource = INDEX_NONE; }
        return FReply::Handled();
    }
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
    {
        if (MoveSource != INDEX_NONE) { MoveSource = INDEX_NONE; return FReply::Handled(); }
    }
    return Super::NativeOnKeyDown(G, Event);
}

void UKalmalaInventoryGridWidget::CancelMove()
{
    MoveSource = DragSource = INDEX_NONE;
    MoveItem = DragItem = NAME_None;
}
