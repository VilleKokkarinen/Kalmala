#pragma once
#include "CoreMinimal.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaInventoryComponent.h"

struct FKalmalaSkillNotice
{
    EKalmalaSkill Skill = EKalmalaSkill::None;
    int32 Level = 1;
    float Remaining = 0;
    FName ItemId;
    int32 Quantity = 0;
};

/** Owner-snapshot observer: first complete state is silent, repeated state is inert. */
class KALMALAUI_API FKalmalaSkillNoticeQueue
{
public:
    static constexpr int32 MaxRows = 3;
    void Reset();
    bool Observe(const TArray<FKalmalaSkillState>& Snapshot, float Lifetime);
    bool ObserveGains(const TArray<FKalmalaItemGainReceipt>& Receipts, float Lifetime);
    void Tick(float DeltaTime);
    const TArray<FKalmalaSkillNotice>& GetRows() const { return Rows; }
private:
    TMap<EKalmalaSkill, int32> Levels;
    TArray<FKalmalaSkillNotice> Rows;
    int64 LastGainSequence = 0;
    bool bGainBaseline = false;
};
