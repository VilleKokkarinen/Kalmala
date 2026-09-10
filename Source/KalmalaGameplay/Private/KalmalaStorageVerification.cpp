#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"

void UKalmalaCraftingComponent::RunStorageVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    auto* C = GetCharacter(); if (!C || !C->GetPlayerState()) return;
    auto* Pack = C->FindComponentByClass<UKalmalaInventoryComponent>(); if (!Pack) return;
    StorageVerificationElapsed += DeltaTime;
    auto Check = [&](bool Passed, const TCHAR* Label) {
        if (!Passed) UE_LOG(LogTemp, Error, TEXT("Storage fixture FAILED: %s"), Label);
        return Passed;
    };
    auto Quantity = [](const TArray<FKalmalaInventoryStack>& Stacks) {
        const auto* Wood = Stacks.FindByPredicate([](const auto& S) { return S.ItemId == TEXT("Wood"); });
        return Wood ? Wood->Quantity : 0;
    };
    if (C->HasAuthority() && StorageVerificationStage == 0 && StorageVerificationElapsed > 3)
    {
        int32 Players = 0;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It) if (It->GetPlayerState()) ++Players;
        if (Players < 2) return;
        C->GetCharacterMovement()->StopMovementImmediately();
        C->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        FString Reason;
        AKalmalaConstructionActor* Chest = nullptr;
        const bool Restore = FParse::Param(FCommandLine::Get(), TEXT("KalmalaStorageRestore"));
        if (Restore)
        {
            TArray<AKalmalaConstructionActor*> Chests;
            for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
                if (It->GetConstructionKit() == TEXT("StorageKit")) Chests.Add(*It);
            Chests.Sort([](const auto& A, const auto& B) { return A.GetConstructionId() < B.GetConstructionId(); });
            if (!Check(Chests.Num() == 2, TEXT("Exactly two saved chests restored"))) { StorageVerificationStage = 99; return; }
            Chest = Chests[C->IsLocallyControlled() ? 0 : 1];
            C->SetActorLocation(Chest->GetActorLocation() + FVector(-165, 0, 40));
            // Restored fixtures may have another piece on this side. Find an unobstructed approach.
            for (int32 Turn = 0; Turn < 8 && !Chest->CanUse(C); ++Turn)
                C->SetActorLocation(Chest->GetActorLocation() + FRotator(0, Turn*45, 0).Vector()*165 + FVector(0,0,40));
        }
        else
        {
            Check(Pack->TryGrantFromServer(TEXT("StorageKit"), 1), TEXT("Seed paid chest kit"));
            TSet<AKalmalaConstructionActor*> Before;
            for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) Before.Add(*It);
            bool Placed = false;
            for (int32 Turn = 0; Turn < 8 && !Placed; ++Turn)
            { C->SetActorRotation(FRotator(0, Turn*45, 0)); Placed = PlaceConstructionFromServer(TEXT("StorageKit"), Reason); }
            for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
                if (!Before.Contains(*It)) Chest = *It;
            if (!Check(Placed && Chest && Pack->GetQuantity(TEXT("StorageKit")) == 0, TEXT("Normal server placement pays for chest")))
            { StorageVerificationStage = 99; return; }
            Check(OpenStorageFromServer(Chest), TEXT("Open paid chest"));
            Check(Pack->TryGrantFromServer(TEXT("Wood"), 3), TEXT("Seed storage materials"));
            for (int32 N = 0; N < 3; ++N) Check(TransferStorageFromServer(TEXT("Wood"), true, Reason), TEXT("Save initial chest materials"));
            Check(Pack->TryGrantFromServer(TEXT("WorkbenchKit"), 1) && Pack->TryGrantFromServer(TEXT("ConstructionSupply"), 2), TEXT("Seed bench and assembly cost"));
            bool BenchPlaced = false;
            for (int32 Turn = 0; Turn < 8 && !BenchPlaced; ++Turn)
            { C->SetActorRotation(FRotator(0, Turn*45, 0)); BenchPlaced = PlaceConstructionFromServer(TEXT("WorkbenchKit"), Reason); }
            Check(BenchPlaced && FindNearbyWorkbench() && CraftFromServer(TEXT("Floor"), 1, Reason)
                && Pack->GetQuantity(TEXT("FloorKit")) == 1 && Pack->GetQuantity(TEXT("ConstructionSupply")) == 0, TEXT("Paid visible workbench assembles a floor without hearth"));
            Pack->TryConsumeFromServer(TEXT("FloorKit"), 1);
        }
        // Give each owner a position where the no-target inspect RPC resolves its assigned chest.
        // Merely being in range of it can select the other restored player's nearer chest.
        for (int32 Turn = 0; Turn < 16 && FindNearbyConstruction(TEXT("StorageKit")) != Chest; ++Turn)
            C->SetActorLocation(Chest->GetActorLocation() + FRotator(0, Turn*22.5f, 0).Vector()*140 + FVector(0,0,40));
        if (!Check(FindNearbyConstruction(TEXT("StorageKit")) == Chest && OpenStorageFromServer(Chest) && Quantity(StorageView) == 3,
            TEXT("Fresh/restored saved chest is nearest and contains exactly three wood")))
        { StorageVerificationStage = 99; return; }
        const FVector Original = C->GetActorLocation();
        C->SetActorLocation(Original + FVector(0,0,1000));
        Check(!TransferStorageFromServer(TEXT("Wood"), false, Reason) && !HasStorageView(), TEXT("Distant transfer rejected and view cleared"));
        C->SetActorLocation(Original);
        auto* Blocker = GetWorld()->SpawnActor<AActor>();
        auto* Box = NewObject<UBoxComponent>(Blocker); Blocker->SetRootComponent(Box); Blocker->AddInstanceComponent(Box);
        Box->SetBoxExtent(FVector(24,24,100)); Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
        Blocker->SetActorLocation((C->GetPawnViewLocation() + Chest->GetActorLocation())*.5);
        Check(!OpenStorageFromServer(Chest), TEXT("Opaque obstruction rejects chest inspection"));
        Blocker->Destroy();
        Check(OpenStorageFromServer(Chest), TEXT("Unblocked chest can be reopened"));
        Check(!TransferStorageFromServer(TEXT("Forged"), false, Reason) && Quantity(StorageView) == 3, TEXT("Unknown item cannot alter storage"));
        Check(Pack->TryGrantFromServer(TEXT("Wood"), 1), TEXT("Seed one item for owning RPC transfer"));
        Check(!Pack->TransferStorageFromServer(StorageView, TEXT("Wood"), true, [](const auto&) { return false; }, Reason)
            && Pack->GetQuantity(TEXT("Wood")) == 1 && Quantity(StorageView) == 3, TEXT("Failed persistence leaves pack and chest unchanged"));
        UE_LOG(LogTemp, Display, TEXT("Storage server ready: Restore=%d Id=%s Wood=%d"), Restore, *Chest->GetConstructionId(), Quantity(StorageView));
        PublishResult(TEXT("Storage fixture ready")); StorageVerificationStage = 1; StorageVerificationElapsed = 0;
    }
    if (C->HasAuthority() && StorageVerificationStage == 1 && StorageVerificationElapsed > 14)
    {
        TArray<FKalmalaInventoryStack> Persisted;
        auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
        // Local close intentionally discarded ActiveStorage; locate the same nearby server-selected chest.
        auto* Chest = FindNearbyConstruction(TEXT("StorageKit"));
        const bool Passed = Mode && Chest && Mode->ReadStorage(Chest, Persisted) && Quantity(Persisted) == 3
            && Pack->GetQuantity(TEXT("Wood")) == 1 && Pack->GetStacks().Num() == 1 && !HasStorageView() && StorageView.IsEmpty();
        Check(Passed, TEXT("RPC transfers conserve saved chest and private pack; closing clears view"));
        UE_LOG(LogTemp, Display, TEXT("Storage server final: Passed=%d Wood=%d"), Passed, Quantity(Persisted));
        StorageVerificationStage = 2;
    }
    if (!C->IsLocallyControlled()) return;
    LocalVerificationElapsed += DeltaTime;
    if (LocalVerificationStage == 0 && LastResult == TEXT("Storage fixture ready") && Pack->GetQuantity(TEXT("Wood")) == 1)
    { ServerOpenStorage(); LocalVerificationStage = 1; LocalVerificationElapsed = 0; }
    else if (LocalVerificationStage == 1 && LocalVerificationElapsed > 1 && HasStorageView() && Quantity(StorageView) == 3)
    { ServerDepositStorage(TEXT("Wood")); LocalVerificationStage = 2; LocalVerificationElapsed = 0; }
    else if (LocalVerificationStage == 2 && LocalVerificationElapsed > 1 && Quantity(StorageView) == 4 && Pack->GetQuantity(TEXT("Wood")) == 0)
    { ServerWithdrawStorage(TEXT("Wood")); LocalVerificationStage = 3; LocalVerificationElapsed = 0; }
    else if (LocalVerificationStage == 3 && LocalVerificationElapsed > 1 && Quantity(StorageView) == 3 && Pack->GetQuantity(TEXT("Wood")) == 1)
    { ServerWithdrawStorage(TEXT("Forged")); LocalVerificationStage = 4; LocalVerificationElapsed = 0; }
    else if (LocalVerificationStage == 4 && LocalVerificationElapsed > 1)
    {
        bool Private = true;
        if (!C->HasAuthority())
        {
            FString Reason;
            Private &= !TransferStorageFromServer(TEXT("Wood"), false, Reason);
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It) if (*It != C && It->GetPlayerState())
            {
                const auto* Other = It->FindComponentByClass<UKalmalaCraftingComponent>();
                Private &= Other && !Other->HasStorageView() && Other->GetStorageView().IsEmpty();
            }
        }
        const bool Passed = Private && Quantity(StorageView) == 3 && Pack->GetQuantity(TEXT("Wood")) == 1;
        Check(Passed, TEXT("Owner snapshot matches; simulated peers have no chest contents"));
        UE_LOG(LogTemp, Display, TEXT("Storage owner final: Passed=%d Authority=%d Private=%d Wood=%d"), Passed, C->HasAuthority(), Private, Quantity(StorageView));
        ServerCloseStorage(); LocalVerificationStage = 5; LocalVerificationElapsed = 0;
    }
    else if (LocalVerificationStage == 5 && LocalVerificationElapsed > 1 && !HasStorageView() && StorageView.IsEmpty())
    {
        UE_LOG(LogTemp, Display, TEXT("Storage owner closed: Authority=%d Cleared=1"), C->HasAuthority());
        LocalVerificationStage = 6;
    }
#endif
}
