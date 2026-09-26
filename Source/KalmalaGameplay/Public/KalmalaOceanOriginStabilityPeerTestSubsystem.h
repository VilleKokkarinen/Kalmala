#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KalmalaOceanOriginStabilityPeerTestSubsystem.generated.h"

UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API UKalmalaOceanOriginStabilityPeerTestSubsystem final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaOceanOriginStabilityPeerTestSubsystem, STATGROUP_Tickables);
	}

private:
	TWeakObjectPtr<class AKalmalaOceanSkiff> ObservedSkiff;
	FVector2D JourneyStartPosition = FVector2D::ZeroVector;
	FIntVector JourneyStartWorldOrigin = FIntVector::ZeroValue;
	FString LocalSeat = TEXT("None");
	bool bHasStarted = false;
	bool bOriginRemainedStable = true;
	bool bVerificationReported = false;
};
