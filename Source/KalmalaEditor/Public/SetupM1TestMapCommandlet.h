#pragma once

#include "Commandlets/Commandlet.h"
#include "SetupM1TestMapCommandlet.generated.h"

/** Places the reproducible two-player interaction fixtures into the M1 prototype map. */
UCLASS()
class KALMALAEDITOR_API USetupM1TestMapCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    USetupM1TestMapCommandlet();
    virtual int32 Main(const FString& Params) override;
};
