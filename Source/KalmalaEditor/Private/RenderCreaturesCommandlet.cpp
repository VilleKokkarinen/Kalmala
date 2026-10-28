#include "RenderCreaturesCommandlet.h"

#include "KalmalaWildlifeSpawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/UnrealMemory.h"

namespace
{
    struct FCreatureRenderDefinition
    {
        const TCHAR* Name;
        EKalmalaWildlifeArchetype Archetype;
    };

    struct FCreatureRenderView
    {
        const TCHAR* Name;
        float YawDegrees;
        bool bPreserveCurrentSideView;
        bool bLevelCamera;
    };

    constexpr int32 CreatureViewGap = 4;
    const FCreatureRenderView CreatureRenderViews[] = {
        {TEXT("Side"), 0.0f, true, false},
        {TEXT("Front"), 90.0f, false, true},
        {TEXT("Back"), -90.0f, false, true},
        {TEXT("ThreeQuarter"), 45.0f, false, false}
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

    bool RasterizeCreatureMesh(UProceduralMeshComponent& Mesh, const int32 Width, const int32 Height,
        const FCreatureRenderView& View, TArray<FColor>& OutPixels, int32& OutVertexCount)
    {
        struct FProjectedVertex
        {
            FVector2D Screen;
            float Depth = 0.0f;
            FVector Normal = FVector::UpVector;
            FLinearColor Color = FLinearColor::White;
        };

        OutVertexCount = 0;
        FBox Bounds(ForceInit);
        const FTransform MeshToWorld = Mesh.GetComponentTransform();
        for (int32 SectionIndex = 0; SectionIndex < Mesh.GetNumSections(); ++SectionIndex)
        {
            const FProcMeshSection* Section = Mesh.GetProcMeshSection(SectionIndex);
            if (Section == nullptr || !Section->bSectionVisible) continue;
            OutVertexCount += Section->ProcVertexBuffer.Num();
            for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
            {
                Bounds += MeshToWorld.TransformPosition(Vertex.Position);
            }
        }
        if (OutVertexCount == 0 || !Bounds.IsValid) return false;

        const FVector BoundsOrigin = Bounds.GetCenter();
        const FVector BoundsExtent = Bounds.GetExtent();
        const float CameraDistance = FMath::Max(BoundsExtent.Size() * 3.0f, 600.0f);
        FVector CameraLocation;
        if (View.bPreserveCurrentSideView)
        {
            CameraLocation = BoundsOrigin + FVector(BoundsExtent.X * 0.12f,
                -CameraDistance, BoundsExtent.Z * 0.15f);
        }
        else
        {
            const float YawRadians = FMath::DegreesToRadians(View.YawDegrees);
            const FVector HorizontalDirection(FMath::Sin(YawRadians), -FMath::Cos(YawRadians), 0.0f);
            const float CameraHeight = View.bLevelCamera ? 0.0f : BoundsExtent.Z * 0.15f;
            CameraLocation = BoundsOrigin + HorizontalDirection * CameraDistance + FVector(0.0f, 0.0f, CameraHeight);
        }
        const FVector Forward = (BoundsOrigin - CameraLocation).GetSafeNormal();
        const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
        const FVector Up = FVector::CrossProduct(Right, Forward).GetSafeNormal();
        const float Aspect = static_cast<float>(Width) / static_cast<float>(Height);
        const float ProjectedHalfWidth = FMath::Abs(Right.X) * BoundsExtent.X
            + FMath::Abs(Right.Y) * BoundsExtent.Y + FMath::Abs(Right.Z) * BoundsExtent.Z;
        const float ProjectedHalfHeight = FMath::Abs(Up.X) * BoundsExtent.X
            + FMath::Abs(Up.Y) * BoundsExtent.Y + FMath::Abs(Up.Z) * BoundsExtent.Z;
        const float SideHalfWidth = FMath::Max(BoundsExtent.X, BoundsExtent.Z * Aspect);
        const float OrthoWidth = 2.5f * (View.bPreserveCurrentSideView
            ? SideHalfWidth : FMath::Max(ProjectedHalfWidth, ProjectedHalfHeight * Aspect));
        const float OrthoHeight = OrthoWidth / Aspect;

        TArray<TArray<FProjectedVertex>> ProjectedSections;
        ProjectedSections.SetNum(Mesh.GetNumSections());
        for (int32 SectionIndex = 0; SectionIndex < Mesh.GetNumSections(); ++SectionIndex)
        {
            const FProcMeshSection* Section = Mesh.GetProcMeshSection(SectionIndex);
            if (Section == nullptr || !Section->bSectionVisible) continue;
            TArray<FProjectedVertex>& Projected = ProjectedSections[SectionIndex];
            Projected.Reserve(Section->ProcVertexBuffer.Num());
            for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
            {
                const FVector WorldPosition = MeshToWorld.TransformPosition(Vertex.Position);
                const FVector FromCentre = WorldPosition - BoundsOrigin;
                FProjectedVertex& Item = Projected.AddDefaulted_GetRef();
                Item.Screen = FVector2D(Width * 0.5f + FVector::DotProduct(FromCentre, Right) * Width / OrthoWidth,
                    Height * 0.5f - FVector::DotProduct(FromCentre, Up) * Height / OrthoHeight);
                Item.Depth = FVector::DotProduct(WorldPosition - CameraLocation, Forward);
                Item.Normal = MeshToWorld.TransformVectorNoScale(Vertex.Normal).GetSafeNormal();
                Item.Color = FLinearColor(Vertex.Color);
            }
        }

        const int32 PixelCount = Width * Height;
        OutPixels.SetNumUninitialized(PixelCount);
        TArray<float> DepthBuffer;
        DepthBuffer.Init(TNumericLimits<float>::Max(), PixelCount);
        const FLinearColor Horizon(0.16f, 0.21f, 0.24f, 1.0f);
        const FLinearColor Ground(0.045f, 0.065f, 0.075f, 1.0f);
        for (int32 Y = 0; Y < Height; ++Y)
        {
            const FColor Background = FMath::Lerp(Horizon, Ground,
                static_cast<float>(Y) / FMath::Max(Height - 1, 1)).ToFColor(true);
            for (int32 X = 0; X < Width; ++X) OutPixels[Y * Width + X] = Background;
        }

        const FVector LightDirection = FVector(-0.36f, -0.48f, 0.80f).GetSafeNormal();
        bool bRasterizedTriangle = false;
        const auto Edge = [](const FVector2D& A, const FVector2D& B, const FVector2D& Point)
        {
            return (Point.X - A.X) * (B.Y - A.Y) - (Point.Y - A.Y) * (B.X - A.X);
        };
        for (int32 SectionIndex = 0; SectionIndex < ProjectedSections.Num(); ++SectionIndex)
        {
            const TArray<FProjectedVertex>& Projected = ProjectedSections[SectionIndex];
            const FProcMeshSection* Section = Mesh.GetProcMeshSection(SectionIndex);
            if (Section == nullptr || !Section->bSectionVisible) continue;
            const TArray<uint32>& Indices = Section->ProcIndexBuffer;
            for (int32 Index = 0; Index + 2 < Indices.Num(); Index += 3)
            {
                const uint32 I0 = Indices[Index];
                const uint32 I1 = Indices[Index + 1];
                const uint32 I2 = Indices[Index + 2];
                if (!Projected.IsValidIndex(static_cast<int32>(I0))
                    || !Projected.IsValidIndex(static_cast<int32>(I1))
                    || !Projected.IsValidIndex(static_cast<int32>(I2))) continue;

                const FProjectedVertex& V0 = Projected[I0];
                const FProjectedVertex& V1 = Projected[I1];
                const FProjectedVertex& V2 = Projected[I2];
                const float Area = Edge(V0.Screen, V1.Screen, V2.Screen);
                if (FMath::Abs(Area) < KINDA_SMALL_NUMBER) continue;

                const float MinScreenX = FMath::Min3(V0.Screen.X, V1.Screen.X, V2.Screen.X);
                const float MaxScreenX = FMath::Max3(V0.Screen.X, V1.Screen.X, V2.Screen.X);
                const float MinScreenY = FMath::Min3(V0.Screen.Y, V1.Screen.Y, V2.Screen.Y);
                const float MaxScreenY = FMath::Max3(V0.Screen.Y, V1.Screen.Y, V2.Screen.Y);
                if (MaxScreenX < 0.0f || MinScreenX >= Width || MaxScreenY < 0.0f || MinScreenY >= Height) continue;
                const int32 MinX = FMath::Clamp(FMath::FloorToInt(MinScreenX), 0, Width - 1);
                const int32 MaxX = FMath::Clamp(FMath::CeilToInt(MaxScreenX), 0, Width - 1);
                const int32 MinY = FMath::Clamp(FMath::FloorToInt(MinScreenY), 0, Height - 1);
                const int32 MaxY = FMath::Clamp(FMath::CeilToInt(MaxScreenY), 0, Height - 1);
                for (int32 Y = MinY; Y <= MaxY; ++Y)
                {
                    for (int32 X = MinX; X <= MaxX; ++X)
                    {
                        const FVector2D Sample(X + 0.5f, Y + 0.5f);
                        const float W0 = Edge(V1.Screen, V2.Screen, Sample) / Area;
                        const float W1 = Edge(V2.Screen, V0.Screen, Sample) / Area;
                        const float W2 = 1.0f - W0 - W1;
                        if (W0 < -0.0001f || W1 < -0.0001f || W2 < -0.0001f) continue;

                        const float Depth = W0 * V0.Depth + W1 * V1.Depth + W2 * V2.Depth;
                        const int32 PixelIndex = Y * Width + X;
                        if (Depth >= DepthBuffer[PixelIndex]) continue;
                        DepthBuffer[PixelIndex] = Depth;

                        const FVector Normal = (V0.Normal * W0 + V1.Normal * W1 + V2.Normal * W2).GetSafeNormal();
                        const float Shade = 0.38f + 0.62f * FMath::Max(0.0f, FVector::DotProduct(Normal, LightDirection));
                        const FLinearColor Color = (V0.Color * W0 + V1.Color * W1 + V2.Color * W2) * Shade;
                        OutPixels[PixelIndex] = Color.ToFColor(true);
                        bRasterizedTriangle = true;
                    }
                }
            }
        }
        return bRasterizedTriangle;
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

    UProceduralMeshComponent* Mesh = Creature->FindComponentByClass<UProceduralMeshComponent>();
    int32 VertexCount = 0;
    if (Mesh == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatureRender: %s produced no rasterizable triangles (Vertices=%d); preview rejected."),
            Definition->Name, VertexCount);
        DestroyPreviewWorld();
        return 7;
    }

    const int32 SheetWidth = Width * 2 + CreatureViewGap;
    const int32 SheetHeight = Height * 2 + CreatureViewGap;
    constexpr int32 CreatureRenderViewCount = static_cast<int32>(UE_ARRAY_COUNT(CreatureRenderViews));
    TArray<FColor> Pixels;
    Pixels.Init(FColor(20, 28, 34, 255), SheetWidth * SheetHeight);
    for (int32 ViewIndex = 0; ViewIndex < CreatureRenderViewCount; ++ViewIndex)
    {
        const FCreatureRenderView& View = CreatureRenderViews[ViewIndex];
        TArray<FColor> ViewPixels;
        if (!RasterizeCreatureMesh(*Mesh, Width, Height, View, ViewPixels, VertexCount))
        {
            UE_LOG(LogTemp, Error,
                TEXT("CreatureRender: %s view produced no rasterizable triangles (View=%s Vertices=%d); preview rejected."),
                Definition->Name, View.Name, VertexCount);
            DestroyPreviewWorld();
            return 7;
        }

        const int32 DestinationX = (ViewIndex % 2) * (Width + CreatureViewGap);
        const int32 DestinationY = (ViewIndex / 2) * (Height + CreatureViewGap);
        for (int32 Y = 0; Y < Height; ++Y)
        {
            FMemory::Memcpy(Pixels.GetData() + (DestinationY + Y) * SheetWidth + DestinationX,
                ViewPixels.GetData() + Y * Width, sizeof(FColor) * Width);
        }
    }

    TArray64<uint8> PNG;
    FImageUtils::PNGCompressImageArray(SheetWidth, SheetHeight,
        TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PNG);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output), true);
    const bool bSaved = FFileHelper::SaveArrayToFile(PNG, *Output, &IFileManager::Get(), FILEWRITE_NoReplaceExisting);
    DestroyPreviewWorld();
    UE_LOG(LogTemp, Display,
        TEXT("CreatureRender: Creature=%s Size=%dx%d Vertices=%d Saved=%d Seconds=%.2f Output=%s ViewSize=%dx%d Views=Side,Front,Back,ThreeQuarter"),
        Definition->Name, SheetWidth, SheetHeight, VertexCount, bSaved,
        FPlatformTime::Seconds() - RenderStartSeconds, *Output, Width, Height);
    return bSaved ? 0 : 6;
}
