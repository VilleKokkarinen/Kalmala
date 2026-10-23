#include "KalmalaWildlifeSpawn.h"
#include "KalmalaResistanceComponent.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWolfPackContract.h"
#include "KalmalaTerrainHeightSampler.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"

namespace
{
    FLinearColor BlendEcologyAccent(const FLinearColor Base, const FLinearColor Accent)
    {
        constexpr float Weight = 0.24f;
        return FLinearColor(
            FMath::Lerp(Base.R, Accent.R, Weight),
            FMath::Lerp(Base.G, Accent.G, Weight),
            FMath::Lerp(Base.B, Accent.B, Weight),
            Base.A);
    }

    FLinearColor GetEcologyAccent(const EKalmalaWildlifeEcology Ecology)
    {
        switch (Ecology)
        {
        case EKalmalaWildlifeEcology::OpenGrazer: return FLinearColor(0.66f, 0.78f, 0.38f);
        case EKalmalaWildlifeEcology::ShoreForager: return FLinearColor(0.30f, 0.70f, 0.72f);
        case EKalmalaWildlifeEcology::CanopyBrowser: return FLinearColor(0.30f, 0.55f, 0.30f);
        case EKalmalaWildlifeEcology::HummockScavenger: return FLinearColor(0.52f, 0.38f, 0.22f);
        case EKalmalaWildlifeEcology::WindGrazer: return FLinearColor(0.70f, 0.76f, 0.86f);
        case EKalmalaWildlifeEcology::RidgeForager: return FLinearColor(0.52f, 0.58f, 0.66f);
        case EKalmalaWildlifeEcology::Generalist:
        default: return FLinearColor(0.45f, 0.45f, 0.45f);
        }
    }
    struct FWildlifeStatProfile
    {
        float Health = 0.0f;
        float Armor = 0.0f;
        FKalmalaDamageChannels Attack;
    };

    bool TryGetWildlifeStatProfile(const EKalmalaWildlifeArchetype Archetype, FWildlifeStatProfile& OutProfile)
    {
        OutProfile = FWildlifeStatProfile{};
        switch (Archetype)
        {
        case EKalmalaWildlifeArchetype::Boar:
            OutProfile.Health = 20.0f;
            OutProfile.Armor = 5.0f;
            OutProfile.Attack.Crush = 10.0f;
            return true;
        case EKalmalaWildlifeArchetype::Deer:
            OutProfile.Health = 30.0f;
            OutProfile.Armor = 5.0f;
            return true;
        case EKalmalaWildlifeArchetype::Mireling:
            OutProfile.Health = 75.0f;
            OutProfile.Armor = 15.0f;
            OutProfile.Attack.Crush = 20.0f;
            return true;
        case EKalmalaWildlifeArchetype::Rat:
            OutProfile.Health = 10.0f;
            OutProfile.Armor = 0.0f;
            OutProfile.Attack.Pierce = 5.0f;
            return true;
        case EKalmalaWildlifeArchetype::Badger:
            OutProfile.Health = 100.0f;
            OutProfile.Armor = 10.0f;
            OutProfile.Attack.Slash = 20.0f;
            return true;
        case EKalmalaWildlifeArchetype::MoonBear:
            OutProfile.Health = 250.0f;
            OutProfile.Armor = 20.0f;
            OutProfile.Attack.Crush = 75.0f;
            return true;
        case EKalmalaWildlifeArchetype::DeathAdder:
            OutProfile.Health = 125.0f;
            OutProfile.Armor = 20.0f;
            OutProfile.Attack.Stab = 75.0f;
            return true;
        case EKalmalaWildlifeArchetype::Viper:
            OutProfile.Health = 100.0f;
            OutProfile.Armor = 10.0f;
            OutProfile.Attack.Stab = 50.0f;
            return true;
        case EKalmalaWildlifeArchetype::GiantSnake:
            OutProfile.Health = 250.0f;
            OutProfile.Armor = 30.0f;
            OutProfile.Attack.Stab = 100.0f;
            return true;
        case EKalmalaWildlifeArchetype::MosquitoSwarm:
            OutProfile.Health = 25.0f;
            OutProfile.Armor = 0.0f;
            OutProfile.Attack.Stab = 5.0f;
            return true;
        case EKalmalaWildlifeArchetype::PeatHusk:
            OutProfile.Health = 300.0f;
            OutProfile.Armor = 50.0f;
            OutProfile.Attack.Crush = 150.0f;
            return true;
        case EKalmalaWildlifeArchetype::Wolf:
            OutProfile.Health = 250.0f;
            OutProfile.Armor = 40.0f;
            OutProfile.Attack.Slash = 100.0f;
            return true;
        case EKalmalaWildlifeArchetype::Lynx:
            OutProfile.Health = 150.0f;
            OutProfile.Armor = 30.0f;
            OutProfile.Attack.Slash = 75.0f;
            return true;
        case EKalmalaWildlifeArchetype::PolarBear:
            OutProfile.Health = 750.0f;
            OutProfile.Armor = 60.0f;
            OutProfile.Attack.Crush = 200.0f;
            return true;
        case EKalmalaWildlifeArchetype::HornedGoat:
            OutProfile.Health = 200.0f;
            OutProfile.Armor = 50.0f;
            OutProfile.Attack.Crush = 50.0f;
            return true;
        case EKalmalaWildlifeArchetype::ThunderSentinel:
            OutProfile.Health = 1250.0f;
            OutProfile.Armor = 75.0f;
            OutProfile.Attack.Crush = 300.0f;
            return true;
        case EKalmalaWildlifeArchetype::ReefMaw:
            OutProfile.Health = 180.0f;
            OutProfile.Armor = 2.0f;
            OutProfile.Attack.Stab = 50.0f;
            return true;
        default:
            return false;
        }
    }
}

AKalmalaWildlifeSpawn::AKalmalaWildlifeSpawn()
{
    CreateDefaultSubobject<UKalmalaResistanceComponent>(TEXT("Resistances"));
    PrimaryActorTick.bCanEverTick = true;
    USphereComponent* Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(70.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    RootComponent = Collision;
    MirelingMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MirelingMesh"));
    MirelingMesh->SetupAttachment(RootComponent);
    MirelingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SnakeBodyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("SnakeBodyCollision"));
    SnakeBodyCollision->SetupAttachment(RootComponent);
    SnakeBodyCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SnakeBodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    SnakeBodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    SnakeBodyCollision->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    BuildArchetypePresentation();
    bReplicates = true;
    SetReplicateMovement(true);
}

void AKalmalaWildlifeSpawn::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    InitializeServer(Spawn, GetArchetypeForSpawnSeed(Spawn.SpawnSeed));
}

void AKalmalaWildlifeSpawn::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn,
    const EKalmalaWildlifeArchetype InitialArchetype, const int32 WolfPackMemberIndex)
{
    if (HasAuthority())
    {
        FangArrowPoisonRemainingSeconds = 0.0f;
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
        SetActorLocation(Spawn.Location);
        SpawnOrigin = Spawn.Location;
        BehaviourDestination = SpawnOrigin;
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        CreatureNicheId = Spawn.ContentId;
        Archetype = InitialArchetype;
        if (Archetype == EKalmalaWildlifeArchetype::ReefMaw)
        {
            FVector SwimLocation = Spawn.Location;
            SwimLocation.Z = FKalmalaTerrainHeightSampler::SeaLevelWorldHeight - ReefMawSpawnDepthCentimetres;
            SetActorLocation(SwimLocation);
            SpawnOrigin = SwimLocation;
            BehaviourDestination = SpawnOrigin;
        }
        if (Archetype == EKalmalaWildlifeArchetype::Wolf)
        {
            PersistentSpawnId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(Spawn, WolfPackMemberIndex);
            if (PersistentSpawnId.IsEmpty()) return;
        }
        SnakeAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        SnakeAttackTarget.Reset();
        SnakeAttackSecondsRemaining = 0.0f;
        SnakeLungeDistanceRemaining = 0.0f;
        RatAttackCue = static_cast<uint8>(EKalmalaRatAttackCue::None);
        RatAttackTarget.Reset();
        RatAttackSecondsRemaining = 0.0f;
        BadgerAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        BadgerAttackTarget.Reset();
        BadgerAttackSecondsRemaining = 0.0f;
        MoonBearAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        MoonBearAttackTarget.Reset();
        MoonBearAttackSecondsRemaining = 0.0f;
        HornedGoatAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        HornedGoatAttackTarget.Reset();
        HornedGoatAttackSecondsRemaining = 0.0f;
        ThunderSentinelAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        ThunderSentinelAttackTarget.Reset();
        ThunderSentinelAttackSecondsRemaining = 0.0f;
        ThunderSentinelStepDistanceRemaining = 0.0f;
        bThunderSentinelSlamHitApplied = false;
        ReefMawAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        ReefMawAttackTarget.Reset();
        ReefMawAttackSecondsRemaining = 0.0f;
        bReefMawBiteHitApplied = false;
        WolfAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        WolfAttackTarget.Reset();
        WolfAttackSecondsRemaining = 0.0f;
        WolfLungeDistanceRemaining = 0.0f;
        LynxAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        LynxAttackTarget.Reset();
        LynxAttackSecondsRemaining = 0.0f;
        LynxLeapDistanceRemaining = 0.0f;
        PolarBearAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);
        PolarBearAttackTarget.Reset();
        PolarBearAttackSecondsRemaining = 0.0f;
        PolarBearChargeDistanceRemaining = 0.0f;
        PolarBearRetreatDistanceRemaining = 0.0f;
        bPolarBearChargeHitApplied = false;
        Health = GetMaximumHealthForArchetype(Archetype);
        if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm)
        {
            float GroundHeight = 0.0f;
            if (IsMosquitoPositionValid(Spawn.Location, GroundHeight))
            {
                FVector FlightLocation = Spawn.Location;
                FlightLocation.Z = GroundHeight + MosquitoFlightHeightCentimetres;
                SetActorLocation(FlightLocation);
            }
            MosquitoFlightElapsedSeconds = static_cast<float>(FCrc::StrCrc32(*PersistentSpawnId) % 1000u) * 0.001f;
        }
        if (UKalmalaResistanceComponent* Defenses = FindComponentByClass<UKalmalaResistanceComponent>())
        {
            Defenses->Profile.Armor = GetArmorForArchetype(Archetype);
        }
        BuildArchetypePresentation();
        if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm) PlayMosquitoBuzz();
    }
}

float AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(const EKalmalaWildlifeArchetype InArchetype)
{
    FWildlifeStatProfile Profile;
    if (TryGetWildlifeStatProfile(InArchetype, Profile)) return Profile.Health;

    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Boar: return 20.0f;
    case EKalmalaWildlifeArchetype::Deer: return 30.0f;
    case EKalmalaWildlifeArchetype::Rat: return 10.0f;
    case EKalmalaWildlifeArchetype::Badger: return 100.0f;
    case EKalmalaWildlifeArchetype::MoonBear: return 250.0f;
    case EKalmalaWildlifeArchetype::DeathAdder: return 125.0f;
    case EKalmalaWildlifeArchetype::Viper: return 100.0f;
    case EKalmalaWildlifeArchetype::GiantSnake: return 250.0f;
    case EKalmalaWildlifeArchetype::Mireling:
    default: return 75.0f;
    }
}

float AKalmalaWildlifeSpawn::GetArmorForArchetype(const EKalmalaWildlifeArchetype InArchetype)
{
    FWildlifeStatProfile Profile;
    if (TryGetWildlifeStatProfile(InArchetype, Profile)) return Profile.Armor;

    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Boar:
    case EKalmalaWildlifeArchetype::Deer: return 5.0f;
    case EKalmalaWildlifeArchetype::Rat: return 0.0f;
    case EKalmalaWildlifeArchetype::Badger: return 10.0f;
    case EKalmalaWildlifeArchetype::MoonBear: return 20.0f;
    case EKalmalaWildlifeArchetype::DeathAdder: return 20.0f;
    case EKalmalaWildlifeArchetype::Viper: return 10.0f;
    case EKalmalaWildlifeArchetype::GiantSnake: return 30.0f;
    case EKalmalaWildlifeArchetype::Mireling:
    default: return 15.0f;
    }
}

bool AKalmalaWildlifeSpawn::IsRatBiteInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= RatBiteReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsBadgerClawInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= BadgerClawReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsWolfBiteInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= WolfBiteReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsMoonBearPawInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= MoonBearPawReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsHornedGoatHornInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= HornedGoatHornReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsPeatHuskSweepInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres, const float ForwardDot)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= PeatHuskSweepReachCentimetres
        && FMath::IsFinite(ForwardDot) && ForwardDot >= PeatHuskSweepMinimumForwardDot && ForwardDot <= 1.0f;
}

bool AKalmalaWildlifeSpawn::IsMosquitoBiteInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= MosquitoBiteReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsMosquitoFireRepelling(const bool bServerAuthority, const bool bActiveFlame,
    const float DistanceCentimetres, const float RadiusCentimetres)
{
    return bServerAuthority && bActiveFlame && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && FMath::IsFinite(RadiusCentimetres)
        && RadiusCentimetres > 0.0f && DistanceCentimetres <= RadiusCentimetres;
}

float AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(const EKalmalaWildlifeArchetype InArchetype)
{
    return GetBaseAttackChannelsForArchetype(InArchetype).Total();
}

FKalmalaDamageChannels AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(const EKalmalaWildlifeArchetype InArchetype)
{
    FWildlifeStatProfile Profile;
    if (TryGetWildlifeStatProfile(InArchetype, Profile)) return Profile.Attack;

    FKalmalaDamageChannels Damage;
    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Boar: Damage.Crush = 10.0f; break;
    case EKalmalaWildlifeArchetype::Mireling: Damage.Crush = 20.0f; break;
    case EKalmalaWildlifeArchetype::Rat: Damage.Pierce = 5.0f; break;
    case EKalmalaWildlifeArchetype::Badger: Damage.Slash = 20.0f; break;
    case EKalmalaWildlifeArchetype::MoonBear: Damage.Crush = 75.0f; break;
    case EKalmalaWildlifeArchetype::HornedGoat: Damage.Crush = 50.0f; break;
    case EKalmalaWildlifeArchetype::DeathAdder: Damage.Stab = 75.0f; break;
    case EKalmalaWildlifeArchetype::Viper: Damage.Stab = 50.0f; break;
    case EKalmalaWildlifeArchetype::GiantSnake: Damage.Stab = 100.0f; break;
    case EKalmalaWildlifeArchetype::Deer:
    default: break;
    }
    return Damage;
}

int32 AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(const EKalmalaWildlifeArchetype InArchetype)
{
    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Boar:
    case EKalmalaWildlifeArchetype::Deer:
    case EKalmalaWildlifeArchetype::Badger:
    case EKalmalaWildlifeArchetype::Rat:
    case EKalmalaWildlifeArchetype::DeathAdder:
    case EKalmalaWildlifeArchetype::Viper: return 1;
    case EKalmalaWildlifeArchetype::GiantSnake: return 2;
    case EKalmalaWildlifeArchetype::MoonBear: return 3;
    case EKalmalaWildlifeArchetype::PolarBear: return 3;
    case EKalmalaWildlifeArchetype::ReefMaw: return 3;
    case EKalmalaWildlifeArchetype::HornedGoat: return 2;
    case EKalmalaWildlifeArchetype::Wolf: return 2;
    case EKalmalaWildlifeArchetype::Lynx: return 2;
    default: return 0;
    }
}

int32 AKalmalaWildlifeSpawn::GetStoneRewardQuantityForArchetype(
    const EKalmalaWildlifeArchetype InArchetype)
{
    return InArchetype == EKalmalaWildlifeArchetype::ThunderSentinel
        ? ThunderSentinelStoneRewardQuantity : 0;
}

bool AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(const bool bHardEnemy,
    const bool bHasGeneratedPlayerStart, const float DistanceCentimetres)
{
    if (!bHardEnemy || !bHasGeneratedPlayerStart) return true;
    return FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= HardEnemyStartExclusionCentimetres;
}

bool AKalmalaWildlifeSpawn::IsDefeatAllowed(const bool bServerAuthority, const bool bAlreadyDefeated)
{
    return bServerAuthority && !bAlreadyDefeated;
}

bool AKalmalaWildlifeSpawn::IsDeerSinewDropSuccessful(const float ServerRoll)
{
    return FMath::IsFinite(ServerRoll) && ServerRoll >= 0.0f && ServerRoll < 0.5f;
}

bool AKalmalaWildlifeSpawn::DefeatServer()
{
    if (!IsDefeatAllowed(HasAuthority(), bDefeated))
    {
        return false;
    }

    if (Archetype == EKalmalaWildlifeArchetype::Rat)
    {
        if (AKalmalaGameMode* Mode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr)
        {
            if (!Mode->RecordRatDefeatReward(PersistentSpawnId, LastValidatedAttacker.Get())) return false;
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Badger)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordBadgerDefeatReward(PersistentSpawnId, LastValidatedAttacker.Get()))
            return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::MoonBear)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordMoonBearDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Wolf)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordWolfDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Lynx)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordLynxDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PolarBear)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordPolarBearDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::HornedGoat)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordHornedGoatDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::ThunderSentinel)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordThunderSentinelDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::ReefMaw)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordReefMawDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PeatHusk)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordPeatHuskDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Viper)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordViperDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::GiantSnake)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordGiantSnakeDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::DeathAdder)
    {
        AKalmalaGameMode* Mode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (Mode == nullptr || !Mode->RecordDeathAdderDefeatReward(
                PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }

    FangArrowPoisonRemainingSeconds = 0.0f;
    FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
    bDefeated = true;
    // The population owner records the sparse server delta before this actor exposes any owner-only reward.
    OnDefeated.Broadcast(PersistentSpawnId);
    ApplyDefeatedState();
    ForceNetUpdate();
    return true;
}

bool AKalmalaWildlifeSpawn::ApplyCombatDamageFromServer(const float Damage, AKalmalaCharacter* Attacker)
{
    if (!FMath::IsFinite(Damage) || Damage <= 0.0f || Damage > 100.0f) return false;
    FKalmalaDamageChannels Channels;
    Channels.Crush = Damage;
    return ApplyCombatDamageFromServer(Channels, Attacker);
}

bool AKalmalaWildlifeSpawn::ApplyCombatDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker)
{ return ApplyValidatedDamageFromServer(Damage, Attacker, true); }

bool AKalmalaWildlifeSpawn::ApplyArrowDamageFromServer(const FKalmalaDamageChannels& Damage,
    AKalmalaCharacter* Attacker, const FName ArrowItemId)
{
    if (!Attacker || ArrowItemId.IsNone()) return false;
    const bool bDamageAccepted = ApplyValidatedDamageFromServer(Damage, Attacker, false);
    if (bDamageAccepted && ArrowItemId == TEXT("FangArrows")) ApplyFangArrowWeakPoisonFromServer();
    return bDamageAccepted;
}

void AKalmalaWildlifeSpawn::ApplyFangArrowWeakPoisonFromServer()
{
    if (!HasAuthority() || bDefeated || !FMath::IsFinite(Health) || Health <= 0.0f) return;
    const bool bPoisonWasActive = FMath::IsFinite(FangArrowPoisonRemainingSeconds)
        && FangArrowPoisonRemainingSeconds > 0.0f;
    if (bPoisonWasActive)
    {
        FangArrowPoisonRemainingSeconds = FMath::Max(FangArrowPoisonRemainingSeconds,
            FangArrowWeakPoisonDurationSeconds);
        return;
    }
    FangArrowPoisonRemainingSeconds = FangArrowWeakPoisonDurationSeconds;
    FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
}

void AKalmalaWildlifeSpawn::AdvanceFangArrowWeakPoisonFromServer(const float DeltaSeconds)
{
    if (!HasAuthority()) return;
    if (!FMath::IsFinite(FangArrowPoisonRemainingSeconds) || FangArrowPoisonRemainingSeconds <= 0.0f)
    {
        FangArrowPoisonRemainingSeconds = 0.0f;
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
        return;
    }
    if (bDefeated || !FMath::IsFinite(Health) || Health <= 0.0f)
    {
        FangArrowPoisonRemainingSeconds = 0.0f;
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
        return;
    }

    const float SafeDelta = FMath::Max(0.0f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f);
    if (SafeDelta <= 0.0f) return;
    const float TimeToTick = FMath::Clamp(FangArrowPoisonSecondsUntilTick,
        0.0f, UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds);
    if (TimeToTick <= SafeDelta + KINDA_SMALL_NUMBER)
    {
        if (FangArrowPoisonRemainingSeconds + KINDA_SMALL_NUMBER >= TimeToTick)
        {
            FKalmalaDamageChannels PoisonDamage;
            PoisonDamage.Poison = UKalmalaPlayerStatusComponent::WeakPoisonDamagePerSecond;
            // The previous accepted arrow hit retains reward credit for a poison defeat.
            ApplyValidatedDamageFromServer(PoisonDamage, nullptr, false);
        }
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
    }
    else
    {
        FangArrowPoisonSecondsUntilTick = FMath::Max(0.0f, TimeToTick - SafeDelta);
    }

    if (bDefeated || Health <= 0.0f)
    {
        FangArrowPoisonRemainingSeconds = 0.0f;
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
        return;
    }
    FangArrowPoisonRemainingSeconds = FMath::Max(0.0f, FangArrowPoisonRemainingSeconds - SafeDelta);
    if (FangArrowPoisonRemainingSeconds <= 0.0f)
        FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
}

