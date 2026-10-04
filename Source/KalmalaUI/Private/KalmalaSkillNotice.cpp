#include "KalmalaSkillNotice.h"

void FKalmalaSkillNoticeQueue::Reset()
{
    Levels.Reset(); Rows.Reset();
}

bool FKalmalaSkillNoticeQueue::Observe(const TArray<FKalmalaSkillState>& Snapshot, float Lifetime)
{
    const auto Skills = FKalmalaSkillProgressionContract::GetAllowlistedSkills();
    if (Snapshot.Num() != Skills.Num()) return false;
    TMap<EKalmalaSkill, int32> Next;
    for (const auto& State : Snapshot)
    {
        if (!State.IsValid() || !Skills.Contains(State.Skill) || Next.Contains(State.Skill)) return false;
        Next.Add(State.Skill, State.Level);
    }
    bool bBaseline = Levels.IsEmpty();
    for (const auto& Pair : Next)
        if (const auto* Previous = Levels.Find(Pair.Key); Previous && Pair.Value < *Previous) bBaseline = true;
    if (bBaseline)
    {
        Levels = MoveTemp(Next); Rows.Reset(); return true;
    }
    const float Duration = FMath::IsFinite(Lifetime) ? FMath::Clamp(Lifetime, 1.f, 10.f) : 4.f;
    // Allowlist order gives deterministic ordering even if replication reorders rows.
    for (const auto Skill : Skills)
    {
        if (Next[Skill] <= Levels[Skill]) continue;
        if (auto* Existing = Rows.FindByPredicate([Skill](const auto& Row) { return Row.Skill == Skill; }))
        {
            Existing->Level = Next[Skill]; Existing->Remaining = Duration;
        }
        else
        {
            if (Rows.Num() == MaxRows) Rows.RemoveAt(0);
            Rows.Add({Skill, Next[Skill], Duration});
        }
    }
    Levels = MoveTemp(Next);
    return true;
}

void FKalmalaSkillNoticeQueue::Tick(float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0) return;
    for (auto& Row : Rows) Row.Remaining -= DeltaTime;
    Rows.RemoveAll([](const auto& Row) { return Row.Remaining <= 0; });
}
