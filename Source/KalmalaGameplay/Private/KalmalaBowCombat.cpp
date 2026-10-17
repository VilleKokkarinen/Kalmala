#include "KalmalaCombatComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaArrowProjectile.h"
#include "KalmalaGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

float UKalmalaCombatComponent::CalculateBowDrawProgress(float Elapsed, float Duration, float ArcherySpeedMultiplier)
{
    if (!FMath::IsFinite(Elapsed) || !FMath::IsFinite(Duration) || Duration <= 0
        || !FMath::IsFinite(ArcherySpeedMultiplier) || ArcherySpeedMultiplier < 1 || ArcherySpeedMultiplier > 4) return 0;
    return FMath::Clamp(Elapsed * ArcherySpeedMultiplier / Duration, 0.0f, 1.0f);
}
float UKalmalaCombatComponent::GetBowCrosshairRadius(float Progress)
{ return FMath::Lerp(50.0f, 4.0f, FMath::Clamp(FMath::IsFinite(Progress) ? Progress : 0.0f, 0.0f, 1.0f)); }
float UKalmalaCombatComponent::GetBowDrawDuration() const
{
    const auto* Pawn = Cast<AKalmalaCharacter>(GetOwner());
    const auto* Profile = Pawn ? UKalmalaToolCatalogue::Get()->FindProfile(Pawn->GetInventoryComponent()->GetActiveItem()) : nullptr;
    return Profile ? Profile->DrawDuration : 0;
}
int32 UKalmalaCombatComponent::FindArrowStack() const
{
    const auto* Pawn = Cast<AKalmalaCharacter>(GetOwner());
    if (!Pawn || !Pawn->GetInventoryComponent()) return INDEX_NONE;
    const auto& Stacks = Pawn->GetInventoryComponent()->GetStacks();
    const auto IsArrow = [](const auto& Stack) { return Stack.ItemId == TEXT("WoodenArrow") && Stack.Quantity > 0; };
    const int32 Preferred = Stacks.IndexOfByPredicate([&](const auto& Stack)
        { return IsArrow(Stack) && Stack.InstanceGuid == PreferredArrowStack; });
    return Preferred != INDEX_NONE ? Preferred : Stacks.IndexOfByPredicate(IsArrow);
}
bool UKalmalaCombatComponent::CanDrawBow() const
{
    const auto* Pawn = Cast<AKalmalaCharacter>(GetOwner());
    if (!Pawn || !GetWorld() || Pawn->GetHealth() <= 1 || !FMath::IsFinite(Pawn->GetHealth())) return false;
    const auto* Inventory = Pawn->GetInventoryComponent();
    if (!Inventory || Inventory->GetActiveItem() != TEXT("Bow") || FindArrowStack() == INDEX_NONE) return false;
    const int32 Maximum = FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("Bow"), Pawn->GetCarriedToolLevel(TEXT("Bow")));
    return Maximum > 0 && Pawn->GetToolDurability(TEXT("Bow")) > 0 && Pawn->GetToolDurability(TEXT("Bow")) <= Maximum
        && GetBowDrawDuration() >= 0.05f && GetBowDrawDuration() <= 10
        && (!bBowDrawing || !Pawn->HasAuthority() || Inventory->GetMainHandGuid() == DrawingBowGuid);
}
bool UKalmalaCombatComponent::IsBowDrawing() const
{
    const auto* Pawn = Cast<APawn>(GetOwner());
    return Pawn && Pawn->IsLocallyControlled() ? CanDrawBow() && (bLocalBowDrawing || bBowDrawing) : bBowDrawing;
}
float UKalmalaCombatComponent::GetBowDrawProgress() const
{
    if (!IsBowDrawing()) return 0;
    const auto* Pawn = Cast<APawn>(GetOwner());
    const auto* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const bool bLocal = Pawn && Pawn->IsLocallyControlled() && bLocalBowDrawing;
    return CalculateBowDrawProgress(bLocal ? GetWorld()->GetTimeSeconds() - LocalBowDrawStart : Now - BowDrawStartTime, GetBowDrawDuration());
}
void UKalmalaCombatComponent::StartBowDraw(uint32 Sequence)
{
    if (!CanDrawBow() || ActionPhase != EKalmalaCombatActionPhase::Idle) return;
    bLocalBowDrawing = true; LocalBowDrawStart = GetWorld()->GetTimeSeconds(); ServerBeginBowDraw(Sequence);
}
void UKalmalaCombatComponent::ReleaseBowDraw(uint32 Sequence, bool bCancel)
{
    if (!bLocalBowDrawing && !bBowDrawing) return;
    bLocalBowDrawing = false; ServerReleaseBow(Sequence, bCancel);
}
void UKalmalaCombatComponent::ServerPreferArrowStack_Implementation(FGuid Guid)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    if (!Guid.IsValid()) { PreferredArrowStack.Invalidate(); GetOwner()->ForceNetUpdate(); return; }
    const auto* Pawn = Cast<AKalmalaCharacter>(GetOwner());
    if (!Pawn) return;
    for (const auto& Stack : Pawn->GetInventoryComponent()->GetStacks())
        if (Stack.ItemId == TEXT("WoodenArrow") && Stack.Quantity > 0
            && (Stack.InstanceGuid == Guid || Stack.ItemGuids.Contains(Guid)))
        { PreferredArrowStack = Stack.InstanceGuid; GetOwner()->ForceNetUpdate(); return; }
}
void UKalmalaCombatComponent::CancelBowDrawFromServer()
{
    bBowDrawing = false; bLocalBowDrawing = false; DrawingBowGuid.Invalidate(); GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::ServerBeginBowDraw_Implementation(uint32 Sequence)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || bBowDrawing || !CanDrawBow()
        || Sequence == 0 || Sequence <= LastRequestSequence || ActionPhase != EKalmalaCombatActionPhase::Idle) return;
    LastRequestSequence = Sequence;
    DrawingBowGuid = Cast<AKalmalaCharacter>(GetOwner())->GetInventoryComponent()->GetMainHandGuid();
    const auto* State = GetWorld()->GetGameState();
    BowDrawStartTime = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    bBowDrawing = true; GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::ServerReleaseBow_Implementation(uint32 Sequence, bool bCancel)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !bBowDrawing || Sequence == 0 || Sequence <= LastRequestSequence) return;
    LastRequestSequence = Sequence;
    const bool bCanFire = !bCancel && CanDrawBow();
    const auto* State = GetWorld()->GetGameState();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const float Progress = CalculateBowDrawProgress(Now - BowDrawStartTime, GetBowDrawDuration());
    CancelBowDrawFromServer();
    if (!bCanFire) return;
    auto* Pawn = Cast<AKalmalaCharacter>(GetOwner());
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    const int32 StackIndex = FindArrowStack();
    int32* Condition = Pawn->FindToolDurabilityFromServer(Inventory->GetMainHandGuid());
    if (!Mode || StackIndex == INDEX_NONE || !Condition || *Condition <= 0) return;
    const auto Before = Inventory->GetStacks();
    auto After = Before;
    auto& Stack = After[StackIndex];
    --Stack.Quantity;
    if (!Stack.ItemGuids.IsEmpty()) Stack.ItemGuids.RemoveAt(0);
    const bool bStackRemains = Stack.Quantity > 0;
    if (!bStackRemains) After.RemoveAt(StackIndex);
    // Stack row identity is stable while individual unit GUIDs are consumed.
    const FGuid NextPreference = bStackRemains ? After[StackIndex].InstanceGuid : FGuid();
    FVector View = Pawn->GetActorLocation() + FVector(0, 0, 35);
    FRotator Aim = Pawn->GetBaseAimRotation();
    if (Pawn->GetController()) Pawn->GetController()->GetPlayerViewPoint(View, Aim);
    if (View.ContainsNaN() || Aim.ContainsNaN()) return;
    const FVector Start = Pawn->GetActorLocation() + Pawn->GetActorRightVector() * 20 + FVector(0, 0, 35);
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BowAim), true, Pawn);
    const FVector End = View + Aim.Vector() * 10000;
    const FVector AimPoint = GetWorld()->LineTraceSingleByChannel(Hit, View, End, ECC_Visibility, Query) ? Hit.ImpactPoint : End;
    const FVector Direction = FMath::VRandCone((AimPoint - Start).GetSafeNormal(), FMath::DegreesToRadians(FMath::Lerp(8.0f, 0.25f, Progress)));
    auto* Arrow = GetWorld()->SpawnActorDeferred<AKalmalaArrowProjectile>(AKalmalaArrowProjectile::StaticClass(),
        FTransform(Direction.Rotation(), Start), Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Arrow) return;
    if (!Inventory->TryCommitStacksFromServer(Before, After)) { Arrow->Destroy(); return; }
    --*Condition;
    if (!Mode->PersistPlayerStateFromServer(Pawn))
    { ++*Condition; Inventory->TryCommitStacksFromServer(After, Before); Arrow->Destroy(); return; }
    if (PreferredArrowStack == Before[StackIndex].InstanceGuid) PreferredArrowStack = NextPreference;
    Arrow->InitializeShot(Direction, Progress);
    Arrow->FinishSpawning(FTransform(Direction.Rotation(), Start));
    ++ActionSerial; BeginRecovery();
}

void UKalmalaCombatComponent::OnRep_BowDrawing() { if (!bBowDrawing) bLocalBowDrawing = false; }
void UKalmalaCombatComponent::CancelLocalBowDraw()
{ if (bLocalBowDrawing) { bLocalBowDrawing = false; ServerCancelBowDraw(); } }
void UKalmalaCombatComponent::ServerCancelBowDraw_Implementation()
{ if (GetOwner() && GetOwner()->HasAuthority() && bBowDrawing) CancelBowDrawFromServer(); }
