#include "KalmalaOceanSkiff.h"

#include "KalmalaCharacter.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace KalmalaOceanSkiff
{
    constexpr float MaximumInteractionDistance = 250.0f;
    constexpr float MinimumWaterDepth = 100.0f;
    constexpr float MaximumExitSpeed = 50.0f;
    constexpr float ExitOffset = 190.0f;
    constexpr float SeaSurfaceZ = 0.0f;
}

AKalmalaOceanSkiff::AKalmalaOceanSkiff()
{
    bReplicates = true;
    SetReplicateMovement(false);

    HullCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HullCollision"));
    HullCollision->SetBoxExtent(FVector(140.0f, 70.0f, 40.0f));
    HullCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HullCollision->SetCollisionObjectType(ECC_WorldDynamic);
    HullCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    HullCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HullCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    RootComponent = HullCollision;

    HullMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HullMesh"));
    HullMesh->SetupAttachment(RootComponent);
    HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BuildOriginalHull(HullMesh);
}

void AKalmalaOceanSkiff::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaOceanSkiff, HelmOccupant);
    DOREPLIFETIME(AKalmalaOceanSkiff, PassengerOccupant);
    DOREPLIFETIME(AKalmalaOceanSkiff, Mode);
}

bool AKalmalaOceanSkiff::IsLaunchAllowed(const bool bServerAuthority, const bool bGeneratedTerrainHit,
    const bool bInRange, const bool bDeepOcean, const bool bWorldBounded, const bool bSessionSlotAvailable)
{
    return bServerAuthority && bGeneratedTerrainHit && bInRange && bDeepOcean && bWorldBounded && bSessionSlotAvailable;
}

EKalmalaOceanSkiffSeat AKalmalaOceanSkiff::ChooseSeat(const bool bHelmOccupied, const bool bPassengerOccupied)
{
    if (!bHelmOccupied) return EKalmalaOceanSkiffSeat::Helm;
    if (!bPassengerOccupied) return EKalmalaOceanSkiffSeat::Passenger;
    return EKalmalaOceanSkiffSeat::None;
}

bool AKalmalaOceanSkiff::IsDisembarkAllowed(const bool bServerAuthority, const bool bIsOccupant,
    const float Speed, const bool bHasSafePlacement)
{
    return bServerAuthority && bIsOccupant && FMath::IsFinite(Speed)
        && Speed >= 0.0f && Speed <= KalmalaOceanSkiff::MaximumExitSpeed && bHasSafePlacement;
}

AKalmalaOceanSkiff* AKalmalaOceanSkiff::TryLaunchFromServer(AKalmalaCharacter* Interactor, const FHitResult& TerrainHit)
{
    if (!IsValid(Interactor) || !Interactor->HasAuthority() || Interactor->GetWorld() == nullptr
        || Interactor->GetPlayerState() == nullptr)
    {
        return nullptr;
    }

    UWorld* World = Interactor->GetWorld();
    const AKalmalaWorldGenerationGameState* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
    const AKalmalaGeneratedTerrainPatch* TerrainPatch = Cast<AKalmalaGeneratedTerrainPatch>(TerrainHit.GetActor());
    const bool bGeneratedTerrainHit = TerrainPatch != nullptr && TerrainPatch->HasGenerationData()
        && TerrainHit.bBlockingHit && TerrainHit.Component.IsValid()
        && TerrainHit.Component->GetCollisionObjectType() == ECC_WorldStatic;
    const FVector2D LaunchPosition(TerrainHit.Location);
    const FKalmalaOceanSample Ocean = State != nullptr
        ? FKalmalaOceanSampler::Sample(State->GetWorldGenerationConfig(), LaunchPosition)
        : FKalmalaOceanSample();

    bool bSessionSlotAvailable = true;
    for (TActorIterator<AKalmalaOceanSkiff> Iterator(World); Iterator; ++Iterator)
    {
        bSessionSlotAvailable = false;
        break;
    }

    const bool bInRange = TerrainHit.Distance >= 0.0f && FMath::IsFinite(TerrainHit.Distance)
        && TerrainHit.Distance <= KalmalaOceanSkiff::MaximumInteractionDistance
        && FVector::DistSquared(Interactor->GetActorLocation(), TerrainHit.Location)
            <= FMath::Square(KalmalaOceanSkiff::MaximumInteractionDistance);
    const bool bDeepOcean = Ocean.bIsValid && Ocean.WaterDepth >= KalmalaOceanSkiff::MinimumWaterDepth;
    const bool bWorldBounded = State != nullptr
        && FKalmalaWorldBounds::Contains(State->GetWorldGenerationConfig(), LaunchPosition, 160.0);

    if (!IsLaunchAllowed(Interactor->HasAuthority(), bGeneratedTerrainHit, bInRange,
        bDeepOcean, bWorldBounded, bSessionSlotAvailable))
    {
        return nullptr;
    }

    const FRotator SpawnRotation(0.0f, Interactor->GetActorRotation().Yaw, 0.0f);
    const FVector SpawnLocation(LaunchPosition.X, LaunchPosition.Y, KalmalaOceanSkiff::SeaSurfaceZ);
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = nullptr;
    SpawnParameters.Instigator = nullptr;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    return World->SpawnActor<AKalmalaOceanSkiff>(SpawnLocation, SpawnRotation, SpawnParameters);
}

