#include "KalmalaCampfire.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "KalmalaCampfireWeatherResponse.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Controller.h"
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

    HearthMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HearthRing"));
    HearthMesh->SetupAttachment(RootComponent);
    HearthMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TArray<FVector> Vertices; TArray<int32> Triangles;
    TArray<FVector> Normals; TArray<FVector2D> UV; TArray<FLinearColor> Colors;
    // Original low polygon stone ring. No imported mesh or engine primitive asset.
    for (int32 Stone = 0; Stone < 8; ++Stone)
    {
        const float Angle = Stone * PI / 4;
        const FVector Center(FMath::Cos(Angle) * 35, FMath::Sin(Angle) * 35, -40);
        const FVector Points[] = { Center+FVector(-10,-10,-10), Center+FVector(10,-10,-10),
            Center+FVector(10,10,-10), Center+FVector(-10,10,-10), Center+FVector(0,0,12) };
        const int32 Faces[] = {0,4,1,1,4,2,2,4,3,3,4,0,0,1,2,0,2,3};
        for (int32 F=0; F<18; F+=3)
        {
            const FVector N = FVector::CrossProduct(Points[Faces[F+1]]-Points[Faces[F]], Points[Faces[F+2]]-Points[Faces[F]]).GetSafeNormal();
            for (int32 V=0; V<3; ++V) { Triangles.Add(Vertices.Num()); Vertices.Add(Points[Faces[F+V]]); Normals.Add(N); UV.Add(FVector2D::ZeroVector); Colors.Add(FLinearColor(.35f,.32f,.28f)); }
        }
    }
    HearthMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, {}, false);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> StoneMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    if (StoneMaterial.Succeeded()) HearthMesh->SetMaterial(0, StoneMaterial.Object);
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
    return bServerAuthority && FMath::IsFinite(InFuelWetness) && InFuelWetness >= 0
        && InFuelWetness < FKalmalaCampfireWeatherResponse::ExtinguishWetness;
}

bool AKalmalaCampfire::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return HasAuthority() && IsValid(Interactor) && Interactor->HasAuthority() && CanUse(Interactor)
        && !bIsLit && FuelSeconds > 0 && IsLightingAllowed(true, FuelWetness);
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
    DOREPLIFETIME(AKalmalaCampfire, FuelSeconds);
    DOREPLIFETIME(AKalmalaCampfire, bRoofProtected);
    DOREPLIFETIME(AKalmalaCampfire, bWindProtected);
    DOREPLIFETIME(AKalmalaCampfire, bSharedUse);
}

void AKalmalaCampfire::UpdateFromServerWeather(const float DeltaSeconds)
{
    const AKalmalaWorldGenerationGameState* WorldState = GetWorld() != nullptr ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (WorldState == nullptr)
    {
        return;
    }

    const FKalmalaWeatherState& Weather = WorldState->GetWeatherState();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampfireProtection), false, this);
    FHitResult Hit;
    const FVector Start = GetActorLocation() + FVector(0,0,60);
    bRoofProtected = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + FVector(0,0,400), ECC_Visibility, Query)
        && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("KalmalaShelterRoof"));
    const FVector WindDirection = FRotator(0, Weather.WindDirectionDegrees, 0).Vector();
    bWindProtected = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - WindDirection * 400, ECC_Visibility, Query)
        && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("KalmalaShelterWindbreak"));
    AdvanceFromServer(DeltaSeconds, bRoofProtected ? 0 : Weather.PrecipitationIntensity, bWindProtected ? 0 : Weather.WindStrength);
}