bool AKalmalaWildlifeSpawn::ApplyValidatedDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker, bool bMelee)
{
    if (!HasAuthority() || bDefeated || !Damage.IsValid() || Damage.Total() <= 0.0f) return false;
    if (Attacker != nullptr && (!Attacker->HasAuthority() || Attacker->GetWorld() != GetWorld()
        || (bMelee && FVector::DistSquared(Attacker->GetActorLocation(), GetActorLocation()) > FMath::Square(220.0f)))) return false;
    const UKalmalaResistanceComponent* Defenses = FindComponentByClass<UKalmalaResistanceComponent>();
    float ResolvedDamage = 0.0f;
    if (!Defenses || !Defenses->ResolveFromServer(Damage, ResolvedDamage)) return false;
    if (Attacker != nullptr) LastValidatedAttacker = Attacker;
    Health = FMath::Clamp(Health - ResolvedDamage, 0.0f, GetMaximumHealthForArchetype(Archetype));
    if (Health <= 0.0f) return DefeatServer();
    if (Archetype == EKalmalaWildlifeArchetype::Viper || Archetype == EKalmalaWildlifeArchetype::GiantSnake)
    {
        SnakeAttackTarget.Reset();
        SnakeAttackSecondsRemaining = 0.0f;
        SnakeLungeDistanceRemaining = 0.0f;
        SetSnakeAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    if (Archetype == EKalmalaWildlifeArchetype::Boar && Attacker != nullptr)
    {
        BoarChargeTarget = Attacker;
        BoarChargeSecondsRemaining = 1.25f;
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Rat && Attacker != nullptr)
    {
        RatAttackTarget = Attacker;
        RatAttackSecondsRemaining = 0.0f;
        SetRatAttackCue(EKalmalaRatAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Badger && Attacker != nullptr)
    {
        BadgerAttackTarget = Attacker;
        BadgerAttackSecondsRemaining = 0.0f;
        SetBadgerAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Wolf && Attacker != nullptr)
    {
        WolfAttackTarget = Attacker;
        WolfAttackSecondsRemaining = 0.0f;
        WolfLungeDistanceRemaining = 0.0f;
        SetWolfAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Lynx && Attacker != nullptr)
    {
        LynxAttackTarget = Attacker;
        LynxAttackSecondsRemaining = 0.0f;
        LynxLeapDistanceRemaining = 0.0f;
        SetLynxAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PolarBear && Attacker != nullptr)
    {
        PolarBearAttackTarget = Attacker;
        PolarBearAttackSecondsRemaining = 0.0f;
        PolarBearChargeDistanceRemaining = 0.0f;
        PolarBearRetreatDistanceRemaining = 0.0f;
        bPolarBearChargeHitApplied = false;
        SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::MoonBear && Attacker != nullptr)
    {
        MoonBearAttackTarget = Attacker;
        MoonBearAttackSecondsRemaining = 0.0f;
        SetMoonBearAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PeatHusk && Attacker != nullptr)
    {
        PeatHuskAttackTarget = Attacker;
        PeatHuskAttackSecondsRemaining = 0.0f;
        SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::None);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm && Attacker != nullptr)
    {
        MosquitoTarget = Attacker;
        MosquitoApproachSecondsRemaining = MosquitoApproachSeconds;
        MosquitoBiteSecondsRemaining = 0.0f;
    }
    else
    {
        BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f, SpawnOrigin + GetDeterministicOffset(GetEcologicalFleeDistance(CreatureNicheId)));
        if (Archetype == EKalmalaWildlifeArchetype::Deer)
        {
            // A successful server combat execution is the only initial noise source. Nearby deer
            // derive their own bounded flight destinations; no client can select a herd or destination.
            AlertNearbyDeerFromServer();
        }
    }
    ForceNetUpdate();
    return true;
}

bool AKalmalaWildlifeSpawn::ApplyDeerCallFromServer(const FVector& SourceLocation)
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::Deer
        || Behaviour != EKalmalaWildlifeBehaviour::Idle || !FMath::IsFinite(SourceLocation.X)
        || !FMath::IsFinite(SourceLocation.Y) || !FMath::IsFinite(SourceLocation.Z)) return false;

    const FVector AwayFromSource = (GetActorLocation() - SourceLocation).GetSafeNormal2D();
    const FVector Direction = AwayFromSource.IsNearlyZero() ? GetDeterministicOffset(1.0f).GetSafeNormal2D() : AwayFromSource;
    BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, DeerCallInfluenceDurationSeconds,
        GetActorLocation() + Direction * GetEcologicalFleeDistance(CreatureNicheId), true);
    ForceNetUpdate();
    return Behaviour == EKalmalaWildlifeBehaviour::Flee;
}

bool AKalmalaWildlifeSpawn::IsMirelingBossRewardCandidate() const
{
    return Archetype == EKalmalaWildlifeArchetype::Mireling
        && !PersistentSpawnId.IsEmpty()
        && FCrc::StrCrc32(*PersistentSpawnId) % 5u == 0u;
}

EKalmalaWildlifeEcology AKalmalaWildlifeSpawn::GetEcologyForNiche(const FName NicheId)
{
    if (NicheId == TEXT("meadows-open-grazer")) return EKalmalaWildlifeEcology::OpenGrazer;
    if (NicheId == TEXT("lakes-shore-forager")) return EKalmalaWildlifeEcology::ShoreForager;
    if (NicheId == TEXT("elderwood-canopy-browser")) return EKalmalaWildlifeEcology::CanopyBrowser;
    if (NicheId == TEXT("mire-hummock-scavenger")) return EKalmalaWildlifeEcology::HummockScavenger;
    if (NicheId == TEXT("tundra-wind-grazer")) return EKalmalaWildlifeEcology::WindGrazer;
    if (NicheId == TEXT("mountains-ridge-forager")) return EKalmalaWildlifeEcology::RidgeForager;
    return EKalmalaWildlifeEcology::Generalist;
}

float AKalmalaWildlifeSpawn::GetEcologicalFleeDistance(const FName NicheId)
{
    switch (GetEcologyForNiche(NicheId))
    {
    case EKalmalaWildlifeEcology::OpenGrazer: return 300.0f;
    case EKalmalaWildlifeEcology::ShoreForager: return 240.0f;
    case EKalmalaWildlifeEcology::CanopyBrowser: return 210.0f;
    case EKalmalaWildlifeEcology::HummockScavenger: return 180.0f;
    case EKalmalaWildlifeEcology::WindGrazer: return 300.0f;
    case EKalmalaWildlifeEcology::RidgeForager: return 260.0f;
    case EKalmalaWildlifeEcology::Generalist:
    default: return 220.0f;
    }
}

EKalmalaWildlifeArchetype AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(const uint64 SpawnSeed)
{
    // A stable server descriptor seed selects the archetype; it never depends on actor order or a client observation.
    if (SpawnSeed % 3ull == 0ull) return EKalmalaWildlifeArchetype::Boar;
    if (SpawnSeed % 5ull == 1ull) return EKalmalaWildlifeArchetype::Deer;
    return EKalmalaWildlifeArchetype::Mireling;
}

bool AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(const EKalmalaBiome Biome, const bool bIsWater)
{
    return Biome == EKalmalaBiome::MossyMire && !bIsWater;
}

bool AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
    const EKalmalaBiome Biome, const bool bIsWater, const bool bTraversable)
{
    return Biome == EKalmalaBiome::ThunderMountains && !bIsWater && bTraversable;
}

bool AKalmalaWildlifeSpawn::IsThunderSentinelSlamInReach(const bool bServerAuthority,
    const bool bValidTarget, const float HorizontalDistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(HorizontalDistanceCentimetres)
        && HorizontalDistanceCentimetres >= 0.0f
        && HorizontalDistanceCentimetres <= ThunderSentinelSlamReachCentimetres;
}

bool AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(const EKalmalaBiome Biome,
    const bool bLakeWater, const float WaterDepthCentimetres)
{
    return Biome == EKalmalaBiome::Ocean && !bLakeWater && FMath::IsFinite(WaterDepthCentimetres)
        && WaterDepthCentimetres >= ReefMawMinimumOceanDepthCentimetres;
}

bool AKalmalaWildlifeSpawn::IsReefMawBiteInReach(const bool bServerAuthority,
    const bool bValidTarget, const bool bTargetSwimming, const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && bTargetSwimming && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= ReefMawBiteReachCentimetres;
}

bool AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(const uint64 SpawnSeed, const EKalmalaBiome Biome,
    EKalmalaWildlifeArchetype& OutArchetype)
{
    const uint64 Slot = SpawnSeed % 10ull;
    if (Biome == EKalmalaBiome::Meadows)
    {
        // Keep the 90% Boar/Deer share local; Rat owns the tenth slot.
        switch (Slot)
        {
        case 0: case 2: case 4: case 6: case 8:
            OutArchetype = EKalmalaWildlifeArchetype::Boar;
            return true;
        case 1: case 3: case 5: case 7:
            OutArchetype = EKalmalaWildlifeArchetype::Deer;
            return true;
        case 9:
            OutArchetype = EKalmalaWildlifeArchetype::Rat;
            return true;
        default:
            return false;
        }
    }

    if (Biome == EKalmalaBiome::Elderwood)
    {
        // Retain a local Boar/Deer roster; Mireling is reserved for Mossy Mire.
        switch (Slot)
        {
        case 0: case 2: case 4:
            OutArchetype = EKalmalaWildlifeArchetype::Boar;
            return true;
        case 1: case 3:
            OutArchetype = EKalmalaWildlifeArchetype::Deer;
            return true;
        case 5: case 6: case 7: case 8:
            OutArchetype = EKalmalaWildlifeArchetype::Badger;
            return true;
        case 9:
            OutArchetype = EKalmalaWildlifeArchetype::MoonBear;
            return true;
        default:
            return false;
        }
    }

    if (Biome == EKalmalaBiome::ShimmeringLakes)
    {
        // Assign the 50/40/10 candidate-slot mix directly from the stable seed.
        // The first five of ten slots retain the existing Boar/Deer roster.
        switch (Slot)
        {
        case 0: case 2: case 4: OutArchetype = EKalmalaWildlifeArchetype::Boar; return true;
        case 1: case 3: OutArchetype = EKalmalaWildlifeArchetype::Deer; return true;
        case 5: case 6: case 7: case 8: OutArchetype = EKalmalaWildlifeArchetype::Viper; return true;
        case 9: OutArchetype = EKalmalaWildlifeArchetype::GiantSnake; return true;
        default: return false;
        }
    }

    if (Biome == EKalmalaBiome::MossyMire && Slot <= 4)
    {
        OutArchetype = EKalmalaWildlifeArchetype::Mireling;
        return true;
    }

    if (Biome == EKalmalaBiome::MossyMire && (Slot == 5 || Slot == 6))
    {
        OutArchetype = EKalmalaWildlifeArchetype::DeathAdder;
        return true;
    }

    if (Biome == EKalmalaBiome::MossyMire && (Slot == 7 || Slot == 8))
    {
        OutArchetype = EKalmalaWildlifeArchetype::MosquitoSwarm;
        return true;
    }

    if (Biome == EKalmalaBiome::MossyMire && Slot == 9)
    {
        OutArchetype = EKalmalaWildlifeArchetype::PeatHusk;
        return true;
    }

    // Keep the reviewed 20/20/10 Wolf, Lynx and rare Polar Bear shares deterministic.
    if (Biome == EKalmalaBiome::FreezingTundra && (Slot == 5 || Slot == 6))
    {
        OutArchetype = EKalmalaWildlifeArchetype::Wolf;
        return true;
    }

    if (Biome == EKalmalaBiome::FreezingTundra && (Slot == 7 || Slot == 8))
    {
        OutArchetype = EKalmalaWildlifeArchetype::Lynx;
        return true;
    }

    if (Biome == EKalmalaBiome::FreezingTundra && Slot == 9)
    {
        OutArchetype = EKalmalaWildlifeArchetype::PolarBear;
        return true;
    }

    if (Biome == EKalmalaBiome::ThunderMountains && Slot >= 5 && Slot <= 8)
    {
        OutArchetype = EKalmalaWildlifeArchetype::HornedGoat;
        return true;
    }

    if (Biome == EKalmalaBiome::ThunderMountains && Slot == 9)
    {
        OutArchetype = EKalmalaWildlifeArchetype::ThunderSentinel;
        return true;
    }

    if (Biome == EKalmalaBiome::Ocean && Slot == 9)
    {
        OutArchetype = EKalmalaWildlifeArchetype::ReefMaw;
        return true;
    }

    // Unsupported biomes produce no wildlife candidates.
    return false;
}

bool AKalmalaWildlifeSpawn::IsBoarChargeAllowed(const bool bServerAuthority, const bool bAlreadyDefeated, const bool bAtRest, const float DistanceToRestingArea)
{
    return bServerAuthority && !bAlreadyDefeated && bAtRest && FMath::IsFinite(DistanceToRestingArea)
        && DistanceToRestingArea >= 0.0f && DistanceToRestingArea <= 500.0f;
}

bool AKalmalaWildlifeSpawn::IsBehaviourTransitionAllowed(const bool bServerAuthority, const bool bAlreadyDefeated, const EKalmalaWildlifeBehaviour From, const EKalmalaWildlifeBehaviour To)
{
    if (!bServerAuthority || bAlreadyDefeated)
    {
        return false;
    }

    return (From == EKalmalaWildlifeBehaviour::Idle && To == EKalmalaWildlifeBehaviour::Flee)
        || (From == EKalmalaWildlifeBehaviour::Flee && To == EKalmalaWildlifeBehaviour::Investigate)
        || (From == EKalmalaWildlifeBehaviour::Investigate && To == EKalmalaWildlifeBehaviour::Return)
        || (From == EKalmalaWildlifeBehaviour::Return && To == EKalmalaWildlifeBehaviour::Idle);
}

void AKalmalaWildlifeSpawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMosquitoPresentation(DeltaSeconds);
    if (HasAuthority() && !bDefeated)
    {
        AdvanceFangArrowWeakPoisonFromServer(DeltaSeconds);
        AdvanceServerBehaviour(DeltaSeconds);
        if (Archetype == EKalmalaWildlifeArchetype::Mireling) UpdateMirelingScavenge(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Boar) UpdateBoarTerritory(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Rat) UpdateRatAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Badger) UpdateBadgerAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Wolf) UpdateWolfAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Lynx) UpdateLynxAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::PolarBear) UpdatePolarBearAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::MoonBear) UpdateMoonBearAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::HornedGoat) UpdateHornedGoatAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::ThunderSentinel) UpdateThunderSentinelAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::ReefMaw) UpdateReefMawAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::PeatHusk) UpdatePeatHuskAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Viper
            || Archetype == EKalmalaWildlifeArchetype::GiantSnake
            || Archetype == EKalmalaWildlifeArchetype::DeathAdder)
            UpdateSnakeAmbush(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm)
            UpdateMosquitoSwarm(DeltaSeconds);
    }
}

void AKalmalaWildlifeSpawn::UpdateMirelingScavenge(const float DeltaSeconds)
{
    MirelingMeleeCooldown = FMath::Max(0.0f, MirelingMeleeCooldown - FMath::Clamp(DeltaSeconds, 0.0f, 0.10f));
    AKalmalaCharacter* Nearest = nullptr; float BestDistanceSquared = FMath::Square(500.0f);
    for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
    {
        AKalmalaCharacter* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->HasAuthority()) continue;
        if (Candidate->GetHealth() <= 1.0f) continue;
        const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
        if (DistanceSquared < BestDistanceSquared) { Nearest = Candidate; BestDistanceSquared = DistanceSquared; }
    }
    if (Nearest == nullptr) return;
    if (BestDistanceSquared > FMath::Square(145.0f) && Behaviour == EKalmalaWildlifeBehaviour::Idle)
    {
        const FVector Direction = (Nearest->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        SetActorLocation(GetActorLocation() + Direction * FMath::Min(170.0f * FMath::Clamp(DeltaSeconds, 0.0f, 0.10f), FMath::Sqrt(BestDistanceSquared) - 145.0f), true);
    }
    else if (MirelingMeleeCooldown <= 0.0f && BestDistanceSquared <= FMath::Square(180.0f))
    {
        const bool bApplied = Nearest->ApplyWildlifeDamageFromServer(this, GetBaseAttackDamageForArchetype(Archetype));
        UE_LOG(LogTemp, Display, TEXT("Mireling encounter attempt: Applied=%d Authority=%d SourceAuthority=%d Distance=%.1f Health=%.1f"), bApplied, Nearest->HasAuthority(), HasAuthority(), FMath::Sqrt(BestDistanceSquared), Nearest->GetHealth());
        if (bApplied) MirelingMeleeCooldown = MirelingMeleeCooldownSeconds;
    }
}

void AKalmalaWildlifeSpawn::UpdateBoarTerritory(const float DeltaSeconds)
{
    const float StepSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.10f);
    BoarMeleeCooldown = FMath::Max(0.0f, BoarMeleeCooldown - StepSeconds);
    AKalmalaCharacter* Target = BoarChargeTarget.Get();
    if (Target == nullptr && Behaviour == EKalmalaWildlifeBehaviour::Idle)
    {
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            const float RestDistance = FVector::Dist2D(Candidate->GetActorLocation(), SpawnOrigin);
            if (IsValid(Candidate) && Candidate->HasAuthority() && Candidate->GetHealth() > 1.0f
                && IsBoarChargeAllowed(true, false, true, RestDistance))
            {
                Target = Candidate;
                BoarChargeTarget = Candidate;
                BoarChargeSecondsRemaining = 1.25f;
                break;
            }
        }
    }

    if (Target != nullptr && Target->HasAuthority() && Target->GetHealth() > 1.0f && BoarChargeSecondsRemaining > 0.0f)
    {
        const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
        const float Distance = ToTarget.Size2D();
        if (Distance > 145.0f) SetActorLocation(GetActorLocation() + ToTarget.GetSafeNormal2D() * FMath::Min(520.0f * StepSeconds, Distance - 145.0f), true);
        if (Distance <= 180.0f && BoarMeleeCooldown <= 0.0f
            && Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackDamageForArchetype(Archetype))) BoarMeleeCooldown = 1.25f;
        BoarChargeSecondsRemaining -= StepSeconds;
        return;
    }

    BoarChargeTarget.Reset();
    BoarChargeSecondsRemaining = 0.0f;
    if (Behaviour == EKalmalaWildlifeBehaviour::Idle && FVector::DistSquared2D(GetActorLocation(), SpawnOrigin) > FMath::Square(15.0f))
    {
        SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds, 260.0f), true);
    }
}

