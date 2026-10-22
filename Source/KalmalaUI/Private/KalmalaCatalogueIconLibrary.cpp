#include "KalmalaCatalogueIconLibrary.h"

#include "Engine/Texture2D.h"
#include "KalmalaIconWidget.h"
#include "KalmalaStatusIconLibrary.h"

bool FKalmalaCatalogueIconLibrary::GetTextureObjectPath(FName CanonicalId, FSoftObjectPath& OutObjectPath)
{
    OutObjectPath = FSoftObjectPath();

    EKalmalaIcon FallbackIcon = EKalmalaIcon::Unknown;
    int32 FallbackVariant = 0;
    if (!UKalmalaIconWidget::FindCatalogueIcon(CanonicalId, FallbackIcon, FallbackVariant))
        return false;

    FName StatusIconId = NAME_None;
    if (CanonicalId == TEXT("MendingScroll")) StatusIconId = TEXT("Mending");
    else if (CanonicalId == TEXT("HearthShieldScroll")) StatusIconId = TEXT("HearthShield");
    else if (CanonicalId == TEXT("BearsVigorScroll")) StatusIconId = TEXT("BearsVigor");
    else if (CanonicalId == TEXT("DeerCallScroll")) StatusIconId = TEXT("DeerCall");
    if (!StatusIconId.IsNone()) return FKalmalaStatusIconLibrary::GetTextureObjectPath(StatusIconId, OutObjectPath);

    FName TextureAssetId = CanonicalId;
    if (CanonicalId == TEXT("StoneKnife")) TextureAssetId = TEXT("ReedKnife");
    else if (CanonicalId == TEXT("StoneAxe")) TextureAssetId = TEXT("FieldHatchet");
    else if (CanonicalId == TEXT("CampFire")) TextureAssetId = TEXT("CampfireKit");
    else if (CanonicalId == TEXT("GardenPlot")) TextureAssetId = TEXT("GardenPlotKit");
    else if (CanonicalId == TEXT("TimberDoor")) TextureAssetId = TEXT("TimberDoorKit");
    else if (CanonicalId == TEXT("TimberStairs")) TextureAssetId = TEXT("TimberStairsKit");
    else if (CanonicalId == TEXT("CampBed")) TextureAssetId = TEXT("CampBedKit");
    else if (CanonicalId == TEXT("StandingLight")) TextureAssetId = TEXT("StandingLightKit");
    const FString AssetName = TextureAssetId.ToString();
    OutObjectPath = FSoftObjectPath(FString::Printf(
        TEXT("/Game/Kalmala/UI/Icons/Items/%s.%s"), *AssetName, *AssetName));
    return OutObjectPath.IsValid();
}

UTexture2D* FKalmalaCatalogueIconLibrary::LoadTexture(FName CanonicalId)
{
    FSoftObjectPath ObjectPath;
    if (!GetTextureObjectPath(CanonicalId, ObjectPath))
        return nullptr;

    TSoftObjectPtr<UTexture2D> Texture(ObjectPath);
    return Texture.LoadSynchronous();
}
