#include "KalmalaOceanTravelTestFixture.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AKalmalaOceanTravelTestFixture::AKalmalaOceanTravelTestFixture()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetNetUpdateFrequency(10.0f);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    WaterSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterSurface"));
    WaterSurface->SetupAttachment(SceneRoot);
    WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterSurface->SetGenerateOverlapEvents(false);
    WaterSurface->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedWater.M_GeneratedWater"));
    if (WaterMaterial.Succeeded())
    {
        WaterSurface->SetMaterial(0, WaterMaterial.Object);
    }
}

void AKalmalaOceanTravelTestFixture::Initialize(const FVector2D InEntryPoint, const FVector2D InTargetPoint,
    const float InHalfWidth, const float InWaterDepth)
{
    check(HasAuthority());
    EntryPoint = InEntryPoint;
    TargetPoint = InTargetPoint;
    HalfWidth = FMath::Clamp(InHalfWidth, 100.0f, 5000.0f);
    WaterDepth = FMath::Clamp(InWaterDepth, 150.0f, 1000.0f);
    bIsConfigured = true;
    SetActorLocation(FVector(EntryPoint, 0.0f));
    BuildWaterSurface();
    ForceNetUpdate();
}

bool AKalmalaOceanTravelTestFixture::TryGetWaterDepth(const FVector2D Position, float& OutDepth) const
{
    OutDepth = 0.0f;
    if (!bIsConfigured || !FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y))
    {
        return false;
    }

    const FVector2D Segment = TargetPoint - EntryPoint;
    const double SegmentLengthSquared = Segment.SizeSquared();
    const double Alpha = SegmentLengthSquared > UE_DOUBLE_SMALL_NUMBER
        ? FMath::Clamp(FVector2D::DotProduct(Position - EntryPoint, Segment) / SegmentLengthSquared, 0.0, 1.0)
        : 0.0;
    const FVector2D ClosestPoint = EntryPoint + Segment * Alpha;
    if (FVector2D::DistSquared(Position, ClosestPoint) > FMath::Square(HalfWidth))
    {
        return false;
    }

    OutDepth = WaterDepth;
    return true;
}

void AKalmalaOceanTravelTestFixture::OnRep_FixtureData()
{
    if (bIsConfigured)
    {
        SetActorLocation(FVector(EntryPoint, 0.0f));
        BuildWaterSurface();
    }
}

void AKalmalaOceanTravelTestFixture::BuildWaterSurface()
{
    if (bWaterSurfaceBuilt || !bIsConfigured || WaterSurface == nullptr)
    {
        return;
    }

    const FVector2D Direction = (TargetPoint - EntryPoint).GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return;
    }

    const FVector2D Side(-Direction.Y, Direction.X);
    const FVector2D LocalEnd = TargetPoint - EntryPoint;
    const TArray<FVector> Vertices = {
        FVector(-Side * HalfWidth, 0.0f), FVector(Side * HalfWidth, 0.0f),
        FVector(LocalEnd + Side * HalfWidth, 0.0f), FVector(LocalEnd - Side * HalfWidth, 0.0f)
    };
    const TArray<int32> Triangles = { 0, 1, 2, 0, 2, 3 };
    const TArray<FVector> Normals = { FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector };
    const TArray<FVector2D> UVs = { FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 1.0f), FVector2D(0.0f, 1.0f) };
    const TArray<FLinearColor> Colors = { FLinearColor::White, FLinearColor::White, FLinearColor::White, FLinearColor::White };
    const TArray<FProcMeshTangent> Tangents = {
        FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f),
        FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f)
    };
    WaterSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
    bWaterSurfaceBuilt = true;
}

void AKalmalaOceanTravelTestFixture::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaOceanTravelTestFixture, EntryPoint);
    DOREPLIFETIME(AKalmalaOceanTravelTestFixture, TargetPoint);
    DOREPLIFETIME(AKalmalaOceanTravelTestFixture, HalfWidth);
    DOREPLIFETIME(AKalmalaOceanTravelTestFixture, WaterDepth);
    DOREPLIFETIME(AKalmalaOceanTravelTestFixture, bIsConfigured);
}
