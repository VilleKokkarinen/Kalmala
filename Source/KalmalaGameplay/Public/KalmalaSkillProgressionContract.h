#pragma once

#include "CoreMinimal.h"
#include "KalmalaSkillProgressionContract.generated.h"

/** The first bounded, original skill set for M7 progression. */
UENUM(BlueprintType)
enum class EKalmalaSkill : uint8
{
    None,
    Gathering,
    Woodcutting,
    Mining,
    Crafting,
    Cooking,
    Survival
};

/** Unlock tiers are derived from server-owned level, never submitted by a client. */
UENUM(BlueprintType)
enum class EKalmalaSkillUnlock : uint8
{
    None = 0,
    FirstTier = 1 << 0,
    SecondTier = 1 << 1,
    Mastery = 1 << 2
};

USTRUCT(BlueprintType)
struct KALMALAGAMEPLAY_API FKalmalaSkillState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    EKalmalaSkill Skill = EKalmalaSkill::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 Experience = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 Level = 1;

    /** Bitmask of EKalmalaSkillUnlock values derived from Level. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    uint8 UnlockMask = 0;

    bool IsValid() const;
};

/**
 * Server-owned progression rules. Awarded experience must come from an
 * accepted server action; there is intentionally no client RPC or client
 * supplied progression value in this contract.
 */
class KALMALAGAMEPLAY_API FKalmalaSkillProgressionContract
{
public:
    static constexpr int32 MaxLevel = 10;
    static constexpr int32 ExperiencePerLevel = 100;
    static constexpr int32 MaxExperience = MaxLevel * ExperiencePerLevel;
    static constexpr int32 MaxAwardPerAcceptedAction = 25;

    static TArray<EKalmalaSkill> GetAllowlistedSkills();
    static bool IsKnownSkill(EKalmalaSkill Skill);
    static bool IsExperienceAwardAllowed(
        bool bServerAuthority,
        bool bAcceptedServerAction,
        EKalmalaSkill Skill,
        int32 AwardedExperience);
    static int32 GetLevelForExperience(int32 Experience);
    static int32 GetExperienceForLevel(int32 Level);
    static uint8 GetUnlockMaskForLevel(int32 Level);
    static bool IsUnlockMaskValid(int32 Level, uint8 UnlockMask);
    static bool ApplyServerAward(
        FKalmalaSkillState& State,
        bool bServerAuthority,
        bool bAcceptedServerAction,
        int32 AwardedExperience);
};

/** Transient server-owned collection used before persistence and replication integration. */
USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaSkillProgressionLedger
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FKalmalaSkillState> Skills;

    void Initialize();
    FKalmalaSkillState* Find(EKalmalaSkill Skill);
    const FKalmalaSkillState* Find(EKalmalaSkill Skill) const;
    bool IsValid() const;
    bool AwardExperienceFromServer(
        EKalmalaSkill Skill,
        bool bServerAuthority,
        bool bAcceptedServerAction,
        int32 AwardedExperience);
};
