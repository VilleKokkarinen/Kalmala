#include "KalmalaConstructionActor.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"

namespace
{
    void AddBox(TArray<FVector>& Vertices, TArray<int32>& Triangles, const FVector& Centre,
        const FVector& HalfExtent, const FQuat& Orientation = FQuat::Identity)
    {
        const int32 Base = Vertices.Num();
        for (const FVector& Corner : { FVector(-1,-1,-1), FVector(1,-1,-1), FVector(1,1,-1), FVector(-1,1,-1), FVector(-1,-1,1), FVector(1,-1,1), FVector(1,1,1), FVector(-1,1,1) })
            Vertices.Add(Centre + Orientation.RotateVector(Corner * HalfExtent));
        for (const int32 Index : { 0,2,1, 0,3,2, 4,5,6, 4,6,7, 0,1,5, 0,5,4, 1,2,6, 1,6,5, 2,3,7, 2,7,6, 3,0,4, 3,4,7 }) Triangles.Add(Base + Index);
    }

    void AddBeam(TArray<FVector>& Vertices, TArray<int32>& Triangles, const FVector& Start,
        const FVector& End, const float HalfWidth)
    {
        const FVector Delta = End - Start;
        const float Length = Delta.Size();
        if (Length <= KINDA_SMALL_NUMBER) return;
        AddBox(Vertices, Triangles, (Start + End) * 0.5f,
            FVector(Length * 0.5f, HalfWidth, HalfWidth), FRotationMatrix::MakeFromX(Delta).ToQuat());
    }
}

AKalmalaConstructionActor::AKalmalaConstructionActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = 1.0f;
    CreateDefaultSubobject<UKalmalaResistanceComponent>(TEXT("Resistances"));
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
    StandingLightComponent = CreateDefaultSubobject<UPointLightComponent>(TEXT("StandingLightGlow"));
    StandingLightComponent->SetupAttachment(Collision);
    StandingLightComponent->SetMobility(EComponentMobility::Movable);
    StandingLightComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
    StandingLightComponent->SetIntensity(1800.0f);
    StandingLightComponent->SetLightColor(FLinearColor(1.0f, 0.56f, 0.22f));
    StandingLightComponent->SetAttenuationRadius(600.0f);
    StandingLightComponent->SetCastShadows(false);
    StandingLightComponent->SetVisibility(false);
    DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
    DoorPivot->SetupAttachment(Collision);
    DoorPivot->SetRelativeLocation(FVector(-50.0f, 0.0f, 0.0f));
    DoorLeafMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DoorLeaf"));
    DoorLeafMesh->SetupAttachment(DoorPivot);
    DoorLeafMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorLeafMesh->SetGenerateOverlapEvents(false);
    DoorLeafMesh->SetCanEverAffectNavigation(false);
    DoorLeftFrameCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorLeftFrameCollision"));
    DoorLeftFrameCollision->SetupAttachment(Collision);
    DoorLeftFrameCollision->SetBoxExtent(FVector(30.0f, 12.0f, 110.0f));
    DoorLeftFrameCollision->SetRelativeLocation(FVector(-90.0f, 0.0f, 0.0f));
    DoorLeftFrameCollision->SetCollisionProfileName(TEXT("BlockAll"));
    DoorLeftFrameCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorRightFrameCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorRightFrameCollision"));
    DoorRightFrameCollision->SetupAttachment(Collision);
    DoorRightFrameCollision->SetBoxExtent(FVector(30.0f, 12.0f, 110.0f));
    DoorRightFrameCollision->SetRelativeLocation(FVector(90.0f, 0.0f, 0.0f));
    DoorRightFrameCollision->SetCollisionProfileName(TEXT("BlockAll"));
    DoorRightFrameCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorHeaderCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorHeaderCollision"));
    DoorHeaderCollision->SetupAttachment(Collision);
    DoorHeaderCollision->SetBoxExtent(FVector(50.0f, 12.0f, 5.0f));
    DoorHeaderCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    DoorHeaderCollision->SetCollisionProfileName(TEXT("BlockAll"));
    DoorHeaderCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DoorLeafCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DoorLeafCollision"));
    DoorLeafCollision->SetupAttachment(DoorPivot);
    DoorLeafCollision->SetBoxExtent(FVector(50.0f, 6.0f, 100.0f));
    DoorLeafCollision->SetRelativeLocation(FVector(50.0f, 0.0f, 0.0f));
    DoorLeafCollision->SetCollisionProfileName(TEXT("BlockAll"));
    DoorLeafCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    for (int32 StepIndex = 0; StepIndex < 5; ++StepIndex)
    {
        const FName StepName(*FString::Printf(TEXT("StairsStepCollision%d"), StepIndex + 1));
        UBoxComponent* StepCollision = CreateDefaultSubobject<UBoxComponent>(StepName);
        StepCollision->SetupAttachment(Collision);
        StepCollision->SetCollisionProfileName(TEXT("BlockAll"));
        StepCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        StepCollision->SetGenerateOverlapEvents(false);
        StepCollision->SetCanEverAffectNavigation(false);
        StairsStepCollisions.Add(StepCollision);
    }
}

