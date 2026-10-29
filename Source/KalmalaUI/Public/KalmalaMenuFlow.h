#pragma once

#include "CoreMinimal.h"

// Front-end only. No screen is opened until the menu explicitly activates the flow.
enum class EKalmalaMenuScreen : uint8
{
    Inactive, Home, Player, Worlds, NewWorld, WorldDetails, Join, Options,
    SaveInformation, Credits, ExitConfirmation, PreparingWorld, Connecting, Gameplay
};

enum class EKalmalaMenuRequest : uint8 { Launch, Connect };

/** One local player's front-end state and one-shot travel gate. Game-thread only. */
class KALMALAUI_API FKalmalaMenuFlow
{
public:
    FKalmalaMenuFlow() = default;
    FKalmalaMenuFlow(const FKalmalaMenuFlow&) = delete;
    FKalmalaMenuFlow& operator=(const FKalmalaMenuFlow&) = delete;

    EKalmalaMenuScreen GetScreen() const { return Screen; }
    bool Activate();
    bool Navigate(EKalmalaMenuScreen Destination);
    bool TryBegin(EKalmalaMenuRequest Request, uint64& OutToken);
    bool Cancel(uint64 Token);
    bool CancelConnect(uint64 Token);
    // Validates and marks travel committed before invoking StartTravel, including under reentrancy.
    bool TryCommitTravel(uint64 Token, TFunctionRef<void()> StartTravel);
    bool CompleteSuccess(uint64 Token);
    bool CompleteFailure(uint64 Token);
    // Called during owner teardown; never resets the token counter.
    void Shutdown();

private:
    enum class EPhase : uint8 { None, Pending, Traveling };
    bool Matches(uint64 Token) const;
    void ClearRequest();

    EKalmalaMenuScreen Screen = EKalmalaMenuScreen::Inactive;
    EKalmalaMenuScreen Origin = EKalmalaMenuScreen::Inactive;
    EKalmalaMenuRequest ActiveRequest = EKalmalaMenuRequest::Launch;
    EPhase Phase = EPhase::None;
    uint64 LastToken = 0;
    uint64 ActiveToken = 0;
};
