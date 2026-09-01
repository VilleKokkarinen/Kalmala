#include "SetupM1TestMapCommandlet.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "KalmalaInteractionTestActor.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

USetupM1TestMapCommandlet::USetupM1TestMapCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 USetupM1TestMapCommandlet::Main(const FString& Params)
{
    const FString MapFilename(TEXT("E:/dev/Kalmala/Content/Kalmala/Maps/Prototype/L_Prototype.umap"));
    UPackage* Package = LoadPackage(nullptr, *MapFilename, LOAD_None);
    if (Package == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load the M1 prototype map."));
        return 1;
    }

    UWorld* World = UWorld::FindWorldInPackage(Package);
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("No editor world is available after loading the M1 prototype map."));
        return 1;
    }

    bool bChanged = false;
    const auto EnsureActor = [&World, &bChanged](const FName Tag, UClass* ActorClass, const FTransform& Transform)
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->Tags.Contains(Tag))
            {
                return;
            }
        }

        FActorSpawnParameters SpawnParams;
        AActor* SpawnedActor = World->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
        if (SpawnedActor != nullptr)
        {
            SpawnedActor->Tags.Add(Tag);
            bChanged = true;
        }
    };

    EnsureActor(TEXT("M1PlayerStartOne"), APlayerStart::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0.0f, -200.0f, 100.0f)));
    EnsureActor(TEXT("M1PlayerStartTwo"), APlayerStart::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(0.0f, 200.0f, 100.0f)));
    EnsureActor(TEXT("M1InteractionTarget"), AKalmalaInteractionTestActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(300.0f, 0.0f, 50.0f)));

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    if (bChanged && !UPackage::SavePackage(Package, World, *MapFilename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not save the M1 prototype map."));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("M1 prototype-map fixtures are ready."));
    return 0;
}
