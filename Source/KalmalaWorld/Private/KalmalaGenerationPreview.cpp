#include "KalmalaGenerationPreview.h"
#include "CoreGlobals.h"
#include "KalmalaRegionalGeneration.h"

namespace
{
    thread_local FKalmalaGenerationPreviewSettings Settings;
    thread_local uint64 SettingsSerial = 0;
}
const FKalmalaGenerationPreviewSettings& FKalmalaGenerationPreview::Get() { return Settings; }
uint64 FKalmalaGenerationPreview::Serial() { return SettingsSerial; }
bool FKalmalaGenerationPreview::Set(const FKalmalaGenerationPreviewSettings& Value)
{
#if WITH_EDITOR
    if (IsRunningCommandlet())
    {
        FKalmalaRegionalGeneration::ClearHydrologyCache();
        Settings = Value; ++SettingsSerial; return true;
    }
#endif
    return false;
}
void FKalmalaGenerationPreview::Reset()
{
    FKalmalaRegionalGeneration::ClearHydrologyCache();
    Settings = {}; ++SettingsSerial;
}
