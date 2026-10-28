#pragma once

#include "Commandlets/Commandlet.h"
#include "RenderCreaturesCommandlet.generated.h"

/** Offscreen side-view previews of the live wildlife archetypes. */
UCLASS()
class KALMALAEDITOR_API URenderCreaturesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    URenderCreaturesCommandlet();
    virtual int32 Main(const FString& Params) override;
};
