#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldMapPinsSaveGame.generated.h"

/** Original local pin treatments; these are presentation labels, not gameplay claims. */
UENUM()
enum class EKalmalaWorldMapPinStyle : uint8
{
    Cairn,
    Lantern,
    Thread
};

/** A bounded personal annotation; it never names an actor, discovery, or server instruction. */
USTRUCT()
struct FKalmalaWorldMapPersonalPin
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FVector2D WorldLocation = FVector2D::ZeroVector;

    UPROPERTY(SaveGame)
    FString Label;

    UPROPERTY(SaveGame)
    EKalmalaWorldMapPinStyle Style = EKalmalaWorldMapPinStyle::Cairn;

    UPROPERTY(SaveGame)
    bool bComplete = false;

    UPROPERTY(SaveGame)
    bool bVisible = true;
};

/**
 * Versioned, local-only personal pins. This is intentionally separate from
 * generated-world deltas and does not participate in replication or RPCs.
 */
UCLASS()
class KALMALAUI_API UKalmalaWorldMapPinsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 PinSchemaVersion = 1;
    static constexpr int32 MaxPersonalPins = 256;
    static constexpr int32 MaxPinLabelLength = 32;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const;
    bool SetPins(const TArray<FKalmalaWorldMapPersonalPin>& InPins);
    const TArray<FKalmalaWorldMapPersonalPin>& GetPins() const { return Pins; }

private:
    static bool IsValidPin(const FKalmalaWorldMapPersonalPin& Pin);

    UPROPERTY(SaveGame)
    int32 SchemaVersion = PinSchemaVersion;

    UPROPERTY(SaveGame)
    FKalmalaWorldGenerationConfig WorldConfig;

    /** Insertion order is retained; overflowing local data keeps the newest pins. */
    UPROPERTY(SaveGame)
    TArray<FKalmalaWorldMapPersonalPin> Pins;
};
