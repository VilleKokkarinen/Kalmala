#pragma once
#include "CoreMinimal.h"
#include "KalmalaSkillProgressionContract.h"

struct FKalmalaSkillNotice
{
    EKalmalaSkill Skill = EKalmalaSkill::None;
    int32 Level = 1;
    float Remaining = 0;
};

/** Owner-snapshot observer: first complete state is silent, repeated state is inert. */
class KALMALAUI_API FKalmalaSkillNoticeQueue
{
public:
    static constexpr int32 MaxRows = 3;
    void Reset();
    bool Observe(const TArray<FKalmalaSkillState>& Snapshot, float Lifetime);
    void Tick(float DeltaTime);
    const TArray<FKalmalaSkillNotice>& GetRows() const { return Rows; }
private:
    TMap<EKalmalaSkill, int32> Levels;
    TArray<FKalmalaSkillNotice> Rows;
};
