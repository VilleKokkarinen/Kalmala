#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaToolProgressionContract.h"

namespace
{
    bool IsValidToolRecordData(const FKalmalaPlayerToolSaveRecord& Record)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Record.ItemId);
        const FName CanonicalId = FKalmalaToolLifecycleContract::CanonicalizeItemId(Record.ItemId);
        const int32 MaxDurability = FKalmalaToolLifecycleContract::GetMaxDurability(Record.ItemId, Record.ToolLevel);
        return Definition != nullptr && CanonicalId == Definition->ItemId
            && FKalmalaToolProgressionContract::IsValidToolLevel(Record.ItemId, Record.ToolLevel)
            && Record.ToolId.IsNone() && MaxDurability > 0
            && Record.Condition >= 0 && Record.Condition <= MaxDurability;
    }

    bool IsKnownLearnedEffectId(const FString& Id)
    {
        return Id == TEXT("Effect:mending")
            || Id == TEXT("Effect:hearth-shield")
            || Id == TEXT("Effect:bears-vigor")
            || Id == TEXT("Effect:deer-call");
    }

    bool ParseCanonicalInteger(const FString& Text, int32& OutValue)
    {
        if (Text.IsEmpty() || !LexTryParseString(OutValue, *Text)) return false;
        return FString::FromInt(OutValue) == Text;
    }

    bool IsCanonicalFirstWaveDiscoveryId(const FString& Id)
    {
        if (!FKalmalaM7SparseDelta::IsValidStableId(Id)) return false;

        TArray<FString> Parts;
        Id.ParseIntoArray(Parts, TEXT(":"), false);
        if (Parts.Num() == 4 && Parts[0] == TEXT("Scroll") && Parts[1] == TEXT("1")
            && Parts[3] == TEXT("mireling-boss"))
        {
            return Parts[2] == TEXT("mending") || Parts[2] == TEXT("hearth-shield")
                || Parts[2] == TEXT("bears-vigor") || Parts[2] == TEXT("deer-call");
        }

        if (Parts.Num() != 5 || (Parts[0] != TEXT("Poi") && Parts[0] != TEXT("Scroll"))
            || Parts[1] != TEXT("1") || Parts[2].IsEmpty()) return false;

        FString XText;
        FString YText;
        int32 KeyX = 0;
        int32 KeyY = 0;
        int32 Ordinal = 0;
        if (!Parts[3].Split(TEXT(","), &XText, &YText) || YText.Contains(TEXT(","))
            || !ParseCanonicalInteger(XText, KeyX) || !ParseCanonicalInteger(YText, KeyY)
            || !ParseCanonicalInteger(Parts[4], Ordinal) || Ordinal < 0 || Ordinal >= 8) return false;

        return Id == FString::Printf(TEXT("%s:1:%s:%d,%d:%d"), *Parts[0], *Parts[2], KeyX, KeyY, Ordinal);
    }

}

void UKalmalaPlayerDiscoverySaveGame::InitializeForPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity)
{ SchemaVersion = Schema; WorldConfig = InWorld; PlayerIdentity = InPlayerIdentity; DiscoveryIds.Reset(); LearnedEffectIds.Reset(); }
bool UKalmalaPlayerDiscoverySaveGame::Matches(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity) const
{ return SchemaVersion == Schema && WorldConfig == InWorld && !InPlayerIdentity.IsEmpty() && PlayerIdentity == InPlayerIdentity; }
bool UKalmalaPlayerDiscoverySaveGame::HasDiscovery(const FString& Id) const { return !Id.IsEmpty() && DiscoveryIds.Contains(Id); }
bool UKalmalaPlayerDiscoverySaveGame::AddDiscovery(const FString& Id)
{ if (Id.IsEmpty() || DiscoveryIds.Contains(Id) || DiscoveryIds.Num() >= MaxDiscoveries) return false; DiscoveryIds.Add(Id); return true; }
void UKalmalaPlayerDiscoverySaveGame::RemoveDiscovery(const FString& Id) { DiscoveryIds.Remove(Id); }
bool UKalmalaPlayerDiscoverySaveGame::HasLearnedEffect(const FString& Id) const { return !Id.IsEmpty() && LearnedEffectIds.Contains(Id); }
bool UKalmalaPlayerDiscoverySaveGame::AddLearnedEffect(const FString& Id)
{ if ((Id != TEXT("Effect:mending") && Id != TEXT("Effect:hearth-shield") && Id != TEXT("Effect:bears-vigor") && Id != TEXT("Effect:deer-call")) || LearnedEffectIds.Contains(Id) || LearnedEffectIds.Num() >= MaxDiscoveries) return false; LearnedEffectIds.Add(Id); return true; }
void UKalmalaPlayerDiscoverySaveGame::RemoveLearnedEffect(const FString& Id) { LearnedEffectIds.Remove(Id); }

