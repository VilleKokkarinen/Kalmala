#pragma once

#include "CoreMinimal.h"
#include "KalmalaForestLayout.h"

/** Stable mapping from shared tree kind to its existing authoritative timber source. */
struct KALMALAGAMEPLAY_API FKalmalaForestResourceContract
{
    static FName GetSourceId(const EKalmalaForestTreeKind TreeKind)
    {
        switch (TreeKind)
        {
        case EKalmalaForestTreeKind::MeadowsBirch:
        case EKalmalaForestTreeKind::FreezingTundraBirch:
            return TEXT("meadows-birch-trunk");
        case EKalmalaForestTreeKind::MeadowsAspen:
            return TEXT("meadows-aspen-trunk");
        case EKalmalaForestTreeKind::ElderwoodIronheart:
            return TEXT("elderwood-ironheart-trunk");
        default:
            return NAME_None;
        }
    }
};
