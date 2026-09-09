#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaInventoryComponent.generated.h"

USTRUCT()
struct FKalmalaInventoryStack
{
    GENERATED_BODY()
    UPROPERTY() FName ItemId;
    UPROPERTY() int32 Quantity = 0;
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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
private:
    UPROPERTY(Replicated) TArray<FKalmalaInventoryStack> Stacks;
    bool bVerificationComplete = false;
};
