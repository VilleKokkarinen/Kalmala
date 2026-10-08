#include "KalmalaCatalogueIconLibrary.h"

#include "Engine/Texture2D.h"
#include "KalmalaIconWidget.h"

bool FKalmalaCatalogueIconLibrary::GetTextureObjectPath(FName CanonicalId, FSoftObjectPath& OutObjectPath)
{
    OutObjectPath = FSoftObjectPath();

    EKalmalaIcon FallbackIcon = EKalmalaIcon::Unknown;
    int32 FallbackVariant = 0;
    if (!UKalmalaIconWidget::FindCatalogueIcon(CanonicalId, FallbackIcon, FallbackVariant))
        return false;

    const FString AssetName = CanonicalId.ToString();
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
