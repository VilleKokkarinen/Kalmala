#include "KalmalaGameMode.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaCraftingComponent.h"
#include "GameFramework/GameStateBase.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"

namespace
{
bool AreA9PeerStacksEqual(const TArray<FKalmalaInventoryStack>& Left,
    const TArray<FKalmalaInventoryStack>& Right)
{
    if (Left.Num() != Right.Num()) return false;
    for (int32 Index = 0; Index < Left.Num(); ++Index)
    {
        if (Left[Index].ItemId != Right[Index].ItemId || Left[Index].Quantity != Right[Index].Quantity
            || Left[Index].InstanceGuid != Right[Index].InstanceGuid
            || Left[Index].ItemGuids != Right[Index].ItemGuids) return false;
    }
    return true;
}
}

void AKalmalaGameMode::DriveA9PeerTest()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaA9RestartCheck")))
    {
        if (A9PeerStage == 99) return;
        FString GemId, TreasureId;
        FParse::Value(FCommandLine::Get(), TEXT("A9GemId="), GemId);
        FParse::Value(FCommandLine::Get(), TEXT("A9TreasureId="), TreasureId, false);
        const auto* Gem = ReadGemstoneState(GemId);
        const auto* Treasure = PopulationSaveGame ? PopulationSaveGame->FindTreasure(TreasureId) : nullptr;
        if (!PopulationSaveGame) return;
        const bool bPassed = Gem && Gem->Hits == 3 && Gem->Outcome == 3 && Treasure && Treasure->bClaimed;
        UE_LOG(LogTemp, Display, TEXT("A9 restart: Passed=%d GemClaimed=%d TreasureClaimed=%d"), bPassed, Gem && Gem->Outcome == 3, Treasure && Treasure->bClaimed);
        A9PeerStage = 99;
        return;
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaA9PeerTest")) || A9PeerStage == 99) return;
#if WITH_DEV_AUTOMATION_TESTS
    if (!bA9PopulationWriterInstalled)
    {
        PopulationSaveWriteAttemptForTests = [this](UKalmalaWorldPopulationSaveGame* Candidate,
            const FString& Slot, const int32 UserIndex)
        {
            if (bA9FailNextTreasureSaveWrite && Candidate)
            {
                const FKalmalaTreasureState* Treasure = Candidate->FindTreasure(A9TreasureIdToFailSave);
                if (Treasure && Treasure->bClaimed)
                {
                    bA9FailNextTreasureSaveWrite = false;
                    return false;
                }
            }
            return UGameplayStatics::SaveGameToSlot(Candidate, Slot, UserIndex);
        };
        bA9PopulationWriterInstalled = true;
    }
#endif
    TArray<AKalmalaCharacter*> Players;
    AKalmalaCharacter* Host = nullptr;
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        auto* Controller = It->Get();
        auto* Pawn = Controller ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
        if (Controller && !Pawn)
        {
            // Simultaneous headless joins can occupy the sole generated start.
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Pawn = GetWorld()->SpawnActor<AKalmalaCharacter>(Controller->GetSpawnLocation()+FVector(0,300*Players.Num(),0), FRotator::ZeroRotator, Params);
            if (Pawn) Controller->Possess(Pawn);
        }
        if (!Pawn || !Pawn->GetPlayerState()) continue;
        if (Controller->IsLocalController()) Host = Pawn;
        else Players.Add(Pawn);
    }
    if (!Host || Players.Num() < 2) return;
    Players.Insert(Host, 0);
    auto* Miner = Players[1];
    const double Now = GetWorld()->GetTimeSeconds();
    const auto Fail = [this](const TCHAR* Reason) { UE_LOG(LogTemp, Error, TEXT("A9 peer FAILED: %s"), Reason); A9PeerStage = 99; };
    const auto Place = [](AKalmalaCharacter* Pawn, const FVector Position, const float Yaw)
    {
        Pawn->GetCharacterMovement()->StopMovementImmediately();
        // Canonical remote locations can stream in after the teleport. Hold the
        // fixture at interaction height while retaining ordinary owner RPCs.
        Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        Pawn->SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
        Pawn->SetActorRotation(FRotator(0,Yaw,0));
        Pawn->ServerSetAutomaticPickupEnabled(false);
        Pawn->ForceNetUpdate();
    };
    if (A9PeerStage == 0)
    {
        // Bounded per-frame search keeps normal network ticks running.
        for (int32 Attempt = 0; Attempt < 12 && A9SearchIndex < 6561; ++Attempt, ++A9SearchIndex)
        {
            const FIntPoint Key(A9SearchIndex % 81 - 40, A9SearchIndex / 81 - 40);
            for (const auto& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, Key, EKalmalaWorldPopulationKind::HarvestNode))
            {
                if (Spawn.ContentId != TEXT("gem-quartz-rock")) continue;
                const FString Id = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (ReadGemstoneState(Id)) continue;
                bool bSuccess = true;
                for (uint64 Hit = 1; Hit <= 3; ++Hit)
                    bSuccess &= FKalmalaGemstoneContract::Mix(WorldGenerationConfig.WorldSeed ^ uint64(FCrc::StrCrc32(*Id)) ^ (Hit * 0x9E3779B185EBCA87ull)) % 10 != 0;
                if (!bSuccess) continue;
                A9NodeKey = Key; A9NodeSeed = Spawn.SpawnSeed; A9NodeLocation = Spawn.Location;
                auto* Node = GetWorld()->SpawnActor<AKalmalaHarvestNode>(Spawn.Location, FRotator::ZeroRotator);
                if (!Node) { Fail(TEXT("node spawn")); return; }
                Node->InitializeServer(Spawn); A9Node = Node;
                const FName Necklaces[] = {TEXT("QuartzNecklace"),TEXT("GarnetNecklace"),TEXT("AmethystNecklace")};
                const FName Rings[] = {TEXT("AquamarineRing"),TEXT("JadeRing"),TEXT("DuskRing")};
                FString Reason;
                for (int32 Index = 0; Index < 3; ++Index)
                {
                    auto* Inventory = Players[Index]->GetInventoryComponent();
                    for (const FName Item : {Necklaces[Index], Rings[Index]})
                    {
                        if (!Inventory->TryGrantFromServer(Item,1)) { Fail(TEXT("gear fixture grant")); return; }
                        const auto* Stack = Inventory->GetStacks().FindByPredicate([Item](const auto& Row) { return Row.ItemId == Item; });
                        if (!Stack || !Inventory->EquipGearFromServer(Stack->ItemGuids[0],Reason)) { Fail(TEXT("gear equip")); return; }
                    }
                    Place(Players[Index], Spawn.Location + FVector(-1000,Index*300,100),0);
                }
                FKalmalaToolState Pick; Pick.ItemId = TEXT("StonePick"); Pick.ToolLevel=1;
                Pick.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Pick.ItemId,1); Pick.InstanceGuid=FGuid::NewGuid();
                Miner->CarriedTools.Add(Pick); Miner->GetInventoryComponent()->SynchronizeGridFromServer();
                if (!Miner->GetInventoryComponent()->SetMainHandFromServer(Pick.InstanceGuid,Reason)) { Fail(TEXT("pick selection")); return; }
                A9PickBefore = Pick.Durability;
                Place(Miner,Spawn.Location+FVector(-170,0,95),0);
                Place(Host,Spawn.Location+FVector(0,170,95),-90);
                if (Node->ApplyPickHitFromServer(Host) || Node->GetGemHits()!=0) { Fail(TEXT("wrong family guard")); return; }
                Place(Host,Spawn.Location+FVector(-1000,0,100),0);
                UE_LOG(LogTemp, Display, TEXT("A9 peer fixture: Node=%s RemoteMiner=1 CanonicalDescriptor=1"),*Id);
                A9PeerStage=1; return;
            }
        }
        if (A9SearchIndex >= 6561) Fail(TEXT("no unclaimed canonical Quartz candidate"));
        return;
    }
    auto* Node = A9Node.Get();
    if (A9PeerStage <= 4 && !Node) { Fail(TEXT("node disappeared")); return; }
    if (Node && A9PeerStage <= 4)
        for (TActorIterator<AKalmalaHarvestNode> It(GetWorld()); It; ++It)
            if (*It != Node && It->GetPersistentSpawnId() == Node->GetPersistentSpawnId()) It->Destroy();
    if (A9PeerStage == 6 && A9Treasure.IsValid())
        for (TActorIterator<AKalmalaDiscoveryActor> It(GetWorld()); It; ++It)
            if (*It != A9Treasure.Get() && FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(It->GetDescriptor())
                == FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(A9Treasure->GetDescriptor())) It->Destroy();
    if (A9PeerStage == 1 && Node->GetGemHits() == 1)
    {
        FKalmalaWorldPopulationSpawn Spawn; Spawn.Kind=EKalmalaWorldPopulationKind::HarvestNode;
        Spawn.SpatialKey=A9NodeKey; Spawn.SpawnSeed=A9NodeSeed; Spawn.Location=A9NodeLocation; Spawn.ContentId=TEXT("gem-quartz-rock");
        auto* Reload = GetWorld()->SpawnActor<AKalmalaHarvestNode>(Spawn.Location,FRotator::ZeroRotator);
        if (!Reload) { Fail(TEXT("partial reload spawn")); return; }
        Reload->InitializeServer(Spawn);
        if (Reload->GetGemHits()!=1 || Reload->GetGemOutcome()!=0) { Fail(TEXT("partial progress reload")); return; }
        Node->Destroy(); A9Node=Reload; A9PeerStage=2;
        UE_LOG(LogTemp,Display,TEXT("A9 peer partial reload: Passed=1 Hits=1"));
    }
    else if (A9PeerStage == 2 && Node->GetGemHits() == 2)
    {
        auto* Inventory=Miner->GetInventoryComponent(); A9MinerBefore=Inventory->GetStacks();
        TArray<FKalmalaInventoryStack> Full;
        for(int32 Cell=Inventory->CountPackedTools(Miner->CarriedTools);Cell<UKalmalaInventoryComponent::MaxSlots;++Cell) Full.Add({TEXT("Stone"),1});
        if (!Inventory->TryCommitStacksFromServer(A9MinerBefore,Full)) { Fail(TEXT("full pack fixture")); return; }
        A9PeerStage=3;
    }
    else if (A9PeerStage == 3 && Node->GetGemOutcome() == 2)
    {
        if (!A9NextTime)
        {
            A9NextTime=Now+5;
            auto* Other=Players[2]->GetInventoryComponent(); A9ContenderBefore=Other->GetStacks();
            TArray<FKalmalaInventoryStack> Full;
            for(int32 Cell=Other->CountPackedTools(Players[2]->CarriedTools);Cell<UKalmalaInventoryComponent::MaxSlots;++Cell)Full.Add({TEXT("Stone"),1});
            if(!Other->TryCommitStacksFromServer(A9ContenderBefore,Full)){Fail(TEXT("contender full pack"));return;}
            Place(Players[2],A9NodeLocation+FVector(0,-170,95),90);
        }
        if (Now<A9NextTime) return;
        auto* Inventory=Miner->GetInventoryComponent();
        if (Inventory->GetQuantity(TEXT("SmokyQuartz"))!=0 || Miner->GetToolDurability(TEXT("StonePick"))!=A9PickBefore-3
            || !Miner->FindComponentByClass<UKalmalaCraftingComponent>()->GetLastResult().Contains(TEXT("Inventory is full"))
            || !Players[2]->FindComponentByClass<UKalmalaCraftingComponent>()->GetLastResult().Contains(TEXT("Inventory is full")))
            { Fail(TEXT("full pack/durability")); return; }
        const auto Full=Inventory->GetStacks();
        if (!Inventory->TryCommitStacksFromServer(Full,A9MinerBefore)) { Fail(TEXT("full pack restore")); return; }
        auto* Other=Players[2]->GetInventoryComponent();
        if(!Other->TryCommitStacksFromServer(Other->GetStacks(),A9ContenderBefore)){Fail(TEXT("contender pack restore"));return;}
        A9MinerRequestBaseline=Miner->A9InteractionRequests;
        A9ContenderRequestBaseline=Players[2]->A9InteractionRequests;
        bA9FullPackObserved=true;
        A9PeerStage=4; A9NextTime=0;
        UE_LOG(LogTemp,Display,TEXT("A9 peer full pack: Passed=1 RemoteOwnedMining=1 PickSpent=3 Loose=1"));
    }
    else if (A9PeerStage == 4 && Node->IsHarvested())
    {
        int32 Total=0; for(auto* Pawn:Players) Total+=Pawn->GetInventoryComponent()->GetQuantity(TEXT("SmokyQuartz"));
        const auto* Saved=ReadGemstoneState(Node->GetPersistentSpawnId());
        if(Miner->A9InteractionRequests<=A9MinerRequestBaseline || Players[2]->A9InteractionRequests<=A9ContenderRequestBaseline)return;
        if(Total!=1 || !Saved || Saved->Outcome!=3 || !bA9FullPackObserved) { Fail(TEXT("gem claim cardinality/save"));return; }
        UE_LOG(LogTemp,Display,TEXT("A9 peer mining: Passed=1 TotalGem=1 SavedOutcome=3 OwningRequests=2 Node=%s"),*Node->GetPersistentSpawnId());
        A9PeerStage=5; A9SearchIndex=0;
        for(int32 I=0;I<3;++I) Place(Players[I],A9NodeLocation+FVector(-1000,I*300,100),0);
    }
    else if(A9PeerStage==5)
    {
        for(int32 Attempt=0;Attempt<12 && A9SearchIndex<6561;++Attempt,++A9SearchIndex)
        {
            const FIntPoint Key(A9SearchIndex%81-40,A9SearchIndex/81-40);
            for(const auto& Descriptor:FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(WorldGenerationConfig,Key,EKalmalaWorldDiscoveryKind::PointOfInterest))
            {
                const auto Biome=FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig,FVector2D(Descriptor.Location)));
                const TArray<FName> TreasureItems = FKalmalaGemstoneContract::SelectTreasure(
                    Biome, Descriptor.DescriptorSeed ^ WorldGenerationConfig.WorldSeed);
                if (TreasureItems.Num() != 1 || TreasureItems[0] != TEXT("Garnet")) continue;
                auto* Chest=GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(Descriptor.Location,FRotator::ZeroRotator);
                if(!Chest){Fail(TEXT("treasure spawn"));return;}
                Chest->InitializeServer(Descriptor);
                if(!Chest->HasWorldTreasure() || Chest->IsTreasureClaimed()){Chest->Destroy();continue;}
                const FString TreasureId = TEXT("treasure:") +
                    FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor);
                const FKalmalaTreasureState* SavedTreasure = PopulationSaveGame
                    ? PopulationSaveGame->FindTreasure(TreasureId) : nullptr;
                if (!SavedTreasure || SavedTreasure->Items != TreasureItems || SavedTreasure->bClaimed)
                { Chest->Destroy(); continue; }
                auto* MinerInventory = Miner->GetInventoryComponent();
                auto* ContenderInventory = Players[2]->GetInventoryComponent();
                if (!MinerInventory || !ContenderInventory)
                { Chest->Destroy(); Fail(TEXT("treasure inventories")); return; }
                const FKalmalaToolState* ExistingPick = Miner->GetCarriedToolInventory().FindByPredicate(
                    [](const FKalmalaToolState& Tool) { return Tool.ItemId == TEXT("StonePick"); });
                if (!ExistingPick) { Chest->Destroy(); Fail(TEXT("treasure pick fixture")); return; }
                const FKalmalaToolState PickForBoundary = *ExistingPick;
                A9MinerToolsBeforeTreasure = Miner->GetCarriedToolInventory();
                Miner->CarriedTools.Reset();
                Miner->CarriedTools.Add(PickForBoundary);
                MinerInventory->SynchronizeGridFromServer();
                Miner->ForceNetUpdate();
                A9MinerBefore = MinerInventory->GetStacks();
                A9ContenderBefore = ContenderInventory->GetStacks();
                A9TreasureItems = SavedTreasure->Items;
                const auto BuildExpected = [this](const TArray<FKalmalaInventoryStack>& Before,
                    TArray<FKalmalaInventoryStack>& OutExpected)
                {
                    OutExpected = Before;
                    FString Reason;
                    for (const FName Item : A9TreasureItems)
                    {
                        TArray<FKalmalaInventoryStack> Next;
                        if (!UKalmalaInventoryComponent::BuildGrant(OutExpected, Item, 1, Next, Reason)) return false;
                        OutExpected = MoveTemp(Next);
                    }
                    return true;
                };
                if (!BuildExpected(A9MinerBefore, A9MinerTreasureExpected)
                    || !BuildExpected(A9ContenderBefore, A9ContenderTreasureExpected))
                { Chest->Destroy(); Fail(TEXT("treasure expected pack")); return; }
                const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
                if (!Catalogue) { Chest->Destroy(); Fail(TEXT("treasure item catalogue")); return; }
                TArray<FKalmalaInventoryStack> NearlyFullPack;
                for (const FKalmalaItemDefinition& Item : Catalogue->Items)
                {
                    if (Item.ItemId == TEXT("Garnet")) continue;
                    TArray<FKalmalaInventoryStack> Candidate;
                    FString Reason;
                    if (UKalmalaInventoryComponent::BuildGrant(NearlyFullPack, Item.ItemId, 1, Candidate, Reason)
                        && Candidate.Num() > NearlyFullPack.Num()) NearlyFullPack = MoveTemp(Candidate);
                    if (NearlyFullPack.Num() == UKalmalaInventoryComponent::MaxSlots - 1) break;
                }
                const TArray<FKalmalaInventoryStack> EmptyPack;
                if (NearlyFullPack.Num() != UKalmalaInventoryComponent::MaxSlots - 1
                    || !MinerInventory->TryCommitStacksFromServer(A9MinerBefore, EmptyPack, false)
                    || !MinerInventory->TryCommitStacksFromServer(EmptyPack, NearlyFullPack, false))
                { Chest->Destroy(); Fail(TEXT("packed-tool treasure fixture")); return; }
                TArray<FKalmalaInventoryStack> FullCandidate;
                FString GrantReason;
                if (!UKalmalaInventoryComponent::BuildGrant(NearlyFullPack, TEXT("Garnet"), 1,
                        FullCandidate, GrantReason)
                    || MinerInventory->CanFitContents(FullCandidate,
                        MinerInventory->CountPackedTools(Miner->GetCarriedToolInventory())))
                { Chest->Destroy(); Fail(TEXT("packed-tool treasure boundary")); return; }
                A9Treasure=Chest;
                Place(Miner,Descriptor.Location+FVector(-170,0,95),0);
                Place(Players[2],Descriptor.Location+FVector(-1000,300,95),0);
                Place(Host,Descriptor.Location+FVector(-1000,0,100),0);
                A9TreasureFullPack = MoveTemp(NearlyFullPack);
                A9TreasureIdToFailSave = TreasureId;
                A9TreasurePhase = 0;
                A9MinerRequestBaseline = Miner->A9InteractionRequests;
                A9PeerStage=6;return;
            }
        }
        if(A9SearchIndex>=6561)Fail(TEXT("no unclaimed canonical Garnet treasure"));
    }
    else if(A9PeerStage==6 && A9Treasure.IsValid())
    {
        auto* Treasure = A9Treasure.Get();
        auto* MinerInventory = Miner->GetInventoryComponent();
        auto* MinerCrafting = Miner->FindComponentByClass<UKalmalaCraftingComponent>();
        const FString Id = TEXT("treasure:") +
            FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Treasure->GetDescriptor());
        const FKalmalaTreasureState* Saved = PopulationSaveGame ? PopulationSaveGame->FindTreasure(Id) : nullptr;
        if (!MinerInventory || !MinerCrafting || !Saved) { Fail(TEXT("treasure claim state")); return; }

        if (A9TreasurePhase == 0)
        {
            if (Miner->A9InteractionRequests <= A9MinerRequestBaseline
                || MinerCrafting->GetLastResult() != TEXT("Inventory is full; treasure remains")) return;
            if (Treasure->IsTreasureClaimed() || Saved->bClaimed
                || !AreA9PeerStacksEqual(MinerInventory->GetStacks(), A9TreasureFullPack))
            { Fail(TEXT("packed-tool treasure failure state")); return; }
            if (!MinerInventory->TryCommitStacksFromServer(A9TreasureFullPack, A9MinerBefore, false))
            { Fail(TEXT("restore treasure owner pack")); return; }
#if WITH_DEV_AUTOMATION_TESTS
            bA9FailNextTreasureSaveWrite = true;
#else
            Fail(TEXT("A9 save failure injection unavailable")); return;
#endif
            A9MinerRequestBaseline = Miner->A9InteractionRequests;
            A9TreasurePhase = 1;
            return;
        }

        if (A9TreasurePhase == 1)
        {
            if (Miner->A9InteractionRequests <= A9MinerRequestBaseline
                || MinerCrafting->GetLastResult() != TEXT("Treasure remains; claim could not be saved")) return;
            if (Treasure->IsTreasureClaimed() || Saved->bClaimed
                || !AreA9PeerStacksEqual(MinerInventory->GetStacks(), A9MinerBefore))
            { Fail(TEXT("treasure save failure rollback")); return; }
            Place(Players[2], Treasure->GetActorLocation() + FVector(0,-170,95), 90);
            A9TreasurePhase = 2;
            return;
        }

        if (A9TreasurePhase == 2 && Treasure->IsTreasureClaimed())
        {
            auto* ContenderInventory = Players[2]->GetInventoryComponent();
            const int32 TotalGarnet = MinerInventory->GetQuantity(TEXT("Garnet"))
                + ContenderInventory->GetQuantity(TEXT("Garnet"));
            const bool bMinerReceived = AreA9PeerStacksEqual(MinerInventory->GetStacks(), A9MinerTreasureExpected)
                && AreA9PeerStacksEqual(ContenderInventory->GetStacks(), A9ContenderBefore);
            const bool bContenderReceived = AreA9PeerStacksEqual(ContenderInventory->GetStacks(), A9ContenderTreasureExpected)
                && AreA9PeerStacksEqual(MinerInventory->GetStacks(), A9MinerBefore);
            if (TotalGarnet != 1 || !Saved->bClaimed || Saved->Items != A9TreasureItems
                || bMinerReceived == bContenderReceived)
            { Fail(TEXT("treasure cardinality, contents, or saved claim")); return; }
            Miner->CarriedTools = A9MinerToolsBeforeTreasure;
            MinerInventory->SynchronizeGridFromServer();
            Miner->ForceNetUpdate();
            UE_LOG(LogTemp,Display,TEXT("A9 peer treasure: Passed=1 TotalGarnet=1 SavedClaim=1 SavedItems=1 CapacityRetry=1 SaveRetry=1 Treasure=%s"),*Id);
            A9PeerStage=99;
        }
    }
