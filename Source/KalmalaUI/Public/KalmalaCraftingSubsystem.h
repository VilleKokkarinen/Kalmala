#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "Styling/SlateTypes.h"
#include "KalmalaCraftingSubsystem.generated.h"
class UTextBlock;
class UButton;
class UBorder;
class UUniformGridPanel;
class UScrollBox;
class UInputComponent;
class UKalmalaCraftingComponent;

UCLASS()
class KALMALAUI_API UKalmalaStationPromptWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPrompt(const FString& Text);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;
};

UCLASS()
class KALMALAUI_API UKalmalaCraftingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Open();
    void OpenForStation(FName StationKit);
    void Close();
    bool IsOpen() const { return bOpen; }
    FString GetPresentationText() const;
    FString GetRecipeGridSummary() const;
#if !UE_BUILD_SHIPPING
    bool VerifyRecipeGridNavigationForTest();
    bool VerifyInventoryInspectionForTest();
    bool ScrollReviewSectionForTest(bool bFeedback);
    bool ScrollInventoryDetailsForTest();
    bool PrepareBrowseReviewForTest(int32 View);
#endif
    void SetRecipeBrowse(const FString& Query, int32 Category, bool bNameSort);
    TArray<int32> GetVisibleRecipeIndices() const;
    static int32 GetBuildBrowseGroup(FName Output);
    static FString GetBrowseCategoryLabel(int32 Category);
    void EnablePlacementPreview();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UKalmalaCraftingComponent* Model() const;
    void OpenInternal(FName StationKit);
    UFUNCTION() void RecipeSearchChanged(const FText& Text);
    UFUNCTION() void CycleRecipeCategory();
    UFUNCTION() void CycleRecipeSort();
    UFUNCTION() void ClearRecipeSearch();
    UFUNCTION() void Previous();
    UFUNCTION() void Next();
    UFUNCTION() void Craft();
    UFUNCTION() void Preview();
    UFUNCTION() void Place();
    UFUNCTION() void Refuel();
    UFUNCTION() void Light();
    UFUNCTION() void EatFood();
    UFUNCTION() void EatBroth();
    UFUNCTION() void EatSmokedMeat();
    UFUNCTION() void RepairReedKnife();
    UFUNCTION() void RepairFieldHatchet();
    UFUNCTION() void RepairStonePick();
    UFUNCTION() void RepairBronzeAxe();
    UFUNCTION() void RepairIronAxe();
    UFUNCTION() void CraftBronzeAxe();
    UFUNCTION() void UpgradeIronAxe();
    UFUNCTION() void InspectStorage();
    UFUNCTION() void PreviousStorageItem();
    UFUNCTION() void NextStorageItem();
    UFUNCTION() void DepositStorage();
    UFUNCTION() void WithdrawStorage();
    UFUNCTION() void CloseClicked();
    UFUNCTION() void FocusInventoryDetails();
    void Refresh();
    void RefreshRecipeGrid(const TArray<int32>& VisibleIndices, UKalmalaCraftingComponent* Crafting,
        int32 TextScalePercent, int32 ContrastMode);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipesText;
    UPROPERTY(Transient) TObjectPtr<UBorder> MenuBackground;
    UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> RecipeGrid;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> CraftingScrollBox;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> RecipeSlotCards;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotNames;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotStates;
    UPROPERTY(Transient) TArray<uint8> RecipeSlotVisualStates;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> SelectedIcon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InstructionsText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIngredientWidget> Ingredients;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RepairText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolProgressionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StorageText;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaInventoryInspectWidget> InventoryInspector;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> WrappedTextBlocks;
    UPROPERTY(Transient) TObjectPtr<UButton> CraftButton;
    UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> RecipeSearchBox;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipeCategoryLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipeSortLabel;
    UPROPERTY(Transient) FEditableTextBoxStyle RecipeSearchStyle;
    FString RecipeQuery;
    int32 RecipeCategory = 0;
    bool bRecipeNameSort = false;
    int32 Selected = 0;
    FName StationFilterKit;
    FString GeneralInstructions;
    int32 SelectedStorageItem = 0;
    int32 LastDetailTextScalePercent = INDEX_NONE;
    int32 LastDetailContrastMode = INDEX_NONE;
    int32 LastRecipeGridTextScalePercent = INDEX_NONE;
    int32 LastRecipeGridContrastMode = INDEX_NONE;
    int32 RecipeGridUnavailableCount = 0;
    int32 RecipeGridSelectedIndex = INDEX_NONE;
    bool bRecipeGridFocused = false;
    TArray<int32> LastRecipeGridIndices;
    bool bOpen = false;
    bool bPlacementPreviewEnabled = false;
    bool bPreviousCursor = false;
};

UCLASS()
class KALMALAUI_API UKalmalaCraftingSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaCraftingSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
    bool CloseIfOpen();
    bool IsOpen() const { return Widget != nullptr && Widget->IsOpen(); }
private:
    void Toggle();
    void Release();
    void UpdateStationPrompt(APlayerController* PlayerController);
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaStationPromptWidget> StationPrompt;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<UKalmalaCraftingComponent> StationInteractionModel;
    TWeakObjectPtr<UInputComponent> BoundInput;
    uint32 LastStationInteractionSerial = 0;
    bool bHasSeenStationInteraction = false;
    bool bVerified = false;
    bool bCaptureRequested = false;
    int32 ReviewCaptureStage = 0;
    float CaptureWait = 0;
    float VerificationLayoutWait = 0;
};
