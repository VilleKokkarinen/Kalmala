#include "KalmalaWildlifeSpawn.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AKalmalaWildlifeSpawn::AKalmalaWildlifeSpawn()
{
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
    BuildArchetypePresentation();
    bReplicates = true;
    SetReplicateMovement(true);
}

void AKalmalaWildlifeSpawn::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority())
    {
        SetActorLocation(Spawn.Location);
        SpawnOrigin = Spawn.Location;
        BehaviourDestination = SpawnOrigin;
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        Archetype = GetArchetypeForSpawnSeed(Spawn.SpawnSeed);
        BuildArchetypePresentation();
    }
}

bool AKalmalaWildlifeSpawn::IsDefeatAllowed(const bool bServerAuthority, const bool bAlreadyDefeated)
{
    return bServerAuthority && !bAlreadyDefeated;
}

bool AKalmalaWildlifeSpawn::DefeatServer()
{
    if (!IsDefeatAllowed(HasAuthority(), bDefeated))
    {
        return false;
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
    if (!HasAuthority() || bDefeated || !FMath::IsFinite(Damage) || Damage <= 0.0f || Damage > 100.0f) return false;
    if (Attacker != nullptr && (!Attacker->HasAuthority() || Attacker->GetWorld() != GetWorld()
        || FVector::DistSquared(Attacker->GetActorLocation(), GetActorLocation()) > FMath::Square(220.0f))) return false;
    if (Attacker != nullptr) LastValidatedAttacker = Attacker;
    Health = FMath::Clamp(Health - Damage, 0.0f, 100.0f);
    if (Health <= 0.0f) return DefeatServer();
    if (Archetype == EKalmalaWildlifeArchetype::Boar && Attacker != nullptr)
    {
        BoarChargeTarget = Attacker;
        BoarChargeSecondsRemaining = 1.25f;
    }
    else
    {
        BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f, SpawnOrigin + GetDeterministicOffset(300.0f));
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
    BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f,
        GetActorLocation() + Direction * 260.0f);
    ForceNetUpdate();
    return Behaviour == EKalmalaWildlifeBehaviour::Flee;
}

bool AKalmalaWildlifeSpawn::IsMirelingBossRewardCandidate() const
{
    return Archetype == EKalmalaWildlifeArchetype::Mireling
        && !PersistentSpawnId.IsEmpty()
        && FCrc::StrCrc32(*PersistentSpawnId) % 5u == 0u;
}

