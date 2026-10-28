#include "KalmalaWildlifeSpawn.h"
#include "KalmalaResistanceComponent.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaGemstoneContract.h"
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

void AKalmalaWildlifeSpawn::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority()) return;

    if (UWorld* World = GetWorld())
    {
        if (AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>())
        {
            Mode->RegisterLivePopulationCreature(this);
        }
    }
}

void AKalmalaWildlifeSpawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        if (UWorld* World = GetWorld())
        {
            if (AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>())
            {
                Mode->UnregisterLivePopulationCreature(this);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
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
        HomePopulationSpatialKey = Spawn.SpatialKey;
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

    if (Archetype == EKalmalaWildlifeArchetype::Mireling && FKalmalaGemstoneContract::DuskDiamondSourceAccepted)
    {
        auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (!Mode || !Mode->RecordMirelingDefeatReward(PersistentSpawnId, LastValidatedAttacker.Get())) return false;
    }

    FangArrowPoisonRemainingSeconds = 0.0f;
    FangArrowPoisonSecondsUntilTick = UKalmalaPlayerStatusComponent::PoisonTickIntervalSeconds;
    bDefeated = true;
    if (UWorld* World = GetWorld())
    {
        if (AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>())
        {
            Mode->UnregisterLivePopulationCreature(this);
        }
    }
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
{ return ApplyValidatedDamageFromServer(Damage, Attacker, true, nullptr); }

bool AKalmalaWildlifeSpawn::ApplyStormcutCombatDamageFromServer(const FKalmalaDamageChannels& Damage,
    AKalmalaCharacter* Attacker, const FGuid& ActiveSwordGuid)
{
    if (!ActiveSwordGuid.IsValid()) return false;
    return ApplyValidatedDamageFromServer(Damage, Attacker, true, &ActiveSwordGuid);
}

bool AKalmalaWildlifeSpawn::ApplyArrowDamageFromServer(const FKalmalaDamageChannels& Damage,
    AKalmalaCharacter* Attacker, const FName ArrowItemId)
{
    if (!Attacker || ArrowItemId.IsNone()) return false;
    const bool bDamageAccepted = ApplyValidatedDamageFromServer(Damage, Attacker, false, nullptr);
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
            ApplyValidatedDamageFromServer(PoisonDamage, nullptr, false, nullptr);
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

bool AKalmalaWildlifeSpawn::ApplyValidatedDamageFromServer(const FKalmalaDamageChannels& Damage,
    AKalmalaCharacter* Attacker, bool bMelee, const FGuid* StormcutSwordGuid)
{
    if (!HasAuthority() || bDefeated || !Damage.IsValid() || Damage.Total() <= 0.0f) return false;
    if (Attacker != nullptr && (!Attacker->HasAuthority() || Attacker->GetWorld() != GetWorld()
        || (bMelee && FVector::DistSquared(Attacker->GetActorLocation(), GetActorLocation()) > FMath::Square(220.0f)))) return false;
    const UKalmalaResistanceComponent* Defenses = FindComponentByClass<UKalmalaResistanceComponent>();
    float ResolvedDamage = 0.0f;
    if (!Defenses || !Defenses->ResolveFromServer(Damage, ResolvedDamage)) return false;
    if (StormcutSwordGuid != nullptr
        && (Attacker == nullptr || !Attacker->SpendStormcutConditionForAcceptedHitFromServer(*StormcutSwordGuid))) return false;
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

bool AKalmalaWildlifeSpawn::IsPopulationSpawnPositionSupported(
    const FKalmalaWorldGenerationConfig& Config, const EKalmalaWildlifeArchetype Archetype,
    const FVector Position, const FVector SpawnOrigin)
{
    if (!Config.IsValid() || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)
        || !FMath::IsFinite(Position.Z))
    {
        return false;
    }

    const FVector2D XY(Position);
    const auto GetBiome = [&Config, &XY]()
    {
        return FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY));
    };
    const auto IsWater = [&Config, &XY]()
    {
        return FKalmalaOceanSampler::Sample(Config, XY).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, XY);
    };
    switch (Archetype)
    {
    case EKalmalaWildlifeArchetype::Rat:
        return !IsWater();
    case EKalmalaWildlifeArchetype::Badger:
    case EKalmalaWildlifeArchetype::MoonBear:
        return GetBiome() == EKalmalaBiome::Elderwood && !IsWater();
    case EKalmalaWildlifeArchetype::Wolf:
    case EKalmalaWildlifeArchetype::Lynx:
    case EKalmalaWildlifeArchetype::PolarBear:
        return FKalmalaWorldBounds::Contains(Config, XY)
            && GetBiome() == EKalmalaBiome::FreezingTundra && !IsWater()
            && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
    case EKalmalaWildlifeArchetype::HornedGoat:
    case EKalmalaWildlifeArchetype::ThunderSentinel:
    {
        const bool bTraversable = FKalmalaWorldBounds::Contains(Config, XY)
            && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, XY).Z >= 0.82f;
        return IsThunderMountainsSpawnHabitatAllowed(GetBiome(), IsWater(), bTraversable);
    }
    case EKalmalaWildlifeArchetype::ReefMaw:
        return IsReefMawPositionNavigable(Config, XY);
    case EKalmalaWildlifeArchetype::PeatHusk:
        return IsMossyMireSpawnHabitatAllowed(GetBiome(), IsWater());
    case EKalmalaWildlifeArchetype::DeathAdder:
    case EKalmalaWildlifeArchetype::Viper:
    case EKalmalaWildlifeArchetype::GiantSnake:
    {
        if (!FMath::IsFinite(SpawnOrigin.X) || !FMath::IsFinite(SpawnOrigin.Y)
            || !FMath::IsFinite(SpawnOrigin.Z)
            || FVector::DistSquared2D(Position, SpawnOrigin) > FMath::Square(SnakeSpawnLeashCentimetres))
        {
            return false;
        }
        const EKalmalaBiome ExpectedBiome = Archetype == EKalmalaWildlifeArchetype::DeathAdder
            ? EKalmalaBiome::MossyMire : EKalmalaBiome::ShimmeringLakes;
        return GetBiome() == ExpectedBiome && !IsWater();
    }
    case EKalmalaWildlifeArchetype::Mireling:
    case EKalmalaWildlifeArchetype::Boar:
    case EKalmalaWildlifeArchetype::Deer:
    case EKalmalaWildlifeArchetype::MosquitoSwarm:
        // These species use the S20 generated-ground and dry-surface checks,
        // with no separate NavMesh or species-specific traversal restriction.
        return true;
    default:
        return false;
    }
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::Rat, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::Badger, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::Wolf, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::Lynx, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::PolarBear, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::HornedGoat, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::ThunderSentinel, Position, SpawnOrigin);
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
    return IsPopulationSpawnPositionSupported(
        State->GetWorldGenerationConfig(), EKalmalaWildlifeArchetype::ReefMaw, Position, SpawnOrigin);
}

bool AKalmalaWildlifeSpawn::IsReefMawOceanPosition(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D Position, FKalmalaOceanSample* OutOceanSample)
{
    if (OutOceanSample != nullptr) *OutOceanSample = FKalmalaOceanSample{};
    if (!Config.IsValid() || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y)
        || !FKalmalaWorldBounds::Contains(Config, Position))
    {
        return false;
    }

    const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Position);
    if (OutOceanSample != nullptr) *OutOceanSample = Ocean;
    if (!Ocean.IsWater()) return false;

    const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
        FKalmalaWorldFieldSampler::Sample(Config, Position));
    return Biome == EKalmalaBiome::Ocean
        && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
}

