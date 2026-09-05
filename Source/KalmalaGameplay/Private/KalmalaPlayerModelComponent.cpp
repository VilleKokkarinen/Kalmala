#include "KalmalaPlayerModelComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UKalmalaPlayerModelComponent::UKalmalaPlayerModelComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cloth(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain.M_GeneratedTerrain"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Head(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark.M_GeneratedBark"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dark(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    ClothMaterial = Cloth.Object;
    HeadMaterial = Head.Object;
    DarkMaterial = Dark.Object;
}

UProceduralMeshComponent* UKalmalaPlayerModelComponent::AddPart(const FName Name, const FVector Pivot,
    const FVector Centre, const FVector HalfSize, UMaterialInterface* Material)
{
    auto* Part = NewObject<UProceduralMeshComponent>(GetOwner(), Name, RF_Transient);
    Part->SetupAttachment(this);
    Part->SetRelativeLocation(Pivot);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);
    Part->SetMaterial(0, Material);
    GetOwner()->AddInstanceComponent(Part);
    Part->RegisterComponent();
    TArray<FVector> Vertices, Normals;
    TArray<int32> Triangles;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colours;
    TArray<FProcMeshTangent> Tangents;
    const auto Face = [&](FVector C, FVector U, FVector V)
    {
        const int32 I = Vertices.Num();
        Vertices.Append({C - U - V, C - U + V, C + U - V, C + U + V});
        Triangles.Append({I, I + 1, I + 2, I + 2, I + 1, I + 3});
        const FVector Normal = FVector::CrossProduct(U, V).GetSafeNormal();
        for (int32 J = 0; J < 4; ++J)
        {
            Normals.Add(Normal);
            Colours.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent(U.GetSafeNormal(), false));
        }
        UVs.Append({FVector2D(0, 0), FVector2D(0, 1), FVector2D(1, 0), FVector2D(1, 1)});
    };
    const FVector X(HalfSize.X, 0, 0), Y(0, HalfSize.Y, 0), Z(0, 0, HalfSize.Z);
    Face(Centre + X, Y, Z); Face(Centre - X, -Y, Z);
    Face(Centre + Y, -X, Z); Face(Centre - Y, X, Z);
    Face(Centre + Z, X, Y); Face(Centre - Z, X, -Y);
    Part->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);
    Parts.Add(Part);
    return Part;
}

void UKalmalaPlayerModelComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) { SetComponentTickEnabled(false); return; }
    AddPart(TEXT("PlayerTorso"), FVector::ZeroVector, FVector(0, 0, 106), FVector(13, 19, 30), ClothMaterial);
    AddPart(TEXT("PlayerBelt"), FVector::ZeroVector, FVector(0, 0, 79), FVector(14, 20, 5), DarkMaterial);
    AddPart(TEXT("PlayerNeck"), FVector::ZeroVector, FVector(0, 0, 140), FVector(7, 7, 5), HeadMaterial);
    AddPart(TEXT("PlayerHead"), FVector::ZeroVector, FVector(0, 0, 158), FVector(13, 12, 14), HeadMaterial);
    AddPart(TEXT("PlayerFace"), FVector::ZeroVector, FVector(13.5f, 0, 160), FVector(1, 8, 3), DarkMaterial);
    LeftArm = AddPart(TEXT("PlayerLeftArm"), FVector(0, -27, 129), FVector(0, 0, -27), FVector(7, 7, 29), ClothMaterial);
    RightArm = AddPart(TEXT("PlayerRightArm"), FVector(0, 27, 129), FVector(0, 0, -27), FVector(7, 7, 29), ClothMaterial);
    LeftLeg = AddPart(TEXT("PlayerLeftLeg"), FVector(0, -10, 76), FVector(2, 0, -38), FVector(9, 8, 38), DarkMaterial);
    RightLeg = AddPart(TEXT("PlayerRightLeg"), FVector(0, 10, 76), FVector(2, 0, -38), FVector(9, 8, 38), DarkMaterial);
}

void UKalmalaPlayerModelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !LeftArm) return;
    const float Speed = Character->GetVelocity().Size2D();
    const bool bFalling = Character->GetCharacterMovement()->IsFalling();
    GaitPhase = FMath::Fmod(GaitPhase + DeltaTime * Speed / 45.0f, 2.0f * PI);
    const float Swing = bFalling ? 0.0f : FMath::Sin(GaitPhase) * FMath::Clamp(Speed / 600.0f, 0.0f, 1.4f) * 25.0f;
    LeftArm->SetRelativeRotation(FRotator(bFalling ? -35.0f : Swing, 0, 0));
    RightArm->SetRelativeRotation(FRotator(bFalling ? -35.0f : -Swing, 0, 0));
    LeftLeg->SetRelativeRotation(FRotator(bFalling ? 15.0f : -Swing, 0, 0));
    RightLeg->SetRelativeRotation(FRotator(bFalling ? 15.0f : Swing, 0, 0));
}
