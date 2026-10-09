#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.h"
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
    UFUNCTION(Server, Reliable) void ServerRepairTool(FName ToolId);
    UFUNCTION(Server, Reliable) void ServerProgressTool(FName ToolId);
    UFUNCTION(Server, Reliable) void ServerConsumeFood(FName FoodItemId);
    UFUNCTION(Server, Reliable) void ServerOpenStorage();
    UFUNCTION(Server, Reliable) void ServerCloseStorage();
    UFUNCTION(Server, Reliable) void ServerDepositStorage(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerWithdrawStorage(FName ItemId);
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
    bool TransferStorageFromServer(FName ItemId, bool bDeposit, FString& Reason);
    const TArray<FKalmalaInventoryStack>& GetStorageView() const { return StorageView; }
    bool HasStorageView() const { return bStorageViewOpen; }
    uint32 GetResultSerial() const { return ResultSerial; }
    bool WasLastResultAccepted() const { return bLastResultAccepted; }
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
    bool RepairToolFromServer(FName ToolId, FString& Reason);
    bool RepairAllToolsFromServer(AKalmalaConstructionActor* GrindingStone, FString& Reason);
    bool ProgressToolFromServer(FName ToolId, FString& Reason);
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
    void PublishResult(const FString& Result, bool bAccepted, FName AcceptedRecipeId = NAME_None,
        EKalmalaCraftingActionKind AcceptedActionKind = EKalmalaCraftingActionKind::CraftedItem);
    AKalmalaCharacter* GetCharacter() const;
    AKalmalaCampfire* FindNearbyLitFire(const AKalmalaConstructionActor* RequiredStation = nullptr) const;
    void RunVerification(float DeltaTime);
    void RunStorageVerification(float DeltaTime);
    void RunPersistedCampVerification(float DeltaTime);
    void RunRainVerticalSliceVerification(float DeltaTime);
    void RefreshStorageView();
    void ClearStorageView();
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
};
