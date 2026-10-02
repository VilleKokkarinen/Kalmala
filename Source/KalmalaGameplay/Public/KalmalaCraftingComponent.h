#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaCraftingComponent.generated.h"

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
    FName GetLookedAtCookingStationKit() const;
    bool OpenStorageFromServer(AKalmalaConstructionActor* Construction);
    bool TransferStorageFromServer(FName ItemId, bool bDeposit, FString& Reason);
    const TArray<FKalmalaInventoryStack>& GetStorageView() const { return StorageView; }
    bool HasStorageView() const { return bStorageViewOpen; }
    uint32 GetResultSerial() const { return ResultSerial; }
    bool WasLastResultAccepted() const { return bLastResultAccepted; }
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
    FString GetToolProgressionText() const;
    FString GetFoodText() const;
    FString GetNearbyFireText() const;
    const FString& GetLastResult() const { return LastResult; }
    AKalmalaCampfire* FindNearbyFire(bool bRequireUsable) const;
private:
    bool AcceptRequest();
    void PublishResult(const FString& Result, bool bAccepted);
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
    int32 StorageVerificationStage = 0;
    float StorageVerificationElapsed = 0;
    double NextRequestTime = 0;
    UPROPERTY(Replicated) FString LastResult;
    UPROPERTY(Replicated) uint32 ResultSerial = 0;
    UPROPERTY(Replicated) bool bLastResultAccepted = false;
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
