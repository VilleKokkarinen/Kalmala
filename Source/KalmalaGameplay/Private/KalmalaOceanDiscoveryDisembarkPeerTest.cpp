#include "KalmalaOceanDiscoveryDisembarkPeerTest.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/NetConnection.h"
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
#include "Misc/Crc.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "ProfilingDebugging/CsvProfiler.h"

namespace
{
	constexpr float SeaSurfaceZ = 0.0f;
	constexpr float ExitOffsetCm = 260.0f;
	constexpr int32 DiscoverySearchRadiusCells = 24;
	constexpr float ObservationTimeoutSeconds = 30.0f;
	constexpr float ReconnectSetupTimeoutSeconds = 180.0f;

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

	bool GetIdentityHash(const APlayerState* PlayerState, uint32& OutHash)
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
	FString ReconnectPhase;
	if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaOceanReconnectPhase="), ReconnectPhase))
	{
		DriveReconnectPeerTest(ReconnectPhase);
		return;
	}

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
		if (bServerReported && !bNetworkProfileReported && NetworkProfileReportSeconds >= 0.0f
			&& GetWorld()->GetTimeSeconds() >= NetworkProfileReportSeconds)
		{
			ReportPeerNetworkProfile();
		}
	}
	else if (bServerOutcomePublished && !bLocalOwnerReported)
	{
		VerifyLocalOwnerReplica();
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCaptureRenderedFrameTimes")))
	{
		const bool bAcceptedStateObserved = HasAuthority() ? bServerReported : bLocalOwnerReported;
		if (bAcceptedStateObserved && !bRenderedFrameCaptureStarted)
		{
			StartRenderedFrameCapture();
		}
		if (bRenderedFrameCaptureStarted && !bRenderedFrameCaptureCompleted)
		{
			UpdateRenderedFrameCapture();
		}
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
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, bLateJoinOutcomePublished);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, HostIdentityHash);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, HelmIdentityHash);
	DOREPLIFETIME(AKalmalaOceanDiscoveryDisembarkPeerTest, LateJoinIdentityHash);
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::DriveReconnectPeerTest(const FString& Phase)
{
	const bool bResume = Phase.Equals(TEXT("Resume"), ESearchCase::IgnoreCase);
	if (!bResume && !Phase.Equals(TEXT("Seed"), ESearchCase::IgnoreCase))
	{
		FailReconnect(TEXT("phase must be Seed or Resume"));
		return;
	}

	if (HasAuthority())
	{
		if (ReconnectStage == 0 && !bServerFailed && PrepareReconnectScenario(bResume))
		{
			ReconnectStage = 1;
		}
		if (ReconnectStage == 1 && !bLateJoinOutcomePublished && !bServerFailed)
		{
			VerifyReconnectLateJoin(bResume);
		}
	}
	else if (!bReconnectLocalReported && !bServerFailed)
	{
		VerifyReconnectLocalReplica(Phase);
	}

	if (!bLateJoinOutcomePublished && GetWorld() != nullptr
		&& GetWorld()->GetTimeSeconds() - StartedAtSeconds > ReconnectSetupTimeoutSeconds)
	{
		FailReconnect(TEXT("two original players and one late-joining peer did not complete the bounded scenario"));
	}
}

bool AKalmalaOceanDiscoveryDisembarkPeerTest::FindReconnectDiscovery(
	FKalmalaOceanDiscoveryDescriptor& OutDescriptor) const
{
	const AKalmalaWorldGenerationGameState* WorldState = GetWorld() != nullptr
		? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
	if (WorldState == nullptr || !WorldState->GetWorldGenerationConfig().IsValid()) return false;
	const FKalmalaWorldGenerationConfig& Config = WorldState->GetWorldGenerationConfig();
	const FVector2D SearchCenter(447810.0, -31559.0);
	const FIntPoint CenterKey = FKalmalaWorldPopulationLayout::GetSpatialKey(SearchCenter);
	for (int32 Y = -DiscoverySearchRadiusCells; Y <= DiscoverySearchRadiusCells; ++Y)
	{
		for (int32 X = -DiscoverySearchRadiusCells; X <= DiscoverySearchRadiusCells; ++X)
		{
			const FIntPoint Key = CenterKey + FIntPoint(X, Y);
			for (const FKalmalaOceanDiscoveryDescriptor& Candidate : FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(Config, Key))
			{
				const FVector2D CandidatePosition(Candidate.Location);
				if (AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, CandidatePosition, 0.0f)
					&& HasSafeExitCandidate(Config, CandidatePosition))
				{
					OutDescriptor = Candidate;
					return true;
				}
			}
		}
	}
	return false;
}

