#include "KalmalaPlayerModelComponent.h"
#include "ProceduralMeshComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UKalmalaPlayerModelComponent::UKalmalaPlayerModelComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Cloth(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain.M_GeneratedTerrain"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Head(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark.M_GeneratedBark"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dark(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    ClothMaterial = Cloth.Object;
    HeadMaterial = Head.Object;
    DarkMaterial = Dark.Object;
}

UProceduralMeshComponent* UKalmalaPlayerModelComponent::AddPart(const FName Name, const FVector Pivot,
    const FVector Centre, const FVector HalfSize, UMaterialInterface* Material, const bool bTapered,
    USceneComponent* Parent)
{
    auto* Part = NewObject<UProceduralMeshComponent>(GetOwner(), Name, RF_Transient);
    Part->SetupAttachment(Parent ? Parent : this);
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
    const auto Quad = [&](const FVector A, const FVector B, const FVector C, const FVector D)
    {
        const int32 I = Vertices.Num();
        Vertices.Append({A, B, C, D});
        // Unreal front faces use clockwise winding relative to outward shading normals.
        Triangles.Append({I, I + 2, I + 1, I + 2, I + 3, I + 1});
        const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
        for (int32 J = 0; J < 4; ++J)
        {
            Normals.Add(Normal);
            Colours.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent((B - A).GetSafeNormal(), false));
        }
        UVs.Append({FVector2D(0, 0), FVector2D(0, 1), FVector2D(1, 0), FVector2D(1, 1)});
    };
    const auto Triangle = [&](const FVector A, const FVector B, const FVector C)
    {
        const int32 I = Vertices.Num();
        Vertices.Append({A, B, C});
        Triangles.Append({I, I + 2, I + 1});
        const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
        for (int32 J = 0; J < 3; ++J)
        {
            Normals.Add(Normal);
            Colours.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent((B - A).GetSafeNormal(), false));
        }
        UVs.Append({FVector2D(0, 0), FVector2D(0, 1), FVector2D(1, 0)});
    };
    const auto Face = [&](const FVector C, const FVector U, const FVector V)
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
    if (bTapered)
    {
        // An octagonal taper gives the mantle and hood an original carved silhouette
        // while keeping the presentation local-only and free of gameplay collision.
        constexpr int32 SideCount = 8;
        constexpr float TopScale = 0.78f;
        TArray<FVector> BottomRing;
        TArray<FVector> TopRing;
        BottomRing.Reserve(SideCount);
        TopRing.Reserve(SideCount);
        for (int32 Side = 0; Side < SideCount; ++Side)
        {
            const float Angle = (2.0f * PI * static_cast<float>(Side)) / static_cast<float>(SideCount);
            const float Cosine = FMath::Cos(Angle);
            const float Sine = FMath::Sin(Angle);
            BottomRing.Add(Centre + FVector(Cosine * HalfSize.X, Sine * HalfSize.Y, -HalfSize.Z));
            TopRing.Add(Centre + FVector(Cosine * HalfSize.X * TopScale, Sine * HalfSize.Y * TopScale, HalfSize.Z));
        }
        for (int32 Side = 0; Side < SideCount; ++Side)
        {
            const int32 NextSide = (Side + 1) % SideCount;
            Quad(BottomRing[Side], BottomRing[NextSide], TopRing[Side], TopRing[NextSide]);
        }
        const FVector BottomCentre = Centre + FVector(0, 0, -HalfSize.Z);
        const FVector TopCentre = Centre + FVector(0, 0, HalfSize.Z);
        for (int32 Side = 0; Side < SideCount; ++Side)
        {
            const int32 NextSide = (Side + 1) % SideCount;
            Triangle(BottomCentre, BottomRing[NextSide], BottomRing[Side]);
            Triangle(TopCentre, TopRing[Side], TopRing[NextSide]);
        }
    }
    else
    {
        const FVector X(HalfSize.X, 0, 0), Y(0, HalfSize.Y, 0), Z(0, 0, HalfSize.Z);
        Face(Centre + X, Y, Z); Face(Centre - X, -Y, Z);
        Face(Centre + Y, -X, Z); Face(Centre - Y, X, Z);
        Face(Centre + Z, X, Y); Face(Centre - Z, X, -Y);
    }
    Part->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);
    Parts.Add(Part);
    return Part;
}

void UKalmalaPlayerModelComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) return;
    AddPart(TEXT("PlayerTorso"), FVector::ZeroVector, FVector(0, 0, 106), FVector(16, 21, 31), ClothMaterial, true);
    AddPart(TEXT("PlayerBelt"), FVector::ZeroVector, FVector(0, 0, 79), FVector(17, 22, 5), DarkMaterial);
    AddPart(TEXT("PlayerNeck"), FVector::ZeroVector, FVector(0, 0, 140), FVector(7, 7, 5), HeadMaterial);
    AddPart(TEXT("PlayerHead"), FVector::ZeroVector, FVector(0, 0, 158), FVector(14, 13, 14), HeadMaterial, true);
    AddPart(TEXT("PlayerFace"), FVector::ZeroVector, FVector(14.5f, 0, 160), FVector(1.5f, 8, 3), DarkMaterial);
    LeftArm = AddPart(TEXT("PlayerLeftArm"), FVector(0, -27, 129), FVector(0, 0, -27), FVector(7, 7, 29), ClothMaterial);
    RightArm = AddPart(TEXT("PlayerRightArm"), FVector(0, 27, 129), FVector(0, 0, -27), FVector(7, 7, 29), ClothMaterial);
    // Reuse a bounded cosmetic pool; equipment changes never accumulate components.
    for (int32 Index = 0; Index < 3; ++Index)
    {
        HeldToolParts.Add(AddPart(FName(*FString::Printf(TEXT("HeldToolPart%d"), Index)),
            FVector::ZeroVector, FVector::ZeroVector, FVector(1), Index == 0 ? DarkMaterial : HeadMaterial,
            Index == 1, RightArm));
    }
    OnRep_HeldToolVisual();
    LeftLeg = AddPart(TEXT("PlayerLeftLeg"), FVector(0, -10, 76), FVector(2, 0, -38), FVector(9, 8, 38), DarkMaterial);
    RightLeg = AddPart(TEXT("PlayerRightLeg"), FVector(0, 10, 76), FVector(2, 0, -38), FVector(9, 8, 38), DarkMaterial);
}

void UKalmalaPlayerModelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        const auto* Inventory = GetOwner()->FindComponentByClass<UKalmalaInventoryComponent>();
        const auto Visual = GetHeldToolVisualForItem(Inventory ? Inventory->GetActiveItem() : NAME_None);
        if (Visual != HeldToolVisual)
        {
            HeldToolVisual = Visual;
            OnRep_HeldToolVisual();
            GetOwner()->ForceNetUpdate();
        }
    }
    const auto* Character = Cast<ACharacter>(GetOwner());
    if (!Character || !LeftArm) return;
    const float Speed = Character->GetVelocity().Size2D();
    const bool bFalling = Character->GetCharacterMovement()->IsFalling();
    GaitPhase = FMath::Fmod(GaitPhase + DeltaTime * Speed / 45.0f, 2.0f * PI);
    const float Swing = bFalling ? 0.0f : FMath::Sin(GaitPhase) * FMath::Clamp(Speed / 600.0f, 0.0f, 1.4f) * 25.0f;
    float LeftArmPitch = bFalling ? -35.0f : Swing;
    float RightArmPitch = bFalling ? -35.0f : -Swing;
    if (const auto* Combat = Character->FindComponentByClass<UKalmalaCombatComponent>())
    {
        if (Combat->GetActionSerial() != LastAttackSerial)
        {
            LastAttackSerial = Combat->GetActionSerial();
            AttackSwingElapsedSeconds = Combat->GetActionPhase() == EKalmalaCombatActionPhase::Idle ? -1.0f : 0.0f;
        }
        if (AttackSwingElapsedSeconds >= 0.0f)
        {
            if (EvaluateAttackArmPose(AttackSwingElapsedSeconds, RightArmPitch, RightArmPitch))
                AttackSwingElapsedSeconds += DeltaTime;
            else
                AttackSwingElapsedSeconds = -1.0f;
        }
    }
    if (ToolSharpeningElapsedSeconds >= 0.0f)
    {
        if (EvaluateToolSharpeningArmPose(ToolSharpeningElapsedSeconds, LeftArmPitch, RightArmPitch,
            LeftArmPitch, RightArmPitch))
        {
            ToolSharpeningElapsedSeconds += DeltaTime;
        }
        else
        {
            ToolSharpeningElapsedSeconds = -1.0f;
        }
    }
    LeftArm->SetRelativeRotation(FRotator(LeftArmPitch, 0, 0));
    RightArm->SetRelativeRotation(FRotator(RightArmPitch, 0, 0));
    LeftLeg->SetRelativeRotation(FRotator(bFalling ? 15.0f : -Swing, 0, 0));
    RightLeg->SetRelativeRotation(FRotator(bFalling ? 15.0f : Swing, 0, 0));
}

