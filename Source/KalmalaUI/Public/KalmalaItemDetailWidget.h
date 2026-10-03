#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaItemDetailWidget.generated.h"

/** Shared read-only detail surface. Callers supply only owner-visible state. */
UCLASS()
class KALMALAUI_API UKalmalaItemDetailWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetItem(FName Id, const FString& Name, const FString& VisibleState, int32 TextScale, int32 Contrast);
    static FString DescribeItem(FName Id, const FString& VisibleState);
protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildPanel();
    UPROPERTY(Transient) TObjectPtr<class UBorder> Panel;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Details;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> Icon;
};
