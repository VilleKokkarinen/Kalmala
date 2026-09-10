#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UKalmalaCraftingComponent::UKalmalaCraftingComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .25f;
}

AKalmalaCharacter* UKalmalaCraftingComponent::GetCharacter() const { return Cast<AKalmalaCharacter>(GetOwner()); }

void UKalmalaCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, LastResult, COND_OwnerOnly);
}

bool UKalmalaCraftingComponent::AcceptRequest()
{
    auto* Character = GetCharacter();
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRequestTime) { PublishResult(TEXT("Please wait before the next action")); return false; }
    NextRequestTime = Now + .2;
    return true;
}

void UKalmalaCraftingComponent::PublishResult(const FString& Result)
{
    if (GetOwner() && GetOwner()->HasAuthority()) { LastResult = Result.Left(160); GetOwner()->ForceNetUpdate(); }
}

AKalmalaCampfire* UKalmalaCraftingComponent::FindNearbyFire(bool bRequireUsable) const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaCampfire* Closest = nullptr; double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It) || (bRequireUsable && !It->CanUse(Character))) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (Distance <= Best) { Best = Distance; Closest = *It; }
    }
    return Closest;
}

bool UKalmalaCraftingComponent::CraftFromServer(FName RecipeId, int32 Batch, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const auto* Recipe = GetDefault<UKalmalaRecipeCatalogue>()->Find(RecipeId);
    Reason = TEXT("Unknown or disabled recipe");
    if (!Recipe || !Recipe->bEnabled) return false;
    TArray<FKalmalaInventoryStack> Costs; int32 OutputCount;
    Reason = TEXT("Invalid batch quantity");
    if (!UKalmalaRecipeCatalogue::Scale(*Recipe, Batch, Costs, OutputCount)) return false;
    if (Recipe->bRequiresCampfire && !FindNearbyFire(true)) { Reason = TEXT("Need a usable hearth within 2.5 m"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!Inventory || !Inventory->TryExchangeFromServer(Costs, Recipe->Output, OutputCount, Reason)) return false;
    Reason = FString::Printf(TEXT("Crafted %d %s"), OutputCount, *GetDefault<UKalmalaItemCatalogue>()->FindItem(Recipe->Output)->DisplayName);
    return true;
}

void UKalmalaCraftingComponent::ServerCraft_Implementation(FName RecipeId, int32 Batch)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted=CraftFromServer(RecipeId, Batch, Reason); PublishResult(Reason);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
        UE_LOG(LogTemp,Display,TEXT("Crafting RPC: Recipe=%s Batch=%d Accepted=%d"),*RecipeId.ToString(),Batch,Accepted);
#endif
}

bool UKalmalaCraftingComponent::PlaceFromServer(FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) { Reason = TEXT("Waiting for world"); return false; }
    int32 Count = 0;
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It) if (IsValid(*It)) ++Count;
    if (Count >= 32) { Reason = TEXT("Session hearth limit reached (32)"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    const TArray<FKalmalaInventoryStack> Costs = {{TEXT("CampfireKit"),1},{TEXT("Fuel"),1}};
    TArray<FKalmalaInventoryStack> Preview;
    if (!Inventory || !UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Costs, NAME_None, 0, Preview, Reason)) return false;
    const FVector Forward = FRotator(0, Character->GetActorRotation().Yaw, 0).Vector();
    const FVector Probe = Character->GetActorLocation() + Forward * 165;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampfirePlacement), false, Character);
    FHitResult Ground;
    Reason = TEXT("Need clear, dry, gently sloping ground ahead");
    if (Probe.ContainsNaN() || !GetWorld()->LineTraceSingleByChannel(Ground, Probe + FVector(0,0,100), Probe-FVector(0,0,350), ECC_Visibility, Query)
        || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor()) || Ground.ImpactNormal.Z < .85f
        || FVector::DistSquared(Character->GetActorLocation(), Ground.ImpactPoint) > FMath::Square(250.0)) return false;
    const auto& Config = State->GetWorldGenerationConfig();
    if (FKalmalaOceanSampler::Sample(Config, FVector2D(Ground.ImpactPoint)).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, FVector2D(Ground.ImpactPoint))
        || (Config.GeneratorRevision >= 3 && FKalmalaRegionalGeneration::Sample(Config, FVector2D(Ground.ImpactPoint)).bHasWater)) return false;
    const FVector Location = Ground.ImpactPoint + FVector(0,0,56);
    if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(54), Query)) return false;
    // Allocation precedes payment; deferred construction has no gameplay callbacks before payment.
    auto* Fire = GetWorld()->SpawnActorDeferred<AKalmalaCampfire>(AKalmalaCampfire::StaticClass(), FTransform(Location), nullptr, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Fire) { Reason = TEXT("Could not allocate hearth"); return false; }
    if (!Inventory->TryExchangeFromServer(Costs, NAME_None, 0, Reason)) { Fire->Destroy(); return false; }
    Fire->InitializePaidFromServer(Character);
    Fire->FinishSpawning(FTransform(Location));
    Reason = TEXT("Placed hearth with 60 seconds of fuel; light it nearby");
    return true;
}