EKalmalaHeldToolVisual UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(const FName ItemId)
{
    const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(ItemId);
    if (!Definition) return EKalmalaHeldToolVisual::None;
    switch (Definition->Kind)
    {
    case EKalmalaToolKind::ConstructionHammer: return EKalmalaHeldToolVisual::Hammer;
    case EKalmalaToolKind::StoneAxe: case EKalmalaToolKind::BronzeAxe: case EKalmalaToolKind::IronAxe:
        return EKalmalaHeldToolVisual::Axe;
    case EKalmalaToolKind::StoneKnife: case EKalmalaToolKind::BronzeKnife: case EKalmalaToolKind::IronKnife:
        return EKalmalaHeldToolVisual::Knife;
    case EKalmalaToolKind::StonePick: case EKalmalaToolKind::BronzePick: case EKalmalaToolKind::IronPick:
        return EKalmalaHeldToolVisual::Pick;
    case EKalmalaToolKind::StoneSpear: case EKalmalaToolKind::BronzeSpear: case EKalmalaToolKind::IronSpear:
        return EKalmalaHeldToolVisual::Spear;
    case EKalmalaToolKind::StoneMace: case EKalmalaToolKind::BronzeMace: case EKalmalaToolKind::IronMace:
        return EKalmalaHeldToolVisual::Mace;
    case EKalmalaToolKind::Bow: return EKalmalaHeldToolVisual::Bow;
    default: return EKalmalaHeldToolVisual::None;
    }
}

