#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.generated.h"
struct FKalmalaCatalogueRow { FName Id; FString Text; };
/** Compact shared icon/name rows without slots or gameplay actions. */
UCLASS()
class KALMALAUI_API UKalmalaCatalogueRowsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast);
#if !UE_BUILD_SHIPPING
    void SetVerificationBackground();
#endif
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Column;
    FString LastRows;
};
