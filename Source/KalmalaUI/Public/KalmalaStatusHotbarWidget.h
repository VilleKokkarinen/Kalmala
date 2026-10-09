#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaStatusHotbarWidget.generated.h"

struct FKalmalaSurvivalStatusSnapshot;
enum class EKalmalaStatusRefreshPolicy : uint8
{
    None,
    RemainingIncreased,
    ExpiryIncreased,
    AuthorityStampChanged
};

enum class EKalmalaStatusCueKind : uint8
{
    None,
    Started,
    Refreshed,
    Ended
};

struct KALMALAUI_API FKalmalaStatusHotbarEntry
{
    FName Id;
    FString Name;
    FString Duration;
    EKalmalaIcon Icon = EKalmalaIcon::Unknown;
    float RefreshValue = 0.0f;
    EKalmalaStatusRefreshPolicy RefreshPolicy = EKalmalaStatusRefreshPolicy::None;
    EKalmalaStatusCueKind Cue = EKalmalaStatusCueKind::None;
};

struct KALMALAUI_API FKalmalaStatusHotbarTransition
{
    FName Id;
    EKalmalaStatusCueKind Kind = EKalmalaStatusCueKind::None;
    FKalmalaStatusHotbarEntry Entry;
};

/** Transparent owner-local parent. No backgrounds, input, empty slots, or authority. */
UCLASS()
class KALMALAUI_API UKalmalaStatusHotbarWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScale, int32 Contrast,
        bool bResetTransitionHistory = false);
    static TArray<FKalmalaStatusHotbarEntry> BuildEntries(const FKalmalaSurvivalStatusSnapshot& Snapshot);
    static TArray<FKalmalaStatusHotbarTransition> BuildTransitions(
        const TArray<FKalmalaStatusHotbarEntry>& Previous,
        const TArray<FKalmalaStatusHotbarEntry>& Current,
        bool bHasBaseline);
    static bool ShouldReplaceActiveCue(EKalmalaStatusCueKind Existing, EKalmalaStatusCueKind Incoming);
    static float CalculateCueOpacity(float ElapsedSeconds, float DurationSeconds, bool bAnimate, bool bReducedMotion);
#if !UE_BUILD_SHIPPING
    EKalmalaStatusCueKind GetActiveCueKindForVerification(FName Id) const;
    float GetActiveCueOpacityForVerification(FName Id) const;
#endif
    static FVector2D CalculateSize(int32 Count, int32 TextScale, FVector2D Viewport);
protected:
    virtual void NativeOnInitialized() override;
private:
    struct FActiveCue
    {
        FKalmalaStatusHotbarEntry Entry;
        EKalmalaStatusCueKind Kind = EKalmalaStatusCueKind::None;
        double StartedAtSeconds = 0.0;
        double ExpiresAtSeconds = 0.0;
    };

    UPROPERTY(Transient) TObjectPtr<class UWrapBox> EntriesBox;
    UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> Labels;
    UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> Durations;
    UPROPERTY(Transient) TArray<TObjectPtr<class UKalmalaIconWidget>> Icons;
    TArray<FKalmalaStatusHotbarEntry> PreviousEntries;
    TMap<FName, FActiveCue> ActiveCues;
    TArray<FName> CueOrder;
    double TransitionArmedAtSeconds = 0.0;
    bool bHasTransitionBaseline = false;
    FString LastIdentity;
};
