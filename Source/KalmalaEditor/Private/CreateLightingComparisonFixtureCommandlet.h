#pragma once

#include "Commandlets/Commandlet.h"
#include "CreateLightingComparisonFixtureCommandlet.generated.h"

UCLASS()
class UCreateLightingComparisonFixtureCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UCreateLightingComparisonFixtureCommandlet();
    virtual int32 Main(const FString& Params) override;
};
