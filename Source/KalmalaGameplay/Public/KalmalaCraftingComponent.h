#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaCraftingComponent.generated.h"

class AKalmalaCampfire;
class AKalmalaCharacter;
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
    UFUNCTION(Server, Reliable) void ServerRefuel();
    UFUNCTION(Server, Reliable) void ServerLight();
    bool CraftFromServer(FName RecipeId, int32 Batch, FString& Reason);
    bool PlaceFromServer(FString& Reason);
    FString GetRecipeDescription(FName RecipeId) const;
    FString GetRecipeAvailability(FName RecipeId) const;
    FString GetNearbyFireText() const;
    const FString& GetLastResult() const { return LastResult; }
    AKalmalaCampfire* FindNearbyFire(bool bRequireUsable) const;
private:
    bool AcceptRequest();
    void PublishResult(const FString& Result);
    AKalmalaCharacter* GetCharacter() const;
    void RunVerification(float DeltaTime);
    double NextRequestTime = 0;
    UPROPERTY(Replicated) FString LastResult;
    int32 VerificationStage = 0;
    float VerificationElapsed = 0;
    int32 LocalVerificationStage = 0;
    float LocalVerificationElapsed = 0;
    bool bVerificationPassed = true;
    UPROPERTY(Transient) TObjectPtr<AKalmalaCampfire> VerificationFire;
};
