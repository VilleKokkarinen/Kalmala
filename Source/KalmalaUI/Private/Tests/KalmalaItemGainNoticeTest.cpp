#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaSkillNotice.h"
#include "KalmalaNotificationSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaItemGainNoticeTest, "Kalmala.UI.Notifications.ItemGains",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaItemGainNoticeTest::RunTest(const FString& Parameters)
{
    const auto* RPC = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ClientAcceptedGain"));
    if (!TestNotNull(TEXT("Owner receipt RPC exists"), RPC)) return false;
    TestTrue(TEXT("Receipt is reliable server-to-owner only"), RPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable)
        && !RPC->HasAnyFunctionFlags(FUNC_NetServer | FUNC_NetMulticast));
    auto* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient authoritative world"), World)) return false;
    auto* Inventory = NewObject<UKalmalaInventoryComponent>(World->SpawnActor<AActor>());
    FKalmalaSkillNoticeQueue Queue;
    Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestTrue(TEXT("Accepted grant"), Inventory->TryGrantFromServer(TEXT("Wood"), 3));
    TestEqual(TEXT("Grant emits one receipt"), Inventory->GetGainReceipts().Num(), 1);
    TestFalse(TEXT("Forged grant rejected"), Inventory->TryGrantFromServer(TEXT("Forged"), 1));
    TestFalse(TEXT("Excessive grant rejected"), Inventory->TryGrantFromServer(TEXT("Wood"), MAX_int32));
    Inventory->TryConsumeFromServer(TEXT("Wood"), 3);
    TestEqual(TEXT("Consumption/rejection emit no gains"), Inventory->GetGainReceipts().Num(), 1);
    Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    if (!TestEqual(TEXT("Gain survives consumption before refresh"), Queue.GetRows().Num(), 1)) { World->DestroyWorld(false); return false; }
    TestEqual(TEXT("Receipt records awarded quantity"), Queue.GetRows()[0].Quantity, 3);
    Queue.Tick(2); Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestEqual(TEXT("Refresh does not renew gain"), Queue.GetRows()[0].Remaining, 2.f);
    Inventory->TryGrantFromServer(TEXT("Wood"), 2); Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestEqual(TEXT("Same item coalesces accepted gains"), Queue.GetRows()[0].Quantity, 5);
    FString Reason;
    TestTrue(TEXT("Accepted exchange"), Inventory->TryExchangeFromServer({{TEXT("Wood"), 1}}, TEXT("Stone"), 1, Reason));
    TestEqual(TEXT("Exchange records only output gain"), Inventory->GetGainReceipts().Last().ItemId, FName(TEXT("Stone")));
    const int32 BeforeFailure = Inventory->GetGainReceipts().Num();
    TestFalse(TEXT("Failed persistence blocks transfer"), Inventory->TransferStorageFromServer({{TEXT("Stone"), 2}}, TEXT("Stone"), false,
        [](const auto&) { return false; }, Reason));
    TestEqual(TEXT("Failed transfer emits nothing"), Inventory->GetGainReceipts().Num(), BeforeFailure);
    TestTrue(TEXT("Accepted withdrawal"), Inventory->TransferStorageFromServer({{TEXT("Stone"), 2}}, TEXT("Stone"), false,
        [](const auto&) { return true; }, Reason));
    const auto Before = Inventory->GetStacks();
    auto After = Before; After[0].Quantity += 1;
    TestTrue(TEXT("Validated candidate commits"), Inventory->TryCommitStacksFromServer(Before, After));
    const int64 Last = Inventory->GetGainReceipts().Last().Sequence;
    TestFalse(TEXT("Stale candidate rejected"), Inventory->TryCommitStacksFromServer(Before, After));
    TestEqual(TEXT("Stale candidate emits nothing"), Inventory->GetGainReceipts().Last().Sequence, Last);
    Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    auto* Widget = NewObject<UKalmalaNotificationWidget>(); Widget->SetNotices(Queue.GetRows(), 150, 1);
    TestTrue(TEXT("Canonical owner gain label"), Widget->GetPresentationText().Contains(TEXT("Gained 6 Wood")));
    Queue.Tick(4); Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestEqual(TEXT("Expired gains do not replay"), Queue.GetRows().Num(), 0);
    Queue.Reset(); Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestEqual(TEXT("Reconnect receipts silently baseline"), Queue.GetRows().Num(), 0);
    TArray<FKalmalaItemGainReceipt> Invalid{{1, TEXT("Wood"), 1}, {1, TEXT("Stone"), 1}};
    TestFalse(TEXT("Duplicate sequence rejects atomically"), Queue.ObserveGains(Invalid, 4));
    // Many accepted gain/consume cycles never grow the local receipt buffer.
    for (int32 Index = 0; Index < 40; ++Index)
    { Inventory->TryGrantFromServer(TEXT("Stone"), 1); Inventory->TryConsumeFromServer(TEXT("Stone"), 1); }
    TestEqual(TEXT("Owner receipt buffer bounded"), Inventory->GetGainReceipts().Num(), UKalmalaInventoryComponent::MaxGainReceipts);
    Queue.ObserveGains(Inventory->GetGainReceipts(), 4);
    TestEqual(TEXT("Buffered burst coalesces"), Queue.GetRows().Num(), 1);
    FKalmalaSkillNoticeQueue Peer; Peer.ObserveGains({}, 4);
    TestEqual(TEXT("Separate owner sees no receipts"), Peer.GetRows().Num(), 0);
    World->DestroyWorld(false);
    return true;
}
#endif
