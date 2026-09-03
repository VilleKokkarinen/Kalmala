#include "KalmalaWorldGenerationGameState.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

AKalmalaWorldGenerationGameState::AKalmalaWorldGenerationGameState()
{
    WorldGenerationConfig.WorldSeed = 10323456789ull;
    WorldGenerationConfig.GeneratorRevision = 1;
}

void AKalmalaWorldGenerationGameState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!HasAuthority())
    {
        return;
    }

    FParse::Value(FCommandLine::Get(), TEXT("WorldSeed="), WorldGenerationConfig.WorldSeed);
    FParse::Value(FCommandLine::Get(), TEXT("GeneratorRevision="), WorldGenerationConfig.GeneratorRevision);

    if (!WorldGenerationConfig.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid GeneratorRevision %d; falling back to revision 1."), WorldGenerationConfig.GeneratorRevision);
        WorldGenerationConfig.GeneratorRevision = 1;
    }

    ForceNetUpdate();
    LogWorldGenerationIdentity(TEXT("Server selected"));
}

void AKalmalaWorldGenerationGameState::OnRep_WorldGenerationConfig()
{
    LogWorldGenerationIdentity(TEXT("Client received"));
}

void AKalmalaWorldGenerationGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWorldGenerationGameState, WorldGenerationConfig);
}

void AKalmalaWorldGenerationGameState::LogWorldGenerationIdentity(const TCHAR* Source) const
{
    UE_LOG(LogTemp, Display, TEXT("%s world-generation identity: Seed=%llu Revision=%d."), Source, WorldGenerationConfig.WorldSeed, WorldGenerationConfig.GeneratorRevision);
}
