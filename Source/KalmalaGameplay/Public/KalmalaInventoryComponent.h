#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.generated.h"

struct FKalmalaToolState;
class AKalmalaCharacter;

USTRUCT()
struct FKalmalaInventoryStack
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName ItemId;
    UPROPERTY(EditAnywhere) int32 Quantity = 0;
    /** Stable identity for this stack row; non-stackable copies each occupy a separate row. */
    UPROPERTY(EditAnywhere, SaveGame) FGuid InstanceGuid;
    /** Stable identity for every physical item unit represented by Quantity. */
    UPROPERTY(EditAnywhere, SaveGame) TArray<FGuid> ItemGuids;
};

UENUM(BlueprintType)
enum class EKalmalaGearSlot : uint8
{
    Head,
    Cape,
    Chest,
    Shield,
    Legs,
    Boots
};

/** A wearable item moved out of the grid while it occupies its matching slot. */
USTRUCT()
struct FKalmalaEquippedGear
{
    GENERATED_BODY()
    UPROPERTY() EKalmalaGearSlot Slot = EKalmalaGearSlot::Head;
    UPROPERTY() FName ItemId;
    UPROPERTY() FGuid ItemGuid;
    /** Session condition spent on accepted hits; zero means unworn. */
    UPROPERTY() int32 ConditionSpent = 0;
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
    const TArray<FGuid>& GetGridInstanceGuids() const { return GridInstanceGuids; }
    FName GetSlotItem(int32 Slot) const;
    FGuid GetSlotInstanceGuid(int32 Slot) const;
    int32 GetSlotQuantity(int32 Slot) const;
    const TArray<FKalmalaEquippedGear>& GetEquippedGear() const { return EquippedGear; }
    const FKalmalaEquippedGear* FindEquippedGear(EKalmalaGearSlot Slot) const;
    bool EquipGearFromServer(FGuid ItemGuid, FString& Reason);
    bool UnequipGearFromServer(EKalmalaGearSlot Slot, FString& Reason);
    /** Trusted accepted-hit hook; clients cannot supply wear amounts. */
    void WearEquippedGearFromServer();
    uint32 GetGearRequestResultSerial() const { return GearRequestResultSerial; }
    bool WasLastGearRequestAccepted() const { return bLastGearRequestAccepted; }
    const FString& GetLastGearRequestReason() const { return LastGearRequestReason; }
    static bool BuildEquipGearTransaction(const TArray<FKalmalaInventoryStack>& BeforeStacks,
        const TArray<FKalmalaEquippedGear>& BeforeGear, FGuid ItemGuid, const TArray<FGuid>& OtherOwnedGuids,
        TArray<FKalmalaInventoryStack>& AfterStacks, TArray<FKalmalaEquippedGear>& AfterGear, FString& Reason,
        FGuid MainHandGuid = FGuid());
    static bool BuildUnequipGearTransaction(const TArray<FKalmalaInventoryStack>& BeforeStacks,
        const TArray<FKalmalaEquippedGear>& BeforeGear, EKalmalaGearSlot Slot, const TArray<FGuid>& OtherOwnedGuids,
        TArray<FKalmalaInventoryStack>& AfterStacks, TArray<FKalmalaEquippedGear>& AfterGear, FString& Reason,
        FGuid MainHandGuid = FGuid());
    FName GetActiveItem() const { return ActiveItem; }
    FGuid GetActiveInstanceGuid() const { return ActiveInstanceGuid; }
    FGuid GetMainHandGuid() const;
    int32 CountPackedTools(const TArray<FKalmalaToolState>& Tools) const;
    /** Validate all owned identities, then place only items outside Main Hand in the pack. */
    static bool BuildMainHandGrid(const TArray<FGuid>& ToolGuids, const TArray<FGuid>& StackGuids,
        FGuid HandGuid, const TArray<FGuid>& Before, TArray<FGuid>& After);
    /** Move an exact packed weapon/tool into Main Hand; an invalid GUID returns it to the pack. */
    bool SetMainHandFromServer(FGuid ItemGuid, FString& Reason);
    bool OwnsGridItem(FName ItemId) const;
    float GetCarriedWeight() const;
    float GetCarryCapacity() const { return CarryCapacity; }
    bool CanFitContents(const TArray<FKalmalaInventoryStack>& Items, int32 OtherItemCount) const;
    /** Stable, gap-preserving placement; every inventory entry shares these same cells. */
    static bool BuildGridLayout(const TArray<FGuid>& Owned, const TArray<FGuid>& Before, TArray<FGuid>& After);
    void SynchronizeGridFromServer();
    /** Retain a cell when a trusted tool-upgrade transaction replaces its identity. */
    void ReplaceGridItemFromServer(FName Previous, FName Replacement);
    bool MoveSlotFromServer(int32 Source, int32 Target, FName ExpectedSource, FName ExpectedTarget);
    bool MoveSlotByInstanceFromServer(int32 Source, int32 Target, FGuid ExpectedSource, FGuid ExpectedTarget);
    UFUNCTION(Server, Reliable) void ServerMoveSlot(int32 Source, int32 Target, FName ExpectedSource, FName ExpectedTarget);
    UFUNCTION(Server, Reliable) void ServerMoveSlotByInstance(int32 Source, int32 Target, FGuid ExpectedSource, FGuid ExpectedTarget);
    UFUNCTION(Server, Reliable) void ServerUseHotbarSlot(int32 Slot);
    UFUNCTION(Server, Reliable) void ServerSetMainHand(FGuid ItemGuid);
    UFUNCTION(Server, Reliable) void ServerEquipGear(FGuid ItemGuid);
    UFUNCTION(Server, Reliable) void ServerUnequipGear(EKalmalaGearSlot Slot);
    UFUNCTION(Client, Reliable) void ClientGearRequestResult(bool bAccepted, const FString& Reason);
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
        const TArray<FKalmalaInventoryStack>& Destination, FGuid ItemGuid,
        TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason);
    /** Compatibility helper for trusted callers that request a count by type; units are selected in source order. */
    static bool BuildTransfer(const TArray<FKalmalaInventoryStack>& Source,
        const TArray<FKalmalaInventoryStack>& Destination, FName ItemId, int32 Quantity,
        TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason);
    /** Persist the scratch chest before publishing this pack; failed validation/writes mutate neither. */
    bool TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage, FGuid ItemGuid, bool bDeposit,
        TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason);
    /** Trusted type-based helper retained for server-side verification and inventory validation. */
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
    uint32 GearRequestResultSerial = 0;
    bool bLastGearRequestAccepted = false;
    FString LastGearRequestReason;
    UPROPERTY(Replicated) TArray<FKalmalaInventoryStack> Stacks;
    UPROPERTY(Replicated) TArray<FKalmalaEquippedGear> EquippedGear;
    /** Preserve wear when the same physical item is returned to the pack and equipped again. */
    TMap<FGuid, int32> SessionGearWear;
    UPROPERTY(Replicated) TArray<FName> GridSlots;
    UPROPERTY(Replicated) TArray<FGuid> GridInstanceGuids;
    UPROPERTY(Replicated) FName ActiveItem;
    UPROPERTY(Replicated) FGuid ActiveInstanceGuid;
    UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1.0")) float CarryCapacity = 300.0f;
    double LastLayoutRequestTime = -1.0;
    double LastHotbarRequestTime = -1.0;
    double LastGearRequestTime = -1.0;
    bool bVerificationComplete = false;
#if !UE_BUILD_SHIPPING
    static bool VerifyHarvestGrants(AKalmalaCharacter* Character);
    void TickMenuReview(float DeltaTime);
    float MenuReviewElapsed = 0.0f;
    int32 MenuReviewStage = 0;
#endif
};
