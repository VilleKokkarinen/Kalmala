#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaSupportSelectionSubsystem.generated.h"

class UBorder;
class UHorizontalBox;
class UTextBlock;

enum class EKalmalaSupportGlyph : uint8
{
    Mending,
    HearthShield,
    BearsVigor,
    DeerCall
};

UCLASS()
class KALMALAUI_API UKalmalaSupportGlyphWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetGlyphState(EKalmalaSupportGlyph InGlyph, bool bInLearned, bool bInSelected);
    void SetGlyphContrast(int32 ContrastMode);

protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    EKalmalaSupportGlyph Glyph = EKalmalaSupportGlyph::Mending;
    bool bLearned = false;
    bool bSelected = false;
    int32 Contrast = 0;
};

UCLASS()
class KALMALAUI_API UKalmalaSupportSelectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetSnapshot(int32 SelectedIndex, uint8 LearnedMask, int32 TextScale, int32 Contrast);
    static FString BuildSelectedText(int32 SelectedIndex, bool bLearned);
    FString GetSelectionSummary() const;
    FVector2D GetRequiredHudSize() const;

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyAccessibility(int32 TextScale, int32 Contrast);

    UPROPERTY(Transient) TObjectPtr<UBorder> Background;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Heading;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedText;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> GlyphRow;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> Cards;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaSupportGlyphWidget>> Glyphs;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> Labels;
    TArray<uint8> VisualStates;
    int32 LastTextScale = INDEX_NONE;
    int32 LastContrast = INDEX_NONE;
};

/** Keeps the support selection cue independent from the legacy pack HUD. */
UCLASS()
class KALMALAUI_API UKalmalaSupportSelectionSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaSupportSelectionSubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return !IsTemplate(); }
    const UKalmalaSupportSelectionWidget* GetSelectionWidget() const { return Widget; }

private:
    void ReleaseWidget();

    UPROPERTY(Transient) TObjectPtr<UKalmalaSupportSelectionWidget> Widget;
    TWeakObjectPtr<class APlayerController> Controller;
    FVector2D LastViewportSize = FVector2D::ZeroVector;
    FVector2D LastWidgetSize = FVector2D::ZeroVector;
};
