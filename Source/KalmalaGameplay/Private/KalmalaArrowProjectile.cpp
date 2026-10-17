#include "KalmalaArrowProjectile.h"
#include "KalmalaCharacter.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaDamageProfile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

AKalmalaArrowProjectile::AKalmalaArrowProjectile()
{
    bReplicates = true; SetReplicateMovement(true); InitialLifeSpan = 5; PrimaryActorTick.bCanEverTick = true;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("ArrowCollision"));
    Collision->InitSphereRadius(2); RootComponent = Collision;
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    auto* Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ArrowShaft"));
    Mesh->SetupAttachment(Collision); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TArray<FVector> Vertices, Normals; TArray<int32> Indices; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangents;
    UKismetProceduralMeshLibrary::GenerateBoxMesh(FVector(28, 1, 1), Vertices, Indices, Normals, UVs, Tangents);
    TArray<FLinearColor> Colors; Colors.Init(FLinearColor(0.28f, 0.18f, 0.07f), Vertices.Num());
    Mesh->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UVs, Colors, Tangents, false);
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ArrowMovement"));
    Movement->UpdatedComponent = Collision; Movement->MaxSpeed = 4500; Movement->bRotationFollowsVelocity = true;
    Movement->ProjectileGravityScale = 1; Movement->bInitialVelocityInLocalSpace = false;
    Movement->bSweepCollision = false;
    PrimaryActorTick.AddPrerequisite(Movement, Movement->PrimaryComponentTick);
}
void AKalmalaArrowProjectile::BeginPlay()
{
    Super::BeginPlay();
    PreviousPosition = GetActorLocation();
    Collision->IgnoreActorWhenMoving(GetOwner(), true);
    if (!HasAuthority()) { Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); Movement->Deactivate(); SetActorTickEnabled(false); }
}
void AKalmalaArrowProjectile::InitializeShot(const FVector& Direction, float DrawProgress)
{
    if (!HasAuthority() || Direction.ContainsNaN() || !FMath::IsFinite(DrawProgress)) return;
    const float Progress = FMath::Clamp(DrawProgress, 0.0f, 1.0f);
    PreviousPosition = GetActorLocation();
    ShotDamage = FMath::Lerp(10.0f, 40.0f, Progress);
    Movement->Velocity = Direction.GetSafeNormal() * FMath::Lerp(1000.0f, 4500.0f, Progress);
    Collision->IgnoreActorWhenMoving(GetOwner(), true);
}
void AKalmalaArrowProjectile::Hit(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, FVector, const FHitResult&)
{
    if (!HasAuthority() || Other == GetOwner()) return;
    if (auto* Wildlife = Cast<AKalmalaWildlifeSpawn>(Other))
    {
        FKalmalaDamageChannels Damage; Damage.Pierce = ShotDamage;
        Wildlife->ApplyArrowDamageFromServer(Damage, Cast<AKalmalaCharacter>(GetOwner()));
    }
    Destroy();
}

void AKalmalaArrowProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority()) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ArrowFlight), false, this);
    Query.AddIgnoredActor(GetOwner());
    FHitResult Result;
    const FVector Current = GetActorLocation();
    if (GetWorld()->SweepSingleByChannel(Result, PreviousPosition, Current, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(2), Query))
        Hit(Collision, Result.GetActor(), Result.GetComponent(), FVector::ZeroVector, Result);
    PreviousPosition = Current;
}
