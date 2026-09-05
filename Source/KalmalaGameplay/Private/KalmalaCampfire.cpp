#include "KalmalaCampfire.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "KalmalaCampfireWeatherResponse.h"
#include "KalmalaCharacter.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

namespace KalmalaCampfire
{
    constexpr float InteractionRange = 250.0f;
    constexpr float WarmthRadius = 600.0f;
    constexpr float LightIntensity = 2500.0f;
}

AKalmalaCampfire::AKalmalaCampfire()
{
    bReplicates = true;
    SetReplicateMovement(false);
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(55.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;

    FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
    FireLight->SetupAttachment(RootComponent);
    FireLight->SetLightColor(FLinearColor(1.0f, 0.28f, 0.06f));
    FireLight->SetAttenuationRadius(KalmalaCampfire::WarmthRadius);
    FireLight->SetIntensity(0.0f);
}

void AKalmalaCampfire::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority())
    {
        UpdateFromServerWeather(DeltaSeconds);
    }
}

bool AKalmalaCampfire::IsLightingAllowed(const bool bServerAuthority, const float InFuelWetness)
{
    return bServerAuthority && FMath::Clamp(InFuelWetness, 0.0f, 1.0f) < FKalmalaCampfireWeatherResponse::ExtinguishWetness;
}

bool AKalmalaCampfire::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return IsValid(Interactor) && !bIsLit && IsLightingAllowed(HasAuthority(), FuelWetness)
        && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(KalmalaCampfire::InteractionRange);
}

void AKalmalaCampfire::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (CanInteract_Implementation(Interactor))
    {
        bIsLit = true;
        UpdateFromServerWeather(0.0f);
        ForceNetUpdate();
    }
}

float AKalmalaCampfire::GetWarmthContributionAt(const FVector& Location) const
{
    const float Distance = FVector::Dist(Location, GetActorLocation());
    return EffectiveWarmth * FMath::Clamp(1.0f - Distance / KalmalaCampfire::WarmthRadius, 0.0f, 1.0f);
}

void AKalmalaCampfire::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaCampfire, bIsLit);
    DOREPLIFETIME(AKalmalaCampfire, FuelWetness);
    DOREPLIFETIME(AKalmalaCampfire, EffectiveWarmth);
}

void AKalmalaCampfire::UpdateFromServerWeather(const float DeltaSeconds)
{
    const AKalmalaWorldGenerationGameState* WorldState = GetWorld() != nullptr ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (WorldState == nullptr)
    {
        return;
    }

    const FKalmalaWeatherState& Weather = WorldState->GetWeatherState();
    FuelWetness = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(FuelWetness, Weather.PrecipitationIntensity, Weather.WindStrength, DeltaSeconds, bIsLit);
    bIsLit = FKalmalaCampfireWeatherResponse::ShouldRemainLit(bIsLit, FuelWetness);
    EffectiveWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(bIsLit, FuelWetness, Weather.PrecipitationIntensity, Weather.WindStrength);
    ApplyReplicatedState();
}

void AKalmalaCampfire::OnRep_CampfireState()
{
    ApplyReplicatedState();
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureReplicationTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Exposure replication test client received campfire: Lit=%d FuelWetness=%.2f EffectiveWarmth=%.2f."), bIsLit, FuelWetness, EffectiveWarmth);
    }
}

void AKalmalaCampfire::ApplyReplicatedState()
{
    FireLight->SetIntensity(bIsLit ? KalmalaCampfire::LightIntensity * EffectiveWarmth : 0.0f);
}