bool AKalmalaWildlifeSpawn::IsReefMawPositionNavigable(
    const FKalmalaWorldGenerationConfig& Config, const FVector2D Position,
    FKalmalaOceanSample* OutOceanSample)
{
    FKalmalaOceanSample Ocean;
    const bool bOceanPosition = IsReefMawOceanPosition(Config, Position, &Ocean);
    if (OutOceanSample != nullptr) *OutOceanSample = Ocean;
    return bOceanPosition && IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean,
        false, Ocean.WaterDepth);
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
    return IsPopulationSpawnPositionSupported(
        Config, EKalmalaWildlifeArchetype::PeatHusk, Position, SpawnOrigin);
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
    if (Position.ContainsNaN()) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld() != nullptr
        ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (State == nullptr) return false;
    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    return IsPopulationSpawnPositionSupported(Config, Archetype, Position, SpawnOrigin);
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

bool AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
    const EKalmalaWildlifeArchetype InArchetype,
    TArray<FKalmalaWildlifeBodyCollisionShape>& OutShapes)
{
    OutShapes.Reset();
    float RootRadius = 70.0f;
    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::MosquitoSwarm: RootRadius = 28.0f; break;
    case EKalmalaWildlifeArchetype::Rat: RootRadius = 42.0f; break;
    case EKalmalaWildlifeArchetype::ThunderSentinel: RootRadius = 180.0f; break;
    case EKalmalaWildlifeArchetype::ReefMaw: RootRadius = ReefMawBodyCollisionRadiusCentimetres; break;
    case EKalmalaWildlifeArchetype::Mireling:
    case EKalmalaWildlifeArchetype::Boar:
    case EKalmalaWildlifeArchetype::Deer:
    case EKalmalaWildlifeArchetype::Badger:
    case EKalmalaWildlifeArchetype::MoonBear:
    case EKalmalaWildlifeArchetype::DeathAdder:
    case EKalmalaWildlifeArchetype::Viper:
    case EKalmalaWildlifeArchetype::GiantSnake:
    case EKalmalaWildlifeArchetype::PeatHusk:
    case EKalmalaWildlifeArchetype::Wolf:
    case EKalmalaWildlifeArchetype::Lynx:
    case EKalmalaWildlifeArchetype::PolarBear:
    case EKalmalaWildlifeArchetype::HornedGoat:
        break;
    default:
        return false;
    }

    FKalmalaWildlifeBodyCollisionShape& RootShape = OutShapes.AddDefaulted_GetRef();
    RootShape.Type = EKalmalaWildlifeBodyShapeType::Sphere;
    RootShape.RadiusCentimetres = RootRadius;
    RootShape.HalfHeightCentimetres = RootRadius;

    float CapsuleRadius = 0.0f;
    float CapsuleHalfHeight = 0.0f;
    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Viper:
        CapsuleRadius = 16.0f;
        CapsuleHalfHeight = 92.0f;
        break;
    case EKalmalaWildlifeArchetype::GiantSnake:
        CapsuleRadius = 38.0f;
        CapsuleHalfHeight = 210.0f;
        break;
    case EKalmalaWildlifeArchetype::DeathAdder:
        CapsuleRadius = 21.0f;
        CapsuleHalfHeight = 112.0f;
        break;
    default:
        break;
    }
    if (CapsuleRadius > 0.0f)
    {
        FKalmalaWildlifeBodyCollisionShape& CapsuleShape = OutShapes.AddDefaulted_GetRef();
        CapsuleShape.Type = EKalmalaWildlifeBodyShapeType::Capsule;
        CapsuleShape.RelativeLocation = FVector(-22.0f, 0.0f, 0.0f);
        CapsuleShape.RelativeRotation = FRotator(0.0f, 90.0f, 0.0f);
        CapsuleShape.RadiusCentimetres = CapsuleRadius;
        CapsuleShape.HalfHeightCentimetres = CapsuleHalfHeight;
    }
    return true;
}

