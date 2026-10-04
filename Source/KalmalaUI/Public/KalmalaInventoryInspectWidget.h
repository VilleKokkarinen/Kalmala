#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryInspectWidget.generated.h"

/** Owner-supplied inventory inspection inside the existing modal only. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryInspectWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetRows(const TArray<FKalmalaCatalogueRow>& InRows, int32 Scale, int32 Contrast);
    bool Navigate(FKey Key);
    FName GetSelectedItem() const;
    static FString ActionGuidance(FName Id, bool bTool);
protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    void Build();
    void Refresh();
    UFUNCTION() void Previous();
    UFUNCTION() void Next();
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Column;
    UPROPERTY(Transient) TObjectPtr<class UUniformGridPanel> Grid;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaItemDetailWidget> Detail;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Instructions;
    UPROPERTY(Transient) TArray<TObjectPtr<class UButton>> Buttons;
    TArray<FKalmalaCatalogueRow> Rows;
    int32 Selected = 0;
    int32 TextScale = 100;
    int32 ContrastMode = 0;
    FString LastRows;
};
