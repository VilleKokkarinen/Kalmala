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
	void DriveReconnectPeerTest(const FString& Phase);
	bool PrepareReconnectScenario(bool bResume);
	void VerifyReconnectLateJoin(bool bResume);
	void VerifyReconnectLocalReplica(const FString& Phase);
	bool FindReconnectDiscovery(struct FKalmalaOceanDiscoveryDescriptor& OutDescriptor) const;
	bool HasSingleReconnectSkiff() const;
	void FailReconnect(const TCHAR* Reason);
	bool PrepareServerScenario();
	void VerifyServerOutcome();
	void ReportPeerNetworkProfile();
	void VerifyLocalOwnerReplica();
	void Fail(const TCHAR* Reason);

	UPROPERTY(Replicated) TObjectPtr<class AKalmalaOceanSkiff> TestSkiff;
	UPROPERTY(Replicated) TObjectPtr<class APlayerState> HelmPlayerState;
	UPROPERTY(Replicated) TObjectPtr<class APlayerState> PassengerPlayerState;
	UPROPERTY(Replicated) FName ExpectedDiscoveryId = NAME_None;
	UPROPERTY(Replicated) FName ExpectedRewardItemId = NAME_None;
	UPROPERTY(Replicated) int32 ExpectedRewardQuantity = 0;
	UPROPERTY(Replicated) bool bServerOutcomePublished = false;
	UPROPERTY(Replicated) bool bLateJoinOutcomePublished = false;
	UPROPERTY(Replicated) uint32 HostIdentityHash = 0;
	UPROPERTY(Replicated) uint32 HelmIdentityHash = 0;
	UPROPERTY(Replicated) uint32 LateJoinIdentityHash = 0;

	TWeakObjectPtr<class AKalmalaCharacter> HelmCharacter;
	TWeakObjectPtr<class AKalmalaCharacter> PassengerCharacter;
	TWeakObjectPtr<class UNetConnection> ProfiledRemoteConnection;
	float StartedAtSeconds = -1.0f;
	float NetworkProfileStartSeconds = -1.0f;
	float NetworkProfileReportSeconds = -1.0f;
	int32 NetworkProfileStartInBytes = 0;
	int32 NetworkProfileStartOutBytes = 0;
	int32 NetworkProfileStartInPackets = 0;
	int32 NetworkProfileStartOutPackets = 0;
	int32 ReconnectStage = 0;
	bool bReconnectLocalReported = false;
	bool bSetupAttempted = false;
	bool bServerFailed = false;
	bool bServerReported = false;
	bool bNetworkProfileReported = false;
	bool bLocalOwnerReported = false;
};
