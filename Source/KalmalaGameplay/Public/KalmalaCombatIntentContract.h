#pragma once

#include "CoreMinimal.h"

/**
 * Server-derived validation facts for the first combat and progression
 * mutations. This deliberately contains no client payload fields: future RPC
 * handlers must populate every fact from the owning pawn, server trace,
 * descriptor lookup, and persistence transaction before a mutation begins.
 */
struct KALMALAGAMEPLAY_API FKalmalaCombatIntentContract
{
    static bool IsAttackMutationAllowed(
        const bool bServerAuthority,
        const bool bOwnsRequestingPawn,
        const bool bSameWorld,
        const bool bRequestSequenceIsNew,
        const uint32 RequestSequence,
        const bool bFiniteConfiguration,
        const bool bActionAvailable,
        const bool bTargetInRangeAndVisible,
        const bool bTargetAlive)
    {
        return bServerAuthority && bOwnsRequestingPawn && bSameWorld
            && bRequestSequenceIsNew && RequestSequence != 0
            && bFiniteConfiguration && bActionAvailable
            && bTargetInRangeAndVisible && bTargetAlive;
    }

    static bool IsProgressionMutationAllowed(
        const bool bServerAuthority,
        const bool bOwnsRequestingPawn,
        const bool bSameWorld,
        const bool bRequestSequenceIsNew,
        const uint32 RequestSequence,
        const bool bCanonicalExistingContentId,
        const bool bTargetInRangeAndVisible,
        const bool bNotAlreadyCommitted,
        const bool bPersistenceReady)
    {
        return bServerAuthority && bOwnsRequestingPawn && bSameWorld
            && bRequestSequenceIsNew && RequestSequence != 0
            && bCanonicalExistingContentId && bTargetInRangeAndVisible
            && bNotAlreadyCommitted && bPersistenceReady;
    }
};
