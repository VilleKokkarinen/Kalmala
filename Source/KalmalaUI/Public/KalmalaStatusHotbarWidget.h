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
    FString Duration;
    EKalmalaIcon Icon = EKalmalaIcon::Unknown;
};

/** Transparent owner-local parent. No backgrounds, input, empty slots, or authority. */
UCLASS()
class KALMALAUI_API UKalmalaStatusHotbarWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScale, int32 Contrast);
    void ConfigureViewportPlacement(const FVector2D& Size, const FVector2D& Position);
    static TArray<FKalmalaStatusHotbarEntry> BuildEntries(const FKalmalaSurvivalStatusSnapshot& Snapshot);
    static FVector2D CalculateSize(int32 Count, int32 TextScale, FVector2D Viewport);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UWrapBox> EntriesBox;
    UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> Labels;
    UPROPERTY(Transient) TArray<TObjectPtr<class UTextBlock>> Durations;
    FString LastIdentity;
};
