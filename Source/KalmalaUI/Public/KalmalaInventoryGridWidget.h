#pragma once
#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaInventoryGridWidget.generated.h"

class UKalmalaInventoryComponent;
class UKalmalaIconWidget;
class UCanvasPanel;
class UBorder;

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaGridSelection, FName);
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaGridHover, FName);

UCLASS()
class KALMALAUI_API UKalmalaInventoryDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) FGuid ItemGuid;
    UPROPERTY(Transient) FName ItemId;
    UPROPERTY(Transient) int32 SourceSlot = INDEX_NONE;
};

/** One owner inventory view; HUD mode displays only occupied numbered cells. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryGridWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Refresh(UKalmalaInventoryComponent* Inventory, bool bHotbar, FName Selected = NAME_None);
    FKalmalaGridSelection OnItemSelected;
    FKalmalaGridHover OnItemHovered;
    void ClearHover();
    void CancelMove();
    FGuid GetFocusedInstanceGuid() const
    {
        return SlotInstanceGuids.IsValidIndex(FocusedSlot) ? SlotInstanceGuids[FocusedSlot] : FGuid();
    }
    static FString SlotLabel(int32 SlotIndex);
    static TArray<int32> VisibleSlots(const TArray<FName>& Slots, bool bHotbar);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& Event) override;
    virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event,
        UDragDropOperation*& Operation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& Event, UDragDropOperation* Operation) override;
    virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event,
        UDragDropOperation* Operation) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    int32 SlotAt(const FGeometry& Geometry, FVector2D ScreenPosition) const;
    void SelectSlot(int32 SlotIndex);
    void RequestMove(int32 Source, int32 Target, FGuid SourceGuid, FGuid TargetGuid);
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaIconWidget>> Icons;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Backgrounds;
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryComponent> OwnerInventory;
    TArray<FName> SlotItems;
    TArray<FGuid> SlotInstanceGuids;
    TArray<int32> DisplaySlots;
    bool bHotbarOnly = false;
    FName SelectedItem;
    FName HoveredItem;
    int32 FocusedSlot = 0;
    int32 DragSource = INDEX_NONE;
    FGuid DragInstanceGuid;
    int32 MoveSource = INDEX_NONE;
    FGuid MoveInstanceGuid;
    FVector2D LastSize = FVector2D::ZeroVector;
};
