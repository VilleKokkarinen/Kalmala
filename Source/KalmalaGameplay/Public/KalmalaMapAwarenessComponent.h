#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaMapAwarenessComponent.generated.h"

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaMapPing
{
    GENERATED_BODY()
    UPROPERTY() FVector2D Location = FVector2D::ZeroVector;
    UPROPERTY() int32 SenderId = 0;
    UPROPERTY() uint32 Sequence = 0;
    UPROPERTY() double IssuedAt = 0;
    UPROPERTY() double ExpiresAt = 0;
};

/** UI boundary for temporary map pings delivered to each owner's private inbox. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaMapAwarenessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaMapAwarenessComponent();
    void RequestPing(FVector2D Location);
    TArray<FKalmalaMapPing> GetVisiblePings() const;
    static constexpr double PingLifetime = 10.0;
    static constexpr double PingCooldown = 2.0;
    static constexpr int32 MaxInboxPings = 16;
    static bool IsValidPingLocation(FVector2D Location);
    static bool PingLess(const FKalmalaMapPing& A, const FKalmalaMapPing& B);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    friend class FKalmalaMapAwarenessTest;
    APlayerController* Controller() const;
    double ServerNow() const;
    const TCHAR* TryAcceptPing(FVector2D Location);
    void PruneInbox();
    void DeveloperVerification(float DeltaTime);
    UFUNCTION(Server, Unreliable) void ServerRequestPing(FVector2D Location);
    UPROPERTY(Replicated) TArray<FKalmalaMapPing> Inbox;
    double LastAcceptedAt = -1.0e30;
    uint32 NextSequence = 0;
    int32 VerificationStage = 0;
    double VerificationTime = 0;
    double VerificationStageTime = 0;
    TSet<uint64> VerificationSeen;
    bool bVerificationExpiryLogged = false;
};

UCLASS()
class KALMALAGAMEPLAY_API AKalmalaMapPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AKalmalaMapPlayerController();
private:
    UPROPERTY() TObjectPtr<UKalmalaMapAwarenessComponent> MapAwareness;
};
