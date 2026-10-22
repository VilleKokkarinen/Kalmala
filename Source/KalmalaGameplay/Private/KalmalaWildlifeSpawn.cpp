#include "KalmalaWildlifeSpawn.h"
#include "KalmalaResistanceComponent.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
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
    const EKalmalaWildlifeArchetype InitialArchetype)
{
    if (HasAuthority())
    {
        SetActorLocation(Spawn.Location);
        SpawnOrigin = Spawn.Location;
        BehaviourDestination = SpawnOrigin;
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        CreatureNicheId = Spawn.ContentId;
        Archetype = InitialArchetype;
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
        Health = GetMaximumHealthForArchetype(Archetype);
        if (UKalmalaResistanceComponent* Defenses = FindComponentByClass<UKalmalaResistanceComponent>())
        {
            Defenses->Profile.Armor = GetArmorForArchetype(Archetype);
        }
        BuildArchetypePresentation();
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
    case EKalmalaWildlifeArchetype::DeathAdder: Damage.Stab = 75.0f; break;
    case EKalmalaWildlifeArchetype::Viper: Damage.Stab = 50.0f; break;
    case EKalmalaWildlifeArchetype::GiantSnake: Damage.Stab = 100.0f; break;
    case EKalmalaWildlifeArchetype::Deer:
    default: break;
    }
    return Damage;
}

int32 AKalmalaWildlifeSpawn::GetDirectBoneFragmentRewardQuantity(const EKalmalaWildlifeArchetype InArchetype)
{
    switch (InArchetype)
    {
    case EKalmalaWildlifeArchetype::Boar:
    case EKalmalaWildlifeArchetype::Deer:
    case EKalmalaWildlifeArchetype::Badger:
    case EKalmalaWildlifeArchetype::Viper: return 1;
    case EKalmalaWildlifeArchetype::GiantSnake: return 2;
    default: return 0;
    }
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

bool AKalmalaWildlifeSpawn::ApplyArrowDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker)
{ return Attacker && ApplyValidatedDamageFromServer(Damage, Attacker, false); }

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