void AKalmalaWildlifeSpawn::UpdateRatAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    const EKalmalaRatAttackCue Cue = static_cast<EKalmalaRatAttackCue>(RatAttackCue);
    AKalmalaCharacter* Target = RatAttackTarget.Get();

    if (Target != nullptr && !IsRatAttackTargetValid(Target))
    {
        RatAttackTarget.Reset();
        Target = nullptr;
        RatAttackSecondsRemaining = 0.0f;
        SetRatAttackCue(EKalmalaRatAttackCue::None);
    }

    if (Cue == EKalmalaRatAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(RatAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsRatAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) RatAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(10.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds, 150.0f);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanRatMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size();
    if (Distance > RatAttackTriggerCentimetres || ToTarget.IsNearlyZero())
    {
        if (Cue != EKalmalaRatAttackCue::Telegraph && Cue != EKalmalaRatAttackCue::Recovery)
        {
            const float MoveDistance = FMath::Min(160.0f * StepSeconds,
                FMath::Max(0.0f, FVector(ToTarget.X, ToTarget.Y, 0.0f).Size() - RatBiteReachCentimetres + 10.0f));
            if (MoveDistance > 0.0f)
            {
                const FVector Direction = ToTarget.GetSafeNormal2D();
                SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
                FVector End = GetActorLocation() + Direction * MoveDistance;
                if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                    End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
                if (CanRatMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
            }
        }
    }

    if (Cue == EKalmalaRatAttackCue::Telegraph)
    {
        RatAttackSecondsRemaining -= StepSeconds;
        if (RatAttackSecondsRemaining <= 0.0f)
        {
            TryApplyRatBite(Target);
            RatAttackSecondsRemaining = RatRecoverySeconds;
            SetRatAttackCue(EKalmalaRatAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaRatAttackCue::Recovery)
    {
        RatAttackSecondsRemaining -= StepSeconds;
        if (RatAttackSecondsRemaining <= 0.0f)
        {
            RatAttackSecondsRemaining = 0.0f;
            SetRatAttackCue(EKalmalaRatAttackCue::None);
        }
    }
    else if (Distance <= RatAttackTriggerCentimetres)
    {
        BeginRatAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginRatAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsRatAttackTargetValid(Target)
        || FVector::DistSquared(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(RatAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    RatAttackTarget = Target;
    RatAttackSecondsRemaining = RatTelegraphSeconds;
    SetRatAttackCue(EKalmalaRatAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetRatAttackCue(const EKalmalaRatAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (RatAttackCue == NextValue) return;
    RatAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsRatAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || !IsValid(Target) || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin) > FMath::Square(RatSpawnLeashCentimetres))
        return false;

    if (!CanRatMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaRatBiteVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 24.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyRatBite(AKalmalaCharacter* Target)
{
    if (!IsRatAttackTargetValid(Target)
        || !IsRatBiteInReach(HasAuthority(), true, FVector::Dist(GetActorLocation(), Target->GetActorLocation())))
        return false;

    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

bool AKalmalaWildlifeSpawn::IsRatPositionDry(const FVector& Position) const
{
    if (Position.ContainsNaN()) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld() != nullptr
        ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    return !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
}

bool AKalmalaWildlifeSpawn::CanRatMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsRatPositionDry(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateBadgerAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(BadgerAttackCue);
    AKalmalaCharacter* Target = BadgerAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None
        && (Target == nullptr || !IsBadgerAttackTargetValid(Target)))
    {
        BadgerAttackTarget.Reset();
        BadgerAttackSecondsRemaining = 0.0f;
        SetBadgerAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsBadgerAttackTargetValid(Target))
    {
        BadgerAttackTarget.Reset();
        BadgerAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(BadgerAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsBadgerAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) BadgerAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds, 150.0f);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (IsBadgerPositionDry(End) && CanBadgerMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    float Distance = ToTarget.Size2D();
    if (Distance > BadgerClawReachCentimetres)
    {
        const float MoveDistance = FMath::Min(BadgerApproachSpeedCentimetresPerSecond * StepSeconds,
            Distance - BadgerClawReachCentimetres);
        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanBadgerMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        ToTarget = Target->GetActorLocation() - GetActorLocation();
        Distance = ToTarget.Size2D();
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        BadgerAttackSecondsRemaining -= StepSeconds;
        if (BadgerAttackSecondsRemaining <= 0.0f)
        {
            TryApplyBadgerClaw(Target);
            BadgerAttackSecondsRemaining = BadgerRecoverySeconds;
            SetBadgerAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        BadgerAttackSecondsRemaining -= StepSeconds;
        if (BadgerAttackSecondsRemaining <= 0.0f)
        {
            BadgerAttackSecondsRemaining = 0.0f;
            SetBadgerAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
    else if (Distance <= BadgerAttackTriggerCentimetres)
    {
        BeginBadgerAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginBadgerAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsBadgerAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(BadgerAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    BadgerAttackTarget = Target;
    BadgerAttackSecondsRemaining = BadgerTelegraphSeconds;
    SetBadgerAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetBadgerAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (BadgerAttackCue == NextValue) return;
    BadgerAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsBadgerAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || GetWorld() == nullptr || !IsValid(Target)
        || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin) > FMath::Square(BadgerSpawnLeashCentimetres))
        return false;
    if (!CanBadgerMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaBadgerClawVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 32.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyBadgerClaw(AKalmalaCharacter* Target)
{
    if (!IsBadgerAttackTargetValid(Target)
        || !IsBadgerClawInReach(HasAuthority(), true, FVector::Dist(GetActorLocation(), Target->GetActorLocation())))
        return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

bool AKalmalaWildlifeSpawn::IsBadgerPositionDry(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    return FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY)) == EKalmalaBiome::Elderwood
        && !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
}

bool AKalmalaWildlifeSpawn::CanBadgerMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsBadgerPositionDry(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateWolfAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(WolfAttackCue);
    AKalmalaCharacter* Target = WolfAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsWolfAttackTargetValid(Target))
    {
        WolfAttackTarget.Reset();
        WolfAttackSecondsRemaining = 0.0f;
        WolfLungeDistanceRemaining = 0.0f;
        SetWolfAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsWolfAttackTargetValid(Target))
    {
        WolfAttackTarget.Reset();
        WolfAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(WolfAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsWolfAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) WolfAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds,
                WolfApproachSpeedCentimetresPerSecond);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanWolfMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float HorizontalBiteReach = FMath::Sqrt(FMath::Max(0.0f,
        FMath::Square(WolfBiteReachCentimetres) - FMath::Square(ToTarget.Z)));
    const float ApproachStopDistance = Cue == EKalmalaWildlifeAttackCue::None
        ? WolfAttackTriggerCentimetres : HorizontalBiteReach;
    const bool bPursuing = Cue == EKalmalaWildlifeAttackCue::None
        || Cue == EKalmalaWildlifeAttackCue::Telegraph
        || Cue == EKalmalaWildlifeAttackCue::Recovery;
    if (bPursuing && ToTarget.Size2D() > ApproachStopDistance)
    {
        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        const float MoveDistance = FMath::Min(WolfApproachSpeedCentimetresPerSecond * StepSeconds,
            ToTarget.Size2D() - ApproachStopDistance);
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanWolfMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        ToTarget = Target->GetActorLocation() - GetActorLocation();
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        WolfAttackSecondsRemaining -= StepSeconds;
        if (WolfAttackSecondsRemaining <= 0.0f)
        {
            WolfLungeDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
            if (WolfLungeDirection.IsNearlyZero())
            {
                WolfAttackTarget.Reset();
                SetWolfAttackCue(EKalmalaWildlifeAttackCue::None);
                return;
            }
            SetActorRotation(FRotationMatrix::MakeFromX(WolfLungeDirection).Rotator());
            WolfLungeDistanceRemaining = WolfBiteReachCentimetres;
            WolfAttackSecondsRemaining = WolfLungeSeconds;
            SetWolfAttackCue(EKalmalaWildlifeAttackCue::Lunge);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Lunge)
    {
        const float HorizontalDistance = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
        const float VerticalDistance = Target->GetActorLocation().Z - GetActorLocation().Z;
        const float HorizontalImpactReach = FMath::Sqrt(FMath::Max(0.0f,
            FMath::Square(WolfBiteReachCentimetres) - FMath::Square(VerticalDistance)));
        const float MoveDistance = FMath::Min3(WolfLungeDistanceRemaining,
            FMath::Max(0.0f, HorizontalDistance - HorizontalImpactReach),
            WolfBiteReachCentimetres * StepSeconds / WolfLungeSeconds);
        if (MoveDistance > 0.0f)
        {
            FVector End = GetActorLocation() + WolfLungeDirection * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanWolfMoveAlong(GetActorLocation(), End))
            {
                SetActorLocation(End, true);
                WolfLungeDistanceRemaining -= MoveDistance;
            }
            else
            {
                WolfLungeDistanceRemaining = 0.0f;
            }
        }
        WolfAttackSecondsRemaining -= StepSeconds;
        if (WolfAttackSecondsRemaining <= 0.0f)
        {
            TryApplyWolfBite(Target);
            WolfAttackSecondsRemaining = WolfRecoverySeconds;
            WolfLungeDistanceRemaining = 0.0f;
            SetWolfAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        WolfAttackSecondsRemaining -= StepSeconds;
        if (WolfAttackSecondsRemaining <= 0.0f)
        {
            WolfAttackSecondsRemaining = 0.0f;
            SetWolfAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
    else if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) <= WolfAttackTriggerCentimetres)
    {
        BeginWolfAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginWolfAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsWolfAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(WolfAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    WolfAttackTarget = Target;
    WolfAttackSecondsRemaining = WolfTelegraphSeconds;
    SetWolfAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetWolfAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (WolfAttackCue == NextValue) return;
    WolfAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsWolfAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || GetWorld() == nullptr || !IsValid(Target)
        || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin) > FMath::Square(WolfSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin) > FMath::Square(WolfSpawnLeashCentimetres)
        || !CanWolfMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaWolfBiteVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyWolfBite(AKalmalaCharacter* Target)
{
    if (!IsWolfAttackTargetValid(Target)
        || !IsWolfBiteInReach(HasAuthority(), true, FVector::Dist(GetActorLocation(), Target->GetActorLocation())))
        return false;
    const bool bApplied = Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
#if !UE_BUILD_SHIPPING
    if (bApplied && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWolfPeerTest")))
        ++WolfBiteHitCountForVerification;
#endif
    return bApplied;
}

bool AKalmalaWildlifeSpawn::IsWolfPositionTraversable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    return FKalmalaWorldBounds::Contains(Config, XY)
        && FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY))
            == EKalmalaBiome::FreezingTundra
        && !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY)
        && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
}

bool AKalmalaWildlifeSpawn::CanWolfMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsWolfPositionTraversable(Position)) return false;
    }
    return true;
}

bool AKalmalaWildlifeSpawn::IsLynxLeapInReach(const bool bServerAuthority, const bool bValidTarget,
    const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= LynxLeapImpactReachCentimetres;
}

void AKalmalaWildlifeSpawn::UpdateLynxAttack(const float DeltaSeconds)
{
    if (!HasAuthority() || bDefeated || Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(LynxAttackCue);
    AKalmalaCharacter* Target = LynxAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsLynxAttackTargetValid(Target))
    {
        LynxAttackTarget.Reset();
        LynxAttackSecondsRemaining = 0.0f;
        LynxLeapDistanceRemaining = 0.0f;
        SetLynxAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsLynxAttackTargetValid(Target))
    {
        LynxAttackTarget.Reset();
        LynxAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(LynxAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsLynxAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) LynxAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds,
                LynxApproachSpeedCentimetresPerSecond);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanLynxMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float VerticalDistance = ToTarget.Z;
    const float HorizontalImpactReach = FMath::Sqrt(FMath::Max(0.0f,
        FMath::Square(LynxLeapImpactReachCentimetres) - FMath::Square(VerticalDistance)));
    const float ApproachStopDistance = Cue == EKalmalaWildlifeAttackCue::None
        ? LynxAttackTriggerCentimetres : HorizontalImpactReach;
    const bool bPursuing = Cue == EKalmalaWildlifeAttackCue::None
        || Cue == EKalmalaWildlifeAttackCue::Telegraph
        || Cue == EKalmalaWildlifeAttackCue::Recovery;
    if (bPursuing && ToTarget.Size2D() > ApproachStopDistance)
    {
        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        const float MoveDistance = FMath::Min(LynxApproachSpeedCentimetresPerSecond * StepSeconds,
            ToTarget.Size2D() - ApproachStopDistance);
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanLynxMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        ToTarget = Target->GetActorLocation() - GetActorLocation();
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        LynxAttackSecondsRemaining -= StepSeconds;
        if (LynxAttackSecondsRemaining <= 0.0f)
        {
            LynxLeapDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
            if (LynxLeapDirection.IsNearlyZero())
            {
                LynxAttackTarget.Reset();
                SetLynxAttackCue(EKalmalaWildlifeAttackCue::None);
                return;
            }
            SetActorRotation(FRotationMatrix::MakeFromX(LynxLeapDirection).Rotator());
            LynxLeapDistanceRemaining = FMath::Min(LynxLeapImpactReachCentimetres,
                FMath::Max(0.0f, FVector::Dist2D(GetActorLocation(), Target->GetActorLocation())
                    - LynxLeapLandingDistanceCentimetres));
            LynxAttackSecondsRemaining = LynxLeapSeconds;
            SetLynxAttackCue(EKalmalaWildlifeAttackCue::Lunge);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Lunge)
    {
        const float HorizontalDistance = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
        const float MoveDistance = FMath::Min3(LynxLeapDistanceRemaining,
            FMath::Max(0.0f, HorizontalDistance - LynxLeapLandingDistanceCentimetres),
            LynxLeapSpeedCentimetresPerSecond * StepSeconds);
        if (MoveDistance > 0.0f)
        {
            FVector End = GetActorLocation() + LynxLeapDirection * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanLynxMoveAlong(GetActorLocation(), End))
            {
                SetActorLocation(End, true);
                LynxLeapDistanceRemaining -= MoveDistance;
            }
            else
            {
                LynxLeapDistanceRemaining = 0.0f;
            }
        }
        LynxAttackSecondsRemaining -= StepSeconds;
        if (LynxAttackSecondsRemaining <= 0.0f)
        {
            const bool bLandedHit = TryApplyLynxLeap(Target);
            LynxAttackSecondsRemaining = bLandedHit ? LynxRecoverySeconds : LynxMissRecoverySeconds;
            LynxLeapDistanceRemaining = 0.0f;
            SetLynxAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        LynxAttackSecondsRemaining -= StepSeconds;
        if (LynxAttackSecondsRemaining <= 0.0f)
        {
            LynxAttackSecondsRemaining = 0.0f;
            SetLynxAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
    else if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) <= LynxAttackTriggerCentimetres)
    {
        BeginLynxAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginLynxAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsLynxAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(LynxAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    LynxAttackTarget = Target;
    LynxAttackSecondsRemaining = LynxTelegraphSeconds;
    SetLynxAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetLynxAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (LynxAttackCue == NextValue) return;
    LynxAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsLynxAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::Lynx
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || !FMath::IsFinite(Target->GetHealth())
        || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(LynxSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin)
            > FMath::Square(LynxSpawnLeashCentimetres)
        || !CanLynxMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaLynxLeapVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyLynxLeap(AKalmalaCharacter* Target)
{
    if (!IsLynxAttackTargetValid(Target)
        || !IsLynxLeapInReach(HasAuthority(), true,
            FVector::Dist(GetActorLocation(), Target->GetActorLocation()))) return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

bool AKalmalaWildlifeSpawn::IsLynxPositionTraversable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    return FKalmalaWorldBounds::Contains(Config, XY)
        && FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY))
            == EKalmalaBiome::FreezingTundra
        && !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY)
        && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
}

bool AKalmalaWildlifeSpawn::CanLynxMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsLynxPositionTraversable(Position)) return false;
    }
    return true;
}

bool AKalmalaWildlifeSpawn::IsPolarBearChargeInReach(const bool bServerAuthority,
    const bool bValidTarget, const float DistanceCentimetres)
{
    return bServerAuthority && bValidTarget && FMath::IsFinite(DistanceCentimetres)
        && DistanceCentimetres >= 0.0f && DistanceCentimetres <= PolarBearChargeImpactReachCentimetres;
}

void AKalmalaWildlifeSpawn::UpdatePolarBearAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(PolarBearAttackCue);
    AKalmalaCharacter* Target = PolarBearAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsPolarBearAttackTargetValid(Target))
    {
        PolarBearAttackTarget.Reset();
        PolarBearAttackSecondsRemaining = 0.0f;
        PolarBearChargeDistanceRemaining = 0.0f;
        PolarBearRetreatDistanceRemaining = 0.0f;
        bPolarBearChargeHitApplied = false;
        SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsPolarBearAttackTargetValid(Target))
    {
        PolarBearAttackTarget.Reset();
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(PolarBearAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsPolarBearAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) PolarBearAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            const FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds,
                WolfApproachSpeedCentimetresPerSecond);
            if (CanPolarBearMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None)
    {
        const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
        if (ToTarget.Size2D() > PolarBearAttackTriggerCentimetres)
        {
            const FVector Direction = ToTarget.GetSafeNormal2D();
            if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
            const float MoveDistance = FMath::Min(PolarBearApproachSpeedCentimetresPerSecond * StepSeconds,
                ToTarget.Size2D() - PolarBearAttackTriggerCentimetres);
            FVector End = GetActorLocation() + Direction * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanPolarBearMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
            return;
        }
        BeginPolarBearAttack(Target);
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        PolarBearAttackSecondsRemaining -= StepSeconds;
        if (PolarBearAttackSecondsRemaining <= 0.0f)
        {
            PolarBearChargeDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
            if (PolarBearChargeDirection.IsNearlyZero())
            {
                PolarBearAttackTarget.Reset();
                SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::None);
                return;
            }
            SetActorRotation(FRotationMatrix::MakeFromX(PolarBearChargeDirection).Rotator());
            PolarBearChargeDistanceRemaining = PolarBearMaximumChargeCentimetres;
            PolarBearAttackSecondsRemaining = PolarBearChargeSeconds;
            bPolarBearChargeHitApplied = false;
            SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::Lunge);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Lunge)
    {
        const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
        const float VerticalDistance = ToTarget.Z;
        const float HorizontalImpactReach = FMath::Sqrt(FMath::Max(0.0f,
            FMath::Square(PolarBearChargeImpactReachCentimetres) - FMath::Square(VerticalDistance)));
        const float HorizontalDistanceToTarget = static_cast<float>(ToTarget.Size2D());
        const float MoveDistance = FMath::Min3(PolarBearChargeDistanceRemaining,
            FMath::Max(0.0f, HorizontalDistanceToTarget - HorizontalImpactReach),
            PolarBearChargeSpeedCentimetresPerSecond * StepSeconds);
        if (MoveDistance > 0.0f)
        {
            FVector End = GetActorLocation() + PolarBearChargeDirection * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanPolarBearMoveAlong(GetActorLocation(), End))
            {
                SetActorLocation(End, true);
                PolarBearChargeDistanceRemaining -= MoveDistance;
            }
            else
            {
                PolarBearChargeDistanceRemaining = 0.0f;
            }
        }

        if (!bPolarBearChargeHitApplied
            && IsPolarBearChargeInReach(HasAuthority(), true,
                FVector::Dist(GetActorLocation(), Target->GetActorLocation())))
        {
            bPolarBearChargeHitApplied = TryApplyPolarBearCharge(Target);
            if (bDefeated) return;
        }

        PolarBearAttackSecondsRemaining -= StepSeconds;
        if (bPolarBearChargeHitApplied || PolarBearChargeDistanceRemaining <= 0.0f
            || PolarBearAttackSecondsRemaining <= 0.0f)
        {
            PolarBearRetreatDirection = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
            if (PolarBearRetreatDirection.IsNearlyZero()) PolarBearRetreatDirection = -PolarBearChargeDirection;
            if (!PolarBearRetreatDirection.IsNearlyZero())
                SetActorRotation(FRotationMatrix::MakeFromX(PolarBearRetreatDirection).Rotator());
            PolarBearRetreatDistanceRemaining = PolarBearRetreatDistanceCentimetres;
            PolarBearAttackSecondsRemaining = PolarBearRetreatSeconds;
            SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::Retreat);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Retreat)
    {
        const float MoveDistance = FMath::Min(PolarBearRetreatDistanceRemaining,
            PolarBearRetreatSpeedCentimetresPerSecond * StepSeconds);
        if (MoveDistance > 0.0f)
        {
            FVector End = GetActorLocation() + PolarBearRetreatDirection * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanPolarBearMoveAlong(GetActorLocation(), End))
            {
                SetActorLocation(End, true);
                PolarBearRetreatDistanceRemaining -= MoveDistance;
            }
            else
            {
                PolarBearRetreatDistanceRemaining = 0.0f;
            }
        }
        PolarBearAttackSecondsRemaining -= StepSeconds;
        if (PolarBearRetreatDistanceRemaining <= 0.0f || PolarBearAttackSecondsRemaining <= 0.0f)
        {
            PolarBearAttackSecondsRemaining = PolarBearRecoverySeconds;
            SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        PolarBearAttackSecondsRemaining -= StepSeconds;
        if (PolarBearAttackSecondsRemaining <= 0.0f)
        {
            PolarBearAttackSecondsRemaining = 0.0f;
            PolarBearAttackTarget.Reset();
            bPolarBearChargeHitApplied = false;
            SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
}

void AKalmalaWildlifeSpawn::BeginPolarBearAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsPolarBearAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(PolarBearAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    PolarBearAttackTarget = Target;
    PolarBearAttackSecondsRemaining = PolarBearTelegraphSeconds;
    bPolarBearChargeHitApplied = false;
    SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetPolarBearAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (PolarBearAttackCue == NextValue) return;
    PolarBearAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsPolarBearAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::PolarBear
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || !FMath::IsFinite(Target->GetHealth())
        || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(PolarBearSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin)
            > FMath::Square(PolarBearSpawnLeashCentimetres)
        || !CanPolarBearMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaPolarBearChargeVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyPolarBearCharge(AKalmalaCharacter* Target)
{
    if (!IsPolarBearAttackTargetValid(Target)
        || !IsPolarBearChargeInReach(HasAuthority(), true,
            FVector::Dist(GetActorLocation(), Target->GetActorLocation()))) return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

bool AKalmalaWildlifeSpawn::IsPolarBearPositionTraversable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    return FKalmalaWorldBounds::Contains(Config, XY)
        && FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY))
            == EKalmalaBiome::FreezingTundra
        && !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY)
        && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
}

bool AKalmalaWildlifeSpawn::CanPolarBearMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsPolarBearPositionTraversable(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateMoonBearAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(MoonBearAttackCue);
    AKalmalaCharacter* Target = MoonBearAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None
        && (Target == nullptr || !IsMoonBearAttackTargetValid(Target)))
    {
        MoonBearAttackTarget.Reset();
        MoonBearAttackSecondsRemaining = 0.0f;
        SetMoonBearAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsMoonBearAttackTargetValid(Target))
    {
        MoonBearAttackTarget.Reset();
        MoonBearAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(MoonBearAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsMoonBearAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) MoonBearAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds, 120.0f);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (IsBadgerPositionDry(End) && CanBadgerMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    float Distance = ToTarget.Size2D();
    if (Distance > MoonBearPawReachCentimetres)
    {
        const float MoveDistance = FMath::Min(BadgerApproachSpeedCentimetresPerSecond * StepSeconds,
            Distance - MoonBearPawReachCentimetres);
        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanBadgerMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        ToTarget = Target->GetActorLocation() - GetActorLocation();
        Distance = ToTarget.Size2D();
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        MoonBearAttackSecondsRemaining -= StepSeconds;
        if (MoonBearAttackSecondsRemaining <= 0.0f)
        {
            TryApplyMoonBearPaw(Target);
            MoonBearAttackSecondsRemaining = MoonBearRecoverySeconds;
            SetMoonBearAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        MoonBearAttackSecondsRemaining -= StepSeconds;
        if (MoonBearAttackSecondsRemaining <= 0.0f)
        {
            MoonBearAttackSecondsRemaining = 0.0f;
            SetMoonBearAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
    else if (Distance <= MoonBearAttackTriggerCentimetres)
    {
        BeginMoonBearAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginMoonBearAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsMoonBearAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(MoonBearAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    MoonBearAttackTarget = Target;
    MoonBearAttackSecondsRemaining = MoonBearTelegraphSeconds;
    SetMoonBearAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetMoonBearAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (MoonBearAttackCue == NextValue) return;
    MoonBearAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsMoonBearAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || GetWorld() == nullptr || !IsValid(Target)
        || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin) > FMath::Square(MoonBearSpawnLeashCentimetres))
        return false;
    if (!CanBadgerMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaMoonBearPawVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyMoonBearPaw(AKalmalaCharacter* Target)
{
    if (!IsMoonBearAttackTargetValid(Target)
        || !IsMoonBearPawInReach(HasAuthority(), true, FVector::Dist(GetActorLocation(), Target->GetActorLocation())))
        return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

void AKalmalaWildlifeSpawn::UpdateHornedGoatAttack(const float DeltaSeconds)
{
    if (!HasAuthority() || bDefeated || Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(HornedGoatAttackCue);
    AKalmalaCharacter* Target = HornedGoatAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsHornedGoatAttackTargetValid(Target))
    {
        HornedGoatAttackTarget.Reset();
        HornedGoatAttackSecondsRemaining = 0.0f;
        SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsHornedGoatAttackTargetValid(Target))
    {
        HornedGoatAttackTarget.Reset();
        HornedGoatAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(HornedGoatAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsHornedGoatAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) BeginHornedGoatAttack(Target);
        return;
    }
    if (Target == nullptr) return;

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    float Distance = ToTarget.Size2D();
    const FVector Direction = ToTarget.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        const float ChargeDistance = FMath::Min(HornedGoatChargeSpeedCentimetresPerSecond * StepSeconds,
            FMath::Max(0.0f, Distance - HornedGoatHornReachCentimetres));
        if (ChargeDistance > 0.0f)
        {
            FVector End = GetActorLocation() + Direction * ChargeDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanHornedGoatMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        HornedGoatAttackSecondsRemaining -= StepSeconds;
        if (HornedGoatAttackSecondsRemaining <= 0.0f)
        {
            TryApplyHornedGoatCharge(Target);
            HornedGoatAttackSecondsRemaining = HornedGoatRecoverySeconds;
            SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        HornedGoatAttackSecondsRemaining -= StepSeconds;
        if (HornedGoatAttackSecondsRemaining <= 0.0f)
        {
            HornedGoatAttackSecondsRemaining = 0.0f;
            HornedGoatAttackTarget.Reset();
            SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue::None);
        }
        return;
    }

    if (Distance > HornedGoatAttackTriggerCentimetres)
    {
        const float MoveDistance = FMath::Min(HornedGoatApproachSpeedCentimetresPerSecond * StepSeconds,
            Distance - HornedGoatAttackTriggerCentimetres);
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanHornedGoatMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        return;
    }
    BeginHornedGoatAttack(Target);
}

void AKalmalaWildlifeSpawn::BeginHornedGoatAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsHornedGoatAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(HornedGoatAttackTriggerCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    HornedGoatAttackTarget = Target;
    HornedGoatAttackSecondsRemaining = HornedGoatTelegraphSeconds;
    SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetHornedGoatAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (HornedGoatAttackCue == NextValue) return;
    HornedGoatAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsHornedGoatAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::HornedGoat
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || Target->GetAttachParentActor() != nullptr
        || !FMath::IsFinite(Target->GetHealth())
        || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(HornedGoatSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin)
            > FMath::Square(HornedGoatSpawnLeashCentimetres)
        || !CanHornedGoatMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaHornedGoatChargeVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 42.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyHornedGoatCharge(AKalmalaCharacter* Target)
{
    const FVector Direction = Target != nullptr
        ? (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : FVector::ZeroVector;
    if (!IsHornedGoatAttackTargetValid(Target) || Direction.IsNearlyZero()
        || !IsHornedGoatHornInReach(HasAuthority(), true,
            FVector::Dist(GetActorLocation(), Target->GetActorLocation()))) return false;
    if (!Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype))) return false;
    Target->ApplyWildlifeKnockbackFromServer(this, Direction,
        HornedGoatPushCentimetres, HornedGoatStaggerSeconds);
    return true;
}

bool AKalmalaWildlifeSpawn::IsHornedGoatPositionTraversable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY));
    const bool bWater = FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
    const bool bTraversable = FKalmalaWorldBounds::Contains(Config, XY)
        && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
    return IsThunderMountainsSpawnHabitatAllowed(Biome, bWater, bTraversable);
}

bool AKalmalaWildlifeSpawn::CanHornedGoatMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsHornedGoatPositionTraversable(Position)) return false;
    }
    return true;
}

bool AKalmalaWildlifeSpawn::IsThunderSentinelPositionTraversable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY));
    const bool bWater = FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
    const bool bTraversable = FKalmalaWorldBounds::Contains(Config, XY)
        && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
    return IsThunderMountainsSpawnHabitatAllowed(Biome, bWater, bTraversable);
}

