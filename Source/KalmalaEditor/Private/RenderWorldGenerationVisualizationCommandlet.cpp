#include "RenderWorldGenerationVisualizationCommandlet.h"

#include "HAL/FileManager.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaWorldFieldSampler.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace KalmalaWorldGenerationVisualization
{
    constexpr int32 DefaultImageSize = 256;
    constexpr float DefaultWorldExtent = 20000.0f;

    struct FVisualizationPixel
    {
        uint8 R;
        uint8 G;
        uint8 B;
    };

    FVisualizationPixel GetBiomeColor(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::Ocean: return { 23, 88, 160 };
        case EKalmalaBiome::ShimmeringLakes: return { 61, 177, 190 };
        case EKalmalaBiome::Elderwood: return { 30, 100, 47 };
        case EKalmalaBiome::MossyMire: return { 76, 113, 55 };
        case EKalmalaBiome::FreezingTundra: return { 213, 236, 238 };
        case EKalmalaBiome::ThunderMountains: return { 104, 98, 112 };
        case EKalmalaBiome::Meadows: return { 131, 174, 76 };
        default: return { 255, 0, 255 };
        }
    }

    bool WritePpm(const FString& Filename, const int32 ImageSize, const TArray<FVisualizationPixel>& Pixels)
    {
        TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Filename));
        if (!Writer)
        {
            return false;
        }

        const FTCHARToUTF8 Header(*FString::Printf(TEXT("P6\n%d %d\n255\n"), ImageSize, ImageSize));
        Writer->Serialize(const_cast<ANSICHAR*>(Header.Get()), Header.Length());
        Writer->Serialize(const_cast<FVisualizationPixel*>(Pixels.GetData()), Pixels.Num() * sizeof(FVisualizationPixel));
        return !Writer->IsError();
    }
}

