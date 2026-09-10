#include "KalmalaConstructionActor.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"

namespace
{
    void AddBox(TArray<FVector>& Vertices, TArray<int32>& Triangles, const FVector& Centre, const FVector& HalfExtent)
    {
        const int32 Base = Vertices.Num();
        for (const FVector& Corner : { FVector(-1,-1,-1), FVector(1,-1,-1), FVector(1,1,-1), FVector(-1,1,-1), FVector(-1,-1,1), FVector(1,-1,1), FVector(1,1,1), FVector(-1,1,1) })
            Vertices.Add(Centre + Corner * HalfExtent);
        for (const int32 Index : { 0,2,1, 0,3,2, 4,5,6, 4,6,7, 0,1,5, 0,5,4, 1,2,6, 1,6,5, 2,3,7, 2,7,6, 3,0,4, 3,4,7 }) Triangles.Add(Base + Index);
    }
}

AKalmalaConstructionActor::AKalmalaConstructionActor()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("ConstructionCollision"));
    Collision->SetBoxExtent(FVector(54, 54, 56));
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
    PieceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ConstructionPiece"));
    PieceMesh->SetupAttachment(Collision);
    PieceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PieceMesh->SetGenerateOverlapEvents(false);
    PieceMesh->SetCanEverAffectNavigation(false);
}

void AKalmalaConstructionActor::InitializeFromServer(const FName InKit, const FString& InConstructionId)
{
    if (HasAuthority()) { ConstructionKit = InKit; ConstructionId = InConstructionId.Left(64); ApplyConstructionKit(); }
}

bool AKalmalaConstructionActor::CanUse(const AKalmalaCharacter* Character) const
{
    if (!IsValid(Character) || !Character->GetController() || Character->GetWorld() != GetWorld() || ConstructionId.IsEmpty()
        || (ConstructionKit != TEXT("WorkbenchKit") && ConstructionKit != TEXT("StorageKit"))
        || Character->GetActorLocation().ContainsNaN() || GetActorLocation().ContainsNaN()
        || FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) > FMath::Square(250.0)) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionUse), false, Character);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), GetActorLocation(), ECC_Visibility, Query)
        && Hit.GetActor() == this;
}

bool AKalmalaConstructionActor::CanInteract_Implementation(AKalmalaCharacter* Character) const
{
    return HasAuthority() && IsValid(Character) && Character->HasAuthority() && CanUse(Character);
}

void AKalmalaConstructionActor::Interact_Implementation(AKalmalaCharacter* Character)
{
    if (!CanInteract_Implementation(Character)) return;
    if (auto* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>())
        Crafting->InteractWithConstructionFromServer(this);
}

bool AKalmalaConstructionActor::IsShelterKit(const FName KitId)
{
    return KitId == TEXT("WallKit") || KitId == TEXT("RoofKit");
}

FVector AKalmalaConstructionActor::GetCollisionExtent(const FName KitId)
{
    if (KitId == TEXT("FloorKit")) return FVector(120, 120, 12);
    if (KitId == TEXT("WallKit")) return FVector(120, 12, 110);
    if (KitId == TEXT("RoofKit")) return FVector(132, 132, 16);
    return FVector(54, 54, 56);
}

void AKalmalaConstructionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionKit);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionId);
}

void AKalmalaConstructionActor::OnRep_ConstructionState()
{
    ApplyConstructionKit();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest")) && !ConstructionId.IsEmpty()
        && ConstructionKit != NAME_None && LastLoggedReplicationId != ConstructionId)
    {
        LastLoggedReplicationId = ConstructionId;
        UE_LOG(LogTemp, Display, TEXT("Construction replicated: Id=%s Kit=%s"), *ConstructionId, *ConstructionKit.ToString());
    }
#endif
}

void AKalmalaConstructionActor::ApplyConstructionKit()
{
    if (!Collision) return;
    Collision->SetBoxExtent(GetCollisionExtent(ConstructionKit));
    Tags.Remove(TEXT("KalmalaShelterRoof"));
    Tags.Remove(TEXT("KalmalaShelterWindbreak"));
    if (ConstructionKit == TEXT("RoofKit")) Tags.AddUnique(TEXT("KalmalaShelterRoof"));
    if (ConstructionKit == TEXT("WallKit")) Tags.AddUnique(TEXT("KalmalaShelterWindbreak"));
    BuildPiecePresentation();
}

void AKalmalaConstructionActor::BuildPiecePresentation()
{
    if (!PieceMesh) return;
    TArray<FVector> Vertices; TArray<int32> Triangles;
    if (ConstructionKit == TEXT("FloorKit"))
        AddBox(Vertices, Triangles, FVector(0, 0, -44), FVector(120, 120, 12));
    else if (ConstructionKit == TEXT("WallKit"))
        AddBox(Vertices, Triangles, FVector(0, 0, 54), FVector(120, 12, 110));
    else if (ConstructionKit == TEXT("RoofKit"))
        AddBox(Vertices, Triangles, FVector(0, 0, 248), FVector(132, 132, 16));
    else if (ConstructionKit == TEXT("WorkbenchKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, 36), FVector(54, 54, 10));
        for (const float X : {-40.0f, 40.0f}) for (const float Y : {-40.0f, 40.0f})
            AddBox(Vertices, Triangles, FVector(X, Y, -14), FVector(8, 8, 40));
    }
    else if (ConstructionKit == TEXT("StorageKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -10), FVector(50, 50, 44));
        AddBox(Vertices, Triangles, FVector(0, 0, 42), FVector(54, 54, 8));
        AddBox(Vertices, Triangles, FVector(52, 0, 25), FVector(2, 10, 12));
    }
    else
        AddBox(Vertices, Triangles, FVector::ZeroVector, GetCollisionExtent(ConstructionKit));
    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
    TArray<FVector2D> UVs; UVs.Init(FVector2D::ZeroVector, Vertices.Num());
    TArray<FLinearColor> Colours; Colours.Init(FLinearColor::White, Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(), Vertices.Num());
    PieceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);
}
