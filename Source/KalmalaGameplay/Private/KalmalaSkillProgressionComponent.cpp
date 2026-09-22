#include "KalmalaSkillProgressionComponent.h"

#include "Net/UnrealNetwork.h"

bool FKalmalaSkillPeerPresentationState::IsValid() const
{
    return HighestLevel >= 1
        && HighestLevel <= FKalmalaSkillProgressionContract::MaxLevel
        && FKalmalaSkillProgressionContract::IsUnlockMaskValid(HighestLevel, HighestUnlockMask);
}

UKalmalaSkillProgressionComponent::UKalmalaSkillProgressionComponent()
{
    SetIsReplicatedByDefault(true);
}

void UKalmalaSkillProgressionComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        ServerLedger.Initialize();
        PublishServerState();
    }
}

FKalmalaSkillPeerPresentationState UKalmalaSkillProgressionComponent::BuildPeerPresentation(
    const FKalmalaSkillProgressionLedger& Ledger)
{
    FKalmalaSkillPeerPresentationState Presentation;
    if (!Ledger.IsValid()) return Presentation;

    for (const FKalmalaSkillState& State : Ledger.Skills)
    {
        if (!State.IsValid() || State.Level <= Presentation.HighestLevel) continue;
        Presentation.HighestLevel = static_cast<uint8>(State.Level);
        Presentation.HighestUnlockMask = State.UnlockMask;
    }
    return Presentation;
}

void UKalmalaSkillProgressionComponent::PublishServerState()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !ServerLedger.IsValid()) return;

    DetailedProgression = ServerLedger.Skills;
    PeerPresentation = BuildPeerPresentation(ServerLedger);
    GetOwner()->ForceNetUpdate();
}

bool UKalmalaSkillProgressionComponent::AwardExperienceFromAcceptedServerAction(
    const EKalmalaSkill Skill,
    const int32 AwardedExperience)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()
        || !ServerLedger.AwardExperienceFromServer(Skill, true, true, AwardedExperience)) return false;

    PublishServerState();
    return true;
}

void UKalmalaSkillProgressionComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaSkillProgressionComponent, DetailedProgression, COND_OwnerOnly);
    DOREPLIFETIME(UKalmalaSkillProgressionComponent, PeerPresentation);
}
