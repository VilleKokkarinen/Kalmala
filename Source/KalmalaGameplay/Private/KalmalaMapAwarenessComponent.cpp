#include "KalmalaMapAwarenessComponent.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

void AKalmalaMapPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaMapPlayerState, bMapSharingEnabled);
}

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

bool UKalmalaMapAwarenessComponent::IsEligible() const
{
    const APlayerController* PC = Controller();
    const AKalmalaMapPlayerState* PS = PC ? PC->GetPlayerState<AKalmalaMapPlayerState>() : nullptr;
    return PC && !PC->IsActorBeingDestroyed() && PC->GetPawn() && PS && !PS->IsInactive()
        && !PS->IsOnlyASpectator() && PS->IsMapSharingEnabled();
}

bool UKalmalaMapAwarenessComponent::IsSharingEnabled() const { return bLocalSharingRequested && IsEligible(); }

void UKalmalaMapAwarenessComponent::SetSharingEnabled(bool bEnabled)
{
    if (!Controller() || !Controller()->IsLocalController()) return;
    bLocalSharingRequested = bEnabled;
    ServerSetSharingEnabled(bEnabled);
}

void UKalmalaMapAwarenessComponent::ServerSetSharingEnabled_Implementation(bool bEnabled)
{
    if (!GetOwner()->HasAuthority() || !Controller()) return;
    if (AKalmalaMapPlayerState* PS = Controller()->GetPlayerState<AKalmalaMapPlayerState>())
    {
        PS->bMapSharingEnabled = bEnabled && !PS->IsInactive() && !PS->IsOnlyASpectator() && Controller()->GetPawn();
        PS->ForceNetUpdate();
        // Revoke outstanding pings from every inbox immediately on the server.
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (APlayerController* PC = It->Get())
                if (auto* Component = PC->FindComponentByClass<UKalmalaMapAwarenessComponent>()) Component->PruneInbox();
    }
}

double UKalmalaMapAwarenessComponent::ServerNow() const
{
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    return GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
}

bool UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D Location, FVector2D PawnLocation)
{
    return FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y)
        && FMath::IsFinite(PawnLocation.X) && FMath::IsFinite(PawnLocation.Y)
        && FMath::Abs(Location.X) <= 1.0e8 && FMath::Abs(Location.Y) <= 1.0e8
        && FVector2D::DistSquared(Location, PawnLocation) <= FMath::Square(PingRange);
}

bool UKalmalaMapAwarenessComponent::PingLess(const FKalmalaMapPing& A, const FKalmalaMapPing& B)
{
    if (A.IssuedAt != B.IssuedAt) return A.IssuedAt < B.IssuedAt;
    if (A.SenderId != B.SenderId) return A.SenderId < B.SenderId;
    return A.Sequence < B.Sequence;
}

void UKalmalaMapAwarenessComponent::RequestPing(FVector2D Location)
{
    if (Controller() && Controller()->IsLocalController() && IsSharingEnabled()) ServerRequestPing(Location);
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
    if (!GetOwner()->HasAuthority() || !IsEligible()) return TEXT("Unauthorized");
    if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y)
        || FMath::Abs(Location.X) > 1.0e8 || FMath::Abs(Location.Y) > 1.0e8) return TEXT("Malformed");
    if (!IsLocationInRange(Location, FVector2D(Controller()->GetPawn()->GetActorLocation()))) return TEXT("Distant");
    const double Now = ServerNow();
    if (Now - LastAcceptedAt < PingCooldown) return TEXT("Excessive");
    LastAcceptedAt = Now;
    FKalmalaMapPing Ping;
    Ping.Location = Location;
    Ping.SenderId = Controller()->PlayerState->GetPlayerId();
    Ping.Sequence = ++NextSequence;
    Ping.IssuedAt = Now;
    Ping.ExpiresAt = Now + PingLifetime;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        auto* Recipient = PC ? PC->FindComponentByClass<UKalmalaMapAwarenessComponent>() : nullptr;
        if (!Recipient || !Recipient->IsEligible()
            || !IsLocationInRange(Location, FVector2D(PC->GetPawn()->GetActorLocation()))) continue;
        Recipient->PruneInbox();
        Recipient->Inbox.Add(Ping);
        Recipient->Inbox.Sort(PingLess);
        if (Recipient->Inbox.Num() > MaxInboxPings) Recipient->Inbox.RemoveAt(0, Recipient->Inbox.Num() - MaxInboxPings);
        PC->ForceNetUpdate();
    }
    return TEXT("Accepted");
}