bool AKalmalaWildlifeSpawn::CanThunderSentinelMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsThunderSentinelPositionTraversable(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateThunderSentinelAttack(const float DeltaSeconds)
{
    if (!HasAuthority() || bDefeated || Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ThunderSentinelAttackCue);
    AKalmalaCharacter* Target = ThunderSentinelAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsThunderSentinelAttackTargetValid(Target))
    {
        ThunderSentinelAttackTarget.Reset();
        ThunderSentinelAttackSecondsRemaining = 0.0f;
        ThunderSentinelStepDistanceRemaining = 0.0f;
        bThunderSentinelSlamHitApplied = false;
        SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsThunderSentinelAttackTargetValid(Target))
    {
        ThunderSentinelAttackTarget.Reset();
        ThunderSentinelAttackSecondsRemaining = 0.0f;
        ThunderSentinelStepDistanceRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(ThunderSentinelAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsThunderSentinelAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) BeginThunderSentinelAttack(Target);
        return;
    }
    if (Target == nullptr) return;

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    FVector Direction = ToTarget.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = GetActorForwardVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        const float Distance = ToTarget.Size2D();
        const float MoveDistance = FMath::Min3(ThunderSentinelStepDistanceRemaining,
            ThunderSentinelStepSpeedCentimetresPerSecond * StepSeconds,
            FMath::Max(0.0f, Distance - ThunderSentinelSlamReachCentimetres));
        if (MoveDistance > 0.0f)
        {
            FVector End = GetActorLocation() + Direction * MoveDistance;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanThunderSentinelMoveAlong(GetActorLocation(), End))
            {
                SetActorLocation(End, true);
                ThunderSentinelStepDistanceRemaining -= MoveDistance;
            }
            else
            {
                ThunderSentinelStepDistanceRemaining = 0.0f;
            }
        }

        ThunderSentinelAttackSecondsRemaining -= StepSeconds;
        if (ThunderSentinelAttackSecondsRemaining <= 0.0f)
        {
            ThunderSentinelAttackSecondsRemaining = ThunderSentinelSlamCueSeconds;
            bThunderSentinelSlamHitApplied = false;
            SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::Lunge);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Lunge)
    {
        if (!bThunderSentinelSlamHitApplied)
        {
            bThunderSentinelSlamHitApplied = true;
            TryApplyThunderSentinelSlam(Target);
        }
        ThunderSentinelAttackSecondsRemaining -= StepSeconds;
        if (ThunderSentinelAttackSecondsRemaining <= 0.0f)
        {
            ThunderSentinelAttackSecondsRemaining = ThunderSentinelRecoverySeconds;
            SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        ThunderSentinelAttackSecondsRemaining -= StepSeconds;
        if (ThunderSentinelAttackSecondsRemaining <= 0.0f)
        {
            ThunderSentinelAttackSecondsRemaining = 0.0f;
            ThunderSentinelAttackTarget.Reset();
            ThunderSentinelStepDistanceRemaining = 0.0f;
            bThunderSentinelSlamHitApplied = false;
            SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
}

void AKalmalaWildlifeSpawn::BeginThunderSentinelAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsThunderSentinelAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(ThunderSentinelAttackTriggerCentimetres)) return;
    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    ThunderSentinelAttackTarget = Target;
    ThunderSentinelAttackSecondsRemaining = ThunderSentinelTelegraphSeconds;
    ThunderSentinelStepDistanceRemaining = ThunderSentinelStepDistanceCentimetres;
    bThunderSentinelSlamHitApplied = false;
    SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetThunderSentinelAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (ThunderSentinelAttackCue == NextValue) return;
    ThunderSentinelAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsThunderSentinelAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::ThunderSentinel
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || Target->GetAttachParentActor() != nullptr
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(ThunderSentinelSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin)
            > FMath::Square(ThunderSentinelSpawnLeashCentimetres)
        || !CanThunderSentinelMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaThunderSentinelSlamVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 240.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyThunderSentinelSlam(AKalmalaCharacter* Target)
{
    if (!IsThunderSentinelAttackTargetValid(Target)
        || !IsThunderSentinelSlamInReach(HasAuthority(), true,
            FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()))) return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

bool AKalmalaWildlifeSpawn::IsReefMawPositionNavigable(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    if (!FKalmalaWorldBounds::Contains(Config, XY)) return false;
    const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, XY);
    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY));
    return Ocean.IsWater() && IsReefMawSpawnHabitatAllowed(
        Biome, FKalmalaShimmeringLakeSampler::IsWater(Config, XY), Ocean.WaterDepth);
}

bool AKalmalaWildlifeSpawn::CanReefMawSwimAlong(const FVector& Start, const FVector& End) const
{
    if (Start.ContainsNaN() || End.ContainsNaN()) return false;
    const float Distance = FVector::Dist2D(Start, End);
    if (!FMath::IsFinite(Distance)) return false;
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsReefMawPositionNavigable(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateReefMawAttack(const float DeltaSeconds)
{
    if (!HasAuthority() || bDefeated || Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ReefMawAttackCue);
    AKalmalaCharacter* Target = ReefMawAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None && !IsReefMawAttackTargetValid(Target))
    {
        ReefMawAttackTarget.Reset();
        ReefMawAttackSecondsRemaining = 0.0f;
        bReefMawBiteHitApplied = false;
        SetReefMawAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsReefMawAttackTargetValid(Target))
    {
        ReefMawAttackTarget.Reset();
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(ReefMawDetectionCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsReefMawAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) ReefMawAttackTarget = Target;
    }
    if (Target == nullptr) return;

    const FVector Destination = Target->GetActorLocation();
    FVector ToTarget = Destination - GetActorLocation();
    const float Distance = ToTarget.Size();
    FVector Direction = ToTarget.GetSafeNormal();
    if (Direction.IsNearlyZero()) Direction = GetActorForwardVector().GetSafeNormal();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());

    if (Cue == EKalmalaWildlifeAttackCue::None)
    {
        if (Distance <= ReefMawAttackTriggerCentimetres)
        {
            BeginReefMawAttack(Target);
            return;
        }
        const float MoveDistance = FMath::Min(ReefMawApproachSpeedCentimetresPerSecond * StepSeconds,
            FMath::Max(0.0f, Distance - ReefMawAttackTriggerCentimetres));
        if (MoveDistance > 0.0f)
        {
            const FVector End = GetActorLocation() + Direction * MoveDistance;
            if (CanReefMawSwimAlong(GetActorLocation(), End)) SetActorLocation(End, true);
            else ReefMawAttackTarget.Reset();
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        const float ApproachStopDistance = ReefMawBiteReachCentimetres * 0.85f;
        const float MoveDistance = FMath::Min(ReefMawApproachSpeedCentimetresPerSecond * StepSeconds,
            FMath::Max(0.0f, Distance - ApproachStopDistance));
        if (MoveDistance > 0.0f)
        {
            const FVector End = GetActorLocation() + Direction * MoveDistance;
            if (CanReefMawSwimAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        ReefMawAttackSecondsRemaining -= StepSeconds;
        if (ReefMawAttackSecondsRemaining <= 0.0f)
        {
            ReefMawAttackSecondsRemaining = ReefMawBiteCueSeconds;
            bReefMawBiteHitApplied = false;
            SetReefMawAttackCue(EKalmalaWildlifeAttackCue::Lunge);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Lunge)
    {
        if (!bReefMawBiteHitApplied)
        {
            bReefMawBiteHitApplied = true;
            TryApplyReefMawBite(Target);
        }
        ReefMawAttackSecondsRemaining -= StepSeconds;
        if (ReefMawAttackSecondsRemaining <= 0.0f)
        {
            ReefMawAttackSecondsRemaining = ReefMawRecoverySeconds;
            SetReefMawAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
        return;
    }

    if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        ReefMawAttackSecondsRemaining -= StepSeconds;
        if (ReefMawAttackSecondsRemaining <= 0.0f)
        {
            ReefMawAttackTarget.Reset();
            ReefMawAttackSecondsRemaining = 0.0f;
            bReefMawBiteHitApplied = false;
            SetReefMawAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
}

void AKalmalaWildlifeSpawn::BeginReefMawAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsReefMawAttackTargetValid(Target)) return;
    const FVector Destination = Target->GetActorLocation();
    if (FVector::Dist(GetActorLocation(), Destination) > ReefMawAttackTriggerCentimetres) return;
    ReefMawAttackTarget = Target;
    ReefMawAttackSecondsRemaining = ReefMawTelegraphSeconds;
    bReefMawBiteHitApplied = false;
    SetReefMawAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetReefMawAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (ReefMawAttackCue == NextValue) return;
    ReefMawAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsReefMawAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::ReefMaw
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || Target->GetAttachParentActor() != nullptr
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || Target->GetActorLocation().ContainsNaN()
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(ReefMawSpawnLeashCentimetres)
        || FVector::DistSquared2D(GetActorLocation(), SpawnOrigin)
            > FMath::Square(ReefMawSpawnLeashCentimetres)) return false;

    const UKalmalaCharacterMovementComponent* Movement =
        Cast<UKalmalaCharacterMovementComponent>(Target->GetCharacterMovement());
    if (Movement == nullptr || !Movement->IsSwimmingInGeneratedOcean()) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaOceanSample TargetOcean = FKalmalaOceanSampler::Sample(
        State->GetWorldGenerationConfig(), FVector2D(Target->GetActorLocation()));
    const EKalmalaBiome TargetBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(
        State->GetWorldGenerationConfig(), FVector2D(Target->GetActorLocation())));
    if (!TargetOcean.IsWater() || !IsReefMawSpawnHabitatAllowed(TargetBiome,
            FKalmalaShimmeringLakeSampler::IsWater(State->GetWorldGenerationConfig(),
                FVector2D(Target->GetActorLocation())), TargetOcean.WaterDepth)
        || !CanReefMawSwimAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaReefMawBiteVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyReefMawBite(AKalmalaCharacter* Target)
{
    const UKalmalaCharacterMovementComponent* Movement = Target != nullptr
        ? Cast<UKalmalaCharacterMovementComponent>(Target->GetCharacterMovement()) : nullptr;
    if (!IsReefMawAttackTargetValid(Target)
        || !IsReefMawBiteInReach(HasAuthority(), true,
            Movement != nullptr && Movement->IsSwimmingInGeneratedOcean(),
            FVector::Dist(GetActorLocation(), Target->GetActorLocation()))) return false;
    return Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
}

void AKalmalaWildlifeSpawn::UpdatePeatHuskAttack(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(PeatHuskAttackCue);
    AKalmalaCharacter* Target = PeatHuskAttackTarget.Get();

    if (Cue != EKalmalaWildlifeAttackCue::None
        && (Target == nullptr || !IsPeatHuskAttackTargetValid(Target)))
    {
        PeatHuskAttackTarget.Reset();
        PeatHuskAttackSecondsRemaining = 0.0f;
        SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::None);
        Cue = EKalmalaWildlifeAttackCue::None;
        Target = nullptr;
    }
    else if (Target != nullptr && !IsPeatHuskAttackTargetValid(Target))
    {
        PeatHuskAttackTarget.Reset();
        PeatHuskAttackSecondsRemaining = 0.0f;
        Target = nullptr;
    }

    if (Cue == EKalmalaWildlifeAttackCue::None && Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(PeatHuskAttackTriggerCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsPeatHuskAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr) PeatHuskAttackTarget = Target;
    }

    if (Target == nullptr)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        if (ToHome.SizeSquared2D() > FMath::Square(15.0f))
        {
            FVector End = FMath::VInterpConstantTo(GetActorLocation(), SpawnOrigin, StepSeconds,
                PeatHuskApproachSpeedCentimetresPerSecond);
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (IsPeatHuskPositionDry(End) && CanPeatHuskMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        return;
    }

    FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    float Distance = ToTarget.Size2D();
    if (Cue != EKalmalaWildlifeAttackCue::Recovery && Distance > PeatHuskSweepReachCentimetres)
    {
        const float MoveDistance = FMath::Min(PeatHuskApproachSpeedCentimetresPerSecond * StepSeconds,
            Distance - PeatHuskSweepReachCentimetres);
        const FVector Direction = ToTarget.GetSafeNormal2D();
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
        FVector End = GetActorLocation() + Direction * MoveDistance;
        if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
        if (CanPeatHuskMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        ToTarget = Target->GetActorLocation() - GetActorLocation();
        Distance = ToTarget.Size2D();
    }

    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        PeatHuskAttackSecondsRemaining -= StepSeconds;
        if (PeatHuskAttackSecondsRemaining <= 0.0f)
        {
            TryApplyPeatHuskSweep(Target);
            PeatHuskAttackSecondsRemaining = PeatHuskRecoverySeconds;
            SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::Recovery);
        }
    }
    else if (Cue == EKalmalaWildlifeAttackCue::Recovery)
    {
        PeatHuskAttackSecondsRemaining -= StepSeconds;
        if (PeatHuskAttackSecondsRemaining <= 0.0f)
        {
            PeatHuskAttackSecondsRemaining = 0.0f;
            SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
    else if (Distance <= PeatHuskSweepReachCentimetres)
    {
        BeginPeatHuskAttack(Target);
    }
}

void AKalmalaWildlifeSpawn::BeginPeatHuskAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsPeatHuskAttackTargetValid(Target)
        || FVector::DistSquared2D(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(PeatHuskSweepReachCentimetres)) return;

    const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    if (Direction.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(Direction).Rotator());
    PeatHuskAttackTarget = Target;
    PeatHuskAttackSecondsRemaining = PeatHuskTelegraphSeconds;
    SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetPeatHuskAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (PeatHuskAttackCue == NextValue) return;
    PeatHuskAttackCue = NextValue;
    BuildArchetypePresentation();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsPeatHuskAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || GetWorld() == nullptr || !IsValid(Target)
        || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(PeatHuskSpawnLeashCentimetres)) return false;
    if (!CanPeatHuskMoveAlong(GetActorLocation(), Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaPeatHuskSweepVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 88.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 48.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplyPeatHuskSweep(AKalmalaCharacter* Target)
{
    const FVector ToTarget = IsValid(Target) ? Target->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
    const float ForwardDot = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToTarget.GetSafeNormal2D());
    if (!IsPeatHuskAttackTargetValid(Target)
        || !IsPeatHuskSweepInReach(HasAuthority(), true, ToTarget.Size2D(), ForwardDot)) return false;
    const bool bApplied = Target->ApplyWildlifeDamageFromServer(this, GetBaseAttackChannelsForArchetype(Archetype));
#if !UE_BUILD_SHIPPING
    if (bApplied && FParse::Param(FCommandLine::Get(), TEXT("KalmalaPeatHuskPeerTest")))
        ++PeatHuskSweepHitCountForVerification;
#endif
    return bApplied;
}

bool AKalmalaWildlifeSpawn::IsPeatHuskPositionDry(const FVector& Position) const
{
    if (Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY));
    const bool bWater = FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
    return IsMossyMireSpawnHabitatAllowed(Biome, bWater);
}

bool AKalmalaWildlifeSpawn::CanPeatHuskMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsPeatHuskPositionDry(Position)) return false;
    }
    return true;
}

void AKalmalaWildlifeSpawn::UpdateSnakeAmbush(const float DeltaSeconds)
{
    if (Behaviour != EKalmalaWildlifeBehaviour::Idle) return;
    const float StepSeconds = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue);
    const bool bGiantSnake = Archetype == EKalmalaWildlifeArchetype::GiantSnake;
    if (Cue == EKalmalaWildlifeAttackCue::None)
    {
        AKalmalaCharacter* Nearest = nullptr;
        float BestDistanceSquared = FMath::Square(bGiantSnake ? 620.0f : 480.0f);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsSnakeAttackTargetValid(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Nearest = Candidate;
            }
        }
        if (Nearest != nullptr) BeginSnakeAttack(Nearest);
        return;
    }

    AKalmalaCharacter* Target = SnakeAttackTarget.Get();
    if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
    {
        if (!IsSnakeAttackTargetValid(Target))
        {
            SnakeAttackTarget.Reset();
            SnakeAttackSecondsRemaining = 0.0f;
            SetSnakeAttackCue(EKalmalaWildlifeAttackCue::None);
            return;
        }
        SnakeAttackSecondsRemaining -= StepSeconds;
        if (SnakeAttackSecondsRemaining > 0.0f) return;

        const float MaxLunge = bGiantSnake ? 360.0f : 260.0f;
        SnakeLungeDistanceRemaining = FMath::Min(MaxLunge,
            FMath::Max(0.0f, FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) - 145.0f));
        SnakeAttackSecondsRemaining = 0.24f;
        SetSnakeAttackCue(EKalmalaWildlifeAttackCue::Lunge);
    }

    if (static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue) == EKalmalaWildlifeAttackCue::Lunge)
    {
        if (SnakeLungeDistanceRemaining > 0.0f && StepSeconds > 0.0f)
        {
            const float Speed = bGiantSnake ? 1500.0f : 1300.0f;
            const float MoveDistance = FMath::Min(SnakeLungeDistanceRemaining, Speed * StepSeconds);
            const FVector Start = GetActorLocation();
            FVector End = Start + SnakeLungeDirection * MoveDistance;
            const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
            if (State == nullptr)
            {
                SnakeLungeDistanceRemaining = 0.0f;
                SnakeAttackSecondsRemaining = 0.0f;
            }
            else
            {
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            }
            if (State == nullptr || (End - SpawnOrigin).Size2D() > SnakeSpawnLeashCentimetres
                || !CanSnakeMoveAlong(Start, End))
            {
                SnakeLungeDistanceRemaining = 0.0f;
            }
            else
            {
                SetActorLocation(End, true);
                const float Moved = FVector::Dist2D(Start, GetActorLocation());
                SnakeLungeDistanceRemaining = FMath::Max(0.0f, SnakeLungeDistanceRemaining - Moved);
                if (Moved <= KINDA_SMALL_NUMBER) SnakeLungeDistanceRemaining = 0.0f;
            }
        }
        SnakeAttackSecondsRemaining -= StepSeconds;
        if (SnakeLungeDistanceRemaining > 0.0f && SnakeAttackSecondsRemaining > 0.0f) return;

        TryApplySnakeBite(Target);
        SnakeAttackSecondsRemaining = bGiantSnake ? GiantSnakeRecoverySeconds : ViperRecoverySeconds;
        SetSnakeAttackCue(EKalmalaWildlifeAttackCue::Recovery);
    }
    else if (static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue) == EKalmalaWildlifeAttackCue::Recovery)
    {
        const FVector ToHome = SpawnOrigin - GetActorLocation();
        const float ReturnStep = FMath::Min(ToHome.Size2D(), (bGiantSnake ? 300.0f : 325.0f) * StepSeconds);
        if (ReturnStep > 0.0f)
        {
            FVector End = GetActorLocation() + ToHome.GetSafeNormal2D() * ReturnStep;
            if (const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
                End.Z = FKalmalaTerrainHeightSampler::SampleHeight(State->GetWorldGenerationConfig(), FVector2D(End));
            if (CanSnakeMoveAlong(GetActorLocation(), End)) SetActorLocation(End, true);
        }
        SnakeAttackSecondsRemaining -= StepSeconds;
        if (SnakeAttackSecondsRemaining <= 0.0f)
        {
            SnakeAttackTarget.Reset();
            SnakeAttackSecondsRemaining = 0.0f;
            SetSnakeAttackCue(EKalmalaWildlifeAttackCue::None);
        }
    }
}

void AKalmalaWildlifeSpawn::BeginSnakeAttack(AKalmalaCharacter* Target)
{
    if (!HasAuthority() || bDefeated || !IsSnakeAttackTargetValid(Target)) return;
    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    SnakeLungeDirection = ToTarget.GetSafeNormal2D();
    if (SnakeLungeDirection.IsNearlyZero()) return;
    SetActorRotation(FRotationMatrix::MakeFromX(SnakeLungeDirection).Rotator());
    SnakeAttackTarget = Target;
    SnakeAttackSecondsRemaining = Archetype == EKalmalaWildlifeArchetype::GiantSnake
        ? GiantSnakeTelegraphSeconds : ViperTelegraphSeconds;
    SnakeLungeDistanceRemaining = 0.0f;
    SetSnakeAttackCue(EKalmalaWildlifeAttackCue::Telegraph);
}

void AKalmalaWildlifeSpawn::SetSnakeAttackCue(const EKalmalaWildlifeAttackCue NextCue)
{
    if (!HasAuthority()) return;
    const uint8 NextValue = static_cast<uint8>(NextCue);
    if (SnakeAttackCue == NextValue) return;
    SnakeAttackCue = NextValue;
    BuildArchetypePresentation();
    if (NextCue == EKalmalaWildlifeAttackCue::Telegraph) PlaySnakeHiss();
    ForceNetUpdate();
}

bool AKalmalaWildlifeSpawn::IsSnakePositionDry(const FVector& Position) const
{
    if (Position.ContainsNaN() || FVector::DistSquared2D(Position, SpawnOrigin) > FMath::Square(SnakeSpawnLeashCentimetres))
        return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld() != nullptr
        ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    const EKalmalaBiome ExpectedBiome = Archetype == EKalmalaWildlifeArchetype::DeathAdder
        ? EKalmalaBiome::MossyMire : EKalmalaBiome::ShimmeringLakes;
    return FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY)) == ExpectedBiome
        && !FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
}

bool AKalmalaWildlifeSpawn::CanSnakeMoveAlong(const FVector& Start, const FVector& End) const
{
    const float Distance = FVector::Dist2D(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
    for (int32 Index = 1; Index <= Samples; ++Index)
    {
        const FVector SamplePosition = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        if (!IsSnakePositionDry(SamplePosition)) return false;
    }
    return true;
}

bool AKalmalaWildlifeSpawn::IsSnakeAttackTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || !IsValid(Target) || !Target->HasAuthority() || Target->GetWorld() != GetWorld()
        || !FMath::IsFinite(Target->GetHealth()) || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin) > FMath::Square(SnakeSpawnLeashCentimetres)
        || !IsSnakePositionDry(Target->GetActorLocation())) return false;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaSnakeBiteVisibility), true);
    QueryParams.AddIgnoredActor(this);
    FHitResult Blocker;
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 75.0f);
    const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
    return !GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)
        || Blocker.GetActor() == Target;
}

