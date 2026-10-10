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
class UKalmalaSelectedResultWidget;
class AKalmalaCharacter;

struct FKalmalaMenuBrowseMemory
{
    FString Query;
    int32 Category = 0;
    bool bNameSort = false;
    FName SelectedRecipeId;
    float ScrollOffset = 0.0f;
};

struct FKalmalaInventoryInspectionMemory
{
    bool bHasState = false;
    bool bWasLastActive = false;
    FString Query;
    int32 Category = 0;
    int32 Sort = 0;
    FName SelectedItemId;
    float ScrollOffset = 0.0f;
};

class UKalmalaCraftingSubsystem;
enum class EKalmalaCraftingActionKind : uint8;
struct FKalmalaAcceptedCraftingActionReceipt;
class AKalmalaConstructionActor;
class APawn;
class UKalmalaStationContextWidget;
struct FKalmalaRecipe;

UCLASS()
class KALMALAUI_API UKalmalaInteractionPromptWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPrompt(const FString& Text);
    static FString GetConstructionActionName(FName ConstructionKit);
    static FString BuildPromptText(const FString& TargetName, const FString& ActionName,
        const FString& UnavailableReason = FString(), bool bModalOpen = false);
protected:
    virtual void NativeOnInitialized() override;
private:
    void ApplyPromptStyle();
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;
};

UCLASS()
class KALMALAUI_API UKalmalaCraftingWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Open();
    bool OpenAsInventoryCompanion();
    void CloseInventoryCompanion();
    bool OpenForStation(FName StationKit);
    bool OpenInStationContext(AKalmalaConstructionActor* Station, const FString& Section);
    bool IsStationContextValid() const;
    AKalmalaConstructionActor* GetStationContextActor() const { return ContextStationActor.Get(); }
    FName GetStationContextKit() const { return bEmbeddedContext ? StationFilterKit : NAME_None; }
    const FString& GetStationContextConstructionId() const { return ContextConstructionId; }
    const FString& GetStationContextSection() const { return StationContextSection; }
    void Close();
    bool IsOpen() const { return bOpen; }
    FString GetPresentationText() const;
    FString GetRecipeGridSummary() const;
#if !UE_BUILD_SHIPPING
    bool VerifyRecipeGridNavigationForTest();
    bool VerifyMenuBrowseMemoryForTest();
    bool PrepareRecipeActivityReviewForTest();
    bool VerifyInventoryInspectionForTest();
    bool VerifyBuildMenuCleanupForTest();
    bool VerifyStorageContextScopeForTest();
    bool VerifyCookingRackScopeForTest();
    bool VerifyCauldronScopeForTest();
    bool VerifyFryingPanScopeForTest();
    bool VerifyForgeUpgradeScopeForTest();
    bool VerifyForgeRepairScopeForTest();
    bool VerifyWorkbenchRepairScopeForTest();
    bool ScrollReviewSectionForTest(bool bFeedback);
    bool PrepareBrowseReviewForTest(int32 View);
    bool PrepareIngredientReviewForTest(int32 View);
    bool PrepareServiceReviewForTest(int32 View, bool bDetails);