bool UKalmalaCraftingComponent::PlaceConstructionFromServer(const FName KitId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    if (KitId == TEXT("CampfireKit")) return PlaceFromServer(Reason);
    Reason = TEXT("Unknown construction kit");
    if (!FKalmalaPlacementPreview::IsSupportedKit(KitId)) return false;
    const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), Character, KitId);
    Reason = Preview.Message;
    if (!Preview.bIsValid || FVector::DistSquared(Character->GetActorLocation(), Preview.Location) > FMath::Square(250.0f)) return false;
    const FRotator Rotation(0.0f, Character->GetActorRotation().Yaw, 0.0f);
    if (Rotation.ContainsNaN()) { Reason = TEXT("Invalid placement rotation"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    const TArray<FKalmalaInventoryStack> Cost = {{KitId, 1}};
    TArray<FKalmalaInventoryStack> Scratch;
    if (!Inventory || !UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Cost, NAME_None, 0, Scratch, Reason)) return false;
    const FTransform Transform(Rotation, Preview.Location);
    auto* Construction = GetWorld()->SpawnActorDeferred<AKalmalaConstructionActor>(AKalmalaConstructionActor::StaticClass(), Transform, nullptr, Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Construction) { Reason = TEXT("Could not allocate construction"); return false; }
    if (!Inventory->TryExchangeFromServer(Cost, NAME_None, 0, Reason)) { Construction->Destroy(); return false; }
    Construction->InitializeFromServer(KitId);
    Construction->FinishSpawning(Transform);
    Reason = TEXT("Placed construction; server accepted the kit and ground");
    return true;
}

void UKalmalaCraftingComponent::ServerPlaceCampfire_Implementation()
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted=PlaceFromServer(Reason); PublishResult(Reason);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
        UE_LOG(LogTemp,Display,TEXT("Crafting placement RPC: Accepted=%d"),Accepted);
#endif
}

void UKalmalaCraftingComponent::ServerPlaceConstruction_Implementation(const FName KitId)
{
    if (!AcceptRequest()) return;
    FString Reason; PlaceConstructionFromServer(KitId, Reason); PublishResult(Reason);
}

void UKalmalaCraftingComponent::ServerRefuel_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    PublishResult(Fire && Fire->TryRefuelFromServer(GetCharacter()) ? TEXT("Added one ember bundle (60 seconds)")
        : TEXT("Need a usable nearby hearth, one fuel bundle and 60 seconds of free capacity"));
}

void UKalmalaCraftingComponent::ServerLight_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    if (Fire && Fire->CanInteract_Implementation(GetCharacter())) { Fire->Interact_Implementation(GetCharacter()); PublishResult(TEXT("Hearth lit")); }
    else PublishResult(TEXT("Need a usable nearby unlit hearth with dry fuel"));
}

FString UKalmalaCraftingComponent::GetRecipeAvailability(FName Id) const
{
    const auto* R = GetDefault<UKalmalaRecipeCatalogue>()->Find(Id);
    if (!R || !R->bEnabled) return TEXT("Recipe unavailable");
    if (R->bRequiresCampfire && !FindNearbyFire(true)) return TEXT("Need a usable hearth within 2.5 m");
    auto* C = GetCharacter(); auto* Inv = C ? C->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Inv) return TEXT("Waiting for pack");
    TArray<FKalmalaInventoryStack> Next; FString Reason;
    UKalmalaInventoryComponent::BuildExchange(Inv->GetStacks(), R->Ingredients, R->Output, R->OutputCount, Next, Reason);
    return Reason;
}

FString UKalmalaCraftingComponent::GetRecipeDescription(FName Id) const
{
    const auto* R = GetDefault<UKalmalaRecipeCatalogue>()->Find(Id);
    if (!R) return TEXT("Unknown recipe");
    FString Text = R->DisplayName + TEXT("\nCost: ");
    for (const auto& Cost : R->Ingredients)
        Text += FString::Printf(TEXT("%d %s  "), Cost.Quantity, *GetDefault<UKalmalaItemCatalogue>()->FindItem(Cost.ItemId)->DisplayName);
    Text += FString::Printf(TEXT("\nOutput: %d (stack limit %d)"),R->OutputCount,GetDefault<UKalmalaItemCatalogue>()->FindItem(R->Output)->MaxStack);
    return Text + (R->bRequiresCampfire ? TEXT("\nStation: nearby usable hearth") : TEXT("\nHandcrafted; no station"));
}

FString UKalmalaCraftingComponent::GetNearbyFireText() const
{
    const auto* Fire = FindNearbyFire(false);
    return Fire ? Fire->GetStatusText() : TEXT("No hearth within 2.5 m");
}

void UKalmalaCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest"))) RunVerification(DeltaTime);
#endif
}
