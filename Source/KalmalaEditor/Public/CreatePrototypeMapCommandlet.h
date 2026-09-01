#pragma once

#include "Commandlets/Commandlet.h"
#include "CreatePrototypeMapCommandlet.generated.h"

/** Creates the reproducible empty map used as the M0 prototype-map baseline. */
UCLASS()
class KALMALAEDITOR_API UCreatePrototypeMapCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCreatePrototypeMapCommandlet();

    virtual int32 Main(const FString& Params) override;
};
