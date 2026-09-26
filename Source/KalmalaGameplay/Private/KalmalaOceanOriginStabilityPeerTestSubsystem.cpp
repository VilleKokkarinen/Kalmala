#include "KalmalaOceanOriginStabilityPeerTestSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "KalmalaCharacter.h"
#include "KalmalaOceanSkiff.h"
#include "Misc/Parse.h"

bool UKalmalaOceanOriginStabilityPeerTestSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

#if !UE_BUILD_SHIPPING
	return FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest"));
#else
	return false;
#endif
}

void UKalmalaOceanOriginStabilityPeerTestSubsystem::Tick(const float DeltaSeconds)
{
	static_cast<void>(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	UWorld* World = GetWorld();
	if (World == nullptr || bVerificationReported)
	{
		return;
	}

	AKalmalaOceanSkiff* Skiff = ObservedSkiff.Get();
	if (!bHasStarted || !IsValid(Skiff))
	{
		Skiff = nullptr;
		for (TActorIterator<AKalmalaOceanSkiff> SkiffIt(World); SkiffIt; ++SkiffIt)
		{
			AKalmalaOceanSkiff* Candidate = *SkiffIt;
			if (!IsValid(Candidate) || Candidate->GetHelmOccupant() == nullptr
				|| Candidate->GetPassengerOccupant() == nullptr)
			{
				continue;
			}

			AKalmalaCharacter* LocalCharacter = nullptr;
			for (TActorIterator<AKalmalaCharacter> CharacterIt(World); CharacterIt; ++CharacterIt)
			{
				AKalmalaCharacter* Character = *CharacterIt;
				if (IsValid(Character) && Character->IsLocallyControlled()
					&& Character->GetAttachParentActor() == Candidate)
				{
					LocalCharacter = Character;
					break;
				}
			}
			if (LocalCharacter == nullptr)
			{
				continue;
			}

			Skiff = Candidate;
			ObservedSkiff = Candidate;
			JourneyStartPosition = FVector2D(Candidate->GetActorLocation());
			JourneyStartWorldOrigin = World->OriginLocation;
			LocalSeat = Candidate->GetHelmOccupant() == LocalCharacter
				? TEXT("Helm") : TEXT("Passenger");
			bHasStarted = true;
			break;
		}
	}

	if (!bHasStarted || !IsValid(Skiff))
	{
		return;
	}

	const FIntVector CurrentWorldOrigin = World->OriginLocation;
	bOriginRemainedStable &= CurrentWorldOrigin == JourneyStartWorldOrigin;
	const float Travelled = FVector2D::Distance(
		JourneyStartPosition, FVector2D(Skiff->GetActorLocation()));
	if (Skiff->GetMode() != EKalmalaOceanSkiffMode::Moored || Travelled < 239000.0f)
	{
		return;
	}

	const bool bPassed = bOriginRemainedStable
		&& JourneyStartWorldOrigin == FIntVector::ZeroValue
		&& CurrentWorldOrigin == FIntVector::ZeroValue;
	const TCHAR* Peer = World->GetNetMode() == NM_Client
		? TEXT("RemoteClient") : TEXT("ListenServer");
	UE_LOG(LogTemp, Display,
		TEXT("Ocean origin stability verification %s: Peer=%s Seat=%s Distance=%.0f StartOrigin=(%d,%d,%d) EndOrigin=(%d,%d,%d) OriginStable=%d."),
		bPassed ? TEXT("passed") : TEXT("FAILED"), Peer, *LocalSeat, Travelled,
		JourneyStartWorldOrigin.X, JourneyStartWorldOrigin.Y, JourneyStartWorldOrigin.Z,
		CurrentWorldOrigin.X, CurrentWorldOrigin.Y, CurrentWorldOrigin.Z,
		bOriginRemainedStable ? 1 : 0);
	if (!bPassed)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Ocean origin stability verification FAILED: journey peer left the fixed zero-origin contract."));
	}
	bVerificationReported = true;
#endif
}
