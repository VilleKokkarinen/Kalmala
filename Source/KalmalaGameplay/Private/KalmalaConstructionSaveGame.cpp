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

    int32 CountDryingLineRecords(const TArray<FKalmalaConstructionSaveRecord>& Records)
    {
        int32 Count = 0;
        for (const FKalmalaConstructionSaveRecord& Record : Records)
        {
            Count += Record.KitId == TEXT("DryingLineKit") ? 1 : 0;
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

    if (!FKalmalaPlacementPreview::IsSessionOnlyKit(Record.KitId)) return true;
    return FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId)
        || Record.KitId == TEXT("DryingLineKit");
}

bool UKalmalaConstructionSaveGameV2::TryMigrateSchema1(
    const UKalmalaConstructionSaveGame* Legacy,
    const FKalmalaWorldGenerationConfig& RequestedWorld,
    UObject* Outer,
    UKalmalaConstructionSaveGameV2*& OutMigrated)
{
    OutMigrated = nullptr;
    if (Legacy == nullptr || Legacy->GetSchemaVersion() != UKalmalaConstructionSaveGame::CurrentSchemaVersion
        || !RequestedWorld.IsValid() || !Legacy->MatchesWorld(RequestedWorld)
        || Legacy->GetRecords().Num() > MaxRecords || !HasUniqueConstructionIds(Legacy->GetRecords())) return false;

    for (const FKalmalaConstructionSaveRecord& Record : Legacy->GetRecords())
    {
        // Schema one had no approved M9 records. A session-only or malformed
        // record must never be silently carried into the migrated container.
        if (!UKalmalaConstructionSaveGame::IsValidRecord(Record) || !IsValidRecord(Record)) return false;
    }

    UKalmalaConstructionSaveGameV2* Candidate = NewObject<UKalmalaConstructionSaveGameV2>(Outer);
    if (Candidate == nullptr) return false;
    Candidate->InitializeForWorld(RequestedWorld);
    Candidate->Records = Legacy->GetRecords();
    if (!Candidate->MatchesWorld(RequestedWorld)) return false;
    OutMigrated = Candidate;
    return true;
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

bool UKalmalaConstructionSaveGameV2::AddDryingLineRecord(const FKalmalaConstructionSaveRecord& Record)
{
    if (!MatchesWorld(WorldConfig) || !IsValidRecord(Record)
        || Record.KitId != TEXT("DryingLineKit")
        || Records.Num() >= MaxRecords
        || CountDryingLineRecords(Records) >= MaxDryingLines
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
    else if (const auto* Legacy = Cast<UKalmalaConstructionSaveGame>(Existing))
    {
        if (!TryMigrateSchema1(Legacy, RequestedWorld, Outer, Candidate)) return false;
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
    int32 DryingLineCount = 0;
    for (const FKalmalaConstructionSaveRecord& Record : Records)
    {
        if (!IsValidRecord(Record)) return false;
        AttachmentCount += FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId) ? 1 : 0;
        DryingLineCount += Record.KitId == TEXT("DryingLineKit") ? 1 : 0;
    }
    return AttachmentCount <= MaxStationAttachments && DryingLineCount <= MaxDryingLines;
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
