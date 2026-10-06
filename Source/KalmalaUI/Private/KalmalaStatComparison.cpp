#include "KalmalaStatComparison.h"
#include "KalmalaUITheme.h"

FKalmalaStatComparisonLine FKalmalaStatComparisonLine::FromValues(
    const FString& StatLabel,
    const int32 CurrentValue,
    const int32 SelectedValue,
    const bool bHigherIsBetter)
{
    if (StatLabel.TrimStartAndEnd().IsEmpty())
        return Unavailable(TEXT("Stat"), TEXT("the stat label is missing"));

    const int64 Delta = static_cast<int64>(SelectedValue) - static_cast<int64>(CurrentValue);
    const FString DeltaText = Delta > 0 ? TEXT("+") + LexToString(Delta) : LexToString(Delta);

    FKalmalaStatComparisonLine Line;
    Line.Text = FString::Printf(TEXT("%s %s (%s)"),
        *StatLabel, *LexToString(SelectedValue), *DeltaText);
    Line.Tone = Delta == 0 ? EKalmalaStatComparisonTone::Neutral
        : ((Delta > 0) == bHigherIsBetter
            ? EKalmalaStatComparisonTone::Improved
            : EKalmalaStatComparisonTone::Worsened);
    return Line;
}

FKalmalaStatComparisonLine FKalmalaStatComparisonLine::Unavailable(
    const FString& StatLabel,
    const FString& Reason)
{
    FKalmalaStatComparisonLine Line;
    const FString SafeLabel = StatLabel.TrimStartAndEnd().IsEmpty() ? TEXT("Stat") : StatLabel;
    Line.Text = Reason.TrimStartAndEnd().IsEmpty()
        ? FString::Printf(TEXT("%s comparison unavailable."), *SafeLabel)
        : FString::Printf(TEXT("%s comparison unavailable: %s."), *SafeLabel, *Reason);
    Line.Tone = EKalmalaStatComparisonTone::Unavailable;
    return Line;
}

FLinearColor FKalmalaStatComparisonLine::ResolveColor(
    const FKalmalaUITheme& Theme,
    const int32 ContrastMode) const
{
    switch (Tone)
    {
    case EKalmalaStatComparisonTone::Improved:
        return Theme.StatusCueStartedColor;
    case EKalmalaStatComparisonTone::Worsened:
        return Theme.StatusCueEndedColor;
    case EKalmalaStatComparisonTone::Neutral:
    case EKalmalaStatComparisonTone::Unavailable:
    default:
        return Theme.TextColor(false, ContrastMode);
    }
}
