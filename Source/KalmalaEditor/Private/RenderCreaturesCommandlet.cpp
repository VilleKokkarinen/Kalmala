#include "RenderCreaturesCommandlet.h"

#include "KalmalaWildlifeSpawn.h"
#include "Components/LightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/ISlateRHIRendererModule.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ProceduralMeshComponent.h"
#include "RenderingThread.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"

namespace
{
    struct FCreatureRenderDefinition
    {
        const TCHAR* Name;
        EKalmalaWildlifeArchetype Archetype;
    };

    const FCreatureRenderDefinition CreatureDefinitions[] = {
        {TEXT("Mireling"), EKalmalaWildlifeArchetype::Mireling},
        {TEXT("Boar"), EKalmalaWildlifeArchetype::Boar},
        {TEXT("Deer"), EKalmalaWildlifeArchetype::Deer},
        {TEXT("Rat"), EKalmalaWildlifeArchetype::Rat},
        {TEXT("Badger"), EKalmalaWildlifeArchetype::Badger},
        {TEXT("MoonBear"), EKalmalaWildlifeArchetype::MoonBear},
        {TEXT("DeathAdder"), EKalmalaWildlifeArchetype::DeathAdder},
        {TEXT("Viper"), EKalmalaWildlifeArchetype::Viper},
        {TEXT("GiantSnake"), EKalmalaWildlifeArchetype::GiantSnake},
        {TEXT("MosquitoSwarm"), EKalmalaWildlifeArchetype::MosquitoSwarm},
        {TEXT("PeatHusk"), EKalmalaWildlifeArchetype::PeatHusk},
        {TEXT("Wolf"), EKalmalaWildlifeArchetype::Wolf},
        {TEXT("Lynx"), EKalmalaWildlifeArchetype::Lynx},
        {TEXT("PolarBear"), EKalmalaWildlifeArchetype::PolarBear},
        {TEXT("HornedGoat"), EKalmalaWildlifeArchetype::HornedGoat},
        {TEXT("ThunderSentinel"), EKalmalaWildlifeArchetype::ThunderSentinel},
        {TEXT("ReefMaw"), EKalmalaWildlifeArchetype::ReefMaw}
    };

    const FCreatureRenderDefinition* FindCreatureDefinition(const FString& Name)
    {
        for (const FCreatureRenderDefinition& Definition : CreatureDefinitions)
        {
            if (Name.Equals(Definition.Name, ESearchCase::IgnoreCase)) return &Definition;
        }
        return nullptr;
    }
}