bool AKalmalaWildlifeSpawn::TryApplySnakeBite(AKalmalaCharacter* Target)
{
    if (!IsSnakeAttackTargetValid(Target)) return false;
    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float ForwardDistance = FVector::DotProduct(ToTarget, SnakeLungeDirection);
    const FVector LateralOffset = ToTarget - SnakeLungeDirection * FVector::DotProduct(ToTarget, SnakeLungeDirection);
    const float MaxLateral = Archetype == EKalmalaWildlifeArchetype::GiantSnake ? 115.0f : 72.0f;
    if (FVector::DistSquared(GetActorLocation(), Target->GetActorLocation())
            > FMath::Square(SnakeBiteReachCentimetres)
        || ForwardDistance < 0.25f || LateralOffset.Size2D() > MaxLateral) return false;

    const FKalmalaDamageChannels BiteDamage = GetBaseAttackChannelsForArchetype(Archetype);
    if (!Target->ApplyWildlifeDamageFromServer(this, BiteDamage)) return false;
    if (Archetype == EKalmalaWildlifeArchetype::Viper)
    {
        if (UKalmalaPlayerStatusComponent* Statuses = Target->GetStatusComponent())
            Statuses->ApplyWeakPoisonFromServer(ViperWeakPoisonDurationSeconds);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::DeathAdder)
    {
        if (UKalmalaPlayerStatusComponent* Statuses = Target->GetStatusComponent())
            Statuses->ApplyMajorPoisonFromServer(DeathAdderMajorPoisonDurationSeconds);
    }
    return true;
}

bool AKalmalaWildlifeSpawn::IsMosquitoPositionValid(const FVector& Position, float& OutGroundHeight) const
{
    OutGroundHeight = 0.0f;
    if (!HasAuthority() || Position.ContainsNaN() || GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid()) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const FVector2D XY(Position);
    if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY)) != EKalmalaBiome::MossyMire
        || FKalmalaOceanSampler::Sample(Config, XY).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, XY)) return false;
    OutGroundHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, XY);
    return FMath::IsFinite(OutGroundHeight);
}

bool AKalmalaWildlifeSpawn::CanMosquitoFlyAlong(const FVector& Start, const FVector& End, const AActor* Target) const
{
    if (!HasAuthority() || Start.ContainsNaN() || End.ContainsNaN() || GetWorld() == nullptr) return false;
    const float Distance = FVector::Dist(Start, End);
    const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 60.0f));
    for (int32 Index = 0; Index <= Samples; ++Index)
    {
        const FVector Point = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
        float GroundHeight = 0.0f;
        if (!IsMosquitoPositionValid(Point, GroundHeight)) return false;
        const float HeightAboveGround = Point.Z - GroundHeight;
        if (!FMath::IsFinite(HeightAboveGround) || HeightAboveGround < 50.0f || HeightAboveGround > 250.0f)
            return false;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaMosquitoFlightCollision), true);
    QueryParams.AddIgnoredActor(this);
    if (Target != nullptr) QueryParams.AddIgnoredActor(Target);
    FHitResult Blocker;
    return !GetWorld()->SweepSingleByChannel(Blocker, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(12.0f), QueryParams);
}

bool AKalmalaWildlifeSpawn::IsMosquitoRepelledByFire(const FVector& Position,
    const AKalmalaCharacter* Target, FVector& OutFireLocation, float& OutAvoidanceRadius) const
{
    OutFireLocation = FVector::ZeroVector;
    OutAvoidanceRadius = 0.0f;
    if (!HasAuthority() || Position.ContainsNaN() || GetWorld() == nullptr) return false;
    float NearestDistance = TNumericLimits<float>::Max();
    const auto ConsiderFlame = [this, &Position, Target, &OutFireLocation, &OutAvoidanceRadius,
        &NearestDistance](const AActor* Flame, const FVector& FlameLocation, const float Radius,
            const bool bActive) -> bool
    {
        if (!IsValid(Flame) || !IsMosquitoFireRepelling(HasAuthority(), bActive,
                FVector::Dist2D(Position, FlameLocation), Radius)) return false;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaMosquitoFlameVisibility), true);
        QueryParams.AddIgnoredActor(this);
        QueryParams.AddIgnoredActor(Flame);
        if (Target != nullptr) QueryParams.AddIgnoredActor(Target);
        FHitResult Blocker;
        const FVector Start = FlameLocation + FVector(0.0f, 0.0f, 24.0f);
        const FVector End = Position + FVector(0.0f, 0.0f, 24.0f);
        if (GetWorld()->LineTraceSingleByChannel(Blocker, Start, End, ECC_Visibility, QueryParams)) return false;
        const float Distance = FVector::DistSquared2D(Position, FlameLocation);
        if (Distance >= NearestDistance) return false;
        NearestDistance = Distance;
        OutFireLocation = FlameLocation;
        OutAvoidanceRadius = Radius;
        return true;
    };

    for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
    {
        AKalmalaCharacter* Character = *It;
        if (IsValid(Character) && Character->IsHeldTorchFlameEligibleForServer())
        {
            ConsiderFlame(Character, Character->GetActorLocation() + FVector(35.0f, 0.0f, 80.0f),
                MosquitoTorchAvoidanceCentimetres, true);
        }
    }
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        AKalmalaConstructionActor* Light = *It;
        if (IsValid(Light) && Light->IsStandingLightFlameEligibleForServer())
        {
            ConsiderFlame(Light, Light->GetActorLocation() + FVector(0.0f, 0.0f, 68.0f),
                MosquitoTorchAvoidanceCentimetres, true);
        }
    }
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
    {
        AKalmalaCampfire* Fire = *It;
        if (IsValid(Fire) && Fire->IsFlameEligibleForMosquitoesFromServer())
        {
            ConsiderFlame(Fire, Fire->GetActorLocation() + FVector(0.0f, 0.0f, 32.0f),
                MosquitoCampfireAvoidanceCentimetres, true);
        }
    }
    return OutAvoidanceRadius > 0.0f;
}

bool AKalmalaWildlifeSpawn::IsMosquitoTargetValid(const AKalmalaCharacter* Target) const
{
    if (!HasAuthority() || bDefeated || Archetype != EKalmalaWildlifeArchetype::MosquitoSwarm
        || GetWorld() == nullptr || !IsValid(Target) || !Target->HasAuthority()
        || Target->GetWorld() != GetWorld() || Target->GetHealth() <= 1.0f
        || FVector::DistSquared2D(Target->GetActorLocation(), SpawnOrigin)
            > FMath::Square(MosquitoSpawnLeashCentimetres)
        || Target->IsDodgeRollRepellingMosquitoesFromServer()) return false;

    FVector FireLocation;
    float FireRadius = 0.0f;
    if (IsMosquitoRepelledByFire(Target->GetActorLocation(), Target, FireLocation, FireRadius)) return false;
    return CanMosquitoFlyAlong(GetActorLocation(), Target->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), Target);
}

bool AKalmalaWildlifeSpawn::IsMosquitoTargetReservedByOther(const AKalmalaCharacter* Target) const
{
    if (!IsValid(Target) || GetWorld() == nullptr) return true;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
    {
        const AKalmalaWildlifeSpawn* Other = *It;
        if (Other != this && IsValid(Other) && !Other->bDefeated
            && Other->Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm
            && Other->MosquitoTarget.Get() == Target) return true;
    }
    return false;
}

bool AKalmalaWildlifeSpawn::TryApplyMosquitoBite(AKalmalaCharacter* Target)
{
    if (!IsMosquitoTargetValid(Target)
        || !IsMosquitoBiteInReach(HasAuthority(), true,
            FVector::Dist(GetActorLocation(), Target->GetActorLocation()))) return false;
    const FKalmalaDamageChannels Bite = GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype::MosquitoSwarm);
    if (!Target->ApplyWildlifeDamageFromServer(this, Bite)) return false;
#if !UE_BUILD_SHIPPING
    ++MosquitoAcceptedBiteCountForVerification;
#endif
    if (UKalmalaPlayerStatusComponent* Statuses = Target->GetStatusComponent())
        Statuses->ApplyWeakPoisonFromServer(MosquitoWeakPoisonDurationSeconds);
    return true;
}

void AKalmalaWildlifeSpawn::UpdateMosquitoSwarm(const float DeltaSeconds)
{
    if (!HasAuthority() || bDefeated || Behaviour != EKalmalaWildlifeBehaviour::Idle || GetWorld() == nullptr) return;
    const float Step = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    const auto MoveToward = [this, Step](FVector Destination, const AActor* Target)
    {
        float GroundHeight = 0.0f;
        if (!IsMosquitoPositionValid(Destination, GroundHeight)) return;
        const float Phase = MosquitoFlightElapsedSeconds * 3.1f
            + static_cast<float>(FCrc::StrCrc32(*PersistentSpawnId) % 628u) * 0.01f;
        Destination.Z = GroundHeight + MosquitoFlightHeightCentimetres
            + FMath::Sin(Phase) * MosquitoFlightAmplitudeCentimetres;
        const FVector Start = GetActorLocation();
        const FVector End = FMath::VInterpConstantTo(Start, Destination, Step,
            MosquitoFlightSpeedCentimetresPerSecond);
        if (!CanMosquitoFlyAlong(Start, End, Target)) return;
        SetActorLocation(End, true);
        const FVector Direction = End - Start;
        if (!Direction.IsNearlyZero()) SetActorRotation(FRotationMatrix::MakeFromX(Direction.GetSafeNormal2D()).Rotator());
    };

    FVector FireLocation;
    float FireRadius = 0.0f;
    if (IsMosquitoRepelledByFire(GetActorLocation(), nullptr, FireLocation, FireRadius))
    {
        MosquitoCurrentFireLocation = FireLocation;
        MosquitoTarget.Reset();
        MosquitoApproachSecondsRemaining = 0.0f;
        MosquitoBiteSecondsRemaining = 0.0f;
        FVector Away = (GetActorLocation() - FireLocation).GetSafeNormal2D();
        if (Away.IsNearlyZero()) Away = GetDeterministicOffset(1.0f).GetSafeNormal2D();
        MoveToward(FireLocation + Away * (FireRadius + 50.0f), nullptr);
        return;
    }
    MosquitoCurrentFireLocation = FVector::ZeroVector;

    AKalmalaCharacter* Target = MosquitoTarget.Get();
    if (Target != nullptr && !IsMosquitoTargetValid(Target))
    {
        MosquitoTarget.Reset();
        MosquitoApproachSecondsRemaining = 0.0f;
        MosquitoBiteSecondsRemaining = 0.0f;
        Target = nullptr;
    }
    if (Target == nullptr)
    {
        float BestDistanceSquared = FMath::Square(MosquitoSpawnLeashCentimetres);
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsMosquitoTargetValid(Candidate) || IsMosquitoTargetReservedByOther(Candidate)) continue;
            const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target != nullptr)
        {
            MosquitoTarget = Target;
            MosquitoApproachSecondsRemaining = MosquitoApproachSeconds;
            MosquitoBiteSecondsRemaining = 0.0f;
        }
    }

    if (Target == nullptr)
    {
        FVector Home = SpawnOrigin;
        float GroundHeight = 0.0f;
        if (IsMosquitoPositionValid(Home, GroundHeight)) MoveToward(Home, nullptr);
        return;
    }

    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    FVector Direction = ToTarget.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = GetActorForwardVector().GetSafeNormal2D();
    FVector HarassPoint = Target->GetActorLocation() - Direction * 70.0f;
    MoveToward(HarassPoint, Target);
    MosquitoApproachSecondsRemaining = FMath::Max(0.0f, MosquitoApproachSecondsRemaining - Step);
    if (MosquitoApproachSecondsRemaining > 0.0f || !IsMosquitoTargetValid(Target)) return;

    const float ContactDistance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
    if (IsMosquitoBiteInReach(true, true, ContactDistance))
    {
        MosquitoBiteSecondsRemaining -= Step;
        if (MosquitoBiteSecondsRemaining <= 0.0f)
        {
            TryApplyMosquitoBite(Target);
            MosquitoBiteSecondsRemaining = MosquitoBiteIntervalSeconds;
        }
    }
    else
    {
        MosquitoBiteSecondsRemaining = 0.0f;
    }
}

