#include "CreatePrototypeMapCommandlet.h"

#include "Factories/WorldFactory.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

UCreatePrototypeMapCommandlet::UCreatePrototypeMapCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreatePrototypeMapCommandlet::Main(const FString& Params)
{
    const FString PackageName(TEXT("/Game/Kalmala/Maps/Prototype/L_Prototype"));
    FString Filename;

    if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetMapPackageExtension()))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not resolve map package %s."), *PackageName);
        return 1;
    }

    if (FPaths::FileExists(Filename))
    {
        UE_LOG(LogTemp, Display, TEXT("Prototype map already exists at %s."), *Filename);
        return 0;
    }

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    UPackage* Package = CreatePackage(*PackageName);
    UWorldFactory* Factory = NewObject<UWorldFactory>();
    UObject* CreatedObject = Factory->FactoryCreateNew(
        UWorld::StaticClass(),
        Package,
        FName(TEXT("L_Prototype")),
        RF_Public | RF_Standalone,
        nullptr,
        GWarn);
    UWorld* World = Cast<UWorld>(CreatedObject);

    if (World == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not create prototype world."));
        return 1;
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

    if (!UPackage::SavePackage(Package, World, *Filename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not save prototype map to %s."), *Filename);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Created prototype map at %s."), *Filename);
    return 0;
}