void AKalmalaConstructionActor::InitializeFromServer(const FName InKit, const FString& InConstructionId,
    const FKalmalaCharcoalKilnState& InCharcoalKiln, const FKalmalaSmelterState& InSmelter,
    const FKalmalaGardenPlotState& InGardenPlot, const bool bInDoorOpen,
    const FKalmalaStandingLightState& InStandingLight)
{
    if (HasAuthority())
    {
        ConstructionKit = InKit;
        ConstructionId = InConstructionId.Left(64);
        CharcoalKiln = InKit == TEXT("CharcoalKilnKit") && InCharcoalKiln.IsValid()
            ? InCharcoalKiln : FKalmalaCharcoalKilnState();
        Smelter = InKit == TEXT("SmelterKit") && InSmelter.IsValid()
            ? InSmelter : FKalmalaSmelterState();
        GardenPlot = InKit == TEXT("GardenPlotKit") && InGardenPlot.IsValid()
            ? InGardenPlot : FKalmalaGardenPlotState();
        StandingLight = InKit == TEXT("StandingLightKit") && InStandingLight.IsValid()
            ? InStandingLight : FKalmalaStandingLightState();
        bDoorOpen = InKit == TEXT("TimberDoorKit") && bInDoorOpen;
        SetActorTickEnabled((InKit == TEXT("CharcoalKilnKit")
                && (CharcoalKiln.JobRemainingSeconds > 0.0f
                    || (CharcoalKiln.InputWood > 0 && CharcoalKiln.OutputCoal < FKalmalaCharcoalKilnState::OutputCapacity)))
            || (InKit == TEXT("SmelterKit")
                && (Smelter.JobRemainingSeconds > 0.0f
                    || (!Smelter.InputOreQueue.IsEmpty() && Smelter.InputCoal > 0
                        && Smelter.GetOutputCount() < FKalmalaSmelterState::OutputCapacity)))
            || (InKit == TEXT("GardenPlotKit") && GardenPlot.IsPlanted() && !GardenPlot.IsMature())
            || (InKit == TEXT("StandingLightKit") && StandingLight.RemainingFuelSeconds > 0.0f));
        ApplyConstructionKit();
    }
}

void AKalmalaConstructionActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (ConstructionKit == TEXT("StandingLightKit"))
    {
        const bool bRainBlocked = IsStandingLightExposedToRain();
        if (StandingLightComponent)
            StandingLightComponent->SetVisibility(StandingLight.RemainingFuelSeconds > 0.0f && !bRainBlocked);
        if (!HasAuthority() || ConstructionId.IsEmpty() || StandingLight.RemainingFuelSeconds <= 0.0f || bRainBlocked) return;
        AKalmalaGameMode* LightMode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
        if (!LightMode) return;
        FKalmalaStandingLightState Candidate = StandingLight;
        if (!Candidate.Advance(DeltaSeconds) || !LightMode->PersistStandingLightState(this, Candidate)) return;
        ApplyStandingLightStateFromServer(Candidate);
        return;
    }
    if (!HasAuthority() || ConstructionId.IsEmpty()) return;
    AKalmalaGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Mode) return;
    if (ConstructionKit == TEXT("CharcoalKilnKit"))
    {
        FKalmalaCharcoalKilnState Candidate = CharcoalKiln;
        if (!Candidate.Advance(DeltaSeconds) || !Mode->PersistCharcoalKilnState(this, Candidate)) return;
        ApplyCharcoalKilnStateFromServer(Candidate);
    }
    else if (ConstructionKit == TEXT("SmelterKit"))
    {
        FKalmalaSmelterState Candidate = Smelter;
        if (!Candidate.Advance(DeltaSeconds) || !Mode->PersistSmelterState(this, Candidate)) return;
        ApplySmelterStateFromServer(Candidate);
    }
    else if (ConstructionKit == TEXT("GardenPlotKit") && GardenPlot.IsPlanted() && !GardenPlot.IsMature()
        && Mode->IsTerrainPatchActiveAt(GetActorLocation()))
    {
        FKalmalaGardenPlotState Candidate = GardenPlot;
        if (!Candidate.Advance(DeltaSeconds) || !Mode->PersistGardenPlotState(this, Candidate)) return;
        ApplyGardenPlotStateFromServer(Candidate);
    }
}

bool AKalmalaConstructionActor::ApplyCharcoalKilnStateFromServer(const FKalmalaCharcoalKilnState& State)
{
    if (!HasAuthority() || ConstructionKit != TEXT("CharcoalKilnKit") || !State.IsValid()) return false;
    CharcoalKiln = State;
    SetActorTickEnabled(CharcoalKiln.JobRemainingSeconds > 0.0f
        || (CharcoalKiln.InputWood > 0 && CharcoalKiln.OutputCoal < FKalmalaCharcoalKilnState::OutputCapacity));
    ForceNetUpdate();
    return true;
}

bool AKalmalaConstructionActor::ApplySmelterStateFromServer(const FKalmalaSmelterState& State)
{
    if (!HasAuthority() || ConstructionKit != TEXT("SmelterKit") || !State.IsValid()) return false;
    Smelter = State;
    SetActorTickEnabled(Smelter.JobRemainingSeconds > 0.0f
        || (!Smelter.InputOreQueue.IsEmpty() && Smelter.InputCoal > 0
            && Smelter.GetOutputCount() < FKalmalaSmelterState::OutputCapacity));
    ForceNetUpdate();
    return true;
}

bool AKalmalaConstructionActor::ApplyGardenPlotStateFromServer(const FKalmalaGardenPlotState& State)
{
    if (!HasAuthority() || ConstructionKit != TEXT("GardenPlotKit") || !State.IsValid()) return false;
    GardenPlot = State;
    SetActorTickEnabled(State.IsPlanted() && !State.IsMature());
    ForceNetUpdate();
    return true;
}

bool AKalmalaConstructionActor::ApplyStandingLightStateFromServer(const FKalmalaStandingLightState& State)
{
    if (!HasAuthority() || ConstructionKit != TEXT("StandingLightKit") || !State.IsValid()) return false;
    StandingLight = State;
    SetActorTickEnabled(State.RemainingFuelSeconds > 0.0f);
    UpdateStandingLightPresentation();
    ForceNetUpdate();
    return true;
}

bool AKalmalaConstructionActor::ReplaceWallWithDoorFromServer()
{
    if (!HasAuthority() || ConstructionKit != TEXT("WallKit") || ConstructionId.IsEmpty()) return false;
    ConstructionKit = TEXT("TimberDoorKit");
    bDoorOpen = false;
    ApplyConstructionKit();
    ForceNetUpdate();
    return true;
}

bool AKalmalaConstructionActor::ToggleDoorFromServer(AKalmalaCharacter* Character, FString& Reason)
{
    Reason = TEXT("Server authority required");
    if (!HasAuthority() || ConstructionKit != TEXT("TimberDoorKit") || ConstructionId.IsEmpty()
        || !IsValid(Character) || !Character->HasAuthority()) return false;
    Reason = TEXT("Move within 2 m and face the Timber Door");
    if (!CanUse(Character)) return false;
    const bool bNextOpen = !bDoorOpen;
    if (!bNextOpen && IsPawnInDoorway())
    {
        Reason = TEXT("The Timber Door cannot close while a pawn is in the opening");
        return false;
    }
    AKalmalaGameMode* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    Reason = TEXT("Could not save Timber Door state");
    if (!Mode || !Mode->PersistDoorOpenState(this, bNextOpen)) return false;
    bDoorOpen = bNextOpen;
    ApplyDoorState();
    ForceNetUpdate();
    Reason = bDoorOpen ? TEXT("Timber Door opened") : TEXT("Timber Door closed");
    return true;
}

