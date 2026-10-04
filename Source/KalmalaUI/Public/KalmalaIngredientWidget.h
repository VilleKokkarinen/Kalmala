#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaIngredientWidget.generated.h"

/** Read-only batch-one costs against the supplied owning pawn's inventory. */
UCLASS()
class KALMALAUI_API UKalmalaIngredientWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetIngredients(const TArray<FKalmalaInventoryStack>& Costs,
        const UKalmalaInventoryComponent* Inventory, int32 TextScale, int32 Contrast);
    FString GetPresentationText() const { return Presentation; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> Rows;
    FString Presentation;
    int32 LastScale = INDEX_NONE;
    int32 LastContrast = INDEX_NONE;
};
