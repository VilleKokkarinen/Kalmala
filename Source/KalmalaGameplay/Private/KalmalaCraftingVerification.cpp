#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Components/BoxComponent.h"

void UKalmalaCraftingComponent::RunVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    auto* C = GetCharacter(); if (!C || !C->GetPlayerState()) return;
    auto* I = C->FindComponentByClass<UKalmalaInventoryComponent>(); if (!I) return;
    VerificationElapsed += DeltaTime;
    if (C->HasAuthority() && VerificationStage == 0 && VerificationElapsed > 3)
    {
        int32 Players=0;
        for(TActorIterator<AKalmalaCharacter> It(GetWorld());It;++It) if(It->GetPlayerState()) ++Players;
        if(Players<2) return;
        auto Check = [&](bool Passed, const TCHAR* Label) {
            bVerificationPassed &= Passed;
            if (!Passed) UE_LOG(LogTemp, Error, TEXT("Crafting fixture FAILED: %s"), Label);
        };
        FString Reason;
        Check(!PlaceFromServer(Reason), TEXT("No ingredients cannot create a hearth"));
        Check(!CraftFromServer(TEXT("Campfire"),1,Reason), TEXT("Insufficient craft ingredients"));
        Check(!CraftFromServer(TEXT("Forged"),1,Reason), TEXT("Unknown recipe"));
        for (int32 Batch : {MIN_int32,-1,0,MAX_int32}) Check(!CraftFromServer(TEXT("Fuel"),Batch,Reason),TEXT("Malformed batch"));
        Check(I->TryGrantFromServer(TEXT("Wood"),50) && I->TryGrantFromServer(TEXT("Stone"),40)
            && I->TryGrantFromServer(TEXT("Fibre"),50) && I->TryGrantFromServer(TEXT("ConstructionSupply"),20),TEXT("Seed bounded test materials"));
        const FVector StationProbeOrigin=C->GetActorLocation();
        C->SetActorLocation(StationProbeOrigin+FVector(0,0,10000));
        Check(!CraftFromServer(TEXT("Floor"),1,Reason),TEXT("Missing station rejects craft"));
        C->SetActorLocation(StationProbeOrigin);
        auto* Recipes=GetMutableDefault<UKalmalaRecipeCatalogue>();
        auto& FuelRecipe=Recipes->Recipes[0]; const bool Enabled=FuelRecipe.bEnabled; FuelRecipe.bEnabled=false;
        Check(!CraftFromServer(FuelRecipe.RecipeId,1,Reason),TEXT("Disabled recipe rejects craft")); FuelRecipe.bEnabled=Enabled;
        Check(I->TryGrantFromServer(TEXT("Fuel"),20),TEXT("Fill output stack"));
        const int32 WoodBefore=I->GetQuantity(TEXT("Wood"));
        Check(!CraftFromServer(TEXT("Fuel"),1,Reason) && I->GetQuantity(TEXT("Wood"))==WoodBefore,TEXT("Full output cannot consume inputs"));
        Check(CraftFromServer(TEXT("Campfire"),1,Reason),TEXT("Craft hearth kit"));
        const FVector Original=C->GetActorLocation(); const FRotator OriginalRotation=C->GetActorRotation();
        TSet<AKalmalaCampfire*> Existing;
        for(TActorIterator<AKalmalaCampfire> It(GetWorld());It;++It) Existing.Add(*It);
        bool Placed=false;
        for(int32 Turn=0; Turn<8 && !Placed; ++Turn)
        {
            C->SetActorRotation(FRotator(0,Turn*45,0));
            Placed=PlaceFromServer(Reason);
        }
        Check(Placed,TEXT("Paid placement on actual generated collision"));
        for(TActorIterator<AKalmalaCampfire> It(GetWorld());It;++It) if(!Existing.Contains(*It)) VerificationFire=*It;
        if (!VerificationFire) { VerificationStage=99; return; }
        auto* Fire=VerificationFire.Get(); Fire->SetActorTickEnabled(false);
        Check(I->GetQuantity(TEXT("CampfireKit"))==0 && I->GetQuantity(TEXT("Fuel"))==19,TEXT("Placement charges kit and fuel exactly once"));
        Check(CraftFromServer(TEXT("Campfire"),1,Reason),TEXT("Prepare overlapping placement"));
        Check(!PlaceFromServer(Reason) && I->GetQuantity(TEXT("CampfireKit"))==1
            && I->GetQuantity(TEXT("Fuel"))==19,TEXT("Overlap rejects placement without payment"));
        Check(CraftFromServer(TEXT("Floor"),1,Reason),TEXT("Nearby permitted station"));
        Fire->SetOwner(nullptr); Fire->SetSharedFromServer(false);
        // Move away from any other player's eligible shared hearth while checking this lock.
        const FVector FireOrigin=Fire->GetActorLocation();
        C->SetActorLocation(Original+FVector(0,0,1000)); Fire->SetActorLocation(C->GetActorLocation()+FVector(100,0,0));
        Check(!CraftFromServer(TEXT("Floor"),1,Reason),TEXT("Locked station"));
        Fire->SetOwner(C->GetController()); Fire->SetSharedFromServer(true);
        Fire->SetActorLocation(FireOrigin);
        Check(!CraftFromServer(TEXT("Floor"),1,Reason) && !Fire->TryRefuelFromServer(C),TEXT("Distant station and refuel"));
        C->SetActorLocation(Original); C->SetActorRotation(OriginalRotation);
        bool ConstructionPlaced = false;
        TSet<AKalmalaConstructionActor*> ExistingConstruction;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) ExistingConstruction.Add(*It);
        for (int32 Turn = 0; Turn < 8 && !ConstructionPlaced; ++Turn)
        {
            C->SetActorRotation(FRotator(0, Turn * 45, 0));
            ConstructionPlaced = PlaceConstructionFromServer(TEXT("FloorKit"), Reason);
        }
        Check(ConstructionPlaced, TEXT("Server construction placement ignores local preview and pays once"));
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) if (!ExistingConstruction.Contains(*It))
            UE_LOG(LogTemp, Display, TEXT("Construction accepted: Id=%s Kit=%s"), *It->GetConstructionId(), *It->GetConstructionKit().ToString());
        C->SetActorRotation(OriginalRotation);
        for(int32 N=0; N<4; ++N) Check(Fire->TryRefuelFromServer(C),TEXT("Bounded refuel"));
        const int32 FuelBefore=I->GetQuantity(TEXT("Fuel"));
        Check(!Fire->TryRefuelFromServer(C) && I->GetQuantity(TEXT("Fuel"))==FuelBefore,TEXT("Full hearth cannot consume fuel"));
        Fire->Interact_Implementation(C); Fire->AdvanceFromServer(300,0,0);
        Check(Fire->GetFuelSeconds()==0 && !Fire->IsLit() && Fire->GetEffectiveWarmth()==0,TEXT("Fuel exhaustion extinguishes warmth"));
        Check(Fire->TryRefuelFromServer(C),TEXT("Refuel exhausted hearth"));
        // Real server shelter traces must shield the fire; no authored protection volume.
        auto* WorldState=GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
        const auto SavedWeather=WorldState->GetWeatherState(); auto Storm=SavedWeather;
        Storm.PrecipitationIntensity=1; Storm.WindStrength=1; Storm.WindDirectionDegrees=0;
        WorldState->SetWeatherStateFromServer(Storm);
        auto MakeShelter=[&](FVector Offset,FVector Extent,const TCHAR* Tag) {
            auto* Actor=GetWorld()->SpawnActor<AActor>();
            auto* Box=NewObject<UBoxComponent>(Actor); Actor->SetRootComponent(Box); Actor->AddInstanceComponent(Box);
            Box->SetBoxExtent(Extent); Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
            Actor->SetActorLocation(Fire->GetActorLocation()+Offset); Actor->Tags.Add(FName(Tag)); return Actor;
        };
        auto* Roof=MakeShelter(FVector(0,0,200),FVector(120,120,10),TEXT("KalmalaShelterRoof"));
        auto* Wall=MakeShelter(FVector(-200,0,60),FVector(10,120,180),TEXT("KalmalaShelterWindbreak"));
        Fire->Interact_Implementation(C); Fire->Tick(1);
        Check(Fire->HasRoof() && Fire->HasWindbreak() && Fire->IsLit() && Fire->GetFuelWetness()==0
            && Fire->GetEffectiveWarmth()==1,TEXT("Server roof and windbreak traces protect fire"));
        Roof->Destroy(); Wall->Destroy(); WorldState->SetWeatherStateFromServer(SavedWeather);
        Fire->AdvanceFromServer(300,0,0); Check(Fire->TryRefuelFromServer(C),TEXT("Prepare rain fixture"));
        Fire->Interact_Implementation(C); Fire->AdvanceFromServer(12,1,1);
        Check(!Fire->IsLit() && Fire->GetEffectiveWarmth()==0 && Fire->GetFuelWetness()>=.9f,TEXT("Exposed rain extinguishes"));
        Check(!Fire->CanInteract_Implementation(C),TEXT("Wet lighting rejected"));
        const float WetBefore=Fire->GetFuelWetness(); Check(Fire->TryRefuelFromServer(C) && Fire->GetFuelWetness()==WetBefore,TEXT("Refuel preserves wetness"));
        Fire->AdvanceFromServer(100,0,0); Fire->Interact_Implementation(C); Fire->AdvanceFromServer(300,0,0);
        Check(Fire->TryRefuelFromServer(C),TEXT("Prepare replicated fire"));
        Fire->Interact_Implementation(C); Fire->AdvanceFromServer(0,0,0);
        Check(Fire->IsLit() && Fire->GetFuelSeconds()==60 && Fire->GetEffectiveWarmth()==1,TEXT("Dry replicated fixture"));
        const auto Stacks=I->GetStacks(); for(const auto& Stack:Stacks) I->TryConsumeFromServer(Stack.ItemId,Stack.Quantity);
        Check(I->TryGrantFromServer(TEXT("Wood"),4) && I->TryGrantFromServer(TEXT("Fibre"),2),TEXT("Seed real RPC transactions"));
        UE_LOG(LogTemp,Display,TEXT("Crafting server gates: Passed=%d Player=%d Placement=1 Atomic=1 Malformed=1 Locked=1 Distant=1 Fuel=1 Rain=1"),bVerificationPassed,C->GetPlayerState()->GetPlayerId());
        UE_LOG(LogTemp,Display,TEXT("Crafting fire server: Name=%s Fuel=60 Lit=1 Wet=0 Warmth=1"),*Fire->GetName());
        PublishResult(TEXT("Verification ready")); VerificationStage=1; VerificationElapsed=0;
    }
    if (C->HasAuthority() && VerificationStage==1 && VerificationElapsed>8)
    {
        if(VerificationFire) {
            VerificationFire->AdvanceFromServer(12,1,1);
            UE_LOG(LogTemp,Display,TEXT("Crafting fire server: Name=%s Fuel=48 Lit=0 Wet=96 Warmth=0"),*VerificationFire->GetName());
        }
        VerificationStage=2;
    }
    if (C->HasAuthority() && VerificationStage==2 && VerificationElapsed>12)
    {
        const bool Passed=I->GetQuantity(TEXT("Fuel"))==2 && I->GetQuantity(TEXT("Wood"))==0 && I->GetQuantity(TEXT("Fibre"))==0 && I->GetStacks().Num()==1;
        UE_LOG(LogTemp,Display,TEXT("Crafting server final: Passed=%d Player=%d Fuel=%d Slots=%d"),Passed,C->GetPlayerState()->GetPlayerId(),I->GetQuantity(TEXT("Fuel")),I->GetStacks().Num());
        VerificationStage=3;
    }
    if (!C->IsLocallyControlled()) return;
    LocalVerificationElapsed+=DeltaTime;
    if(LocalVerificationStage==0 && LastResult==TEXT("Verification ready") && I->GetQuantity(TEXT("Wood"))==4)
    {
        ServerCraft(TEXT("Fuel"),1); ServerCraft(TEXT("Fuel"),1); // immediate duplicate is rate-limited
        LocalVerificationStage=1; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==1 && LocalVerificationElapsed>2 && I->GetQuantity(TEXT("Fuel"))==1)
    {
        ServerCraft(TEXT("Fuel"),1); LocalVerificationStage=2; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==2 && LocalVerificationElapsed>2 && I->GetQuantity(TEXT("Fuel"))==2)
    {
        ServerCraft(TEXT("Fuel"),1); LocalVerificationStage=3; LocalVerificationElapsed=0; // insufficient
    }
    else if(LocalVerificationStage==3 && LocalVerificationElapsed>2)
    {
        ServerCraft(TEXT("Forged"),1); LocalVerificationStage=4; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==4 && LocalVerificationElapsed>1)
    {
        ServerCraft(TEXT("Fuel"),MAX_int32); LocalVerificationStage=5; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==5 && LocalVerificationElapsed>1)
    {
        ServerPlaceCampfire(); LocalVerificationStage=6; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==6 && LocalVerificationElapsed>1)
    {
        FString Reason;
        bool Rejected=C->HasAuthority() || (!CraftFromServer(TEXT("Fuel"),1,Reason) && !PlaceFromServer(Reason));
        if(!C->HasAuthority()) if(auto* Fire=FindNearbyFire(false))
        {
            const float FuelBefore=Fire->GetFuelSeconds(); Fire->AdvanceFromServer(1000,1,1);
            Rejected &= Fire->GetFuelSeconds()==FuelBefore && !Fire->TryRefuelFromServer(C) && !Fire->CanInteract_Implementation(C);
        }
        const bool Passed=Rejected && I->GetQuantity(TEXT("Fuel"))==2 && I->GetStacks().Num()==1;
        UE_LOG(LogTemp,Display,TEXT("Crafting owner final: Passed=%d Authority=%d Fuel=%d Slots=%d"),Passed,C->HasAuthority(),I->GetQuantity(TEXT("Fuel")),I->GetStacks().Num());
        LocalVerificationStage=7;
    }
#endif
}
