#include "KalmalaDiscoveryActor.h"
#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void AddKalmalaDiscoveryPrism(
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
        for (const FVector& Corner : BottomCorners) Vertices.Add(Centre + Rotation.TransformVector(Corner));
        for (const FVector& Corner : BottomCorners) Vertices.Add(Centre + Rotation.TransformVector(FVector(Corner.X * TopScale, Corner.Y * TopScale, Height)));
        for (int32 VertexIndex = 0; VertexIndex < 8; ++VertexIndex) Colors.Add(Colour);

        const int32 FaceIndices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 6, 5, 4, 7, 6,
            0, 4, 5, 0, 5, 1,
            1, 5, 6, 1, 6, 2,
            2, 6, 7, 2, 7, 3,
            3, 7, 4, 3, 4, 0
        };
        for (const int32 Index : FaceIndices) Triangles.Add(BaseIndex + Index);
    }

    FLinearColor GetDiscoveryAccent(const FName PresentationId)
    {
        if (PresentationId == TEXT("stone-hollow-marker")) return FLinearColor(0.42f, 0.34f, 0.25f);
        if (PresentationId == TEXT("island-cache-marker")) return FLinearColor(0.22f, 0.52f, 0.62f);
        if (PresentationId == TEXT("root-hollow-marker")) return FLinearColor(0.34f, 0.20f, 0.10f);
        if (PresentationId == TEXT("sunken-cache-marker")) return FLinearColor(0.16f, 0.22f, 0.18f);
        if (PresentationId == TEXT("ice-spring-marker")) return FLinearColor(0.58f, 0.82f, 0.88f);
        if (PresentationId == TEXT("storm-overlook-marker")) return FLinearColor(0.30f, 0.36f, 0.48f);
        return FLinearColor::Transparent;
    }
}

AKalmalaDiscoveryActor::AKalmalaDiscoveryActor()
{
    bReplicates = true; SetReplicateMovement(false);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision")); Collision->InitSphereRadius(55.0f); Collision->SetCollisionProfileName(TEXT("BlockAll")); RootComponent = Collision;
    DiscoveryMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DiscoveryPresentation"));
    DiscoveryMesh->SetupAttachment(RootComponent);
    DiscoveryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DiscoveryMesh->SetCastShadow(true);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    if (Material.Succeeded()) DiscoveryMaterial = Material.Object;
}
void AKalmalaDiscoveryActor::InitializeServer(const FKalmalaWorldDiscoveryDescriptor& InDescriptor)
{
    if (!HasAuthority() || !Descriptor.DefinitionId.IsEmpty() || InDescriptor.DefinitionId.IsEmpty() || InDescriptor.Location.ContainsNaN()) return;
    if (InDescriptor.Kind == EKalmalaWorldDiscoveryKind::PointOfInterest)
    {
        const FName CandidateSourceId(*InDescriptor.DefinitionId);
        if (!FKalmalaBiomeContentContract::IsValidRareDiscoverySourceId(CandidateSourceId)) return;
        RareDiscoverySourceId = CandidateSourceId;
    }
    Descriptor = InDescriptor;
    SetActorLocation(InDescriptor.Location);
    BuildDiscoveryPresentation();
}
bool AKalmalaDiscoveryActor::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{ return HasAuthority() && IsValid(Interactor) && Interactor->HasAuthority() && Interactor->GetWorld() == GetWorld() && !Descriptor.DefinitionId.IsEmpty() && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(250.0f); }
void AKalmalaDiscoveryActor::Interact_Implementation(AKalmalaCharacter* Interactor)
{ if (!CanInteract_Implementation(Interactor)) return; if (AKalmalaGameMode* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>()) Mode->ClaimDiscovery(Interactor, Descriptor); }

void AKalmalaDiscoveryActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaDiscoveryActor, RareDiscoverySourceId);
}

void AKalmalaDiscoveryActor::OnRep_RareDiscoverySourceId()
{
    BuildDiscoveryPresentation();
}

void AKalmalaDiscoveryActor::BuildDiscoveryPresentation()
{
    if (!DiscoveryMesh) return;
    DiscoveryMesh->ClearAllMeshSections();

    const FName PresentationId = FKalmalaBiomeContentContract::GetRareDiscoveryPresentationId(RareDiscoverySourceId);
    if (PresentationId.IsNone())
    {
        DiscoveryMesh->SetVisibility(false);
        return;
    }

    const FLinearColor Accent = GetDiscoveryAccent(PresentationId);
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FLinearColor> Colors;
    if (PresentationId == TEXT("stone-hollow-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(-16.0f, 0.0f, 0.0f), FVector(28.0f, 12.0f, 0.0f), 30.0f, 0.78f, -10.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(16.0f, 2.0f, 0.0f), FVector(20.0f, 10.0f, 0.0f), 24.0f, 0.72f, 12.0f, Accent);
    }
    else if (PresentationId == TEXT("island-cache-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 0.0f), FVector(24.0f, 18.0f, 0.0f), 24.0f, 0.88f, 0.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 24.0f), FVector(15.0f, 11.0f, 0.0f), 20.0f, 0.82f, 0.0f, FLinearColor(0.48f, 0.68f, 0.66f));
    }
    else if (PresentationId == TEXT("root-hollow-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(-24.0f, 0.0f, 0.0f), FVector(10.0f, 16.0f, 0.0f), 64.0f, 0.55f, -18.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(24.0f, 0.0f, 0.0f), FVector(10.0f, 16.0f, 0.0f), 64.0f, 0.55f, 18.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 42.0f), FVector(30.0f, 10.0f, 0.0f), 18.0f, 0.75f, 0.0f, Accent);
    }
    else if (PresentationId == TEXT("sunken-cache-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 0.0f), FVector(25.0f, 16.0f, 0.0f), 22.0f, 0.92f, 0.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 22.0f), FVector(21.0f, 6.0f, 0.0f), 8.0f, 0.9f, 0.0f, FLinearColor(0.26f, 0.32f, 0.24f));
    }
    else if (PresentationId == TEXT("ice-spring-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(-16.0f, 0.0f, 0.0f), FVector(8.0f, 8.0f, 0.0f), 54.0f, 0.26f, -8.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(8.0f, 4.0f, 0.0f), FVector(10.0f, 9.0f, 0.0f), 70.0f, 0.2f, 10.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(22.0f, -4.0f, 0.0f), FVector(6.0f, 7.0f, 0.0f), 42.0f, 0.3f, 18.0f, Accent);
    }
    else if (PresentationId == TEXT("storm-overlook-marker"))
    {
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 0.0f), FVector(18.0f, 14.0f, 0.0f), 32.0f, 0.76f, 0.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 32.0f), FVector(12.0f, 10.0f, 0.0f), 38.0f, 0.68f, 0.0f, Accent);
        AddKalmalaDiscoveryPrism(Vertices, Triangles, Colors, FVector(0.0f, 0.0f, 70.0f), FVector(6.0f, 6.0f, 0.0f), 28.0f, 0.5f, 0.0f, FLinearColor(0.48f, 0.52f, 0.62f));
    }

    DiscoveryMesh->SetMaterial(0, DiscoveryMaterial);
    DiscoveryMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, {}, {}, Colors, {}, false);
    DiscoveryMesh->SetVisibility(true);
}