bool AKalmalaConstructionActor::HasUsablePlantingTrowel(const AKalmalaCharacter* Character) const
{
    if (!IsValid(Character) || Character->GetWorld() != GetWorld()) return false;
    const UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
    if (!Inventory || Inventory->GetActiveItem() != TEXT("PlantingTrowel")
        || !Inventory->GetActiveInstanceGuid().IsValid()
        || Inventory->GetMainHandGuid() != Inventory->GetActiveInstanceGuid()) return false;
    const FGuid ToolGuid = Inventory->GetActiveInstanceGuid();
    const int32 ToolLevel = Character->GetCarriedToolLevel(ToolGuid);
    const int32 MaximumCondition = FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("PlantingTrowel"), ToolLevel);
    const int32 Condition = Character->GetToolDurability(ToolGuid);
    return ToolLevel == 1 && MaximumCondition == 100 && Condition > 0 && Condition <= MaximumCondition;
}

bool AKalmalaConstructionActor::CanUse(const AKalmalaCharacter* Character) const
{
    const bool bIsStorageOrCraftingStation = IsStorageKit(ConstructionKit)
        || IsCraftingStationKit(ConstructionKit);
    const float MaximumUseRange = ConstructionKit == TEXT("TimberDoorKit") || ConstructionKit == TEXT("CampBedKit")
        || ConstructionKit == TEXT("StandingLightKit")
        ? 200.0f : 250.0f;
    if (!IsValid(Character) || !Character->GetController() || Character->GetWorld() != GetWorld() || ConstructionId.IsEmpty()
        || (!bIsStorageOrCraftingStation && ConstructionKit != TEXT("GrindingStoneKit")
            && ConstructionKit != TEXT("GardenPlotKit") && ConstructionKit != TEXT("TimberDoorKit")
            && ConstructionKit != TEXT("CampBedKit") && ConstructionKit != TEXT("StandingLightKit"))
        || Character->GetActorLocation().ContainsNaN() || GetActorLocation().ContainsNaN()
        || FVector::DistSquared(Character->GetActorLocation(), GetActorLocation()) > FMath::Square(MaximumUseRange)) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionUse), false, Character);
    FHitResult Hit;
    const FVector InteractionPoint = ConstructionKit == TEXT("TimberDoorKit")
        ? GetActorTransform().TransformPosition(FVector(-90.0f, 0.0f, 0.0f)) : GetActorLocation();
    return GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), InteractionPoint, ECC_Visibility, Query)
        && Hit.GetActor() == this;
}

void AKalmalaConstructionActor::AdvanceRainWearFromServer(const float DeltaSeconds, const float Precipitation)
{
    if (!HasAuthority() || ConstructionId.IsEmpty() || !GetWorld() || !Collision
        || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f
        || !FMath::IsFinite(Precipitation) || Precipitation <= 0.0f
        || (ConstructionKit != TEXT("FloorKit") && ConstructionKit != TEXT("WallKit")
            && ConstructionKit != TEXT("TimberDoorKit")
            && ConstructionKit != TEXT("TimberBeamKit") && ConstructionKit != TEXT("TimberStairsKit")
            && ConstructionKit != TEXT("CampBedKit") && ConstructionKit != TEXT("StandingLightKit")
            && ConstructionKit != TEXT("WorkbenchKit") && ConstructionKit != TEXT("StorageKit"))) return;
    if (HasShelterRoofOverhead()) return;
    const float NextHealth = FMath::Clamp(Health - RainWearPerSecond * FMath::Clamp(Precipitation, 0.0f, 1.0f) * DeltaSeconds,
        RainHealthFloor, MaximumHealth);
    if (NextHealth != Health)
    {
        Health = NextHealth;
        ForceNetUpdate();
    }
}