void AKalmalaWildlifeSpawn::BuildArchetypePresentation()
{
    MirelingMesh->ClearAllMeshSections();
    TArray<FKalmalaWildlifeBodyCollisionShape> BodyShapes;
    if (GetBodyCollisionShapesForArchetype(Archetype, BodyShapes))
    {
        if (!BodyShapes.IsEmpty())
        {
            if (USphereComponent* RootSphere = Cast<USphereComponent>(GetRootComponent()))
            {
                RootSphere->SetSphereRadius(BodyShapes[0].RadiusCentimetres, true);
            }
        }
        if (SnakeBodyCollision != nullptr)
        {
            const FKalmalaWildlifeBodyCollisionShape* CapsuleShape = BodyShapes.FindByPredicate(
                [](const FKalmalaWildlifeBodyCollisionShape& Shape)
                { return Shape.Type == EKalmalaWildlifeBodyShapeType::Capsule; });
            if (CapsuleShape != nullptr)
            {
                SnakeBodyCollision->SetCapsuleSize(CapsuleShape->RadiusCentimetres,
                    CapsuleShape->HalfHeightCentimetres, true);
                SnakeBodyCollision->SetRelativeLocation(CapsuleShape->RelativeLocation);
                SnakeBodyCollision->SetRelativeRotation(CapsuleShape->RelativeRotation);
                SnakeBodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            }
            else
            {
                SnakeBodyCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
    }
    const bool bSnake = Archetype == EKalmalaWildlifeArchetype::Viper
        || Archetype == EKalmalaWildlifeArchetype::GiantSnake
        || Archetype == EKalmalaWildlifeArchetype::DeathAdder;
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
            for (const int32 Vertex : OutwardFace)
            {
                Vertices.Add(Points[Vertex]);
                Normals.Add((Points[Vertex] - Centroid).GetSafeNormal());
                UV.Add(FVector2D::ZeroVector);
                Colors.Add(Colour);
            }
        }
    };
    const auto AddTetra = [&AddTetraPoints](const FVector Centre, const FVector Extent, const FLinearColor Colour)
    {
        AddTetraPoints(Centre + FVector(-Extent.X,-Extent.Y,0), Centre + FVector(Extent.X,-Extent.Y,0),
            Centre + FVector(0,Extent.Y,0), Centre + FVector(0,0,Extent.Z), Colour);
    };
    // Rounded masses establish recognizable animal anatomy; angular solids below remain for
    // small features such as antlers, claws, fins, facial markings, and pose cues.
    const auto AddEllipsoid = [&Vertices, &Triangles, &Normals, &UV, &Colors](const FVector Centre,
        const FVector Radii, const FLinearColor Colour)
    {
        constexpr int32 LongitudeSegments = 8;
        constexpr int32 LatitudeSegments = 5;
        if (Radii.X <= KINDA_SMALL_NUMBER || Radii.Y <= KINDA_SMALL_NUMBER
            || Radii.Z <= KINDA_SMALL_NUMBER) return;

        TArray<TArray<FVector>> Rings;
        Rings.SetNum(LatitudeSegments - 1);
        for (int32 LatitudeIndex = 1; LatitudeIndex < LatitudeSegments; ++LatitudeIndex)
        {
            const float Latitude = -0.5f * PI + PI * static_cast<float>(LatitudeIndex)
                / static_cast<float>(LatitudeSegments);
            const float RingRadius = FMath::Cos(Latitude);
            TArray<FVector>& Ring = Rings[LatitudeIndex - 1];
            Ring.Reserve(LongitudeSegments);
            for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments; ++LongitudeIndex)
            {
                const float Longitude = 2.0f * PI * static_cast<float>(LongitudeIndex)
                    / static_cast<float>(LongitudeSegments);
                Ring.Add(Centre + FVector(Radii.X * RingRadius * FMath::Cos(Longitude),
                    Radii.Y * RingRadius * FMath::Sin(Longitude), Radii.Z * FMath::Sin(Latitude)));
            }
        }

        const auto AddSurfaceTriangle = [&Vertices, &Triangles, &Normals, &UV, &Colors, Centre, Radii, Colour]
            (FVector A, FVector B, FVector C)
        {
            const auto EllipsoidNormal = [Centre, Radii](const FVector& Position)
            {
                return FVector((Position.X - Centre.X) / FMath::Square(Radii.X),
                    (Position.Y - Centre.Y) / FMath::Square(Radii.Y),
                    (Position.Z - Centre.Z) / FMath::Square(Radii.Z)).GetSafeNormal();
            };
            const FVector FaceNormal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
            if (FVector::DotProduct(FaceNormal, EllipsoidNormal((A + B + C) / 3.0f)) < 0.0f)
                Swap(B, C);

            const int32 FirstVertex = Vertices.Num();
            Vertices.Append({A, B, C});
            Normals.Append({EllipsoidNormal(A), EllipsoidNormal(B), EllipsoidNormal(C)});
            UV.Append({FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector});
            Colors.Append({Colour, Colour, Colour});
            Triangles.Append({FirstVertex, FirstVertex + 2, FirstVertex + 1});
        };

        const FVector SouthPole = Centre + FVector(0.0f, 0.0f, -Radii.Z);
        const FVector NorthPole = Centre + FVector(0.0f, 0.0f, Radii.Z);
        for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments; ++LongitudeIndex)
        {
            const int32 NextLongitude = (LongitudeIndex + 1) % LongitudeSegments;
            AddSurfaceTriangle(SouthPole, Rings[0][LongitudeIndex], Rings[0][NextLongitude]);
            AddSurfaceTriangle(Rings.Last()[LongitudeIndex], NorthPole, Rings.Last()[NextLongitude]);
        }
        for (int32 RingIndex = 0; RingIndex < Rings.Num() - 1; ++RingIndex)
        {
            for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments; ++LongitudeIndex)
            {
                const int32 NextLongitude = (LongitudeIndex + 1) % LongitudeSegments;
                const FVector& A = Rings[RingIndex][LongitudeIndex];
                const FVector& B = Rings[RingIndex][NextLongitude];
                const FVector& C = Rings[RingIndex + 1][LongitudeIndex];
                const FVector& D = Rings[RingIndex + 1][NextLongitude];
                AddSurfaceTriangle(A, B, C);
                AddSurfaceTriangle(B, D, C);
            }
        }
    };
    if (Archetype == EKalmalaWildlifeArchetype::Mireling)
    {
        const FLinearColor PeatSkin(0.16f, 0.25f, 0.16f);
        const FLinearColor Moss(0.31f, 0.39f, 0.18f);
        const FLinearColor Bark(0.10f, 0.16f, 0.11f);
        const FLinearColor AmberEye(0.76f, 0.48f, 0.12f);
        const FLinearColor Pupil(0.035f, 0.045f, 0.025f);

        // A hunched bog brute has a moss-backed torso, heavy head, long arms,
        // and short root-like legs rather than the fallback's empty mesh.
        AddEllipsoid(FVector(-18, 0, 64), FVector(67, 53, 76), PeatSkin);
        AddEllipsoid(FVector(-44, 0, 112), FVector(48, 51, 39), Moss);
        AddEllipsoid(FVector(40, 0, 125), FVector(47, 43, 47), PeatSkin);
        AddEllipsoid(FVector(79, 0, 99), FVector(24, 34, 22), Bark);

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(59, Side * 35.0f, 137), FVector(7, 6, 8), AmberEye);
            AddEllipsoid(FVector(63, Side * 39.0f, 137), FVector(3, 2, 4), Pupil);
            AddTetraPoints(FVector(3, Side * 40.0f, 93), FVector(21, Side * 55.0f, 90),
                FVector(62, Side * 61.0f, 35), FVector(82, Side * 56.0f, 12), PeatSkin);
            AddTetraPoints(FVector(-48, Side * 29.0f, 19), FVector(-20, Side * 34.0f, 20),
                FVector(-29, Side * 29.0f, -38), FVector(-3, Side * 26.0f, -45), Bark);
            AddTetraPoints(FVector(-77, Side * 17.0f, 130), FVector(-52, Side * 20.0f, 132),
                FVector(-71, Side * 13.0f, 159), FVector(-44, Side * 15.0f, 145), Moss);
        }
        AddTetra(FVector(-21, 0, 144), FVector(28, 39, 16), Moss);
        AddTetra(FVector(-56, 0, 119), FVector(26, 38, 13), Moss);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Boar)
    {
        const FLinearColor BoarHide(0.20f, 0.11f, 0.065f);
        const FLinearColor BoarShoulder(0.27f, 0.15f, 0.08f);
        const FLinearColor Bristle(0.12f, 0.075f, 0.045f);
        const FLinearColor Muzzle(0.34f, 0.21f, 0.13f);
        const FLinearColor Tusk(0.82f, 0.75f, 0.57f);
        const FLinearColor Nose(0.10f, 0.055f, 0.035f);
        const FLinearColor EyeRim(0.58f, 0.30f, 0.10f);
        const FLinearColor Eye(0.055f, 0.035f, 0.02f);

        AddEllipsoid(FVector(-29, 0, 20), FVector(84, 48, 43), BoarHide);
        AddEllipsoid(FVector(15, 0, 20), FVector(48, 45, 38), BoarShoulder);
        AddEllipsoid(FVector(71, 0, 29), FVector(35, 30, 26), BoarHide);
        AddEllipsoid(FVector(104, 0, 17), FVector(26, 23, 17), Muzzle);
        // A low barrel, joined chest/head masses, and blunt snout distinguish the boar from the deer.

        // Short rounded legs overlap the barrel and chest, with small feet resting at the same ground line.
        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-64.0f, 37.0f})
            {
                const float LegY = Side * 35.0f;
                AddEllipsoid(FVector(LegX, LegY, -15), FVector(14, 12, 25), BoarShoulder);
                AddEllipsoid(FVector(LegX + 4.0f, LegY, -45), FVector(16, 14, 8), Bristle);
            }

            // Side-set eyes, short ears, and upward ivory tusks make the head readable from every view.
            AddEllipsoid(FVector(52, Side * 25.0f, 51), FVector(11, 8, 10), Bristle);
            AddEllipsoid(FVector(55, Side * 29.0f, 52), FVector(5, 3, 5), BoarShoulder);
            AddEllipsoid(FVector(82, Side * 26.0f, 38), FVector(5, 4, 5), EyeRim);
            AddEllipsoid(FVector(84, Side * 29.0f, 38), FVector(2, 2, 2), Eye);
            AddEllipsoid(FVector(126, Side * 7.0f, 19), FVector(3, 3, 3), Nose);
            AddTetraPoints(FVector(78, Side * 19.0f, 3), FVector(91, Side * 23.0f, 3),
                FVector(103, Side * 17.0f, 28), FVector(109, Side * 23.0f, 6), Tusk);
        }

        // A short rounded bristle ridge follows the spine above the shoulder.
        AddEllipsoid(FVector(-50, 0, 58), FVector(48, 12, 8), Bristle);
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

        AddEllipsoid(FVector(-34, 0, 40), FVector(88, 31, 45), DeerHide);
        AddEllipsoid(FVector(107, 0, 98), FVector(30, 16, 20), DeerHide);
        // A raised neck, long legs, and antlers give the herd animal a light, alert outline.
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

        AddEllipsoid(FVector(-15, 0, 24), FVector(47, 24, 22), Fur);
        AddEllipsoid(FVector(43, 0, 25), FVector(25, 16, 15), Fur);
        // Low oval body, long bare tail and large ears keep the tiny scavenger distinct from the herd.
        AddEllipsoid(FVector(-10, 0, 15), FVector(27, 13, 7), Belly);
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
        const FLinearColor Nose(0.075f, 0.055f, 0.045f);
        const FLinearColor Eye(0.035f, 0.024f, 0.018f);

        const float HeadLift = bWarning ? 8.0f : 0.0f;
        AddEllipsoid(FVector(-20, 0, 37), FVector(80, 44, 32), Coat);
        AddEllipsoid(FVector(18, 0, 36), FVector(45, 39, 29), Coat);
        AddEllipsoid(FVector(50, 0, 49 + HeadLift), FVector(32, 28, 21), CoatLight);
        AddEllipsoid(FVector(78, 0, 42 + HeadLift), FVector(24, 22, 13), Muzzle);
        AddEllipsoid(FVector(100, 0, 43 + HeadLift), FVector(6, 11, 7), Nose);

        // Draw the white badger blaze as a single surface that follows the crown
        // from the nose bridge to the back of the head.
        const auto AddCrownStripeSegment = [&Vertices, &Triangles, &Normals, &UV, &Colors,
            &FaceStripe](const FVector& Start, const FVector& End, const float HalfWidth)
        {
            const int32 FirstVertex = Vertices.Num();
            const FVector Points[] = {
                Start + FVector(0, -HalfWidth, 0), End + FVector(0, -HalfWidth, 0),
                End + FVector(0, HalfWidth, 0), Start + FVector(0, HalfWidth, 0)
            };
            const FVector Normal = FVector::CrossProduct(Points[1] - Points[0],
                Points[3] - Points[0]).GetSafeNormal();
            Triangles.Append({FirstVertex, FirstVertex + 2, FirstVertex + 1,
                FirstVertex, FirstVertex + 3, FirstVertex + 2});
            for (const FVector& Point : Points)
            {
                Vertices.Add(Point);
                Normals.Add(Normal);
                UV.Add(FVector2D::ZeroVector);
                Colors.Add(FaceStripe);
            }
        };
        const FVector Crown[] = {
            FVector(38, 0, 68 + HeadLift), FVector(55, 0, 70 + HeadLift),
            FVector(72, 0, 65 + HeadLift), FVector(85, 0, 58 + HeadLift),
            FVector(98, 0, 52 + HeadLift), FVector(105, 0, 49 + HeadLift)
        };
        for (int32 Segment = 0; Segment < UE_ARRAY_COUNT(Crown) - 1; ++Segment)
        {
            AddCrownStripeSegment(Crown[Segment], Crown[Segment + 1], 4.0f);
        }

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(61, Side * 23.0f, 52 + HeadLift), FVector(4, 3, 3), Eye);
            AddEllipsoid(FVector(38, Side * 22.0f, 67 + HeadLift), FVector(8, 7, 8), Coat);
            AddEllipsoid(FVector(40, Side * 27.0f, 68 + HeadLift), FVector(4, 2, 4), Muzzle);

            for (const float LegX : {-58.0f, 37.0f})
            {
                const float LegY = Side * 27.0f;
                AddEllipsoid(FVector(LegX, LegY, -7), FVector(15, 14, 24), Coat);
                AddEllipsoid(FVector(LegX + 3.0f, LegY, -40), FVector(17, 15, 12), Coat);
                AddEllipsoid(FVector(LegX + 19.0f, LegY, -47), FVector(5, 5, 3), Claw);
            }
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

        AddEllipsoid(FVector(-26, 0, 55), FVector(116, 61, 65), Coat);
        AddEllipsoid(FVector(44, 0, 61), FVector(82, 60, 65), Coat);
        const float HeadZ = bWarning ? 96.0f : 81.0f;
        AddEllipsoid(FVector(94, 0, HeadZ), FVector(48, 40, 41), Shoulder);
        AddEllipsoid(FVector(135, 0, HeadZ - 20.0f), FVector(31, 27, 21), Muzzle);
        AddEllipsoid(FVector(160, 0, HeadZ - 18.0f), FVector(9, 15, 11), Eye);

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(66, Side * 31.0f, HeadZ + 25.0f), FVector(13, 11, 14), Coat);
            AddEllipsoid(FVector(111, Side * 34.0f, HeadZ - 3.0f), FVector(5, 4, 5), Eye);

            // Rounded legs meet broad paws; the near forepaw lifts for the overhead strike.
            for (const float LegX : {-72.0f, 42.0f})
            {
                const bool bRaisedPaw = bWarning && LegX > 0.0f && Side < 0.0f;
                const float LegY = Side * 46.0f;
                const float PawX = bRaisedPaw ? 105.0f : LegX + 15.0f;
                if (bRaisedPaw)
                {
                    AddEllipsoid(FVector(76, LegY, 66), FVector(27, 24, 43), Coat);
                    AddEllipsoid(FVector(PawX, LegY, 106), FVector(31, 27, 22), Warning);
                }
                else
                {
                    AddEllipsoid(FVector(LegX, LegY, -2), FVector(29, 25, 43), Coat);
                    AddEllipsoid(FVector(PawX, LegY, -42), FVector(36, 30, 14), Shoulder);
                    AddEllipsoid(FVector(PawX + 28.0f, LegY, -51), FVector(9, 6, 4), Claw);
                }
            }
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
        const float HeadX = bCharging ? 104.0f : 91.0f;
        const float HeadZ = bWarning ? 92.0f : (bRetreating ? 112.0f : 116.0f);

        AddEllipsoid(FVector(-22, 0, 84), FVector(125, 67, 59), SnowCoat);
        AddEllipsoid(FVector(39, 0, 91), FVector(84, 65, 60), SnowCoat);
        // Broad shoulders, pale coat and long muzzle distinguish the original
        // Tundra charger from the smaller Elderwood Moon Bear.
        AddEllipsoid(FVector(72, 0, 101), FVector(55, 49, 47), Cream);
        AddEllipsoid(FVector(HeadX, 0, HeadZ), FVector(44, 39, 38), Cream);
        AddEllipsoid(FVector(HeadX + 30.0f, 0, HeadZ - 15.0f), FVector(26, 22, 16), SnowCoat);
        AddEllipsoid(FVector(HeadX + 53.0f, 0, HeadZ - 12.0f), FVector(9, 13, 9), Nose);

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-74.0f, 42.0f})
            {
                const bool bFrontLeg = LegX > 0.0f;
                const bool bBracedPaw = bWarning && bFrontLeg && Side < 0.0f;
                const float PawX = bBracedPaw ? LegX + 45.0f : LegX + (bCharging && bFrontLeg ? 45.0f : 16.0f);
                const float PawY = Side * (bBracedPaw ? 43.0f : 49.0f);
                const float PawZ = bBracedPaw ? -4.0f : -44.0f;
                const float LegPoseX = LegX + (bBracedPaw ? 15.0f : (bCharging && bFrontLeg ? 14.0f : 0.0f));
                const float LegPoseZ = bBracedPaw ? 27.0f : (bCharging && bFrontLeg ? 2.0f : -3.0f);
                AddEllipsoid(FVector(LegPoseX, Side * 50.0f, LegPoseZ), FVector(28, 25, 40),
                    bFrontLeg ? SnowCoat : ShadowCoat);
                AddEllipsoid(FVector(PawX, PawY, PawZ), FVector(35, 29, bBracedPaw ? 17.0f : 14.0f),
                    bBracedPaw ? Cream : ShadowCoat);
            }
            AddEllipsoid(FVector(HeadX - 23.0f, Side * 26.0f, HeadZ + 27.0f),
                FVector(12, 10, 13), ShadowCoat);
            AddEllipsoid(FVector(HeadX + 10.0f, Side * 35.0f, HeadZ + 5.0f),
                FVector(5, 3, 4), Eye);
        }
        if (bWarning)
        {
            // A braced paw and lowered muzzle give the charge a full second warning.
            AddEllipsoid(FVector(HeadX + 37.0f, 0, HeadZ - 24.0f), FVector(12, 4, 3), Nose);
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
        const FLinearColor Nose(0.16f, 0.12f, 0.09f);
        const FLinearColor Eye(0.95f, 0.58f, 0.12f);
        const float HeadX = bWarning ? 84.0f : 78.0f;
        const float HeadZ = bWarning ? 48.0f : 60.0f;

        AddEllipsoid(FVector(-23, 0, 49), FVector(82, 42, 39), Coat);
        AddEllipsoid(FVector(23, 0, 55), FVector(53, 42, 43), Coat);
        // A raised neck, compact muzzle, beard and swept-back paired horns
        // identify the mountain goat; its lowered warning pose precedes the charge.
        AddEllipsoid(FVector(48, 0, HeadZ + 6.0f), FVector(36, 30, 34), Neck);
        AddEllipsoid(FVector(HeadX, 0, HeadZ), FVector(32, 25, 26), CoatLight);
        AddEllipsoid(FVector(HeadX + 24.0f, 0, HeadZ - 10.0f), FVector(23, 18, 14), CoatLight);
        AddEllipsoid(FVector(HeadX + 43.0f, 0, HeadZ - 11.0f), FVector(7, 11, 7), Nose);
        AddEllipsoid(FVector(HeadX + 7.0f, 0, HeadZ - 24.0f), FVector(9, 8, 10), Neck);
        AddEllipsoid(FVector(-101, 0, 44), FVector(17, 8, 8), CoatLight);

        for (const float LegX : {-58.0f, 35.0f})
        {
            for (const float Side : {-1.0f, 1.0f})
            {
                const float LegY = Side * 27.0f;
                AddEllipsoid(FVector(LegX, LegY, -7), FVector(17, 15, 29), Coat);
                AddEllipsoid(FVector(LegX + 5.0f, LegY, -45), FVector(17, 14, 10), Hoof);
                AddEllipsoid(FVector(LegX + 12.0f, LegY - 5.0f, -48), FVector(10, 6, 7), Hoof);
                AddEllipsoid(FVector(LegX + 12.0f, LegY + 5.0f, -48), FVector(10, 6, 7), Hoof);
            }
        }

        const auto AddHorn = [&Vertices, &Triangles, &Normals, &UV, &Colors, &Horn]
            (const FVector* Path, const float* Radii, const int32 PathCount)
        {
            constexpr int32 CrossSectionCount = 8;
            const FVector AxisY(0, 1, 0);
            TArray<TArray<int32>> RingIndices;
            RingIndices.SetNum(PathCount);

            for (int32 PointIndex = 0; PointIndex < PathCount; ++PointIndex)
            {
                const int32 PreviousIndex = FMath::Max(0, PointIndex - 1);
                const int32 NextIndex = FMath::Min(PathCount - 1, PointIndex + 1);
                const FVector Tangent = (Path[NextIndex] - Path[PreviousIndex]).GetSafeNormal();
                const FVector Perpendicular = FVector::CrossProduct(Tangent, AxisY).GetSafeNormal();
                TArray<int32>& Ring = RingIndices[PointIndex];
                Ring.Reserve(CrossSectionCount);

                for (int32 SectionIndex = 0; SectionIndex < CrossSectionCount; ++SectionIndex)
                {
                    const float Angle = 2.0f * PI * static_cast<float>(SectionIndex)
                        / static_cast<float>(CrossSectionCount);
                    const FVector Normal = (Perpendicular * FMath::Cos(Angle) + AxisY * FMath::Sin(Angle))
                        .GetSafeNormal();
                    Ring.Add(Vertices.Add(Path[PointIndex] + Normal * Radii[PointIndex]));
                    Normals.Add(Normal);
                    UV.Add(FVector2D::ZeroVector);
                    Colors.Add(Horn);
                }
            }

            const auto AddHornTriangle = [&Vertices, &Triangles, &Normals](const int32 A, int32 B, int32 C)
            {
                const FVector FaceNormal = FVector::CrossProduct(Vertices[B] - Vertices[A],
                    Vertices[C] - Vertices[A]).GetSafeNormal();
                const FVector ExpectedNormal = (Normals[A] + Normals[B] + Normals[C]).GetSafeNormal();
                if (FVector::DotProduct(FaceNormal, ExpectedNormal) < 0.0f)
                    Triangles.Append({A, C, B});
                else
                    Triangles.Append({A, B, C});
            };

            for (int32 RingIndex = 0; RingIndex < PathCount - 1; ++RingIndex)
            {
                for (int32 SectionIndex = 0; SectionIndex < CrossSectionCount; ++SectionIndex)
                {
                    const int32 NextSection = (SectionIndex + 1) % CrossSectionCount;
                    const int32 A = RingIndices[RingIndex][SectionIndex];
                    const int32 B = RingIndices[RingIndex][NextSection];
                    const int32 C = RingIndices[RingIndex + 1][SectionIndex];
                    const int32 D = RingIndices[RingIndex + 1][NextSection];
                    AddHornTriangle(A, B, C);
                    AddHornTriangle(B, D, C);
                }
            }

            const FVector StartTangent = (Path[1] - Path[0]).GetSafeNormal();
            const int32 StartCap = Vertices.Add(Path[0] - StartTangent * 2.0f);
            Normals.Add(-StartTangent);
            UV.Add(FVector2D::ZeroVector);
            Colors.Add(Horn);
            const FVector EndTangent = (Path[PathCount - 1] - Path[PathCount - 2]).GetSafeNormal();
            const int32 Tip = Vertices.Add(Path[PathCount - 1] + EndTangent * 2.0f);
            Normals.Add(EndTangent);
            UV.Add(FVector2D::ZeroVector);
            Colors.Add(Horn);
            for (int32 SectionIndex = 0; SectionIndex < CrossSectionCount; ++SectionIndex)
            {
                const int32 NextSection = (SectionIndex + 1) % CrossSectionCount;
                AddHornTriangle(StartCap, RingIndices[0][NextSection], RingIndices[0][SectionIndex]);
                AddHornTriangle(Tip, RingIndices.Last()[SectionIndex], RingIndices.Last()[NextSection]);
            }
        };

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(HeadX - 22.0f, Side * 22.0f, HeadZ + 2.0f),
                FVector(12, 8, 7), Neck);
            AddEllipsoid(FVector(HeadX + 10.0f, Side * 22.0f, HeadZ + 3.0f),
                FVector(4, 3, 4), Eye);

            // Each swept horn is one tapered surface rooted inside the skull.
            const FVector HornPath[] = {
                FVector(HeadX - 10.0f, Side * 12.0f, HeadZ + 15.0f),
                FVector(HeadX - 16.0f, Side * 14.0f, HeadZ + 28.0f),
                FVector(HeadX - 27.0f, Side * 16.0f, HeadZ + 38.0f),
                FVector(HeadX - 39.0f, Side * 18.0f, HeadZ + 39.0f),
                FVector(HeadX - 49.0f, Side * 19.0f, HeadZ + 30.0f),
                FVector(HeadX - 51.0f, Side * 20.0f, HeadZ + 17.0f)
            };
            const float HornRadii[] = {10.0f, 9.0f, 7.5f, 6.5f, 4.5f, 1.5f};
            AddHorn(HornPath, HornRadii, UE_ARRAY_COUNT(HornPath));
        }
        if (bWarning)
            AddEllipsoid(FVector(HeadX + 34.0f, 0, HeadZ - 19.0f), FVector(11, 3, 2), Nose);
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

        // Build the mountain sentinel from overlapping stone masses and
        // articulated limbs; sharp geometry is kept for small mineral details.
        AddEllipsoid(FVector(-15,0,153), FVector(102,79,125), StoneShadow);
        AddEllipsoid(FVector(-13,0,229), FVector(119,91,75), StoneBase);
        AddEllipsoid(FVector(19,0,336), FVector(52,47,56), StoneLight);
        AddEllipsoid(FVector(57,0,322), FVector(32,39,27), StoneBase);
        AddEllipsoid(FVector(-92,0,ShoulderZ - 8.0f), FVector(58,72,61), StoneLight);
        AddEllipsoid(FVector(80,0,ShoulderZ - 8.0f), FVector(57,70,58), StoneBase);

        // The left arm hangs beside the torso; the right arm follows the
        // replicated telegraph, slam, and recovery poses.
        AddEllipsoid(FVector(-98,0,210), FVector(41,42,73), StoneBase);
        AddEllipsoid(FVector(-137,-4,123), FVector(36,38,68), StoneLight);
        AddEllipsoid(FVector(-151,-8,54), FVector(47,47,35), StoneShadow);
        AddEllipsoid(FVector(91,-43,ShoulderZ - 43.0f), FVector(47,42,58), StoneLight);
        const float ElbowZ = FMath::Lerp(ShoulderZ, RaisedFistZ, 0.55f);
        const float ForearmHalfHeight = FMath::Max(39.0f, FMath::Abs(RaisedFistZ - ShoulderZ) * 0.42f);
        AddEllipsoid(FVector((RaisedFistX + 87.0f) * 0.5f,-57,ElbowZ),
            FVector(42,42,ForearmHalfHeight), bWarning ? Warning : StoneBase);
        AddEllipsoid(FVector(RaisedFistX,-64,RaisedFistZ), FVector(42,40,39),
            bSlamming ? Warning : StoneLight);

        AddEllipsoid(FVector(-53,-30,43), FVector(43,40,66), StoneBase);
        AddEllipsoid(FVector(54,30,43), FVector(43,40,66), StoneLight);
        AddEllipsoid(FVector(-53,-34,7), FVector(56,48,29), StoneShadow);
        AddEllipsoid(FVector(61,35,7), FVector(56,48,29), StoneShadow);

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(54,Side * 37.0f,347), FVector(8,7,9), bWarning ? Warning : ThunderVein);
            AddEllipsoid(FVector(60,Side * 42.0f,347), FVector(3,2,4), StoneShadow);
            AddEllipsoid(FVector(30,Side * 67.0f,229), FVector(28,15,43),
                bWarning ? Warning : ThunderVein);
            AddEllipsoid(FVector(-100,Side * 62.0f,280), FVector(30,17,25), StoneLight);
        }
        if (bSlamming)
        {
            // A bright fist at the ground makes the one-hit impact phase visible to clients.
            AddEllipsoid(FVector(188,-68,15), FVector(48,39,23), Warning);
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
        AddEllipsoid(FVector(-18, 0, 17), FVector(137, 62, 42), ReefBack);
        AddEllipsoid(FVector(-24, 0, 47), FVector(88, 48, 20), ReefLight);
        AddEllipsoid(FVector(-20, 0, -9), FVector(96, 50, 20), FLinearColor(0.24f,0.53f,0.52f));

        // Original deep-bodied reef predator with a forked tail, coral ridges,
        // angular fins, and a warning-to-bite jaw pose.
        AddTetraPoints(FVector(-162,-8,10), FVector(-162,8,10),
            FVector(-205,-52,4), FVector(-205,52,4), ReefShadow);
        AddTetraPoints(FVector(-205,-52,4), FVector(-205,52,4),
            FVector(-239,-40,14), FVector(-239,40,14), ReefLight);
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
        const float HeadX = bLunging ? 140.0f : 117.0f;
        const float HeadZ = bTelegraph ? 78.0f : (bLunging ? 50.0f : 62.0f);

        AddEllipsoid(FVector(-20, 0, 58), FVector(98, 48, 52), WinterCoat);
        AddEllipsoid(FVector(42, 0, 63), FVector(59, 46, 54), WinterCoat);
        // A continuous shoulder mantle, throat, head, and long muzzle define the winter wolf.
        AddEllipsoid(FVector(-31, 0, bTelegraph ? 93.0f : 88.0f), FVector(61, 42, 21), Mantle);
        AddEllipsoid(FVector(HeadX - 33.0f, 0, HeadZ - 4.0f), FVector(42, 31, 30), Throat);
        AddEllipsoid(FVector(HeadX, 0, HeadZ), FVector(33, 25, 24), WinterCoat);
        AddEllipsoid(FVector(HeadX + 20.0f, 0, HeadZ - 9.0f), FVector(23, 19, 14), Muzzle);
        AddEllipsoid(FVector(HeadX + 42.0f, 0, HeadZ - 10.0f), FVector(7, 11, 6), Paw);

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(HeadX - 18.0f, Side * 12.0f, HeadZ + 14.0f),
                FVector(8, 7, 11), Mantle);
            AddEllipsoid(FVector(HeadX - 19.0f, Side * 12.0f, HeadZ + 22.0f),
                FVector(4, 4, 6), Paw);
            AddEllipsoid(FVector(HeadX + 2.0f, Side * 21.0f, HeadZ + 4.0f),
                FVector(4, 3, 4), Eye);

            for (const float LegX : {-60.0f, 34.0f})
            {
                const bool bFrontLeg = LegX > 0.0f;
                const bool bRaised = bTelegraph && bFrontLeg && Side < 0.0f;
                const float LegY = Side * 33.0f;
                const float FootX = bLunging && bFrontLeg ? LegX + 42.0f
                    : LegX + (bRaised ? 23.0f : 8.0f);
                const float FootZ = bRaised ? -12.0f : (bLunging ? -45.0f : -47.0f);
                const float LegOffsetX = bLunging && bFrontLeg ? 12.0f : 0.0f;
                AddEllipsoid(FVector(LegX + LegOffsetX, LegY, -6), FVector(18, 16, 36),
                    bFrontLeg ? WinterCoat : Mantle);
                AddEllipsoid(FVector(FootX, LegY, FootZ), FVector(22, 17, 12), Paw);
            }
        }

        AddEllipsoid(FVector(-106, 0, 49), FVector(34, 15, 13), Mantle);
        AddEllipsoid(FVector(-135, 0, 38), FVector(20, 11, 10), WinterCoat);
        if (bTelegraph)
        {
            // The lifted paw and raised muzzle communicate the shared half-second warning.
            AddEllipsoid(FVector(HeadX + 22.0f, 0, HeadZ - 12.0f), FVector(9, 16, 4), Paw);
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
        const float HeadX = bLeaping ? 123.0f : 103.0f;
        const float HeadZ = bTelegraph ? 54.0f : (bLeaping ? 72.0f : 63.0f);

        AddEllipsoid(FVector(-18, 0, 47), FVector(86, 40, 41), SnowCoat);
        AddEllipsoid(FVector(39, 0, 49), FVector(56, 39, 39), SnowCoat);
        // A compact spotted body, cheek ruffs, short tail and tufted ears distinguish
        // this Tundra pouncer from the longer, darker Wolf.
        AddEllipsoid(FVector(-30, 0, bTelegraph ? 70.0f : 76.0f), FVector(50, 32, 13), BackCoat);
        AddEllipsoid(FVector(HeadX - 31.0f, 0, HeadZ - 6.0f), FVector(43, 29, 28), Cream);
        AddEllipsoid(FVector(HeadX, 0, HeadZ), FVector(30, 23, 24), SnowCoat);
        AddEllipsoid(FVector(HeadX + 19.0f, 0, HeadZ - 10.0f), FVector(21, 18, 12), Cream);
        AddEllipsoid(FVector(HeadX + 38.0f, 0, HeadZ - 10.0f), FVector(6, 10, 5), Nose);

        for (int32 SpotIndex = 0; SpotIndex < 6; ++SpotIndex)
        {
            const float X = -48.0f + static_cast<float>(SpotIndex) * 15.0f;
            const float Side = SpotIndex % 2 == 0 ? -1.0f : 1.0f;
            const float Z = 38.0f + (SpotIndex % 3) * 7.0f;
            const float BodySurfaceY = 40.0f * FMath::Sqrt(FMath::Max(0.0f,
                1.0f - FMath::Square((X + 18.0f) / 86.0f) - FMath::Square((Z - 47.0f) / 41.0f)));
            AddEllipsoid(FVector(X, Side * (BodySurfaceY + 0.2f), Z), FVector(5, 1.1f, 4), Spot);
        }

        for (const float Side : {-1.0f, 1.0f})
        {
            for (const float LegX : {-54.0f, 31.0f})
            {
                const bool bForeleg = LegX > 0.0f;
                const bool bCrouchedForeleg = bTelegraph && bForeleg && Side < 0.0f;
                const float FootX = bLeaping ? LegX + (bForeleg ? 45.0f : -24.0f)
                    : (LegX + (bCrouchedForeleg ? 24.0f : 4.0f));
                const float FootZ = bCrouchedForeleg ? -13.0f : (bLeaping ? -31.0f : -44.0f);
                const float LegOffsetX = bLeaping && bForeleg ? 15.0f : 0.0f;
                const float LegY = Side * 28.0f;
                AddEllipsoid(FVector(LegX + LegOffsetX, LegY, -5), FVector(17, 15, 35),
                    bForeleg ? SnowCoat : BackCoat);
                AddEllipsoid(FVector(FootX, LegY, FootZ), FVector(20, 16, 11), BackCoat);
            }

            // Full cheek ruffs and short black-tipped ear tufts define the lynx face.
            AddEllipsoid(FVector(HeadX - 9.0f, Side * 20.0f, HeadZ - 6.0f),
                FVector(16, 10, 13), Cream);
            AddEllipsoid(FVector(HeadX - 14.0f, Side * 11.0f, HeadZ + 17.0f),
                FVector(8, 7, 12), BackCoat);
            AddEllipsoid(FVector(HeadX - 14.0f, Side * 12.0f, HeadZ + 24.0f),
                FVector(4, 4, 7), EarTip);
            AddEllipsoid(FVector(HeadX + 4.0f, Side * 19.0f, HeadZ + 5.0f),
                FVector(4, 3, 4), Eye);
        }
        AddEllipsoid(FVector(-94, 0, 37), FVector(27, 13, 12), BackCoat);
        AddEllipsoid(FVector(-113, 0, 30), FVector(15, 10, 9), EarTip);
        if (bTelegraph)
        {
            // The lowered shoulders and tucked forelegs expose the half-second pounce warning.
            AddEllipsoid(FVector(5, 0, 40), FVector(20, 9, 4), Spot);
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
        const float HeadHeight = bGiant ? (bCoiled ? 112.0f : 78.0f)
            : (bDeathAdder ? (bCoiled ? 76.0f : 51.0f) : (bCoiled ? 64.0f : 51.0f));
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
            AddEllipsoid(FVector(X, Curve, Z + Radius * 0.2f),
                FVector(SegmentHalf * 1.65f, Radius, Radius * 0.78f), BodyColour);
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
        if (bDeathAdder)
        {
            const float HeadX = NeckX + 30.0f;
            const float NeckZ = bCoiled ? 43.0f : 24.0f;
            AddEllipsoid(FVector(NeckX + 11.0f, 0.0f, NeckZ),
                FVector(25.0f, BodyRadius * 0.84f, bCoiled ? 39.0f : 22.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX, 0.0f, HeadZ), FVector(24.0f, 20.0f, 19.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX + 19.0f, 0.0f, HeadZ - 9.0f), FVector(15.0f, 16.0f, 10.0f), Belly);
            AddEllipsoid(FVector(HeadX + 31.0f, 0.0f, HeadZ - 9.0f), FVector(6.0f, 10.0f, 5.0f), ScaleDark);
            for (const float Side : {-1.0f, 1.0f})
            {
                AddEllipsoid(FVector(HeadX + 4.0f, Side * 15.0f, HeadZ + 6.0f),
                    FVector(4.0f, 3.0f, 4.0f), Eye);
            }
        }
        else if (!bGiant)
        {
            const float HeadX = NeckX + 23.0f;
            const float NeckZ = bCoiled ? 35.0f : 21.0f;
            AddEllipsoid(FVector(NeckX + 10.0f, 0.0f, NeckZ),
                FVector(19.0f, BodyRadius * 0.75f, bCoiled ? 31.0f : 20.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX, 0.0f, HeadZ), FVector(18.0f, 15.0f, 14.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX + 13.0f, 0.0f, HeadZ - 7.0f), FVector(12.0f, 12.0f, 7.0f), Belly);
            AddEllipsoid(FVector(HeadX + 24.0f, 0.0f, HeadZ - 8.0f), FVector(5.0f, 7.0f, 4.0f), ScaleDark);
            for (const float Side : {-1.0f, 1.0f})
            {
                AddEllipsoid(FVector(HeadX + 4.0f, Side * 11.0f, HeadZ + 4.0f),
                    FVector(3.0f, 2.0f, 3.0f), Eye);
            }
        }
        else
        {
            const float HeadX = NeckX + 43.0f;
            const float NeckZ = bCoiled ? 80.0f : 42.0f;
            AddEllipsoid(FVector(NeckX + 17.0f, 0.0f, NeckZ),
                FVector(34.0f, BodyRadius * 0.86f, bCoiled ? 54.0f : 32.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX, 0.0f, HeadZ), FVector(38.0f, 31.0f, 29.0f), ScaleBase);
            AddEllipsoid(FVector(HeadX + 30.0f, 0.0f, HeadZ - 11.0f),
                FVector(26.0f, 29.0f, 16.0f), Belly);
            AddEllipsoid(FVector(HeadX + 51.0f, 0.0f, HeadZ - 10.0f),
                FVector(8.0f, 17.0f, 11.0f), ScaleDark);
            for (const float Side : {-1.0f, 1.0f})
            {
                AddEllipsoid(FVector(HeadX + 7.0f, Side * 25.0f, HeadZ + 8.0f),
                    FVector(5.0f, 4.0f, 5.0f), Eye);
            }
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
            AddEllipsoid(Centre, FVector(7.0f, 4.0f, 4.0f), Body);
            AddEllipsoid(Centre + FVector(-5.0f, 0.0f, 1.0f), FVector(4.0f, 3.0f, 3.0f), Thorax);
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

        // Broad overlapping masses make a compact peat-and-root brute, with a
        // readable head, shoulders, belly, and root feet instead of a stack of
        // disconnected pyramid plates.
        AddEllipsoid(FVector(-17,0,73), FVector(70,59,82), RootShadow);
        AddEllipsoid(FVector(-7,0,125), FVector(77,64,65), PeatBark);
        AddEllipsoid(FVector(-28,0,178), FVector(55,51,43), RootShadow);
        AddEllipsoid(FVector(34,0,174), FVector(48,45,40), Lichen);
        AddEllipsoid(FVector(72,0,151), FVector(28,35,24), PeatBark);
        AddEllipsoid(FVector(45,-30,121), FVector(35,27,37), Lichen);
        AddEllipsoid(FVector(2,38,120), FVector(40,29,42), RootShadow);

        for (const float Side : {-1.0f, 1.0f})
        {
            AddEllipsoid(FVector(67,Side * 33.0f,184), FVector(8,7,9),
                bWarning ? SweepWarning : WarmEye);
            AddEllipsoid(FVector(72,Side * 37.0f,184), FVector(3,2,4), RootShadow);
            const float LegX = Side < 0.0f ? -48.0f : 22.0f;
            AddEllipsoid(FVector(LegX,Side * 29.0f,7), FVector(31,27,48), RootShadow);
            AddEllipsoid(FVector(LegX + 15.0f,Side * 32.0f,-20), FVector(38,25,18), PeatBark);
        }

        if (bWarning)
        {
            // A raised glowing forearm and fist preserve the server warning pose.
            AddEllipsoid(FVector(26,-53,112), FVector(37,28,48), PeatBark);
            AddEllipsoid(FVector(64,-62,159), FVector(31,24,47), SweepWarning);
            AddEllipsoid(FVector(93,-62,196), FVector(25,23,25), SweepWarning);
        }
        else if (bRecovering)
        {
            // The arm follows through low and forward during the server recovery timer.
            AddEllipsoid(FVector(28,-53,105), FVector(37,28,48), PeatBark);
            AddEllipsoid(FVector(71,-62,62), FVector(30,23,48), PeatBark);
            AddEllipsoid(FVector(105,-64,20), FVector(25,23,23), SweepWarning);
        }
        else
        {
            AddEllipsoid(FVector(27,-53,107), FVector(37,28,48), PeatBark);
            AddEllipsoid(FVector(65,-62,62), FVector(29,23,45), RootShadow);
            AddEllipsoid(FVector(91,-65,16), FVector(25,23,24), RootShadow);
        }
        AddEllipsoid(FVector(21,52,101), FVector(31,24,48), PeatBark);
        AddEllipsoid(FVector(63,59,56), FVector(28,22,43), RootShadow);
        AddEllipsoid(FVector(89,62,12), FVector(24,22,23), RootShadow);
        AddEllipsoid(FVector(23,-51,142), FVector(27,16,31), Lichen);
        AddEllipsoid(FVector(-58,-16,183), FVector(28,18,21), Lichen);
        AddEllipsoid(FVector(-73,12,159), FVector(20,17,16), Lichen);
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
    if (!HasAuthority()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWolfPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::Wolf)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(WolfAttackCue);
        UE_LOG(LogTemp, Display, TEXT("Wolf verification client received actor: SpawnId=%s Cue=%d."),
            *PersistentSpawnId, static_cast<int32>(Cue));
        if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
        {
            UE_LOG(LogTemp, Display, TEXT("Wolf verification client observed replicated telegraph: SpawnId=%s."),
                *PersistentSpawnId);
        }
    }
    if (!HasAuthority()
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaPolarBearPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::PolarBear)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(PolarBearAttackCue);
        UE_LOG(LogTemp, Display, TEXT("Polar Bear verification client received actor: SpawnId=%s Cue=%d."),
            *PersistentSpawnId, static_cast<int32>(Cue));
        if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
        {
            UE_LOG(LogTemp, Display, TEXT("Polar Bear verification client observed replicated telegraph: SpawnId=%s."),
                *PersistentSpawnId);
        }
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
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaLynxPeerTest"))
        && static_cast<EKalmalaWildlifeAttackCue>(LynxAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Lynx verification client observed replicated telegraph: SpawnId=%s."),
            *PersistentSpawnId);
    }
#endif
}