#endif
    void SetRecipeBrowse(const FString& Query, int32 Category, bool bNameSort);
    TArray<int32> GetVisibleRecipeIndices() const;
    static int32 GetBuildBrowseGroup(FName Output);
    static FString GetBrowseCategoryLabel(int32 Category);
    static bool CanBuildMenuCraftRecipe(const FKalmalaRecipe& Recipe);
    void EnablePlacementPreview();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UKalmalaCraftingComponent* Model() const;
    UKalmalaCraftingSubsystem* GetLocalCraftingSubsystem() const;
    bool IsRecipeFavorite(FName RecipeId) const;
    bool IsRecipeRecent(FName RecipeId) const;
    void RememberMenuBrowseState();
    bool RestoreMenuBrowseState(FName StationKit);
    bool IsInventoryInspectionFocused() const;
    void RememberInventoryInspectionState();
    void RestoreInventoryInspectionState();
    bool OpenInternal(FName StationKit, AKalmalaConstructionActor* StationActor = nullptr,
        FString StationContextConstructionId = FString(), bool bEmbeddedContext = false,
        FString StationContextSection = FString());
    void ConfigureStationContextPresentation(const FString& Section);
    void ApplyStationCraftLayout();
    void RefreshStationContextState();
    void RefreshForgeUpgradeState(UKalmalaCraftingComponent* Crafting);
    void RefreshWorkbenchRepairState(UKalmalaCraftingComponent* Crafting);
    UFUNCTION() void SelectWorkbenchCraftSection();
    UFUNCTION() void SelectWorkbenchRepairSection();
    UFUNCTION() void SelectForgeCraftSection();
    UFUNCTION() void SelectForgeUpgradeSection();
    UFUNCTION() void SelectForgeRepairSection();
    UFUNCTION() void RecipeSearchChanged(const FText& Text);
    UFUNCTION() void CycleRecipeCategory();
    UFUNCTION() void CycleRecipeSort();
    UFUNCTION() void ClearRecipeSearch();
    UFUNCTION() void ToggleSelectedFavorite();
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
    UFUNCTION() void RepairWorkbenchSelectedTool();
    UFUNCTION() void CraftBronzeAxe();
    UFUNCTION() void UpgradeIronAxe();
    UFUNCTION() void InspectStorage();
    UFUNCTION() void PreviousStorageItem();
    UFUNCTION() void NextStorageItem();
    UFUNCTION() void DepositStorage();
    UFUNCTION() void WithdrawStorage();
    UFUNCTION() void CloseClicked();
    void Refresh();
    void RefreshInlineToolUpgradeComparison(const AKalmalaCharacter* Character,
        int32 ContrastMode);
    void RefreshRecipeGrid(const TArray<int32>& VisibleIndices, UKalmalaCraftingComponent* Crafting,
        int32 TextScalePercent, int32 ContrastMode);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipesText;
    UPROPERTY(Transient) TObjectPtr<UBorder> MenuBackground;
    UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> RecipeGrid;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> CraftingScrollBox;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> RecipeSlotCards;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> RecipeSlotFavoriteFrames;
    UPROPERTY(Transient) TArray<TObjectPtr<class UKalmalaIconWidget>> RecipeSlotIcons;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotNames;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotStates;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotFavoriteMarkers;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotRankMarkers;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RecipeSlotRecentMarkers;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> RecipeSlotRecentBadgeFrames;
    UPROPERTY(Transient) TArray<uint8> RecipeSlotVisualStates;
    UPROPERTY(Transient) TObjectPtr<UKalmalaSelectedResultWidget> SelectedResultPreview;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> SelectedIcon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RequirementText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InstructionsText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StationContextStatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkbenchRepairContextText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkbenchRepairStatusText;
    UPROPERTY(Transient) TObjectPtr<UWidget> StationSectionSwitcher;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIngredientWidget> Ingredients;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RepairText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolProgressionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeComparisonTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeLevelComparison;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeConditionComparison;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StorageText;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> StorageContextPanel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StorageContextStatusText;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaInventoryInspectWidget> StoragePackInspector;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaInventoryInspectWidget> StorageContentsInspector;
    UPROPERTY(Transient) TObjectPtr<UButton> StorageStoreButton;
    UPROPERTY(Transient) TObjectPtr<UButton> StorageWithdrawButton;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> WrappedTextBlocks;
    UPROPERTY(Transient) TObjectPtr<UButton> CraftButton;
    UPROPERTY(Transient) TObjectPtr<UButton> PlacementPreviewButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BuildPlacementButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CampfireLightButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CraftBronzeAxeButton;
    UPROPERTY(Transient) TObjectPtr<UButton> UpgradeIronAxeButton;
    UPROPERTY(Transient) TObjectPtr<class USizeBox> UpgradeTargetIconBox;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> UpgradeTargetIcon;
    UPROPERTY(Transient) TObjectPtr<class UHorizontalBox> ToolProgressionActions;
    UPROPERTY(Transient) TObjectPtr<UButton> WorkbenchCraftSectionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> WorkbenchRepairSectionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ForgeCraftSectionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ForgeUpgradeSectionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ForgeRepairSectionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> WorkbenchRepairButton;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaInventoryInspectWidget> WorkbenchRepairInspector;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> WorkbenchRepairExcludedWidgets;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> StationCraftExcludedWidgets;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> CookingRackExcludedWidgets;
    UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> BuildExcludedWidgets;
    UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> RecipeSearchBox;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipeCategoryLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipeSortLabel;
    UPROPERTY(Transient) TObjectPtr<UButton> FavoriteButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FavoriteActionLabel;
    UPROPERTY(Transient) FEditableTextBoxStyle RecipeSearchStyle;
    FString RecipeQuery;
    int32 RecipeCategory = 3;
    bool bRecipeNameSort = false;
    TMap<FName, FKalmalaMenuBrowseMemory> MenuBrowseMemory;
    FKalmalaInventoryInspectionMemory InventoryInspectionMemory;
    bool bPendingMenuScrollRestore = false;
    float PendingMenuScrollRestoreOffset = 0.0f;
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
    bool bEmbeddedContext = false;
    bool bInventoryCompanion = false;
    bool bCookingRackContext = false;
    bool bCauldronContext = false;
    bool bFryingPanContext = false;
    bool bWorkbenchCraftContext = false;
    bool bForgeCraftContext = false;
    bool bForgeUpgradeContext = false;
    bool bForgeRepairContext = false;
    bool bWorkbenchRepairContext = false;
    bool bStorageContext = false;
    bool bWorkbenchRepairPending = false;
