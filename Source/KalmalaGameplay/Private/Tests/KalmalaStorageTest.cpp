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
    TestEqual(TEXT("Storage save schema records the distinct chest-capacity format"),
        UKalmalaStorageSaveGame::CurrentSchemaVersion, 3);
    TestEqual(TEXT("Normal Chest keeps its sixteen-stack capacity"),
        UKalmalaStorageSaveGame::GetCapacityForKit(TEXT("StorageKit")), 16);
    TestEqual(TEXT("Reinforced Chest has twenty-four shared stack spaces"),
        UKalmalaStorageSaveGame::GetCapacityForKit(TEXT("ReinforcedChestKit")), 24);
    TestTrue(TEXT("Empty identity container is valid"), Save->MatchesWorld(Config));
    TestTrue(TEXT("Store two materials"), Save->UpsertRecord(TEXT("chest-1"), TEXT("StorageKit"),
        {{TEXT("Wood"),3},{TEXT("Stone"),2}}));
    for (const int32 Quantity : {0, -1, MIN_int32, MAX_int32})
        TestFalse(TEXT("Invalid stack rejected"), Save->UpsertRecord(TEXT("chest-1"), TEXT("StorageKit"),
            {{TEXT("Wood"),Quantity}}));
    const FGuid WoodGuidA = FGuid::NewGuid();
    const FGuid WoodGuidB = FGuid::NewGuid();
    FKalmalaInventoryStack WoodStackA{TEXT("Wood"),1,FGuid::NewGuid()}; WoodStackA.ItemGuids.Add(WoodGuidA);
    FKalmalaInventoryStack WoodStackB{TEXT("Wood"),1,FGuid::NewGuid()}; WoodStackB.ItemGuids.Add(WoodGuidB);
    FKalmalaInventoryStack DuplicateWoodStack{TEXT("Wood"),1,FGuid::NewGuid()};
    DuplicateWoodStack.ItemGuids.Add(WoodGuidA);
    TestTrue(TEXT("Matching item types can occupy separate GUID-addressed stacks"), UKalmalaStorageSaveGame::IsValidStacks(
        {WoodStackA,WoodStackB}));
    TestFalse(TEXT("Duplicate item instance GUID rejected"), UKalmalaStorageSaveGame::IsValidStacks(
        {WoodStackA,DuplicateWoodStack}));
    TestFalse(TEXT("Unknown item rejected"), Save->UpsertRecord(TEXT("chest-1"), TEXT("StorageKit"), {{TEXT("Forged"),1}}));
    TestFalse(TEXT("Path-like identity rejected"), Save->UpsertRecord(TEXT("../chest"), TEXT("StorageKit"), {}));
    TestFalse(TEXT("Empty identity rejected"), Save->UpsertRecord(TEXT(""), TEXT("StorageKit"), {}));
    TestFalse(TEXT("Oversized identity rejected"), Save->UpsertRecord(FString::ChrN(65, 'a'), TEXT("StorageKit"), {}));
    const auto* PreservedRecord = Save->FindRecord(TEXT("chest-1"));
    if (TestNotNull(TEXT("Rejected writes preserve the existing record"), PreservedRecord))
    {
        if (TestTrue(TEXT("Preserved record keeps its material stack"), !PreservedRecord->Stacks.IsEmpty()))
        {
            TestEqual(TEXT("Rejected writes leave contents intact"), PreservedRecord->Stacks[0].Quantity, 3);
        }
    }
    TArray<FKalmalaInventoryStack> ReinforcedContents;
    for (int32 Slot = 0; Slot < UKalmalaStorageSaveGame::MaxReinforcedStorageSlots; ++Slot)
        ReinforcedContents.Add({TEXT("Wood"), 1});
    TestTrue(TEXT("A Reinforced Chest accepts twenty-four distinct shared stacks"),
        Save->UpsertRecord(TEXT("reinforced-1"), TEXT("ReinforcedChestKit"), ReinforcedContents));
    ReinforcedContents.Add({TEXT("Stone"), 1});
    TestFalse(TEXT("A Reinforced Chest rejects a twenty-fifth stack"),
        Save->UpsertRecord(TEXT("reinforced-1"), TEXT("ReinforcedChestKit"), ReinforcedContents));
    TestFalse(TEXT("A stored chest cannot change its construction type"),
        Save->UpsertRecord(TEXT("reinforced-1"), TEXT("StorageKit"), {}));
    for (int32 N = 2; N <= UKalmalaStorageSaveGame::MaxRecords - 1; ++N)
        TestTrue(TEXT("Bounded chest record accepted"), Save->UpsertRecord(
            FString::Printf(TEXT("chest-%d"),N), TEXT("StorageKit"), {}));
    TestFalse(TEXT("129th chest rejected"), Save->UpsertRecord(TEXT("overflow"), TEXT("StorageKit"), {}));
    TestTrue(TEXT("Existing chest remains writable at cap"),
        Save->UpsertRecord(TEXT("chest-1"), TEXT("StorageKit"), {{TEXT("Wood"),3}}));
    TArray<uint8> Bytes; TestTrue(TEXT("Serialize storage in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Loaded = Cast<UKalmalaStorageSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Reload typed storage"), Loaded))
    {
        TestTrue(TEXT("Reload validates all records"), Loaded->MatchesWorld(Config));
        const auto* Record = Loaded->FindRecord(TEXT("chest-1"));
        if (TestNotNull(TEXT("Stable ID survives reload"), Record)) TestEqual(TEXT("Contents survive reload"), Record->Stacks[0].Quantity, 3);
        const auto* Reinforced = Loaded->FindRecord(TEXT("reinforced-1"));
        if (TestNotNull(TEXT("Reinforced chest survives reload"), Reinforced))
        {
            TestEqual(TEXT("Reinforced chest retains its capacity identity"), Reinforced->KitId, FName(TEXT("ReinforcedChestKit")));
            TestEqual(TEXT("All twenty-four reinforced slots survive reload"), Reinforced->Stacks.Num(), 24);
        }
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
    TestFalse(TEXT("Missing item rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Amber"),1,NextPack,NextChest,Reason));
    TestFalse(TEXT("Unknown item rejected"), UKalmalaInventoryComponent::BuildTransfer(Pack,Chest,TEXT("Forged"),1,NextPack,NextChest,Reason));
    const auto* Wood = UKalmalaItemCatalogue::Get()->FindItem(TEXT("Wood"));
    TArray<FKalmalaInventoryStack> FullDestination;
    for (int32 Slot = 0; Slot < UKalmalaInventoryComponent::MaxSlots; ++Slot)
        FullDestination.Add({TEXT("Wood"),Wood->MaxStack});
    TestFalse(TEXT("Full inventory rejects a new stack"), UKalmalaInventoryComponent::BuildTransfer(
        Pack,FullDestination,TEXT("Wood"),1,NextPack,NextChest,Reason));
    TestTrue(TEXT("Matching destination items remain separate GUID-addressed stacks"),
        UKalmalaInventoryComponent::BuildTransfer(Pack,{{TEXT("Stone"),1},{TEXT("Stone"),1}},
            TEXT("Wood"),1,NextPack,NextChest,Reason));
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
        if (Transfer) TestNotNull(TEXT("Transfer supplies only exact item identity"), Function->FindPropertyByName(TEXT("ItemGuid")));
    }
    TArray<FLifetimeProperty> Props; GetDefault<UKalmalaCraftingComponent>()->GetLifetimeReplicatedProps(Props);
    for (const FName Name : {FName(TEXT("StorageView")), FName(TEXT("bStorageViewOpen")),
        FName(TEXT("LastInteractedCookingStationKit")), FName(TEXT("CookingStationInteractionSerial")),
        FName(TEXT("LastResult")), FName(TEXT("ResultSerial")), FName(TEXT("bLastResultAccepted"))})
    {
        const auto* Property = Class->FindPropertyByName(Name);
        if (!TestNotNull(TEXT("Owner-only result or storage property exists"), Property)) continue;
        const auto* Rep = Props.FindByPredicate([&](const auto& P) { return P.RepIndex == Property->RepIndex; });
        if (TestNotNull(TEXT("Owner-only result or storage property replicates"), Rep))
            TestEqual(TEXT("Only owner receives result or storage details"), int32(Rep->Condition), int32(COND_OwnerOnly));
    }
    TestNull(TEXT("Shared construction actor carries no replicated contents"), AKalmalaConstructionActor::StaticClass()->FindPropertyByName(TEXT("StorageView")));
    return true;
}
#endif
