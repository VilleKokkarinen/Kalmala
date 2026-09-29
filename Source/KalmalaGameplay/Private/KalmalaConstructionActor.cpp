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
    const bool bIsStorageOrCraftingStation = IsStorageKit(ConstructionKit)
        || IsCraftingStationKit(ConstructionKit);
    if (!IsValid(Character) || !Character->GetController() || Character->GetWorld() != GetWorld() || ConstructionId.IsEmpty()
        || (!bIsStorageOrCraftingStation && ConstructionKit != TEXT("GrindingStoneKit"))
        || Character->GetActorLocation().ContainsNaN() || GetActorLocation().ContainsNaN()
        || FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) > FMath::Square(250.0)) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionUse), false, Character);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), GetActorLocation(), ECC_Visibility, Query)
        && Hit.GetActor() == this;
}

void AKalmalaConstructionActor::AdvanceRainWearFromServer(const float DeltaSeconds, const float Precipitation)
{
    if (!HasAuthority() || ConstructionId.IsEmpty() || !GetWorld() || !Collision
        || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f
        || !FMath::IsFinite(Precipitation) || Precipitation <= 0.0f
        || (ConstructionKit != TEXT("FloorKit") && ConstructionKit != TEXT("WallKit")
            && ConstructionKit != TEXT("WorkbenchKit") && ConstructionKit != TEXT("StorageKit"))) return;
    const FVector Start = Collision->GetComponentLocation() + FVector(0, 0, 60);
    if (Start.ContainsNaN()) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionRainProtection), false, this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + FVector(0, 0, 400), ECC_Visibility, Query)
        && IsValid(Hit.GetActor()) && Hit.GetActor()->ActorHasTag(TEXT("KalmalaShelterRoof"))) return;
    const float NextHealth = FMath::Clamp(Health - RainWearPerSecond * FMath::Clamp(Precipitation, 0.0f, 1.0f) * DeltaSeconds,
        RainHealthFloor, MaximumHealth);
    if (NextHealth != Health)
    {
        Health = NextHealth;
        ForceNetUpdate();
    }
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

bool AKalmalaConstructionActor::IsCraftingStationKit(const FName KitId)
{
    return KitId == TEXT("WorkbenchKit") || KitId == TEXT("ForgeKit") || KitId == TEXT("CookingRackKit") || KitId == TEXT("CauldronKit")
        || KitId == TEXT("FryingPanKit") || KitId == TEXT("SmokeFrameKit") || KitId == TEXT("DryingLineKit");
}

bool AKalmalaConstructionActor::IsStorageKit(const FName KitId)
{
    return KitId == TEXT("StorageKit");
}

FVector AKalmalaConstructionActor::GetCollisionExtent(const FName KitId)
{
    if (KitId == TEXT("FloorKit")) return FVector(120, 120, 12);
    if (KitId == TEXT("WallKit")) return FVector(120, 12, 110);
    if (KitId == TEXT("RoofKit")) return FVector(132, 132, 16);
    if (KitId == TEXT("SmokeFrameKit")) return FVector(56, 56, 64);
    if (KitId == TEXT("DryingLineKit")) return FVector(54, 40, 54);
    if (KitId == TEXT("FryingPanKit")) return FVector(64, 40, 12);
    if (KitId == TEXT("ForgeKit")) return FVector(60, 60, 62);
    if (KitId == TEXT("WorkbenchToolRackKit")) return FVector(32, 25, 34);
    if (KitId == TEXT("ForgeAnvilKit")) return FVector(28, 16, 28);
    if (KitId == TEXT("GrindingStoneKit")) return FVector(42, 34, 18);
    return FVector(54, 54, 56);
}

void AKalmalaConstructionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionKit);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionId);
    DOREPLIFETIME(AKalmalaConstructionActor, Health);
}

void AKalmalaConstructionActor::OnRep_ConstructionState()
{
    ApplyConstructionKit();
#if !UE_BUILD_SHIPPING
    if ((FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("KalmalaPersistedCampRestoreTest"))) && !ConstructionId.IsEmpty()
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
    if (ConstructionKit == TEXT("RoofKit"))
        Tags.AddUnique(TEXT("KalmalaShelterRoof"));
    if (ConstructionKit == TEXT("WallKit")) Tags.AddUnique(TEXT("KalmalaShelterWindbreak"));
    BuildPiecePresentation();
}

