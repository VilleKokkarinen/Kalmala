#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaIconWidget.generated.h"

/** Original line art shared by read-only HUD and catalogue presentation. */
enum class EKalmalaIcon : uint8
{
    Unknown, Drop, Sun, Snow, Cloud, Storm, Bowl, Shield, Cross, Paw, Antlers,
    Log, Fibre, Rock, Ingot, Hide, Meat, Root, Seed, Axe, Knife, Pick, Hammer,
    Floor, Wall, Roof, Bench, Rack, Cauldron, Pan, Forge, Chest, Fire, Bed, Fence, Lamp, Skiff
};

UCLASS()
class KALMALAUI_API UKalmalaIconWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetIcon(EKalmalaIcon InIcon, int32 InVariant = 0);
    static bool FindCatalogueIcon(FName CanonicalId, EKalmalaIcon& OutIcon, int32& OutVariant);
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override;
private:
    EKalmalaIcon Icon = EKalmalaIcon::Unknown;
    int32 Variant = 0;
};
