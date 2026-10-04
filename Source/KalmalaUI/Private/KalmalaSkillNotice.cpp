#include "KalmalaSkillNotice.h"
#include "KalmalaItemCatalogue.h"

void FKalmalaSkillNoticeQueue::Reset()
{
    Levels.Reset(); Rows.Reset(); LastGainSequence = 0; bGainBaseline = false;
    LastDiscoverySerial = 0; bDiscoveryBaseline = false;
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
        Levels = MoveTemp(Next);
        Rows.RemoveAll([](const auto& Row) { return Row.ItemId.IsNone() && Row.DiscoveryText.IsEmpty(); });
        return true;
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

bool FKalmalaSkillNoticeQueue::ObserveGains(const TArray<FKalmalaItemGainReceipt>& Receipts, float Lifetime)
{
    if (Receipts.Num() > UKalmalaInventoryComponent::MaxGainReceipts) return false;
    int64 Previous = 0;
    for (const auto& Receipt : Receipts)
    {
        if (Receipt.Sequence <= Previous || !UKalmalaItemCatalogue::Get()->IsValidStack(Receipt.ItemId, Receipt.Quantity))
            return false;
        Previous = Receipt.Sequence;
    }
    if (!bGainBaseline)
    {
        LastGainSequence = Previous; bGainBaseline = true; return true;
    }
    const float Duration = FMath::IsFinite(Lifetime) ? FMath::Clamp(Lifetime, 1.f, 10.f) : 4.f;
    for (const auto& Receipt : Receipts)
    {
        if (Receipt.Sequence <= LastGainSequence) continue;
        auto* Existing = Rows.FindByPredicate([&Receipt](const auto& Row) { return Row.ItemId == Receipt.ItemId; });
        if (Existing)
        {
            Existing->Quantity = static_cast<int32>(FMath::Min<int64>(MAX_int32,
                static_cast<int64>(Existing->Quantity) + Receipt.Quantity));
            Existing->Remaining = Duration;
        }
        else
        {
            if (Rows.Num() == MaxRows) Rows.RemoveAt(0);
            FKalmalaSkillNotice Row; Row.ItemId = Receipt.ItemId; Row.Quantity = Receipt.Quantity; Row.Remaining = Duration;
            Rows.Add(Row);
        }
        LastGainSequence = Receipt.Sequence;
    }
    return true;
}

bool FKalmalaSkillNoticeQueue::ObserveDiscovery(
    uint32 Serial, EKalmalaDiscoveryFeedback Feedback, const FString& Label, float Lifetime)
{
    if (!bDiscoveryBaseline)
    {
        LastDiscoverySerial = Serial;
        bDiscoveryBaseline = true;
        return true;
    }
    if (Serial < LastDiscoverySerial)
    {
        LastDiscoverySerial = Serial;
        return true;
    }
    if (Serial == LastDiscoverySerial) return true;

    LastDiscoverySerial = Serial;
    if (Feedback != EKalmalaDiscoveryFeedback::LandmarkFound && Feedback != EKalmalaDiscoveryFeedback::ScrollFound)
        return true;

    const float Duration = FMath::IsFinite(Lifetime) ? FMath::Clamp(Lifetime, 1.f, 10.f) : 4.f;
    FKalmalaSkillNotice Row;
    Row.Remaining = Duration;
    Row.DiscoveryText = Label.TrimStartAndEnd().Left(48);
    if (Row.DiscoveryText.IsEmpty())
    {
        Row.DiscoveryText = Feedback == EKalmalaDiscoveryFeedback::ScrollFound
            ? TEXT("Scroll found") : TEXT("Discovery found");
    }
    if (Rows.Num() == MaxRows) Rows.RemoveAt(0);
    Rows.Add(MoveTemp(Row));
    return true;
}

void FKalmalaSkillNoticeQueue::Tick(float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0) return;
    for (auto& Row : Rows) Row.Remaining -= DeltaTime;
    Rows.RemoveAll([](const auto& Row) { return Row.Remaining <= 0; });
}