void AKalmalaWildlifeSpawn::UpdateMosquitoPresentation(const float DeltaSeconds)
{
    if (Archetype != EKalmalaWildlifeArchetype::MosquitoSwarm || bDefeated || MirelingMesh == nullptr
        || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
    MosquitoFlightElapsedSeconds += FMath::Min(DeltaSeconds, 0.25f);
    const float Phase = MosquitoFlightElapsedSeconds * 3.1f;
    MirelingMesh->SetRelativeRotation(FRotator(0.0f, FMath::Sin(Phase * 0.72f) * 16.0f, 0.0f));
}

void AKalmalaWildlifeSpawn::PlayMosquitoBuzz()
{
    if (MosquitoBuzzComponent != nullptr && MosquitoBuzzComponent->IsPlaying()) return;
    if (MosquitoBuzzSound == nullptr)
    {
        MosquitoBuzzSound = NewObject<USoundWaveProcedural>(this, TEXT("MosquitoSwarmBuzz"));
        MosquitoBuzzSound->SetSampleRate(22050);
        MosquitoBuzzSound->NumChannels = 1;
        MosquitoBuzzSound->Duration = 1.0f;
        MosquitoBuzzSound->bLooping = true;
        constexpr int32 SampleRate = 22050;
        MosquitoBuzzAudioData.SetNumUninitialized(SampleRate * sizeof(int16));
        for (int32 Index = 0; Index < SampleRate; ++Index)
        {
            const float Time = static_cast<float>(Index) / SampleRate;
            const float Modulation = 1.0f + 0.12f * FMath::Sin(2.0f * PI * 8.0f * Time);
            const float Wave = FMath::Sin(2.0f * PI * 184.0f * Modulation * Time)
                + 0.38f * FMath::Sin(2.0f * PI * 368.0f * Modulation * Time);
            const int16 Sample = static_cast<int16>(FMath::Clamp(Wave * 1700.0f, -32767.0f, 32767.0f));
            FMemory::Memcpy(MosquitoBuzzAudioData.GetData() + Index * sizeof(int16), &Sample, sizeof(Sample));
        }
        MosquitoBuzzSound->QueueAudio(MosquitoBuzzAudioData.GetData(), MosquitoBuzzAudioData.Num());
    }
    if (MosquitoBuzzAttenuation == nullptr)
    {
        MosquitoBuzzAttenuation = NewObject<USoundAttenuation>(this, TEXT("MosquitoBuzzAttenuation"));
        MosquitoBuzzAttenuation->Attenuation.bAttenuate = true;
        MosquitoBuzzAttenuation->Attenuation.bSpatialize = true;
        MosquitoBuzzAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        MosquitoBuzzAttenuation->Attenuation.AttenuationShapeExtents = FVector(0.0f, 0.0f, 0.0f);
        MosquitoBuzzAttenuation->Attenuation.FalloffDistance = 2500.0f;
    }
    MosquitoBuzzComponent = UGameplayStatics::SpawnSoundAttached(MosquitoBuzzSound, GetRootComponent(), NAME_None,
        FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, 0.12f, 1.0f,
        0.0f, MosquitoBuzzAttenuation, nullptr, true);
}

void AKalmalaWildlifeSpawn::StopMosquitoBuzz()
{
    if (MosquitoBuzzComponent != nullptr)
    {
        MosquitoBuzzComponent->FadeOut(0.08f, 0.0f);
        MosquitoBuzzComponent = nullptr;
    }
}

void AKalmalaWildlifeSpawn::PlaySnakeHiss()
{
    if (SnakeHissSound == nullptr)
    {
        SnakeHissSound = NewObject<USoundWaveProcedural>(this, TEXT("SnakeTelegraphHiss"));
        SnakeHissSound->SetSampleRate(22050);
        SnakeHissSound->NumChannels = 1;
        SnakeHissSound->Duration = 0.36f;
        SnakeHissSound->bLooping = false;
        constexpr int32 SampleRate = 22050;
        constexpr int32 SampleCount = SampleRate * 36 / 100;
        SnakeHissAudioData.SetNumUninitialized(SampleCount * sizeof(int16));
        float FilteredNoise = 0.0f;
        for (int32 Index = 0; Index < SampleCount; ++Index)
        {
            const float Noise = FMath::FRandRange(-1.0f, 1.0f);
            FilteredNoise = FMath::Lerp(FilteredNoise, Noise, 0.34f);
            const float Envelope = FMath::Min(1.0f, static_cast<float>(Index) / 180.0f)
                * FMath::Min(1.0f, static_cast<float>(SampleCount - Index) / 900.0f);
            const int16 Sample = static_cast<int16>(FMath::Clamp(FilteredNoise * Envelope * 3100.0f, -32767.0f, 32767.0f));
            FMemory::Memcpy(SnakeHissAudioData.GetData() + Index * sizeof(int16), &Sample, sizeof(Sample));
        }
    }
    SnakeHissSound->QueueAudio(SnakeHissAudioData.GetData(), SnakeHissAudioData.Num());
    UGameplayStatics::PlaySoundAtLocation(this, SnakeHissSound, GetActorLocation(), FRotator::ZeroRotator, 0.45f, 1.0f);
}

void AKalmalaWildlifeSpawn::BuildArchetypePresentation()
{
    MirelingMesh->ClearAllMeshSections();
    if (USphereComponent* RootSphere = Cast<USphereComponent>(GetRootComponent()))
        RootSphere->SetSphereRadius(Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm ? 28.0f
            : (Archetype == EKalmalaWildlifeArchetype::Rat ? 42.0f
                : (Archetype == EKalmalaWildlifeArchetype::ThunderSentinel ? 180.0f
                    : (Archetype == EKalmalaWildlifeArchetype::ReefMaw ? 92.0f : 70.0f))), true);
    const bool bSnake = Archetype == EKalmalaWildlifeArchetype::Viper
        || Archetype == EKalmalaWildlifeArchetype::GiantSnake
        || Archetype == EKalmalaWildlifeArchetype::DeathAdder;
    if (SnakeBodyCollision != nullptr)
    {
        if (bSnake)
        {
            const bool bGiant = Archetype == EKalmalaWildlifeArchetype::GiantSnake;
            const bool bDeathAdder = Archetype == EKalmalaWildlifeArchetype::DeathAdder;
            SnakeBodyCollision->SetCapsuleSize(bGiant ? 38.0f : (bDeathAdder ? 21.0f : 16.0f),
                bGiant ? 210.0f : (bDeathAdder ? 112.0f : 92.0f), true);
            SnakeBodyCollision->SetRelativeLocation(FVector(-22.0f, 0.0f, 0.0f));
            SnakeBodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
        else
        {
            SnakeBodyCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    // Original low-poly silhouettes use only project procedural geometry and vertex colour.
    TArray<FVector> Vertices; TArray<int32> Triangles; TArray<FVector> Normals; TArray<FVector2D> UV; TArray<FLinearColor> Colors;
    const auto AddTetraPoints = [&Vertices, &Triangles, &Normals, &UV, &Colors](const FVector& Point0, const FVector& Point1, const FVector& Point2, const FVector& Point3, const FLinearColor Colour)
    {
        const FVector Points[] = { Point0, Point1, Point2, Point3 };
        const FVector Centroid = (Point0 + Point1 + Point2 + Point3) * 0.25f;
        const int32 Faces[] = { 0,2,1, 0,1,3, 1,2,3, 2,0,3 };
        for (int32 Face = 0; Face < UE_ARRAY_COUNT(Faces); Face += 3)
        {
            int32 A = Faces[Face];
            int32 B = Faces[Face + 1];
            int32 C = Faces[Face + 2];
            FVector Normal = FVector::CrossProduct(Points[B] - Points[A], Points[C] - Points[A]).GetSafeNormal();
            const FVector FaceCentre = (Points[A] + Points[B] + Points[C]) / 3.0f;
            if (FVector::DotProduct(Normal, FaceCentre - Centroid) < 0.0f)
            {
                Swap(B, C);
                Normal *= -1.0f;
            }
            const int32 OutwardFace[] = { A, B, C };
            const int32 FirstVertex = Vertices.Num();
            // Keep outward normals; reverse only indices for Unreal's clockwise front faces.
            Triangles.Append({FirstVertex, FirstVertex + 2, FirstVertex + 1});
            for (const int32 Vertex : OutwardFace) { Vertices.Add(Points[Vertex]); Normals.Add(Normal); UV.Add(FVector2D::ZeroVector); Colors.Add(Colour); }
        }
    };
    const auto AddTetra = [&AddTetraPoints](const FVector Centre, const FVector Extent, const FLinearColor Colour)
    {
        AddTetraPoints(Centre + FVector(-Extent.X,-Extent.Y,0), Centre + FVector(Extent.X,-Extent.Y,0),
            Centre + FVector(0,Extent.Y,0), Centre + FVector(0,0,Extent.Z), Colour);
    };
    if (Archetype == EKalmalaWildlifeArchetype::Boar)
    {
        const FLinearColor BoarHide(0.20f, 0.11f, 0.065f);
        const FLinearColor BoarShoulder(0.27f, 0.15f, 0.08f);
        const FLinearColor Bristle(0.12f, 0.075f, 0.045f);
        const FLinearColor Muzzle(0.34f, 0.21f, 0.13f);
        const FLinearColor Tusk(0.82f, 0.75f, 0.57f);
        const FLinearColor Eye(0.055f, 0.035f, 0.02f);

        // A low, wedge-backed mass gives the boar a distinct profile from the taller deer.
        AddTetraPoints(FVector(-108,-42,-2), FVector(-108,42,-2), FVector(48,0,8), FVector(-34,0,68), BoarHide);
        AddTetraPoints(FVector(-72,-43,0), FVector(-72,43,0), FVector(40,0,13), FVector(-27,0,79), BoarShoulder);
        AddTetraPoints(FVector(17,-31,12), FVector(17,31,12), FVector(96,0,6), FVector(54,0,53), BoarHide);
        AddTetraPoints(FVector(87,-20,10), FVector(87,20,10), FVector(145,0,1), FVector(104,0,31), Muzzle);

        // Four compact legs keep the belly close to the ground without extending below the old mesh.
        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-64.0f, 37.0f})
            {
                const float LegY = Side * 35.0f;
                AddTetraPoints(FVector(LegX - 12.0f, LegY - 9.0f, -52.0f),
                    FVector(LegX - 12.0f, LegY + 9.0f, -52.0f),
                    FVector(LegX + 5.0f, LegY, -5.0f), FVector(LegX + 20.0f, LegY, -2.0f), Bristle);
            }

            // Short ears, side-set eyes, and upward ivory tusks make the head readable in profile.
            AddTetraPoints(FVector(25, Side * 19.0f, 42), FVector(25, Side * 35.0f, 42),
                FVector(12, Side * 31.0f, 68), FVector(43, Side * 29.0f, 50), Bristle);
            AddTetraPoints(FVector(64, Side * 23.0f, 19), FVector(74, Side * 22.0f, 20),
                FVector(69, Side * 30.0f, 27), FVector(72, Side * 25.0f, 31), Eye);
            AddTetraPoints(FVector(78, Side * 19.0f, 3), FVector(91, Side * 23.0f, 3),
                FVector(103, Side * 17.0f, 28), FVector(109, Side * 23.0f, 6), Tusk);
        }

        // A small broken ridge of coarse bristles emphasizes the shoulder without changing the actor scale.
        AddTetraPoints(FVector(-78,-7,52), FVector(-78,7,52), FVector(-60,0,78), FVector(-48,0,49), Bristle);
        AddTetraPoints(FVector(-46,-7,64), FVector(-46,7,64), FVector(-28,0,86), FVector(-16,0,59), Bristle);
        AddTetraPoints(FVector(-13,-7,60), FVector(-13,7,60), FVector(4,0,78), FVector(16,0,54), Bristle);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Deer)
    {
        const FLinearColor DeerHide(0.43f, 0.27f, 0.14f);
        const FLinearColor DeerShoulder(0.51f, 0.33f, 0.18f);
        const FLinearColor DeerLeg(0.25f, 0.16f, 0.09f);
        const FLinearColor DeerHoof(0.12f, 0.085f, 0.055f);
        const FLinearColor RumpMark(0.68f, 0.52f, 0.32f);
        const FLinearColor Muzzle(0.60f, 0.39f, 0.23f);
        const FLinearColor Antler(0.73f, 0.60f, 0.39f);
        const FLinearColor Eye(0.07f, 0.045f, 0.025f);

        // A narrow raised shoulder and long legs give the wary herd animal a light, alert outline.
        AddTetraPoints(FVector(-104,-24,9), FVector(-104,24,9), FVector(5,-22,27), FVector(-49,0,72), DeerHide);
        AddTetraPoints(FVector(-55,-25,13), FVector(-55,25,13), FVector(52,-16,35), FVector(12,0,78), DeerShoulder);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(-99,Side * 22.0f,29), FVector(-85,Side * 24.0f,28),
                FVector(-72,Side * 20.0f,48), FVector(-83,Side * 16.0f,57), RumpMark);
        }

        // The narrow neck reaches forward from the shoulder into a raised head and tapered muzzle.
        AddTetraPoints(FVector(26,-18,39), FVector(26,18,39), FVector(77,-10,84), FVector(59,0,108), DeerShoulder);
        AddTetraPoints(FVector(69,-12,91), FVector(69,12,91), FVector(115,0,87), FVector(99,0,119), DeerHide);
        AddTetraPoints(FVector(101,-7,88), FVector(101,7,88), FVector(137,0,81), FVector(123,0,98), Muzzle);

        for (const float Side : {-1.0f, 1.0f})
        {
            // Side-set eyes and pointed ears make the forward-facing head legible at a distance.
            AddTetraPoints(FVector(100,Side * 10.0f,99), FVector(108,Side * 10.0f,98),
                FVector(109,Side * 16.0f,101), FVector(105,Side * 12.0f,104), Eye);
            AddTetraPoints(FVector(79,Side * 10.0f,105), FVector(72,Side * 16.0f,103),
                FVector(52,Side * 29.0f,121), FVector(81,Side * 27.0f,117), DeerShoulder);

            // Four separated leg lines and small dark hooves keep the body visibly off the ground.
            const auto AddLongLeg = [&AddTetraPoints, &DeerLeg, &DeerHoof](const float HipX, const float KneeX, const float LegSide)
            {
                AddTetraPoints(FVector(HipX - 8.0f, LegSide * 19.0f, 32), FVector(HipX + 8.0f, LegSide * 23.0f, 29),
                    FVector(KneeX + 7.0f, LegSide * 17.0f, -7), FVector(KneeX - 6.0f, LegSide * 20.0f, -19), DeerLeg);
                AddTetraPoints(FVector(KneeX - 6.0f, LegSide * 17.0f, -16), FVector(KneeX + 7.0f, LegSide * 19.0f, -17),
                    FVector(KneeX + 5.0f, LegSide * 15.0f, -45), FVector(KneeX - 5.0f, LegSide * 17.0f, -54), DeerLeg);
                AddTetraPoints(FVector(KneeX - 8.0f, LegSide * 12.0f, -51), FVector(KneeX + 8.0f, LegSide * 12.0f, -51),
                    FVector(KneeX + 12.0f, LegSide * 11.0f, -58), FVector(KneeX - 12.0f, LegSide * 11.0f, -58), DeerHoof);
            };
            AddLongLeg(-61.0f, -70.0f, Side);
            AddLongLeg(36.0f, 43.0f, Side);

            // Keep the paired antlers, with a forked backward tine on each warm ivory beam.
            AddTetraPoints(FVector(83,Side * 8.0f,110), FVector(90,Side * 11.0f,112),
                FVector(87,Side * 27.0f,127), FVector(77,Side * 33.0f,139), Antler);
            AddTetraPoints(FVector(86,Side * 24.0f,126), FVector(92,Side * 28.0f,129),
                FVector(101,Side * 34.0f,139), FVector(97,Side * 38.0f,144), Antler);
            AddTetraPoints(FVector(80,Side * 29.0f,132), FVector(86,Side * 33.0f,135),
                FVector(67,Side * 41.0f,140), FVector(59,Side * 44.0f,145), Antler);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Rat)
    {
        const bool bTelegraph = static_cast<EKalmalaRatAttackCue>(RatAttackCue)
            == EKalmalaRatAttackCue::Telegraph;
        const FLinearColor Fur(0.34f, 0.25f, 0.17f);
        const FLinearColor FurDark(0.18f, 0.13f, 0.09f);
        const FLinearColor Belly(0.68f, 0.57f, 0.42f);
        const FLinearColor Ear(0.70f, 0.31f, 0.29f);
        const FLinearColor Eye(0.035f, 0.022f, 0.015f);
        const FLinearColor Tooth(0.92f, 0.86f, 0.70f);

        // Low oval body, long bare tail and large ears keep the tiny scavenger distinct from the herd.
        AddTetraPoints(FVector(-58,-22,10), FVector(-58,22,10), FVector(28,-17,10), FVector(-10,0,40), FurDark);
        AddTetraPoints(FVector(-48,-20,12), FVector(-48,20,12), FVector(24,-15,12), FVector(-8,0,bTelegraph ? 46.0f : 38.0f), Fur);
        AddTetraPoints(FVector(-36,-9,13), FVector(-36,9,13), FVector(18,-6,15), FVector(-9,0,24), Belly);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(-38,Side * 14,6), FVector(-26,Side * 18,6), FVector(-28,Side * 13,22), FVector(-18,Side * 14,18), FurDark);
            AddTetraPoints(FVector(4,Side * 13,6), FVector(18,Side * 16,6), FVector(10,Side * 12,21), FVector(22,Side * 12,17), FurDark);
            AddTetraPoints(FVector(32,Side * 8,24), FVector(40,Side * 10,24), FVector(37,Side * 14,57), FVector(24,Side * 13,39), Fur);
            AddTetraPoints(FVector(34,Side * 10,31), FVector(38,Side * 11,31), FVector(36,Side * 13,49), FVector(29,Side * 13,38), Ear);
            AddTetraPoints(FVector(48,Side * 9,28), FVector(55,Side * 8,27), FVector(52,Side * 12,32), FVector(50,Side * 9,34), Eye);
            AddTetraPoints(FVector(62,Side * 1.5f,19), FVector(65,Side * 3.0f,19), FVector(72,Side * 1.0f,20), FVector(68,Side * 1.5f,23), Tooth);
        }
        AddTetraPoints(FVector(-62,-3,10), FVector(-62,3,10), FVector(-89,-5,14), FVector(-91,5,13), FurDark);
        AddTetraPoints(FVector(-89,-5,14), FVector(-91,5,13), FVector(-118,-5,8), FVector(-121,4,11), Fur);
        AddTetraPoints(FVector(-118,-5,8), FVector(-121,4,11), FVector(-143,-2,14), FVector(-145,3,17), FurDark);
        if (bTelegraph)
        {
            // A dark open mouth and lifted muzzle mark the half-second defensive bite warning.
            AddTetraPoints(FVector(57,-5,17), FVector(57,5,17), FVector(70,-4,13), FVector(70,4,13), FurDark);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Badger)
    {
        const bool bWarning = static_cast<EKalmalaWildlifeAttackCue>(BadgerAttackCue)
            == EKalmalaWildlifeAttackCue::Telegraph;
        const FLinearColor Coat(0.12f, 0.105f, 0.09f);
        const FLinearColor CoatLight(0.24f, 0.20f, 0.16f);
        const FLinearColor FaceStripe(0.78f, 0.72f, 0.60f);
        const FLinearColor Muzzle(0.44f, 0.40f, 0.34f);
        const FLinearColor Claw(0.82f, 0.74f, 0.57f);
        const FLinearColor Eye(0.035f, 0.024f, 0.018f);

        // A low, broad back and white facial stripe make the territorial badger
        // distinct from the Meadow rat and the taller forest deer.
        AddTetraPoints(FVector(-88,-37,13), FVector(-88,37,13), FVector(17,-31,22),
            FVector(-35,0,bWarning ? 76.0f : 63.0f), Coat);
        AddTetraPoints(FVector(-63,-31,24), FVector(-63,31,24), FVector(6,-26,31),
            FVector(-31,0,bWarning ? 88.0f : 76.0f), CoatLight);
        AddTetraPoints(FVector(4,-26,24), FVector(4,26,24), FVector(50,-18,35),
            FVector(34,0,bWarning ? 89.0f : 72.0f), Coat);
        AddTetraPoints(FVector(35,-17,28), FVector(35,17,28), FVector(76,-12,38),
            FVector(64,0,bWarning ? 79.0f : 65.0f), Muzzle);
        AddTetraPoints(FVector(59,-8,bWarning ? 48.0f : 43.0f),
            FVector(59,8,bWarning ? 48.0f : 43.0f), FVector(83,-4,38), FVector(83,4,38), FaceStripe);

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-58.0f, 37.0f})
            {
                const float LegY = Side * 27.0f;
                AddTetraPoints(FVector(LegX - 13.0f, LegY - 9.0f, -51.0f),
                    FVector(LegX - 13.0f, LegY + 9.0f, -51.0f),
                    FVector(LegX + 7.0f, LegY, 18.0f), FVector(LegX + 17.0f, LegY, 22.0f), Coat);
                AddTetraPoints(FVector(LegX + 5.0f, LegY - 8.0f, -50.0f),
                    FVector(LegX + 5.0f, LegY + 8.0f, -50.0f),
                    FVector(LegX + 23.0f, LegY, -47.0f), FVector(LegX + 22.0f, LegY, -42.0f), Claw);
            }
            AddTetraPoints(FVector(55,Side * 11.0f,53), FVector(63,Side * 12.0f,53),
                FVector(63,Side * 16.0f,59), FVector(57,Side * 15.0f,64), Eye);
            AddTetraPoints(FVector(28,Side * 17.0f,65), FVector(41,Side * 21.0f,64),
                FVector(31,Side * 22.0f,77), FVector(19,Side * 20.0f,76), Coat);
        }
        if (bWarning)
        {
            // The raised head and visible claws telegraph the common 0.5 s strike.
            AddTetraPoints(FVector(59,-5,28), FVector(59,5,28), FVector(78,-4,22),
                FVector(78,4,22), Coat);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::MoonBear)
    {
        const bool bWarning = static_cast<EKalmalaWildlifeAttackCue>(MoonBearAttackCue)
            == EKalmalaWildlifeAttackCue::Telegraph;
        const FLinearColor Coat(0.18f, 0.13f, 0.095f);
        const FLinearColor Shoulder(0.30f, 0.22f, 0.15f);
        const FLinearColor Muzzle(0.56f, 0.43f, 0.30f);
        const FLinearColor Claw(0.78f, 0.67f, 0.49f);
        const FLinearColor Warning(0.96f, 0.68f, 0.20f);
        const FLinearColor Eye(0.045f, 0.025f, 0.012f);

        // A broad hump, barrel body, round ears and heavy paws keep the bear distinct from the low badger.
        AddTetraPoints(FVector(-116,-43,4), FVector(-116,43,4), FVector(27,-49,14),
            FVector(-43,0,132), Coat);
        AddTetraPoints(FVector(-67,-45,38), FVector(-67,45,38), FVector(52,-47,41),
            FVector(-16,0,151), Shoulder);
        AddTetraPoints(FVector(34,-45,30), FVector(34,45,30), FVector(98,-32,51),
            FVector(72,0,128), Coat);
        AddTetraPoints(FVector(72,-27,49), FVector(72,27,49), FVector(127,-16,57),
            FVector(105,0,bWarning ? 132.0f : 108.0f), Shoulder);
        AddTetraPoints(FVector(111,-16,48), FVector(111,16,48), FVector(150,-10,52),
            FVector(137,0,78), Muzzle);

        for (const float Side : {-1.0f, 1.0f})
        {
            // Thick legs end in pale hooked claws; the near paw lifts high before the overhead strike.
            for (const float LegX : {-72.0f, 42.0f})
            {
                const bool bRaisedPaw = bWarning && LegX > 0.0f && Side < 0.0f;
                const float PawX = bRaisedPaw ? 121.0f : LegX + 21.0f;
                const float PawY = Side * (bRaisedPaw ? 43.0f : 45.0f);
                const float PawZ = bRaisedPaw ? 117.0f : -49.0f;
                AddTetraPoints(FVector(LegX - 21.0f, Side * 33.0f, -51.0f),
                    FVector(LegX - 21.0f, Side * 55.0f, -51.0f),
                    FVector(LegX + 12.0f, Side * 34.0f, 30.0f),
                    FVector(LegX + 18.0f, Side * 52.0f, 38.0f), Coat);
                AddTetraPoints(FVector(LegX + 6.0f, Side * 34.0f, -48.0f),
                    FVector(LegX + 6.0f, Side * 52.0f, -48.0f),
                    FVector(PawX, PawY - 10.0f, PawZ), FVector(PawX, PawY + 10.0f, PawZ + 12.0f),
                    bRaisedPaw ? Warning : Shoulder);
                if (!bRaisedPaw)
                {
                    AddTetraPoints(FVector(PawX - 5.0f, PawY - 8.0f, -48.0f),
                        FVector(PawX - 5.0f, PawY + 8.0f, -48.0f),
                        FVector(PawX + 11.0f, PawY, -54.0f), FVector(PawX + 9.0f, PawY, -49.0f), Claw);
                }
            }
            AddTetraPoints(FVector(119,Side * 13.0f,78), FVector(127,Side * 14.0f,78),
                FVector(123,Side * 23.0f,92), FVector(115,Side * 22.0f,90), Coat);
            AddTetraPoints(FVector(135,Side * 10.0f,78), FVector(142,Side * 9.0f,77),
                FVector(140,Side * 14.0f,81), FVector(137,Side * 11.0f,83), Eye);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PolarBear)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(PolarBearAttackCue);
        const bool bWarning = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bCharging = Cue == EKalmalaWildlifeAttackCue::Lunge;
        const bool bRetreating = Cue == EKalmalaWildlifeAttackCue::Retreat;
        const FLinearColor SnowCoat(0.86f, 0.83f, 0.75f);
        const FLinearColor ShadowCoat(0.66f, 0.63f, 0.57f);
        const FLinearColor Cream(0.96f, 0.91f, 0.80f);
        const FLinearColor Nose(0.16f, 0.13f, 0.12f);
        const FLinearColor Eye(0.10f, 0.09f, 0.08f);
        const float HeadX = bCharging ? 148.0f : 128.0f;
        const float HeadZ = bWarning ? 109.0f : (bRetreating ? 128.0f : 139.0f);
        const float ShoulderZ = bWarning ? 101.0f : (bCharging ? 119.0f : 111.0f);

        // Broad shoulders, pale coat and long muzzle distinguish the original
        // Tundra charger from the smaller Elderwood Moon Bear.
        AddTetraPoints(FVector(-111,-42,50), FVector(-111,42,50),
            FVector(66,-58,54), FVector(58,58,54), ShadowCoat);
        AddTetraPoints(FVector(-95,-50,86), FVector(-95,50,86),
            FVector(39,-61,ShoulderZ), FVector(39,61,ShoulderZ), SnowCoat);
        AddTetraPoints(FVector(-65,-47,116), FVector(-65,47,116),
            FVector(37,-48,155), FVector(37,48,155), Cream);
        AddTetraPoints(FVector(14,-46,107), FVector(14,46,107),
            FVector(100,-38,132), FVector(100,38,132), SnowCoat);
        AddTetraPoints(FVector(HeadX - 35.0f,-29,HeadZ - 20.0f),
            FVector(HeadX - 35.0f,29,HeadZ - 20.0f),
            FVector(HeadX + 9.0f,-27,HeadZ + 8.0f),
            FVector(HeadX + 9.0f,27,HeadZ + 8.0f), Cream);
        AddTetraPoints(FVector(HeadX - 2.0f,-23,HeadZ - 14.0f),
            FVector(HeadX - 2.0f,23,HeadZ - 14.0f),
            FVector(HeadX + 49.0f,-16,HeadZ - 31.0f),
            FVector(HeadX + 49.0f,16,HeadZ - 31.0f), SnowCoat);
        AddTetraPoints(FVector(HeadX + 40.0f,-13,HeadZ - 31.0f),
            FVector(HeadX + 40.0f,13,HeadZ - 31.0f),
            FVector(HeadX + 64.0f,-10,HeadZ - 36.0f),
            FVector(HeadX + 64.0f,10,HeadZ - 36.0f), Nose);

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-74.0f, 42.0f})
            {
                const bool bFrontLeg = LegX > 0.0f;
                const bool bBracedPaw = bWarning && bFrontLeg && Side < 0.0f;
                const float PawX = bBracedPaw ? LegX + 45.0f : LegX + (bCharging && bFrontLeg ? 48.0f : 25.0f);
                const float PawY = Side * (bBracedPaw ? 43.0f : 49.0f);
                const float PawZ = bBracedPaw ? -7.0f : -51.0f;
                AddTetraPoints(FVector(LegX - 25.0f,Side * 32.0f,-52),
                    FVector(LegX - 25.0f,Side * 57.0f,-52),
                    FVector(LegX + 16.0f,Side * 37.0f,56),
                    FVector(LegX + 19.0f,Side * 54.0f,59), SnowCoat);
                AddTetraPoints(FVector(LegX + 8.0f,Side * 36.0f,-48),
                    FVector(LegX + 8.0f,Side * 53.0f,-48),
                    FVector(PawX,PawY - 14.0f,PawZ), FVector(PawX,PawY + 14.0f,PawZ + 9.0f),
                    bBracedPaw ? Cream : ShadowCoat);
                if (!bBracedPaw)
                    AddTetraPoints(FVector(PawX - 8.0f,PawY - 11.0f,-49),
                        FVector(PawX - 8.0f,PawY + 11.0f,-49),
                        FVector(PawX + 13.0f,PawY - 5.0f,-56),
                        FVector(PawX + 13.0f,PawY + 5.0f,-56), Cream);
            }
            const float EarX = HeadX - 12.0f;
            AddTetraPoints(FVector(EarX - 7.0f,Side * 20.0f,HeadZ + 1.0f),
                FVector(EarX + 5.0f,Side * 20.0f,HeadZ + 1.0f),
                FVector(EarX - 4.0f,Side * 33.0f,HeadZ + 23.0f),
                FVector(EarX + 5.0f,Side * 31.0f,HeadZ + 19.0f), ShadowCoat);
            AddTetraPoints(FVector(HeadX - 3.0f,Side * 19.0f,HeadZ - 12.0f),
                FVector(HeadX + 4.0f,Side * 19.0f,HeadZ - 10.0f),
                FVector(HeadX + 11.0f,Side * 26.0f,HeadZ - 8.0f),
                FVector(HeadX + 8.0f,Side * 27.0f,HeadZ - 2.0f), Eye);
        }
        if (bWarning)
        {
            // A braced paw and lowered muzzle give the charge a full second warning.
            AddTetraPoints(FVector(HeadX + 4.0f,-8,HeadZ - 29.0f),
                FVector(HeadX + 4.0f,8,HeadZ - 29.0f),
                FVector(HeadX + 25.0f,-8,HeadZ - 33.0f),
                FVector(HeadX + 25.0f,8,HeadZ - 33.0f), Nose);
        }
        else if (bCharging || bRetreating)
        {
            AddTetraPoints(FVector(HeadX + 28.0f,-8,HeadZ - 31.0f),
                FVector(HeadX + 28.0f,8,HeadZ - 31.0f),
                FVector(HeadX + 45.0f,-7,HeadZ - 27.0f),
                FVector(HeadX + 45.0f,7,HeadZ - 27.0f), Nose);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::HornedGoat)
    {
        const bool bWarning = static_cast<EKalmalaWildlifeAttackCue>(HornedGoatAttackCue)
            == EKalmalaWildlifeAttackCue::Telegraph;
        const FLinearColor Coat(0.34f, 0.27f, 0.19f);
        const FLinearColor CoatLight(0.53f, 0.43f, 0.30f);
        const FLinearColor Neck(0.42f, 0.34f, 0.23f);
        const FLinearColor Horn(0.78f, 0.69f, 0.51f);
        const FLinearColor Hoof(0.12f, 0.10f, 0.08f);
        const FLinearColor Eye(0.95f, 0.58f, 0.12f);
        const float HeadZ = bWarning ? 49.0f : 65.0f;

        // A compact ridged body, split hooves, beard and swept-back paired horns
        // identify the Mountain goat; its lowered warning pose precedes the charge.
        AddTetraPoints(FVector(-84,-32,12), FVector(-84,32,12), FVector(36,-31,18),
            FVector(-26,0,bWarning ? 74.0f : 69.0f), Coat);
        AddTetraPoints(FVector(-53,-27,30), FVector(-53,27,30), FVector(7,-27,31),
            FVector(-18,0,88.0f), CoatLight);
        AddTetraPoints(FVector(2,-27,19), FVector(2,27,19), FVector(54,-22,34),
            FVector(35,0,77.0f), Neck);
        AddTetraPoints(FVector(36,-20,28), FVector(36,20,28), FVector(83,-15,36),
            FVector(67,0,HeadZ), CoatLight);
        AddTetraPoints(FVector(70,-14,HeadZ - 8.0f), FVector(70,14,HeadZ - 8.0f),
            FVector(104,-8,HeadZ - 12.0f), FVector(104,8,HeadZ - 12.0f), Coat);
        AddTetraPoints(FVector(101,-8,HeadZ - 12.0f), FVector(101,8,HeadZ - 12.0f),
            FVector(112,-6,HeadZ - 18.0f), FVector(112,6,HeadZ - 18.0f), Hoof);

        for (const float LegX : {-58.0f, 35.0f})
        {
            for (const float Side : {-1.0f, 1.0f})
            {
                AddTetraPoints(FVector(LegX - 8.0f,Side * 18.0f,22.0f),
                    FVector(LegX + 8.0f,Side * 21.0f,18.0f),
                    FVector(LegX + 6.0f,Side * 18.0f,-42.0f),
                    FVector(LegX - 5.0f,Side * 18.0f,-42.0f), Coat);
                AddTetraPoints(FVector(LegX - 7.0f,Side * 18.0f,-39.0f),
                    FVector(LegX + 7.0f,Side * 18.0f,-39.0f),
                    FVector(LegX + 11.0f,Side * 20.0f,-52.0f),
                    FVector(LegX - 11.0f,Side * 20.0f,-52.0f), Hoof);
                AddTetraPoints(FVector(77,Side * 11.0f,HeadZ + 9.0f),
                    FVector(82,Side * 13.0f,HeadZ + 24.0f),
                    FVector(63,Side * 18.0f,HeadZ + 38.0f),
                    FVector(46,Side * 20.0f,HeadZ + 34.0f), Horn);
                AddTetraPoints(FVector(63,Side * 18.0f,HeadZ + 38.0f),
                    FVector(46,Side * 20.0f,HeadZ + 34.0f),
                    FVector(26,Side * 21.0f,HeadZ + 23.0f),
                    FVector(19,Side * 22.0f,HeadZ + 11.0f), Horn);
                AddTetraPoints(FVector(75,Side * 13.0f,HeadZ - 2.0f),
                    FVector(80,Side * 14.0f,HeadZ + 1.0f),
                    FVector(83,Side * 14.0f,HeadZ - 4.0f),
                    FVector(78,Side * 14.0f,HeadZ - 6.0f), Eye);
            }
        }
        AddTetraPoints(FVector(89,-6,HeadZ - 11.0f), FVector(89,6,HeadZ - 11.0f),
            FVector(95,-7,HeadZ - 35.0f), FVector(95,7,HeadZ - 35.0f), Coat);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::ThunderSentinel)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ThunderSentinelAttackCue);
        const bool bWarning = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bSlamming = Cue == EKalmalaWildlifeAttackCue::Lunge;
        const FLinearColor StoneBase(0.24f, 0.30f, 0.34f);
        const FLinearColor StoneLight(0.40f, 0.47f, 0.49f);
        const FLinearColor StoneShadow(0.12f, 0.17f, 0.21f);
        const FLinearColor ThunderVein(0.18f, 0.66f, 0.79f);
        const FLinearColor Warning(0.98f, 0.59f, 0.14f);
        const float ShoulderZ = bWarning ? 304.0f : 274.0f;
        const float RaisedFistZ = bWarning ? 416.0f : (bSlamming ? 18.0f : 192.0f);
        const float RaisedFistX = bSlamming ? 188.0f : 108.0f;

        // An original towering stone silhouette uses broad plates, split feet,
        // cyan mineral seams, and a raised-then-descending arm for the slam cue.
        AddTetraPoints(FVector(-196,-98,132), FVector(-196,98,132),
            FVector(128,-106,154), FVector(128,106,154), StoneShadow);
        AddTetraPoints(FVector(-151,-112,166), FVector(-151,112,166),
            FVector(98,-121,205), FVector(98,121,205), StoneBase);
        AddTetraPoints(FVector(-104,-99,203), FVector(-104,99,203),
            FVector(62,-92,ShoulderZ - 14.0f), FVector(62,92,ShoulderZ - 14.0f), StoneLight);
        AddTetraPoints(FVector(10,-72,208), FVector(10,72,208),
            FVector(98,-58,210), FVector(98,58,210), StoneBase);
        AddTetraPoints(FVector(20,-73,207), FVector(20,73,207),
            FVector(62,-39,287), FVector(62,39,287), StoneLight);
        AddTetraPoints(FVector(60,-43,265), FVector(60,43,265),
            FVector(115,-35,258), FVector(115,35,258), StoneShadow);
        AddTetraPoints(FVector(18,-105,ShoulderZ - 36.0f), FVector(18,-8,ShoulderZ - 38.0f),
            FVector(84,-88,ShoulderZ + 26.0f), FVector(83,-25,ShoulderZ + 22.0f), StoneLight);
        AddTetraPoints(FVector(40,-93,ShoulderZ + 10.0f), FVector(85,-80,ShoulderZ - 6.0f),
            FVector(RaisedFistX,-105,RaisedFistZ + 30.0f), FVector(RaisedFistX,-59,RaisedFistZ + 18.0f),
            bWarning ? Warning : StoneBase);
        AddTetraPoints(FVector(RaisedFistX - 19.0f,-110,RaisedFistZ - 14.0f),
            FVector(RaisedFistX + 19.0f,-110,RaisedFistZ - 14.0f),
            FVector(RaisedFistX - 13.0f,-64,RaisedFistZ + 21.0f),
            FVector(RaisedFistX + 13.0f,-64,RaisedFistZ + 21.0f), bSlamming ? Warning : StoneLight);
        AddTetraPoints(FVector(-138,-94,166), FVector(-138,94,166),
            FVector(-117,-75,54), FVector(-117,75,54), StoneBase);
        AddTetraPoints(FVector(-91,-78,151), FVector(-91,78,151),
            FVector(-70,-60,38), FVector(-70,60,38), StoneLight);
        AddTetraPoints(FVector(73,-74,159), FVector(73,74,159),
            FVector(91,-52,46), FVector(91,52,46), StoneBase);
        AddTetraPoints(FVector(112,-55,141), FVector(112,55,141),
            FVector(130,-39,32), FVector(130,39,32), StoneLight);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(61,Side * 31.0f,274), FVector(76,Side * 29.0f,274),
                FVector(75,Side * 32.0f,287), FVector(60,Side * 33.0f,286), bWarning ? Warning : ThunderVein);
            AddTetraPoints(FVector(89,Side * 23.0f,252), FVector(101,Side * 22.0f,252),
                FVector(100,Side * 24.0f,263), FVector(88,Side * 25.0f,262), StoneShadow);
            AddTetraPoints(FVector(-91,Side * 78.0f,38), FVector(-70,Side * 60.0f,38),
                FVector(-57,Side * 61.0f,0), FVector(-105,Side * 86.0f,0), StoneShadow);
            AddTetraPoints(FVector(73,Side * 74.0f,46), FVector(91,Side * 52.0f,46),
                FVector(104,Side * 55.0f,0), FVector(59,Side * 82.0f,0), StoneShadow);
        }
        if (bSlamming)
        {
            // A bright fist at the ground makes the one-hit impact phase visible to clients.
            AddTetraPoints(FVector(166,-114,5), FVector(210,-114,5),
                FVector(167,-59,32), FVector(208,-60,31), Warning);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::ReefMaw)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ReefMawAttackCue);
        const bool bWarning = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bBiting = Cue == EKalmalaWildlifeAttackCue::Lunge;
        const FLinearColor ReefBack(0.035f, 0.19f, 0.25f);
        const FLinearColor ReefLight(0.075f, 0.34f, 0.39f);
        const FLinearColor ReefShadow(0.018f, 0.085f, 0.12f);
        const FLinearColor CoralMark(0.78f, 0.39f, 0.18f);
        const FLinearColor Eye = bWarning ? FLinearColor(1.0f, 0.59f, 0.12f)
            : FLinearColor(0.46f, 0.93f, 0.84f);
        const float JawDrop = bBiting ? -25.0f : (bWarning ? -8.0f : 0.0f);

        // Original deep-bodied reef predator with a forked tail, coral ridges,
        // angular fins, and a warning-to-bite jaw pose.
        AddTetraPoints(FVector(-162,-8,10), FVector(-162,8,10),
            FVector(-205,-52,4), FVector(-205,52,4), ReefShadow);
        AddTetraPoints(FVector(-205,-52,4), FVector(-205,52,4),
            FVector(-239,-40,14), FVector(-239,40,14), ReefLight);
        AddTetraPoints(FVector(-146,-52,14), FVector(-146,52,14),
            FVector(92,-61,25), FVector(92,61,25), ReefBack);
        AddTetraPoints(FVector(-132,-58,-15), FVector(-132,58,-15),
            FVector(104,-48,-13), FVector(104,48,-13), ReefShadow);
        AddTetraPoints(FVector(-118,-49,26), FVector(-118,49,26),
            FVector(72,-51,46), FVector(72,51,46), ReefLight);
        AddTetraPoints(FVector(-85,-48,-14), FVector(-85,48,-14),
            FVector(66,-42,3), FVector(66,42,3), FLinearColor(0.24f,0.53f,0.52f));
        AddTetraPoints(FVector(47,-43,21), FVector(47,43,21),
            FVector(137,-27,14), FVector(137,27,14), ReefLight);
        AddTetraPoints(FVector(91,-33,5), FVector(91,33,5),
            FVector(157,-22,-12 + JawDrop), FVector(157,22,-12 + JawDrop), ReefShadow);
        AddTetraPoints(FVector(97,-29,0), FVector(97,29,0),
            FVector(151,-17,-22 + JawDrop), FVector(151,17,-22 + JawDrop), CoralMark);
        AddTetraPoints(FVector(-82,-28,40), FVector(-80,28,40),
            FVector(-18,-17,104), FVector(-20,17,104), ReefBack);
        AddTetraPoints(FVector(-20,-24,42), FVector(-20,24,42),
            FVector(12,-13,83), FVector(12,13,83), ReefLight);
        AddTetraPoints(FVector(-66,-43,8), FVector(-66,43,8),
            FVector(-112,-101,-9), FVector(-112,101,-9), ReefLight);
        AddTetraPoints(FVector(-112,-101,-9), FVector(-112,101,-9),
            FVector(-35,-72,-24), FVector(-35,72,-24), ReefShadow);
        AddTetraPoints(FVector(23,-44,8), FVector(23,44,8),
            FVector(72,-92,-11), FVector(72,92,-11), ReefBack);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(78,Side * 43.0f,16), FVector(94,Side * 38.0f,17),
                FVector(71,Side * 48.0f,35), FVector(86,Side * 44.0f,38), Eye);
            AddTetraPoints(FVector(-80,Side * 50.0f,10), FVector(-73,Side * 51.0f,25),
                FVector(-45,Side * 54.0f,-1), FVector(-42,Side * 55.0f,14), CoralMark);
            AddTetraPoints(FVector(127,Side * 17.0f,-5), FVector(140,Side * 14.0f,-5),
                FVector(139,Side * 12.0f,-16 + JawDrop), FVector(128,Side * 15.0f,-17 + JawDrop),
                FLinearColor(0.91f, 0.82f, 0.60f));
            AddTetraPoints(FVector(101,Side * 25.0f,1), FVector(111,Side * 22.0f,1),
                FVector(110,Side * 20.0f,-9 + JawDrop), FVector(102,Side * 23.0f,-10 + JawDrop),
                FLinearColor(0.91f, 0.82f, 0.60f));
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Wolf)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(WolfAttackCue);
        const bool bTelegraph = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bLunging = Cue == EKalmalaWildlifeAttackCue::Lunge;
        const FLinearColor WinterCoat(0.39f, 0.43f, 0.44f);
        const FLinearColor Mantle(0.20f, 0.24f, 0.25f);
        const FLinearColor Throat(0.73f, 0.75f, 0.70f);
        const FLinearColor Muzzle(0.58f, 0.61f, 0.58f);
        const FLinearColor Paw(0.16f, 0.18f, 0.18f);
        const FLinearColor Eye(0.92f, 0.64f, 0.20f);
        const float HeadX = bLunging ? 153.0f : 132.0f;
        const float HeadZ = bTelegraph ? 83.0f : (bLunging ? 43.0f : 68.0f);

        // Long muzzle, high shoulders, winter mantle, and bushy tail give the Tundra predator
        // a distinct four-legged silhouette. Telegraph and lunge cues change its readable pose.
        AddTetraPoints(FVector(-102,-42,13), FVector(-102,42,13), FVector(45,-41,20),
            FVector(-31,0,bTelegraph ? 91.0f : 80.0f), WinterCoat);
        AddTetraPoints(FVector(-63,-35,31), FVector(-63,35,31), FVector(19,-32,41),
            FVector(-26,0,bTelegraph ? 111.0f : 96.0f), Mantle);
        AddTetraPoints(FVector(7,-30,32), FVector(7,30,32), FVector(75,-22,37),
            FVector(48,0,bTelegraph ? 88.0f : (bLunging ? 49.0f : 74.0f)), WinterCoat);
        AddTetraPoints(FVector(44,-23,48), FVector(44,23,48), FVector(HeadX - 19.0f,-15,HeadZ + 6.0f),
            FVector(HeadX - 19.0f,15,HeadZ + 6.0f), Throat);
        AddTetraPoints(FVector(HeadX - 30.0f,-17,HeadZ), FVector(HeadX - 30.0f,17,HeadZ),
            FVector(HeadX + 7.0f,-12,HeadZ - 12.0f), FVector(HeadX + 7.0f,12,HeadZ - 12.0f), Muzzle);
        AddTetraPoints(FVector(HeadX + 2.0f,-12,HeadZ - 13.0f), FVector(HeadX + 2.0f,12,HeadZ - 13.0f),
            FVector(HeadX + 28.0f,-7,HeadZ - 14.0f), FVector(HeadX + 28.0f,7,HeadZ - 14.0f), Paw);

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-60.0f, 34.0f})
            {
                const bool bFrontLeg = LegX > 0.0f;
                const bool bRaised = bTelegraph && bFrontLeg && Side < 0.0f;
                const float FootX = bLunging && bFrontLeg ? LegX + 42.0f : LegX + (bRaised ? 23.0f : 8.0f);
                const float FootZ = bRaised ? -12.0f : -58.0f;
                AddTetraPoints(FVector(LegX - 12.0f,Side * 21.0f,31), FVector(LegX + 10.0f,Side * 34.0f,29),
                    FVector(FootX - 12.0f,Side * 23.0f,-10), FVector(FootX + 8.0f,Side * 31.0f,-12),
                    bFrontLeg ? WinterCoat : Mantle);
                AddTetraPoints(FVector(FootX - 13.0f,Side * 22.0f,FootZ + 13.0f),
                    FVector(FootX - 1.0f,Side * 32.0f,FootZ + 12.0f),
                    FVector(FootX + 12.0f,Side * 24.0f,FootZ), FVector(FootX + 9.0f,Side * 34.0f,FootZ + 3.0f), Paw);
            }

            AddTetraPoints(FVector(HeadX - 27.0f,Side * 10.0f,HeadZ + 4.0f),
                FVector(HeadX - 18.0f,Side * 17.0f,HeadZ + 7.0f),
                FVector(HeadX - 47.0f,Side * 18.0f,HeadZ + 37.0f),
                FVector(HeadX - 57.0f,Side * 15.0f,HeadZ + 25.0f), Mantle);
            AddTetraPoints(FVector(HeadX - 11.0f,Side * 12.0f,HeadZ + 7.0f),
                FVector(HeadX - 4.0f,Side * 16.0f,HeadZ + 8.0f),
                FVector(HeadX + 2.0f,Side * 15.0f,HeadZ + 11.0f),
                FVector(HeadX,Side * 12.0f,HeadZ + 14.0f), Eye);
        }

        AddTetraPoints(FVector(-91,-9,24), FVector(-91,9,24), FVector(-130,-15,27),
            FVector(-135,15,26), Mantle);
        AddTetraPoints(FVector(-130,-15,27), FVector(-135,15,26), FVector(-158,-13,20),
            FVector(-165,13,18), WinterCoat);
        if (bTelegraph)
        {
            // The lifted paw and raised muzzle communicate the shared half-second warning.
            AddTetraPoints(FVector(HeadX + 7.0f,-5,HeadZ - 19.0f), FVector(HeadX + 7.0f,5,HeadZ - 19.0f),
                FVector(HeadX + 24.0f,-4,HeadZ - 28.0f), FVector(HeadX + 24.0f,4,HeadZ - 28.0f), Paw);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Lynx)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(LynxAttackCue);
        const bool bTelegraph = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bLeaping = Cue == EKalmalaWildlifeAttackCue::Lunge;
        const FLinearColor SnowCoat(0.68f, 0.66f, 0.59f);
        const FLinearColor BackCoat(0.39f, 0.36f, 0.30f);
        const FLinearColor Cream(0.84f, 0.80f, 0.69f);
        const FLinearColor Spot(0.20f, 0.17f, 0.14f);
        const FLinearColor EarTip(0.08f, 0.075f, 0.065f);
        const FLinearColor Nose(0.26f, 0.19f, 0.17f);
        const FLinearColor Eye(0.91f, 0.65f, 0.19f);
        const float HeadX = bLeaping ? 139.0f : 122.0f;
        const float HeadZ = bTelegraph ? 56.0f : (bLeaping ? 82.0f : 72.0f);

        // A compact spotted body, cheek ruffs, short tail and tufted ears distinguish
        // this Tundra pouncer from the longer, darker Wolf.
        AddTetraPoints(FVector(-91,-34,8), FVector(-91,34,8), FVector(39,-31,13),
            FVector(-25,0,bTelegraph ? 54.0f : (bLeaping ? 72.0f : 68.0f)), SnowCoat);
        AddTetraPoints(FVector(-60,-31,25), FVector(-60,31,25), FVector(20,-27,31),
            FVector(-24,0,bTelegraph ? 66.0f : 83.0f), BackCoat);
        AddTetraPoints(FVector(10,-26,22), FVector(10,26,22), FVector(71,-20,31),
            FVector(51,0,bLeaping ? 75.0f : 62.0f), SnowCoat);
        AddTetraPoints(FVector(43,-22,43), FVector(43,22,43), FVector(HeadX - 18.0f,-15,HeadZ + 3.0f),
            FVector(HeadX - 18.0f,15,HeadZ + 3.0f), Cream);
        AddTetraPoints(FVector(HeadX - 23.0f,-16,HeadZ), FVector(HeadX - 23.0f,16,HeadZ),
            FVector(HeadX + 11.0f,-11,HeadZ - 9.0f), FVector(HeadX + 11.0f,11,HeadZ - 9.0f), SnowCoat);
        AddTetraPoints(FVector(HeadX + 7.0f,-10,HeadZ - 10.0f), FVector(HeadX + 7.0f,10,HeadZ - 10.0f),
            FVector(HeadX + 27.0f,-6,HeadZ - 16.0f), FVector(HeadX + 27.0f,6,HeadZ - 16.0f), Nose);

        for (int32 SpotIndex = 0; SpotIndex < 6; ++SpotIndex)
        {
            const float X = -48.0f + static_cast<float>(SpotIndex) * 15.0f;
            const float Side = SpotIndex % 2 == 0 ? -1.0f : 1.0f;
            const float Z = 38.0f + (SpotIndex % 3) * 7.0f;
            AddTetraPoints(FVector(X - 5.0f,Side * 24.0f,Z - 4.0f),
                FVector(X - 5.0f,Side * 30.0f,Z - 4.0f), FVector(X + 5.0f,Side * 27.0f,Z - 4.0f),
                FVector(X,Side * 27.0f,Z + 3.0f), Spot);
        }

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-54.0f, 31.0f})
            {
                const bool bForeleg = LegX > 0.0f;
                const bool bCrouchedForeleg = bTelegraph && bForeleg && Side < 0.0f;
                const float FootX = bLeaping ? LegX + (bForeleg ? 45.0f : -24.0f)
                    : (LegX + (bCrouchedForeleg ? 24.0f : 4.0f));
                const float FootZ = bCrouchedForeleg ? -13.0f : (bLeaping ? -31.0f : -55.0f);
                AddTetraPoints(FVector(LegX - 10.0f,Side * 18.0f,31),
                    FVector(LegX + 10.0f,Side * 28.0f,27),
                    FVector(FootX - 8.0f,Side * 18.0f,-8), FVector(FootX + 9.0f,Side * 25.0f,-11),
                    bForeleg ? SnowCoat : BackCoat);
                AddTetraPoints(FVector(FootX - 9.0f,Side * 18.0f,FootZ + 10.0f),
                    FVector(FootX - 2.0f,Side * 27.0f,FootZ + 10.0f),
                    FVector(FootX + 11.0f,Side * 18.0f,FootZ), FVector(FootX + 9.0f,Side * 27.0f,FootZ + 2.0f), Spot);
            }

            // Wide cheek tufts and black-tipped triangular ears give the short face a lynx outline.
            AddTetraPoints(FVector(HeadX - 27.0f,Side * 11.0f,HeadZ - 1.0f),
                FVector(HeadX - 20.0f,Side * 26.0f,HeadZ + 6.0f),
                FVector(HeadX - 5.0f,Side * 18.0f,HeadZ - 18.0f),
                FVector(HeadX + 3.0f,Side * 13.0f,HeadZ - 16.0f), Cream);
            AddTetraPoints(FVector(HeadX - 15.0f,Side * 10.0f,HeadZ + 5.0f),
                FVector(HeadX - 7.0f,Side * 16.0f,HeadZ + 4.0f),
                FVector(HeadX - 12.0f,Side * 13.0f,HeadZ + 33.0f),
                FVector(HeadX - 25.0f,Side * 12.0f,HeadZ + 22.0f), BackCoat);
            AddTetraPoints(FVector(HeadX - 13.0f,Side * 11.0f,HeadZ + 24.0f),
                FVector(HeadX - 9.0f,Side * 13.0f,HeadZ + 23.0f),
                FVector(HeadX - 12.0f,Side * 12.0f,HeadZ + 39.0f),
                FVector(HeadX - 20.0f,Side * 11.0f,HeadZ + 30.0f), EarTip);
            AddTetraPoints(FVector(HeadX + 1.0f,Side * 11.0f,HeadZ + 1.0f),
                FVector(HeadX + 8.0f,Side * 14.0f,HeadZ + 2.0f),
                FVector(HeadX + 7.0f,Side * 14.0f,HeadZ + 7.0f),
                FVector(HeadX + 2.0f,Side * 12.0f,HeadZ + 9.0f), Eye);
        }
        AddTetraPoints(FVector(-84,-7,28), FVector(-84,7,28), FVector(-111,-11,23),
            FVector(-118,11,22), BackCoat);
        AddTetraPoints(FVector(-114,-11,23), FVector(-118,11,22), FVector(-130,-10,18),
            FVector(-133,10,17), EarTip);
        if (bTelegraph)
        {
            // The lowered shoulders and tucked forelegs expose the half-second pounce warning.
            AddTetraPoints(FVector(-7,-5,49), FVector(-7,5,49), FVector(22,-4,29),
                FVector(22,4,29), Spot);
        }
    }
    else if (bSnake)
    {
        const bool bGiant = Archetype == EKalmalaWildlifeArchetype::GiantSnake;
        const bool bDeathAdder = Archetype == EKalmalaWildlifeArchetype::DeathAdder;
        const bool bCoiled = static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue)
            == EKalmalaWildlifeAttackCue::Telegraph;
        const float HalfLength = bGiant ? 180.0f : (bDeathAdder ? 94.0f : 76.0f);
        const float BodyRadius = bGiant ? 36.0f : (bDeathAdder ? 21.0f : 16.0f);
        const float CoilWidth = bGiant ? 62.0f : (bDeathAdder ? 38.0f : 27.0f);
        const float HeadHeight = bGiant ? (bCoiled ? 214.0f : 146.0f)
            : (bDeathAdder ? (bCoiled ? 130.0f : 85.0f) : (bCoiled ? 104.0f : 69.0f));
        const FLinearColor ScaleBase = bGiant ? FLinearColor(0.12f, 0.20f, 0.16f)
            : (bDeathAdder ? FLinearColor(0.25f, 0.23f, 0.11f) : FLinearColor(0.27f, 0.34f, 0.18f));
        const FLinearColor ScaleDark = bGiant ? FLinearColor(0.055f, 0.095f, 0.075f)
            : (bDeathAdder ? FLinearColor(0.12f, 0.15f, 0.075f) : FLinearColor(0.13f, 0.19f, 0.11f));
        const FLinearColor ScaleMark = bGiant ? FLinearColor(0.53f, 0.47f, 0.27f)
            : (bDeathAdder ? FLinearColor(0.60f, 0.47f, 0.18f) : FLinearColor(0.70f, 0.60f, 0.31f));
        const FLinearColor Belly = bGiant ? FLinearColor(0.40f, 0.34f, 0.20f)
            : (bDeathAdder ? FLinearColor(0.47f, 0.40f, 0.22f) : FLinearColor(0.72f, 0.65f, 0.43f));
        const FLinearColor Eye = bDeathAdder ? FLinearColor(0.96f, 0.36f, 0.08f) : FLinearColor(0.95f, 0.72f, 0.12f);
        const int32 BodySegments = bGiant ? 16 : (bDeathAdder ? 11 : 9);
        for (int32 Segment = 0; Segment < BodySegments; ++Segment)
        {
            const float T = static_cast<float>(Segment) / static_cast<float>(BodySegments - 1);
            const float X = FMath::Lerp(-HalfLength, HalfLength * 0.60f, T);
            const float Curve = FMath::Sin(T * 3.0f * PI) * CoilWidth;
            const float Z = BodyRadius * 0.45f + FMath::Sin(T * 2.0f * PI) * BodyRadius * 0.20f;
            const float SegmentHalf = bGiant ? 16.0f : (bDeathAdder ? 11.0f : 9.0f);
            const float Radius = BodyRadius * (0.72f + 0.28f * FMath::Sin(T * PI));
            const FLinearColor BodyColour = Segment % 3 == 1 ? ScaleDark : ScaleBase;
            AddTetraPoints(FVector(X - SegmentHalf, Curve - Radius, Z),
                FVector(X - SegmentHalf, Curve + Radius, Z), FVector(X + SegmentHalf, Curve, Z + Radius * 0.45f),
                FVector(X, Curve, Z + Radius), BodyColour);
            if (Segment % 2 == 0)
            {
                // Repeated diamond plates give the larger shoreline ambusher its own scale pattern.
                AddTetraPoints(FVector(X - SegmentHalf * 0.35f, Curve - Radius * 0.55f, Z + Radius * 0.45f),
                    FVector(X - SegmentHalf * 0.35f, Curve + Radius * 0.55f, Z + Radius * 0.45f),
                    FVector(X + SegmentHalf * 0.35f, Curve, Z + Radius * 0.48f),
                    FVector(X, Curve, Z + Radius * 1.05f), ScaleMark);
            }
        }

        const float NeckX = HalfLength * 0.60f;
        const float HeadZ = bCoiled ? HeadHeight : HeadHeight * 0.78f;
        AddTetraPoints(FVector(NeckX - BodyRadius, -BodyRadius, BodyRadius * 0.4f),
            FVector(NeckX - BodyRadius, BodyRadius, BodyRadius * 0.4f),
            FVector(NeckX + BodyRadius * 1.4f, -BodyRadius * 0.55f, HeadZ - BodyRadius),
            FVector(NeckX + BodyRadius * 1.4f, BodyRadius * 0.55f, HeadZ - BodyRadius), Belly);
        AddTetraPoints(FVector(NeckX + BodyRadius, -BodyRadius * 0.75f, HeadZ - BodyRadius * 0.85f),
            FVector(NeckX + BodyRadius, BodyRadius * 0.75f, HeadZ - BodyRadius * 0.85f),
            FVector(NeckX + BodyRadius * 3.2f, -BodyRadius * 0.45f, HeadZ),
            FVector(NeckX + BodyRadius * 3.2f, BodyRadius * 0.45f, HeadZ + BodyRadius * 0.25f), ScaleBase);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(NeckX + BodyRadius * 2.0f, Side * BodyRadius * 0.48f, HeadZ + BodyRadius * 0.12f),
                FVector(NeckX + BodyRadius * 2.0f, Side * BodyRadius * 0.72f, HeadZ + BodyRadius * 0.08f),
                FVector(NeckX + BodyRadius * 2.45f, Side * BodyRadius * 0.53f, HeadZ + BodyRadius * 0.34f),
                FVector(NeckX + BodyRadius * 2.55f, Side * BodyRadius * 0.65f, HeadZ + BodyRadius * 0.13f), Eye);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm)
    {
        const FLinearColor Body(0.16f, 0.12f, 0.075f);
        const FLinearColor Thorax(0.31f, 0.23f, 0.12f);
        const FLinearColor Wing(0.66f, 0.71f, 0.59f);
        const FLinearColor Eye(0.78f, 0.20f, 0.08f);
        for (int32 Insect = 0; Insect < 6; ++Insect)
        {
            const float Angle = (2.0f * PI * static_cast<float>(Insect)) / 6.0f;
            const FVector Centre(FMath::Cos(Angle) * 34.0f, FMath::Sin(Angle) * 34.0f,
                FMath::Sin(Angle * 2.0f) * 15.0f);
            AddTetraPoints(Centre + FVector(-7.0f, 0.0f, 0.0f), Centre + FVector(5.0f, 0.0f, 0.0f),
                Centre + FVector(-1.0f, -2.5f, 0.0f), Centre + FVector(-1.0f, 2.5f, 0.0f), Body);
            AddTetraPoints(Centre + FVector(-3.0f, -2.0f, 1.0f), Centre + FVector(3.0f, -3.0f, 1.0f),
                Centre + FVector(-1.0f, -15.0f, 3.0f), Centre + FVector(2.0f, -13.0f, 0.0f), Wing);
            AddTetraPoints(Centre + FVector(-3.0f, 2.0f, 1.0f), Centre + FVector(3.0f, 3.0f, 1.0f),
                Centre + FVector(-1.0f, 15.0f, 3.0f), Centre + FVector(2.0f, 13.0f, 0.0f), Wing);
            AddTetraPoints(Centre + FVector(4.0f, -1.0f, 1.0f), Centre + FVector(11.0f, -1.0f, 1.0f),
                Centre + FVector(9.0f, -2.5f, 3.0f), Centre + FVector(9.0f, 2.5f, 3.0f), Thorax);
            AddTetraPoints(Centre + FVector(10.0f, -1.0f, 1.0f), Centre + FVector(13.0f, -1.0f, 1.0f),
                Centre + FVector(12.0f, 0.0f, 2.5f), Centre + FVector(14.0f, 0.0f, 1.0f), Eye);
        }
    }
    else if (Archetype == EKalmalaWildlifeArchetype::PeatHusk)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(PeatHuskAttackCue);
        const bool bWarning = Cue == EKalmalaWildlifeAttackCue::Telegraph;
        const bool bRecovering = Cue == EKalmalaWildlifeAttackCue::Recovery;
        const FLinearColor RootShadow(0.10f, 0.14f, 0.11f);
        const FLinearColor PeatBark(0.20f, 0.17f, 0.12f);
        const FLinearColor Lichen(0.34f, 0.49f, 0.25f);
        const FLinearColor WarmEye(0.72f, 0.42f, 0.17f);
        const FLinearColor SweepWarning(0.94f, 0.57f, 0.16f);

        AddTetra(FVector(-7,0,0), FVector(54,44,104), RootShadow);
        AddTetra(FVector(27,0,5), FVector(42,37,78), PeatBark);
        AddTetra(FVector(48,0,70), FVector(28,28,46), RootShadow);
        AddTetra(FVector(61,0,bWarning ? 104.0f : 98.0f), FVector(43,35,52), Lichen);

        AddTetra(FVector(36,-21,132), FVector(16,12,48), PeatBark);
        AddTetra(FVector(36,21,132), FVector(16,12,48), PeatBark);
        AddTetra(FVector(35,0,133), FVector(15,14,42), Lichen);
        AddTetra(FVector(86,-13,122), FVector(6,5,7), bWarning ? SweepWarning : WarmEye);
        AddTetra(FVector(86,13,122), FVector(6,5,7), bWarning ? SweepWarning : WarmEye);

        if (bWarning)
        {
            // Lift one root-like arm into a clear one-second warning silhouette.
            AddTetraPoints(FVector(16,-33,62), FVector(43,-51,73),
                FVector(66,-38,105), FVector(112,-43,119), SweepWarning);
            AddTetra(FVector(118,-43,116), FVector(22,17,18), SweepWarning);
        }
        else if (bRecovering)
        {
            // The arm follows through low and forward while the server recovery timer runs.
            AddTetraPoints(FVector(16,-33,62), FVector(46,-51,49),
                FVector(84,-39,34), FVector(133,-41,25), PeatBark);
            AddTetra(FVector(140,-42,22), FVector(20,15,16), SweepWarning);
        }
        else
        {
            AddTetraPoints(FVector(16,-33,62), FVector(43,-51,54), FVector(54,-35,43), FVector(88,-69,5), PeatBark);
            AddTetra(FVector(98,-68,0), FVector(17,13,20), RootShadow);
        }
        AddTetraPoints(FVector(16,33,62), FVector(54,35,43), FVector(43,51,54), FVector(88,69,5), PeatBark);
        AddTetra(FVector(98,68,0), FVector(17,13,20), RootShadow);

        AddTetra(FVector(-39,-22,0), FVector(21,17,72), RootShadow);
        AddTetra(FVector(-39,22,0), FVector(21,17,72), RootShadow);
        AddTetra(FVector(-4,-22,0), FVector(20,14,17), PeatBark);
        AddTetra(FVector(-4,22,0), FVector(20,14,17), PeatBark);

        AddTetra(FVector(15,-39,30), FVector(13,9,30), Lichen);
        AddTetra(FVector(15,39,30), FVector(13,9,30), Lichen);
    }

    // Preserve each original silhouette while tinting a sparse set of faces with
    // the server-selected niche, so nearby peers can read habitat identity without
    // receiving a second client-authored gameplay state.
    const FLinearColor EcologyAccent = GetEcologyAccent(GetEcologyForNiche(CreatureNicheId));
    for (int32 VertexIndex = 0; VertexIndex + 2 < Colors.Num(); VertexIndex += 3 * 5)
    {
        Colors[VertexIndex] = BlendEcologyAccent(Colors[VertexIndex], EcologyAccent);
        Colors[VertexIndex + 1] = BlendEcologyAccent(Colors[VertexIndex + 1], EcologyAccent);
        Colors[VertexIndex + 2] = BlendEcologyAccent(Colors[VertexIndex + 2], EcologyAccent);
    }
    MirelingMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, {}, false);
}

