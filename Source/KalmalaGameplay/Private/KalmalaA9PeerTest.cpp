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
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"

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
                if(!FKalmalaGemstoneContract::SelectTreasure(Biome,Descriptor.DescriptorSeed^WorldGenerationConfig.WorldSeed).Contains(TEXT("Garnet")))continue;
                auto* Chest=GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(Descriptor.Location,FRotator::ZeroRotator);
                if(!Chest){Fail(TEXT("treasure spawn"));return;}
                Chest->InitializeServer(Descriptor);
                if(!Chest->HasWorldTreasure() || Chest->IsTreasureClaimed()){Chest->Destroy();continue;}
                A9Treasure=Chest;
                Place(Miner,Descriptor.Location+FVector(-170,0,95),0);
                Place(Players[2],Descriptor.Location+FVector(0,-170,95),90);
                Place(Host,Descriptor.Location+FVector(-1000,0,100),0);
                A9PeerStage=6;return;
            }
        }
        if(A9SearchIndex>=6561)Fail(TEXT("no unclaimed canonical Garnet treasure"));
    }
    else if(A9PeerStage==6 && A9Treasure.IsValid() && A9Treasure->IsTreasureClaimed())
    {
        int32 Total=0;for(auto* Pawn:Players)Total+=Pawn->GetInventoryComponent()->GetQuantity(TEXT("Garnet"));
        const FString Id=TEXT("treasure:")+FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(A9Treasure->GetDescriptor());
        const auto* Saved=PopulationSaveGame?PopulationSaveGame->FindTreasure(Id):nullptr;
        if(Total!=1 || !Saved || !Saved->bClaimed){Fail(TEXT("treasure cardinality/save"));return;}
        UE_LOG(LogTemp,Display,TEXT("A9 peer treasure: Passed=1 TotalGarnet=1 SavedClaim=1 Treasure=%s"),*Id);
        A9PeerStage=99;
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
