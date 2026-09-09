#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
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

struct KALMALAGAMEPLAY_API FKalmalaMapPeerMarker
{
    FVector2D Location;
    int32 PlayerId;
};

/** Consent is session-only, off on connection/reconnection, and never copied to inactive states. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaMapPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    bool IsMapSharingEnabled() const { return bMapSharingEnabled; }
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    friend class UKalmalaMapAwarenessComponent;
    UPROPERTY(Replicated) bool bMapSharingEnabled = false;
};

/** UI boundary: normal relevant pawn transforms in, temporary owner-only ping inbox out. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaMapAwarenessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaMapAwarenessComponent();
    void SetSharingEnabled(bool bEnabled);
    bool IsSharingEnabled() const;
    bool IsSharingRequested() const { return bLocalSharingRequested; }
    void RequestPing(FVector2D Location);
    TArray<FKalmalaMapPeerMarker> GetPeerMarkers() const;
    TArray<FKalmalaMapPing> GetVisiblePings() const;
    FString GetStatusText() const;
    static constexpr double PingLifetime = 6.0;
    static constexpr double PingCooldown = 2.0;
    static constexpr double PingRange = 6500.0;
    static constexpr int32 MaxInboxPings = 16;
    static bool IsLocationInRange(FVector2D Location, FVector2D PawnLocation);
    static bool PingLess(const FKalmalaMapPing& A, const FKalmalaMapPing& B);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    friend class FKalmalaMapAwarenessTest;
    APlayerController* Controller() const;
    bool IsEligible() const;
    bool IsSenderConnectedAndSharing(int32 PlayerId) const;
    double ServerNow() const;
    const TCHAR* TryAcceptPing(FVector2D Location);
    void PruneInbox();
    void DeveloperVerification(float DeltaTime);
    UFUNCTION(Server, Reliable) void ServerSetSharingEnabled(bool bEnabled);
    UFUNCTION(Server, Unreliable) void ServerRequestPing(FVector2D Location);
    UPROPERTY(Replicated) TArray<FKalmalaMapPing> Inbox;
    bool bLocalSharingRequested = false;
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