void AKalmalaWildlifeSpawn::AlertNearbyDeerFromServer()
{
    if (!HasAuthority() || bDefeated) return;
    int32 AlertedCount = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
    {
        AKalmalaWildlifeSpawn* Deer = *It;
        if (!IsValid(Deer) || Deer->bDefeated || Deer->Archetype != EKalmalaWildlifeArchetype::Deer
            || Deer->Behaviour != EKalmalaWildlifeBehaviour::Idle
            || FVector::DistSquared2D(Deer->GetActorLocation(), GetActorLocation()) > FMath::Square(800.0f)) continue;
        Deer->BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f,
            Deer->SpawnOrigin + Deer->GetDeterministicOffset(GetEcologicalFleeDistance(Deer->CreatureNicheId)));
        ++AlertedCount;
    }
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Deer peer herd alert: Source=%s Alerted=%d"), *GetName(), AlertedCount);
    }
#endif
}

void AKalmalaWildlifeSpawn::BeginServerBehaviour(const EKalmalaWildlifeBehaviour NextBehaviour, const float DurationSeconds,
    const FVector& Destination, const bool bHoldDurationAtDestination)
{
    if (!IsBehaviourTransitionAllowed(HasAuthority(), bDefeated, Behaviour, NextBehaviour) || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f || !FMath::IsFinite(Destination.X) || !FMath::IsFinite(Destination.Y) || !FMath::IsFinite(Destination.Z))
    {
        return;
    }

    Behaviour = NextBehaviour;
    bHoldBehaviourDurationAtDestination = bHoldDurationAtDestination && NextBehaviour == EKalmalaWildlifeBehaviour::Flee;
    const float DurationLimit = bHoldBehaviourDurationAtDestination ? DeerCallInfluenceDurationSeconds : 2.0f;
    BehaviourSecondsRemaining = FMath::Min(DurationSeconds, DurationLimit);
    BehaviourDestination = Destination;
}

