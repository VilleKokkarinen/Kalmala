#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaStatusHotbarWidget.generated.h"

struct FKalmalaSurvivalStatusSnapshot;
struct KALMALAUI_API FKalmalaStatusHotbarEntry
{
    FName Id;
    FString Name;
    FString TimerText;
    EKalmalaIcon Icon = EKalmalaIcon::Unknown;
    FName StatusIconId = NAME_None;
};

/** Transparent owner-local parent. No backgrounds, input, empty slots, or authority. */
UCLASS()
class KALMALAUI_API UKalmalaStatusHotbarWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static constexpr float StatusCellWidth = 72.0f;
    static constexpr float StatusCellContentHeight = 86.0f;
    static constexpr float StatusCellGap = 4.0f;
    static constexpr float MaximumStatusRowWidth = 384.0f;

    void SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScale, int32 Contrast);
    void ConfigureViewportPlacement(const FVector2D& Size, const FVector2D& Position);
    static TArray<FKalmalaStatusHotbarEntry> BuildEntries(const FKalmalaSurvivalStatusSnapshot& Snapshot);
    static FVector2D CalculateSize(int32 Count, int32 TextScale, FVector2D Viewport, float StatusGroupRightEdge);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UWrapBox> EntriesBox;
    UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> Timers;
    FString LastIdentity;
};