bool AKalmalaOceanSkiff::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    if (!HasAuthority() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || Interactor->GetWorld() != GetWorld() || Interactor->GetPlayerState() == nullptr
        || !FMath::IsFinite(FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()))
        || FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation())
            > FMath::Square(KalmalaOceanSkiff::MaximumInteractionDistance))
    {
        return false;
    }

    if (Interactor == HelmOccupant || Interactor == PassengerOccupant)
    {
        return true;
    }

    const APlayerState* InteractorState = Interactor->GetPlayerState();
    if ((IsValid(HelmOccupant) && HelmOccupant->GetPlayerState() == InteractorState)
        || (IsValid(PassengerOccupant) && PassengerOccupant->GetPlayerState() == InteractorState))
    {
        return false;
    }

    if (Interactor->GetAttachParentActor() != nullptr)
    {
        return false;
    }

    return ChooseSeat(IsValid(HelmOccupant), IsValid(PassengerOccupant)) != EKalmalaOceanSkiffSeat::None;
}

void AKalmalaOceanSkiff::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (!CanInteract_Implementation(Interactor)) return;

    if (Interactor == HelmOccupant || Interactor == PassengerOccupant)
    {
        TryDisembarkFromServer(Interactor);
        return;
    }

    const EKalmalaOceanSkiffSeat Seat = ChooseSeat(IsValid(HelmOccupant), IsValid(PassengerOccupant));
    if (Seat != EKalmalaOceanSkiffSeat::None)
    {
        SetSeatOccupant(Interactor, Seat);
    }
}

bool AKalmalaOceanSkiff::TryDisembarkFromServer(AKalmalaCharacter* Interactor)
{
    if (!HasAuthority() || !IsValid(Interactor) || Interactor->GetWorld() != GetWorld()
        || (Interactor != HelmOccupant && Interactor != PassengerOccupant))
    {
        return false;
    }

    FVector ExitLocation;
    const float Speed = GetVelocity().Size();
    if (!FindSafeExitLocation(Interactor, ExitLocation)
        || !IsDisembarkAllowed(HasAuthority(), Interactor == HelmOccupant || Interactor == PassengerOccupant,
            Speed, true))
    {
        return false;
    }

    Interactor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Interactor->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
    ClearSeatOccupant(Interactor);
    Interactor->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Interactor->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    Interactor->ForceNetUpdate();
    ForceNetUpdate();
    return true;
}

bool AKalmalaOceanSkiff::FindSafeExitLocation(AKalmalaCharacter* Interactor, FVector& OutLocation) const
{
    if (!IsValid(Interactor) || GetWorld() == nullptr || Interactor->GetCapsuleComponent() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid()) return false;

    const FKalmalaWorldGenerationConfig& Config = State->GetWorldGenerationConfig();
    const float Radius = Interactor->GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float HalfHeight = Interactor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaSkiffExit), false, Interactor);
    QueryParams.AddIgnoredActor(this);

    for (int32 DirectionIndex = 0; DirectionIndex < 8; ++DirectionIndex)
    {
        const float Angle = FMath::DegreesToRadians(DirectionIndex * 45.0f);
        const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
        const FVector2D CandidateXY = FVector2D(GetActorLocation()) + Direction * KalmalaOceanSkiff::ExitOffset;
        if (!FKalmalaWorldBounds::Contains(Config, CandidateXY, Radius + 2.0f)) continue;

        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, CandidateXY);
        if (!Ocean.bIsValid || (Ocean.WaterDepth > 0.0f && Ocean.WaterDepth < KalmalaOceanSkiff::MinimumWaterDepth)
            || FKalmalaShimmeringLakeSampler::IsWater(Config, CandidateXY)) continue;
        const float CandidateZ = Ocean.WaterDepth >= KalmalaOceanSkiff::MinimumWaterDepth
            ? KalmalaOceanSkiff::SeaSurfaceZ + HalfHeight + 8.0f
            : Ocean.TerrainHeight + HalfHeight + 4.0f;
        const FVector Candidate(CandidateXY, CandidateZ);
        const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        if (!GetWorld()->OverlapBlockingTestByChannel(Candidate, FQuat(Interactor->GetActorRotation()),
            ECC_Pawn, Capsule, QueryParams))
        {
            OutLocation = Candidate;
            return true;
        }
    }
    return false;
}

