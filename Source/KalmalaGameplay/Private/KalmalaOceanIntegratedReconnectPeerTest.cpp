#include "KalmalaOceanDiscoveryDisembarkPeerTest.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaOceanDiscoveryCatalogue.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Misc/Crc.h"

namespace
{
	bool HasReconnectSafeExitCandidate(const FKalmalaWorldGenerationConfig& Config, const FVector2D Center)
	{
		for (int32 DirectionIndex = 0; DirectionIndex < 8; ++DirectionIndex)
		{
			const float Angle = FMath::DegreesToRadians(DirectionIndex * 45.0f);
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			if (AKalmalaOceanSkiff::IsSafeExitSurfaceForConfig(Config,
				Center + Direction * 260.0f, 44.0f))
			{
				return true;
			}
		}
		return false;
	}

	bool GetIntegratedReconnectIdentityHash(const APlayerState* PlayerState, uint32& OutHash)
	{
		if (PlayerState == nullptr) return false;
		const FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
		const TSharedPtr<const FUniqueNetId> AuthenticatedId = UniqueId.GetUniqueNetId();
		if (!UniqueId.IsValid() || !AuthenticatedId.IsValid()) return false;
		const FString Identity = AuthenticatedId->GetType().ToString() + TEXT(":") + AuthenticatedId->ToString();
		if (Identity.IsEmpty() || Identity.Len() > 128) return false;
		OutHash = FCrc::StrCrc32(*Identity);
		return true;
	}
}

