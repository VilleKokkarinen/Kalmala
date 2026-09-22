#pragma once
#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "KalmalaInventorySubsystem.generated.h"

class UTextBlock;
class UBorder;
class UHorizontalBox;

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
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
    EKalmalaSupportGlyph Glyph = EKalmalaSupportGlyph::Mending;
    bool bLearned = false;
    bool bSelected = false;
};

UCLASS()
class KALMALAUI_API UKalmalaInventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetPackText(const FString& Text);
    void SetSupportGlyphState(int32 Index, EKalmalaSupportGlyph Glyph, bool bLearned, bool bSelected);
    void SetSupportGlyphsVisible(bool bVisible);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackText;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SupportGlyphRow;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> SupportGlyphCards;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaSupportGlyphWidget>> SupportGlyphs;
    TArray<uint8> SupportGlyphVisualStates;
};

/** Read-only local inventory presentation, with no input bindings or network requests. */
UCLASS()
class KALMALAUI_API UKalmalaInventorySubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaInventorySubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryWidget> Widget;
    bool bVerified = false;
};