void AKalmalaWildlifeSpawn::AdvanceServerBehaviour(float DeltaSeconds)
{
    DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0f, 0.10f);
    if (Behaviour == EKalmalaWildlifeBehaviour::Idle || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const FVector ToDestination = BehaviourDestination - GetActorLocation();
    const float Step = FMath::Min(220.0f * DeltaSeconds, ToDestination.Size());
    if (Step > 0.0f)
    {
        SetActorLocation(GetActorLocation() + ToDestination.GetSafeNormal() * Step, true);
    }

    BehaviourSecondsRemaining -= DeltaSeconds;
    if (BehaviourSecondsRemaining > 0.0f
        && (bHoldBehaviourDurationAtDestination || !ToDestination.IsNearlyZero(5.0f)))
    {
        return;
    }

    if (Behaviour == EKalmalaWildlifeBehaviour::Flee)
    {
        BeginServerBehaviour(EKalmalaWildlifeBehaviour::Investigate, 1.00f, SpawnOrigin + GetDeterministicOffset(120.0f));
    }
    else if (Behaviour == EKalmalaWildlifeBehaviour::Investigate)
    {
        BeginServerBehaviour(EKalmalaWildlifeBehaviour::Return, 2.00f, SpawnOrigin);
    }
    else if (Behaviour == EKalmalaWildlifeBehaviour::Return)
    {
        Behaviour = EKalmalaWildlifeBehaviour::Idle;
        BehaviourSecondsRemaining = 0.0f;
        bHoldBehaviourDurationAtDestination = false;
        BehaviourDestination = SpawnOrigin;
    }
}

FVector AKalmalaWildlifeSpawn::GetDeterministicOffset(const float Distance) const
{
    const uint32 Hash = FCrc::StrCrc32(*PersistentSpawnId);
    const float Angle = (float(Hash % 360u) / 360.0f) * 2.0f * PI;
    return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FMath::Clamp(Distance, 0.0f, 300.0f);
}

void AKalmalaWildlifeSpawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, bDefeated);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, Health);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, PersistentSpawnId);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, Archetype);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, CreatureNicheId);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, SnakeAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, RatAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, BadgerAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, WolfAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, LynxAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, PolarBearAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, HornedGoatAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, ThunderSentinelAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, ReefMawAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, MoonBearAttackCue);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, PeatHuskAttackCue);
}

void AKalmalaWildlifeSpawn::OnRep_Archetype()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaPeatHuskPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::PeatHusk
        && static_cast<EKalmalaWildlifeAttackCue>(PeatHuskAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Peat Husk verification client observed replicated telegraph."));
    }
#endif
    BuildArchetypePresentation();
    if (Archetype == EKalmalaWildlifeArchetype::MosquitoSwarm && !bDefeated) PlayMosquitoBuzz();
    else StopMosquitoBuzz();
}

void AKalmalaWildlifeSpawn::OnRep_CreatureNicheId()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_SnakeAttackCue()
{
    BuildArchetypePresentation();
    if (static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
        PlaySnakeHiss();
}

void AKalmalaWildlifeSpawn::OnRep_RatAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_BadgerAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_WolfAttackCue()
{
    BuildArchetypePresentation();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWolfPeerTest"))
        && static_cast<EKalmalaWildlifeAttackCue>(WolfAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Wolf verification client observed replicated telegraph: SpawnId=%s."),
            *PersistentSpawnId);
    }
#endif
}

void AKalmalaWildlifeSpawn::OnRep_LynxAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_PolarBearAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_HornedGoatAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_ThunderSentinelAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_ReefMawAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_MoonBearAttackCue()
{
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_PeatHuskAttackCue()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaPeatHuskPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::PeatHusk
        && static_cast<EKalmalaWildlifeAttackCue>(PeatHuskAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Peat Husk verification client observed replicated telegraph."));
    }
#endif
    BuildArchetypePresentation();
}

void AKalmalaWildlifeSpawn::OnRep_Defeated()
{
    ApplyDefeatedState();
    const bool bBadgerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBadgerPeerTest"));
    const bool bMoonBearPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMoonBearPeerTest"));
    const bool bRatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest"));
    const bool bViperPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaViperPeerTest"));
    const bool bGiantSnakePeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaGiantSnakePeerTest"));
    if (bDefeated && !bClientCombatVerificationDefeatLogged
        && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest"))
            || FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"))
            || bBadgerPeerTest || bMoonBearPeerTest || bRatPeerTest || bViperPeerTest || bGiantSnakePeerTest))
    {
        bClientCombatVerificationDefeatLogged = true;
        const TCHAR* VerificationName = bRatPeerTest ? TEXT("Rat")
            : bViperPeerTest ? TEXT("Viper")
            : bGiantSnakePeerTest ? TEXT("Giant Snake")
            : bBadgerPeerTest ? TEXT("Badger")
            : bMoonBearPeerTest ? TEXT("Moon Bear")
            : FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) ? TEXT("Boar")
            : FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest")) ? TEXT("Deer")
            : FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) ? TEXT("Mireling") : TEXT("Combat");
        UE_LOG(LogTemp, Display, TEXT("%s verification client observed relevant wildlife defeat."), VerificationName);
    }
}

void AKalmalaWildlifeSpawn::ApplyDefeatedState()
{
    SetActorHiddenInGame(bDefeated);
    if (bDefeated)
    {
        StopMosquitoBuzz();
        MosquitoTarget.Reset();
        MosquitoApproachSecondsRemaining = 0.0f;
        MosquitoBiteSecondsRemaining = 0.0f;
        PeatHuskAttackTarget.Reset();
        PeatHuskAttackSecondsRemaining = 0.0f;
        SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue::None);
        WolfAttackTarget.Reset();
        WolfAttackSecondsRemaining = 0.0f;
        WolfLungeDistanceRemaining = 0.0f;
        SetWolfAttackCue(EKalmalaWildlifeAttackCue::None);
        LynxAttackTarget.Reset();
        LynxAttackSecondsRemaining = 0.0f;
        LynxLeapDistanceRemaining = 0.0f;
        SetLynxAttackCue(EKalmalaWildlifeAttackCue::None);
        PolarBearAttackTarget.Reset();
        PolarBearAttackSecondsRemaining = 0.0f;
        PolarBearChargeDistanceRemaining = 0.0f;
        PolarBearRetreatDistanceRemaining = 0.0f;
        bPolarBearChargeHitApplied = false;
        SetPolarBearAttackCue(EKalmalaWildlifeAttackCue::None);
        HornedGoatAttackTarget.Reset();
        HornedGoatAttackSecondsRemaining = 0.0f;
        SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue::None);
        ThunderSentinelAttackTarget.Reset();
        ThunderSentinelAttackSecondsRemaining = 0.0f;
        ThunderSentinelStepDistanceRemaining = 0.0f;
        bThunderSentinelSlamHitApplied = false;
        SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue::None);
        ReefMawAttackTarget.Reset();
        ReefMawAttackSecondsRemaining = 0.0f;
        bReefMawBiteHitApplied = false;
        SetReefMawAttackCue(EKalmalaWildlifeAttackCue::None);
        GrantDefeatReward();
    }
}

void AKalmalaWildlifeSpawn::GrantDefeatReward()
{
    if (!HasAuthority() || bDefeatRewardProcessed) return;
    bDefeatRewardProcessed = true;
    AKalmalaCharacter* Attacker = LastValidatedAttacker.Get();
    UKalmalaInventoryComponent* Inventory = Attacker ? Attacker->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Inventory == nullptr || !Inventory->GetOwner() || !Inventory->GetOwner()->HasAuthority()) return;
    if (Archetype == EKalmalaWildlifeArchetype::Boar)
    {
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1);
        Inventory->TryGrantFromServer(TEXT("BoarHide"), 1);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Deer)
    {
        Inventory->TryGrantFromServer(TEXT("DeerMeat"), 1);
        Inventory->TryGrantFromServer(TEXT("DeerHide"), 1);
        const float SinewRoll = FMath::FRand();
        if (IsDeerSinewDropSuccessful(SinewRoll))
        {
            Inventory->TryGrantFromServer(TEXT("Sinew"), 1);
        }
    }

    const int32 BoneFragmentQuantity = GetBoneFragmentRewardQuantity(Archetype);
    if (BoneFragmentQuantity > 0 && Archetype != EKalmalaWildlifeArchetype::Rat
        && Archetype != EKalmalaWildlifeArchetype::Badger && Archetype != EKalmalaWildlifeArchetype::MoonBear
        && Archetype != EKalmalaWildlifeArchetype::Viper && Archetype != EKalmalaWildlifeArchetype::GiantSnake
        && Archetype != EKalmalaWildlifeArchetype::DeathAdder
        && Archetype != EKalmalaWildlifeArchetype::Wolf
        && Archetype != EKalmalaWildlifeArchetype::Lynx
        && Archetype != EKalmalaWildlifeArchetype::PolarBear
        && Archetype != EKalmalaWildlifeArchetype::HornedGoat
        && Archetype != EKalmalaWildlifeArchetype::ReefMaw
        && Archetype != EKalmalaWildlifeArchetype::PeatHusk)
    {
        Inventory->TryGrantFromServer(TEXT("BoneFragment"), BoneFragmentQuantity);
    }
    // Credited Rat, Badger, bear, snake, Wolf, Lynx, Goat, Sentinel, Reef Maw and Peat Husk
    // rewards use the persistent queue recorded before defeated state is replicated.

    if (Archetype == EKalmalaWildlifeArchetype::Mireling && IsMirelingBossRewardCandidate())
    {
        if (AKalmalaGameMode* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>())
        {
            Mode->ClaimMirelingBossScroll(Attacker, PersistentSpawnId);
        }
    }
}
