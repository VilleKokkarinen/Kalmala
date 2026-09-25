#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaOceanDiscoveryDisembarkPeerTest.generated.h"

/** Isolated development fixture for owner-scoped sea discovery and safe disembark. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaOceanDiscoveryDisembarkPeerTest : public AActor
{
	GENERATED_BODY()

public:
	AKalmalaOceanDiscoveryDisembarkPeerTest();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	bool PrepareServerScenario();
	void VerifyServerOutcome();
	void VerifyLocalOwnerReplica();
	void Fail(const TCHAR* Reason);

	UPROPERTY(Replicated) TObjectPtr<class AKalmalaOceanSkiff> TestSkiff;
	UPROPERTY(Replicated) TObjectPtr<class APlayerState> HelmPlayerState;
	UPROPERTY(Replicated) TObjectPtr<class APlayerState> PassengerPlayerState;
	UPROPERTY(Replicated) FName ExpectedDiscoveryId = NAME_None;
	UPROPERTY(Replicated) FName ExpectedRewardItemId = NAME_None;
	UPROPERTY(Replicated) int32 ExpectedRewardQuantity = 0;
	UPROPERTY(Replicated) bool bServerOutcomePublished = false;

	TWeakObjectPtr<class AKalmalaCharacter> HelmCharacter;
	TWeakObjectPtr<class AKalmalaCharacter> PassengerCharacter;
	float StartedAtSeconds = -1.0f;
	bool bSetupAttempted = false;
	bool bServerFailed = false;
	bool bServerReported = false;
	bool bLocalOwnerReported = false;
};
