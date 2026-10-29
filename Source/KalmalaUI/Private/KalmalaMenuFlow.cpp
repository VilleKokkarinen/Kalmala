#include "KalmalaMenuFlow.h"

bool FKalmalaMenuFlow::Activate()
{
    if (Screen != EKalmalaMenuScreen::Inactive || Phase != EPhase::None) return false;
    Screen = EKalmalaMenuScreen::Home;
    return true;
}

bool FKalmalaMenuFlow::Navigate(const EKalmalaMenuScreen Destination)
{
    if (Phase != EPhase::None || Screen == EKalmalaMenuScreen::Inactive
        || Screen == EKalmalaMenuScreen::Gameplay) return false;
    switch (Destination)
    {
    case EKalmalaMenuScreen::Home: case EKalmalaMenuScreen::Player:
    case EKalmalaMenuScreen::Worlds: case EKalmalaMenuScreen::NewWorld:
    case EKalmalaMenuScreen::WorldDetails: case EKalmalaMenuScreen::Join:
    case EKalmalaMenuScreen::Options: case EKalmalaMenuScreen::SaveInformation:
    case EKalmalaMenuScreen::Credits: case EKalmalaMenuScreen::ExitConfirmation:
        Screen = Destination;
        return true;
    default:
        return false;
    }
}

bool FKalmalaMenuFlow::TryBegin(const EKalmalaMenuRequest Request, uint64& OutToken)
{
    OutToken = 0;
    if (Phase != EPhase::None || LastToken == MAX_uint64) return false;
    if ((Request == EKalmalaMenuRequest::Launch && Screen != EKalmalaMenuScreen::WorldDetails)
        || (Request == EKalmalaMenuRequest::Connect && Screen != EKalmalaMenuScreen::Join)
        || (Request != EKalmalaMenuRequest::Launch && Request != EKalmalaMenuRequest::Connect)) return false;
    Origin = Screen;
    ActiveRequest = Request;
    Screen = Request == EKalmalaMenuRequest::Launch ? EKalmalaMenuScreen::PreparingWorld : EKalmalaMenuScreen::Connecting;
    ActiveToken = ++LastToken;
    Phase = EPhase::Pending;
    OutToken = ActiveToken;
    return true;
}

bool FKalmalaMenuFlow::Matches(const uint64 Token) const
{
    return Token != 0 && Token == ActiveToken && Phase != EPhase::None;
}

void FKalmalaMenuFlow::ClearRequest()
{
    ActiveToken = 0;
    Origin = EKalmalaMenuScreen::Inactive;
    ActiveRequest = EKalmalaMenuRequest::Launch;
    Phase = EPhase::None;
}

bool FKalmalaMenuFlow::Cancel(const uint64 Token)
{
    if (!Matches(Token) || Phase != EPhase::Pending) return false;
    Screen = Origin;
    ClearRequest();
    return true;
}

bool FKalmalaMenuFlow::CancelConnect(const uint64 Token)
{
    if (!Matches(Token) || ActiveRequest != EKalmalaMenuRequest::Connect || Phase != EPhase::Traveling) return false;
    Screen = Origin;
    ClearRequest();
    return true;
}

bool FKalmalaMenuFlow::TryCommitTravel(const uint64 Token, TFunctionRef<void()> StartTravel)
{
    if (!Matches(Token) || Phase != EPhase::Pending) return false;
    Phase = EPhase::Traveling;
    StartTravel();
    return true;
}

bool FKalmalaMenuFlow::CompleteSuccess(const uint64 Token)
{
    if (!Matches(Token) || Phase != EPhase::Traveling) return false;
    Screen = EKalmalaMenuScreen::Gameplay;
    ClearRequest();
    return true;
}

bool FKalmalaMenuFlow::CompleteFailure(const uint64 Token)
{
    if (!Matches(Token)) return false;
    Screen = Origin;
    ClearRequest();
    return true;
}

bool FKalmalaMenuFlow::ReturnFromGameplay()
{
    if (Screen != EKalmalaMenuScreen::Gameplay || Phase != EPhase::None) return false;
    Screen = EKalmalaMenuScreen::Home;
    return true;
}

void FKalmalaMenuFlow::Shutdown()
{
    ClearRequest();
    Screen = EKalmalaMenuScreen::Inactive;
}
