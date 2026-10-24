#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaCraftingComponent.generated.h"

UENUM()
enum class EKalmalaCraftingActionKind : uint8
{
    BuiltPiece,
    CookedRecipe,
    CraftedItem
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaAcceptedCraftingActionReceipt
{
    GENERATED_BODY()

    UPROPERTY() uint64 Sequence = 0;
    UPROPERTY() FName RecipeId = NAME_None;
    UPROPERTY() EKalmalaCraftingActionKind Kind = EKalmalaCraftingActionKind::CraftedItem;
};

class AKalmalaCampfire;
class AKalmalaCharacter;
class AKalmalaConstructionActor;
class AKalmalaWildlifeSpawn;
struct FKalmalaRecipe;

/** Owner intent and read-only presentation seam. Server recomputes all costs and targets. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaCraftingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    static constexpr int32 MaxAcceptedCraftingActionReceipts = 64;
    UKalmalaCraftingComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UFUNCTION(Server, Reliable) void ServerCraft(FName RecipeId, int32 Batch);
    UFUNCTION(Server, Reliable) void ServerPlaceCampfire();
    UFUNCTION(Server, Reliable) void ServerPlaceConstruction(FName BuildableId);
    UFUNCTION(Server, Reliable) void ServerRefuel();
    UFUNCTION(Server, Reliable) void ServerLight();
    UFUNCTION(Server, Reliable) void ServerRepairTool(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerRefuelTorch(FGuid ToolInstanceGuid);
    UFUNCTION(Server, Reliable) void ServerRepairAllTools();
    UFUNCTION(Server, Reliable) void ServerProgressTool(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerConsumeFood(FName FoodItemId);
    UFUNCTION(Server, Reliable) void ServerOpenStorage();
    UFUNCTION(Server, Reliable) void ServerCloseStorage();
    UFUNCTION(Server, Reliable) void ServerDepositStorage(FGuid ItemGuid);
    UFUNCTION(Server, Reliable) void ServerWithdrawStorage(FGuid ItemGuid);
    UFUNCTION(Server, Reliable) void ServerLoadCharcoalKilnWood();
    UFUNCTION(Server, Reliable) void ServerCollectCharcoalKilnCoal();
    UFUNCTION(Server, Reliable) void ServerLoadSmelterItem(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerCollectSmelterItem(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerPlantGardenCarrot(AKalmalaConstructionActor* Plot, EKalmalaGardenCropMode Mode);
    UFUNCTION(Server, Reliable) void ServerHarvestGardenCrop(AKalmalaConstructionActor* Plot);
    void InteractWithConstructionFromServer(AKalmalaConstructionActor* Construction);
    FName GetLastInteractedCookingStationKit() const { return LastInteractedCookingStationKit; }
    uint32 GetCookingStationInteractionSerial() const { return CookingStationInteractionSerial; }
    AKalmalaConstructionActor* GetLastStationContextActor() const { return LastStationContextActor; }
    FName GetLastStationContextKit() const { return LastStationContextKit; }
    const FString& GetLastStationContextConstructionId() const { return LastStationContextConstructionId; }
    uint32 GetStationContextInteractionSerial() const { return StationContextInteractionSerial; }
    /** Owner-local advisory check for the exact server-accepted station context. */
    bool IsStationContextTargetCurrent(AKalmalaConstructionActor* ExpectedActor, FName ExpectedKit,
        const FString& ExpectedConstructionId) const;
    bool IsStationContextUsable(AKalmalaConstructionActor* ExpectedActor, FName ExpectedKit,
        const FString& ExpectedConstructionId) const;
    bool OpenStorageFromServer(AKalmalaConstructionActor* Construction);
    bool TransferStorageFromServer(FGuid ItemGuid, bool bDeposit, FString& Reason);
    bool LoadCharcoalKilnWoodFromServer(FString& Reason);
    bool CollectCharcoalKilnCoalFromServer(FString& Reason);
    bool LoadSmelterItemFromServer(FName ItemId, FString& Reason);
    bool CollectSmelterItemFromServer(FName ItemId, FString& Reason);
    bool PlantGardenCropFromServer(AKalmalaConstructionActor* Plot, EKalmalaGardenCropMode Mode, FString& Reason);
    bool HarvestGardenCropFromServer(AKalmalaConstructionActor* Plot, FString& Reason);
    const TArray<FKalmalaInventoryStack>& GetStorageView() const { return StorageView; }
    bool HasStorageView() const { return bStorageViewOpen; }
    uint32 GetResultSerial() const { return ResultSerial; }
    bool WasLastResultAccepted() const { return bLastResultAccepted; }
    void PublishInteractionResultFromServer(const FString& Result, bool bAccepted);
    const TArray<FKalmalaAcceptedCraftingActionReceipt>& GetAcceptedCraftingActionReceipts() const
    {
        return AcceptedCraftingActionReceipts;
    }
#if WITH_DEV_AUTOMATION_TESTS
    void PublishResultForTest(const FString& Result, bool bAccepted, FName RecipeId,
        EKalmalaCraftingActionKind Kind)
    {
        PublishResult(Result, bAccepted, RecipeId, Kind);
    }
#endif
    AKalmalaConstructionActor* FindNearbyWorkbench() const;
    FString GetNearbyWorkbenchText() const;
    FString GetNearbyConstructionText() const;
    bool CraftFromServer(FName RecipeId, int32 Batch, FString& Reason);
    bool RepairToolFromServer(FName ItemId, FString& Reason);
    bool RefuelTorchFromServer(FGuid ToolInstanceGuid, FString& Reason);
    bool RepairAllToolsAtStationFromServer(FString& Reason);
    bool RepairAllToolsFromServer(AKalmalaConstructionActor* GrindingStone, FString& Reason);
    bool ProgressToolFromServer(FName ItemId, FString& Reason);
    bool ConsumeFoodFromServer(FName FoodItemId, FString& Reason);
    bool PlaceFromServer(FString& Reason);
    bool PlaceConstructionFromServer(FName BuildableId, FString& Reason);
    FString GetRecipeDescription(FName RecipeId) const;
    FString GetRecipeAvailability(FName RecipeId) const;
    FString GetToolProgressionText(FName StationFilterKit = NAME_None) const;
    FString GetFoodText() const;
    FString GetNearbyFireText() const;
    const FString& GetLastResult() const { return LastResult; }
    AKalmalaCampfire* FindNearbyFire(bool bRequireUsable) const;
private:
    bool AcceptRequest();
    bool CommitRepairAllToolsFromServer(const TCHAR* StationName, FString& Reason);
    void PublishResult(const FString& Result, bool bAccepted, FName AcceptedRecipeId = NAME_None,
        EKalmalaCraftingActionKind AcceptedActionKind = EKalmalaCraftingActionKind::CraftedItem);
    AKalmalaCharacter* GetCharacter() const;
    AKalmalaCampfire* FindNearbyLitFire(const AKalmalaConstructionActor* RequiredStation = nullptr) const;
    void RunVerification(float DeltaTime);
    void RunStorageVerification(float DeltaTime);
    void RunPersistedCampVerification(float DeltaTime);
    void RunRainVerticalSliceVerification(float DeltaTime);
#if !UE_BUILD_SHIPPING
    void RunGardenPlotVerification(float DeltaTime);
    void RunA8bTonicPeerVerification(float DeltaTime);
    void RunA8bVestPeerVerification(float DeltaTime);
    void RunA8bRoastedBadgerPeerVerification(float DeltaTime);
    void RunA8bFangArrowsPeerVerification(float DeltaTime);
    void RunA8bScaleCuirassPeerVerification(float DeltaTime);
    void RunA8bAntidotePeerVerification(float DeltaTime);
    void RunA8bPeatHeartStewPeerVerification(float DeltaTime);
    void RunA8bWolfPeltBootsPeerVerification(float DeltaTime);
    void RunA8bFangSpearPeerVerification(float DeltaTime);
#endif
    void RefreshStorageView();
    void ClearStorageView();
    AKalmalaConstructionActor* GetCurrentCharcoalKilnFromServer() const;
    AKalmalaConstructionActor* GetCurrentSmelterFromServer() const;
    AKalmalaConstructionActor* FindNearbyConstruction(FName Kit, FName AlternateKit = NAME_None) const;
    AKalmalaConstructionActor* FindNearbyConstruction(const TArray<FName>& RequiredStations) const;
    AKalmalaConstructionActor* FindNearbyToolProgressionStation(FName Kit) const;
    TWeakObjectPtr<AKalmalaConstructionActor> ActiveStorage;
    UPROPERTY(Replicated) TArray<FKalmalaInventoryStack> StorageView;
    UPROPERTY(Replicated) bool bStorageViewOpen = false;
    UPROPERTY(Replicated) FName LastInteractedCookingStationKit;
    UPROPERTY(Replicated) uint32 CookingStationInteractionSerial = 0;
    UPROPERTY(Replicated) TObjectPtr<AKalmalaConstructionActor> LastStationContextActor = nullptr;
    UPROPERTY(Replicated) FName LastStationContextKit;
    UPROPERTY(Replicated) FString LastStationContextConstructionId;
    UPROPERTY(Replicated) uint32 StationContextInteractionSerial = 0;
    int32 StorageVerificationStage = 0;
    float StorageVerificationElapsed = 0;
    double NextRequestTime = 0;
    UPROPERTY(Replicated) FString LastResult;
    UPROPERTY(Replicated) uint32 ResultSerial = 0;
    UPROPERTY(Replicated) bool bLastResultAccepted = false;
    UPROPERTY(Replicated) TArray<FKalmalaAcceptedCraftingActionReceipt> AcceptedCraftingActionReceipts;
    uint64 NextAcceptedCraftingActionSequence = 0;
    int32 VerificationStage = 0;
    float VerificationElapsed = 0;
    int32 LocalVerificationStage = 0;
    float LocalVerificationElapsed = 0;
    bool bVerificationPassed = true;
    UPROPERTY(Transient) TObjectPtr<AKalmalaCampfire> VerificationFire;
    int32 PersistedCampVerificationStage = 0;
    float PersistedCampVerificationElapsed = 0.0f;
    float PersistedCampClientDiagnosticElapsed = 0.0f;
    bool bPersistedCampOwnerReported = false;
    bool bPersistedCampAuthorityProbeReported = false;
    bool bRainVerticalSliceClientReported = false;
    float RainVerticalSliceClientObservationSeconds = 0.0f;
#if !UE_BUILD_SHIPPING
    int32 GardenPlotVerificationStage = 0;
    float GardenPlotVerificationElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaConstructionActor> GardenPlotVerificationActor;
    FGuid GardenPlotVerificationTrowelGuid;
    int32 GardenPlotVerificationStartingCarrotSeeds = 0;
    int32 GardenPlotVerificationStartingTrowelCondition = 0;
    uint32 GardenPlotVerificationStartingResultSerial = 0;
    TArray<FKalmalaInventoryStack> GardenPlotVerificationPlacementCost;
    TArray<int32> GardenPlotVerificationCostBefore;
    int32 A8bTonicPeerStage = 0;
    float A8bTonicPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bTonicPeerTarget;
    uint32 A8bTonicPeerReadyResultSerial = 0;
    bool bA8bTonicPeerFixtureObserved = false;
    int32 A8bVestPeerStage = 0;
    float A8bVestPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bVestPeerTarget;
    FGuid A8bVestPeerItemGuid;
    uint32 A8bVestPeerReadyResultSerial = 0;
    uint32 A8bVestPeerGearRequestSerial = 0;
    float A8bVestPeerBaselineHealth = 100.0f;
    int32 A8bVestPeerObservedOwnerPlayerId = INDEX_NONE;
    bool bA8bVestPeerFixtureObserved = false;
    int32 A8bRoastedBadgerPeerStage = 0;
    float A8bRoastedBadgerPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bRoastedBadgerPeerTarget;
    uint32 A8bRoastedBadgerPeerResultSerial = 0;
    float A8bRoastedBadgerPeerBaselineStaminaCostAddition = 0.0f;
    float A8bRoastedBadgerPeerMealSecondsAfterUse = 0.0f;
    int32 A8bRoastedBadgerPeerObservedOwnerPlayerId = INDEX_NONE;
    bool bA8bRoastedBadgerPeerFixtureObserved = false;
    int32 A8bFangArrowsPeerStage = 0;
    float A8bFangArrowsPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bFangArrowsPeerTarget;
    TWeakObjectPtr<AKalmalaWildlifeSpawn> A8bFangArrowsPeerWildlife;
    FGuid A8bFangArrowsPeerBowGuid;
    uint32 A8bFangArrowsPeerResultSerial = 0;
    int32 A8bFangArrowsPeerBaselineViperFang = 0;
    int32 A8bFangArrowsPeerBaselineWood = 0;
    int32 A8bFangArrowsPeerBaselineFangArrows = 0;
    int32 A8bFangArrowsPeerHostBaselineViperFang = 0;
    int32 A8bFangArrowsPeerHostBaselineFangArrows = 0;
    int32 A8bFangArrowsPeerBaselineBowCondition = 0;
    float A8bFangArrowsPeerBaselineWildlifeHealth = 0.0f;
    float A8bFangArrowsPeerHealthAfterShot = 0.0f;
    int32 A8bFangArrowsPeerObservedOwnerPlayerId = INDEX_NONE;
    float A8bFangArrowsPeerObserverBaselineHealth = 0.0f;
    bool bA8bFangArrowsPeerFixtureObserved = false;
    int32 A8bScaleCuirassPeerStage = 0;
    float A8bScaleCuirassPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bScaleCuirassPeerTarget;
    FGuid A8bScaleCuirassPeerItemGuid;
    uint32 A8bScaleCuirassPeerResultSerial = 0;
    uint32 A8bScaleCuirassPeerGearRequestSerial = 0;
    int32 A8bScaleCuirassPeerBaselineGiantScale = 0;
    int32 A8bScaleCuirassPeerBaselineBronze = 0;
    int32 A8bScaleCuirassPeerBaselineCuirass = 0;
    int32 A8bScaleCuirassPeerHostBaselineGiantScale = 0;
    int32 A8bScaleCuirassPeerHostBaselineBronze = 0;
    int32 A8bScaleCuirassPeerHostBaselineCuirass = 0;
    float A8bScaleCuirassPeerBaselineHealth = 0.0f;
    int32 A8bScaleCuirassPeerObservedOwnerPlayerId = INDEX_NONE;
    float A8bScaleCuirassPeerObserverBaselineHealth = 0.0f;
    bool bA8bScaleCuirassPeerFixtureObserved = false;
    int32 A8bAntidotePeerStage = 0;
    float A8bAntidotePeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bAntidotePeerTarget;
    uint32 A8bAntidotePeerResultSerial = 0;
    int32 A8bAntidotePeerBaselineVenomSac = 0;
    int32 A8bAntidotePeerBaselineLingonberry = 0;
    int32 A8bAntidotePeerBaselineDose = 0;
    int32 A8bAntidotePeerHostBaselineVenomSac = 0;
    int32 A8bAntidotePeerHostBaselineLingonberry = 0;
    int32 A8bAntidotePeerHostBaselineDose = 0;
    int32 A8bAntidotePeerObservedOwnerPlayerId = INDEX_NONE;
    float A8bAntidotePeerObserverBaselineHealth = 100.0f;
    bool bA8bAntidotePeerFixtureObserved = false;
    int32 A8bPeatHeartStewPeerStage = 0;
    float A8bPeatHeartStewPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bPeatHeartStewPeerTarget;
    uint32 A8bPeatHeartStewPeerResultSerial = 0;
    int32 A8bPeatHeartStewPeerBaselinePeatHeart = 0;
    int32 A8bPeatHeartStewPeerBaselinePotato = 0;
    int32 A8bPeatHeartStewPeerBaselineOnion = 0;
    int32 A8bPeatHeartStewPeerBaselineStew = 0;
    int32 A8bPeatHeartStewPeerHostBaselinePeatHeart = 0;
    int32 A8bPeatHeartStewPeerHostBaselinePotato = 0;
    int32 A8bPeatHeartStewPeerHostBaselineOnion = 0;
    int32 A8bPeatHeartStewPeerHostBaselineStew = 0;
    float A8bPeatHeartStewPeerBaselineStaminaCostAddition = 0.0f;
    float A8bPeatHeartStewPeerMealSecondsAfterUse = 0.0f;
    int32 A8bPeatHeartStewPeerObservedOwnerPlayerId = INDEX_NONE;
    bool bA8bPeatHeartStewPeerFixtureObserved = false;
    bool bA8bPeatHeartStewPeerObserverDiscoveryLogged = false;
    int32 A8bWolfPeltBootsPeerStage = 0;
    float A8bWolfPeltBootsPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bWolfPeltBootsPeerTarget;
    FGuid A8bWolfPeltBootsPeerItemGuid;
    uint32 A8bWolfPeltBootsPeerResultSerial = 0;
    uint32 A8bWolfPeltBootsPeerGearRequestSerial = 0;
    int32 A8bWolfPeltBootsPeerBaselineWolfPelt = 0;
    int32 A8bWolfPeltBootsPeerBaselineIron = 0;
    int32 A8bWolfPeltBootsPeerBaselineBoots = 0;
    int32 A8bWolfPeltBootsPeerHostBaselineWolfPelt = 0;
    int32 A8bWolfPeltBootsPeerHostBaselineIron = 0;
    int32 A8bWolfPeltBootsPeerHostBaselineBoots = 0;
    float A8bWolfPeltBootsPeerBaselineHealth = 0.0f;
    int32 A8bWolfPeltBootsPeerObservedOwnerPlayerId = INDEX_NONE;
    float A8bWolfPeltBootsPeerObserverBaselineHealth = 100.0f;
    bool bA8bWolfPeltBootsPeerFixtureObserved = false;
    int32 A8bFangSpearPeerStage = 0;
    float A8bFangSpearPeerElapsed = 0.0f;
    TWeakObjectPtr<AKalmalaCharacter> A8bFangSpearPeerTarget;
    TWeakObjectPtr<AKalmalaWildlifeSpawn> A8bFangSpearPeerWildlife;
    FGuid A8bFangSpearPeerItemGuid;
    uint32 A8bFangSpearPeerResultSerial = 0;
    uint32 A8bFangSpearPeerGearRequestSerial = 0;
    uint32 A8bFangSpearPeerBaselineActionSerial = 0;
    int32 A8bFangSpearPeerBaselineWolfFang = 0;
    int32 A8bFangSpearPeerBaselineIron = 0;
    int32 A8bFangSpearPeerBaselineDensewood = 0;
    int32 A8bFangSpearPeerBaselineSpear = 0;
    int32 A8bFangSpearPeerHostBaselineWolfFang = 0;
    int32 A8bFangSpearPeerHostBaselineIron = 0;
    int32 A8bFangSpearPeerHostBaselineDensewood = 0;
    int32 A8bFangSpearPeerHostBaselineSpear = 0;
    int32 A8bFangSpearPeerHostBaselineTools = 0;
    float A8bFangSpearPeerBaselineWildlifeHealth = 0.0f;
    int32 A8bFangSpearPeerObservedOwnerPlayerId = INDEX_NONE;
    bool bA8bFangSpearPeerFixtureObserved = false;
#endif
};
