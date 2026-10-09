#include "KalmalaStatusIconLibrary.h"

#include "Engine/Texture2D.h"

namespace
{
    struct FStatusIconMapping
    {
        const TCHAR* EntryId;
        const TCHAR* IconId;
    };

    const FStatusIconMapping Mappings[] = {
        {TEXT("Wet"), TEXT("Wet")},
        {TEXT("SteadyMeal"), TEXT("SteadyMeal")},
        {TEXT("Heat"), TEXT("Heat")},
        {TEXT("Cold"), TEXT("Cold")},
        {TEXT("Mending"), TEXT("Mending")},
        {TEXT("Shield"), TEXT("HearthShield")},
        {TEXT("Vigor"), TEXT("BearsVigor")},
        {TEXT("Call"), TEXT("DeerCall")},
        {TEXT("Weather"), TEXT("Storm")},
    };
}

bool FKalmalaStatusIconLibrary::GetIconIdForEntry(FName EntryId, FName& OutIconId)
{
    OutIconId = NAME_None;
    for (const FStatusIconMapping& Mapping : Mappings)
    {
        if (EntryId == FName(Mapping.EntryId))
        {
            OutIconId = FName(Mapping.IconId);
            return true;
        }
    }
    return false;
}

bool FKalmalaStatusIconLibrary::GetTextureObjectPath(FName IconId, FSoftObjectPath& OutObjectPath)
{
    OutObjectPath = FSoftObjectPath();
    bool bSupported = false;
    for (const FStatusIconMapping& Mapping : Mappings)
    {
        if (IconId == FName(Mapping.IconId))
        {
            bSupported = true;
            break;
        }
    }
    if (!bSupported)
    {
        return false;
    }

    const FString AssetName = IconId.ToString();
    OutObjectPath = FSoftObjectPath(FString::Printf(
        TEXT("/Game/Kalmala/UI/Icons/Status/%s.%s"), *AssetName, *AssetName));
    return OutObjectPath.IsValid();
}

UTexture2D* FKalmalaStatusIconLibrary::LoadTexture(FName IconId)
{
    FSoftObjectPath ObjectPath;
    if (!GetTextureObjectPath(IconId, ObjectPath))
    {
        return nullptr;
    }

    TSoftObjectPtr<UTexture2D> Texture(ObjectPath);
    return Texture.LoadSynchronous();
}