bool AKalmalaConstructionActor::HasShelterRoofOverhead() const
{
    if (!GetWorld() || !Collision) return false;
    const FVector Start = Collision->GetComponentLocation() + FVector(0.0f, 0.0f, 60.0f);
    if (Start.ContainsNaN()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionRainProtection), false, this);
    FHitResult Hit;
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + FVector(0.0f, 0.0f, 400.0f),
            ECC_Visibility, Query)
        && IsValid(Hit.GetActor()) && Hit.GetActor()->ActorHasTag(TEXT("KalmalaShelterRoof"));
}

bool AKalmalaConstructionActor::IsStandingLightExposedToRain() const
{
    const AKalmalaWorldGenerationGameState* State = GetWorld()
        ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    return State == nullptr || (State->GetWeatherState().PrecipitationIntensity >= 0.05f
        && !HasShelterRoofOverhead());
}

void AKalmalaConstructionActor::UpdateStandingLightPresentation()
{
    if (!StandingLightComponent) return;
    const bool bLit = ConstructionKit == TEXT("StandingLightKit")
        && StandingLight.RemainingFuelSeconds > 0.0f && !IsStandingLightExposedToRain();
    StandingLightComponent->SetVisibility(bLit);
}

bool AKalmalaConstructionActor::CanInteract_Implementation(AKalmalaCharacter* Character) const
{
    return HasAuthority() && IsValid(Character) && Character->HasAuthority() && CanUse(Character)
        && (ConstructionKit != TEXT("GardenPlotKit") || HasUsablePlantingTrowel(Character));
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
        || KitId == TEXT("FryingPanKit") || KitId == TEXT("SmokeFrameKit")
        || KitId == TEXT("CharcoalKilnKit") || KitId == TEXT("SmelterKit");
}

bool AKalmalaConstructionActor::IsStorageKit(const FName KitId)
{
    return KitId == TEXT("StorageKit");
}

bool AKalmalaConstructionActor::IsStructuralEndpointKit(const FName KitId)
{
    return KitId == TEXT("FloorKit") || KitId == TEXT("WallKit") || KitId == TEXT("TimberDoorKit")
        || KitId == TEXT("RoofKit") || KitId == TEXT("TimberBeamKit") || KitId == TEXT("TimberStairsKit");
}

void AKalmalaConstructionActor::GetStructuralEndpoints(const AKalmalaConstructionActor* Construction,
    TArray<FVector>& OutEndpoints)
{
    if (!IsValid(Construction) || Construction->GetConstructionId().IsEmpty()) return;
    const FName KitId = Construction->GetConstructionKit();
    const FTransform Transform = Construction->GetActorTransform();
    if (KitId == TEXT("TimberStairsKit"))
    {
        OutEndpoints.Add(Transform.TransformPosition(FVector(-100.0f, 0.0f, -50.0f)));
        OutEndpoints.Add(Transform.TransformPosition(FVector(100.0f, 0.0f, 50.0f)));
    }
    else if (KitId == TEXT("TimberBeamKit"))
    {
        OutEndpoints.Add(Transform.TransformPosition(FVector(-100.0f, 0.0f, 0.0f)));
        OutEndpoints.Add(Transform.TransformPosition(FVector(100.0f, 0.0f, 0.0f)));
    }
    else if (KitId == TEXT("FloorKit") || KitId == TEXT("RoofKit"))
    {
        const float HalfSize = KitId == TEXT("FloorKit") ? 120.0f : 132.0f;
        const float Top = GetCollisionExtent(KitId).Z;
        for (const float X : {-HalfSize, HalfSize})
            for (const float Y : {-HalfSize, HalfSize})
                OutEndpoints.Add(Transform.TransformPosition(FVector(X, Y, Top)));
    }
    else if (KitId == TEXT("WallKit") || KitId == TEXT("TimberDoorKit"))
    {
        for (const float X : {-120.0f, 120.0f})
            for (const float Z : {-110.0f, 110.0f})
                OutEndpoints.Add(Transform.TransformPosition(FVector(X, 0.0f, Z)));
    }
}

