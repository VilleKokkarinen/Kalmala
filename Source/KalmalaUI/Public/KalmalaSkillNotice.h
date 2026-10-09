#pragma once
#include "CoreMinimal.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaSupportMagicComponent.h"

enum class EKalmalaNoticeKind : uint8
{
    Skill,
    ItemGain,
    Discovery,
    Combat,
    Support
};

struct FKalmalaSkillNotice
{
    EKalmalaSkill Skill = EKalmalaSkill::None;
    int32 Level = 1;
    float Remaining = 0;
    FName ItemId;
    int32 Quantity = 0;
    FString DiscoveryText;
    FString ActionText;
    EKalmalaNoticeKind Kind = EKalmalaNoticeKind::Skill;
};

/** Owner-local notification queue; initial state is silently baselined for every source. */
class KALMALAUI_API FKalmalaSkillNoticeQueue
{
public:
    static constexpr int32 MaxRows = 3;
    void Reset();
    bool Observe(const TArray<FKalmalaSkillState>& Snapshot, float Lifetime);
    bool ObserveGains(const TArray<FKalmalaItemGainReceipt>& Receipts, float Lifetime);
    bool ObserveDiscovery(uint32 Serial, EKalmalaDiscoveryFeedback Feedback, const FString& Label, float Lifetime);
    bool ObserveCombat(uint32 Serial, EKalmalaCombatFeedback Feedback, float Lifetime);
    bool ObserveSupport(uint32 Serial, EKalmalaSupportFeedback Feedback, float Lifetime);
    void Tick(float DeltaTime);
    const TArray<FKalmalaSkillNotice>& GetRows() const { return Rows; }
private:
    TMap<EKalmalaSkill, int32> Levels;
    TArray<FKalmalaSkillNotice> Rows;
    int64 LastGainSequence = 0;
    bool bGainBaseline = false;
    uint32 LastDiscoverySerial = 0;
    bool bDiscoveryBaseline = false;
    uint32 LastCombatSerial = 0;
    bool bCombatBaseline = false;
    uint32 LastSupportSerial = 0;
    bool bSupportBaseline = false;
};
