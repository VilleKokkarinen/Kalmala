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
class UHorizontalBox;
class UVerticalBox;
class UUniformGridPanel;
class UScrollBox;
class UInputComponent;
class UKalmalaCraftingComponent;
class UKalmalaInventoryComponent;
class UKalmalaSelectedResultWidget;
class UKalmalaIconWidget;
class AKalmalaCharacter;
struct FKalmalaInventoryStack;

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
class KALMALAUI_API UKalmalaBowCrosshairWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override;
};

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
    bool VerifyInventoryCraftLayoutForTest(FString& OutFailureReason);
    void PrepareMenuRender(bool bPlayerCrafting, bool bWorkbenchCrafting = false,
        bool bForgeCrafting = false, const FString& StationSection = TEXT("Craft"));
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
    static bool CanInventoryMenuCraftRecipe(const FKalmalaRecipe& Recipe);
    static FVector2D GetInventoryCraftPanelSize(FVector2D Viewport);
    void EnablePlacementPreview();
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
private:
    void BuildInventoryCraftLayout();
    void RefreshInventoryCraftLayout();
    void RefreshBuildRequirements(const FString& Title,
        const TArray<FKalmalaInventoryStack>& Costs, const UKalmalaInventoryComponent* Inventory,
        const FString& Details, int32 TextScalePercent, int32 ContrastMode);
    UFUNCTION() void SelectInventoryCraftRecipe();
    UPROPERTY(Transient) TObjectPtr<UBorder> InventoryCraftBackground;
    UPROPERTY(Transient) TObjectPtr<UBorder> StationServiceBackground;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> StationServiceContent;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> BuildMenuStack;
    UPROPERTY(Transient) TObjectPtr<class USizeBox> BuildRequirementsSizeBox;
    UPROPERTY(Transient) TObjectPtr<UBorder> BuildRequirementsPanel;
    UPROPERTY(Transient) TObjectPtr<UBorder> BuildRequirementsFrame;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BuildRequirementsTitle;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> BuildRequirementsCosts;
    UPROPERTY(Transient) TObjectPtr<UButton> BuildCloseButton;
    UPROPERTY(Transient) TObjectPtr<UBorder> InventoryCraftListFrame;
    UPROPERTY(Transient) TObjectPtr<UBorder> InventoryCraftDetailFrame;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> InventoryCraftList;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> InventoryCraftCosts;
    UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> InventoryCraftSearch;
    UPROPERTY(Transient) TObjectPtr<UButton> InventoryCraftClear;
    UPROPERTY(Transient) TObjectPtr<UButton> InventoryCraftButton;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> InventoryCraftIcon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryCraftTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryCraftDescription;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InventoryCraftStats;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> InventoryCraftChoices;
    TArray<int32> InventoryCraftIndices;
    FString InventoryCraftCostSignature;
    bool bUpdatingInventoryCraftSearch = false;
    FString BuildRequirementCostSignature;
    int32 LastBuildRequirementTextScale = INDEX_NONE;
    int32 LastBuildRequirementContrast = INDEX_NONE;
    UKalmalaCraftingComponent* Model() const;
    UKalmalaCraftingSubsystem* GetLocalCraftingSubsystem() const;
    bool IsRecipeFavorite(FName RecipeId) const;
    bool IsRecipeRecent(FName RecipeId) const;
    void RememberMenuBrowseState();
    bool RestoreMenuBrowseState(FName StationKit);
    void RefreshBrowseControlLabels();
    void BeginPlacementForSelectedBuild();
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
    void RefreshMaceCraftState();
    void RefreshStoneCraftState();
    void CraftStoneTool(FName ItemId);
    void RefreshWorkbenchRepairState(UKalmalaCraftingComponent* Crafting);
    void RefreshStationRepairActionState(UKalmalaCraftingComponent* Crafting);
    void RequestStationRepairAll(FName RequiredStationKit);
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
    UFUNCTION() void RepairStoneKnife();
    UFUNCTION() void RepairStoneAxe();
    UFUNCTION() void RepairStonePick();
    UFUNCTION() void RepairBronzeAxe();
    UFUNCTION() void RepairIronAxe();
    UFUNCTION() void RepairWorkbenchSelectedTool();
    UFUNCTION() void CraftBronzeAxe();
    UFUNCTION() void CraftBronzeMace();
    UFUNCTION() void CraftIronMace();
    UFUNCTION() void CraftStoneAxe();
    UFUNCTION() void CraftStoneKnife();
    UFUNCTION() void CraftStonePick();
    UFUNCTION() void CraftStoneSpear();
    UFUNCTION() void CraftStoneMace();
    UFUNCTION() void CraftBow();
    UFUNCTION() void UpgradeBronzeAxe();
    UFUNCTION() void InspectStorage();
    UFUNCTION() void PreviousStorageItem();
    UFUNCTION() void NextStorageItem();
    UFUNCTION() void DepositStorage();
    UFUNCTION() void WithdrawStorage();
    UFUNCTION() void CloseClicked();
    void Refresh();
    void RefreshInlineToolUpgradeComparison(int32 ContrastMode);
    void RefreshRecipeGrid(const TArray<int32>& VisibleIndices, UKalmalaCraftingComponent* Crafting,
        int32 TextScalePercent, int32 ContrastMode);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RecipesText;
    UPROPERTY(Transient) TObjectPtr<UBorder> MenuBackground;
    UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> RecipeGrid;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> BrowseControlsRow;
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
    UPROPERTY(Transient) TObjectPtr<UBorder> SelectedIconTile;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RequirementText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InstructionsText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StationContextStatusText;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> StationHeaderRow;
    UPROPERTY(Transient) TObjectPtr<UKalmalaIconWidget> StationHeaderIcon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StationHeaderNameText;
    UPROPERTY(Transient) TObjectPtr<UBorder> WorkbenchRepairPanel;
    UPROPERTY(Transient) TObjectPtr<UBorder> StationUpgradePanel;
    UPROPERTY(Transient) TObjectPtr<UBorder> UpgradeRequirementsFrame;
    UPROPERTY(Transient) TObjectPtr<UBorder> UpgradeComparisonFrame;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkbenchRepairContextText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WorkbenchRepairStatusText;
    UPROPERTY(Transient) TObjectPtr<UWidget> StationSectionSwitcher;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailText;
    UPROPERTY(Transient) TObjectPtr<class USizeBox> BuildRequiredStationIconBox;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIconWidget> BuildRequiredStationIcon;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaIngredientWidget> Ingredients;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RepairText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolProgressionText;
    UPROPERTY(Transient) TObjectPtr<class UHorizontalBox> ToolUpgradeComparisonHeadingRow;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeComparisonTitle;
    UPROPERTY(Transient) TObjectPtr<class USizeBox> ToolUpgradeLevelIconBox;
    UPROPERTY(Transient) TObjectPtr<UKalmalaIconWidget> ToolUpgradeLevelIcon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeLevelComparison;
    UPROPERTY(Transient) TObjectPtr<class UHorizontalBox> ToolUpgradeBenefitRow;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeBenefitComparison;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolUpgradeBenefitDelta;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> ToolUpgradeAdditionalStatsBox;
    UPROPERTY(Transient) TObjectPtr<class UHorizontalBox> ToolUpgradeMaterialCosts;
    UPROPERTY(Transient) TArray<TObjectPtr<class UHorizontalBox>> ToolUpgradeAdditionalStatRows;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ToolUpgradeAdditionalStatValues;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ToolUpgradeAdditionalStatDeltas;
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
    UPROPERTY(Transient) TObjectPtr<UButton> CraftBronzeMaceButton;
    UPROPERTY(Transient) TObjectPtr<UButton> CraftIronMaceButton;
    UPROPERTY(Transient) TObjectPtr<class UHorizontalBox> MaceCraftActions;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StoneCraftTitleText;
    UPROPERTY(Transient) TObjectPtr<class UVerticalBox> StoneCraftActions;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> StoneCraftButtons;
    TArray<FName> StoneCraftItemIds;
    UPROPERTY(Transient) TObjectPtr<UButton> UpgradeBronzeAxeButton;
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
    UPROPERTY(Transient) TObjectPtr<UButton> RecipeCategoryButton;
    UPROPERTY(Transient) TObjectPtr<UButton> RecipeSortButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ClearSearchButton;
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
    FString ToolUpgradeMaterialCostSignature;
    int32 LastDetailTextScalePercent = INDEX_NONE;
    int32 LastDetailContrastMode = INDEX_NONE;
    int32 LastMenuPanelContext = INDEX_NONE;
    int32 LastRecipeGridTextScalePercent = INDEX_NONE;
    int32 LastRecipeGridContrastMode = INDEX_NONE;
    int32 RecipeGridUnavailableCount = 0;
    int32 RecipeGridSelectedIndex = INDEX_NONE;
    bool bRecipeGridFocused = false;
    bool bLastRecipeGridIconOnlyBuild = false;
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
    bool bStationRepairPending = false;
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
    FName WorkbenchRepairPendingItemId = NAME_None;
    FName WorkbenchRepairResultItemId = NAME_None;
    uint32 WorkbenchRepairResultSerial = 0;
    uint32 StationRepairResultSerial = 0;
    FString StationRepairFeedback;
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
    bool BeginBuildPlacement(FName BuildableId);
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
    UFUNCTION() void OpenBuildFromRightClick();
    void ConfirmBuildPlacementInput();
    void CancelBuildPlacementInput();
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
    UPROPERTY(Transient) TObjectPtr<UKalmalaBowCrosshairWidget> BowCrosshair;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    FName PendingBuildPlacementKit;
    uint64 PendingBuildPlacementStartedFrame = 0;
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
