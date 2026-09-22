#include "KalmalaSkillProgressionContract.h"

namespace KalmalaSkillProgression
{
    constexpr uint8 FirstTierMask = static_cast<uint8>(EKalmalaSkillUnlock::FirstTier);
    constexpr uint8 SecondTierMask = static_cast<uint8>(EKalmalaSkillUnlock::SecondTier);
    constexpr uint8 MasteryMask = static_cast<uint8>(EKalmalaSkillUnlock::Mastery);
}

bool FKalmalaSkillState::IsValid() const
{
    return FKalmalaSkillProgressionContract::IsKnownSkill(Skill)
        && Experience >= 0
        && Experience <= FKalmalaSkillProgressionContract::MaxExperience
        && Level == FKalmalaSkillProgressionContract::GetLevelForExperience(Experience)
        && FKalmalaSkillProgressionContract::IsUnlockMaskValid(Level, UnlockMask);
}

TArray<EKalmalaSkill> FKalmalaSkillProgressionContract::GetAllowlistedSkills()
{
    return {
        EKalmalaSkill::Gathering,
        EKalmalaSkill::Woodcutting,
        EKalmalaSkill::Mining,
        EKalmalaSkill::Crafting,
        EKalmalaSkill::Cooking,
        EKalmalaSkill::Survival
    };
}

bool FKalmalaSkillProgressionContract::IsKnownSkill(const EKalmalaSkill Skill)
{
    return Skill >= EKalmalaSkill::Gathering && Skill <= EKalmalaSkill::Survival;
}

bool FKalmalaSkillProgressionContract::IsExperienceAwardAllowed(
    const bool bServerAuthority,
    const bool bAcceptedServerAction,
    const EKalmalaSkill Skill,
    const int32 AwardedExperience)
{
    return bServerAuthority
        && bAcceptedServerAction
        && IsKnownSkill(Skill)
        && AwardedExperience > 0
        && AwardedExperience <= MaxAwardPerAcceptedAction;
}

int32 FKalmalaSkillProgressionContract::GetLevelForExperience(const int32 Experience)
{
    const int32 ClampedExperience = FMath::Clamp(Experience, 0, MaxExperience);
    return FMath::Min(MaxLevel, (ClampedExperience / ExperiencePerLevel) + 1);
}

int32 FKalmalaSkillProgressionContract::GetExperienceForLevel(const int32 Level)
{
    return FMath::Clamp(Level - 1, 0, MaxLevel - 1) * ExperiencePerLevel;
}

uint8 FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(const int32 Level)
{
    const int32 ClampedLevel = FMath::Clamp(Level, 1, MaxLevel);
    uint8 UnlockMask = 0;
    if (ClampedLevel >= 2) UnlockMask |= KalmalaSkillProgression::FirstTierMask;
    if (ClampedLevel >= 5) UnlockMask |= KalmalaSkillProgression::SecondTierMask;
    if (ClampedLevel >= MaxLevel) UnlockMask |= KalmalaSkillProgression::MasteryMask;
    return UnlockMask;
}

bool FKalmalaSkillProgressionContract::IsUnlockMaskValid(const int32 Level, const uint8 UnlockMask)
{
    return Level >= 1 && Level <= MaxLevel && UnlockMask == GetUnlockMaskForLevel(Level);
}

bool FKalmalaSkillProgressionContract::ApplyServerAward(
    FKalmalaSkillState& State,
    const bool bServerAuthority,
    const bool bAcceptedServerAction,
    const int32 AwardedExperience)
{
    if (!State.IsValid()
        || !IsExperienceAwardAllowed(bServerAuthority, bAcceptedServerAction, State.Skill, AwardedExperience)
        || State.Experience >= MaxExperience)
    {
        return false;
    }

    const int32 NewExperience = FMath::Min(MaxExperience, State.Experience + AwardedExperience);
    if (NewExperience == State.Experience)
    {
        return false;
    }

    State.Experience = NewExperience;
    State.Level = GetLevelForExperience(State.Experience);
    State.UnlockMask = GetUnlockMaskForLevel(State.Level);
    return true;
}

void FKalmalaSkillProgressionLedger::Initialize()
{
    Skills.Reset();
    for (const EKalmalaSkill Skill : FKalmalaSkillProgressionContract::GetAllowlistedSkills())
    {
        FKalmalaSkillState& State = Skills.AddDefaulted_GetRef();
        State.Skill = Skill;
        State.Experience = 0;
        State.Level = 1;
        State.UnlockMask = 0;
    }
}

FKalmalaSkillState* FKalmalaSkillProgressionLedger::Find(const EKalmalaSkill Skill)
{
    return Skills.FindByPredicate([Skill](const FKalmalaSkillState& State) { return State.Skill == Skill; });
}

const FKalmalaSkillState* FKalmalaSkillProgressionLedger::Find(const EKalmalaSkill Skill) const
{
    return Skills.FindByPredicate([Skill](const FKalmalaSkillState& State) { return State.Skill == Skill; });
}

bool FKalmalaSkillProgressionLedger::IsValid() const
{
    const TArray<EKalmalaSkill> AllowlistedSkills = FKalmalaSkillProgressionContract::GetAllowlistedSkills();
    if (Skills.Num() != AllowlistedSkills.Num()) return false;

    for (const EKalmalaSkill Skill : AllowlistedSkills)
    {
        const FKalmalaSkillState* State = Find(Skill);
        if (!State || !State->IsValid()) return false;
    }
    return true;
}

bool FKalmalaSkillProgressionLedger::AwardExperienceFromServer(
    const EKalmalaSkill Skill,
    const bool bServerAuthority,
    const bool bAcceptedServerAction,
    const int32 AwardedExperience)
{
    if (!IsValid()) return false;
    FKalmalaSkillState* State = Find(Skill);
    return State && FKalmalaSkillProgressionContract::ApplyServerAward(
        *State, bServerAuthority, bAcceptedServerAction, AwardedExperience);
}
