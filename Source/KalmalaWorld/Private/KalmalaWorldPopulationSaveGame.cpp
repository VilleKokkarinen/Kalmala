#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaGemstoneContract.h"

namespace
{
    constexpr TCHAR BadgerRewardClaimPrefix[] = TEXT("badger-reward|");
    constexpr TCHAR MoonBearRewardClaimPrefix[] = TEXT("moonbear-reward|");
    constexpr TCHAR ViperRewardClaimPrefix[] = TEXT("viper-reward|");
    constexpr TCHAR GiantSnakeRewardClaimPrefix[] = TEXT("giantsnake-reward|");
    constexpr TCHAR DeathAdderRewardClaimPrefix[] = TEXT("deathadder-reward|");
    constexpr TCHAR PeatHuskRewardClaimPrefix[] = TEXT("peathusk-reward|");
    constexpr TCHAR PeatHeartRewardSuffix[] = TEXT("|heart");
    constexpr TCHAR NoPeatHeartRewardSuffix[] = TEXT("|noheart");
    constexpr TCHAR WolfRewardClaimPrefix[] = TEXT("wolf-reward|");
    constexpr TCHAR WolfFangRewardSuffix[] = TEXT("|fang");
    constexpr TCHAR NoWolfFangRewardSuffix[] = TEXT("|nofang");
    constexpr TCHAR LynxRewardClaimPrefix[] = TEXT("lynx-reward|");
    constexpr TCHAR LynxPeltRewardSuffix[] = TEXT("|pelt");
    constexpr TCHAR NoLynxPeltRewardSuffix[] = TEXT("|nopelt");
    constexpr TCHAR PolarBearRewardClaimPrefix[] = TEXT("polarbear-reward|");
    constexpr TCHAR HornedGoatRewardClaimPrefix[] = TEXT("hornedgoat-reward|");
    constexpr TCHAR ThunderSentinelRewardClaimPrefix[] = TEXT("thundersentinel-reward|");
    constexpr TCHAR ThunderCoreRewardSuffix[] = TEXT("|core");
    constexpr TCHAR NoThunderCoreRewardSuffix[] = TEXT("|nocore");
    constexpr TCHAR ReefMawRewardClaimPrefix[] = TEXT("reefmaw-reward|");
    constexpr TCHAR ReefToothRewardSuffix[] = TEXT("|tooth");
    constexpr TCHAR NoReefToothRewardSuffix[] = TEXT("|notooth");
}

void UKalmalaWorldPopulationSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    SchemaVersion = 2;
    Gemstones.Reset();
    Treasures.Reset();
    WorldConfig = InWorldConfig;
    HarvestedSpawnIds.Reset();
    DefeatedSpawnIds.Reset();
    PendingWildlifeRewards.Reset();
}

bool UKalmalaWorldPopulationSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const
{
    if (SchemaVersion != 2 || WorldConfig != InWorldConfig || Gemstones.Num() > 65536) return false;
    for (const auto& Reward : PendingWildlifeRewards)
        if (!Reward.GemItemId.IsNone() && !(Reward.GemItemId == TEXT("Moonstone")
            && Reward.PersistentSpawnId.StartsWith(TEXT("moonbear-reward|")))) return false;
    if (Treasures.Num() > 65536) return false;
    TSet<FString> TreasureIds;
    for (const auto& Treasure : Treasures)
    {
        if (Treasure.Id.IsEmpty() || Treasure.Id.Len() > 128 || TreasureIds.Contains(Treasure.Id) || Treasure.Items.Num() > 2) return false;
        TSet<FName> SeenItems;
        for (const FName Item : Treasure.Items)
        {
            if ((Item != TEXT("Garnet") && Item != TEXT("Aquamarine") && Item != TEXT("StormSapphire")) || SeenItems.Contains(Item)) return false;
            SeenItems.Add(Item);
        }
        TreasureIds.Add(Treasure.Id);
    }
    TSet<FString> Seen;
    for (const FKalmalaGemstoneState& State : Gemstones)
    {
        if (State.SpawnId.IsEmpty() || State.SpawnId.Len() > 128 || Seen.Contains(State.SpawnId)
            || !FKalmalaGemstoneContract::IsGemSource(State.SourceId) || State.Hits < 1 || State.Hits > 3
            || State.Outcome > 3 || (State.Outcome == 0 && State.Hits >= 3)
            || (State.Outcome >= 2 && State.Hits != 3)) return false;
        Seen.Add(State.SpawnId);
    }
    return true;
}

