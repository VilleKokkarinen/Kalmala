#pragma once

#include "CoreMinimal.h"
#include <limits>

/** Server-supplied sample for one creature already owned by an environmental source. */
struct FKalmalaEnvironmentalSpawnerCreatureSample
{
    FString SourceId;
    FVector Location = FVector::ZeroVector;
    bool bIsLiving = false;
};

/** Reusable finite local-source scheduling and ownership policy. */
struct FKalmalaEnvironmentalSpawnerPolicy final
{
    static constexpr int32 SourceSlotCount = 2;
    static constexpr double CheckIntervalSeconds = 8.0;
    static constexpr int32 MaximumLivingCreaturesPerSource = 2;
    static constexpr double LocalLivingRadiusCentimetres = 2400.0;
    static constexpr int32 MaximumCandidatesPerCheck = 4;
    static constexpr double CandidateRadiusCentimetres = 1200.0;
    static constexpr int32 MaximumCreationsPerCheck = 1;

    static_assert(SourceSlotCount == 2, "Environmental sources have two finite slots.");
    static_assert(CheckIntervalSeconds == 8.0,
        "Environmental source checks use the fixed eight-second interval.");
    static_assert(LocalLivingRadiusCentimetres == 24.0 * 100.0,
        "The local source cap uses an inclusive 24 m radius.");
    static_assert(MaximumCandidatesPerCheck == 4 && CandidateRadiusCentimetres == 12.0 * 100.0,
        "Each source check examines at most four fixed candidates within the 12 m 3D radius.");
    static_assert(MaximumCreationsPerCheck == 1,
        "One source check can create at most one creature.");

    static bool IsSourceSlotIndexValid(const int32 SlotIndex)
    {
        return SlotIndex >= 0 && SlotIndex < SourceSlotCount;
    }

    static bool IsSourceConfigured(const bool bHasServerAuthority, const bool bSourceConfigured,
        const FString& SourceId, const int32 SourceSlotIndex)
    {
        return bHasServerAuthority && bSourceConfigured
            && IsSourceSlotIndexValid(SourceSlotIndex)
            && !SourceId.TrimStartAndEnd().IsEmpty();
    }

    static bool IsCheckDue(const bool bHasServerAuthority, const bool bSourceConfigured,
        const FString& SourceId, const int32 SourceSlotIndex, const double NowSeconds, const double NextDueSeconds)
    {
        return IsSourceConfigured(bHasServerAuthority, bSourceConfigured, SourceId, SourceSlotIndex)
            && FMath::IsFinite(NowSeconds) && FMath::IsFinite(NextDueSeconds)
            && NowSeconds >= NextDueSeconds;
    }

    /** Set this from the actual service time so missed checks never accumulate. */
    static bool TryGetNextDueTimeAfterService(const double ServicedAtSeconds, double& OutNextDueSeconds)
    {
        OutNextDueSeconds = std::numeric_limits<double>::quiet_NaN();
        if (!FMath::IsFinite(ServicedAtSeconds)) return false;

        const double NextDueSeconds = ServicedAtSeconds + CheckIntervalSeconds;
        if (!FMath::IsFinite(NextDueSeconds)) return false;

        OutNextDueSeconds = NextDueSeconds;
        return true;
    }

    static int32 GetCandidateBudgetForCheck(const int32 RemainingGlobalCandidateBudget)
    {
        return FMath::Clamp(RemainingGlobalCandidateBudget, 0, MaximumCandidatesPerCheck);
    }

    /** Uses a 3D radius; all source and candidate coordinates must be finite. */
    static bool IsCandidateWithinSourceRadius(const FVector& SourceOrigin, const FVector& CandidateLocation)
    {
        if (!FMath::IsFinite(SourceOrigin.X) || !FMath::IsFinite(SourceOrigin.Y)
            || !FMath::IsFinite(SourceOrigin.Z) || !FMath::IsFinite(CandidateLocation.X)
            || !FMath::IsFinite(CandidateLocation.Y) || !FMath::IsFinite(CandidateLocation.Z))
        {
            return false;
        }

        const double DeltaX = static_cast<double>(CandidateLocation.X) - SourceOrigin.X;
        const double DeltaY = static_cast<double>(CandidateLocation.Y) - SourceOrigin.Y;
        const double DeltaZ = static_cast<double>(CandidateLocation.Z) - SourceOrigin.Z;
        return DeltaX * DeltaX + DeltaY * DeltaY + DeltaZ * DeltaZ
            <= FMath::Square(CandidateRadiusCentimetres);
    }

    static bool CanCreateMoreThisCheck(const bool bHasServerAuthority, const bool bSourceConfigured,
        const FString& SourceId, const int32 SourceSlotIndex, const int32 CreatedCountThisCheck)
    {
        return IsSourceConfigured(bHasServerAuthority, bSourceConfigured, SourceId, SourceSlotIndex)
            && CreatedCountThisCheck >= 0 && CreatedCountThisCheck < MaximumCreationsPerCheck;
    }

    static bool CanAdmitLivingCreature(const bool bHasServerAuthority, const bool bSourceConfigured,
        const FString& SourceId, const int32 SourceSlotIndex, const int32 LivingSourceOwnedCount)
    {
        return IsSourceConfigured(bHasServerAuthority, bSourceConfigured, SourceId, SourceSlotIndex)
            && LivingSourceOwnedCount >= 0
            && LivingSourceOwnedCount < MaximumLivingCreaturesPerSource;
    }

    /** Counts finite, living, same-source samples using inclusive horizontal distance. */
    static bool TryCountLivingOwnedWithinRadius(const bool bHasServerAuthority,
        const bool bSourceConfigured, const FString& SourceId, const int32 SourceSlotIndex, const FVector& SourceOrigin,
        const TArray<FKalmalaEnvironmentalSpawnerCreatureSample>& Samples,
        int32& OutLivingCount)
    {
        OutLivingCount = -1;
        if (!IsSourceConfigured(bHasServerAuthority, bSourceConfigured, SourceId, SourceSlotIndex)
            || !FMath::IsFinite(SourceOrigin.X) || !FMath::IsFinite(SourceOrigin.Y)
            || !FMath::IsFinite(SourceOrigin.Z))
        {
            return false;
        }

        OutLivingCount = 0;
        constexpr double RadiusSquared = LocalLivingRadiusCentimetres * LocalLivingRadiusCentimetres;
        for (const FKalmalaEnvironmentalSpawnerCreatureSample& Sample : Samples)
        {
            if (!Sample.bIsLiving || Sample.SourceId != SourceId) continue;
            if (!FMath::IsFinite(Sample.Location.X) || !FMath::IsFinite(Sample.Location.Y)
                || !FMath::IsFinite(Sample.Location.Z))
            {
                OutLivingCount = -1;
                return false;
            }

            const double DeltaX = static_cast<double>(Sample.Location.X) - SourceOrigin.X;
            const double DeltaY = static_cast<double>(Sample.Location.Y) - SourceOrigin.Y;
            if (DeltaX * DeltaX + DeltaY * DeltaY <= RadiusSquared)
            {
                ++OutLivingCount;
            }
        }
        return true;
    }
};