FVector AKalmalaConstructionActor::GetCollisionExtent(const FName KitId)
{
    if (KitId == TEXT("FloorKit")) return FVector(120, 120, 12);
    if (KitId == TEXT("WallKit")) return FVector(120, 12, 110);
    if (KitId == TEXT("TimberDoorKit")) return FVector(120, 12, 110);
    if (KitId == TEXT("TimberBeamKit")) return FVector(100, 7.5f, 7.5f);
    if (KitId == TEXT("TimberStairsKit")) return FVector(100, 50, 50);
    if (KitId == TEXT("CampBedKit")) return FVector(100, 50, 22);
    if (KitId == TEXT("StandingLightKit")) return FVector(35, 35, 75);
    if (KitId == TEXT("RoofKit")) return FVector(132, 132, 16);
    if (KitId == TEXT("CharcoalKilnKit")) return FVector(60, 60, 62);
    if (KitId == TEXT("SmelterKit")) return FVector(60, 60, 82);
    if (KitId == TEXT("SmokeFrameKit")) return FVector(56, 56, 64);
    if (KitId == TEXT("FryingPanKit")) return FVector(64, 40, 12);
    if (KitId == TEXT("ForgeKit")) return FVector(60, 60, 62);
    if (KitId == TEXT("WorkbenchToolRackKit")) return FVector(32, 25, 34);
    if (KitId == TEXT("ForgeAnvilKit")) return FVector(28, 16, 28);
    if (KitId == TEXT("GrindingStoneKit")) return FVector(42, 34, 18);
    if (KitId == TEXT("GardenPlotKit")) return FVector(50, 50, 8);
    return FVector(54, 54, 56);
}

void AKalmalaConstructionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionKit);
    DOREPLIFETIME(AKalmalaConstructionActor, ConstructionId);
    DOREPLIFETIME(AKalmalaConstructionActor, Health);
    DOREPLIFETIME(AKalmalaConstructionActor, CharcoalKiln);
    DOREPLIFETIME(AKalmalaConstructionActor, Smelter);
    DOREPLIFETIME(AKalmalaConstructionActor, GardenPlot);
    DOREPLIFETIME(AKalmalaConstructionActor, StandingLight);
    DOREPLIFETIME(AKalmalaConstructionActor, bDoorOpen);
}

void AKalmalaConstructionActor::OnRep_DoorState()
{
    ApplyDoorState();
}

void AKalmalaConstructionActor::OnRep_StandingLightState()
{
    SetActorTickEnabled(StandingLight.RemainingFuelSeconds > 0.0f);
    UpdateStandingLightPresentation();
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
    ApplyDoorState();
    UpdateStandingLightPresentation();
}

void AKalmalaConstructionActor::ApplyDoorState()
{
    if (!Collision || !DoorPivot || !DoorLeafMesh) return;
    for (const TObjectPtr<UBoxComponent>& Step : StairsStepCollisions)
        if (Step) Step->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (ConstructionKit == TEXT("TimberDoorKit"))
    {
        Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
        Tags.Remove(TEXT("KalmalaShelterWindbreak"));
        if (!bDoorOpen) Tags.AddUnique(TEXT("KalmalaShelterWindbreak"));
        DoorPivot->SetRelativeRotation(FRotator(0.0f, bDoorOpen ? -90.0f : 0.0f, 0.0f));
        for (UBoxComponent* Frame : {DoorLeftFrameCollision.Get(), DoorRightFrameCollision.Get(), DoorHeaderCollision.Get(), DoorLeafCollision.Get()})
            if (Frame) Frame->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        return;
    }
    if (ConstructionKit == TEXT("TimberStairsKit"))
    {
        Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
        Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for (int32 StepIndex = 0; StepIndex < StairsStepCollisions.Num(); ++StepIndex)
        {
            UBoxComponent* Step = StairsStepCollisions[StepIndex];
            if (!Step) continue;
            const float StepTop = static_cast<float>((StepIndex + 1) * 20);
            const float HalfHeight = StepTop * 0.5f;
            Step->SetBoxExtent(FVector(20.0f, 50.0f, HalfHeight));
            Step->SetRelativeLocation(FVector(-80.0f + static_cast<float>(StepIndex * 40),
                0.0f, HalfHeight - 50.0f));
            Step->SetCollisionProfileName(TEXT("BlockAll"));
            Step->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
        return;
    }
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    DoorPivot->SetRelativeRotation(FRotator::ZeroRotator);
    for (UBoxComponent* Frame : {DoorLeftFrameCollision.Get(), DoorRightFrameCollision.Get(), DoorHeaderCollision.Get(), DoorLeafCollision.Get()})
        if (Frame) Frame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AKalmalaConstructionActor::IsPawnInDoorway() const
{
    if (!HasAuthority() || !GetWorld() || ConstructionKit != TEXT("TimberDoorKit")) return true;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TimberDoorClose), false, this);
    TArray<FOverlapResult> Overlaps;
    GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), GetActorQuat(), ECC_Pawn,
        FCollisionShape::MakeBox(FVector(50.0f, 12.0f, 100.0f)), Query);
    for (const FOverlapResult& Overlap : Overlaps)
        if (Cast<APawn>(Overlap.GetActor()) != nullptr) return true;
    return false;
}

