#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.generated.h"
struct FKalmalaCatalogueRow
{
    FName Id;
    FString Name;
    FString Detail;
    bool bCarriedTool = false;
};

/** Read-only pack slots plus separately framed carried tools. */
UCLASS()
class KALMALAUI_API UKalmalaCatalogueRowsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static FString BuildToolDetail(int32 Level, int32 Condition, int32 MaximumCondition);
    void SetRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 SlotCapacity, int32 TextScale, int32 Contrast,
        FName SelectedItem = NAME_None);
    /** Compact read-only catalogue presentation used by the developer icon gallery. */
    void SetRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast);
    int32 GetSlotCapacity() const { return SlotCapacity; }
    int32 GetFilledSlotCount() const { return FilledSlotCount; }
    int32 GetEmptySlotCount() const { return FMath::Max(0, SlotCapacity - FilledSlotCount); }
    int32 GetCarriedToolCount() const { return CarriedToolCount; }
#if !UE_BUILD_SHIPPING
    void SetVerificationBackground();
#endif
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Column;
    int32 SlotCapacity = 0;
    int32 FilledSlotCount = 0;
    int32 CarriedToolCount = 0;
    FString LastRows;
};