bool UKalmalaWorldPopulationSaveGame::IsHarvested(const FString& PersistentSpawnId) const
{
    return !PersistentSpawnId.IsEmpty() && HarvestedSpawnIds.Contains(PersistentSpawnId);
}

void UKalmalaWorldPopulationSaveGame::MarkHarvested(const FString& PersistentSpawnId)
{
    if (!PersistentSpawnId.IsEmpty())
    {
        HarvestedSpawnIds.Add(PersistentSpawnId);
    }
}

bool UKalmalaWorldPopulationSaveGame::IsDefeated(const FString& PersistentSpawnId) const
{
    return !PersistentSpawnId.IsEmpty() && DefeatedSpawnIds.Contains(PersistentSpawnId);
}

void UKalmalaWorldPopulationSaveGame::MarkDefeated(const FString& PersistentSpawnId)
{
    if (!PersistentSpawnId.IsEmpty())
    {
        DefeatedSpawnIds.Add(PersistentSpawnId);
    }
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingWildlifeReward(
    const FString& RewardClaimId, const FString& PlayerIdentity)
{
    if (RewardClaimId.IsEmpty() || RewardClaimId.Len() > 128
        || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    if (const FKalmalaPendingWildlifeReward* Existing = PendingWildlifeRewards.FindByPredicate(
        [&RewardClaimId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == RewardClaimId; }))
    {
        return Existing->PlayerIdentity == PlayerIdentity;
    }
    if (PendingWildlifeRewards.Num() >= MaxPendingWildlifeRewards) return false;

    FKalmalaPendingWildlifeReward& Reward = PendingWildlifeRewards.AddDefaulted_GetRef();
    Reward.PersistentSpawnId = RewardClaimId;
    Reward.PlayerIdentity = PlayerIdentity;
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingBadgerReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeBadgerPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeBadgerPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(BadgerRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(BadgerRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetBadgerSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(BadgerRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(BadgerRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingMoonBearReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeMoonBearPendingRewardClaimId(PersistentSpawnId);
    if (ClaimId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        if (TryGetMoonBearSpawnIdFromPendingRewardClaimId(Existing.PersistentSpawnId, ExistingSpawnId)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }
    return TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(MoonBearRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(MoonBearRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetMoonBearSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(MoonBearRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(MoonBearRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingViperReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeViperPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ViperRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(ViperRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetViperSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(ViperRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ViperRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingGiantSnakeReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeGiantSnakePendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeGiantSnakePendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(GiantSnakeRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(GiantSnakeRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetGiantSnakeSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(GiantSnakeRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(GiantSnakeRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingDeathAdderReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeDeathAdderPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeDeathAdderPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(DeathAdderRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(DeathAdderRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetDeathAdderSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(DeathAdderRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(DeathAdderRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingPeatHuskReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity, const bool bHasPeatHeart)
{
    if (PersistentSpawnId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        bool bExistingHasPeatHeart = false;
        if (TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
                Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasPeatHeart)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }

    const FString ClaimId = MakePeatHuskPendingRewardClaimId(PersistentSpawnId, bHasPeatHeart);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakePeatHuskPendingRewardClaimId(
    const FString& PersistentSpawnId, const bool bHasPeatHeart)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(PeatHuskRewardClaimPrefix) - 1;
    const TCHAR* Suffix = bHasPeatHeart ? PeatHeartRewardSuffix : NoPeatHeartRewardSuffix;
    const int32 SuffixLength = FCString::Strlen(Suffix);
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)
        || PersistentSpawnId.Len() > 128 - PrefixLength - SuffixLength) return FString();
    return FString(PeatHuskRewardClaimPrefix) + PersistentSpawnId + Suffix;
}

bool UKalmalaWorldPopulationSaveGame::TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasPeatHeart)
{
    OutPersistentSpawnId.Reset();
    OutHasPeatHeart = false;
    const int32 PrefixLength = UE_ARRAY_COUNT(PeatHuskRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(PeatHuskRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    FString EncodedSpawnId = ClaimId.RightChop(PrefixLength);
    const int32 PeatHeartSuffixLength = FCString::Strlen(PeatHeartRewardSuffix);
    const int32 NoPeatHeartSuffixLength = FCString::Strlen(NoPeatHeartRewardSuffix);
    if (EncodedSpawnId.EndsWith(PeatHeartRewardSuffix, ESearchCase::CaseSensitive))
    {
        OutHasPeatHeart = true;
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - PeatHeartSuffixLength);
    }
    else if (EncodedSpawnId.EndsWith(NoPeatHeartRewardSuffix, ESearchCase::CaseSensitive))
    {
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - NoPeatHeartSuffixLength);
    }
    // Unsuffixed claims predate this drop and retain their original baseline-only reward.
    if (EncodedSpawnId.IsEmpty() || EncodedSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)) return false;

    OutPersistentSpawnId = MoveTemp(EncodedSpawnId);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingWolfReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity, const bool bHasWolfFang)
{
    if (PersistentSpawnId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        bool bExistingHasWolfFang = false;
        if (TryGetWolfSpawnIdFromPendingRewardClaimId(
                Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasWolfFang)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }

    const FString ClaimId = MakeWolfPendingRewardClaimId(PersistentSpawnId, bHasWolfFang);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeWolfPendingRewardClaimId(
    const FString& PersistentSpawnId, const bool bHasWolfFang)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(WolfRewardClaimPrefix) - 1;
    const TCHAR* Suffix = bHasWolfFang ? WolfFangRewardSuffix : NoWolfFangRewardSuffix;
    const int32 SuffixLength = FCString::Strlen(Suffix);
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)
        || PersistentSpawnId.Len() > 128 - PrefixLength - SuffixLength) return FString();
    return FString(WolfRewardClaimPrefix) + PersistentSpawnId + Suffix;
}

bool UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasWolfFang)
{
    OutPersistentSpawnId.Reset();
    OutHasWolfFang = false;
    const int32 PrefixLength = UE_ARRAY_COUNT(WolfRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(WolfRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    FString EncodedSpawnId = ClaimId.RightChop(PrefixLength);
    const int32 WolfFangSuffixLength = FCString::Strlen(WolfFangRewardSuffix);
    const int32 NoWolfFangSuffixLength = FCString::Strlen(NoWolfFangRewardSuffix);
    if (EncodedSpawnId.EndsWith(WolfFangRewardSuffix, ESearchCase::CaseSensitive))
    {
        OutHasWolfFang = true;
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - WolfFangSuffixLength);
    }
    else if (EncodedSpawnId.EndsWith(NoWolfFangRewardSuffix, ESearchCase::CaseSensitive))
    {
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - NoWolfFangSuffixLength);
    }
    // Unsuffixed claims predate the optional drop and remain baseline-only.
    if (EncodedSpawnId.IsEmpty() || EncodedSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)) return false;

    OutPersistentSpawnId = MoveTemp(EncodedSpawnId);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingLynxReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity, const bool bHasLynxPelt)
{
    if (PersistentSpawnId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        bool bExistingHasLynxPelt = false;
        if (TryGetLynxSpawnIdFromPendingRewardClaimId(
                Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasLynxPelt)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }

    const FString ClaimId = MakeLynxPendingRewardClaimId(PersistentSpawnId, bHasLynxPelt);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(
    const FString& PersistentSpawnId, const bool bHasLynxPelt)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(LynxRewardClaimPrefix) - 1;
    const TCHAR* Suffix = bHasLynxPelt ? LynxPeltRewardSuffix : NoLynxPeltRewardSuffix;
    const int32 SuffixLength = FCString::Strlen(Suffix);
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)
        || PersistentSpawnId.Len() > 128 - PrefixLength - SuffixLength) return FString();
    return FString(LynxRewardClaimPrefix) + PersistentSpawnId + Suffix;
}

bool UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasLynxPelt)
{
    OutPersistentSpawnId.Reset();
    OutHasLynxPelt = false;
    const int32 PrefixLength = UE_ARRAY_COUNT(LynxRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(LynxRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    FString EncodedSpawnId = ClaimId.RightChop(PrefixLength);
    const int32 LynxPeltSuffixLength = FCString::Strlen(LynxPeltRewardSuffix);
    const int32 NoLynxPeltSuffixLength = FCString::Strlen(NoLynxPeltRewardSuffix);
    if (EncodedSpawnId.EndsWith(LynxPeltRewardSuffix, ESearchCase::CaseSensitive))
    {
        OutHasLynxPelt = true;
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - LynxPeltSuffixLength);
    }
    else if (EncodedSpawnId.EndsWith(NoLynxPeltRewardSuffix, ESearchCase::CaseSensitive))
    {
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - NoLynxPeltSuffixLength);
    }
    // Unsuffixed claims predate the optional pelt and remain baseline-only.
    if (EncodedSpawnId.IsEmpty() || EncodedSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)) return false;

    OutPersistentSpawnId = MoveTemp(EncodedSpawnId);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingPolarBearReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakePolarBearPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakePolarBearPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(PolarBearRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(PolarBearRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(PolarBearRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(PolarBearRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingHornedGoatReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeHornedGoatPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeHornedGoatPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(HornedGoatRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(HornedGoatRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(HornedGoatRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(HornedGoatRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingThunderSentinelReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity, const bool bHasThunderCore)
{
    if (PersistentSpawnId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        bool bExistingHasThunderCore = false;
        if (TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
                Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasThunderCore)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }

    const FString ClaimId = MakeThunderSentinelPendingRewardClaimId(PersistentSpawnId, bHasThunderCore);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(
    const FString& PersistentSpawnId, const bool bHasThunderCore)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ThunderSentinelRewardClaimPrefix) - 1;
    const TCHAR* Suffix = bHasThunderCore ? ThunderCoreRewardSuffix : NoThunderCoreRewardSuffix;
    const int32 SuffixLength = FCString::Strlen(Suffix);
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)
        || PersistentSpawnId.Len() > 128 - PrefixLength - SuffixLength) return FString();
    return FString(ThunderSentinelRewardClaimPrefix) + PersistentSpawnId + Suffix;
}

bool UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasThunderCore)
{
    OutPersistentSpawnId.Reset();
    OutHasThunderCore = false;
    const int32 PrefixLength = UE_ARRAY_COUNT(ThunderSentinelRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ThunderSentinelRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    FString EncodedSpawnId = ClaimId.RightChop(PrefixLength);
    const int32 ThunderCoreSuffixLength = FCString::Strlen(ThunderCoreRewardSuffix);
    const int32 NoThunderCoreSuffixLength = FCString::Strlen(NoThunderCoreRewardSuffix);
    if (EncodedSpawnId.EndsWith(ThunderCoreRewardSuffix, ESearchCase::CaseSensitive))
    {
        OutHasThunderCore = true;
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - ThunderCoreSuffixLength);
    }
    else if (EncodedSpawnId.EndsWith(NoThunderCoreRewardSuffix, ESearchCase::CaseSensitive))
    {
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - NoThunderCoreSuffixLength);
    }
    // Older unsuffixed claims remain baseline-only.
    if (EncodedSpawnId.IsEmpty() || EncodedSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)) return false;

    OutPersistentSpawnId = MoveTemp(EncodedSpawnId);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingReefMawReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity, const bool bHasReefTooth)
{
    if (PersistentSpawnId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        bool bExistingHasReefTooth = false;
        if (TryGetReefMawSpawnIdFromPendingRewardClaimId(
                Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasReefTooth)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }

    const FString ClaimId = MakeReefMawPendingRewardClaimId(PersistentSpawnId, bHasReefTooth);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(
    const FString& PersistentSpawnId, const bool bHasReefTooth)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ReefMawRewardClaimPrefix) - 1;
    const TCHAR* Suffix = bHasReefTooth ? ReefToothRewardSuffix : NoReefToothRewardSuffix;
    const int32 SuffixLength = FCString::Strlen(Suffix);
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)
        || PersistentSpawnId.Len() > 128 - PrefixLength - SuffixLength) return FString();
    return FString(ReefMawRewardClaimPrefix) + PersistentSpawnId + Suffix;
}

bool UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasReefTooth)
{
    OutPersistentSpawnId.Reset();
    OutHasReefTooth = false;
    const int32 PrefixLength = UE_ARRAY_COUNT(ReefMawRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ReefMawRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    FString EncodedSpawnId = ClaimId.RightChop(PrefixLength);
    const int32 ReefToothSuffixLength = FCString::Strlen(ReefToothRewardSuffix);
    const int32 NoReefToothSuffixLength = FCString::Strlen(NoReefToothRewardSuffix);
    if (EncodedSpawnId.EndsWith(ReefToothRewardSuffix, ESearchCase::CaseSensitive))
    {
        OutHasReefTooth = true;
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - ReefToothSuffixLength);
    }
    else if (EncodedSpawnId.EndsWith(NoReefToothRewardSuffix, ESearchCase::CaseSensitive))
    {
        EncodedSpawnId = EncodedSpawnId.Left(EncodedSpawnId.Len() - NoReefToothSuffixLength);
    }
    // Older unsuffixed claims remain baseline-only.
    if (EncodedSpawnId.IsEmpty() || EncodedSpawnId.Contains(TEXT("|"), ESearchCase::CaseSensitive)) return false;

    OutPersistentSpawnId = MoveTemp(EncodedSpawnId);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::RemovePendingWildlifeReward(const FString& RewardClaimId)
{
    if (RewardClaimId.IsEmpty()) return false;
    return PendingWildlifeRewards.RemoveAll([&RewardClaimId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == RewardClaimId; }) > 0;
}

const FKalmalaGemstoneState* UKalmalaWorldPopulationSaveGame::FindGemstone(const FString& SpawnId) const
{
    return Gemstones.FindByPredicate([&SpawnId](const auto& State) { return State.SpawnId == SpawnId; });
}

bool UKalmalaWorldPopulationSaveGame::SetGemstone(const FKalmalaGemstoneState& State)
{
    if (State.SpawnId.IsEmpty() || State.SpawnId.Len() > 128 || !FKalmalaGemstoneContract::IsGemSource(State.SourceId)
        || State.Hits < 1 || State.Hits > 3 || State.Outcome > 3 || (State.Outcome == 0 && State.Hits >= 3)
        || (State.Outcome >= 2 && State.Hits != 3)) return false;
    auto* Existing = Gemstones.FindByPredicate([&State](const auto& Entry) { return Entry.SpawnId == State.SpawnId; });
    if (Existing) { *Existing = State; return true; }
    if (Gemstones.Num() >= 65536) return false;
    Gemstones.Add(State);
    return true;
}

bool UKalmalaWorldPopulationSaveGame::RemoveGemstone(const FString& SpawnId)
{
    return Gemstones.RemoveAll([&SpawnId](const auto& State) { return State.SpawnId == SpawnId; }) == 1;
}

bool UKalmalaWorldPopulationSaveGame::SetPendingGemReward(const FString& ClaimId, const FName GemItemId)
{
    if (!GemItemId.IsNone() && !(GemItemId == TEXT("Moonstone") && ClaimId.StartsWith(TEXT("moonbear-reward|")))) return false;
    auto* Reward = PendingWildlifeRewards.FindByPredicate([&ClaimId](const auto& Entry) { return Entry.PersistentSpawnId == ClaimId; });
    if (!Reward) return false;
    Reward->GemItemId = GemItemId;
    return true;
}

const FKalmalaTreasureState* UKalmalaWorldPopulationSaveGame::FindTreasure(const FString& Id) const
{
    return Treasures.FindByPredicate([&Id](const auto& State) { return State.Id == Id; });
}

bool UKalmalaWorldPopulationSaveGame::SetTreasure(const FKalmalaTreasureState& State)
{
    if (State.Id.IsEmpty() || State.Id.Len() > 128 || State.Items.Num() > 2) return false;
    TSet<FName> Seen;
    for (const FName Item : State.Items)
    {
        if ((Item != TEXT("Garnet") && Item != TEXT("Aquamarine") && Item != TEXT("StormSapphire")) || Seen.Contains(Item)) return false;
        Seen.Add(Item);
    }
    auto* Existing = Treasures.FindByPredicate([&State](const auto& Entry) { return Entry.Id == State.Id; });
    if (Existing)
    {
        if (Existing->Items != State.Items || Existing->bClaimed || !State.bClaimed) return false;
        *Existing = State;
        return true;
    }
    if (Treasures.Num() >= 65536 || State.bClaimed) return false;
    Treasures.Add(State);
    return true;
}
