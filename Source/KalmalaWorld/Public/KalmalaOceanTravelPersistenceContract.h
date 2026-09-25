#pragma once

#include "CoreMinimal.h"
#include "KalmalaM7PersistenceContract.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaOceanTravelPersistenceContract.generated.h"

UENUM()
enum class EKalmalaOceanTravelSavedSeat : uint8
{
    Helm,
    Passenger
};

/** The only world-scoped vessel data admitted by the first travel-save schema. */
USTRUCT()
struct KALMALAWORLD_API FKalmalaOceanTravelVesselState
{
    GENERATED_BODY()

    static constexpr int32 MaxVesselIdLength = 128;
    static constexpr double MaxWorldRadiusCm = 1600000.0;
    static constexpr double MaxVerticalOffsetCm = 100000.0;

    UPROPERTY(SaveGame)
    FString VesselId;

    /** Last server-accepted safe transform. Saves are captured only while moored. */
    UPROPERTY(SaveGame)
    FVector SafeLocation = FVector::ZeroVector;

    UPROPERTY(SaveGame)
    float YawDegrees = 0.0f;

    bool IsValid() const;
    static bool IsValidVesselId(const FString& InVesselId);
};

/** A player-scoped reference to one accepted seat on a stable vessel. */
USTRUCT()
struct KALMALAWORLD_API FKalmalaOceanTravelPassengerState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FString VesselId;

    UPROPERTY(SaveGame)
    EKalmalaOceanTravelSavedSeat Seat = EKalmalaOceanTravelSavedSeat::Helm;

    bool IsValid() const;
};

/**
 * Versioned M8 persistence gate. World scope stores at most one moored vessel
 * snapshot; player scope stores at most one authenticated seat association.
 * The first skiff has no cargo, velocity, steering, or saved occupancy state.
 */
UCLASS()
class KALMALAWORLD_API UKalmalaOceanTravelPersistenceSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr int32 MaxSerializedRecordBytes = 3072;

    void Initialize(const FKalmalaM7SaveIdentity& InIdentity);
    bool Matches(const FKalmalaM7SaveIdentity& InIdentity) const;
    static EKalmalaM7SchemaDecision EvaluateSchemaVersion(int32 CandidateSchemaVersion);

    bool SetVesselState(const FKalmalaOceanTravelVesselState& State);
    bool SetPassengerState(const FKalmalaOceanTravelPassengerState& State);
    bool HasVesselState() const { return bHasVesselState; }
    bool HasPassengerState() const { return bHasPassengerState; }
    const FKalmalaOceanTravelVesselState& GetVesselState() const { return VesselState; }
    const FKalmalaOceanTravelPassengerState& GetPassengerState() const { return PassengerState; }
    const FKalmalaM7SaveIdentity& GetIdentity() const { return Identity; }
    bool HasSerializedSizeBudget();

    /** A player seat association is usable only with its matching world vessel record. */
    bool CanPairWithWorldSave(const UKalmalaOceanTravelPersistenceSaveGame& WorldSave) const;

private:
    UPROPERTY(SaveGame)
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(SaveGame)
    FKalmalaM7SaveIdentity Identity;

    UPROPERTY(SaveGame)
    bool bHasVesselState = false;

    UPROPERTY(SaveGame)
    FKalmalaOceanTravelVesselState VesselState;

    UPROPERTY(SaveGame)
    bool bHasPassengerState = false;

    UPROPERTY(SaveGame)
    FKalmalaOceanTravelPassengerState PassengerState;
};
