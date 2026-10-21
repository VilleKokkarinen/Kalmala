#include "KalmalaIconWidget.h"
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"

void UKalmalaIconWidget::SetIcon(EKalmalaIcon InIcon, int32 InVariant)
{
    SetIsFocusable(false);
    Icon = InIcon; Variant = InVariant; CatalogueId = NAME_None; CatalogueTexture = nullptr;
    Invalidate(EInvalidateWidget::Paint);
}

void UKalmalaIconWidget::SetCatalogueIcon(FName CanonicalId)
{
    SetIsFocusable(false);
    if (CatalogueId == CanonicalId && CatalogueTexture)
        return;
    CatalogueId = CanonicalId;
    const bool bKnownId = FindCatalogueIcon(CanonicalId, Icon, Variant);
    if (bKnownId)
        CatalogueTexture = FKalmalaCatalogueIconLibrary::LoadTexture(CanonicalId);
    else
        CatalogueTexture = nullptr;
    Invalidate(EInvalidateWidget::Paint);
}

void UKalmalaIconWidget::SetCueColor(const FLinearColor& InColor)
{
    if (CueColor.Equals(InColor, 0.001f)) return;
    CueColor = InColor;
    Invalidate(EInvalidateWidget::Paint);
}

int32 UKalmalaIconWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& C,
    FSlateWindowElementList& E, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
    Layer = Super::NativePaint(Args, G, C, E, Layer, Style, bEnabled) + 1;
    const FVector2D Size = G.GetLocalSize();
    if (CatalogueTexture)
    {
        FSlateBrush TextureBrush;
        TextureBrush.SetResourceObject(CatalogueTexture);
        TextureBrush.DrawAs = ESlateBrushDrawType::Image;
        TextureBrush.ImageSize = Size;
        FSlateDrawElement::MakeBox(E, Layer, G.ToPaintGeometry(), &TextureBrush,
            ESlateDrawEffect::None, Style.GetColorAndOpacityTint());
        return Layer + 1;
    }
    const float Scale = FMath::Min(Size.X, Size.Y) / 36.0f;
    const FVector2D Offset = (Size - FVector2D(36, 36) * Scale) * .5;
    if (CueColor.A > 0.0f)
    {
        TArray<FVector2D> CueRing;
        for (int32 Index = 0; Index <= 24; ++Index)
        {
            const float Angle = 2.0f * PI * static_cast<float>(Index) / 24.0f;
            CueRing.Add(Offset + (FVector2D(18.0f, 18.0f)
                + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 16.5f) * Scale);
        }
        FSlateDrawElement::MakeLines(E, Layer, G.ToPaintGeometry(), CueRing, ESlateDrawEffect::None,
            CueColor, true, 2.4f * Scale);
    }
    const FLinearColor Ink = FKalmalaUITheme::Get().TextColor(false, UKalmalaSettingsWidget::GetContrastMode());
    const auto Path = [&](TArray<FVector2D> Points, bool bClose = false)
    {
        if (bClose && Points.Num()) { const FVector2D First = Points[0]; Points.Add(First); }
        for (auto& P : Points) P = Offset + P * Scale;
        FSlateDrawElement::MakeLines(E, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None,
            FLinearColor::Black * Style.GetColorAndOpacityTint(), true, 3.8f);
        FSlateDrawElement::MakeLines(E, Layer + 1, G.ToPaintGeometry(), Points, ESlateDrawEffect::None,
            Ink * Style.GetColorAndOpacityTint(), true, 1.8f);
    };
    const auto Circle = [&](FVector2D Centre, float Radius)
    {
        TArray<FVector2D> Points;
        for (int32 I = 0; I <= 24; ++I) { const float A = 2 * PI * I / 24; Points.Add(Centre + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius); }
        Path(Points);
    };
    switch (Icon)
    {
    case EKalmalaIcon::Drop: Path({{18,4},{9,19},{9,26},{14,31},{22,31},{27,26},{27,19}}, true); Path({{13,22},{14,27},{18,28}}); break;
    case EKalmalaIcon::Sun:
        Circle({18,18},7);
        for (int32 I=0; I<8; ++I) { const float A=PI*I/4; const FVector2D D(FMath::Cos(A),FMath::Sin(A)); Path({FVector2D(18,18)+D*11,FVector2D(18,18)+D*15}); } break;
    case EKalmalaIcon::Snow:
        for (int32 I=0; I<3; ++I) { const float A=PI*I/3; const FVector2D D(FMath::Cos(A),FMath::Sin(A)); Path({FVector2D(18,18)-D*13,FVector2D(18,18)+D*13}); } Path({{8,10},{8,16},{14,16}}); Path({{22,20},{28,20},{28,26}}); break;
    case EKalmalaIcon::Cloud: case EKalmalaIcon::Storm:
        Path({{5,23},{4,16},{8,12},{13,12},{16,6},{23,7},{26,13},{31,15},{31,23},{5,23}});
        if (Icon==EKalmalaIcon::Storm) Path({{20,23},{14,28},{20,28},{16,34}});
        else { Path({{10,27},{8,31}}); Path({{25,27},{23,31}}); } break;
    case EKalmalaIcon::Bowl: Path({{5,18},{8,27},{15,31},{23,31},{29,27},{31,18}},true); Path({{11,12},{9,8},{12,4}}); Path({{22,12},{20,8},{23,4}}); break;
    case EKalmalaIcon::Shield:
        Path({{7,5},{12,8},{18,4},{24,8},{29,5},{28,23},{24,29},{18,33},{12,29},{8,23}},true);
        Path({{11,11},{18,8},{25,11},{24,22},{18,29},{12,22}},true);
        Path({{18,12},{18,24}}); Path({{13,17},{23,17}}); break;
    case EKalmalaIcon::Cross: Circle({18,18},13); Path({{18,10},{18,26}}); Path({{10,18},{26,18}}); break;
    case EKalmalaIcon::Paw: Circle({9,10},3); Circle({17,7},3); Circle({25,10},3); Path({{10,23},{13,17},{23,17},{28,24},{25,30},{12,30}},true); break;
    case EKalmalaIcon::Antlers: Path({{18,31},{18,18},{10,12},{9,4}}); Path({{18,18},{26,12},{27,4}}); Path({{10,12},{4,9},{4,4}}); Path({{26,12},{32,9},{32,4}}); break;
    case EKalmalaIcon::Log: Path({{9,7},{28,7},{31,12},{31,25},{27,29},{9,29}},true); Circle({9,18},10); Circle({9,18},4); Path({{18,12},{27,12}}); break;
    case EKalmalaIcon::Gathering: Path({{10,31},{16,12},{14,5}}); Path({{16,31},{21,10},{25,5}}); Path({{22,31},{27,14},{31,11}}); Path({{9,24},{25,24}}); break;
    case EKalmalaIcon::Rock: Path({{5,26},{8,14},{17,6},{27,11},{32,25},{23,31},{12,31}},true); Path({{8,14},{18,19},{27,11}}); Path({{18,19},{23,31}}); break;
    case EKalmalaIcon::Ingot: Path({{4,21},{10,12},{26,12},{32,21},{27,28},{9,28}},true); Path({{4,21},{32,21}}); break;
    case EKalmalaIcon::Hide: Path({{8,5},{15,10},{22,10},{28,5},{31,13},{25,17},{27,27},{22,31},{18,28},{13,31},{8,27},{10,17},{5,13}},true); break;
    case EKalmalaIcon::Meat: Path({{8,9},{17,5},{27,10},{30,20},{22,29},{10,30},{5,22}},true); Circle({18,17},5); Path({{9,24},{14,26},{23,23}}); break;
    case EKalmalaIcon::Root:
        if (Variant==0) Path({{11,12},{25,12},{17,31}},true);
        else if (Variant==3) Path({{18,12},{9,20},{10,28},{18,32},{27,28},{28,20}},true);
        else { Circle({18,22},10); if(Variant==2) Path({{18,32},{16,35}}); }
        Path({{18,12},{14,5},{9,4}}); Path({{18,12},{22,5},{27,4}}); break;
    case EKalmalaIcon::Seed: Path({{8,12},{28,12},{27,31},{9,31}},true); Path({{8,8},{28,8}}); Circle({18,22},4); break;
    case EKalmalaIcon::Axe: Path({{11,32},{21,7}}); Path({{19,7},{29,8},{31,19},{23,20},{17,13}},true); break;
    case EKalmalaIcon::Spear: Path({{5,31},{29,7}}); Path({{23,7},{31,5},{29,13}},true); Path({{13,23},{18,28}}); break;
    case EKalmalaIcon::Sword:
        Path({{5,3},{12,7},{25,21},{21,25},{7,12}},true);
        Path({{5,3},{22,22}}); Path({{16,27},{27,16},{29,18},{18,29}},true);
        Path({{23,25},{28,30},{31,27},{26,22}},true);
        Path({{25,27},{28,26}}); Circle({31,31},2); break;
    case EKalmalaIcon::Knife: Path({{10,31},{15,22},{29,5},{29,15},{19,25},{16,23}},true); Path({{12,21},{20,27}}); break;
    case EKalmalaIcon::Pick: Path({{12,32},{21,9}}); Path({{5,13},{17,6},{27,8},{32,17},{23,12},{15,12},{5,13}}); break;
    case EKalmalaIcon::Mace: Path({{15,17},{29,31},{32,28},{18,14}},true); Path({{5,7},{13,3},{22,12},{17,21},{8,19},{3,12}},true); break;
    case EKalmalaIcon::Hammer: Path({{14,32},{20,14}}); Path({{9,5},{28,10},{26,18},{7,13}},true); break;
    case EKalmalaIcon::Floor: Path({{4,19},{17,8},{32,17},{19,29}},true); Path({{9,22},{22,11}}); Path({{14,25},{27,14}}); break;
    case EKalmalaIcon::Wall: Path({{7,5},{29,5},{29,31},{7,31}},true); Path({{14,5},{14,31}}); Path({{22,5},{22,31}}); Path({{7,12},{29,25}}); break;
    case EKalmalaIcon::Roof: Path({{3,21},{18,5},{33,21},{27,25},{18,15},{9,25}},true); Path({{11,17},{18,9},{25,17}}); break;
    case EKalmalaIcon::Bench: Path({{4,13},{32,13},{32,19},{4,19}},true); Path({{8,19},{8,31}}); Path({{28,19},{28,31}}); Path({{8,26},{28,26}}); break;
    case EKalmalaIcon::Rack: Path({{6,31},{10,5},{26,5},{30,31}}); Path({{8,14},{28,14}}); Path({{14,14},{14,24},{19,26},{23,21},{23,14}}); break;
    case EKalmalaIcon::Cauldron: Path({{7,12},{29,12},{31,23},{26,30},{10,30},{5,23}},true); Path({{9,12},{9,7},{27,7},{27,12}}); Path({{12,30},{10,34}}); Path({{24,30},{26,34}}); break;
    case EKalmalaIcon::Pan: Circle({14,21},10); Path({{22,14},{30,4},{33,7},{25,17}}); break;
    case EKalmalaIcon::Forge: Path({{4,31},{4,19},{10,19},{10,8},{25,8},{25,19},{32,19},{32,31}},true); Path({{12,30},{12,23},{18,20},{24,23},{24,30}}); break;
    case EKalmalaIcon::Kiln: Path({{4,31},{5,20},{9,13},{14,9},{22,9},{27,13},{31,20},{32,31}},true); Path({{12,31},{12,25},{15,21},{21,21},{24,25},{24,31}},true); Path({{14,9},{14,4},{23,4},{23,9}}); Path({{11,17},{25,17}}); break;
    case EKalmalaIcon::Smelter: Path({{4,31},{4,18},{9,18},{9,10},{24,10},{24,18},{32,18},{32,31}},true); Path({{13,31},{13,25},{17,21},{22,25},{22,31}},true); Path({{13,10},{13,4},{24,4},{24,10}}); Path({{25,21},{33,21},{33,24}}); break;
    case EKalmalaIcon::Chest: Path({{5,11},{30,11},{30,31},{5,31}},true); Path({{5,19},{30,19}}); Path({{15,16},{20,16},{20,23},{15,23}},true); break;
    case EKalmalaIcon::Fire: Path({{18,4},{25,14},{26,21},{23,27},{13,27},{9,21},{10,13},{15,18}},true); Path({{6,31},{30,31}}); break;
    case EKalmalaIcon::Discovery: Circle({14,14},8); Path({{20,20},{31,31}}); Path({{14,9},{14,19}}); Path({{9,14},{19,14}}); break;
    case EKalmalaIcon::Level: Circle({18,18},13); Path({{10,22},{18,14},{26,22}}); Path({{18,14},{18,26}}); break;
    case EKalmalaIcon::Helmet:
        Path({{7,30},{7,15},{10,8},{18,4},{26,8},{29,15},{29,30},{23,32},{23,22},{13,22},{13,32}},true);
        Path({{11,16},{12,11},{18,8},{24,11},{25,16},{21,18},{15,18}},true);
        Path({{18,8},{18,25}}); Path({{8,20},{13,22}}); Path({{28,20},{23,22}});
        Path({{10,25},{10,29}}); Path({{26,25},{26,29}}); break;
    case EKalmalaIcon::Cape:
        Path({{12,6},{14,3},{22,3},{24,6},{22,11},{26,18},{30,32},{22,30},{18,32},{14,30},{6,32},{10,18},{14,11}},true);
        Path({{12,6},{18,10},{24,6}}); Circle({18,10},2);
        Path({{14,14},{11,27}}); Path({{22,14},{25,27}}); Path({{18,16},{18,28}}); break;
    case EKalmalaIcon::Tunic:
        Path({{12,5},{7,8},{3,17},{9,20},{11,15},{11,32},{25,32},{25,15},{27,20},{33,17},{29,8},{24,5},{21,9},{15,9}},true);
        Path({{8,9},{12,12},{11,17}}); Path({{28,9},{24,12},{25,17}});
        Path({{18,10},{18,24}}); Path({{11,25},{25,25}}); Path({{11,29},{25,29}});
        Path({{16,24},{20,24},{20,28},{16,28}},true); break;
    case EKalmalaIcon::Leggings:
        Path({{8,4},{28,4},{26,14},{25,32},{20,32},{18,19},{16,32},{11,32},{10,14}},true);
        Path({{8,9},{28,9}}); Path({{14,9},{18,15},{22,9}});
        Path({{11,21},{15,21},{15,26},{11,26}},true);
        Path({{21,21},{25,21},{25,26},{21,26}},true); Path({{18,4},{18,8}}); break;
    case EKalmalaIcon::Boots:
        Path({{7,5},{15,5},{15,20},{12,26},{12,31},{3,31},{3,27},{7,21}},true);
        Path({{21,5},{29,5},{29,21},{33,27},{33,31},{24,31},{24,26},{21,20}},true);
        Path({{7,9},{15,9}}); Path({{21,9},{29,9}});
        Path({{9,13},{13,15},{9,17},{13,19}}); Path({{23,13},{27,15},{23,17},{27,19}});
        Path({{3,28},{12,28}}); Path({{24,28},{33,28}}); break;
    case EKalmalaIcon::Necklace:
        Path({{8,5},{6,12},{8,20},{14,25},{22,25},{28,20},{30,12},{28,5}});
        Path({{10,7},{9,13},{11,19},{16,22}}); Path({{26,7},{27,13},{25,19},{20,22}});
        Path({{18,21},{23,26},{18,32},{13,26}},true); Path({{18,24},{18,29}}); break;
    case EKalmalaIcon::Ring:
        Circle({18,23},10); Circle({18,23},6);
        Path({{12,8},{15,4},{21,4},{24,8},{18,15}},true);
        Path({{12,8},{24,8}}); Path({{15,4},{18,8},{21,4}}); Path({{18,8},{18,15}}); break;
    case EKalmalaIcon::Bone:
        Path({{7,29},{5,25},{8,21},{12,22},{23,11},{22,7},{26,4},{31,7},{32,12},{28,15},{25,14},{14,25},{13,30},{9,32}},true);
        Path({{23,11},{28,15}}); break;
    case EKalmalaIcon::String:
        Circle({18,17},11); Circle({18,17},7); Circle({18,17},3);
        Path({{26,24},{30,28},{25,33}}); break;
    case EKalmalaIcon::Bow:
        Path({{5,31},{8,17},{19,7},{31,5},{5,31}});
        Path({{16,14},{21,19}}); break;
    case EKalmalaIcon::Arrow:
        Path({{6,30},{30,6},{23,8},{28,13},{30,6}});
        Path({{6,30},{6,22},{14,22},{14,30},{6,30}},true); break;
    default: Path({{18,5},{31,18},{18,31},{5,18}},true); Path({{15,13},{18,11},{21,13},{18,18},{18,22}}); Circle({18,27},1); break;
    }
    // Authored monochrome accents distinguish related materials/foods/tools at small size.
    for (int32 I=0; I<FMath::Clamp(Variant,0,7); ++I) Path({{float(5+I*4),33},{float(5+I*4),35}});
    return Layer + 1;
}

