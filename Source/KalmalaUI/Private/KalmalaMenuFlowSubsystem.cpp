#include "KalmalaMenuFlowSubsystem.h"

void UKalmalaMenuFlowSubsystem::Deinitialize()
{
    Flow.Shutdown();
    Super::Deinitialize();
}