EKalmalaWildlifeArchetype AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(const uint64 SpawnSeed)
{
    // A stable server descriptor seed selects the archetype; it never depends on actor order or a client observation.
    if (SpawnSeed % 3ull == 0ull) return EKalmalaWildlifeArchetype::Boar;
    if (SpawnSeed % 5ull == 1ull) return EKalmalaWildlifeArchetype::Deer;
    return EKalmalaWildlifeArchetype::Mireling;
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
        const bool bApplied = Nearest->ApplyWildlifeDamageFromServer(this, 10.0f);
        UE_LOG(LogTemp, Display, TEXT("Mireling encounter attempt: Applied=%d Authority=%d SourceAuthority=%d Distance=%.1f Health=%.1f"), bApplied, Nearest->HasAuthority(), HasAuthority(), FMath::Sqrt(BestDistanceSquared), Nearest->GetHealth());
        if (bApplied) MirelingMeleeCooldown = 1.0f;
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
        if (Distance <= 180.0f && BoarMeleeCooldown <= 0.0f && Target->ApplyWildlifeDamageFromServer(this, 15.0f)) BoarMeleeCooldown = 1.25f;
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

void AKalmalaWildlifeSpawn::BuildArchetypePresentation()
{
    MirelingMesh->ClearAllMeshSections();
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
            for (const int32 Vertex : OutwardFace) { Triangles.Add(Vertices.Num()); Vertices.Add(Points[Vertex]); Normals.Add(Normal); UV.Add(FVector2D::ZeroVector); Colors.Add(Colour); }
        }
    };
    const auto AddTetra = [&Vertices, &Triangles, &Normals, &UV, &Colors](const FVector Centre, const FVector Extent, const FLinearColor Colour)
    {
        const FVector Points[] = { Centre + FVector(-Extent.X,-Extent.Y,0), Centre + FVector(Extent.X,-Extent.Y,0), Centre + FVector(0,Extent.Y,0), Centre + FVector(0,0,Extent.Z) };
        const int32 Faces[] = { 0,2,1, 0,1,3, 1,2,3, 2,0,3 };
        for (int32 Face = 0; Face < UE_ARRAY_COUNT(Faces); Face += 3)
        {
            const FVector Normal = FVector::CrossProduct(Points[Faces[Face + 1]] - Points[Faces[Face]], Points[Faces[Face + 2]] - Points[Faces[Face]]).GetSafeNormal();
            for (int32 Vertex = 0; Vertex < 3; ++Vertex) { Triangles.Add(Vertices.Num()); Vertices.Add(Points[Faces[Face + Vertex]]); Normals.Add(Normal); UV.Add(FVector2D::ZeroVector); Colors.Add(Colour); }
        }
    };
    if (Archetype == EKalmalaWildlifeArchetype::Boar)
    {
        AddTetra(FVector(0,0,0), FVector(95,52,58), FLinearColor(0.20f,0.11f,0.06f));
        AddTetra(FVector(85,0,12), FVector(48,38,42), FLinearColor(0.27f,0.15f,0.08f));
        AddTetra(FVector(-45,-32,-52), FVector(18,18,58), FLinearColor(0.10f,0.06f,0.04f));
        AddTetra(FVector(-45,32,-52), FVector(18,18,58), FLinearColor(0.10f,0.06f,0.04f));
        AddTetra(FVector(45,-32,-52), FVector(18,18,58), FLinearColor(0.10f,0.06f,0.04f));
        AddTetra(FVector(45,32,-52), FVector(18,18,58), FLinearColor(0.10f,0.06f,0.04f));
        AddTetra(FVector(128,-22,1), FVector(34,7,8), FLinearColor(0.82f,0.75f,0.54f));
        AddTetra(FVector(128,22,1), FVector(34,7,8), FLinearColor(0.82f,0.75f,0.54f));
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Deer)
    {
        AddTetra(FVector(0,0,4), FVector(92,38,52), FLinearColor(0.42f,0.25f,0.12f));
        AddTetra(FVector(88,0,42), FVector(35,26,62), FLinearColor(0.50f,0.31f,0.16f));
        AddTetra(FVector(-48,-24,-58), FVector(11,11,72), FLinearColor(0.25f,0.15f,0.08f));
        AddTetra(FVector(-48,24,-58), FVector(11,11,72), FLinearColor(0.25f,0.15f,0.08f));
        AddTetra(FVector(42,-24,-58), FVector(11,11,72), FLinearColor(0.25f,0.15f,0.08f));
        AddTetra(FVector(42,24,-58), FVector(11,11,72), FLinearColor(0.25f,0.15f,0.08f));
        AddTetra(FVector(112,-18,88), FVector(24,4,42), FLinearColor(0.75f,0.65f,0.42f));
        AddTetra(FVector(112,18,88), FVector(24,4,42), FLinearColor(0.75f,0.65f,0.42f));
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
        Deer->BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f, Deer->SpawnOrigin + Deer->GetDeterministicOffset(300.0f));
    }
}

void AKalmalaWildlifeSpawn::BeginServerBehaviour(const EKalmalaWildlifeBehaviour NextBehaviour, const float DurationSeconds, const FVector& Destination)
{
    if (!IsBehaviourTransitionAllowed(HasAuthority(), bDefeated, Behaviour, NextBehaviour) || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f || !FMath::IsFinite(Destination.X) || !FMath::IsFinite(Destination.Y) || !FMath::IsFinite(Destination.Z))
    {
        return;
    }

    Behaviour = NextBehaviour;
    BehaviourSecondsRemaining = FMath::Min(DurationSeconds, 2.0f);
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
    if (BehaviourSecondsRemaining > 0.0f && !ToDestination.IsNearlyZero(5.0f))
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
}

void AKalmalaWildlifeSpawn::OnRep_Archetype()
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
    AKalmalaCharacter* Attacker = LastValidatedAttacker.Get();
    UKalmalaInventoryComponent* Inventory = Attacker ? Attacker->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Inventory == nullptr || !Inventory->GetOwner() || !Inventory->GetOwner()->HasAuthority()) return;
    if (Archetype == EKalmalaWildlifeArchetype::Mireling) Inventory->TryGrantFromServer(TEXT("MirelingAsh"), 1);
    else if (Archetype == EKalmalaWildlifeArchetype::Boar)
    {
        Inventory->TryGrantFromServer(TEXT("BoarMeat"), 1);
        Inventory->TryGrantFromServer(TEXT("BoarHide"), 1);
    }
    else if (Archetype == EKalmalaWildlifeArchetype::Deer)
    {
        Inventory->TryGrantFromServer(TEXT("DeerMeat"), 1);
        Inventory->TryGrantFromServer(TEXT("DeerHide"), 1);
    }

    if (Archetype == EKalmalaWildlifeArchetype::Mireling && IsMirelingBossRewardCandidate())
    {
        if (AKalmalaGameMode* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>())
        {
            Mode->ClaimMirelingBossScroll(Attacker, PersistentSpawnId);
        }
    }
}