void UKalmalaPlayerDiscoverySaveGameV2::InitializeForPlayer(
    const FKalmalaWorldGenerationConfig& InWorld,
    const FString& InPlayerIdentity)
{
    SchemaVersion = SchemaVersionValue;
    WorldConfig = InWorld;
    Identity = FKalmalaM7SaveIdentity::ForPlayer(InWorld.WorldSeed, InPlayerIdentity);
    PlayerIdentity = InPlayerIdentity;
    DiscoveryIds.Reset();
    M9ClaimIds.Reset();
    LearnedEffectIds.Reset();
    ToolRecords.Reset();
}

bool UKalmalaPlayerDiscoverySaveGameV2::IsValidLegacyDiscoveryId(const FString& Id)
{
    return Id.Len() <= FKalmalaM7SparseDelta::MaxStableIdLength && IsCanonicalFirstWaveDiscoveryId(Id);
}

bool UKalmalaPlayerDiscoverySaveGameV2::IsValidM9ClaimId(const FString& Id)
{
    if (!FKalmalaM7SparseDelta::IsValidStableId(Id)) return false;

    for (const FKalmalaM9ExplorationRewardDefinition& Definition : FKalmalaM9ExplorationRewardCatalogue::GetDefinitions())
    {
        const FString Prefix = FString::Printf(TEXT("land-discovery:m9:%d:%s:"),
            FKalmalaM9ExplorationRewardCatalogue::StableIdentityVersion, *Definition.CandidateId.ToString());
        if (!Id.StartsWith(Prefix, ESearchCase::CaseSensitive)) continue;

        FString XText;
        FString YText;
        int32 KeyX = 0;
        int32 KeyY = 0;
        const FString CoordinateText = Id.RightChop(Prefix.Len());
        if (!CoordinateText.Split(TEXT(","), &XText, &YText) || YText.Contains(TEXT(","))
            || !ParseCanonicalInteger(XText, KeyX) || !ParseCanonicalInteger(YText, KeyY)) return false;

        return FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
            Definition.CandidateId, FIntPoint(KeyX, KeyY)) == Id;
    }
    return false;
}

bool UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(const FKalmalaPlayerToolSaveRecord& Record)
{
    return Record.InstanceGuid.IsValid() && IsValidToolRecordData(Record);
}

