#include "KalmalaMapAwarenessComponent.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "KalmalaWorldBounds.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include <limits>

AKalmalaMapPlayerController::AKalmalaMapPlayerController()
{
    MapAwareness = CreateDefaultSubobject<UKalmalaMapAwarenessComponent>(TEXT("MapAwareness"));
}

UKalmalaMapAwarenessComponent::UKalmalaMapAwarenessComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}

void UKalmalaMapAwarenessComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaMapAwarenessComponent, Inbox, COND_OwnerOnly);
}

APlayerController* UKalmalaMapAwarenessComponent::Controller() const { return Cast<APlayerController>(GetOwner()); }

double UKalmalaMapAwarenessComponent::ServerNow() const
{
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
}

bool UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D Location)
{
    if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y)) return false;
    FKalmalaWorldGenerationConfig Config;
    return FKalmalaWorldBounds::Contains(Config, Location);
}

bool UKalmalaMapAwarenessComponent::PingLess(const FKalmalaMapPing& A, const FKalmalaMapPing& B)
{
    if (A.IssuedAt != B.IssuedAt) return A.IssuedAt < B.IssuedAt;
    if (A.SenderId != B.SenderId) return A.SenderId < B.SenderId;
    return A.Sequence < B.Sequence;
}

void UKalmalaMapAwarenessComponent::RequestPing(FVector2D Location)
{
    if (Controller() && Controller()->IsLocalController()) ServerRequestPing(Location);
}

void UKalmalaMapAwarenessComponent::ServerRequestPing_Implementation(FVector2D Location)
{
    const TCHAR* Result = TryAcceptPing(Location);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaMapAwarenessTest")))
        UE_LOG(LogTemp, Display, TEXT("Map ping request: Result=%s"), Result);
#endif
}

const TCHAR* UKalmalaMapAwarenessComponent::TryAcceptPing(FVector2D Location)
{
    const APlayerController* Requester = Controller();
    const APlayerState* RequesterState = Requester ? Requester->PlayerState : nullptr;
    if (!GetOwner()->HasAuthority() || !Requester || Requester->IsActorBeingDestroyed()
        || !RequesterState || RequesterState->IsInactive() || RequesterState->IsOnlyASpectator())
        return TEXT("Unauthorized");
    if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y)) return TEXT("Malformed");
    if (!IsValidPingLocation(Location)) return TEXT("OutOfBounds");

    const double Now = ServerNow();
    if (Now - LastAcceptedAt < PingCooldown) return TEXT("Excessive");
    LastAcceptedAt = Now;
    FKalmalaMapPing Ping;
    Ping.Location = Location;
    Ping.SenderId = RequesterState->GetPlayerId();
    Ping.Sequence = ++NextSequence;
    Ping.IssuedAt = Now;
    Ping.ExpiresAt = Now + PingLifetime;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (!PC || PC->IsActorBeingDestroyed() || !PC->PlayerState || PC->PlayerState->IsInactive()) continue;
        auto* Recipient = PC->FindComponentByClass<UKalmalaMapAwarenessComponent>();
        if (!Recipient) continue;
        Recipient->PruneInbox();
        Recipient->Inbox.Add(Ping);
        Recipient->Inbox.Sort(PingLess);
        if (Recipient->Inbox.Num() > MaxInboxPings) Recipient->Inbox.RemoveAt(0, Recipient->Inbox.Num() - MaxInboxPings);
        PC->ForceNetUpdate();
    }
    return TEXT("Accepted");
}

void UKalmalaMapAwarenessComponent::PruneInbox()
{
    if (!GetOwner()->HasAuthority()) return;
    const int32 Removed = Inbox.RemoveAll([this](const FKalmalaMapPing& Ping)
    {
        return Ping.ExpiresAt <= ServerNow();
    });
    if (Removed) GetOwner()->ForceNetUpdate();
}

TArray<FKalmalaMapPing> UKalmalaMapAwarenessComponent::GetVisiblePings() const
{
    TArray<FKalmalaMapPing> Result;
    for (const FKalmalaMapPing& Ping : Inbox)
        if (Ping.ExpiresAt > ServerNow()) Result.Add(Ping);
    Result.Sort(PingLess);
    return Result;
}

void UKalmalaMapAwarenessComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    PruneInbox();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaMapAwarenessTest"))) DeveloperVerification(DeltaTime);
#endif
}

void UKalmalaMapAwarenessComponent::DeveloperVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    if (!Controller() || !Controller()->IsLocalController() || !Controller()->GetPawn()) return;
    const AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS || GS->PlayerArray.Num() < 2) return;
    VerificationTime += DeltaTime;
    const FVector2D Here(Controller()->GetPawn()->GetActorLocation());
    if (VerificationStage == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("Map awareness ping-only default: Pings=%d"), GetVisiblePings().Num());
        RequestPing(Here);
        VerificationStage = 1;
        VerificationStageTime = VerificationTime;
    }
    else if (VerificationStage == 1 && VerificationTime - VerificationStageTime > 2.5)
    {
        ServerRequestPing(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0));
        ServerRequestPing(FVector2D(FKalmalaWorldBounds::Radius + 1.0, 0));
        ServerRequestPing(FVector2D(1000000.0, 0));
        ServerRequestPing(Here);
        VerificationStage = 2;
    }

    const TArray<FKalmalaMapPing> Pings = GetVisiblePings();
    for (const FKalmalaMapPing& Ping : Pings)
    {
        const uint64 Key = (uint64(uint32(Ping.SenderId)) << 32) | Ping.Sequence;
        if (!VerificationSeen.Contains(Key))
        {
            VerificationSeen.Add(Key);
            UE_LOG(LogTemp, Display, TEXT("Map ping observed: Sender=%d Sequence=%u Issued=%.6f Expires=%.6f X=%.1f Y=%.1f"),
                Ping.SenderId, Ping.Sequence, Ping.IssuedAt, Ping.ExpiresAt, Ping.Location.X, Ping.Location.Y);
        }
    }
    if (VerificationSeen.Num() == 4 && Pings.IsEmpty() && !bVerificationExpiryLogged)
    {
        bVerificationExpiryLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Map awareness verification: Seen=4 Expired=1"));
    }
#endif
}
