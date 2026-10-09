#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.generated.h"

USTRUCT()
struct FKalmalaInventoryStack
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName ItemId;
    UPROPERTY(EditAnywhere) int32 Quantity = 0;
};

/** Pawn-lifetime inventory. Only trusted server gameplay may mutate its contents. */
struct FKalmalaItemGainReceipt
{
    int64 Sequence = 0;
    FName ItemId;
    int32 Quantity = 0;
};

UCLASS()
class KALMALAGAMEPLAY_API UKalmalaInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaInventoryComponent();
    static constexpr int32 Columns = 10;
    static constexpr int32 Rows = 4;
    static constexpr int32 MaxSlots = Columns * Rows;
    const TArray<FName>& GetGridSlots() const { return GridSlots; }
    FName GetSlotItem(int32 Slot) const;
    FName GetActiveItem() const { return ActiveItem; }
    bool OwnsGridItem(FName ItemId) const;
    float GetCarriedWeight() const;
    float GetCarryCapacity() const { return CarryCapacity; }
    bool CanFitContents(const TArray<FKalmalaInventoryStack>& Items, int32 ToolCount) const;
    /** Stable, gap-preserving placement; tools and stacks share these same cells. */
    static bool BuildGridLayout(const TArray<FName>& Owned, const TArray<FName>& Before, TArray<FName>& After);
    void SynchronizeGridFromServer();
    /** Retain a cell when a trusted tool-upgrade transaction replaces its identity. */
    void ReplaceGridItemFromServer(FName Previous, FName Replacement);
    bool MoveSlotFromServer(int32 Source, int32 Target, FName ExpectedSource, FName ExpectedTarget);
    UFUNCTION(Server, Reliable) void ServerMoveSlot(int32 Source, int32 Target, FName ExpectedSource, FName ExpectedTarget);
    UFUNCTION(Server, Reliable) void ServerUseHotbarSlot(int32 Slot);
    const TArray<FKalmalaInventoryStack>& GetStacks() const { return Stacks; }
    /** Local owner receipt buffer, never used as inventory authority or persistence. */
    const TArray<FKalmalaItemGainReceipt>& GetGainReceipts() const { return GainReceipts; }
    static constexpr int32 MaxGainReceipts = 32;
    int32 GetQuantity(FName ItemId) const;
    bool TryGrantFromServer(FName ItemId, int32 Quantity);
    bool TryConsumeFromServer(FName ItemId, int32 Quantity);
    /** Validate in a scratch array and publish once; no partial ingredient removal. */
    static bool BuildExchange(const TArray<FKalmalaInventoryStack>& Before,
        const TArray<FKalmalaInventoryStack>& Costs, FName Output, int32 OutputCount,
        TArray<FKalmalaInventoryStack>& After, FString& Reason);
    /** Build a catalogue-validated grant without mutating the live pack. */
    static bool BuildGrant(const TArray<FKalmalaInventoryStack>& Before, FName ItemId, int32 Quantity,
        TArray<FKalmalaInventoryStack>& After, FString& Reason);
    /** Publish a previously validated candidate only if the live pack still matches its snapshot. */
    bool TryCommitStacksFromServer(const TArray<FKalmalaInventoryStack>& ExpectedBefore,
        const TArray<FKalmalaInventoryStack>& CandidateAfter);
    bool TryExchangeFromServer(const TArray<FKalmalaInventoryStack>& Costs,
        FName Output, int32 OutputCount, FString& Reason);
    static bool BuildTransfer(const TArray<FKalmalaInventoryStack>& Source,
        const TArray<FKalmalaInventoryStack>& Destination, FName ItemId, int32 Quantity,
        TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason);
    /** Persist the scratch chest before publishing this pack; failed validation/writes mutate neither. */
    bool TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage, FName ItemId, bool bDeposit,
        TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
private:
    void RecordAcceptedGains(const TArray<FKalmalaInventoryStack>& Before,
        const TArray<FKalmalaInventoryStack>& After);
    UFUNCTION(Client, Reliable) void ClientAcceptedGain(FName ItemId, int32 Quantity);
    TArray<FKalmalaItemGainReceipt> GainReceipts;
    int64 GainSequence = 0;
    UPROPERTY(Replicated) TArray<FKalmalaInventoryStack> Stacks;
    UPROPERTY(Replicated) TArray<FName> GridSlots;
    UPROPERTY(Replicated) FName ActiveItem;
    UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1.0")) float CarryCapacity = 300.0f;
    double LastLayoutRequestTime = -1.0;
    double LastHotbarRequestTime = -1.0;
    bool bVerificationComplete = false;
#if !UE_BUILD_SHIPPING
    void TickMenuReview(float DeltaTime);
    float MenuReviewElapsed = 0.0f;
    int32 MenuReviewStage = 0;
#endif
};