bool AKalmalaOceanDiscoveryDisembarkPeerTest::HasSingleReconnectSkiff() const
{
	if (GetWorld() == nullptr || !IsValid(TestSkiff)) return false;
	int32 Count = 0;
	for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It) ++Count;
	return Count == 1;
}

bool AKalmalaOceanDiscoveryDisembarkPeerTest::PrepareReconnectScenario(const bool bResume)
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		FailReconnect(TEXT("authoritative world is unavailable"));
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
		if (Controller == nullptr || Character == nullptr || Character->GetPlayerState() == nullptr) continue;
		++PlayerCount;
		if (Controller->IsLocalController()) Host = Character;
		else if (Remote == nullptr) Remote = Character;
	}
	if (PlayerCount < 2) return false;
	if (PlayerCount != 2 || Host == nullptr || Remote == nullptr)
	{
		FailReconnect(TEXT("expected exactly one listen host and one returning remote peer before setup"));
		return false;
	}

	AKalmalaGameMode* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
	AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
	FKalmalaOceanDiscoveryDescriptor DiscoveryDescriptor;
	uint32 LocalHostHash = 0;
	uint32 LocalHelmHash = 0;
	if (GameMode == nullptr || WorldState == nullptr || WorldState->GetWorldGenerationConfig().WorldSeed != 418
		|| !GetIdentityHash(Host->GetPlayerState(), LocalHostHash)
		|| !GetIdentityHash(Remote->GetPlayerState(), LocalHelmHash) || LocalHostHash == LocalHelmHash
		|| !FindReconnectDiscovery(DiscoveryDescriptor))
	{
		FailReconnect(TEXT("world seed, authenticated peer identities, or canonical sea discovery is unavailable"));
		return false;
	}

	const FKalmalaOceanDiscoveryDefinition* Definition =
		FKalmalaOceanDiscoveryCatalogue::FindDefinition(DiscoveryDescriptor.DiscoveryId);
	UKalmalaInventoryComponent* HostInventory = Host->GetInventoryComponent();
	UKalmalaInventoryComponent* HelmInventory = Remote->GetInventoryComponent();
	UKalmalaDiscoveryProgressComponent* HostDiscovery = Host->GetDiscoveryProgressComponent();
	UKalmalaDiscoveryProgressComponent* HelmDiscovery = Remote->GetDiscoveryProgressComponent();
	if (Definition == nullptr || HostInventory == nullptr || HelmInventory == nullptr
		|| HostDiscovery == nullptr || HelmDiscovery == nullptr)
	{
		FailReconnect(TEXT("peer inventory, discovery presentation, or reward catalogue is unavailable"));
		return false;
	}

	ExpectedDiscoveryId = DiscoveryDescriptor.DiscoveryId;
	ExpectedRewardItemId = Definition->RewardItemId;
	ExpectedRewardQuantity = Definition->RewardQuantity;
	HostIdentityHash = LocalHostHash;
	HelmIdentityHash = LocalHelmHash;

	if (bResume)
	{
		int32 SkiffCount = 0;
		TestSkiff = nullptr;
		for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
		{
			++SkiffCount;
			if (It->GetPersistentVesselId() == TEXT("ocean-skiff:primary")) TestSkiff = *It;
		}
		const FVector ExpectedLocation(DiscoveryDescriptor.Location.X, DiscoveryDescriptor.Location.Y, SeaSurfaceZ);
		const bool bRestoredSeats = HasSingleReconnectSkiff()
			&& TestSkiff->GetPersistentVesselId() == TEXT("ocean-skiff:primary")
			&& TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored
			&& TestSkiff->GetActorLocation().Equals(ExpectedLocation, 1.0f)
			&& TestSkiff->GetHelmOccupant() == Remote
			&& TestSkiff->GetPassengerOccupant() == Host
			&& Remote->GetAttachParentActor() == TestSkiff
			&& Host->GetAttachParentActor() == TestSkiff;
		if (SkiffCount != 1 || !bRestoredSeats
			|| HelmInventory->GetQuantity(ExpectedRewardItemId) != 0
			|| HostInventory->GetQuantity(ExpectedRewardItemId) != 0)
		{
			FailReconnect(TEXT("restart did not restore exactly one moored vessel and the authenticated original seats, or inventory baseline was not fresh"));
			return false;
		}

		if (GameMode->ClaimOceanDiscovery(Remote, DiscoveryDescriptor)
			|| HelmDiscovery->GetFeedback() != EKalmalaDiscoveryFeedback::AlreadyFound
			|| HelmInventory->GetQuantity(ExpectedRewardItemId) != 0)
		{
			FailReconnect(TEXT("reconnected owner could replay the sea discovery or received a duplicate reward"));
			return false;
		}
	}
	else
	{
		for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
		{
			FailReconnect(TEXT("fresh seed phase already contains a vessel"));
			return false;
		}
		if (HostInventory->GetQuantity(ExpectedRewardItemId) != 0
			|| HelmInventory->GetQuantity(ExpectedRewardItemId) != 0
			|| HostDiscovery->GetFeedback() != EKalmalaDiscoveryFeedback::None
			|| HelmDiscovery->GetFeedback() != EKalmalaDiscoveryFeedback::None)
		{
			FailReconnect(TEXT("fresh peer reward and discovery baselines are not empty"));
			return false;
		}

		const FVector Location(DiscoveryDescriptor.Location.X, DiscoveryDescriptor.Location.Y, SeaSurfaceZ);
		Host->SetActorLocationAndRotation(Location, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		Remote->SetActorLocationAndRotation(Location, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		TestSkiff = GetWorld()->SpawnActor<AKalmalaOceanSkiff>(Location, FRotator::ZeroRotator, Parameters);
		if (TestSkiff == nullptr || !TestSkiff->InitializePersistentIdentityFromServer(TEXT("ocean-skiff:primary")))
		{
			FailReconnect(TEXT("could not create the one server-owned primary skiff"));
			return false;
		}
		GameMode->OnOceanSkiffStateChanged(TestSkiff);
		if (!TestSkiff->TryInteractFromServer(Remote) || !TestSkiff->TryInteractFromServer(Host)
			|| TestSkiff->GetHelmOccupant() != Remote || TestSkiff->GetPassengerOccupant() != Host
			|| !GameMode->ClaimOceanDiscovery(Remote, DiscoveryDescriptor)
			|| HelmInventory->GetQuantity(ExpectedRewardItemId) != ExpectedRewardQuantity
			|| HostInventory->GetQuantity(ExpectedRewardItemId) != 0
			|| HelmDiscovery->GetFeedback() != EKalmalaDiscoveryFeedback::LandmarkFound
			|| HostDiscovery->GetFeedback() != EKalmalaDiscoveryFeedback::None)
		{
			FailReconnect(TEXT("server could not persist the two authenticated seats and one owner-only discovery reward"));
			return false;
		}
	}

	HelmCharacter = Remote;
	PassengerCharacter = Host;
	HelmPlayerState = Remote->GetPlayerState();
	PassengerPlayerState = Host->GetPlayerState();
	bServerOutcomePublished = true;
	ForceNetUpdate();

	UE_LOG(LogTemp, Display,
		TEXT("Ocean skiff reconnect %s server passed: Seed=418 VesselActors=1 VesselId=ocean-skiff:primary HostId=%08x HelmId=%08x Seats=Helm,Passenger Discovery=%s Reward=%s:%d ClaimState=%s InventoryQuantity=%d"),
		bResume ? TEXT("resume") : TEXT("seed"), HostIdentityHash, HelmIdentityHash,
		*ExpectedDiscoveryId.ToString(), *ExpectedRewardItemId.ToString(), ExpectedRewardQuantity,
		bResume ? TEXT("AlreadyFound") : TEXT("LandmarkFound"),
		bResume ? 0 : HelmInventory->GetQuantity(ExpectedRewardItemId));
	return true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyReconnectLateJoin(const bool bResume)
{
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
	if (PlayerCount != 3 || LateJoinCharacter == nullptr || !HasSingleReconnectSkiff()
		|| !IsValid(HelmCharacter.Get()) || !IsValid(PassengerCharacter.Get())
		|| TestSkiff->GetMode() != EKalmalaOceanSkiffMode::Moored
		|| TestSkiff->GetHelmOccupant() != HelmCharacter.Get()
		|| TestSkiff->GetPassengerOccupant() != PassengerCharacter.Get()
		|| LateJoinCharacter->GetAttachParentActor() != nullptr)
	{
		FailReconnect(TEXT("late join changed vessel count, original occupancy, or the late player's attachment state"));
		return;
	}

	uint32 LateJoinHash = 0;
	UKalmalaInventoryComponent* Inventory = LateJoinCharacter->GetInventoryComponent();
	UKalmalaDiscoveryProgressComponent* Discovery = LateJoinCharacter->GetDiscoveryProgressComponent();
	if (!GetIdentityHash(LateJoinCharacter->GetPlayerState(), LateJoinHash)
		|| LateJoinHash == HostIdentityHash || LateJoinHash == HelmIdentityHash
		|| Inventory == nullptr || Discovery == nullptr
		|| Inventory->GetQuantity(ExpectedRewardItemId) != 0
		|| Discovery->GetFeedback() != EKalmalaDiscoveryFeedback::None)
	{
		FailReconnect(TEXT("late join did not have a distinct authenticated identity and private discovery baseline"));
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
		TEXT("Ocean skiff reconnect %s late-join server passed: Seed=418 LateJoinId=%08x VesselActors=1 Helm=OriginalOwner Passenger=Host Attached=0 RewardLeak=0"),
		bResume ? TEXT("resume") : TEXT("seed"), LateJoinHash);
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::VerifyReconnectLocalReplica(const FString& Phase)
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
	if (Controller == nullptr || !Controller->IsLocalController() || Character == nullptr
		|| Character->GetPlayerState() == nullptr || !bServerOutcomePublished) return;

	UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
	UKalmalaDiscoveryProgressComponent* Discovery = Character->GetDiscoveryProgressComponent();
	const APlayerState* LocalState = Character->GetPlayerState();
	if (!HasSingleReconnectSkiff() || Inventory == nullptr || Discovery == nullptr) return;
	const bool bResume = Phase.Equals(TEXT("Resume"), ESearchCase::IgnoreCase);
	const bool bSkiffState = TestSkiff->GetMode() == EKalmalaOceanSkiffMode::Moored
		&& IsValid(TestSkiff->GetHelmOccupant()) && IsValid(TestSkiff->GetPassengerOccupant())
		&& TestSkiff->GetHelmOccupant()->GetPlayerState() == HelmPlayerState
		&& TestSkiff->GetPassengerOccupant()->GetPlayerState() == PassengerPlayerState;

	if (LocalState == HelmPlayerState)
	{
		const bool bExpectedFeedback = bResume
			? Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::AlreadyFound && Discovery->GetFeedbackSerial() > 0
			: Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound && Discovery->GetFeedbackSerial() > 0;
		const int32 ExpectedQuantity = bResume ? 0 : ExpectedRewardQuantity;
		if (!bSkiffState || !bExpectedFeedback || Inventory->GetQuantity(ExpectedRewardItemId) != ExpectedQuantity
			|| Character->GetAttachParentActor() != TestSkiff) return;
		UE_LOG(LogTemp, Display,
			TEXT("Ocean skiff reconnect %s owner peer passed: Authority=0 Seat=Helm Seed=418 DiscoveryFeedback=%s Reward=%s:%d VesselActors=1"),
			bResume ? TEXT("resume") : TEXT("seed"), bResume ? TEXT("AlreadyFound") : TEXT("LandmarkFound"),
			*ExpectedRewardItemId.ToString(), Inventory->GetQuantity(ExpectedRewardItemId));
		bReconnectLocalReported = true;
		return;
	}
	if (LocalState == PassengerPlayerState)
	{
		if (!bSkiffState || Character->GetAttachParentActor() != TestSkiff
			|| Inventory->GetQuantity(ExpectedRewardItemId) != 0
			|| Discovery->GetFeedback() != EKalmalaDiscoveryFeedback::None) return;
		UE_LOG(LogTemp, Display,
			TEXT("Ocean skiff reconnect %s host peer passed: Authority=1 Seat=Passenger Seed=418 OwnerRewardLeak=0 VesselActors=1"),
			bResume ? TEXT("resume") : TEXT("seed"));
		bReconnectLocalReported = true;
		return;
	}
	uint32 LocalIdentityHash = 0;
	const bool bLocalIsAuthenticatedLateJoin = GetIdentityHash(LocalState, LocalIdentityHash)
		&& LocalIdentityHash == LateJoinIdentityHash;
	if (bLateJoinOutcomePublished && bLocalIsAuthenticatedLateJoin
		&& Character->GetAttachParentActor() == nullptr
		&& IsValid(TestSkiff->GetHelmOccupant()) && IsValid(TestSkiff->GetPassengerOccupant())
		&& TestSkiff->GetHelmOccupant()->GetPlayerState() == HelmPlayerState
		&& TestSkiff->GetPassengerOccupant()->GetPlayerState() == PassengerPlayerState
		&& Inventory->GetQuantity(ExpectedRewardItemId) == 0
		&& Discovery->GetFeedback() == EKalmalaDiscoveryFeedback::None)
	{
		UE_LOG(LogTemp, Display,
			TEXT("Ocean skiff reconnect %s late-join peer passed: Authority=0 Seed=418 VesselActors=1 Seats=OriginalPeers LatePlayerAttached=0 RewardLeak=0"),
			bResume ? TEXT("resume") : TEXT("seed"));
		bReconnectLocalReported = true;
	}
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::FailReconnect(const TCHAR* Reason)
{
	if (bServerFailed) return;
	bServerFailed = true;
	UE_LOG(LogTemp, Error, TEXT("Ocean skiff reconnect peer verification FAILED: %s"), Reason);
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
	APlayerController* RemoteController = Cast<APlayerController>(Remote->GetController());
	UNetConnection* RemoteConnection = RemoteController != nullptr ? RemoteController->GetNetConnection() : nullptr;
	if (RemoteConnection == nullptr)
	{
		Fail(TEXT("remote peer network connection is unavailable for the M8 traffic profile"));
		return false;
	}
	ProfiledRemoteConnection = RemoteConnection;
	NetworkProfileStartSeconds = GetWorld()->GetTimeSeconds();
	NetworkProfileStartInBytes = RemoteConnection->InTotalBytes;
	NetworkProfileStartOutBytes = RemoteConnection->OutTotalBytes;
	NetworkProfileStartInPackets = RemoteConnection->InTotalPackets;
	NetworkProfileStartOutPackets = RemoteConnection->OutTotalPackets;

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
	NetworkProfileReportSeconds = GetWorld()->GetTimeSeconds() + 1.0f;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::ReportPeerNetworkProfile()
{
	const UNetConnection* Connection = ProfiledRemoteConnection.Get();
	if (Connection == nullptr || NetworkProfileStartSeconds < 0.0f)
	{
		Fail(TEXT("remote peer connection closed before the M8 traffic profile completed"));
		return;
	}

	const uint32 InBytes = static_cast<uint32>(Connection->InTotalBytes)
		- static_cast<uint32>(NetworkProfileStartInBytes);
	const uint32 OutBytes = static_cast<uint32>(Connection->OutTotalBytes)
		- static_cast<uint32>(NetworkProfileStartOutBytes);
	const uint32 InPackets = static_cast<uint32>(Connection->InTotalPackets)
		- static_cast<uint32>(NetworkProfileStartInPackets);
	const uint32 OutPackets = static_cast<uint32>(Connection->OutTotalPackets)
		- static_cast<uint32>(NetworkProfileStartOutPackets);
	const float WindowSeconds = GetWorld()->GetTimeSeconds() - NetworkProfileStartSeconds;
	UE_LOG(LogTemp, Display,
		TEXT("Ocean M8 peer connection profile: Peer=Client WindowSeconds=%.2f InBytes=%u OutBytes=%u InPackets=%u OutPackets=%u"),
		WindowSeconds, InBytes, OutBytes, InPackets, OutPackets);
	bNetworkProfileReported = true;
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::StartRenderedFrameCapture()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen"))
		|| FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
	{
		Fail(TEXT("rendered frame capture requires the offscreen RHI mode without NullRHI"));
		return;
	}

	FString OutputDirectory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("KalmalaFrameTimeOutputDir="), OutputDirectory)
		|| !FPaths::DirectoryExists(OutputDirectory)
		|| !FParse::Value(FCommandLine::Get(), TEXT("ResX="), RenderedFrameCaptureWidth)
		|| !FParse::Value(FCommandLine::Get(), TEXT("ResY="), RenderedFrameCaptureHeight)
		|| RenderedFrameCaptureWidth <= 0 || RenderedFrameCaptureHeight <= 0)
	{
		Fail(TEXT("rendered frame capture requires an existing output directory and positive ResX/ResY"));
		return;
	}

#if CSV_PROFILER
	RenderedFrameCapturePeer = HasAuthority() ? TEXT("ListenServer") : TEXT("Client");
	RenderedFrameCaptureFilename = HasAuthority() ? TEXT("M8SkiffListenServer") : TEXT("M8SkiffClient");
	FCsvProfiler::Get()->BeginCapture(300, OutputDirectory, RenderedFrameCaptureFilename,
		ECsvProfilerFlags::WriteCompletionFile);
	bRenderedFrameCaptureStarted = true;
	UE_LOG(LogTemp, Display,
		TEXT("Ocean M8 rendered frame capture started: Peer=%s Resolution=%dx%d RenderMode=OffscreenRHI Frames=300 Csv=%s.csv"),
		*RenderedFrameCapturePeer, RenderedFrameCaptureWidth, RenderedFrameCaptureHeight,
		*RenderedFrameCaptureFilename);
#else
	Fail(TEXT("rendered frame capture requested but CSV_PROFILER is disabled in this build"));
#endif
}

void AKalmalaOceanDiscoveryDisembarkPeerTest::UpdateRenderedFrameCapture()
{
#if CSV_PROFILER
	FCsvProfiler* CsvProfiler = FCsvProfiler::Get();
	if (CsvProfiler->IsCapturing())
	{
		bRenderedFrameCaptureSawStart = true;
		return;
	}

	if (!bRenderedFrameCaptureSawStart || CsvProfiler->IsWritingFile())
	{
		return;
	}

	bRenderedFrameCaptureCompleted = true;
	UE_LOG(LogTemp, Display,
		TEXT("Ocean M8 rendered frame capture complete: Peer=%s Resolution=%dx%d RenderMode=OffscreenRHI Frames=300 Csv=%s.csv"),
		*RenderedFrameCapturePeer, RenderedFrameCaptureWidth, RenderedFrameCaptureHeight,
		*RenderedFrameCaptureFilename);
#endif
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
