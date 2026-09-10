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
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaInventoryComponent();
    static constexpr int32 MaxSlots = 16;
    const TArray<FKalmalaInventoryStack>& GetStacks() const { return Stacks; }
    int32 GetQuantity(FName ItemId) const;
    bool TryGrantFromServer(FName ItemId, int32 Quantity);
    bool TryConsumeFromServer(FName ItemId, int32 Quantity);
    /** Validate in a scratch array and publish once; no partial ingredient removal. */
    static bool BuildExchange(const TArray<FKalmalaInventoryStack>& Before,
        const TArray<FKalmalaInventoryStack>& Costs, FName Output, int32 OutputCount,
        TArray<FKalmalaInventoryStack>& After, FString& Reason);
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
    UPROPERTY(Replicated) TArray<FKalmalaInventoryStack> Stacks;
    bool bVerificationComplete = false;
};