void AKalmalaOceanSkiff::SetSeatOccupant(AKalmalaCharacter* Interactor, const EKalmalaOceanSkiffSeat Seat)
{
    if (!HasAuthority() || !IsValid(Interactor) || Seat == EKalmalaOceanSkiffSeat::None) return;
    if (Seat == EKalmalaOceanSkiffSeat::Helm) HelmOccupant = Interactor;
    else PassengerOccupant = Interactor;
    RefreshSeatOccupancyPresentation();
    ForceNetUpdate();
}

void AKalmalaOceanSkiff::ClearSeatOccupant(AKalmalaCharacter* Interactor)
{
    if (!HasAuthority() || !IsValid(Interactor)) return;
    if (HelmOccupant == Interactor) HelmOccupant = nullptr;
    if (PassengerOccupant == Interactor) PassengerOccupant = nullptr;
    RefreshSeatOccupancyPresentation();
}

void AKalmalaOceanSkiff::OnRep_SeatOccupants()
{
    RefreshSeatOccupancyPresentation();
}

void AKalmalaOceanSkiff::RefreshSeatOccupancyPresentation()
{
    auto UpdateSeat = [this](TWeakObjectPtr<AKalmalaCharacter>& Previous,
        AKalmalaCharacter* Current, const FVector& RelativeSeatLocation)
    {
        if (Previous.IsValid() && Previous.Get() != Current)
        {
            AKalmalaCharacter* Former = Previous.Get();
            if (Former->GetAttachParentActor() == this)
            {
                Former->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            }
            if (UCapsuleComponent* Capsule = Former->GetCapsuleComponent())
            {
                Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            }
            if (UCharacterMovementComponent* Movement = Former->GetCharacterMovement())
            {
                Movement->SetMovementMode(MOVE_Walking);
            }
        }

        if (IsValid(Current))
        {
            if (Current->GetAttachParentActor() != this)
            {
                Current->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
            }
            Current->GetCharacterMovement()->StopMovementImmediately();
            Current->GetCharacterMovement()->DisableMovement();
            Current->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Current->SetActorRelativeLocation(RelativeSeatLocation, false, nullptr, ETeleportType::TeleportPhysics);
        }
        Previous = Current;
    };

    UpdateSeat(PresentedHelmOccupant, HelmOccupant, FVector(55.0f, 0.0f, 120.0f));
    UpdateSeat(PresentedPassengerOccupant, PassengerOccupant, FVector(-55.0f, 0.0f, 120.0f));
}

bool AKalmalaOceanSkiff::BuildOriginalHull(UProceduralMeshComponent* Mesh)
{
    if (Mesh == nullptr) return false;
    TArray<FVector> Vertices = {
        FVector(-140, -55, -24), FVector(140, -55, -24), FVector(140, 55, -24), FVector(-140, 55, -24),
        FVector(-120, -45, 18), FVector(120, -45, 18), FVector(120, 45, 18), FVector(-120, 45, 18),
        FVector(10, -45, 30), FVector(100, -45, 30), FVector(100, 45, 30), FVector(10, 45, 30),
        FVector(-110, -45, 30), FVector(-20, -45, 30), FVector(-20, 45, 30), FVector(-110, 45, 30)
    };
    TArray<int32> Triangles = {
        0,1,5, 0,5,4, 1,2,6, 1,6,5, 2,3,7, 2,7,6, 3,0,4, 3,4,7,
        4,5,6, 4,6,7, 0,3,2, 0,2,1,
        8,9,10, 8,10,11, 12,14,13, 12,15,14
    };
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    Colors.Init(FLinearColor(0.42f, 0.27f, 0.12f, 1.0f), Vertices.Num());
    Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, TArray<FVector>(), UVs, Colors,
        TArray<FProcMeshTangent>(), false);
    if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.42f, 0.27f, 0.12f, 1.0f));
    }
    return true;
}