#endif
}

void AKalmalaCharacter::VerifyA9Peer()
{
#if !UE_BUILD_SHIPPING
    if(!FParse::Param(FCommandLine::Get(),TEXT("KalmalaA9PeerTest")) || !IsLocallyControlled() || !Controller || !GetWorld())return;
    const auto* Movement=Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement());
    if(!bA9StatsLogged && Inventory && Inventory->GetEquippedGear().Num()==2 && Movement)
    {
        int32 Others=0;bool bPrivate=true;
        for(TActorIterator<AKalmalaCharacter> It(GetWorld());It;++It)
            if(*It!=this && It->GetPlayerState()) {++Others;bPrivate&=It->GetInventoryComponent()->GetEquippedGear().IsEmpty();}
        const auto Modifiers=Statuses->GetAccessoryModifiers();
        if(Others>=2 && (HasAuthority() || bPrivate) && FMath::IsNearlyEqual(GetMaximumHealth(),100.f+Modifiers.MaxHealth)
            && FMath::IsNearlyEqual(Movement->GetMaximumStamina(),100.f+Modifiers.MaxStamina))
        {
            UE_LOG(LogTemp,Display,TEXT("A9 peer owner stats: Passed=1 Gear=2 MaxHealth=%.1f MaxStamina=%.1f ForeignGearHidden=%d"),GetMaximumHealth(),Movement->GetMaximumStamina(),HasAuthority()?1:bPrivate);
            bA9StatsLogged=true;
        }
    }
    if(HasAuthority())return; // Actual mining/loot input comes from owning remote clients.
    static double NextProbe = 0;
    if (GetWorld()->GetTimeSeconds() >= NextProbe)
    {
        NextProbe = GetWorld()->GetTimeSeconds() + 5;
        float Nearest = TNumericLimits<float>::Max();
        int32 Nodes = 0;
        for (TActorIterator<AKalmalaHarvestNode> It(GetWorld()); It; ++It)
            if (It->IsGemstone()) { ++Nodes; Nearest = FMath::Min(Nearest, FVector::Distance(GetActorLocation(), It->GetActorLocation())); }
        UE_LOG(LogTemp, Display, TEXT("A9 peer client probe: Pos=%s GemNodes=%d Nearest=%.1f Active=%s"), *GetActorLocation().ToCompactString(), Nodes, Nearest, *Inventory->GetActiveItem().ToString());
    }
    AActor* Target=nullptr;bool bMine=false;
    for(TActorIterator<AKalmalaHarvestNode> It(GetWorld());It;++It)
        if(It->IsGemstone() && !It->IsHarvested() && It->GetGemOutcome()!=1 && FVector::DistSquared(GetActorLocation(),It->GetActorLocation())<FMath::Square(240.f))
        {Target=*It;bMine=It->GetGemOutcome()==0;break;}
    if(!Target)for(TActorIterator<AKalmalaDiscoveryActor> It(GetWorld());It;++It)
        if(It->HasWorldTreasure() && !It->IsTreasureClaimed() && FVector::DistSquared(GetActorLocation(),It->GetActorLocation())<FMath::Square(240.f)) {Target=*It;break;}
    auto* Crafting = FindComponentByClass<UKalmalaCraftingComponent>();
    const auto GetGarnetReceiptTotals = [this](int32& OutCount, int32& OutQuantity)
    {
        OutCount = 0;
        OutQuantity = 0;
        for (const FKalmalaItemGainReceipt& Receipt : Inventory->GetGainReceipts())
            if (Receipt.ItemId == TEXT("Garnet"))
            { ++OutCount; OutQuantity += Receipt.Quantity; }
    };
    if (const auto* Treasure = Cast<AKalmalaDiscoveryActor>(Target))
    {
        const FString Id = TEXT("treasure:") +
            FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Treasure->GetDescriptor());
        if (A9TreasureOwnerObservedId != Id)
        {
            A9TreasureOwnerObservedId = Id;
            A9TreasureOwnerLastResultSerial = Crafting ? Crafting->GetResultSerial() : 0;
            GetGarnetReceiptTotals(A9TreasureOwnerGarnetReceiptCountBaseline,
                A9TreasureOwnerGarnetQuantityBaseline);
            A9TreasureOwnerPendingFailure = 0;
            A9TreasureOwnerFailureCheckTime = 0.0;
            bA9TreasureOwnerSuccessResultObserved = false;
            A9TreasureOwnerSuccessReceiptDeadline = 0.0;
            bA9TreasureOwnerCapacityFailureObserved = false;
            bA9TreasureOwnerSaveFailureObserved = false;
            bA9TreasureOwnerSuccessReceiptObserved = false;
        }
    }
    if (Crafting && Inventory && !A9TreasureOwnerObservedId.IsEmpty()
        && Crafting->GetResultSerial() != A9TreasureOwnerLastResultSerial)
    {
        A9TreasureOwnerLastResultSerial = Crafting->GetResultSerial();
        const FString Result = Crafting->GetLastResult();
        if (Result == TEXT("Inventory is full; treasure remains")
            && !bA9TreasureOwnerCapacityFailureObserved)
        {
            A9TreasureOwnerPendingFailure = 1;
            A9TreasureOwnerFailureCheckTime = GetWorld()->GetTimeSeconds() + 0.5;
        }
        else if (Result == TEXT("Treasure remains; claim could not be saved")
            && !bA9TreasureOwnerSaveFailureObserved)
        {
            A9TreasureOwnerPendingFailure = 2;
            A9TreasureOwnerFailureCheckTime = GetWorld()->GetTimeSeconds() + 0.5;
        }
        else if (Result == TEXT("Treasure collected") && !bA9TreasureOwnerSuccessResultObserved)
        {
            bA9TreasureOwnerSuccessResultObserved = true;
            A9TreasureOwnerSuccessReceiptDeadline = GetWorld()->GetTimeSeconds() + 5.0;
        }
    }
    const double A9Now = GetWorld()->GetTimeSeconds();
    if (A9TreasureOwnerPendingFailure != 0 && A9Now >= A9TreasureOwnerFailureCheckTime)
    {
        int32 GarnetReceiptCount = 0;
        int32 GarnetReceiptQuantity = 0;
        GetGarnetReceiptTotals(GarnetReceiptCount, GarnetReceiptQuantity);
        if (GarnetReceiptCount != A9TreasureOwnerGarnetReceiptCountBaseline
            || GarnetReceiptQuantity != A9TreasureOwnerGarnetQuantityBaseline)
            UE_LOG(LogTemp, Error, TEXT("A9 peer FAILED: owner received a Garnet receipt on treasure failure"));
        else if (A9TreasureOwnerPendingFailure == 1)
        {
            bA9TreasureOwnerCapacityFailureObserved = true;
            UE_LOG(LogTemp, Display, TEXT("A9 peer treasure owner: CapacityFailure=1 GarnetReceiptDelta=0"));
        }
        else
        {
            bA9TreasureOwnerSaveFailureObserved = true;
            UE_LOG(LogTemp, Display, TEXT("A9 peer treasure owner: SaveFailure=1 GarnetReceiptDelta=0"));
        }
        A9TreasureOwnerPendingFailure = 0;
    }
    if (bA9TreasureOwnerSuccessResultObserved && !bA9TreasureOwnerSuccessReceiptObserved)
    {
        int32 GarnetReceiptCount = 0;
        int32 GarnetReceiptQuantity = 0;
        GetGarnetReceiptTotals(GarnetReceiptCount, GarnetReceiptQuantity);
        if (GarnetReceiptCount == A9TreasureOwnerGarnetReceiptCountBaseline + 1
            && GarnetReceiptQuantity == A9TreasureOwnerGarnetQuantityBaseline + 1)
        {
            bA9TreasureOwnerSuccessReceiptObserved = true;
            UE_LOG(LogTemp, Display, TEXT("A9 peer treasure owner: SuccessReceipt=1 GarnetReceiptDelta=1"));
        }
        else if (GarnetReceiptCount > A9TreasureOwnerGarnetReceiptCountBaseline + 1
            || GarnetReceiptQuantity > A9TreasureOwnerGarnetQuantityBaseline + 1
            || A9Now >= A9TreasureOwnerSuccessReceiptDeadline)
            UE_LOG(LogTemp, Error, TEXT("A9 peer FAILED: owner treasure retry receipt did not match the saved reward"));
    }
    if(!Target){A9NextActionTime=0;return;}
    FVector View;FRotator ViewRotation;Controller->GetPlayerViewPoint(View,ViewRotation);
    Controller->SetControlRotation((Target->GetActorLocation()-View).Rotation());
    const double Now=GetWorld()->GetTimeSeconds();
    if(bMine)
    {
        if(!A9NextActionTime){A9NextActionTime=Now+1.25;return;}
        if(Now<A9NextActionTime)return;
        A9NextActionTime=Now+1.25;
        if(Inventory->GetActiveItem()==TEXT("StonePick"))RequestAttack();
    }
    else
    {
        const auto* State=GetWorld()->GetGameState();
        const double ServerNow=State?State->GetServerWorldTimeSeconds():Now;
        const int32 Cycle=FMath::FloorToInt(ServerNow/2.0);
        if(Cycle==A9LastLootCycle || FMath::Fmod(ServerNow,2.0)>.10)return;
        A9LastLootCycle=Cycle;
        UE_LOG(LogTemp,Display,TEXT("A9 peer owning interaction request: Target=%s"),*Target->GetClass()->GetName());
        RequestInteract();
    }
#endif
}
