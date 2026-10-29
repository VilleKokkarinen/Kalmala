#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaHomeSubsystem.generated.h"

class APlayerController;
class UKalmalaCreditsWidget;
class UKalmalaHomeWidget;
class UKalmalaJoinAddressWidget;
class UKalmalaPlayerStepWidget;
class UKalmalaSaveInformationWidget;
class UKalmalaWorldListWidget;
enum class EKalmalaHomeAction : uint8;

/** Shows Home only for the dedicated menu controller and active Home state. */
UCLASS()
class KALMALAUI_API UKalmalaHomeSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UKalmalaHomeWidget* GetHomeWidget() const { return HomeWidget; }
    UKalmalaPlayerStepWidget* GetPlayerStep() const { return PlayerStep; }
    void ReturnFromOptions();
    void AbandonOptions();
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaHomeSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    void Release();
    void HandleHomeAction(EKalmalaHomeAction Action);
    void HandlePlayerBack();
    void HandlePlayerContinue();
    void HandlePlayerJoinRequested();
    void HandleJoinBackRequested();
    void HandleJoinCancelRequested();
    void HandleJoinAddressRequested(FString Address);
    void HandleSaveInformationBack();
    void HandleCreditsBack();
    void StartWatchingConnection(uint64 Token);
    void StopWatchingConnection();
    void FinishConnectionFailure(uint64 Token, const FText& Message);
    void CancelConnectionAttempt(const FText& Message);
    void ClearScheduledTravel(UWorld* OriginWorld);
    void StartWatchingLaunch(uint64 Token);
    void StopWatchingLaunch();
    void FinishLaunchFailure(uint64 Token, const FText& Message);
    void HandleWorldListBack();
    void HandleNewWorldRequested();
    void HandleReturnToWorldsRequested();
    void HandleWorldSelected(uint64 Seed);
    void HandleSoloPlayRequested(uint64 Seed, bool bConfirmedExisting);
    void HandleHostCoopRequested(uint64 Seed, bool bConfirmedExisting);
    void HandleWorldLaunchRequested(uint64 Seed, bool bConfirmedExisting, bool bListenHost);
    void ReleasePlayerStep();
    void ReleaseJoinAddressWidget();
    void ReleaseSaveInformationWidget();
    void ReleaseCreditsWidget();
    void ReleaseWorldList();
    UPROPERTY(Transient) TObjectPtr<UKalmalaHomeWidget> HomeWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaJoinAddressWidget> JoinAddressWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaPlayerStepWidget> PlayerStep;
    UPROPERTY(Transient) TObjectPtr<UKalmalaSaveInformationWidget> SaveInformationWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCreditsWidget> CreditsWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaWorldListWidget> WorldList;
    UPROPERTY(Transient) TObjectPtr<APlayerController> HomeController;
    TWeakObjectPtr<UWorld> ConnectionOriginWorld;
    TWeakObjectPtr<UWorld> LaunchOriginWorld;
    FDelegateHandle NetworkFailureHandle;
    FDelegateHandle TravelFailureHandle;
    FDelegateHandle LaunchTravelFailureHandle;
    uint64 ActiveConnectToken = 0;
    double ConnectionStartedAtSeconds = 0.0;
    uint64 ActiveLaunchToken = 0;
    double LaunchStartedAtSeconds = 0.0;
    bool bLaunchIsListenHost = false;
    bool bLaunchDelayWarningShown = false;
};
