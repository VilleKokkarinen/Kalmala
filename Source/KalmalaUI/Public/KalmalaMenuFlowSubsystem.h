#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "KalmalaMenuFlow.h"
#include "KalmalaMenuFlowSubsystem.generated.h"

/** Owns an inactive flow until the future front end activates it. */
UCLASS()
class KALMALAUI_API UKalmalaMenuFlowSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    // Async callbacks must retain a weak pointer to this owner, never a flow reference.
    FKalmalaMenuFlow& GetFlow() { return Flow; }
    virtual void Deinitialize() override;
private:
    FKalmalaMenuFlow Flow;
};
