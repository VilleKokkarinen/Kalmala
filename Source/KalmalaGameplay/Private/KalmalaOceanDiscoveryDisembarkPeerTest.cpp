#include "KalmalaOceanDiscoveryDisembarkPeerTest.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaOceanDiscoveryCatalogue.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float SeaSurfaceZ = 0.0f;
	constexpr float ExitOffsetCm = 260.0f;
	constexpr int32 DiscoverySearchRadiusCells = 24;
	constexpr float ObservationTimeoutSeconds = 30.0f;

	bool HasSafeExitCandidate(const FKalmalaWorldGenerationConfig& Config, const FVector2D Center)
	{
		for (int32 DirectionIndex = 0; DirectionIndex < 8; ++DirectionIndex)
		{
			const float Angle = FMath::DegreesToRadians(DirectionIndex * 45.0f);
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			if (AKalmalaOceanSkiff::IsSafeExitSurfaceForConfig(Config,
				Center + Direction * ExitOffsetCm, 44.0))
			{
				return true;
			}
		}
		return false;
	}
}

AKalmalaOceanDiscoveryDisembarkPeerTest::AKalmalaOceanDiscoveryDisembarkPeerTest()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(5.0f);
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetWorld() == nullptr || StartedAtSeconds < 0.0f)
	{
		StartedAtSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	}

#if !UE_BUILD_SHIPPING
	if (HasAuthority())
	{
		if (!bSetupAttempted && !bServerFailed && PrepareServerScenario())
		{
			bSetupAttempted = true;
		}
		if (bSetupAttempted && bServerOutcomePublished && !bServerReported && !bServerFailed)
		{
			VerifyServerOutcome();
		}
	}
	else if (bServerOutcomePublished && !bLocalOwnerReported)
	{
		VerifyLocalOwnerReplica();
	}
#endif
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, TestSkiff);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, HelmPlayerState);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, PassengerPlayerState);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, ExpectedDiscoveryId);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, ExpectedRewardItemId);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, ExpectedRewardQuantity);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, bServerOutcomePublished);
}