bool UKalmalaIconWidget::FindCatalogueIcon(FName Id, EKalmalaIcon& OutIcon, int32& OutVariant)
{
    struct FMapping { const TCHAR* Id; EKalmalaIcon Icon; int32 Variant; };
    // Runtime/save identities; JSON's external construction names resolve to these IDs.
    static const FMapping Mappings[] = {
        {TEXT("BoneFragment"),EKalmalaIcon::Bone,0},
        {TEXT("BowString"),EKalmalaIcon::String,0}, {TEXT("Sinew"),EKalmalaIcon::Bone,0},
        {TEXT("Bow"),EKalmalaIcon::Bow,0}, {TEXT("LightBow"),EKalmalaIcon::Bow,0},
        {TEXT("WoodenArrow"),EKalmalaIcon::Arrow,0}, {TEXT("StoneArrow"),EKalmalaIcon::Arrow,0},
        {TEXT("BronzeArrow"),EKalmalaIcon::Arrow,0},
        {TEXT("Wood"),EKalmalaIcon::Log,0}, {TEXT("Lightwood"),EKalmalaIcon::Log,1}, {TEXT("Densewood"),EKalmalaIcon::Log,2},
        {TEXT("Coal"),EKalmalaIcon::Rock,1}, {TEXT("Stone"),EKalmalaIcon::Rock,0}, {TEXT("Iron"),EKalmalaIcon::Ingot,0},
        {TEXT("Bronze"),EKalmalaIcon::Ingot,2}, {TEXT("Copper"),EKalmalaIcon::Ingot,3}, {TEXT("Tin"),EKalmalaIcon::Ingot,4},
        {TEXT("CopperOre"),EKalmalaIcon::Rock,4}, {TEXT("TinOre"),EKalmalaIcon::Rock,5}, {TEXT("IronOre"),EKalmalaIcon::Rock,6},
        {TEXT("Amber"),EKalmalaIcon::Rock,2}, {TEXT("FrostSalt"),EKalmalaIcon::Snow,0},
        {TEXT("CampfireKit"),EKalmalaIcon::Fire,0}, {TEXT("CampFire"),EKalmalaIcon::Fire,0},
        {TEXT("WorkbenchKit"),EKalmalaIcon::Bench,0}, {TEXT("ForgeKit"),EKalmalaIcon::Forge,0},
        {TEXT("CharcoalKilnKit"),EKalmalaIcon::Kiln,0},
        {TEXT("SmelterKit"),EKalmalaIcon::Smelter,0},
        {TEXT("WorkbenchToolRackKit"),EKalmalaIcon::Rack,1}, {TEXT("ForgeAnvilKit"),EKalmalaIcon::Ingot,1},
        {TEXT("GrindingStoneKit"),EKalmalaIcon::Rock,3}, {TEXT("StorageKit"),EKalmalaIcon::Chest,0}, {TEXT("CookingRackKit"),EKalmalaIcon::Rack,0},
        {TEXT("FryingPanKit"),EKalmalaIcon::Pan,0}, {TEXT("CauldronKit"),EKalmalaIcon::Cauldron,0},
        {TEXT("FloorKit"),EKalmalaIcon::Floor,0}, {TEXT("WallKit"),EKalmalaIcon::Wall,0}, {TEXT("RoofKit"),EKalmalaIcon::Roof,0},
        {TEXT("BoarMeat"),EKalmalaIcon::Meat,0}, {TEXT("DeerMeat"),EKalmalaIcon::Meat,1},
        {TEXT("BoarHide"),EKalmalaIcon::Hide,0}, {TEXT("DeerHide"),EKalmalaIcon::Hide,1},
        {TEXT("CookedBoarMeat"),EKalmalaIcon::Meat,2}, {TEXT("CookedDeerMeat"),EKalmalaIcon::Meat,3},
        {TEXT("HearthBroth"),EKalmalaIcon::Bowl,0}, {TEXT("MeatStew"),EKalmalaIcon::Bowl,1},
        {TEXT("RootVegetableSoup"),EKalmalaIcon::Bowl,2}, {TEXT("RoastedRootVegetables"),EKalmalaIcon::Pan,1}, {TEXT("DeerRootRoast"),EKalmalaIcon::Pan,2},
        {TEXT("Carrot"),EKalmalaIcon::Root,0}, {TEXT("Potato"),EKalmalaIcon::Root,1}, {TEXT("Rutabaga"),EKalmalaIcon::Root,2}, {TEXT("Onion"),EKalmalaIcon::Root,3},
        {TEXT("CarrotSeed"),EKalmalaIcon::Seed,0}, {TEXT("PotatoSeed"),EKalmalaIcon::Seed,1}, {TEXT("RutabagaSeed"),EKalmalaIcon::Seed,2}, {TEXT("OnionSeed"),EKalmalaIcon::Seed,3},
        {TEXT("StoneKnife"),EKalmalaIcon::Knife,0}, {TEXT("StoneAxe"),EKalmalaIcon::Axe,0}, {TEXT("StonePick"),EKalmalaIcon::Pick,0},
        {TEXT("ReedKnife"),EKalmalaIcon::Knife,0}, {TEXT("FieldHatchet"),EKalmalaIcon::Axe,0},
        {TEXT("StoneSpear"),EKalmalaIcon::Spear,0},
        {TEXT("BronzeAxe"),EKalmalaIcon::Axe,1}, {TEXT("BronzeKnife"),EKalmalaIcon::Knife,1},
        {TEXT("BronzePick"),EKalmalaIcon::Pick,1}, {TEXT("BronzeSpear"),EKalmalaIcon::Spear,1},
        {TEXT("IronAxe"),EKalmalaIcon::Axe,2}, {TEXT("IronKnife"),EKalmalaIcon::Knife,2},
        {TEXT("IronPick"),EKalmalaIcon::Pick,2}, {TEXT("IronSpear"),EKalmalaIcon::Spear,2},
        {TEXT("StoneMace"),EKalmalaIcon::Mace,0}, {TEXT("BronzeMace"),EKalmalaIcon::Mace,1}, {TEXT("IronMace"),EKalmalaIcon::Mace,2},
        {TEXT("BronzeSword"),EKalmalaIcon::Sword,1}, {TEXT("IronSword"),EKalmalaIcon::Sword,2},
        {TEXT("WoodenClub"),EKalmalaIcon::Mace,0},
        {TEXT("ConstructionHammer"),EKalmalaIcon::Hammer,0},
        {TEXT("WoodShield"),EKalmalaIcon::Shield,1},
        {TEXT("BoarHideHelmet"),EKalmalaIcon::Helmet,0}, {TEXT("BoarHideCape"),EKalmalaIcon::Cape,0},
        {TEXT("BoarHideTunic"),EKalmalaIcon::Tunic,0}, {TEXT("BoarHideLeggings"),EKalmalaIcon::Leggings,0},
        {TEXT("BoarHideBoots"),EKalmalaIcon::Boots,0},
        {TEXT("MendingScroll"),EKalmalaIcon::Cross,0}, {TEXT("HearthShieldScroll"),EKalmalaIcon::Shield,0},
        {TEXT("BearsVigorScroll"),EKalmalaIcon::Paw,0}, {TEXT("DeerCallScroll"),EKalmalaIcon::Antlers,0}
    };
    for (const auto& M : Mappings)
        if (Id == M.Id) { OutIcon = M.Icon; OutVariant = M.Variant; return true; }
    OutIcon = EKalmalaIcon::Unknown; OutVariant = 0;
    return false;
}