#if !UE_BUILD_SHIPPING
    uint32 WorkbenchRepairRequestCountForTest = 0;
    uint32 ForgeUpgradeRequestCountForTest = 0;
    uint32 CookingRackCraftRequestCountForTest = 0;
    uint32 CauldronCraftRequestCountForTest = 0;
    uint32 FryingPanCraftRequestCountForTest = 0;
    uint32 StorageTransferRequestCountForTest = 0;
#endif
    bool bPreviousMoveInputIgnored = false;
    bool bPreviousLookInputIgnored = false;
    bool bPlacementPreviewEnabled = false;
    bool bPreviousCursor = false;
    TWeakObjectPtr<AKalmalaConstructionActor> ContextStationActor;
    TWeakObjectPtr<APawn> ContextOwnerPawn;
    FString ContextConstructionId;
    FString StationContextSection;
    FString WorkbenchRepairResultText;
    FName WorkbenchRepairPendingToolId = NAME_None;
    FName WorkbenchRepairResultToolId = NAME_None;
    uint32 WorkbenchRepairResultSerial = 0;
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
    bool IsOpen() const;
    bool IsRecipeFavorite(FName RecipeId) const { return FavoriteRecipeIds.Contains(RecipeId); }
    bool CanFavoriteRecipe(FName RecipeId) const;
    bool SetRecipeFavorite(FName RecipeId, bool bFavorite);
    void PruneRecipeFavorites();
    uint32 GetRecipeActivityCount(EKalmalaCraftingActionKind Kind, FName RecipeId) const;
    TMap<FName, int32> GetRecipeActivityRanks(EKalmalaCraftingActionKind Kind) const;
    int32 GetRecipeActivityRank(EKalmalaCraftingActionKind Kind, FName RecipeId) const;
    FName GetRecentRecipeActivity(EKalmalaCraftingActionKind Kind) const;
    void PruneRecipeActivity();
#if !UE_BUILD_SHIPPING
    void ResetRecipeActivityForTest();
    void SetRecipeActivityForTest(EKalmalaCraftingActionKind Kind, FName RecipeId, uint32 Count, bool bRecent);
#endif
#if WITH_DEV_AUTOMATION_TESTS
    void ObserveRecipeActivityForTest(UKalmalaCraftingComponent* Crafting) { ObserveRecipeActivity(Crafting); }
#endif
private:
    void Toggle();
    void Release();
    void ObserveRecipeActivity(UKalmalaCraftingComponent* Crafting);
    void RecordAcceptedRecipeActivity(const FKalmalaAcceptedCraftingActionReceipt& Receipt);
    void UpdateInteractionPrompt(APlayerController* PlayerController);
#if !UE_BUILD_SHIPPING
    void UpdateInteractionPromptReview(APlayerController* PlayerController, float DeltaTime);
#endif
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> Widget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaStationContextWidget> StationContextWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaInteractionPromptWidget> InteractionPrompt;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TSet<FName> FavoriteRecipeIds;
    UPROPERTY(Transient) TMap<FName, uint32> BuiltPieceCounts;
    UPROPERTY(Transient) TMap<FName, uint32> CookedRecipeCounts;
    UPROPERTY(Transient) TMap<FName, uint32> CraftedItemCounts;
    TWeakObjectPtr<UKalmalaCraftingComponent> ActivityCraftingComponent;
    uint64 LastAcceptedRecipeActivitySequence = 0;
    bool bHasObservedRecipeActivityComponent = false;
    FName RecentBuiltPieceRecipeId;
    FName RecentCookedRecipeId;
    FName RecentCraftedItemRecipeId;
    TWeakObjectPtr<UKalmalaCraftingComponent> StationInteractionModel;
    TWeakObjectPtr<UInputComponent> BoundInput;
    uint32 LastStationInteractionSerial = 0;
    bool bHasSeenStationInteraction = false;
    uint32 LastStationContextSerial = 0;
    bool bHasSeenStationContext = false;
    bool bVerified = false;
    bool bCaptureRequested = false;
    int32 ReviewCaptureStage = 0;
    float CaptureWait = 0;
    float VerificationLayoutWait = 0;
#if !UE_BUILD_SHIPPING
    int32 InteractionPromptReviewStage = 0;
    float InteractionPromptReviewWait = 0.0f;
    bool bInteractionPromptReviewComplete = false;
#endif
};