bool AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(const uint64 SpawnSeed, const EKalmalaBiome Biome,
    EKalmalaWildlifeArchetype& OutArchetype)
{
    if (Biome == EKalmalaBiome::Meadows && SpawnSeed % 10ull == 9ull)
    {
        OutArchetype = EKalmalaWildlifeArchetype::Rat;
        return true;
    }

    if (Biome == EKalmalaBiome::Elderwood)
    {
        // Keep the existing roster at 50%, use Badger for the 40% common share,
        // and leave the 10% hard-enemy share empty until its species is ready.
        switch (SpawnSeed % 10ull)
        {
        case 0: case 1: case 2: case 3: case 4:
            OutArchetype = GetArchetypeForSpawnSeed(SpawnSeed);
            return true;
        case 5: case 6: case 7: case 8:
            OutArchetype = EKalmalaWildlifeArchetype::Badger;
            return true;
        default:
            return false;
        }
    }

    if (Biome == EKalmalaBiome::ShimmeringLakes)
    {
        // Assign the 50/40/10 candidate-slot mix directly from the stable seed.
        // The first five of ten slots retain the existing Boar/Deer roster.
        switch (SpawnSeed % 10ull)
        {
        case 0: case 2: case 4: OutArchetype = EKalmalaWildlifeArchetype::Boar; return true;
        case 1: case 3: OutArchetype = EKalmalaWildlifeArchetype::Deer; return true;
        case 5: case 6: case 7: case 8: OutArchetype = EKalmalaWildlifeArchetype::Viper; return true;
        case 9: OutArchetype = EKalmalaWildlifeArchetype::GiantSnake; return true;
        default: return false;
        }
    }

    const bool bBoarDeerBiome = Biome == EKalmalaBiome::Meadows || Biome == EKalmalaBiome::Elderwood;
    if (!bBoarDeerBiome && (SpawnSeed % 3ull == 0ull || SpawnSeed % 5ull == 1ull)) return false;

    OutArchetype = GetArchetypeForSpawnSeed(SpawnSeed);
    return true;
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
    if (HasAuthority() && !bDefeated)
    {
        AdvanceServerBehaviour(DeltaSeconds);
        if (Archetype == EKalmalaWildlifeArchetype::Mireling) UpdateMirelingScavenge(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Boar) UpdateBoarTerritory(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Rat) UpdateRatAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Badger) UpdateBadgerAttack(DeltaSeconds);
        else if (Archetype == EKalmalaWildlifeArchetype::Viper || Archetype == EKalmalaWildlifeArchetype::GiantSnake)
            UpdateSnakeAmbush(DeltaSeconds);
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
    return FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, XY)) == EKalmalaBiome::ShimmeringLakes
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
    if (FVector::DistSquared(GetActorLocation(), Target->GetActorLocation()) > FMath::Square(180.0f)
        || ForwardDistance < 0.25f || LateralOffset.Size2D() > MaxLateral) return false;

    const FKalmalaDamageChannels BiteDamage = GetBaseAttackChannelsForArchetype(Archetype);
    if (!Target->ApplyWildlifeDamageFromServer(this, BiteDamage)) return false;
    if (Archetype == EKalmalaWildlifeArchetype::Viper)
    {
        if (UKalmalaPlayerStatusComponent* Statuses = Target->GetStatusComponent())
            Statuses->ApplyWeakPoisonFromServer(5.0f);
    }
    return true;
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
        RootSphere->SetSphereRadius(Archetype == EKalmalaWildlifeArchetype::Rat ? 42.0f : 70.0f, true);
    const bool bSnake = Archetype == EKalmalaWildlifeArchetype::Viper
        || Archetype == EKalmalaWildlifeArchetype::GiantSnake;
    if (SnakeBodyCollision != nullptr)
    {
        if (bSnake)
        {
            const bool bGiant = Archetype == EKalmalaWildlifeArchetype::GiantSnake;
            SnakeBodyCollision->SetCapsuleSize(bGiant ? 38.0f : 16.0f, bGiant ? 210.0f : 92.0f, true);
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
    else if (bSnake)
    {
        const bool bGiant = Archetype == EKalmalaWildlifeArchetype::GiantSnake;
        const bool bCoiled = static_cast<EKalmalaWildlifeAttackCue>(SnakeAttackCue)
            == EKalmalaWildlifeAttackCue::Telegraph;
        const float HalfLength = bGiant ? 180.0f : 76.0f;
        const float BodyRadius = bGiant ? 36.0f : 16.0f;
        const float CoilWidth = bGiant ? 62.0f : 27.0f;
        const float HeadHeight = bGiant ? (bCoiled ? 214.0f : 146.0f) : (bCoiled ? 104.0f : 69.0f);
        const FLinearColor ScaleBase = bGiant ? FLinearColor(0.12f, 0.20f, 0.16f) : FLinearColor(0.27f, 0.34f, 0.18f);
        const FLinearColor ScaleDark = bGiant ? FLinearColor(0.055f, 0.095f, 0.075f) : FLinearColor(0.13f, 0.19f, 0.11f);
        const FLinearColor ScaleMark = bGiant ? FLinearColor(0.53f, 0.47f, 0.27f) : FLinearColor(0.70f, 0.60f, 0.31f);
        const FLinearColor Belly = bGiant ? FLinearColor(0.40f, 0.34f, 0.20f) : FLinearColor(0.72f, 0.65f, 0.43f);
        const FLinearColor Eye(0.95f, 0.72f, 0.12f);
        const int32 BodySegments = bGiant ? 16 : 9;
        for (int32 Segment = 0; Segment < BodySegments; ++Segment)
        {
            const float T = static_cast<float>(Segment) / static_cast<float>(BodySegments - 1);
            const float X = FMath::Lerp(-HalfLength, HalfLength * 0.60f, T);
            const float Curve = FMath::Sin(T * 3.0f * PI) * CoilWidth;
            const float Z = BodyRadius * 0.45f + FMath::Sin(T * 2.0f * PI) * BodyRadius * 0.20f;
            const float SegmentHalf = bGiant ? 16.0f : 9.0f;
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
            FVector(NeckX + BodyRadius * 3.2f, BodyRadius * 0.45f, HeadZ), ScaleBase);
        for (const float Side : {-1.0f, 1.0f})
        {
            AddTetraPoints(FVector(NeckX + BodyRadius * 2.0f, Side * BodyRadius * 0.48f, HeadZ + BodyRadius * 0.12f),
                FVector(NeckX + BodyRadius * 2.0f, Side * BodyRadius * 0.72f, HeadZ + BodyRadius * 0.08f),
                FVector(NeckX + BodyRadius * 2.45f, Side * BodyRadius * 0.53f, HeadZ + BodyRadius * 0.34f),
                FVector(NeckX + BodyRadius * 2.55f, Side * BodyRadius * 0.65f, HeadZ + BodyRadius * 0.13f), Eye);
        }
    }
    else
    {
        // Camp-pressure readability: a low forward hunch, long reaching arms, and a split crown give the scavenger a distinctive silhouette at a distance.
        const FLinearColor RootShadow(0.10f, 0.14f, 0.11f);
        const FLinearColor PeatBark(0.20f, 0.17f, 0.12f);
        const FLinearColor Lichen(0.34f, 0.49f, 0.25f);
        const FLinearColor WarmEye(0.72f, 0.42f, 0.17f);

        AddTetra(FVector(-7,0,0), FVector(54,44,104), RootShadow);
        AddTetra(FVector(27,0,5), FVector(42,37,78), PeatBark);
        AddTetra(FVector(48,0,70), FVector(28,28,46), RootShadow);
        AddTetra(FVector(61,0,98), FVector(43,35,52), Lichen);

        AddTetra(FVector(36,-21,132), FVector(16,12,48), PeatBark);
        AddTetra(FVector(36,21,132), FVector(16,12,48), PeatBark);
        AddTetra(FVector(35,0,133), FVector(15,14,42), Lichen);
        AddTetra(FVector(86,-13,122), FVector(6,5,7), WarmEye);
        AddTetra(FVector(86,13,122), FVector(6,5,7), WarmEye);

        AddTetraPoints(FVector(16,-33,62), FVector(43,-51,54), FVector(54,-35,43), FVector(88,-69,5), PeatBark);
        AddTetraPoints(FVector(16,33,62), FVector(54,35,43), FVector(43,51,54), FVector(88,69,5), PeatBark);
        AddTetra(FVector(98,-68,0), FVector(17,13,20), RootShadow);
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
    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
    {
        AKalmalaWildlifeSpawn* Deer = *It;
        if (!IsValid(Deer) || Deer->bDefeated || Deer->Archetype != EKalmalaWildlifeArchetype::Deer
            || Deer->Behaviour != EKalmalaWildlifeBehaviour::Idle
            || FVector::DistSquared2D(Deer->GetActorLocation(), GetActorLocation()) > FMath::Square(800.0f)) continue;
        Deer->BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f,
            Deer->SpawnOrigin + Deer->GetDeterministicOffset(GetEcologicalFleeDistance(Deer->CreatureNicheId)));
    }
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
}

void AKalmalaWildlifeSpawn::OnRep_Archetype()
{
    BuildArchetypePresentation();
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

void AKalmalaWildlifeSpawn::OnRep_Defeated()
{
    ApplyDefeatedState();
    if (bDefeated && !bClientCombatVerificationDefeatLogged && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"))))
    {
        bClientCombatVerificationDefeatLogged = true;
        UE_LOG(LogTemp, Display, TEXT("%s verification client observed relevant wildlife defeat."), FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")) ? TEXT("Boar") : (FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest")) ? TEXT("Deer") : (FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) ? TEXT("Mireling") : TEXT("Combat"))));
    }
}

void AKalmalaWildlifeSpawn::ApplyDefeatedState()
{
    SetActorHiddenInGame(bDefeated);
    if (bDefeated)
    {
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

    const int32 BoneFragmentQuantity = GetDirectBoneFragmentRewardQuantity(Archetype);
    if (BoneFragmentQuantity > 0)
    {
        Inventory->TryGrantFromServer(TEXT("BoneFragment"), BoneFragmentQuantity);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Rat)
    {
        // Rat's guaranteed Bone Fragment and Rat Tail use the persistent, all-or-none
        // reward queue recorded before this defeated state is replicated.
    }

    if (Archetype == EKalmalaWildlifeArchetype::Mireling && IsMirelingBossRewardCandidate())
    {
        if (AKalmalaGameMode* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>())
        {
            Mode->ClaimMirelingBossScroll(Attacker, PersistentSpawnId);
        }
    }
}