URenderWorldGenerationVisualizationCommandlet::URenderWorldGenerationVisualizationCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URenderWorldGenerationVisualizationCommandlet::Main(const FString& Params)
{
    using namespace KalmalaWorldGenerationVisualization;

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 10323456789ull;
    Config.GeneratorRevision = FKalmalaWorldGenerationConfig::CurrentGeneratorRevision;
    int32 ImageSize = DefaultImageSize;
    float WorldExtent = DefaultWorldExtent;

    FParse::Value(*Params, TEXT("Seed="), Config.WorldSeed);
    FParse::Value(*Params, TEXT("Revision="), Config.GeneratorRevision);
    FParse::Value(*Params, TEXT("Size="), ImageSize);
    FParse::Value(*Params, TEXT("Extent="), WorldExtent);

    if (!Config.IsValid() || ImageSize < 16 || ImageSize > 2048 || WorldExtent <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid visualization parameters. Revision must be positive, Size must be 16-2048, and Extent must be positive."));
        return 1;
    }

    FString OutputDirectory = FPaths::ProjectContentDir() / TEXT("Kalmala/Developer/WorldGeneration");
    FParse::Value(*Params, TEXT("Output="), OutputDirectory);
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);

    TArray<FVisualizationPixel> ElevationPixels;
    TArray<FVisualizationPixel> HumidityPixels;
    TArray<FVisualizationPixel> TemperaturePixels;
    TArray<FVisualizationPixel> FloraPixels;
    TArray<FVisualizationPixel> BiomePixels;
    TArray<FVisualizationPixel> WeightPixels[7], OverlapPixels, HydrologyPixels, HeightPixels, BoundaryPixels;
    TArray<uint8> BiomeIds;
    ElevationPixels.Reserve(ImageSize * ImageSize);
    HumidityPixels.Reserve(ImageSize * ImageSize);
    TemperaturePixels.Reserve(ImageSize * ImageSize);
    FloraPixels.Reserve(ImageSize * ImageSize);
    BiomePixels.Reserve(ImageSize * ImageSize);

    const auto AddFieldPixel = [](TArray<FVisualizationPixel>& Pixels, const float Value)
    {
        const uint8 Shade = static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 255.0f));
        Pixels.Add({ Shade, Shade, Shade });
    };

    for (int32 Y = 0; Y < ImageSize; ++Y)
    {
        for (int32 X = 0; X < ImageSize; ++X)
        {
            const FVector2D NormalizedPosition(
                static_cast<float>(X) / static_cast<float>(ImageSize - 1),
                static_cast<float>(Y) / static_cast<float>(ImageSize - 1));
            const FVector2D WorldPosition = (NormalizedPosition - FVector2D(0.5f, 0.5f)) * WorldExtent;
            const FKalmalaWorldFieldSample Sample = FKalmalaWorldFieldSampler::Sample(Config, WorldPosition);

            AddFieldPixel(ElevationPixels, Sample.Elevation);
            AddFieldPixel(HumidityPixels, Sample.Humidity);
            AddFieldPixel(TemperaturePixels, Sample.Temperature);
            AddFieldPixel(FloraPixels, Sample.Flora);
            BiomePixels.Add(GetBiomeColor(FKalmalaBiomeClassifier::Classify(Sample)));
            if (Config.GeneratorRevision >= 3)
            {
                const auto Region = FKalmalaRegionalGeneration::Sample(Sample);
                for (int32 I = 0; I < 7; ++I) AddFieldPixel(WeightPixels[I], Region.Weights[I]);
                float Strongest = 0;
                for (float W : Region.Weights) Strongest = FMath::Max(Strongest, W);
                AddFieldPixel(OverlapPixels, 1 - Strongest);
                AddFieldPixel(HeightPixels, (Region.Height + 500) / 2500.0f);
                HydrologyPixels.Add({ uint8(Region.RiverWeight * 255), uint8(Region.StreamWeight * 255), uint8(Region.BasinWeight * 255) });
                BiomeIds.Add(Region.Biome);
                const int32 Index = BiomeIds.Num() - 1;
                const bool Edge = (X > 0 && BiomeIds[Index - 1] != Region.Biome)
                    || (Y > 0 && BiomeIds[Index - ImageSize] != Region.Biome);
                BoundaryPixels.Add(Edge ? FVisualizationPixel{255, 40, 40} : BiomePixels.Last());
            }
        }
    }

    bool bWroteAllImages =
        WritePpm(OutputDirectory / TEXT("Elevation.ppm"), ImageSize, ElevationPixels) &&
        WritePpm(OutputDirectory / TEXT("Humidity.ppm"), ImageSize, HumidityPixels) &&
        WritePpm(OutputDirectory / TEXT("Temperature.ppm"), ImageSize, TemperaturePixels) &&
        WritePpm(OutputDirectory / TEXT("Flora.ppm"), ImageSize, FloraPixels) &&
        WritePpm(OutputDirectory / TEXT("BiomeClassification.ppm"), ImageSize, BiomePixels);

    if (Config.GeneratorRevision >= 3)
    {
        // Overlay the actual indexed splines, so subpixel-width streams remain legible.
        const int32 First = int32(FMath::FloorToInt(-WorldExtent * 0.5 / FKalmalaRegionalTuning::GridCell));
        const int32 Last = int32(FMath::FloorToInt(WorldExtent * 0.5 / FKalmalaRegionalTuning::GridCell));
        for (int32 Y = First; Y <= Last; ++Y) for (int32 X = First; X <= Last; ++X)
        for (const auto& Segment : FKalmalaRegionalGeneration::GetHydrology(Config, FIntPoint(X, Y)))
        {
            const FVector2D A = (FVector2D(Segment.A) / WorldExtent + FVector2D(0.5)) * (ImageSize - 1);
            const FVector2D B = (FVector2D(Segment.B) / WorldExtent + FVector2D(0.5)) * (ImageSize - 1);
            const int32 Steps = FMath::Max(1, int32(FMath::CeilToInt(FVector2D::Distance(A, B))));
            for (int32 I = 0; I <= Steps; ++I)
            {
                const auto P = FMath::Lerp(A, B, double(I) / Steps);
                const int32 PX = int32(FMath::RoundToInt(P.X)), PY = int32(FMath::RoundToInt(P.Y));
                if (PX >= 0 && PX < ImageSize && PY >= 0 && PY < ImageSize)
                {
                    auto& Pixel = HydrologyPixels[PY * ImageSize + PX];
                    if (Segment.bStream) Pixel.G = 255; else Pixel.R = 255;
                }
            }
        }
        for (int32 I = 0; I < 7; ++I) bWroteAllImages &= WritePpm(OutputDirectory / FString::Printf(TEXT("Weight%d.ppm"), I), ImageSize, WeightPixels[I]);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Overlap.ppm"), ImageSize, OverlapPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Hydrology.ppm"), ImageSize, HydrologyPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("ShapedHeight.ppm"), ImageSize, HeightPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Boundaries.ppm"), ImageSize, BoundaryPixels);
        const FString Tuning = FString::Printf(TEXT("Seed=%llu Revision=%d\nBiomeScale=%.0f RegionFrequency=%.9f ElevationFrequency=%.9f ClimateFrequency=%.9f\nWarpStrength=%.0f WarpFrequency=%.9f EdgeWaveStrength=%.3f RingOverlap=%.3f\nRiverSpacing=%.0f MergeDistance=%.0f RiverRange=%.0f SplineAmplitude=%.0f SplineWavelength=%.0f SplineStep=%.0f GridCell=%.0f\n"),
            Config.WorldSeed, Config.GeneratorRevision, FKalmalaRegionalTuning::BiomeScale, FKalmalaRegionalTuning::RegionFrequency,
            FKalmalaRegionalTuning::ElevationFrequency, FKalmalaRegionalTuning::ClimateFrequency,
            FKalmalaRegionalTuning::WarpStrength, FKalmalaRegionalTuning::WarpFrequency, FKalmalaRegionalTuning::EdgeWaveStrength, FKalmalaRegionalTuning::RingOverlap,
            FKalmalaRegionalTuning::RiverSpacing, FKalmalaRegionalTuning::MergeDistance, FKalmalaRegionalTuning::RiverRange, FKalmalaRegionalTuning::SplineAmplitude,
            FKalmalaRegionalTuning::SplineWavelength, FKalmalaRegionalTuning::SplineStep, FKalmalaRegionalTuning::GridCell);
        bWroteAllImages &= FFileHelper::SaveStringToFile(Tuning, *(OutputDirectory / TEXT("Tuning.txt")));
    }

    if (!bWroteAllImages)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not write all world-generation visualization images to %s."), *OutputDirectory);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Rendered world-generation previews for seed %llu revision %d to %s."), Config.WorldSeed, Config.GeneratorRevision, *OutputDirectory);
    return 0;
}