void AKalmalaConstructionActor::BuildPiecePresentation()
{
    if (!PieceMesh) return;
    TArray<FVector> Vertices; TArray<int32> Triangles;
    // Placement and saved transforms already locate the collision centre.
    // Derive the visible solid from that same footprint on both peers.
    if (ConstructionKit == TEXT("FloorKit") || IsShelterKit(ConstructionKit))
        AddBox(Vertices, Triangles, FVector::ZeroVector, GetCollisionExtent(ConstructionKit));
    else if (ConstructionKit == TEXT("CookingRackKit"))
    {
        for (const float X : {-34.0f, 34.0f}) for (const float Y : {-30.0f, 30.0f})
            AddBox(Vertices, Triangles, FVector(X, Y, -12), FVector(5, 5, 42));
        AddBox(Vertices, Triangles, FVector(0, 0, 34), FVector(44, 38, 5));
        for (const float Y : {-28.0f, -12.0f, 4.0f, 20.0f})
            AddBox(Vertices, Triangles, FVector(0, Y, 42), FVector(38, 2, 3));
    }
    else if (ConstructionKit == TEXT("CauldronKit"))
    {
        for (const float X : {-27.0f, 27.0f}) for (const float Y : {-27.0f, 27.0f})
            AddBox(Vertices, Triangles, FVector(X, Y, -12), FVector(5, 5, 34));
        AddBox(Vertices, Triangles, FVector(0, 0, 12), FVector(38, 38, 24));
        AddBox(Vertices, Triangles, FVector(0, 0, 38), FVector(44, 44, 4));
        AddBox(Vertices, Triangles, FVector(-47, 0, 38), FVector(7, 3, 3));
        AddBox(Vertices, Triangles, FVector(47, 0, 38), FVector(7, 3, 3));
    }
    else if (ConstructionKit == TEXT("FryingPanKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -2), FVector(36, 36, 5));
        AddBox(Vertices, Triangles, FVector(43, 0, 1), FVector(17, 5, 4));
        AddBox(Vertices, Triangles, FVector(57, 0, 1), FVector(5, 7, 5));
    }
    else if (ConstructionKit == TEXT("SmokeFrameKit"))
    {
        for (const float X : {-40.0f, 40.0f}) for (const float Y : {-40.0f, 40.0f})
            AddBox(Vertices, Triangles, FVector(X, Y, -4), FVector(5, 5, 60));
        for (const float Z : {22.0f, 48.0f})
        {
            AddBox(Vertices, Triangles, FVector(0, -40, Z), FVector(44, 4, 4));
            AddBox(Vertices, Triangles, FVector(0, 40, Z), FVector(44, 4, 4));
            AddBox(Vertices, Triangles, FVector(-40, 0, Z), FVector(4, 44, 4));
            AddBox(Vertices, Triangles, FVector(40, 0, Z), FVector(4, 44, 4));
        }
        for (const float Y : {-24.0f, -8.0f, 8.0f, 24.0f})
            AddBox(Vertices, Triangles, FVector(0, Y, 30), FVector(36, 2, 3));
        AddBox(Vertices, Triangles, FVector(0, 0, 56), FVector(34, 34, 3));
    }
    else if (ConstructionKit == TEXT("DryingLineKit"))
    {
        for (const float X : {-38.0f, 38.0f})
            for (const float Y : {-24.0f, 24.0f})
                AddBox(Vertices, Triangles, FVector(X, Y, -4), FVector(4, 4, 50));
        AddBox(Vertices, Triangles, FVector(0, -24, 46), FVector(46, 4, 4));
        AddBox(Vertices, Triangles, FVector(0, 24, 46), FVector(46, 4, 4));
        for (const float X : {-28.0f, -14.0f, 0.0f, 14.0f, 28.0f})
        {
            AddBox(Vertices, Triangles, FVector(X, 0, 40), FVector(2, 22, 2));
            AddBox(Vertices, Triangles, FVector(X, 0, 14), FVector(5, 4, 11));
        }
    }
    else if (ConstructionKit == TEXT("WorkbenchKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, 36), FVector(54, 54, 10));
        for (const float X : {-40.0f, 40.0f}) for (const float Y : {-40.0f, 40.0f})
            AddBox(Vertices, Triangles, FVector(X, Y, -14), FVector(8, 8, 40));
    }
    else if (ConstructionKit == TEXT("ForgeKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -34), FVector(58, 58, 18));
        AddBox(Vertices, Triangles, FVector(0, 0, -2), FVector(42, 44, 24));
        AddBox(Vertices, Triangles, FVector(18, 10, 34), FVector(16, 18, 14));
        AddBox(Vertices, Triangles, FVector(-30, -18, 24), FVector(20, 15, 5));
        AddBox(Vertices, Triangles, FVector(-50, -18, 28), FVector(8, 6, 3));
    }
    else if (ConstructionKit == TEXT("WorkbenchToolRackKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, 22), FVector(26, 5, 5));
        for (const float X : {-21.0f, 21.0f})
            AddBox(Vertices, Triangles, FVector(X, 0, 0), FVector(4, 5, 34));
        for (const float X : {-12.0f, 0.0f, 12.0f})
            AddBox(Vertices, Triangles, FVector(X, -13, 8), FVector(2, 12, 2));
    }
    else if (ConstructionKit == TEXT("ForgeAnvilKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -17), FVector(15, 16, 11));
        AddBox(Vertices, Triangles, FVector(0, 0, -2), FVector(18, 11, 7));
        AddBox(Vertices, Triangles, FVector(-20, 0, 6), FVector(8, 8, 3));
        AddBox(Vertices, Triangles, FVector(18, 0, 6), FVector(9, 7, 3));
    }
    else if (ConstructionKit == TEXT("GrindingStoneKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -8), FVector(42, 34, 10));
        AddBox(Vertices, Triangles, FVector(0, 0, 5), FVector(35, 28, 4));
        AddBox(Vertices, Triangles, FVector(0, 0, 12), FVector(26, 8, 3));
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