bool UKalmalaPlayerDiscoverySaveGameV2::MatchesPlayer(
    const FKalmalaWorldGenerationConfig& InWorld,
    const FString& InPlayerIdentity) const
{
    if (SchemaVersion != SchemaVersionValue || !InWorld.IsValid() || WorldConfig != InWorld
        || InPlayerIdentity.IsEmpty() || InPlayerIdentity.Len() > FKalmalaM7SaveIdentity::MaxOwnerIdentityLength
        || PlayerIdentity != InPlayerIdentity
        || !Identity.Matches(FKalmalaM7SaveIdentity::ForPlayer(InWorld.WorldSeed, InPlayerIdentity))
        || DiscoveryIds.Num() > MaxLegacyDiscoveries || M9ClaimIds.Num() > MaxM9Claims
        || DiscoveryIds.Num() + M9ClaimIds.Num() > MaxCombinedDiscoveries
        || LearnedEffectIds.Num() > MaxLearnedEffects || ToolRecords.Num() > MaxInventorySlots)
    {
        return false;
    }

    for (const FString& DiscoveryId : DiscoveryIds)
    {
        if (!IsValidLegacyDiscoveryId(DiscoveryId)) return false;
    }

    TSet<FString> SeenClaimIds;
    for (const FString& ClaimId : M9ClaimIds)
    {
        if (!IsValidM9ClaimId(ClaimId) || SeenClaimIds.Contains(ClaimId) || DiscoveryIds.Contains(ClaimId)) return false;
        SeenClaimIds.Add(ClaimId);
    }

    for (const FString& EffectId : LearnedEffectIds)
    {
        if (!IsKnownLearnedEffectId(EffectId)) return false;
    }

    TSet<FGuid> SeenToolGuids;
    for (const FKalmalaPlayerToolSaveRecord& ToolRecord : ToolRecords)
    {
        if (!IsValidToolRecord(ToolRecord) || SeenToolGuids.Contains(ToolRecord.InstanceGuid)) return false;
        SeenToolGuids.Add(ToolRecord.InstanceGuid);
    }
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::HasDiscovery(const FString& Id) const
{
    return DiscoveryIds.Contains(Id) || M9ClaimIds.Contains(Id);
}

bool UKalmalaPlayerDiscoverySaveGameV2::HasLearnedEffect(const FString& Id) const
{
    return LearnedEffectIds.Contains(Id);
}

bool UKalmalaPlayerDiscoverySaveGameV2::AddDiscovery(const FString& Id)
{
    if (!MatchesPlayer(WorldConfig, PlayerIdentity) || !IsValidLegacyDiscoveryId(Id)
        || DiscoveryIds.Contains(Id) || DiscoveryIds.Num() >= MaxLegacyDiscoveries
        || DiscoveryIds.Num() + M9ClaimIds.Num() >= MaxCombinedDiscoveries) return false;
    DiscoveryIds.Add(Id);
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::AddM9Claim(const FString& Id)
{
    if (!MatchesPlayer(WorldConfig, PlayerIdentity) || !IsValidM9ClaimId(Id)
        || M9ClaimIds.Contains(Id) || DiscoveryIds.Contains(Id) || M9ClaimIds.Num() >= MaxM9Claims
        || DiscoveryIds.Num() + M9ClaimIds.Num() >= MaxCombinedDiscoveries) return false;
    M9ClaimIds.Add(Id);
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::AddLearnedEffect(const FString& Id)
{
    if (!MatchesPlayer(WorldConfig, PlayerIdentity) || !IsKnownLearnedEffectId(Id)
        || LearnedEffectIds.Contains(Id) || LearnedEffectIds.Num() >= MaxLearnedEffects) return false;
    LearnedEffectIds.Add(Id);
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::AddToolRecord(const FKalmalaPlayerToolSaveRecord& Record)
{
    FKalmalaPlayerToolSaveRecord CanonicalRecord = Record;
    CanonicalRecord.ItemId = FKalmalaToolLifecycleContract::CanonicalizeItemId(Record.ItemId);
    if (!CanonicalRecord.InstanceGuid.IsValid()) CanonicalRecord.InstanceGuid = FGuid::NewGuid();
    if (!MatchesPlayer(WorldConfig, PlayerIdentity) || !IsValidToolRecord(CanonicalRecord)
        || ToolRecords.Num() >= MaxInventorySlots
        || ToolRecords.ContainsByPredicate([&CanonicalRecord](const FKalmalaPlayerToolSaveRecord& Existing)
            { return Existing.InstanceGuid == CanonicalRecord.InstanceGuid; })) return false;
    ToolRecords.Add(CanonicalRecord);
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::TryReplaceToolRecords(
    const TArray<FKalmalaPlayerToolSaveRecord>& Records)
{
    if (!MatchesPlayer(WorldConfig, PlayerIdentity) || Records.Num() > MaxInventorySlots) return false;

    TArray<FKalmalaPlayerToolSaveRecord> Candidate = Records;
    TSet<FGuid> SeenGuids;
    for (FKalmalaPlayerToolSaveRecord& Record : Candidate)
    {
        Record.ItemId = FKalmalaToolLifecycleContract::CanonicalizeItemId(Record.ItemId);
        if (!Record.InstanceGuid.IsValid()) Record.InstanceGuid = FGuid::NewGuid();
        if (!IsValidToolRecord(Record) || SeenGuids.Contains(Record.InstanceGuid)) return false;
        SeenGuids.Add(Record.InstanceGuid);
    }

    ToolRecords = MoveTemp(Candidate);
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::EnsureToolInstanceGuids(bool& bOutChanged)
{
    bOutChanged = false;
    if (ToolRecords.Num() > MaxInventorySlots) return false;
    TSet<FGuid> SeenGuids;
    for (FKalmalaPlayerToolSaveRecord& Record : ToolRecords)
    {
        if (!IsValidToolRecordData(Record)) return false;
        if (!Record.InstanceGuid.IsValid())
        {
            do { Record.InstanceGuid = FGuid::NewGuid(); } while (!Record.InstanceGuid.IsValid() || SeenGuids.Contains(Record.InstanceGuid));
            bOutChanged = true;
        }
        if (SeenGuids.Contains(Record.InstanceGuid)) return false;
        SeenGuids.Add(Record.InstanceGuid);
    }
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::TryMigrateSchema2(
    const UKalmalaPlayerDiscoverySaveGameV2* Legacy,
    const FKalmalaWorldGenerationConfig& RequestedWorld,
    const FString& AuthenticatedPlayerIdentity,
    UObject* Outer,
    UKalmalaPlayerDiscoverySaveGameV2*& OutMigrated)
{
    OutMigrated = nullptr;
    const FKalmalaM7SaveIdentity ExpectedIdentity = FKalmalaM7SaveIdentity::ForPlayer(
        RequestedWorld.WorldSeed, AuthenticatedPlayerIdentity);
    if (Legacy == nullptr || Legacy->SchemaVersion != PreviousSchemaVersionValue
        || !RequestedWorld.IsValid() || !ExpectedIdentity.IsValid()
        || Legacy->WorldConfig != RequestedWorld || Legacy->PlayerIdentity != AuthenticatedPlayerIdentity
        || !Legacy->Identity.Matches(ExpectedIdentity)) return false;

    UKalmalaPlayerDiscoverySaveGameV2* Candidate = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Outer);
    if (Candidate == nullptr) return false;
    Candidate->InitializeForPlayer(RequestedWorld, AuthenticatedPlayerIdentity);
    Candidate->DiscoveryIds = Legacy->DiscoveryIds;
    Candidate->M9ClaimIds = Legacy->M9ClaimIds;
    Candidate->LearnedEffectIds = Legacy->LearnedEffectIds;
    Candidate->ToolRecords = Legacy->ToolRecords;
    for (FKalmalaPlayerToolSaveRecord& Record : Candidate->ToolRecords)
    {
        if (!Record.ItemId.IsNone() || Record.ToolId.IsNone()) return false;
        Record.ItemId = FKalmalaToolLifecycleContract::CanonicalizeItemId(Record.ToolId);
        Record.ToolId = NAME_None;
    }

    bool bMigratedInstanceGuids = false;
    if (!Candidate->EnsureToolInstanceGuids(bMigratedInstanceGuids)) return false;
    if (!Candidate->MatchesPlayer(RequestedWorld, AuthenticatedPlayerIdentity)) return false;
    OutMigrated = Candidate;
    return true;
}

bool UKalmalaPlayerDiscoverySaveGameV2::TryMigrateSchema1(
    const UKalmalaPlayerDiscoverySaveGame* Legacy,
    const FKalmalaWorldGenerationConfig& RequestedWorld,
    const FString& AuthenticatedPlayerIdentity,
    UObject* Outer,
    UKalmalaPlayerDiscoverySaveGameV2*& OutMigrated)
{
    OutMigrated = nullptr;
    const FKalmalaM7SaveIdentity ExpectedIdentity = FKalmalaM7SaveIdentity::ForPlayer(
        RequestedWorld.WorldSeed, AuthenticatedPlayerIdentity);
    if (Legacy == nullptr || Legacy->SchemaVersion != UKalmalaPlayerDiscoverySaveGame::Schema
        || !RequestedWorld.IsValid() || !ExpectedIdentity.IsValid()
        || !Legacy->Matches(RequestedWorld, AuthenticatedPlayerIdentity)
        || Legacy->DiscoveryIds.Num() > MaxLegacyDiscoveries
        || Legacy->LearnedEffectIds.Num() > MaxLearnedEffects) return false;

    for (const FString& DiscoveryId : Legacy->DiscoveryIds)
    {
        if (!IsValidLegacyDiscoveryId(DiscoveryId)) return false;
    }
    for (const FString& EffectId : Legacy->LearnedEffectIds)
    {
        if (!IsKnownLearnedEffectId(EffectId)) return false;
    }

    UKalmalaPlayerDiscoverySaveGameV2* Candidate = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Outer);
    if (Candidate == nullptr) return false;
    Candidate->InitializeForPlayer(RequestedWorld, AuthenticatedPlayerIdentity);
    Candidate->DiscoveryIds = Legacy->DiscoveryIds;
    Candidate->LearnedEffectIds = Legacy->LearnedEffectIds;
    if (!Candidate->MatchesPlayer(RequestedWorld, AuthenticatedPlayerIdentity)) return false;
    OutMigrated = Candidate;
    return true;
}
