#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "KalmalaInventorySubsystem.generated.h"

class UTextBlock;
class UBorder;
class UKalmalaCatalogueRowsWidget;
class UScrollBox;
struct FKalmalaCatalogueRow;

UCLASS()
class KALMALAUI_API UKalmalaInventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPackText(const FString& Text);
    void SetCatalogueRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast);
    FString GetCatalogueGridSummary() const;
    void SetPackTextAccessibility(int32 TextScalePercent, int32 ContrastMode);
    float GetRequiredPanelHeight() const;
    static FString BuildPreparedFoodDetails(bool bHasPreparedFood, float MealSecondsRemaining);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UBorder> Background;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCatalogueRowsWidget> CatalogueRows;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> CatalogueScroll;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};

/** Read-only local inventory presentation, with no input bindings or network requests. */
UCLASS()
class KALMALAUI_API UKalmalaInventorySubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    void SetCraftingMenuSuppressed(bool bSuppressed);
    bool IsCraftingMenuSuppressed() const;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaInventorySubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryWidget> Widget;
    bool bCraftingMenuSuppressed = false;
    bool bVerified = false;
    int32 GridCaptureStage = 0;
    float GridCaptureWait = 0.0f;
    FString GridCaptureBasePath;
};
