#include "KalmalaWorldPopulationSaveGame.h"

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
    constexpr TCHAR ReefMawRewardClaimPrefix[] = TEXT("reefmaw-reward|");
}

void UKalmalaWorldPopulationSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    WorldConfig = InWorldConfig;
    HarvestedSpawnIds.Reset();
    DefeatedSpawnIds.Reset();
    PendingWildlifeRewards.Reset();
}

bool UKalmalaWorldPopulationSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const
{
    return SchemaVersion == 1 && WorldConfig == InWorldConfig;
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
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeThunderSentinelPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(
    const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ThunderSentinelRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(ThunderSentinelRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(ThunderSentinelRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ThunderSentinelRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingReefMawReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeReefMawPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(
    const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ReefMawRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(ReefMawRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(ReefMawRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ReefMawRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::RemovePendingWildlifeReward(const FString& RewardClaimId)
{
    if (RewardClaimId.IsEmpty()) return false;
    return PendingWildlifeRewards.RemoveAll([&RewardClaimId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == RewardClaimId; }) > 0;
}
