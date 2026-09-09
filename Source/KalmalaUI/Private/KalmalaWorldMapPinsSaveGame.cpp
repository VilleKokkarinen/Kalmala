#include "KalmalaWorldMapPinsSaveGame.h"

void UKalmalaWorldMapPinsSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    SchemaVersion = PinSchemaVersion;
    WorldConfig = InWorldConfig;
    Pins.Reset();
}

bool UKalmalaWorldMapPinsSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const
{
    return SchemaVersion == PinSchemaVersion && WorldConfig == InWorldConfig;
}

bool UKalmalaWorldMapPinsSaveGame::SetPins(const TArray<FKalmalaWorldMapPersonalPin>& InPins)
{
    TArray<FKalmalaWorldMapPersonalPin> ValidPins;
    ValidPins.Reserve(FMath::Min(InPins.Num(), MaxPersonalPins));
    const int32 FirstRetainedPin = FMath::Max(0, InPins.Num() - MaxPersonalPins);
    for (int32 Index = FirstRetainedPin; Index < InPins.Num(); ++Index)
    {
        if (!IsValidPin(InPins[Index])) return false;
        ValidPins.Add(InPins[Index]);
    }
    Pins = MoveTemp(ValidPins);
    return true;
}

bool UKalmalaWorldMapPinsSaveGame::IsValidPin(const FKalmalaWorldMapPersonalPin& Pin)
{
    const bool bKnownStyle = Pin.Style == EKalmalaWorldMapPinStyle::Cairn || Pin.Style == EKalmalaWorldMapPinStyle::Lantern
        || Pin.Style == EKalmalaWorldMapPinStyle::Thread;
    return FMath::IsFinite(Pin.WorldLocation.X) && FMath::IsFinite(Pin.WorldLocation.Y) && !Pin.Label.IsEmpty()
        && Pin.Label.Len() <= MaxPinLabelLength && bKnownStyle;
}