bool UKalmalaMapAwarenessComponent::IsSenderConnectedAndSharing(int32 PlayerId) const
{
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS) return false;
    for (const APlayerState* State : GS->PlayerArray)
    {
        const auto* PS = Cast<AKalmalaMapPlayerState>(State);
        if (PS && PS->GetPlayerId() == PlayerId && !PS->IsInactive() && !PS->IsOnlyASpectator()
            && !PS->IsActorBeingDestroyed() && PS->IsMapSharingEnabled() && PS->GetPawn()) return true;
    }
    return false;
}

void UKalmalaMapAwarenessComponent::PruneInbox()
{
    if (!GetOwner()->HasAuthority()) return;
    const int32 Removed = Inbox.RemoveAll([this](const FKalmalaMapPing& Ping)
    {
        return !IsEligible() || Ping.ExpiresAt <= ServerNow() || !IsSenderConnectedAndSharing(Ping.SenderId);
    });
    if (Removed) GetOwner()->ForceNetUpdate();
}

TArray<FKalmalaMapPeerMarker> UKalmalaMapAwarenessComponent::GetPeerMarkers() const
{
    TArray<FKalmalaMapPeerMarker> Result;
    const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!IsSharingEnabled() || !GS) return Result;
    for (const APlayerState* State : GS->PlayerArray)
    {
        const auto* PS = Cast<AKalmalaMapPlayerState>(State);
        const APawn* Pawn = PS ? PS->GetPawn() : nullptr;
        if (PS && PS != Controller()->PlayerState && PS->IsMapSharingEnabled() && !PS->IsInactive()
            && !PS->IsOnlyASpectator() && Pawn && !Pawn->IsActorBeingDestroyed() && !Pawn->IsHidden())
            Result.Add({FVector2D(Pawn->GetActorLocation()), PS->GetPlayerId()});
    }
    return Result;
}

TArray<FKalmalaMapPing> UKalmalaMapAwarenessComponent::GetVisiblePings() const
{
    TArray<FKalmalaMapPing> Result;
    if (!IsSharingEnabled()) return Result;
    for (const FKalmalaMapPing& Ping : Inbox)
        if (Ping.ExpiresAt > ServerNow() && IsSenderConnectedAndSharing(Ping.SenderId)) Result.Add(Ping);
    Result.Sort(PingLess);
    return Result;
}

FString UKalmalaMapAwarenessComponent::GetStatusText() const
{
    if (!bLocalSharingRequested) return TEXT("Co-op OFF (private; resets on reconnect)");
    if (!IsSharingEnabled()) return TEXT("Co-op awaiting server / unavailable");
    return FString::Printf(TEXT("Co-op ON: sharing position + nearby pings; %d visible peers (offline/private peers hidden)"), GetPeerMarkers().Num());
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
    const bool bHost = GetOwner()->HasAuthority();
    const FVector2D Here(Controller()->GetPawn()->GetActorLocation());
    if (VerificationStage == 0)
    {
        UE_LOG(LogTemp, Display, TEXT("Map awareness default: Private=%d Peers=%d Pings=%d"), !IsSharingEnabled(), GetPeerMarkers().Num(), GetVisiblePings().Num());
        ServerRequestPing(Here);
        VerificationStage = 1;
        VerificationStageTime = VerificationTime;
    }
    else if (VerificationStage == 1 && VerificationTime - VerificationStageTime > 1.0)
    {
        SetSharingEnabled(true);
        VerificationStage = 2;
    }
    else if (VerificationStage == 2 && IsSharingEnabled() && GetPeerMarkers().Num() == 1)
    {
        // Delay the client so cross-sender ordering is observable rather than a single array update.
        VerificationStage = 3;
        VerificationStageTime = VerificationTime;
    }
    else if (VerificationStage == 3 && VerificationTime - VerificationStageTime > (bHost ? 0.5 : 1.5))
    {
        ServerRequestPing(FVector2D(1.0e9, 0));
        ServerRequestPing(Here + FVector2D(PingRange + 1000, 0));
        ServerRequestPing(Here);
        ServerRequestPing(Here);
        VerificationStage = 4;
        VerificationStageTime = VerificationTime;
    }
    else if (VerificationStage == 4 && VerificationTime - VerificationStageTime > 2.5)
    {
        ServerRequestPing(Here);
        VerificationStage = 5;
    }
    const auto Pings = GetVisiblePings();
    for (const auto& Ping : Pings)
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
        UE_LOG(LogTemp, Display, TEXT("Map awareness verification: Peers=%d Seen=4 Expired=1"), GetPeerMarkers().Num());
        SetSharingEnabled(false);
        UE_LOG(LogTemp, Display, TEXT("Map awareness revoked: Peers=%d Pings=%d"), GetPeerMarkers().Num(), GetVisiblePings().Num());
    }
#endif
}
