#include "KalmalaCombatComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaHarvestNode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

UKalmalaCombatComponent::UKalmalaCombatComponent() { PrimaryComponentTick.bCanEverTick = true; SetIsReplicatedByDefault(true); }
bool UKalmalaCombatComponent::IsAttackRequestAllowed(const bool bAuthority, const bool bNewSequence,
    const uint32 Sequence, const bool bActionIdle, const bool bSwingAvailable)
{
    // Beginning an action does not require a damage target; impact validates that separately.
    return bAuthority && bNewSequence && Sequence != 0 && bActionIdle && bSwingAvailable;
}
bool UKalmalaCombatComponent::IsAttackItemAllowed(const FName ItemId)
{
    return ItemId.IsNone() || (ItemId != TEXT("ConstructionHammer") && ItemId != TEXT("BasicTorch")
        && ItemId != TEXT("PlantingTrowel")
        && ItemId != TEXT("Bow") && ItemId != TEXT("LightBow") && ItemId != TEXT("HornBow")
        && FKalmalaToolLifecycleContract::FindDefinition(ItemId) != nullptr);
}
bool UKalmalaCombatComponent::CanSwingWithoutTarget(const FName ItemId, const int32 Level, const int32 Condition)
{
    if (ItemId.IsNone()) return true;
    const int32 Maximum = FKalmalaToolLifecycleContract::GetMaxDurability(ItemId, Level);
    return IsAttackItemAllowed(ItemId) && Maximum > 0 && Condition > 0 && Condition <= Maximum;
}
void UKalmalaCombatComponent::ServerRequestAttack_Implementation(const uint32 RequestSequence)
{
    // Every usable held item can make a melee swing; bow shooting remains separate.
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    const auto* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    const FName EquippedItem = Inventory ? Inventory->GetActiveItem() : NAME_None;
    if (!IsAttackItemAllowed(EquippedItem)) return;
    APawn* Pawn = Cast<APawn>(GetOwner()); AActor* Target = FindServerTarget();
    const bool bSwingAvailable = Character && !Character->IsWildlifeStaggered()
        && FMath::IsFinite(Character->GetHealth()) && Character->GetHealth() > 1.0f
        && CanSwingWithoutTarget(EquippedItem, Character->GetCarriedToolLevel(EquippedItem),
            Character->GetToolDurability(EquippedItem));
    const bool bAllowed = Pawn != nullptr && IsAttackRequestAllowed(Pawn->HasAuthority(), RequestSequence != 0 && RequestSequence > LastRequestSequence, RequestSequence, ActionPhase == EKalmalaCombatActionPhase::Idle && !bBowDrawing, bSwingAvailable);
    if (!bAllowed)
    {
        if (Pawn != nullptr && Pawn->HasAuthority() && GetWorld() != nullptr && GetWorld()->GetTimeSeconds() >= LastUnavailableFeedbackTime + 0.25f)
        {
            LastUnavailableFeedbackTime = GetWorld()->GetTimeSeconds();
            PublishFeedbackFromServer(EKalmalaCombatFeedback::Unavailable);
        }
        return;
    }
    LastRequestSequence = RequestSequence; ++ActionSerial; PendingTarget = Target; ActionPhase = EKalmalaCombatActionPhase::Windup; PhaseEndTime = GetWorld()->GetTimeSeconds() + WindupSeconds; GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bLocalBowDrawing && !CanDrawBow()) bLocalBowDrawing = false;
    if (GetOwner() && GetOwner()->HasAuthority() && bBowDrawing && !CanDrawBow()) CancelBowDrawFromServer();
    const bool bCombatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest"));
    const bool bMirelingPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest"));
    const bool bBoarPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest"));
    const bool bDeerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"));
    const bool bBadgerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBadgerPeerTest"));
    const bool bMoonBearPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMoonBearPeerTest"));
    const bool bRatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest"));
    const bool bViperPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaViperPeerTest"));
    const bool bGiantSnakePeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaGiantSnakePeerTest"));
    const uint32 ExpectedActionSerial = bRatPeerTest ? 1u : 4u;
    if ((bCombatPeerTest || bMirelingPeerTest || bBoarPeerTest || bDeerPeerTest || bBadgerPeerTest || bMoonBearPeerTest || bRatPeerTest || bViperPeerTest || bGiantSnakePeerTest)
        && GetOwner() != nullptr && !GetOwner()->HasAuthority())
    {
        const TCHAR* VerificationName = bRatPeerTest ? TEXT("Rat")
            : bViperPeerTest ? TEXT("Viper")
            : bGiantSnakePeerTest ? TEXT("Giant Snake")
            : bBadgerPeerTest ? TEXT("Badger")
            : bMoonBearPeerTest ? TEXT("Moon Bear")
            : bBoarPeerTest ? TEXT("Boar")
            : bDeerPeerTest ? TEXT("Deer")
            : bMirelingPeerTest ? TEXT("Mireling") : TEXT("Combat");
        const APawn* OwnerPawn = Cast<APawn>(GetOwner());
        if (!bClientCombatVerificationActionLogged && ActionSerial >= ExpectedActionSerial)
        {
            bClientCombatVerificationActionLogged = true;
            UE_LOG(LogTemp, Display, TEXT("%s verification client observed shared action serial=%u."), VerificationName, ActionSerial);
        }
        if (!bClientCombatVerificationRejectionLogged && OwnerPawn != nullptr && OwnerPawn->IsLocallyControlled() && FeedbackSerial > 0 && Feedback == EKalmalaCombatFeedback::Unavailable)
        {
            bClientCombatVerificationRejectionLogged = true;
            UE_LOG(LogTemp, Display, TEXT("%s verification client rejected invalid owned attack without target data."), VerificationName);
        }
    }
    if (!GetOwner()->HasAuthority() || ActionPhase == EKalmalaCombatActionPhase::Idle || GetWorld() == nullptr || GetWorld()->GetTimeSeconds() < PhaseEndTime) return;
    if (ActionPhase == EKalmalaCombatActionPhase::Windup)
    {
        if (AActor* Target = PendingTarget.Get(); IsServerTargetValid(Target) && FindServerTarget() == Target)
        {
            const AKalmalaCharacter* OwnerCharacter = Cast<AKalmalaCharacter>(GetOwner());
            const auto* Support = OwnerCharacter ? OwnerCharacter->GetSupportMagicComponent() : nullptr;
            const float Now = GetWorld()->GetTimeSeconds();
            const UKalmalaInventoryComponent* Inventory = OwnerCharacter ? OwnerCharacter->GetInventoryComponent() : nullptr;
            const FName EquippedItem = Inventory ? Inventory->GetActiveItem() : NAME_None;
            const int32 EquippedLevel = OwnerCharacter ? OwnerCharacter->GetCarriedToolLevel(EquippedItem) : 0;
            // Revalidate the held item and living owner after windup, including swaps/breakage.
            if (!OwnerCharacter || OwnerCharacter->IsWildlifeStaggered() || OwnerCharacter->GetHealth() <= 1.0f
                || !CanSwingWithoutTarget(EquippedItem, EquippedLevel, OwnerCharacter->GetToolDurability(EquippedItem)))
            { BeginRecovery(); return; }
            FKalmalaDamageChannels Damage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
                EquippedItem, EquippedLevel, TEXT("Basic"));
            if (!OwnerCharacter || OwnerCharacter->GetToolDurability(EquippedItem) <= 0 || Damage.Total() <= 0.0f)
            {
                Damage = {};
                Damage.Crush = UnarmedAttackDamage;
            }
            const float Multiplier = UKalmalaSupportMagicComponent::CalculateBearsVigorDamage(true,
                Support && Support->GetBearsVigorExpiry() > Now, 1.0f,
                Support ? Support->GetBearsVigorStrengthMultiplier() : 1.0f);
            Damage = Damage.Scaled(Multiplier);
            AKalmalaWildlifeSpawn* Wildlife = Cast<AKalmalaWildlifeSpawn>(Target);
            AKalmalaHarvestNode* Tree = Cast<AKalmalaHarvestNode>(Target);
            const bool bApplied = Wildlife ? Wildlife->ApplyCombatDamageFromServer(Damage, Cast<AKalmalaCharacter>(GetOwner()))
                : Tree && Tree->ApplyAxeDamageFromServer(Cast<AKalmalaCharacter>(GetOwner()), Damage);
            const bool bDefeated = bApplied && Wildlife && Wildlife->GetHealth() <= 0.0f;
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")))
            {
                UE_LOG(LogTemp, Display, TEXT("Boar verification attack execution: Applied=%d Distance=%.1f Health=%.1f."), bApplied, FVector::Dist(Cast<APawn>(GetOwner())->GetActorLocation(), Target->GetActorLocation()), Wildlife ? Wildlife->GetHealth() : 0.0f);
            }
#endif
            PublishFeedbackFromServer(bApplied ? (bDefeated ? EKalmalaCombatFeedback::Defeat : EKalmalaCombatFeedback::Hit) : EKalmalaCombatFeedback::Unavailable);
        }
        else
        {
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")))
            {
                const AActor* Pending = PendingTarget.Get();
                const APawn* OwnerPawn = Cast<APawn>(GetOwner());
                UE_LOG(LogTemp, Display, TEXT("Boar verification attack execution: target valid=%d distance=%.1f."), Pending != nullptr, Pending != nullptr && OwnerPawn != nullptr ? FVector::Dist(OwnerPawn->GetActorLocation(), Pending->GetActorLocation()) : -1.0f);
            }
#endif
            // Any accepted melee swing can miss; it spends no condition and grants nothing.
            PublishFeedbackFromServer(EKalmalaCombatFeedback::None);
        }
        BeginRecovery();
        return;
    }
    ActionPhase = EKalmalaCombatActionPhase::Idle; PendingTarget.Reset(); GetOwner()->ForceNetUpdate();
}
AActor* UKalmalaCombatComponent::FindServerTarget() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner()); if (Pawn == nullptr || GetWorld() == nullptr) return nullptr;
    const FVector Start = Pawn->GetActorLocation() + FVector(0, 0, 55); FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(KalmalaCombatTarget), false, Pawn);
    const bool bLineHit = GetWorld()->LineTraceSingleByChannel(Hit, Start,
        Start + Pawn->GetActorForwardVector() * AttackRange, ECC_Visibility, Params);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Rat verification melee trace: Hit=%d Actor=%s Start=%s End=%s."),
            bLineHit, Hit.GetActor() != nullptr ? *Hit.GetActor()->GetName() : TEXT("None"),
            *Start.ToCompactString(), *(Start + Pawn->GetActorForwardVector() * AttackRange).ToCompactString());
    }
