#include "KalmalaWildlifeSpawn.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
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
    BuildMirelingPresentation();
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
    ApplyDefeatedState();
    OnDefeated.Broadcast(PersistentSpawnId);
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
    BeginServerBehaviour(EKalmalaWildlifeBehaviour::Flee, 1.50f, SpawnOrigin + GetDeterministicOffset(300.0f));
    ForceNetUpdate();
    return true;
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
        UpdateMirelingScavenge(DeltaSeconds);
    }
}

void AKalmalaWildlifeSpawn::UpdateMirelingScavenge(const float DeltaSeconds)
{
    MirelingMeleeCooldown = FMath::Max(0.0f, MirelingMeleeCooldown - FMath::Clamp(DeltaSeconds, 0.0f, 0.10f));
    AKalmalaCharacter* Nearest = nullptr; float BestDistanceSquared = FMath::Square(500.0f);
    for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
    {
        AKalmalaCharacter* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->HasAuthority() || Candidate->GetHealth() <= 1.0f) continue;
        const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
        if (DistanceSquared < BestDistanceSquared) { Nearest = Candidate; BestDistanceSquared = DistanceSquared; }
    }
    if (Nearest == nullptr) return;
    if (BestDistanceSquared > FMath::Square(145.0f) && Behaviour == EKalmalaWildlifeBehaviour::Idle)
    {
        const FVector Direction = (Nearest->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        SetActorLocation(GetActorLocation() + Direction * FMath::Min(170.0f * FMath::Clamp(DeltaSeconds, 0.0f, 0.10f), FMath::Sqrt(BestDistanceSquared) - 145.0f), true);
    }
    else if (MirelingMeleeCooldown <= 0.0f && BestDistanceSquared <= FMath::Square(180.0f) && Nearest->ApplyMirelingDamageFromServer(this, 10.0f)) MirelingMeleeCooldown = 1.0f;
}

void AKalmalaWildlifeSpawn::BuildMirelingPresentation()
{
    // An original low-poly silhouette: lichen body, two root-like legs, and a crown.
    TArray<FVector> Vertices; TArray<int32> Triangles; TArray<FVector> Normals; TArray<FVector2D> UV; TArray<FLinearColor> Colors;
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
    AddTetra(FVector(0,0,15), FVector(52,38,115), FLinearColor(0.12f,0.20f,0.15f));
    AddTetra(FVector(-26,0,-45), FVector(14,14,75), FLinearColor(0.18f,0.16f,0.12f));
    AddTetra(FVector(26,0,-45), FVector(14,14,75), FLinearColor(0.18f,0.16f,0.12f));
    AddTetra(FVector(0,0,122), FVector(48,30,55), FLinearColor(0.38f,0.55f,0.28f));
    MirelingMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, {}, false);
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
}

void AKalmalaWildlifeSpawn::OnRep_Defeated()
{
    ApplyDefeatedState();
    if (bDefeated && !bClientCombatVerificationDefeatLogged && (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest")) || FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest"))))
    {
        bClientCombatVerificationDefeatLogged = true;
        UE_LOG(LogTemp, Display, TEXT("%s verification client observed relevant wildlife defeat."), FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest")) ? TEXT("Mireling") : TEXT("Combat"));
    }
}

void AKalmalaWildlifeSpawn::ApplyDefeatedState()
{
    SetActorHiddenInGame(bDefeated);
    if (bDefeated)
    {
        if (AKalmalaCharacter* Attacker = LastValidatedAttacker.Get())
        {
            if (UKalmalaInventoryComponent* Inventory = Attacker->FindComponentByClass<UKalmalaInventoryComponent>()) Inventory->TryGrantFromServer(TEXT("MirelingAsh"), 1);
        }
    }
}
