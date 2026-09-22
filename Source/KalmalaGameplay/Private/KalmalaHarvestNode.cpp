#include "KalmalaHarvestNode.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void AddTaperedBox(
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles,
        TArray<FLinearColor>& Colors,
        const FVector Centre,
        const FVector HalfSize,
        const float Height,
        const float TopScale,
        const float YawDegrees,
        const FLinearColor Colour)
    {
        const FRotationMatrix Rotation(FRotator(0.0f, YawDegrees, 0.0f));
        const int32 BaseIndex = Vertices.Num();
        const FVector BottomCorners[] =
        {
            FVector(-HalfSize.X, -HalfSize.Y, 0.0f),
            FVector(HalfSize.X, -HalfSize.Y, 0.0f),
            FVector(HalfSize.X, HalfSize.Y, 0.0f),
            FVector(-HalfSize.X, HalfSize.Y, 0.0f)
        };
        for (const FVector& Corner : BottomCorners)
        {
            Vertices.Add(Centre + Rotation.TransformVector(Corner));
        }
        for (const FVector& Corner : BottomCorners)
        {
            Vertices.Add(Centre + Rotation.TransformVector(FVector(Corner.X * TopScale, Corner.Y * TopScale, Height)));
        }
        for (int32 VertexIndex = 0; VertexIndex < 8; ++VertexIndex)
        {
            Colors.Add(Colour);
        }

        const int32 FaceIndices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            0, 4, 5, 0, 5, 1,
            1, 5, 6, 1, 6, 2,
            2, 6, 7, 2, 7, 3,
            3, 7, 4, 3, 4, 0
        };
        for (const int32 Index : FaceIndices)
        {
            Triangles.Add(BaseIndex + Index);
        }
    }

    FLinearColor GetSourceAccent(const FName PresentationId)
    {
        if (PresentationId == TEXT("birch-bark-bundle")) return FLinearColor(0.52f, 0.34f, 0.18f);
        if (PresentationId == TEXT("reed-cluster")) return FLinearColor(0.35f, 0.64f, 0.24f);
        if (PresentationId == TEXT("resinwood-bundle")) return FLinearColor(0.30f, 0.20f, 0.10f);
        if (PresentationId == TEXT("bog-iron-vein")) return FLinearColor(0.30f, 0.24f, 0.20f);
        if (PresentationId == TEXT("frostmoss-clump")) return FLinearColor(0.54f, 0.76f, 0.72f);
        if (PresentationId == TEXT("slate-vein")) return FLinearColor(0.30f, 0.36f, 0.43f);
        return FLinearColor::Transparent;
    }

    bool IsRockSource(const FName PresentationId)
    {
        return PresentationId == TEXT("bog-iron-vein") || PresentationId == TEXT("slate-vein");
    }
}

AKalmalaHarvestNode::AKalmalaHarvestNode()
{
    bReplicates = true;
    SetReplicateMovement(false);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(50.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
    SourceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GatheringSource"));
    SourceMesh->SetupAttachment(RootComponent);
    SourceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SourceMesh->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Bark(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark.M_GeneratedBark"));
    if (Bark.Succeeded()) BarkMaterial = Bark.Object;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Rock(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    if (Rock.Succeeded()) RockMaterial = Rock.Object;
}

void AKalmalaHarvestNode::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority() && PersistentSpawnId.IsEmpty() && !bHarvested
        && Spawn.Kind == EKalmalaWorldPopulationKind::HarvestNode && !Spawn.Location.ContainsNaN()
        && (Spawn.ContentId.IsNone() || FKalmalaBiomeContentContract::IsValidGatheringSourceId(Spawn.ContentId)))
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        GatheringSourceId = Spawn.ContentId;
        BuildSourcePresentation();
    }
}

void AKalmalaHarvestNode::InitializeDiscoveryServer(const FString& InPersistentSpawnId, const FVector& InLocation)
{
    if (HasAuthority() && PersistentSpawnId.IsEmpty() && !bHarvested
        && !InPersistentSpawnId.IsEmpty() && !InLocation.ContainsNaN())
    {
        SetActorLocation(InLocation);
        PersistentSpawnId = InPersistentSpawnId;
        GatheringSourceId = NAME_None;
        BuildSourcePresentation();
    }
}

bool AKalmalaHarvestNode::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return IsValid(Interactor) && Interactor->HasAuthority() && Interactor->GetWorld() == GetWorld()
        && !PersistentSpawnId.IsEmpty()
        && IsHarvestAllowed(HasAuthority(), bHarvested, Interactor->GetActorLocation(), GetActorLocation());
}