#endif
    if (bLineHit
        && (Cast<AKalmalaWildlifeSpawn>(Hit.GetActor()) || Cast<AKalmalaHarvestNode>(Hit.GetActor()))) return Hit.GetActor();
    // The lower melee sweep reaches fallen logs and small wildlife below the ordinary aim trace.
    const FVector LowStart = Pawn->GetActorLocation() - FVector(0, 0, 30);
    const bool bLowSweepHit = GetWorld()->SweepSingleByChannel(Hit, LowStart,
        LowStart + Pawn->GetActorForwardVector() * AttackRange, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(60.0f), Params);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Rat verification melee sweep: Hit=%d Actor=%s Start=%s End=%s."),
            bLowSweepHit, Hit.GetActor() != nullptr ? *Hit.GetActor()->GetName() : TEXT("None"),
            *LowStart.ToCompactString(), *(LowStart + Pawn->GetActorForwardVector() * AttackRange).ToCompactString());
    }
#endif
    if (bLowSweepHit)
    {
        const auto* Wildlife = Cast<AKalmalaWildlifeSpawn>(Hit.GetActor());
        if (Wildlife != nullptr && Wildlife->GetArchetype() == EKalmalaWildlifeArchetype::Rat
            && !Wildlife->IsDefeated()) return Hit.GetActor();
        const auto* Tree = Cast<AKalmalaHarvestNode>(Hit.GetActor());
        if (Tree && Tree->CanReceiveAxeAttack()) return Hit.GetActor();
    }
    return nullptr;
}
bool UKalmalaCombatComponent::IsServerTargetValid(const AActor* Target) const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !IsValid(Target) || Target->GetWorld() != GetWorld()) return false;
    if (const auto* Tree = Cast<AKalmalaHarvestNode>(Target))
        return Tree->IsWithinAxeRange(Pawn->GetActorLocation(), AttackRange);
    if (const auto* Wildlife = Cast<AKalmalaWildlifeSpawn>(Target))
        return !Wildlife->IsDefeated()
            && FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(AttackRange);
    return false;
}
void UKalmalaCombatComponent::BeginRecovery() { ActionPhase = EKalmalaCombatActionPhase::Recovery; PhaseEndTime = GetWorld()->GetTimeSeconds() + RecoverySeconds; GetOwner()->ForceNetUpdate(); }
void UKalmalaCombatComponent::PublishFeedbackFromServer(const EKalmalaCombatFeedback NewFeedback)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    Feedback = NewFeedback;
    ++FeedbackSerial;
    GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UKalmalaCombatComponent, bBowDrawing);
    DOREPLIFETIME(UKalmalaCombatComponent, BowDrawStartTime);
    DOREPLIFETIME_CONDITION(UKalmalaCombatComponent, PreferredArrowStack, COND_OwnerOnly);
    DOREPLIFETIME(UKalmalaCombatComponent, ActionPhase);
    DOREPLIFETIME(UKalmalaCombatComponent, ActionSerial);
    DOREPLIFETIME_CONDITION(UKalmalaCombatComponent, Feedback, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCombatComponent, FeedbackSerial, COND_OwnerOnly);
}
