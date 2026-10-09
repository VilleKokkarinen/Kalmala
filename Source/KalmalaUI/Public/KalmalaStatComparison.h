#pragma once

#include "CoreMinimal.h"

struct FKalmalaUITheme;

/** Local presentation result for one current-versus-selected stat line. */
enum class EKalmalaStatComparisonTone : uint8
{
    Improved,
    Worsened,
    Neutral,
    Unavailable
};

/** Formats an existing numeric stat without defining or authorizing gameplay values. */
struct KALMALAUI_API FKalmalaStatComparisonLine
{
    FString Text;
    EKalmalaStatComparisonTone Tone = EKalmalaStatComparisonTone::Unavailable;

    static FKalmalaStatComparisonLine FromValues(
        const FString& StatLabel,
        int32 CurrentValue,
        int32 SelectedValue,
        bool bHigherIsBetter);
    static FKalmalaStatComparisonLine Unavailable(
        const FString& StatLabel,
        const FString& Reason);

    FLinearColor ResolveColor(const FKalmalaUITheme& Theme, int32 ContrastMode) const;
};