bool AKalmalaOceanDiscoveryDisembarkPeerTest::PrepareServerScenario()
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		Fail(TEXT("server world is unavailable"));
		return false;
	}

	AKalmalaCharacter* Host = nullptr;
	AKalmalaCharacter* Remote = nullptr;
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		AKalmalaCharacter* Character = Controller != nullptr
			? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
		if (Controller == nullptr || Character == nullptr || Character->GetPlayerState() == nullptr)
		{
			continue;
		}
		++PlayerCount;
		if (Controller->IsLocalController()) Host = Character;
		else Remote = Character;
	}
	if (PlayerCount != 2 || Host == nullptr || Remote == nullptr)
	{
		if (GetWorld()->GetTimeSeconds() - StartedAtSeconds > 120.0f)
		{
			Fail(TEXT("expected one listen host and one remote player before the setup timeout"));
		}
		return false;
	}

	AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
	AKalmalaGameMode* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
	if (WorldState == nullptr || GameMode == nullptr || !WorldState->GetWorldGenerationConfig().IsValid())
	{
		Fail(TEXT("authoritative world-generation state is unavailable"));
		return false;
	}
	const FKalmalaWorldGenerationConfig& Config = WorldState->GetWorldGenerationConfig();

	FKalmalaOceanDiscoveryDescriptor DiscoveryDescriptor;
	bool bFoundDiscovery = false;
	const FVector2D SearchCenter(447810.0, -31559.0);
	const FIntPoint CenterKey = FKalmalaWorldPopulationLayout::GetSpatialKey(SearchCenter);
	for (int32 Y = -DiscoverySearchRadiusCells; Y <= DiscoverySearchRadiusCells && !bFoundDiscovery; ++Y)
	{
		for (int32 X = -DiscoverySearchRadiusCells; X <= DiscoverySearchRadiusCells && !bFoundDiscovery; ++X)
		{
			const FIntPoint Key = CenterKey + FIntPoint(X, Y);
			for (const FKalmalaOceanDiscoveryDescriptor& Candidate : FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(Config, Key))
			{
				const FVector2D CandidatePosition(Candidate.Location);
				if (!AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, CandidatePosition, 0.0f)
					|| !HasSafeExitCandidate(Config, CandidatePosition))
				{
					continue;
				}
				DiscoveryDescriptor = Candidate;
				bFoundDiscovery = true;
				break;
			}
		}
	}
	if (!bFoundDiscovery)
	{
		Fail(TEXT("bounded seed-418 ocean search found no deep-water discovery with a safe exit"));
		return false;
	}

	const FKalmalaOceanDiscoveryDefinition* Definition =
		FKalmalaOceanDiscoveryCatalogue::FindDefinition(DiscoveryDescriptor.DiscoveryId);
	UKalmalaInventoryComponent* HostInventory = Host->GetInventoryComponent();
	UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
	if (Definition == nullptr || HostInventory == nullptr || RemoteInventory == nullptr
		|| HostInventory->GetQuantity(Definition->RewardItemId) != 0
		|| RemoteInventory->GetQuantity(Definition->RewardItemId) != 0)
	{
		Fail(TEXT("fresh peer reward baseline or discovery catalogue is invalid"));
		return false;
	}

	for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
	{
		Fail(TEXT("isolated peer profile already contains an ocean skiff"));
		return false;
	}

	const FVector SkiffLocation(DiscoveryDescriptor.Location.X, DiscoveryDescriptor.Location.Y, SeaSurfaceZ);
	Host->SetActorLocationAndRotation(SkiffLocation, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	Remote->SetActorLocationAndRotation(SkiffLocation, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TestSkiff = GetWorld()->SpawnActor<AKalmalaOceanSkiff>(SkiffLocation, FRotator::ZeroRotator, SpawnParameters);
	if (TestSkiff == nullptr || !TestSkiff->InitializePersistentIdentityFromServer(TEXT("ocean-skiff:primary")))
	{
		Fail(TEXT("server could not create the bounded moored skiff"));
		return false;
	}
	GameMode->OnOceanSkiffStateChanged(TestSkiff);

	if (!TestSkiff->TryInteractFromServer(Remote) || !TestSkiff->TryInteractFromServer(Host)
		|| TestSkiff->GetHelmOccupant() != Remote || TestSkiff->GetPassengerOccupant() != Host)
	{
		Fail(TEXT("server could not assign the expected helm and passenger seats"));
		return false;
	}
	HelmCharacter = Remote;
	PassengerCharacter = Host;
	HelmPlayerState = Remote->GetPlayerState();
	PassengerPlayerState = Host->GetPlayerState();
	ExpectedDiscoveryId = DiscoveryDescriptor.DiscoveryId;
	ExpectedRewardItemId = Definition->RewardItemId;
	ExpectedRewardQuantity = Definition->RewardQuantity;

	const bool bHelmClaimed = GameMode->ClaimOceanDiscovery(Remote, DiscoveryDescriptor);
	const bool bPassengerClaimed = GameMode->ClaimOceanDiscovery(Host, DiscoveryDescriptor);
	if (!bHelmClaimed || !bPassengerClaimed)
	{
		Fail(TEXT("server rejected a canonical in-range discovery claim for one of the seated peers"));
		return false;
	}

	UKalmalaDiscoveryProgressComponent* HelmDiscovery = Remote->GetDiscoveryProgressComponent();
	UKalmalaDiscoveryProgressComponent* PassengerDiscovery = Host->GetDiscoveryProgressComponent();
	const bool bClaimsAccepted = HelmDiscovery != nullptr && PassengerDiscovery != nullptr
		&& HelmDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& PassengerDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& RemoteInventory->GetQuantity(Definition->RewardItemId) == Definition->RewardQuantity
		&& HostInventory->GetQuantity(Definition->RewardItemId) == Definition->RewardQuantity;
	if (!bClaimsAccepted)
	{
		Fail(TEXT("both owner-scoped discovery claims did not grant exactly one catalogue reward"));
		return false;
	}

	const bool bPassengerDisembarked = TestSkiff->TryDisembarkFromServer(Host);
	const bool bHelmDisembarked = TestSkiff->TryDisembarkFromServer(Remote);
	if (!bPassengerDisembarked || !bHelmDisembarked)
	{
		Fail(TEXT("a moored peer could not safely disembark"));
		return false;
	}

	bServerOutcomePublished = true;
	ForceNetUpdate();
	return true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyServerOutcome()
{
	AKalmalaCharacter* Helm = HelmCharacter.Get();
	AKalmalaCharacter* Passenger = PassengerCharacter.Get();
	const UKalmalaDiscoveryProgressComponent* HelmDiscovery = Helm != nullptr
		? Helm->GetDiscoveryProgressComponent() : nullptr;
	const UKalmalaDiscoveryProgressComponent* PassengerDiscovery = Passenger != nullptr
		? Passenger->GetDiscoveryProgressComponent() : nullptr;
	const UKalmalaOceanTravelFeedbackComponent* HelmTravel = Helm != nullptr
		? Helm->GetOceanTravelFeedbackComponent() : nullptr;
	const UKalmalaOceanTravelFeedbackComponent* PassengerTravel = Passenger != nullptr
		? Passenger->GetOceanTravelFeedbackComponent() : nullptr;
	const UKalmalaInventoryComponent* HelmInventory = Helm != nullptr ? Helm->GetInventoryComponent() : nullptr;
	const UKalmalaInventoryComponent* PassengerInventory = Passenger != nullptr ? Passenger->GetInventoryComponent() : nullptr;
	const bool bPassed = IsValid(TestSkiff) && Helm != nullptr && Passenger != nullptr
		&& Helm->GetAttachParentActor() == nullptr && Passenger->GetAttachParentActor() == nullptr
		&& TestSkiff->GetHelmOccupant() == nullptr && TestSkiff->GetPassengerOccupant() == nullptr
		&& TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored
		&& HelmDiscovery != nullptr && PassengerDiscovery != nullptr
		&& HelmDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& PassengerDiscovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& HelmTravel != nullptr && PassengerTravel != nullptr
		&& HelmTravel->GetFeedback() == EKalmalaOceanTravelFeedback::Disembarked
		&& PassengerTravel->GetFeedback() == EKalmalaOceanTravelFeedback::Disembarked
		&& HelmInventory != nullptr && PassengerInventory != nullptr
		&& HelmInventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity
		&& PassengerInventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity;
	if (!bPassed)
	{
		Fail(TEXT("server did not retain both discovery rewards and stopped disembark outcomes"));
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("Ocean discovery-stop server passed: Seed=418 Players=2 Discovery=%s Reward=%s:%d Claims=2 Mode=Moored Disembarked=2"),
		*ExpectedDiscoveryId.ToString(), *ExpectedRewardItemId.ToString(), ExpectedRewardQuantity);
	bServerReported = true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyLocalOwnerReplica()
{
	APlayerController* LocalController = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	AKalmalaCharacter* LocalCharacter = LocalController != nullptr
		? Cast<AKalmalaCharacter>(LocalController->GetPawn()) : nullptr;
	if (LocalController == nullptr || !LocalController->IsLocalController() || LocalCharacter == nullptr)
	{
		return;
	}
	const UKalmalaDiscoveryProgressComponent* Discovery = LocalCharacter->GetDiscoveryProgressComponent();
	const UKalmalaOceanTravelFeedbackComponent* Travel = LocalCharacter->GetOceanTravelFeedbackComponent();
	const UKalmalaInventoryComponent* Inventory = LocalCharacter->GetInventoryComponent();
	if (Discovery == nullptr || Travel == nullptr || Inventory == nullptr || !IsValid(TestSkiff)) return;

	const bool bKnownPeer = LocalCharacter->GetPlayerState() == HelmPlayerState
		|| LocalCharacter->GetPlayerState() == PassengerPlayerState;
	const bool bAcceptedOwnerState = Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
		&& Discovery->GetFeedbackSerial() > 0
		&& Discovery->GetFeedbackLabel().StartsWith(TEXT("Sea discovery:"))
		&& Inventory->GetQuantity(ExpectedRewardItemId) == ExpectedRewardQuantity
		&& Travel->GetFeedback() == EKalmalaOceanTravelFeedback::Disembarked
		&& LocalCharacter->GetAttachParentActor() == nullptr
		&& TestSkiff->GetHelmOccupant() == nullptr && TestSkiff->GetPassengerOccupant() == nullptr
		&& TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored;
	if (!bKnownPeer || !bAcceptedOwnerState)
	{
		if (GetWorld()->GetTimeSeconds() - StartedAtSeconds > ObservationTimeoutSeconds)
		{
			Fail(TEXT("client did not observe its owner reward, stopped disembark, and empty replicated seats"));
		}
		return;
	}

	const TCHAR* Seat = LocalCharacter->GetPlayerState() == HelmPlayerState ? TEXT("Helm") : TEXT("Passenger");
	UE_LOG(LogTemp, Display,
		TEXT("Ocean discovery-stop peer replica passed: Authority=0 Seat=%s Discovery=%s DiscoveryFeedback=LandmarkFound Reward=%s:%d Disembarked=1 EmptySeats=1 Mode=Moored"),
		Seat, *ExpectedDiscoveryId.ToString(), *ExpectedRewardItemId.ToString(), ExpectedRewardQuantity);
	bLocalOwnerReported = true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::Fail(const TCHAR* Reason)
{
	if (bServerFailed) return;
	bServerFailed = true;
	UE_LOG(LogTemp, Error, TEXT("Ocean discovery-stop peer verification FAILED: %s"), Reason);
}
