#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMapAwarenessComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMapAwarenessTest, "Kalmala.Gameplay.MapAwareness.Authority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMapAwarenessTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->InitializeNewWorld(UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false));
    auto* GS = World->SpawnActor<AGameStateBase>();
    World->SetGameState(GS);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TArray<UKalmalaMapAwarenessComponent*> Peers;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        auto* PC = World->SpawnActor<AKalmalaMapPlayerController>();
        auto* PS = World->SpawnActor<AKalmalaMapPlayerState>();
        PC->PlayerState = PS;
        PS->SetPlayerId(Index + 1);
        GS->AddPlayerState(PS);
        PC->Possess(World->SpawnActor<ACharacter>(FVector(Index * 100.0, 0, 100), FRotator::ZeroRotator, Spawn));
        Peers.Add(PC->FindComponentByClass<UKalmalaMapAwarenessComponent>());
    }
    auto* Sender = Peers[0];
    auto* Receiver = Peers[1];
    auto* PrivatePeer = Peers[2];
    TestFalse(TEXT("New connection defaults to private"), Sender->IsSharingEnabled());
    TestEqual(TEXT("Unauthorized actual server path"), FString(Sender->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Unauthorized")));
    for (auto* Peer : {Sender, Receiver})
    {
        Peer->bLocalSharingRequested = true;
        Peer->ServerSetSharingEnabled_Implementation(true);
    }
    TestTrue(TEXT("Possessed opted-in owner is eligible"), Sender->IsSharingEnabled());
    TestEqual(TEXT("Only opted-in peer has a marker"), Sender->GetPeerMarkers().Num(), 1);
    TestEqual(TEXT("NaN rejected"), FString(Sender->TryAcceptPing(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0))), FString(TEXT("Malformed")));
    TestEqual(TEXT("Infinity rejected"), FString(Sender->TryAcceptPing(FVector2D(0, std::numeric_limits<double>::infinity()))), FString(TEXT("Malformed")));
    TestEqual(TEXT("Out-of-bound coordinate rejected"), FString(Sender->TryAcceptPing(FVector2D(1.0e9, 0))), FString(TEXT("Malformed")));
    TestEqual(TEXT("Distant ping rejected"), FString(Sender->TryAcceptPing(FVector2D(6501, 0))), FString(TEXT("Distant")));
    TestTrue(TEXT("Exact range boundary allowed"), UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(6500, 0), FVector2D::ZeroVector));
    TestEqual(TEXT("Valid server path accepts"), FString(Sender->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Accepted")));
    TestEqual(TEXT("Excessive actual server path"), FString(Sender->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Excessive")));
    TestEqual(TEXT("Recipient has exactly one ping"), Receiver->Inbox.Num(), 1);
    TestTrue(TEXT("Nonconsenting recipient receives no data"), PrivatePeer->Inbox.IsEmpty());
    TestTrue(TEXT("Nonconsenting recipient sees no markers"), PrivatePeer->GetPeerMarkers().IsEmpty());
    if (Receiver->Inbox.Num() == 1)
    {
        TestEqual(TEXT("Server fixes lifetime"), Receiver->Inbox[0].ExpiresAt - Receiver->Inbox[0].IssuedAt, 6.0);
        TestEqual(TEXT("Same authoritative expiry"), Receiver->Inbox[0].ExpiresAt, Sender->Inbox[0].ExpiresAt);
    }
    Sender->LastAcceptedAt = Sender->ServerNow() - UKalmalaMapAwarenessComponent::PingCooldown;
    Receiver->Controller()->GetPawn()->SetActorLocation(FVector(20000, 0, 100));
    TestEqual(TEXT("Cooldown boundary accepts next sequence"), FString(Sender->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Accepted")));
    TestEqual(TEXT("Distant recipient receives no new ping"), Receiver->Inbox.Num(), 1);
    TestEqual(TEXT("Server assigns monotonic sequence"), Sender->Inbox.Last().Sequence, uint32(2));
    Receiver->Controller()->GetPawn()->SetActorLocation(FVector(100, 0, 100));
    // Deterministic ties cannot depend on array delivery order.
    FKalmalaMapPing A, B, C;
    A.IssuedAt = B.IssuedAt = C.IssuedAt = 10;
    A.SenderId = B.SenderId = 1; C.SenderId = 2;
    A.Sequence = 1; B.Sequence = 2; C.Sequence = 1;
    TArray<FKalmalaMapPing> Ordered{C, B, A};
    Ordered.Sort(UKalmalaMapAwarenessComponent::PingLess);
    TestTrue(TEXT("Equal timestamps use sender then sequence"), Ordered[0].Sequence == 1 && Ordered[1].Sequence == 2 && Ordered[2].SenderId == 2);
    Receiver->Inbox[0].ExpiresAt = Receiver->ServerNow();
    TestTrue(TEXT("Exact expiry is hidden before server replication removal"), Receiver->GetVisiblePings().IsEmpty());
    Receiver->PruneInbox();
    TestTrue(TEXT("Expired server inbox is reclaimed"), Receiver->Inbox.IsEmpty());
    Receiver->Inbox = Sender->Inbox;
    Sender->ServerSetSharingEnabled_Implementation(false);
    TestTrue(TEXT("Opt-out revokes outstanding relays"), Receiver->Inbox.IsEmpty() && Sender->Inbox.IsEmpty());
    TestTrue(TEXT("Opt-out removes markers"), Receiver->GetPeerMarkers().IsEmpty());
    Sender->ServerSetSharingEnabled_Implementation(true);
    Sender->LastAcceptedAt = -1.0e30;
    Sender->TryAcceptPing(FVector2D::ZeroVector);
    Sender->Controller()->GetPlayerState<AKalmalaMapPlayerState>()->SetIsInactive(true);
    Receiver->PruneInbox();
    TestTrue(TEXT("Disconnected/inactive sender removed from outstanding inbox and markers"), Receiver->Inbox.IsEmpty() && Receiver->GetPeerMarkers().IsEmpty());
    TestEqual(TEXT("Inactive sender rejected"), FString(Sender->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Unauthorized")));
    Receiver->Controller()->UnPossess();
    TestEqual(TEXT("Unpossessed owner rejected"), FString(Receiver->TryAcceptPing(FVector2D::ZeroVector)), FString(TEXT("Unauthorized")));
    World->DestroyWorld(false);
    return true;
}
#endif