void AKalmalaWildlifeSpawn::OnRep_PolarBearAttackCue()
{
    BuildArchetypePresentation();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaPolarBearPeerTest"))
        && static_cast<EKalmalaWildlifeAttackCue>(PolarBearAttackCue) == EKalmalaWildlifeAttackCue::Telegraph)
    {
        UE_LOG(LogTemp, Display, TEXT("Polar Bear verification client observed replicated telegraph: SpawnId=%s."),
            *PersistentSpawnId);
    }
#endif
}

void AKalmalaWildlifeSpawn::OnRep_HornedGoatAttackCue()
{
    BuildArchetypePresentation();
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaHornedGoatPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::HornedGoat)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(HornedGoatAttackCue);
        UE_LOG(LogTemp, Display, TEXT("Horned Goat verification client received actor: SpawnId=%s Cue=%d."),
            *PersistentSpawnId, static_cast<int32>(Cue));
        if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
        {
            UE_LOG(LogTemp, Display, TEXT("Horned Goat verification client observed replicated telegraph: SpawnId=%s."),
                *PersistentSpawnId);
        }
    }
#endif
}

void AKalmalaWildlifeSpawn::OnRep_ThunderSentinelAttackCue()
{
    BuildArchetypePresentation();
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaThunderSentinelPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::ThunderSentinel)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ThunderSentinelAttackCue);
        if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
        {
            UE_LOG(LogTemp, Display, TEXT("Thunder Sentinel verification client observed replicated telegraph: SpawnId=%s."),
                *PersistentSpawnId);
        }
        else if (Cue == EKalmalaWildlifeAttackCue::Lunge)
        {
            UE_LOG(LogTemp, Display, TEXT("Thunder Sentinel verification client observed replicated slam: SpawnId=%s."),
                *PersistentSpawnId);
        }
    }
#endif
}

void AKalmalaWildlifeSpawn::OnRep_ReefMawAttackCue()
{
    BuildArchetypePresentation();
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaReefMawPeerTest"))
        && Archetype == EKalmalaWildlifeArchetype::ReefMaw)
    {
        const EKalmalaWildlifeAttackCue Cue = static_cast<EKalmalaWildlifeAttackCue>(ReefMawAttackCue);
        if (Cue == EKalmalaWildlifeAttackCue::Telegraph)
        {
            UE_LOG(LogTemp, Display, TEXT("Reef Maw verification client observed replicated telegraph: SpawnId=%s."),
                *PersistentSpawnId);
        }
        else if (Cue == EKalmalaWildlifeAttackCue::Lunge)
        {
            UE_LOG(LogTemp, Display, TEXT("Reef Maw verification client observed replicated bite: SpawnId=%s."),
                *PersistentSpawnId);
        }
    }
#endif
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
    if (BoneFragmentQuantity > 0 && !(Archetype == EKalmalaWildlifeArchetype::Mireling && FKalmalaGemstoneContract::DuskDiamondSourceAccepted)
        && Archetype != EKalmalaWildlifeArchetype::Rat
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
