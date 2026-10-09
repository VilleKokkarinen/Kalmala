#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "Styling/SlateTypes.h"
#include "KalmalaInventoryInspectWidget.generated.h"

/** Owner-supplied inventory inspection inside the existing modal only. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryInspectWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetRows(const TArray<FKalmalaCatalogueRow>& InRows, int32 Scale, int32 Contrast,
        const FString& ListLabel = TEXT("Inventory"), const FString& EmptyLabel = TEXT("Your inventory is empty."),
        bool bAllowToolFilter = true, int32 GridColumns = 4);
    bool Navigate(FKey Key);
    FName GetSelectedItem() const;
    const FString& GetSearch() const { return Search; }
    void SetSearch(const FString& Query);
    void SetCategory(int32 Category);
    void SetSort(int32 Sort);
    void RestoreBrowseState(const FString& Query, int32 InCategory, int32 InSort, FName SelectedItemId);
    int32 GetVisibleCount() const { return Rows.Num(); }
    int32 GetCategory() const { return Category; }
    int32 GetSort() const { return Sort; }
    static FString ActionGuidance(FName Id, bool bTool);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void Build();
    void Refresh();
    void Refilter();
    UFUNCTION() void SearchChanged(const FText& Text);
    UFUNCTION() void CycleCategory();
    UFUNCTION() void CycleSort();
    UFUNCTION() void ClearSearch();
    UFUNCTION() void Previous();
    UFUNCTION() void Next();
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Column;
    UPROPERTY(Transient) TObjectPtr<class UGridPanel> Grid;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaItemDetailWidget> Detail;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Instructions;
    UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> SearchBox;
    UPROPERTY(Transient) TObjectPtr<class UButton> CategoryButton;
    UPROPERTY(Transient) TObjectPtr<class UButton> SortButton;
    UPROPERTY(Transient) TObjectPtr<class UButton> ClearButton;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Results;
    // UE 5.8's editable-text setter retains the supplied style address in Slate.
    UPROPERTY(Transient) FEditableTextBoxStyle BrowseSearchStyle;
    UPROPERTY(Transient) TArray<TObjectPtr<class UButton>> Buttons;
    TArray<FKalmalaCatalogueRow> Rows;
    TArray<FKalmalaCatalogueRow> OwnerRows;
    FString Search;
    int32 Category = 0; // All, items, carried tools.
    int32 Sort = 0; // Owner order, name, category/name.
    int32 Selected = 0;
    int32 TextScale = 100;
    int32 ContrastMode = 0;
    FString ListLabel = TEXT("Inventory");
    FString EmptyLabel = TEXT("Your inventory is empty.");
    bool bAllowToolFilter = true;
    int32 GridColumns = 4;
    FString LastRows;
};
