#include "KalmalaM9SourceLootContract.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"

bool FKalmalaM9SourceLootContract::IsM9Source(const FName SourceId)
{
    return SourceId == TEXT("meadows-birch-trunk")
        || SourceId == TEXT("elderwood-ironheart-trunk")
        || SourceId == TEXT("mire-peat-amber-seam")
        || SourceId == TEXT("tundra-frost-salt-deposit");
}

bool FKalmalaM9SourceLootContract::GetPrimaryItemId(const FName SourceId, FName& OutItemId)
{
    OutItemId = NAME_None;
    if (SourceId == TEXT("meadows-birch-trunk")) OutItemId = TEXT("Lightwood");
    else if (SourceId == TEXT("elderwood-ironheart-trunk")) OutItemId = TEXT("Densewood");
    else if (SourceId == TEXT("mire-peat-amber-seam")) OutItemId = TEXT("PeatAmber");
    else if (SourceId == TEXT("tundra-frost-salt-deposit")) OutItemId = TEXT("FrostSalt");
    return !OutItemId.IsNone();
}

uint64 FKalmalaM9SourceLootContract::HashAscii(const FString& Value)
{
    FTCHARToUTF8 Utf8(*Value);
    const auto* Bytes = reinterpret_cast<const uint8*>(Utf8.Get());
    uint64 Hash = 14695981039346656037ull;
    for (int32 Index = 0; Index < Utf8.Length(); ++Index)
    {
        Hash ^= Bytes[Index];
        Hash *= 1099511628211ull;
    }
    return Hash;
}

bool FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
    const FKalmalaWorldGenerationConfig& WorldConfig,
    const FName SourceId,
    const FString& PopulationSpawnId,
    int32& OutQuantity)
{
    OutQuantity = 0;
    FName ItemId;
    if (!WorldConfig.IsValid() || !GetPrimaryItemId(SourceId, ItemId) || PopulationSpawnId.IsEmpty()) return false;

    const FString LootKey = FString::Printf(TEXT("m9-loot-v1|%llu|%d|%s|%s"),
        static_cast<unsigned long long>(WorldConfig.WorldSeed),
        FKalmalaM7SaveIdentity::CurrentGeneratorRevision,
        *SourceId.ToString(), *PopulationSpawnId);
    const int32 CandidateQuantity = PrimaryYield + (HashAscii(LootKey) % BonusDivisor == 0 ? 1 : 0);
    const UKalmalaItemCatalogue* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    if (Catalogue == nullptr || !Catalogue->IsValidStack(ItemId, CandidateQuantity)) return false;

    OutQuantity = CandidateQuantity;
    return true;
}

FString FKalmalaM9SourceLootContract::BuildResourceDepletionId(
    const FName SourceId,
    const FString& PopulationSpawnId)
{
    if (!IsM9Source(SourceId) || PopulationSpawnId.IsEmpty()) return FString();
    const FString StableId = FString::Printf(TEXT("resource:m9:v1:%s:%s"), *SourceId.ToString(), *PopulationSpawnId);
    return FKalmalaM7SparseDelta::IsValidStableId(StableId) ? StableId : FString();
}