bool AKalmalaHarvestNode::IsHarvestAllowed(const bool bServerAuthority, const bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, const float MaximumDistance)
{
    return bServerAuthority && !bAlreadyHarvested && FMath::IsFinite(MaximumDistance) && MaximumDistance > 0.0f
        && !InteractorLocation.ContainsNaN() && !NodeLocation.ContainsNaN()
        && FVector::DistSquared(InteractorLocation, NodeLocation) <= FMath::Square(MaximumDistance);
}

FName AKalmalaHarvestNode::GetGatheringPresentationId() const
{
    return FKalmalaBiomeContentContract::GetGatheringPresentationId(GatheringSourceId);
}

void AKalmalaHarvestNode::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    // No client-selected reward or quantity. Commit depletion only after the complete grant succeeds.
    if (!CanInteract_Implementation(Interactor)) return;
    auto* Inventory = Interactor->FindComponentByClass<UKalmalaInventoryComponent>();
    if (IsValid(Inventory) && Inventory->TryGrantFromServer(GetHarvestItemId(), 1))
    {
        bHarvested = true;
        ApplyHarvestedState();
        OnHarvested.Broadcast(PersistentSpawnId);
        ForceNetUpdate();
    }
}

FName AKalmalaHarvestNode::GetHarvestItemId() const
{
    if (PersistentSpawnId.IsEmpty()) return NAME_None;
    static const FName Materials[] = { TEXT("Wood"), TEXT("Stone"), TEXT("Fibre") };
    return Materials[FCrc::StrCrc32(*PersistentSpawnId) % UE_ARRAY_COUNT(Materials)];
}

void AKalmalaHarvestNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaHarvestNode, bHarvested);
    DOREPLIFETIME(AKalmalaHarvestNode, PersistentSpawnId);
    DOREPLIFETIME(AKalmalaHarvestNode, GatheringSourceId);
}

void AKalmalaHarvestNode::OnRep_Harvested()
{
    ApplyHarvestedState();
}

void AKalmalaHarvestNode::OnRep_GatheringSourceId()
{
    BuildSourcePresentation();
}

void AKalmalaHarvestNode::BuildSourcePresentation()
{
    if (!SourceMesh) return;
    SourceMesh->ClearAllMeshSections();

    const FName PresentationId = GetGatheringPresentationId();
    if (PresentationId.IsNone())
    {
        SourceMesh->SetVisibility(false);
        return;
    }

    const FLinearColor Accent = GetSourceAccent(PresentationId);
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FLinearColor> Colors;
    if (PresentationId == TEXT("reed-cluster") || PresentationId == TEXT("frostmoss-clump"))
    {
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-18.0f, 0.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 58.0f, 0.55f, -8.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(0.0f, 7.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), PresentationId == TEXT("frostmoss-clump") ? 42.0f : 78.0f, 0.65f, 4.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -2.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 66.0f, 0.5f, 11.0f, Accent);
    }
    else if (IsRockSource(PresentationId))
    {
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-14.0f, 0.0f, 0.0f), FVector(24.0f, 14.0f, 0.0f), 28.0f, 0.72f, -9.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(14.0f, 3.0f, 0.0f), FVector(18.0f, 12.0f, 0.0f), 38.0f, 0.68f, 12.0f, Accent);
    }
    else
    {
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-16.0f, 0.0f, 0.0f), FVector(8.0f, 15.0f, 0.0f), 72.0f, 0.55f, -12.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(6.0f, 2.0f, 0.0f), FVector(9.0f, 13.0f, 0.0f), 94.0f, 0.6f, 8.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(20.0f, -4.0f, 0.0f), FVector(6.0f, 11.0f, 0.0f), 60.0f, 0.5f, 18.0f, Accent);
    }

    SourceMesh->SetMaterial(0, IsRockSource(PresentationId) ? RockMaterial : BarkMaterial);
    SourceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, {}, {}, Colors, {}, false);
    SourceMesh->SetVisibility(!bHarvested);
}

void AKalmalaHarvestNode::ApplyHarvestedState()
{
    SetActorHiddenInGame(bHarvested);
    Collision->SetCollisionEnabled(bHarvested ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
    if (SourceMesh) SourceMesh->SetVisibility(!bHarvested && !GetGatheringPresentationId().IsNone());
}
