#pragma once

#include "Commandlets/Commandlet.h"
#include "ExportWorldMapsCommandlet.generated.h"

/** Warm, headless PNG exporter using production biome sampling. */
UCLASS()
class KALMALAEDITOR_API UExportWorldMapsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UExportWorldMapsCommandlet();
    virtual int32 Main(const FString& Params) override;
};
