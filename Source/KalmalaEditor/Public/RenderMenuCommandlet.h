#pragma once
#include "Commandlets/Commandlet.h"
#include "RenderMenuCommandlet.generated.h"

/** Offscreen production-UMG menu preview; no map, play session or package. */
UCLASS()
class KALMALAEDITOR_API URenderMenuCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    URenderMenuCommandlet();
    virtual int32 Main(const FString& Params) override;
};
