#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaM7PersistenceContract.h"

namespace
{
    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    bool IsValidConstructionTransform(const FTransform& Transform)
    {
        const FQuat Rotation = Transform.GetRotation();
        return IsFiniteVector(Transform.GetLocation())
            && IsFiniteVector(Transform.GetScale3D())
            && FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y)
            && FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W)
            && FMath::IsNearlyEqual(Rotation.SizeSquared(), 1.0f, 0.01f);
    }

    bool HasUniqueConstructionIds(const TArray<FKalmalaConstructionSaveRecord>& Records)
    {
        TSet<FString> SeenIds;
        for (const FKalmalaConstructionSaveRecord& Record : Records)
        {
            if (SeenIds.Contains(Record.ConstructionId)) return false;
            SeenIds.Add(Record.ConstructionId);
        }
        return true;
    }

    int32 CountStationAttachmentRecords(const TArray<FKalmalaConstructionSaveRecord>& Records)
    {
        int32 Count = 0;
        for (const FKalmalaConstructionSaveRecord& Record : Records)
        {
            Count += FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId) ? 1 : 0;
        }
        return Count;
    }

}

void UKalmalaConstructionSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld)
{
    SchemaVersion = CurrentSchemaVersion;
    WorldConfig = InWorld;
    Records.Reset();
}

bool UKalmalaConstructionSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const
{
    return SchemaVersion == CurrentSchemaVersion && WorldConfig == InWorld;
}

bool UKalmalaConstructionSaveGame::IsValidRecord(const FKalmalaConstructionSaveRecord& Record)
{
    return Record.ConstructionId.Len() > 0 && Record.ConstructionId.Len() <= 64 && FKalmalaPlacementPreview::IsSupportedKit(Record.KitId)
        && !FKalmalaPlacementPreview::IsSessionOnlyKit(Record.KitId)
        && !Record.Transform.GetLocation().ContainsNaN() && !Record.Transform.GetRotation().ContainsNaN();
}

bool UKalmalaConstructionSaveGameV2::IsValidRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (Record.ConstructionId.Len() <= 0 || Record.ConstructionId.Len() > 64
        || !FKalmalaPlacementPreview::IsSupportedKit(Record.KitId)
        || !IsValidConstructionTransform(Record.Transform)) return false;

    if (Record.KitId == TEXT("CharcoalKilnKit"))
    {
        if (!Record.CharcoalKiln.IsValid()) return false;
    }
    else if (!Record.CharcoalKiln.IsValid() || !Record.CharcoalKiln.IsEmpty())
    {
        return false;
    }

    if (!FKalmalaPlacementPreview::IsSessionOnlyKit(Record.KitId)) return true;
    return FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId);
}

bool UKalmalaConstructionSaveGame::AddRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (!IsValidRecord(Record) || Records.Num() >= MaxRecords
        || Records.ContainsByPredicate([&Record](const auto& Existing) { return Existing.ConstructionId == Record.ConstructionId; })) return false;
    Records.Add(Record);
    return true;
}

bool UKalmalaConstructionSaveGameV2::AddStationAttachmentRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (!MatchesWorld(WorldConfig) || !IsValidRecord(Record)
        || !FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId)
        || Records.Num() >= MaxRecords
        || CountStationAttachmentRecords(Records) >= MaxStationAttachments
        || Records.ContainsByPredicate([&Record](const FKalmalaConstructionSaveRecord& Existing)
        {
            return Existing.ConstructionId == Record.ConstructionId;
        })) return false;

    Records.Add(Record);
    return true;
}

bool UKalmalaConstructionSaveGame::RemoveRecord(const FString& ConstructionId)
{
    return !ConstructionId.IsEmpty() && Records.RemoveAll([&ConstructionId](const auto& Record) { return Record.ConstructionId == ConstructionId; }) == 1;
}

