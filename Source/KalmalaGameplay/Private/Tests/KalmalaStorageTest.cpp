#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStorageSaveGame.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStorageSaveTest, "Kalmala.Gameplay.Storage.SaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStorageSaveTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config; Config.WorldSeed = 418; auto* Save = NewObject<UKalmalaStorageSaveGame>(); Save->InitializeForWorld(Config);
    TestTrue(TEXT("Empty identity container is valid"), Save->MatchesWorld(Config));
    TestTrue(TEXT("Store two materials"), Save->UpsertRecord(TEXT("chest-1"), {{TEXT("Wood"),3},{TEXT("Stone"),2}}));
    for (const int32 Quantity : {0, -1, MIN_int32, MAX_int32})
        TestFalse(TEXT("Invalid stack rejected"), Save->UpsertRecord(TEXT("chest-1"), {{TEXT("Wood"),Quantity}}));
    TestFalse(TEXT("Duplicate stack rejected"), Save->UpsertRecord(TEXT("chest-1"), {{TEXT("Wood"),1},{TEXT("Wood"),1}}));
    TestFalse(TEXT("Unknown item rejected"), Save->UpsertRecord(TEXT("chest-1"), {{TEXT("Forged"),1}}));
    TestFalse(TEXT("Path-like identity rejected"), Save->UpsertRecord(TEXT("../chest"), {}));
    TestFalse(TEXT("Empty identity rejected"), Save->UpsertRecord(TEXT(""), {}));
    TestFalse(TEXT("Oversized identity rejected"), Save->UpsertRecord(FString::ChrN(65, 'a'), {}));
    TestEqual(TEXT("Rejected writes leave contents intact"), Save->FindRecord(TEXT("chest-1"))->Stacks[0].Quantity, 3);
    for (int32 N = 2; N <= UKalmalaStorageSaveGame::MaxRecords; ++N)
        TestTrue(TEXT("Bounded chest record accepted"), Save->UpsertRecord(FString::Printf(TEXT("chest-%d"),N), {}));
    TestFalse(TEXT("129th chest rejected"), Save->UpsertRecord(TEXT("overflow"), {}));
    TestTrue(TEXT("Existing chest remains writable at cap"), Save->UpsertRecord(TEXT("chest-1"), {{TEXT("Wood"),3}}));
    TArray<uint8> Bytes; TestTrue(TEXT("Serialize storage in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UKalmalaStorageSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Reload typed storage"), Loaded))
    {
        TestTrue(TEXT("Reload validates all records"), Loaded->MatchesWorld(Config));
        const auto* Record = Loaded->FindRecord(TEXT("chest-1"));
        if (TestNotNull(TEXT("Stable ID survives reload"), Record)) TestEqual(TEXT("Contents survive reload"), Record->Stacks[0].Quantity, 3);
        Config.WorldSeed++;
        TestFalse(TEXT("Different seed rejected"), Loaded->MatchesWorld(Config));
        Config.WorldSeed--; Config.WorldSeed += 2;
        TestFalse(TEXT("Different generator rejected"), Loaded->MatchesWorld(Config));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStorageTransferTest, "Kalmala.Gameplay.Storage.Transfers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStorageTransferTest::RunTest(const FString& Parameters)
{
    const TArray<FKalmalaInventoryStack> Pack = {{TEXT("Wood"),3}};
    const TArray<FKalmalaInventoryStack> Chest = {{TEXT("Stone"),2}};
    TArray<FKalmalaInventoryStack> NextPack, NextChest; FString Reason;
    TestTrue(TEXT("Deposit validates both containers"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Wood"),1,NextPack,NextChest,Reason));
    TestEqual(TEXT("Only one leaves source"), NextPack[0].Quantity, 2);
    TestEqual(TEXT("Only one enters destination"), NextChest[1].Quantity, 1);
    TArray<FKalmalaInventoryStack> RestoredPack, RestoredChest;
    TestTrue(TEXT("Withdrawal reverses transfer"), UKalmalaInventoryComponent::BuildTransfer(NextChest,NextPack,TEXT("Wood"),1,RestoredChest,RestoredPack,Reason));
    TestEqual(TEXT("No duplicate after roundtrip"), RestoredPack[0].Quantity, 3);
    TestEqual(TEXT("Exhausted chest slot removed"), RestoredChest.Num(), 1);
    for (const int32 Quantity : {0,-1,MIN_int32,MAX_int32})
        TestFalse(TEXT("Malformed quantity rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Wood"),Quantity,NextPack,NextChest,Reason));
    TestFalse(TEXT("Missing item rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Fibre"),1,NextPack,NextChest,Reason));
    TestFalse(TEXT("Unknown item rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Forged"),1,NextPack,NextChest,Reason));
    const auto* Wood = GetDefault<UKalmalaItemCatalogue>()->FindItem(TEXT("Wood"));
    TestFalse(TEXT("Full stack rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,{{TEXT("Wood"),Wood->MaxStack}},TEXT("Wood"),1,NextPack,NextChest,Reason));
    TestFalse(TEXT("Duplicate destination fails closed"), UKalmalaInventoryComponent::BuildTransfer(Pack,{{TEXT("Stone"),1},{TEXT("Stone"),1}},TEXT("Wood"),1,NextPack,NextChest,Reason));
    TestEqual(TEXT("Failures preserve previous output"), NextPack[0].Quantity, 2);
    auto* Ownerless = NewObject<UKalmalaInventoryComponent>(); bool PersistCalled = false;
    TestFalse(TEXT("No authority cannot transfer or persist"), Ownerless->TransferStorageFromServer(Chest,TEXT("Stone"),false,
        [&](const auto&) { PersistCalled=true; return true; },Reason));
    TestFalse(TEXT("Unauthorized call never reaches persistence"), PersistCalled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStorageNetworkTest, "Kalmala.Gameplay.Storage.NetworkContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStorageNetworkTest::RunTest(const FString& Parameters)
{
    auto* Class = UKalmalaCraftingComponent::StaticClass();
    Class->SetUpRuntimeReplicationData();
    for (const FName Name : {FName(TEXT("ServerOpenStorage")),FName(TEXT("ServerCloseStorage")),FName(TEXT("ServerDepositStorage")),FName(TEXT("ServerWithdrawStorage"))})
    {
        const auto* Function = Class->FindFunctionByName(Name);
        if (!TestNotNull(TEXT("Storage owner intent exists"), Function)) continue;
        TestTrue(TEXT("Storage intent runs on server"), Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        const bool Transfer = Name == TEXT("ServerDepositStorage") || Name == TEXT("ServerWithdrawStorage");
        TestEqual(TEXT("No client target, count, save or snapshot payload"), int32(Function->NumParms), Transfer ? 1 : 0);
        if (Transfer) TestNotNull(TEXT("Transfer supplies only item identity"), Function->FindPropertyByName(TEXT("ItemId")));
    }
    TArray<FLifetimeProperty> Props; GetDefault<UKalmalaCraftingComponent>()->GetLifetimeReplicatedProps(Props);
    for (const FName Name : {FName(TEXT("StorageView")),FName(TEXT("bStorageViewOpen"))})
    {
        const auto* Property = Class->FindPropertyByName(Name);
        if (!TestNotNull(TEXT("Storage snapshot property exists"),Property)) continue;
        const auto* Rep = Props.FindByPredicate([&](const auto& P) { return P.RepIndex == Property->RepIndex; });
        if (TestNotNull(TEXT("Snapshot replicated"),Rep)) TestEqual(TEXT("Only owner receives snapshot"),int32(Rep->Condition),int32(COND_OwnerOnly));
    }
    TestNull(TEXT("Shared construction actor carries no replicated contents"), AKalmalaConstructionActor::StaticClass()->FindPropertyByName(TEXT("StorageView")));
    return true;
}
#endif