bool AKalmalaOceanDiscoveryDisembarkPeerTest::IsIntegratedReconnectRoute(
	const FKalmalaWorldGenerationConfig& Config, const FVector2D StartPosition) const
{
	if (!Config.IsValid() || !FKalmalaWorldBounds::Contains(Config, StartPosition)) return false;

	constexpr float TravelDistanceCm = 240000.0f;
	constexpr float CoastStopDistanceCm = 3000.0f;
	constexpr float RouteSampleIntervalCm = 5000.0f;
	constexpr float CrossTrackAllowanceCm = 15000.0f;
	const FVector2D Direction(1.0f, 0.0f);
	const FVector2D Side(0.0f, 1.0f);
	const FVector2D StopPosition = StartPosition + Direction * (TravelDistanceCm + CoastStopDistanceCm);
	const FIntPoint StartPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(FVector2D::ZeroVector, StartPosition);
	const FIntPoint StopPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(FVector2D::ZeroVector, StopPosition);
	if (StartPatch == StopPatch || !HasReconnectSafeExitCandidate(Config, StopPosition)) return false;

	for (float Distance = 0.0f; Distance <= TravelDistanceCm + CoastStopDistanceCm; Distance += RouteSampleIntervalCm)
	{
		const FVector2D Center = StartPosition + Direction * Distance;
		for (const float SideOffset : {-CrossTrackAllowanceCm, 0.0f, CrossTrackAllowanceCm})
		{
			const FVector2D SamplePosition = Center + Side * SideOffset;
			if (!FKalmalaWorldBounds::Contains(Config, SamplePosition)
				|| !AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, SamplePosition, 0.0f))
			{
				return false;
			}
		}
	}
	return true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyIntegratedJourneyLateJoin()
{
	if (!HasAuthority() || GetWorld() == nullptr || !IsValid(TestSkiff)
		|| TestSkiff->GetMode() != EKalmalaOceanSkiffMode::Underway || IntegratedJourneyDistance < 100.0f)
	{
		return;
	}

	int32 PlayerCount = 0;
	AKalmalaCharacter* LateJoinCharacter = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		AKalmalaCharacter* Character = Controller != nullptr
			? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
		if (Character == nullptr || Character->GetPlayerState() == nullptr) continue;
		++PlayerCount;
		if (Character->GetPlayerState() != HelmPlayerState && Character->GetPlayerState() != PassengerPlayerState)
		{
			LateJoinCharacter = Character;
		}
	}
	if (PlayerCount < 3) return;
	if (PlayerCount != 3 || LateJoinCharacter == nullptr
		|| !IsValid(HelmCharacter.Get()) || !IsValid(PassengerCharacter.Get())
		|| TestSkiff->GetHelmOccupant() != HelmCharacter.Get()
		|| TestSkiff->GetPassengerOccupant() != PassengerCharacter.Get()
		|| LateJoinCharacter->GetAttachParentActor() != nullptr)
	{
		Fail(TEXT("underway late join changed the original seats, vessel count, or late player's attachment state"));
		return;
	}

	uint32 LateJoinHash = 0;
	UKalmalaInventoryComponent* Inventory = LateJoinCharacter->GetInventoryComponent();
	UKalmalaDiscoveryProgressComponent* Discovery = LateJoinCharacter->GetDiscoveryProgressComponent();
	if (!GetIntegratedReconnectIdentityHash(LateJoinCharacter->GetPlayerState(), LateJoinHash)
		|| LateJoinHash == HostIdentityHash || LateJoinHash == HelmIdentityHash
		|| Inventory == nullptr || Discovery == nullptr
		|| Inventory->GetQuantity(ExpectedRewardItemId) != 0
		|| Discovery->GetFeedback() != EKalmalaDiscoveryFeedback::None)
	{
		Fail(TEXT("underway late join received private discovery feedback or reward, or lacked a distinct authenticated identity"));
		return;
	}

	LateJoinCharacter->SetActorLocation(TestSkiff->GetActorLocation() + FVector(1200.0f, 0.0f, 0.0f),
		false, nullptr, ETeleportType::TeleportPhysics);
	LateJoinCharacter->ForceNetUpdate();
	TestSkiff->ForceNetUpdate();
	LateJoinIdentityHash = LateJoinHash;
	bLateJoinOutcomePublished = true;
	ForceNetUpdate();
	UE_LOG(LogTemp, Display,
		TEXT("Ocean integrated journey late-join server passed: Seed=418 LateJoinId=%08x VesselActors=1 Underway=1 OriginalSeats=1 Attached=0 RewardLeak=0"),
		LateJoinHash);
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyIntegratedJourneyLateJoinReplica()
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
	if (Controller == nullptr || !Controller->IsLocalController() || Character == nullptr
		|| Character->GetPlayerState() == nullptr || !IsValid(TestSkiff)) return;

	uint32 LocalIdentityHash = 0;
	if (!GetIntegratedReconnectIdentityHash(Character->GetPlayerState(), LocalIdentityHash)
		|| LocalIdentityHash != LateJoinIdentityHash) return;

	const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
	const UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
	const UKalmalaDiscoveryProgressComponent* Discovery = Character->GetDiscoveryProgressComponent();
	const bool bAccepted = HasSingleReconnectSkiff() && State != nullptr
		&& State->GetWorldGenerationConfig().WorldSeed == 418
		&& TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Underway
		&& TestSkiff->GetHelmOccupant() != nullptr && TestSkiff->GetPassengerOccupant() != nullptr
		&& TestSkiff->GetHelmOccupant()->GetPlayerState() == HelmPlayerState
		&& TestSkiff->GetPassengerOccupant()->GetPlayerState() == PassengerPlayerState
		&& Character->GetAttachParentActor() == nullptr && Inventory != nullptr && Discovery != nullptr
		&& Inventory->GetQuantity(ExpectedRewardItemId) == 0
		&& Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::None;
	if (!bAccepted)
	{
		Fail(TEXT("late owner did not observe the underway vessel without attachment or private discovery state"));
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("Ocean integrated journey late-join peer passed: Authority=0 Seed=418 VesselActors=1 Underway=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0"));
	bReconnectLocalReported = true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyIntegratedReconnectServerOutcome()
{
	AKalmalaCharacter* Helm = HelmCharacter.Get();
	AKalmalaCharacter* Passenger = PassengerCharacter.Get();
	AKalmalaWorldGenerationGameState* State = GetWorld() != nullptr
		? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
	const UKalmalaDiscoveryProgressComponent* HelmDiscovery = Helm != nullptr
		? Helm->GetDiscoveryProgressComponent() : nullptr;
	const UKalmalaDiscoveryProgressComponent* PassengerDiscovery = Passenger != nullptr
		? Passenger->GetDiscoveryProgressComponent() : nullptr;
	const UKalmalaInventoryComponent* HelmInventory = Helm != nullptr ? Helm->GetInventoryComponent() : nullptr;
	const UKalmalaInventoryComponent* PassengerInventory = Passenger != nullptr ? Passenger->GetInventoryComponent() : nullptr;
	uint32 HostHash = 0;
	uint32 HelmHash = 0;
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		if (Controller != nullptr && Cast<AKalmalaCharacter>(Controller->GetPawn()) != nullptr) ++PlayerCount;
	}
	const bool bPassed = IsValid(TestSkiff) && HasSingleReconnectSkiff() && State != nullptr
		&& State->GetWorldGenerationConfig().WorldSeed == 418
		&& PlayerCount == 3 && bLateJoinOutcomePublished
		&& Helm != nullptr && Passenger != nullptr
		&& GetIntegratedReconnectIdentityHash(Passenger->GetPlayerState(), HostHash)
		&& GetIntegratedReconnectIdentityHash(Helm->GetPlayerState(), HelmHash)
		&& HostHash != HelmHash
		&& TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored
		&& TestSkiff->GetHelmOccupant() == Helm && TestSkiff->GetPassengerOccupant() == Passenger
		&& Helm->GetAttachParentActor() == TestSkiff && Passenger->GetAttachParentActor() == TestSkiff
		&& HelmDiscovery != nullptr && PassengerDiscovery != nullptr
		&& HelmDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& PassengerDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& HelmInventory != nullptr && PassengerInventory != nullptr
		&& HelmInventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity
		&& PassengerInventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity
		&& IntegratedJourneyDistance >= 239000.0f && IntegratedJourneyDistance <= 250000.0f
		&& bIntegratedJourneySawCrosswind && bIntegratedJourneySawCalm && bIntegratedJourneyStreamingPassed;
	if (!bPassed)
	{
		Fail(TEXT("integrated voyage did not retain its two accepted rewards and occupied seats with an unprivileged late joiner"));
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("Ocean integrated reconnect journey server passed: Seed=418 HostId=%08x HelmId=%08x Players=3 Discovery=%s Reward=%s:%d Claims=2 Distance=%.0f Crosswind=1 Calm=1 PatchStart=(%d,%d) PatchEnd=(%d,%d) ActivePatches=%d LateJoin=Underway RewardLeak=0 Seats=Helm,Passenger Mode=Moored StopX=%.0f StopY=%.0f"),
		HostHash, HelmHash, *ExpectedDiscoveryId.ToString(), *ExpectedRewardItemId.ToString(), ExpectedRewardQuantity,
		IntegratedJourneyDistance, IntegratedJourneyStartPatch.X, IntegratedJourneyStartPatch.Y,
		IntegratedJourneyEndPatch.X, IntegratedJourneyEndPatch.Y, IntegratedJourneyActivePatchCount,
		TestSkiff->GetActorLocation().X, TestSkiff->GetActorLocation().Y);
	bServerReported = true;
	NetworkProfileReportSeconds = GetWorld()->GetTimeSeconds() + 1.0f;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyIntegratedReconnectOwnerReplica()
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
	if (Controller == nullptr || !Controller->IsLocalController() || Character == nullptr
		|| Character->GetPlayerState() == nullptr || !IsValid(TestSkiff)) return;
	const APlayerState* LocalState = Character->GetPlayerState();
	const bool bIsHelm = LocalState == HelmPlayerState;
	const bool bIsPassenger = LocalState == PassengerPlayerState;
	if (!bIsHelm && !bIsPassenger) return;

	const UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
	const UKalmalaDiscoveryProgressComponent* Discovery = Character->GetDiscoveryProgressComponent();
	const bool bLocalJourneyAccepted = bIntegratedJourneyStarted
		&& IntegratedJourneyDistance >= 239000.0f && IntegratedJourneyDistance <= 250000.0f
		&& bIntegratedJourneySawCrosswind && bIntegratedJourneySawCalm && bIntegratedJourneyStreamingPassed
		&& bLocalIntegratedJourneySawCrosswind && bLocalIntegratedJourneySawCalm;
	const bool bLocalAccepted = bLocalJourneyAccepted && TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored
		&& TestSkiff->GetHelmOccupant() != nullptr && TestSkiff->GetPassengerOccupant() != nullptr
		&& TestSkiff->GetHelmOccupant()->GetPlayerState() == HelmPlayerState
		&& TestSkiff->GetPassengerOccupant()->GetPlayerState() == PassengerPlayerState
		&& Character->GetAttachParentActor() == TestSkiff && Inventory != nullptr && Discovery != nullptr
		&& Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& Discovery->GetFeedbackSerial() > 0
		&& Discovery->GetFeedbackLabel().StartsWith(TEXT("Sea discovery:"))
		&& Inventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity;
	if (!bLocalAccepted)
	{
		if (LocalIntegratedJourneyObservationStartSeconds < 0.0f)
		{
			LocalIntegratedJourneyObservationStartSeconds = GetWorld()->GetTimeSeconds();
		}
		if (GetWorld()->GetTimeSeconds() - LocalIntegratedJourneyObservationStartSeconds > 30.0f)
		{
			Fail(TEXT("original owner missed its saved seat, discovery reward, or integrated voyage replica"));
		}
		return;
	}

	const TCHAR* Seat = bIsHelm ? TEXT("Helm") : TEXT("Passenger");
	UE_LOG(LogTemp, Display,
		TEXT("Ocean integrated reconnect journey peer passed: Authority=0 Seat=%s Discovery=%s DiscoveryFeedback=LandmarkFound Reward=%s:%d Distance=%.0f Crosswind=1 Calm=1 Attached=1 Seats=Helm,Passenger Mode=Moored"),
		Seat, *ExpectedDiscoveryId.ToString(), *ExpectedRewardItemId.ToString(),
		ExpectedRewardQuantity, IntegratedJourneyDistance);
	bLocalOwnerReported = true;
}
