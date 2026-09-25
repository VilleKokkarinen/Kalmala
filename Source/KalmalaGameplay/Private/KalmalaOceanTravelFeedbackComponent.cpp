#include "KalmalaOceanTravelFeedbackComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UKalmalaOceanTravelFeedbackComponent::UKalmalaOceanTravelFeedbackComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

bool UKalmalaOceanTravelFeedbackComponent::IsFeedbackAllowed(
	const bool bServerAuthority, const EKalmalaOceanTravelFeedback NewFeedback)
{
	return bServerAuthority && NewFeedback > EKalmalaOceanTravelFeedback::None
		&& NewFeedback <= EKalmalaOceanTravelFeedback::TravelSaveUnavailable;
}

FString UKalmalaOceanTravelFeedbackComponent::GetFeedbackText(const EKalmalaOceanTravelFeedback InFeedback)
{
	switch (InFeedback)
	{
	case EKalmalaOceanTravelFeedback::ShallowLaunch:
		return TEXT("▲ TRAVEL · SHALLOW WATER · Launch needs at least 1 m of open-ocean depth beneath the whole hull. Wade or swim farther out, then interact with the generated coast.");
	case EKalmalaOceanTravelFeedback::ShallowHull:
		return TEXT("▲ TRAVEL · SHALLOW WATER · The full hull must fit over deep open ocean; shallow shore water, inland lakes, and the world edge are not navigable.");
	case EKalmalaOceanTravelFeedback::WorldEdge:
		return TEXT("◇ TRAVEL · WORLD EDGE · The playable boundary blocks launch here. Choose another coast inside the world.");
	case EKalmalaOceanTravelFeedback::SessionSkiffExists:
		return TEXT("□ TRAVEL · SKIFF IN USE · One session skiff is available. Board it when a seat is open.");
	case EKalmalaOceanTravelFeedback::LaunchObstructed:
		return TEXT("▲ TRAVEL · COAST BLOCKED · Aim at a clear generated shoreline surface beside open ocean.");
	case EKalmalaOceanTravelFeedback::SkiffLaunched:
		return TEXT("⚓ TRAVEL · SKIFF LAUNCHED · Interact with it to board. Launch and travel need at least 1 m of open-ocean depth beneath the whole hull.");
	case EKalmalaOceanTravelFeedback::HelmAssigned:
		return TEXT("⚓ TRAVEL · HELM SEAT · W/S throttle; A/D steer. The server stops the hull at shallow water, generated land, and world edges. Stop before disembarking.");
	case EKalmalaOceanTravelFeedback::PassengerAssigned:
		return TEXT("⚓ TRAVEL · PASSENGER SEAT · The helm controls the skiff. Shallow water, generated land, and world edges stop it.");
	case EKalmalaOceanTravelFeedback::SeatsFull:
		return TEXT("□ TRAVEL · SEATS FULL · Both skiff seats are occupied.");
	case EKalmalaOceanTravelFeedback::AlreadySeated:
		return TEXT("□ TRAVEL · ALREADY ABOARD · Your session player already occupies a skiff seat.");
	case EKalmalaOceanTravelFeedback::AttachedElsewhere:
		return TEXT("□ TRAVEL · DISMOUNT FIRST · Leave your current attachment before boarding the skiff.");
	case EKalmalaOceanTravelFeedback::OutOfReach:
		return TEXT("□ TRAVEL · OUT OF REACH · Move within 2.5 m of the skiff.");
	case EKalmalaOceanTravelFeedback::StopBeforeDisembarking:
		return TEXT("◇ TRAVEL · STILL MOVING · Release throttle and rudder; disembarking is safe at 0.5 m/s or slower.");
	case EKalmalaOceanTravelFeedback::NoSafeExit:
		return TEXT("◇ TRAVEL · NO SAFE EXIT · No clear landing capsule is available nearby. Move to open water and stop again.");
	case EKalmalaOceanTravelFeedback::Disembarked:
		return TEXT("⚓ TRAVEL · LEFT SKIFF · The server selected a clear landing point; continue from there.");
	case EKalmalaOceanTravelFeedback::TravelSaveUnavailable:
		return TEXT("□ TRAVEL · SAVE UNAVAILABLE · The server could not update your saved seat, so disembarking was cancelled safely.");
	default:
		return FString();
	}
}

bool UKalmalaOceanTravelFeedbackComponent::SetFeedbackFromServer(const EKalmalaOceanTravelFeedback NewFeedback)
{
	const AActor* Owner = GetOwner();
	if (!IsFeedbackAllowed(Owner != nullptr && Owner->HasAuthority(), NewFeedback))
	{
		return false;
	}

	Feedback = NewFeedback;
	FeedbackSerial = FeedbackSerial == TNumericLimits<uint32>::Max() ? 1 : FeedbackSerial + 1;
	if (AActor* MutableOwner = GetOwner())
	{
		MutableOwner->ForceNetUpdate();
	}
	return true;
}

void UKalmalaOceanTravelFeedbackComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UKalmalaOceanTravelFeedbackComponent, Feedback, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UKalmalaOceanTravelFeedbackComponent, FeedbackSerial, COND_OwnerOnly);
}