void AKalmalaConstructionActor::BuildPiecePresentation()
{
    if (!PieceMesh) return;
    TArray<FVector> Vertices; TArray<int32> Triangles;
    // Placement and saved transforms already locate the collision centre.
    // Derive the visible solid from that same footprint on both peers.
    if (ConstructionKit == TEXT("StandingLightKit"))
    {
        const FVector Crown(0.0f, 0.0f, 26.0f);
        AddBeam(Vertices, Triangles, Crown, FVector(25.0f, 0.0f, -72.0f), 5.0f);
        AddBeam(Vertices, Triangles, Crown, FVector(-12.5f, 21.7f, -72.0f), 5.0f);
        AddBeam(Vertices, Triangles, Crown, FVector(-12.5f, -21.7f, -72.0f), 5.0f);
        AddBox(Vertices, Triangles, FVector(0.0f, 0.0f, 35.0f), FVector(17.0f, 17.0f, 4.0f));
        AddBox(Vertices, Triangles, FVector(-14.0f, 0.0f, 49.0f), FVector(3.0f, 17.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(14.0f, 0.0f, 49.0f), FVector(3.0f, 17.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(0.0f, -14.0f, 49.0f), FVector(11.0f, 3.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(0.0f, 14.0f, 49.0f), FVector(11.0f, 3.0f, 6.0f));
    }
    else if (ConstructionKit == TEXT("GardenPlotKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -3), FVector(50, 50, 5));
        for (const float X : {-47.0f, 47.0f})
            AddBox(Vertices, Triangles, FVector(X, 0, 1), FVector(3, 50, 4));
        for (const float Y : {-47.0f, 47.0f})
            AddBox(Vertices, Triangles, FVector(0, Y, 1), FVector(44, 3, 4));
    }
    else if (ConstructionKit == TEXT("TimberStairsKit"))
    {
        for (int32 StepIndex = 0; StepIndex < 5; ++StepIndex)
        {
            const float StepTop = static_cast<float>((StepIndex + 1) * 20);
            const float HalfHeight = StepTop * 0.5f;
            AddBox(Vertices, Triangles,
                FVector(-80.0f + static_cast<float>(StepIndex * 40), 0.0f, HalfHeight - 50.0f),
                FVector(20.0f, 50.0f, HalfHeight));
        }
    }
    else if (ConstructionKit == TEXT("CampBedKit"))
    {
        for (const float X : {-84.0f, 84.0f})
            for (const float Y : {-38.0f, 38.0f})
                AddBox(Vertices, Triangles, FVector(X, Y, -9.0f), FVector(8.0f, 7.0f, 13.0f));
        AddBox(Vertices, Triangles, FVector(0, -41.0f, 4.0f), FVector(92.0f, 5.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(0, 41.0f, 4.0f), FVector(92.0f, 5.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(-94.0f, 0, 4.0f), FVector(5.0f, 39.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(94.0f, 0, 4.0f), FVector(5.0f, 39.0f, 6.0f));
        AddBox(Vertices, Triangles, FVector(0, 0, 13.0f), FVector(84.0f, 34.0f, 5.0f));
    }
    else if (ConstructionKit == TEXT("TimberDoorKit"))
    {
        AddBox(Vertices, Triangles, FVector(-90, 0, 0), FVector(30, 12, 110));
        AddBox(Vertices, Triangles, FVector(90, 0, 0), FVector(30, 12, 110));
        AddBox(Vertices, Triangles, FVector(-55, 0, 0), FVector(5, 16, 110));
        AddBox(Vertices, Triangles, FVector(55, 0, 0), FVector(5, 16, 110));
        AddBox(Vertices, Triangles, FVector(0, 0, 105), FVector(50, 16, 5));
    }
    else if (ConstructionKit == TEXT("TimberBeamKit"))
        AddBox(Vertices, Triangles, FVector::ZeroVector, GetCollisionExtent(ConstructionKit));
    else if (ConstructionKit == TEXT("FloorKit") || IsShelterKit(ConstructionKit))
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
    else if (ConstructionKit == TEXT("CharcoalKilnKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -52), FVector(60, 60, 10));
        AddBox(Vertices, Triangles, FVector(-12, 0, -6), FVector(48, 54, 36));
        AddBox(Vertices, Triangles, FVector(34, -40, -12), FVector(22, 14, 42));
        AddBox(Vertices, Triangles, FVector(34, 40, -12), FVector(22, 14, 42));
        AddBox(Vertices, Triangles, FVector(34, 0, 34), FVector(22, 26, 10));
        AddBox(Vertices, Triangles, FVector(40, 0, 48), FVector(16, 18, 8));
        AddBox(Vertices, Triangles, FVector(-20, 0, 34), FVector(30, 34, 8));
    }
    else if (ConstructionKit == TEXT("SmelterKit"))
    {
        AddBox(Vertices, Triangles, FVector(0, 0, -70), FVector(60, 60, 10));
        AddBox(Vertices, Triangles, FVector(0, 0, -30), FVector(48, 48, 30));
        AddBox(Vertices, Triangles, FVector(0, 0, 8), FVector(38, 40, 8));
        AddBox(Vertices, Triangles, FVector(-10, 0, 45), FVector(20, 24, 37));
        AddBox(Vertices, Triangles, FVector(-10, 0, 77), FVector(25, 29, 5));
        AddBox(Vertices, Triangles, FVector(43, -28, -10), FVector(17, 8, 8));
    }
    else
        AddBox(Vertices, Triangles, FVector::ZeroVector, GetCollisionExtent(ConstructionKit));
    TArray<FVector> Normals; Normals.Init(FVector::UpVector, Vertices.Num());
    TArray<FVector2D> UVs; UVs.Init(FVector2D::ZeroVector, Vertices.Num());
    TArray<FLinearColor> Colours; Colours.Init(FLinearColor::White, Vertices.Num());
    TArray<FProcMeshTangent> Tangents; Tangents.Init(FProcMeshTangent(), Vertices.Num());
    PieceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colours, Tangents, false);

    if (DoorLeafMesh)
    {
        DoorLeafMesh->ClearAllMeshSections();
        TArray<FVector> LeafVertices;
        TArray<int32> LeafTriangles;
        if (ConstructionKit == TEXT("TimberDoorKit"))
        {
            AddBox(LeafVertices, LeafTriangles, FVector(50, 0, 0), FVector(50, 5, 100));
            TArray<FVector> LeafNormals; LeafNormals.Init(FVector::UpVector, LeafVertices.Num());
            TArray<FVector2D> LeafUVs; LeafUVs.Init(FVector2D::ZeroVector, LeafVertices.Num());
            TArray<FLinearColor> LeafColours; LeafColours.Init(FLinearColor::White, LeafVertices.Num());
            TArray<FProcMeshTangent> LeafTangents; LeafTangents.Init(FProcMeshTangent(), LeafVertices.Num());
            DoorLeafMesh->CreateMeshSection_LinearColor(0, LeafVertices, LeafTriangles, LeafNormals,
                LeafUVs, LeafColours, LeafTangents, false);
        }
    }
}
