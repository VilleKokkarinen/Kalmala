#include "KalmalaOceanTravelPersistenceContract.h"
#include "Kismet/GameplayStatics.h"

bool FKalmalaOceanTravelVesselState::IsValidVesselId(const FString& InVesselId)
{
    return InVesselId.Len() <= MaxVesselIdLength
        && InVesselId.StartsWith(TEXT("ocean-skiff:"), ESearchCase::CaseSensitive)
        && FKalmalaM7SparseDelta::IsValidStableId(InVesselId);
}

bool FKalmalaOceanTravelVesselState::IsValid() const
{
    if (!IsValidVesselId(VesselId)
        || !FMath::IsFinite(SafeLocation.X)
        || !FMath::IsFinite(SafeLocation.Y)
        || !FMath::IsFinite(SafeLocation.Z)
        || !FMath::IsFinite(YawDegrees)
        || FMath::Abs(YawDegrees) > 180.0f
        || FMath::Abs(SafeLocation.Z) > MaxVerticalOffsetCm)
    {
        return false;
    }

    const double RadiusSquared = FMath::Square(SafeLocation.X) + FMath::Square(SafeLocation.Y);
    return RadiusSquared <= FMath::Square(MaxWorldRadiusCm);
}

bool UKalmalaOceanTravelPersistenceSaveGame::HasSerializedSizeBudget()
{
    TArray<uint8> SerializedBytes;
    return UGameplayStatics::SaveGameToMemory(this, SerializedBytes)
        && SerializedBytes.Num() <= MaxSerializedRecordBytes;
}

bool FKalmalaOceanTravelPassengerState::IsValid() const
{
    return FKalmalaOceanTravelVesselState::IsValidVesselId(VesselId)
        && (Seat == EKalmalaOceanTravelSavedSeat::Helm
            || Seat == EKalmalaOceanTravelSavedSeat::Passenger);
}

void UKalmalaOceanTravelPersistenceSaveGame::Initialize(const FKalmalaM7SaveIdentity& InIdentity)
{
    SchemaVersion = CurrentSchemaVersion;
    Identity = InIdentity;
    bHasVesselState = false;
    VesselState = FKalmalaOceanTravelVesselState();
    bHasPassengerState = false;
    PassengerState = FKalmalaOceanTravelPassengerState();
}

bool UKalmalaOceanTravelPersistenceSaveGame::Matches(const FKalmalaM7SaveIdentity& InIdentity) const
{
    if (SchemaVersion != CurrentSchemaVersion || !Identity.Matches(InIdentity))
    {
        return false;
    }

    if (Identity.Scope == EKalmalaM7SaveScope::World)
    {
        return bHasVesselState
            && !bHasPassengerState
            && VesselState.IsValid();
    }

    if (Identity.Scope == EKalmalaM7SaveScope::Player)
    {
        return !bHasVesselState
            && bHasPassengerState
            && PassengerState.IsValid();
    }

    return false;
}

EKalmalaM7SchemaDecision UKalmalaOceanTravelPersistenceSaveGame::EvaluateSchemaVersion(
    const int32 CandidateSchemaVersion)
{
    if (CandidateSchemaVersion == CurrentSchemaVersion)
    {
        return EKalmalaM7SchemaDecision::AcceptCurrent;
    }

    if (CandidateSchemaVersion == 0)
    {
        return EKalmalaM7SchemaDecision::MigrateBeforeLoad;
    }

    return EKalmalaM7SchemaDecision::Reject;
}

bool UKalmalaOceanTravelPersistenceSaveGame::SetVesselState(const FKalmalaOceanTravelVesselState& State)
{
    if (SchemaVersion != CurrentSchemaVersion
        || !Identity.IsValid()
        || Identity.Scope != EKalmalaM7SaveScope::World
        || !State.IsValid()
        || bHasPassengerState
        || (bHasVesselState && VesselState.VesselId != State.VesselId))
    {
        return false;
    }

    const FKalmalaOceanTravelVesselState PreviousState = VesselState;
    const bool bHadPreviousState = bHasVesselState;
    VesselState = State;
    bHasVesselState = true;
    if (!HasSerializedSizeBudget())
    {
        VesselState = PreviousState;
        bHasVesselState = bHadPreviousState;
        return false;
    }
    return true;
}

bool UKalmalaOceanTravelPersistenceSaveGame::SetPassengerState(
    const FKalmalaOceanTravelPassengerState& State)
{
    if (SchemaVersion != CurrentSchemaVersion
        || !Identity.IsValid()
        || Identity.Scope != EKalmalaM7SaveScope::Player
        || !State.IsValid()
        || bHasVesselState
        || (bHasPassengerState && PassengerState.VesselId != State.VesselId))
    {
        return false;
    }

    const FKalmalaOceanTravelPassengerState PreviousState = PassengerState;
    const bool bHadPreviousState = bHasPassengerState;
    PassengerState = State;
    bHasPassengerState = true;
    if (!HasSerializedSizeBudget())
    {
        PassengerState = PreviousState;
        bHasPassengerState = bHadPreviousState;
        return false;
    }
    return true;
}

bool UKalmalaOceanTravelPersistenceSaveGame::CanPairWithWorldSave(
    const UKalmalaOceanTravelPersistenceSaveGame& WorldSave) const
{
    return Identity.Scope == EKalmalaM7SaveScope::Player
        && WorldSave.Identity.Scope == EKalmalaM7SaveScope::World
        && Matches(Identity)
        && WorldSave.Matches(WorldSave.Identity)
        && Identity.WorldSeed == WorldSave.Identity.WorldSeed
        && Identity.GeneratorRevision == WorldSave.Identity.GeneratorRevision
        && PassengerState.VesselId == WorldSave.VesselState.VesselId;
}
