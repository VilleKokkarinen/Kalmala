#pragma once

#include "Commandlets/Commandlet.h"
#include "CreateWorldMaterialsCommandlet.generated.h"

/** Creates the project-owned material baseline for generated terrain and water. */
UCLASS()
class KALMALAEDITOR_API UCreateWorldMaterialsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreateWorldMaterialsCommandlet();

    virtual int32 Main(const FString& Params) override;
};
