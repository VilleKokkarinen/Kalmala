#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaOceanTravelFeedbackComponent.generated.h"

UENUM(BlueprintType)
enum class EKalmalaOceanTravelFeedback : uint8
{
	None,
	ShallowLaunch,
	ShallowHull,
	WorldEdge,
	SessionSkiffExists,
	LaunchObstructed,
	SkiffLaunched,
	HelmAssigned,
	PassengerAssigned,
	SeatsFull,
	AlreadySeated,
	AttachedElsewhere,
	OutOfReach,
	StopBeforeDisembarking,
	NoSafeExit,
	Disembarked,
	TravelSaveUnavailable
};

/** Owner-only transient reasons for server-resolved ocean travel interactions. */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaOceanTravelFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKalmalaOceanTravelFeedbackComponent();

	static bool IsFeedbackAllowed(bool bServerAuthority, EKalmalaOceanTravelFeedback Feedback);
	static FString GetFeedbackText(EKalmalaOceanTravelFeedback Feedback);
	bool SetFeedbackFromServer(EKalmalaOceanTravelFeedback NewFeedback);

	EKalmalaOceanTravelFeedback GetFeedback() const { return Feedback; }
	uint32 GetFeedbackSerial() const { return FeedbackSerial; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated, VisibleAnywhere, Category="Ocean Travel")
	EKalmalaOceanTravelFeedback Feedback = EKalmalaOceanTravelFeedback::None;

	UPROPERTY(Replicated, VisibleAnywhere, Category="Ocean Travel")
	uint32 FeedbackSerial = 0;
};