void UKalmalaConstructionSaveGameV2::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld)
{
    SchemaVersion = SchemaVersionValue;
    WorldConfig = InWorld;
    Identity = FKalmalaM7SaveIdentity::ForWorld(InWorld.WorldSeed);
    Records.Reset();
}

bool UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
    USaveGame* Existing,
    const FKalmalaWorldGenerationConfig& RequestedWorld,
    UObject* Outer,
    UKalmalaConstructionSaveGameV2*& OutCandidate)
{
    OutCandidate = nullptr;
    if (!RequestedWorld.IsValid() || Outer == nullptr) return false;

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (Existing == nullptr)
    {
        Candidate = NewObject<UKalmalaConstructionSaveGameV2>(Outer);
        if (Candidate != nullptr) Candidate->InitializeForWorld(RequestedWorld);
    }
    else if (const auto* Current = Cast<UKalmalaConstructionSaveGameV2>(Existing))
    {
        if (!Current->MatchesWorld(RequestedWorld)) return false;
        Candidate = DuplicateObject<UKalmalaConstructionSaveGameV2>(Current, Outer);
    }
    else
    {
        return false;
    }

    if (Candidate == nullptr || !Candidate->MatchesWorld(RequestedWorld)) return false;
    OutCandidate = Candidate;
    return true;
}

bool UKalmalaConstructionSaveGameV2::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const
{
    if (SchemaVersion != SchemaVersionValue || WorldConfig != InWorld
        || !Identity.Matches(FKalmalaM7SaveIdentity::ForWorld(InWorld.WorldSeed))
        || Records.Num() > MaxRecords || !HasUniqueConstructionIds(Records)) return false;

    int32 AttachmentCount = 0;
    for (const FKalmalaConstructionSaveRecord& Record : Records)
    {
        if (!IsValidRecord(Record)) return false;
        AttachmentCount += FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId) ? 1 : 0;
    }
    return AttachmentCount <= MaxStationAttachments;
}

bool UKalmalaConstructionSaveGameV2::AddRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (!MatchesWorld(WorldConfig) || !IsValidRecord(Record) || FKalmalaPlacementPreview::IsSessionOnlyKit(Record.KitId)
        || Records.Num() >= MaxRecords
        || Records.ContainsByPredicate([&Record](const FKalmalaConstructionSaveRecord& Existing)
        {
            return Existing.ConstructionId == Record.ConstructionId;
        })) return false;
    Records.Add(Record);
    return true;
}

bool UKalmalaConstructionSaveGameV2::RemoveRecord(const FString& ConstructionId)
{
    if (!MatchesWorld(WorldConfig) || ConstructionId.IsEmpty()) return false;

    const int32 RecordIndex = Records.IndexOfByPredicate([&ConstructionId](const FKalmalaConstructionSaveRecord& Record)
    {
        return Record.ConstructionId == ConstructionId;
    });
    if (RecordIndex == INDEX_NONE) return false;

    const FKalmalaConstructionSaveRecord& Record = Records[RecordIndex];
    if (Record.KitId == TEXT("CharcoalKilnKit") && !Record.CharcoalKiln.IsEmpty()) return false;

    Records.RemoveAt(RecordIndex);
    return true;
}

bool UKalmalaConstructionSaveGameV2::UpdateCharcoalKilnRecord(const FString& ConstructionId,
    const FKalmalaCharcoalKilnState& State)
{
    if (!MatchesWorld(WorldConfig) || ConstructionId.IsEmpty() || !State.IsValid()) return false;
    FKalmalaConstructionSaveRecord* Record = Records.FindByPredicate(
        [&ConstructionId](FKalmalaConstructionSaveRecord& Candidate)
        {
            return Candidate.ConstructionId == ConstructionId && Candidate.KitId == TEXT("CharcoalKilnKit");
        });
    if (!Record) return false;
    const FKalmalaCharcoalKilnState Previous = Record->CharcoalKiln;
    Record->CharcoalKiln = State;
    if (!MatchesWorld(WorldConfig))
    {
        Record->CharcoalKiln = Previous;
        return false;
    }
    return true;
}
