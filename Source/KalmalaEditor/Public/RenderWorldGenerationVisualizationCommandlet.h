#pragma once

#include "Commandlets/Commandlet.h"
#include "RenderWorldGenerationVisualizationCommandlet.generated.h"

/** Renders developer-only PPM previews of the seed-derived fields and biome classification. */
UCLASS()
class KALMALAEDITOR_API URenderWorldGenerationVisualizationCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URenderWorldGenerationVisualizationCommandlet();
    virtual int32 Main(const FString& Params) override;
};