URenderCreaturesCommandlet::URenderCreaturesCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 URenderCreaturesCommandlet::Main(const FString& Params)
{
    FString CreatureName, Output, Creatures, Outputs;
    int32 Width = 1280;
    int32 Height = 960;
    FParse::Value(*Params, TEXT("Creature="), CreatureName);
    FParse::Value(*Params, TEXT("Output="), Output);
    const bool bHasCreatures = FParse::Value(*Params, TEXT("Creatures="), Creatures);
    const bool bHasOutputs = FParse::Value(*Params, TEXT("Outputs="), Outputs);
    FParse::Value(*Params, TEXT("Width="), Width);
    FParse::Value(*Params, TEXT("Height="), Height);

    if (bHasCreatures || bHasOutputs)
    {
        TArray<FString> CreatureNames;
        TArray<FString> OutputNames;
        Creatures.ParseIntoArray(CreatureNames, TEXT("|"), true);
        Outputs.ParseIntoArray(OutputNames, TEXT("|"), true);
        if (!bHasCreatures || !bHasOutputs || CreatureNames.IsEmpty()
            || CreatureNames.Num() != OutputNames.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Creature batch rendering requires matching -Creatures and -Outputs lists separated by |."));
            return 1;
        }

        for (int32 Index = 0; Index < CreatureNames.Num(); ++Index)
        {
            const FString SingleParams = FString::Printf(
                TEXT("-Creature=\"%s\" -Output=\"%s\" -Width=%d -Height=%d"),
                *CreatureNames[Index], *OutputNames[Index], Width, Height);
            const int32 Result = Main(SingleParams);
            if (Result != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("CreatureRender batch stopped: Creature=%s Code=%d"),
                    *CreatureNames[Index], Result);
                return Result;
            }
        }
        return 0;
    }

    const FCreatureRenderDefinition* Definition = FindCreatureDefinition(CreatureName);
    if (Definition == nullptr || Width < 320 || Height < 240 || Width > 4096 || Height > 2160
        || Output.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Unknown creature or invalid render request. Use a wildlife archetype name, Width=320..4096, Height=240..2160, and Output=<new PNG path>."));
        return 1;
    }

    Output = FPaths::ConvertRelativePathToFull(Output);
    if (Output.Len() >= 260 || FPaths::GetExtension(Output).ToLower() != TEXT("png")
        || FPaths::FileExists(Output))
    {
        UE_LOG(LogTemp, Error, TEXT("Creature render output must be a new PNG path shorter than 260 characters: %s"), *Output);
        return 2;
    }

    static bool bSlateRendererInitialized = false;
    if (!bSlateRendererInitialized)
    {
        ISlateRHIRendererModule& SlateRHIRenderer =
            FModuleManager::LoadModuleChecked<ISlateRHIRendererModule>(TEXT("SlateRHIRenderer"));
        if (!FSlateApplication::IsInitialized())
        {
            FSlateApplication::InitializeAsStandaloneApplication(SlateRHIRenderer.CreateSlateRHIRenderer());
        }
        else
        {
            FSlateApplication::Get().InitializeRenderer(SlateRHIRenderer.CreateSlateRHIRenderer());
        }
        bSlateRendererInitialized = true;
    }

    const double RenderStartSeconds = FPlatformTime::Seconds();
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World == nullptr) return 3;
    if (GEngine == nullptr)
    {
        World->DestroyWorld(false);
        return 3;
    }
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    const auto DestroyPreviewWorld = [World]()
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    ADirectionalLight* KeyLight = World->SpawnActor<ADirectionalLight>();
    ADirectionalLight* FillLight = World->SpawnActor<ADirectionalLight>();
    if (KeyLight == nullptr || FillLight == nullptr)
    {
        DestroyPreviewWorld();
        return 3;
    }
    KeyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    KeyLight->SetActorRotation(FRotator(-35.0f, -42.0f, 0.0f));
    KeyLight->GetLightComponent()->SetIntensity(50000.0f);
    KeyLight->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.92f, 0.80f));
    FillLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    FillLight->SetActorRotation(FRotator(-10.0f, 145.0f, 0.0f));
    FillLight->GetLightComponent()->SetIntensity(14000.0f);
    FillLight->GetLightComponent()->SetLightColor(FLinearColor(0.58f, 0.72f, 1.0f));
    FillLight->GetLightComponent()->SetCastShadows(false);

    AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (Creature == nullptr)
    {
        DestroyPreviewWorld();
        return 3;
    }
    FKalmalaWorldPopulationSpawn Spawn;
    Spawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    Spawn.SpawnSeed = 5;
    Spawn.Location = FVector(1000.0f, 0.0f, 0.0f);
    Creature->InitializeServer(Spawn, Definition->Archetype, 0);
    Creature->SetActorLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);

    FVector BoundsOrigin = FVector::ZeroVector;
    FVector BoundsExtent = FVector(100.0f);
    Creature->GetActorBounds(false, BoundsOrigin, BoundsExtent);
    BoundsExtent.X = FMath::Max(BoundsExtent.X, 20.0f);
    BoundsExtent.Y = FMath::Max(BoundsExtent.Y, 20.0f);
    BoundsExtent.Z = FMath::Max(BoundsExtent.Z, 20.0f);

    UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(World);
    Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
    Target->ClearColor = FLinearColor(0.035f, 0.052f, 0.070f, 1.0f);
    Target->InitAutoFormat(Width, Height);
    Target->UpdateResourceImmediate(true);

    ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>();
    if (CaptureActor == nullptr)
    {
        DestroyPreviewWorld();
        return 3;
    }
    USceneCaptureComponent2D* Capture = CaptureActor->GetCaptureComponent2D();
    Capture->TextureTarget = Target;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->ProjectionType = ECameraProjectionMode::Orthographic;
    Capture->OrthoWidth = 2.5f * FMath::Max(BoundsExtent.X, BoundsExtent.Z * static_cast<float>(Width) / Height);
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->bAlwaysPersistRenderingState = true;
    const float CameraDistance = FMath::Max(BoundsExtent.Size() * 3.0f, 600.0f);
    const FVector CameraLocation = BoundsOrigin + FVector(BoundsExtent.X * 0.12f,
        -CameraDistance, BoundsExtent.Z * 0.15f);
    CaptureActor->SetActorLocation(CameraLocation);
    CaptureActor->SetActorRotation((BoundsOrigin - CameraLocation).Rotation());

    FlushRenderingCommands();
    Capture->CaptureScene();
    FlushRenderingCommands();
    TArray<FColor> Pixels;
    FReadSurfaceDataFlags ReadFlags;
    ReadFlags.SetLinearToGamma(false);
    if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)
        || Pixels.Num() != Width * Height)
    {
        DestroyPreviewWorld();
        return 5;
    }
    for (FColor& Pixel : Pixels) Pixel.A = 255;
    if (Pixels.IsEmpty() || !Pixels.ContainsByPredicate([&Pixels](const FColor& Pixel)
        { return Pixel != Pixels[0]; }))
    {
        UE_LOG(LogTemp, Error, TEXT("CreatureRender: %s produced a uniform image; capture rejected."), Definition->Name);
        DestroyPreviewWorld();
        return 7;
    }

    TArray64<uint8> PNG;
    FImageUtils::PNGCompressImageArray(Width, Height,
        TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output), true);
    const bool bSaved = FFileHelper::SaveArrayToFile(PNG, *Output, &IFileManager::Get(), FILEWRITE_NoReplaceExisting);
    UProceduralMeshComponent* Mesh = Creature->FindComponentByClass<UProceduralMeshComponent>();
    const FProcMeshSection* Section = Mesh != nullptr ? Mesh->GetProcMeshSection(0) : nullptr;
    const int32 VertexCount = Section != nullptr ? Section->ProcVertexBuffer.Num() : 0;
    DestroyPreviewWorld();
    if (VertexCount == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreatureRender: %s produced no procedural vertices."), Definition->Name);
    }
    UE_LOG(LogTemp, Display,
        TEXT("CreatureRender: Creature=%s Size=%dx%d Vertices=%d Saved=%d Seconds=%.2f Output=%s"),
        Definition->Name, Width, Height, VertexCount, bSaved,
        FPlatformTime::Seconds() - RenderStartSeconds, *Output);
    return bSaved ? 0 : 6;
}