void AKalmalaCampfire::AdvanceFromServer(float DeltaSeconds, float Rain, float Wind)
{
    if (!HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0
        || !FMath::IsFinite(Rain) || !FMath::IsFinite(Wind)) return;
    if (bIsLit) FuelSeconds = FMath::Max(0.0f, FuelSeconds - DeltaSeconds);
    FuelWetness = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(FuelWetness, Rain, Wind, DeltaSeconds, bIsLit);
    bIsLit = FuelSeconds > 0 && FKalmalaCampfireWeatherResponse::ShouldRemainLit(bIsLit, FuelWetness);
    EffectiveWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(bIsLit, FuelWetness, Rain, Wind);
    ApplyReplicatedState();
    ForceNetUpdate();
}

bool AKalmalaCampfire::CanUse(const AKalmalaCharacter* Character) const
{
    return IsValid(Character) && Character->GetWorld() == GetWorld() && Character->GetController()
        && (bSharedUse || GetOwner() == Character->GetController())
        && !Character->GetActorLocation().ContainsNaN() && !GetActorLocation().ContainsNaN()
        && FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) <= FMath::Square(KalmalaCampfire::InteractionRange);
}

void AKalmalaCampfire::InitializePaidFromServer(AKalmalaCharacter* Character)
{
    if (!HasAuthority() || !IsValid(Character) || !Character->HasAuthority() || !Character->GetController()
        || Character->GetWorld()!=GetWorld() || GetOwner() || FuelSeconds > 0) return;
    SetOwner(Character->GetController());
    FuelSeconds = FuelSecondsPerBundle;
    ForceNetUpdate();
}

bool AKalmalaCampfire::TryRefuelFromServer(AKalmalaCharacter* Character)
{
    if (!HasAuthority() || !IsValid(Character) || !Character->HasAuthority() || !CanUse(Character)
        || FuelSeconds > MaxFuelSeconds - FuelSecondsPerBundle) return false;
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!Inventory || !Inventory->TryConsumeFromServer(TEXT("Fuel"), 1)) return false;
    FuelSeconds += FuelSecondsPerBundle; // Fresh fuel never erases the existing wetness.
    ForceNetUpdate(); return true;
}

void AKalmalaCampfire::SetSharedFromServer(bool bShared)
{
    if (HasAuthority()) { bSharedUse = bShared; ForceNetUpdate(); }
}

FString AKalmalaCampfire::GetStatusText() const
{
    // This is deliberately a complete textual state, rather than a light/colour-only
    // indication. The crafting panel is a local read-only view of replicated hearth state.
    return FString::Printf(TEXT("Hearth status\nState: %s\nFuel: %.0f / 300 seconds\nFuel condition: %s (%d%% wet)\nRain protection: %s\nWind protection: %s\nAccess: %s"),
        bIsLit ? TEXT("LIT") : TEXT("EXTINGUISHED"), FuelSeconds,
        FuelWetness >= .9f ? TEXT("TOO WET TO LIGHT") : TEXT("DRY ENOUGH TO LIGHT"), FMath::RoundToInt(FuelWetness*100),
        bRoofProtected ? TEXT("ROOF PROTECTED") : TEXT("RAIN EXPOSED"),
        bWindProtected ? TEXT("WINDBREAK PROTECTED") : TEXT("WIND EXPOSED"), bSharedUse ? TEXT("SHARED") : TEXT("OWNER ONLY"));
}

void AKalmalaCampfire::OnRep_CampfireState()
{
    ApplyReplicatedState();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest")))
        UE_LOG(LogTemp, Display, TEXT("Crafting fire client: Name=%s Fuel=%.0f Lit=%d Wet=%d Warmth=%.0f"),
            *GetName(), FuelSeconds, bIsLit, FMath::RoundToInt(FuelWetness*100), EffectiveWarmth);
#endif
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureReplicationTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Exposure replication test client received campfire: Lit=%d FuelWetness=%.2f EffectiveWarmth=%.2f."), bIsLit, FuelWetness, EffectiveWarmth);
    }
}

void AKalmalaCampfire::ApplyReplicatedState()
{
    FireLight->SetIntensity(bIsLit ? KalmalaCampfire::LightIntensity * EffectiveWarmth : 0.0f);
}