void UKalmalaPlayerModelComponent::OnRep_HeldToolVisual()
{
    if (HeldToolParts.Num() != 3) return;
    for (UProceduralMeshComponent* Part : HeldToolParts) Part->SetVisibility(false);
    // Author silhouettes in their existing tool space, then rotate the whole tool
    // about its grip into the palm at the end of the arm (not about each part).
    const bool bBow = HeldToolVisual == EKalmalaHeldToolVisual::Bow;
    const FVector AuthoredGrip = bBow ? FVector(18, 0, -44) : FVector(8, 0, -50);
    const FVector HandGrip(0, 0, -52);
    const FQuat GripRotation = bBow ? FQuat::Identity : FRotator(-90, 0, 0).Quaternion();
    const auto Show = [this, AuthoredGrip, HandGrip, GripRotation](int32 Index, FVector Centre,
        FVector HalfSize, FRotator Rotation = FRotator::ZeroRotator)
    {
        UProceduralMeshComponent* Part = HeldToolParts[Index];
        Part->SetRelativeLocation(HandGrip + GripRotation.RotateVector(Centre - AuthoredGrip));
        Part->SetRelativeScale3D(HalfSize);
        Part->SetRelativeRotation(GripRotation * Rotation.Quaternion());
        Part->SetVisibility(true);
    };
    switch (HeldToolVisual)
    {
    case EKalmalaHeldToolVisual::Hammer:
        Show(0, FVector(8, 0, -47), FVector(2.5f, 2.5f, 13));
        Show(1, FVector(13, 0, -35), FVector(8, 3.5f, 3.5f)); break;
    case EKalmalaHeldToolVisual::Axe:
        Show(0, FVector(8, 0, -45), FVector(2, 2, 18));
        Show(1, FVector(17, 0, -29), FVector(10, 2, 8));
        Show(2, FVector(24, 0, -32), FVector(3, 1, 10)); break;
    case EKalmalaHeldToolVisual::Knife:
        Show(0, FVector(8, 0, -48), FVector(2, 2, 5));
        Show(1, FVector(8, 0, -34), FVector(3, 0.8f, 9)); break;
    case EKalmalaHeldToolVisual::Pick:
        Show(0, FVector(8, 0, -45), FVector(2, 2, 19));
        Show(1, FVector(17, 0, -29), FVector(10, 2, 2), FRotator(-18, 0, 0));
        Show(2, FVector(-1, 0, -29), FVector(10, 2, 2), FRotator(18, 0, 0)); break;
    case EKalmalaHeldToolVisual::Spear:
        Show(0, FVector(8, 0, -32), FVector(1.7f, 1.7f, 45));
        Show(1, FVector(8, 0, 19), FVector(4, 1, 8)); break;
    case EKalmalaHeldToolVisual::Mace:
        Show(0, FVector(8, 0, -45), FVector(2, 2, 16));
        Show(1, FVector(8, 0, -26), FVector(7, 7, 7)); break;
    case EKalmalaHeldToolVisual::Bow:
        Show(0, FVector(12, 0, -25), FVector(2, 2, 20), FRotator(18, 0, 0));
        Show(1, FVector(12, 0, -63), FVector(2, 2, 20), FRotator(-18, 0, 0));
        Show(2, FVector(6, 0, -44), FVector(0.5f, 0.5f, 38)); break;
    default: break;
    }
}

void UKalmalaPlayerModelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UKalmalaPlayerModelComponent, HeldToolVisual);
}

bool UKalmalaPlayerModelComponent::EvaluateAttackArmPose(const float ElapsedSeconds,
    const float RestRightArmPitch, float& OutRightArmPitch)
{
    if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds < 0.0f || ElapsedSeconds >= AttackSwingDurationSeconds)
        return false;
    // Raise during windup, strike down, then blend back to the current gait.
    if (ElapsedSeconds < 0.18f)
        OutRightArmPitch = FMath::Lerp(RestRightArmPitch, -115.0f, ElapsedSeconds / 0.18f);
    else if (ElapsedSeconds < 0.30f)
        OutRightArmPitch = FMath::Lerp(-115.0f, 35.0f, (ElapsedSeconds - 0.18f) / 0.12f);
    else
        OutRightArmPitch = FMath::Lerp(35.0f, RestRightArmPitch, (ElapsedSeconds - 0.30f) / 0.24f);
    return true;
}

bool UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(const float ElapsedSeconds,
    const float RestLeftArmPitch, const float RestRightArmPitch, float& OutLeftArmPitch, float& OutRightArmPitch)
{
    if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds < 0.0f || ElapsedSeconds >= ToolSharpeningDurationSeconds)
        return false;

    constexpr float FadeSeconds = 0.16f;
    constexpr float StrokeCycles = 3.0f;
    const float Envelope = FMath::Clamp(FMath::Min(ElapsedSeconds / FadeSeconds,
        (ToolSharpeningDurationSeconds - ElapsedSeconds) / FadeSeconds), 0.0f, 1.0f);
    const float Stroke = FMath::Sin((ElapsedSeconds / ToolSharpeningDurationSeconds) * StrokeCycles * 2.0f * PI);
    const float BracingPitch = -38.0f;
    const float SharpeningPitch = -52.0f + 16.0f * Stroke;
    OutLeftArmPitch = FMath::Lerp(RestLeftArmPitch, BracingPitch, Envelope);
    OutRightArmPitch = FMath::Lerp(RestRightArmPitch, SharpeningPitch, Envelope);
    return true;
}

void UKalmalaPlayerModelComponent::MulticastPlayGrindingStoneSharpening_Implementation()
{
    ToolSharpeningElapsedSeconds = 0.0f;
}
