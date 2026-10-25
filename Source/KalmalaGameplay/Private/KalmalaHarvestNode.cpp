#include "KalmalaHarvestNode.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaForestLayout.h"
#include "KalmalaForestResourceContract.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationSaveGame.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9SourceLootContract.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"
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
        if (PresentationId == TEXT("peat-amber-seam")) return FLinearColor(0.78f, 0.43f, 0.12f);
        if (PresentationId == TEXT("tundra-salt-crystals")) return FLinearColor(0.72f, 0.84f, 0.88f);
        if (PresentationId == TEXT("birch-bark-bundle")) return FLinearColor(0.52f, 0.34f, 0.18f);
        if (PresentationId == TEXT("aspen-trunk-harvest")) return FLinearColor(0.70f, 0.65f, 0.52f);
        if (PresentationId == TEXT("reed-cluster")) return FLinearColor(0.35f, 0.64f, 0.24f);
        if (PresentationId == TEXT("resinwood-bundle")) return FLinearColor(0.30f, 0.20f, 0.10f);
        if (PresentationId == TEXT("bog-iron-vein")) return FLinearColor(0.30f, 0.24f, 0.20f);
        if (PresentationId == TEXT("frostmoss-clump")) return FLinearColor(0.54f, 0.76f, 0.72f);
        if (PresentationId == TEXT("lingonberry-shrub-harvest")) return FLinearColor(0.66f, 0.08f, 0.12f);
        if (PresentationId == TEXT("chanterelle-patch-harvest")) return FLinearColor(0.88f, 0.50f, 0.12f);
        if (PresentationId == TEXT("carrot-patch-harvest")) return FLinearColor(0.22f, 0.52f, 0.16f);
        if (PresentationId == TEXT("potato-patch-harvest")) return FLinearColor(0.42f, 0.50f, 0.25f);
        if (PresentationId == TEXT("onion-patch-harvest")) return FLinearColor(0.29f, 0.43f, 0.20f);
        if (PresentationId == TEXT("slate-vein")) return FLinearColor(0.30f, 0.36f, 0.43f);
        if (PresentationId == TEXT("copper-ore-vein")) return FLinearColor(0.70f, 0.32f, 0.13f);
        if (PresentationId == TEXT("tin-shore-gravel")) return FLinearColor(0.68f, 0.74f, 0.75f);
        return FLinearColor::Transparent;
    }

    bool IsRockSource(const FName PresentationId)
    {
        return PresentationId == TEXT("bog-iron-vein") || PresentationId == TEXT("slate-vein")
            || PresentationId == TEXT("peat-amber-seam") || PresentationId == TEXT("tundra-salt-crystals")
            || PresentationId == TEXT("copper-ore-vein") || PresentationId == TEXT("tin-shore-gravel");
    }
}

AKalmalaHarvestNode::AKalmalaHarvestNode()
{
    CreateDefaultSubobject<UKalmalaResistanceComponent>(TEXT("Resistances"));
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
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

bool AKalmalaHarvestNode::InitializeForestServer(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D& PatchCenter, const int32 CandidateIndex)
{
    if (!HasAuthority() || !PersistentSpawnId.IsEmpty() || bHarvested) return false;
    const TArray<FKalmalaForestTreeDescriptor> Trees = FKalmalaForestLayout::Generate(Config, PatchCenter);
    const FKalmalaForestTreeDescriptor* Tree = Trees.FindByPredicate(
        [CandidateIndex](const FKalmalaForestTreeDescriptor& Candidate) { return Candidate.CandidateIndex == CandidateIndex; });
    return Tree != nullptr && InitializeGeneratedForestServer(*Tree);
}

bool AKalmalaHarvestNode::InitializeGeneratedForestServer(const FKalmalaForestTreeDescriptor& Tree)
{
    if (!HasAuthority() || !PersistentSpawnId.IsEmpty() || bHarvested
        || Tree.PersistentSpawnId.IsEmpty() || Tree.CandidateIndex < 0
        || Tree.Location.ContainsNaN() || !FMath::IsFinite(Tree.Yaw)
        || !FMath::IsFinite(Tree.TrunkRadius) || Tree.TrunkRadius <= 0
        || !FMath::IsFinite(Tree.TrunkHeight) || Tree.TrunkHeight <= 0
        || !FMath::IsFinite(Tree.CanopyRadius) || Tree.CanopyRadius <= 0
        || !FMath::IsFinite(Tree.CanopyHeight) || Tree.CanopyHeight <= 0) return false;
    const FName SourceId = FKalmalaForestResourceContract::GetSourceId(Tree.TreeKind);
    if (!FKalmalaBiomeContentContract::IsValidGatheringSourceId(SourceId)) return false;
    // Forest candidates may be reconstructed after actor spawning. Replicate the
    // authoritative transform as well as their shape, including late relevance.
    SetReplicateMovement(true);
    // Cosmetics are suppressed for every peer receiving the owning terrain patch.
    // Keep its replacement visible to that same audience, including distant peers.
    bNetUseOwnerRelevancy = true;
    SetActorLocation(Tree.Location);
    SetActorRotation(FRotator(0.0f, Tree.Yaw, 0.0f));
    PersistentSpawnId = Tree.PersistentSpawnId;
    GatheringSourceId = SourceId;
    ForestDimensions = FVector4(Tree.TrunkRadius, Tree.TrunkHeight, Tree.CanopyRadius, Tree.CanopyHeight);
    BuildSourcePresentation();
    ForceNetUpdate();
    return true;
}

void AKalmalaHarvestNode::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority() && PersistentSpawnId.IsEmpty() && !bHarvested
        && Spawn.Kind == EKalmalaWorldPopulationKind::HarvestNode && !Spawn.Location.ContainsNaN()
        && (Spawn.ContentId.IsNone() || FKalmalaGemstoneContract::IsGemSource(Spawn.ContentId) || FKalmalaBiomeContentContract::IsValidGatheringSourceId(Spawn.ContentId)))
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
        GatheringSourceId = Spawn.ContentId;
        if (IsGemstone())
        {
            const auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
            const auto* State = Mode ? Mode->ReadGemstoneState(PersistentSpawnId) : nullptr;
            if (State && State->SourceId == GatheringSourceId)
            { GemHits = State->Hits; GemOutcome = State->Outcome; bHarvested = GemOutcome == 3; }
        }
        BuildSourcePresentation();
        ApplyHarvestedState();
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
    return (!IsGemstone() || GemOutcome == 2) && (!IsTreeResource() || IsWoodDrop()) && IsValid(Interactor) && Interactor->HasAuthority() && Interactor->GetWorld() == GetWorld()
        && !PersistentSpawnId.IsEmpty()
        && IsHarvestAllowed(HasAuthority(), bHarvested, Interactor->GetActorLocation(), GetHarvestInteractionLocation());
}

bool AKalmalaHarvestNode::IsHarvestAllowed(const bool bServerAuthority, const bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, const float MaximumDistance)
{
    return bServerAuthority && !bAlreadyHarvested && FMath::IsFinite(MaximumDistance) && MaximumDistance > 0.0f
        && !InteractorLocation.ContainsNaN() && !NodeLocation.ContainsNaN()
        && FVector::DistSquared(InteractorLocation, NodeLocation) <= FMath::Square(MaximumDistance);
}

FName AKalmalaHarvestNode::GetGatheringPresentationId() const
{
    return IsGemstone() ? GatheringSourceId : FKalmalaBiomeContentContract::GetGatheringPresentationId(GatheringSourceId);
}

FString AKalmalaHarvestNode::GetM9ResourceDepletionId() const
{
    // Individually generated forest trees use the world sparse harvested-ID ledger,
    // rather than the session-only M9 resource overlay.
    if (PersistentSpawnId.StartsWith(TEXT("forest:v1:"))) return FString();
    return FKalmalaM9SourceLootContract::BuildResourceDepletionId(GatheringSourceId, PersistentSpawnId);
}

void AKalmalaHarvestNode::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (IsGemstone()) { ClaimGemFromServer(Interactor); return; }
    if (IsTreeResource())
    {
        if (!IsWoodDrop() || !CanInteract_Implementation(Interactor)) return;
        FName RewardItemId;
        int32 RewardQuantity = 0;
        if (!GetPickupInfo(RewardItemId, RewardQuantity)) return;
        auto* Inventory = Interactor->GetInventoryComponent();
        auto* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
        if (!Inventory || !GameMode) return;
        const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
        TArray<FKalmalaInventoryStack> After;
        FString Reason;
        if (!UKalmalaInventoryComponent::BuildGrant(Before, RewardItemId, RewardQuantity, After, Reason))
        {
            if (IsWoodDrop() && Reason == TEXT("Pack output capacity reached"))
            {
                if (UKalmalaCraftingComponent* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
                    Crafting->PublishInteractionResultFromServer(TEXT("Inventory is full"), false);
            }
            return;
        }
        if (!Inventory->CanFitContents(After, Inventory->CountPackedTools(Interactor->GetCarriedToolInventory())))
        {
            if (IsWoodDrop())
            {
                if (UKalmalaCraftingComponent* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
                    Crafting->PublishInteractionResultFromServer(TEXT("Inventory is full"), false);
            }
            return;
        }
        if (!Inventory->TryCommitStacksFromServer(Before, After)) return;
        if (!GameMode->PersistPlayerStateFromServer(Interactor))
        {
            Inventory->TryCommitStacksFromServer(After, Before);
            return;
        }
        CommitHarvestedStateFromServer();
        return;
    }
    if (IsHandGatherablePatch())
    {
        if (!HasAuthority() || PersistentSpawnId.IsEmpty() || !IsValid(Interactor) || Interactor->GetHealth() <= 0.0f
            || !CanInteract_Implementation(Interactor)) return;
        auto* Inventory = Interactor->GetInventoryComponent();
        auto* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
        if (!Inventory || !GameMode) return;

        const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
        TArray<FKalmalaInventoryStack> AfterReward;
        TArray<FKalmalaInventoryStack> AfterSeed;
        FString Reason;
        const auto PublishFullPack = [Interactor]()
        {
            if (UKalmalaCraftingComponent* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
                Crafting->PublishInteractionResultFromServer(TEXT("Inventory is full"), false);
        };
        const FName RewardItemId = GetHarvestItemId();
        FName SeedItemId = NAME_None;
        if (GatheringSourceId == TEXT("meadows-wild-carrot-patch")) SeedItemId = FName(TEXT("CarrotSeed"));
        else if (GatheringSourceId == TEXT("lakes-wild-rutabaga-patch")) SeedItemId = FName(TEXT("RutabagaSeed"));
        else if (GatheringSourceId == TEXT("elderwood-wild-onion-patch")) SeedItemId = FName(TEXT("OnionSeed"));
        const int32 RewardQuantity = GatheringSourceId == TEXT("meadows-wild-lingonberry-shrub") ? 3 : 2;
        if (RewardItemId.IsNone()
            || !UKalmalaInventoryComponent::BuildGrant(Before, RewardItemId, RewardQuantity, AfterReward, Reason))
        {
            if (Reason == TEXT("Pack output capacity reached")) PublishFullPack();
            return;
        }
        const TArray<FKalmalaInventoryStack>* FinalStacks = &AfterReward;
        if (!SeedItemId.IsNone())
        {
            if (!UKalmalaInventoryComponent::BuildGrant(AfterReward, SeedItemId, 1, AfterSeed, Reason))
            {
                if (Reason == TEXT("Pack output capacity reached")) PublishFullPack();
                return;
            }
            FinalStacks = &AfterSeed;
        }
        if (!Inventory->CanFitContents(*FinalStacks, Inventory->CountPackedTools(Interactor->GetCarriedToolInventory())))
        {
            PublishFullPack();
            return;
        }
        if (!Inventory->TryCommitStacksFromServer(Before, *FinalStacks)) return;
        if (!GameMode->PersistPlayerStateFromServer(Interactor))
        {
            if (!Inventory->TryCommitStacksFromServer(*FinalStacks, Before))
                UE_LOG(LogTemp, Error, TEXT("Failed to roll back inventory after player persistence rejected a wild crop harvest."));
            return;
        }
        CommitHarvestedStateFromServer();
        return;
    }
    // Generated biome sources must pass through the server trace/tool transaction.
    if (!GatheringSourceId.IsNone()) return;
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

bool AKalmalaHarvestNode::InteractWithToolIntentFromServer(AKalmalaCharacter* Interactor,
    const float TraceDistance, const float MaximumRange, const FName ClientItemId, const uint8 ClientAction)
{
    if (IsGemstone())
    {
        if (!FMath::IsFinite(TraceDistance) || TraceDistance < 0.0f || !FMath::IsFinite(MaximumRange)
            || MaximumRange <= 0.0f || TraceDistance > MaximumRange) return false;
        return ClaimGemFromServer(Interactor);
    }
    if (IsTreeResource())
    {
        if (!FMath::IsFinite(TraceDistance) || !FMath::IsFinite(MaximumRange)
            || TraceDistance < 0.0f || MaximumRange <= 0.0f || TraceDistance > MaximumRange
            || !CanInteract_Implementation(Interactor)) return false;
        Interact_Implementation(Interactor);
        return bHarvested;
    }
    if (!HasAuthority() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || Interactor->GetWorld() != GetWorld()
        || !FKalmalaBiomeContentContract::IsValidGatheringSourceId(GatheringSourceId)
        || !IsHarvestAllowed(true, bHarvested, Interactor->GetActorLocation(), GetActorLocation())) return false;

    if (IsHandGatherablePatch())
    {
        if (!FMath::IsFinite(TraceDistance) || !FMath::IsFinite(MaximumRange)
            || TraceDistance < 0.0f || MaximumRange <= 0.0f || TraceDistance > MaximumRange) return false;
        Interact_Implementation(Interactor);
        return bHarvested;
    }

    return Interactor->CommitToolHarvestFromServer(this, TraceDistance, MaximumRange, ClientItemId, ClientAction);
}

bool AKalmalaHarvestNode::IsTreeResource() const
{
    return GatheringSourceId == TEXT("meadows-birch-bark") || GatheringSourceId == TEXT("elderwood-resinwood")
        || GatheringSourceId == TEXT("meadows-aspen-trunk")
        || GatheringSourceId == TEXT("meadows-birch-trunk") || GatheringSourceId == TEXT("elderwood-ironheart-trunk");
}

bool AKalmalaHarvestNode::AdvanceTreeHit(const uint8 Stage, const float Health, const float Damage,
    uint8& OutStage, float& OutHealth)
{
    OutStage = Stage;
    OutHealth = Health;
    if (Stage >= 2 || !FMath::IsFinite(Health) || Health <= 0.0f
        || !FMath::IsFinite(Damage) || Damage <= 0.0f || Damage > 100000.0f) return false;
    OutHealth = FMath::Max(0.0f, Health - Damage);
    if (OutHealth == 0.0f)
    {
        OutStage = Stage == 0 ? 3 : 2;
        OutHealth = OutStage == 3 ? 60.0f : 0.0f; // Excess damage never skips the trunk stage.
    }
    return true;
}

bool AKalmalaHarvestNode::ApplyAxeDamageFromServer(AKalmalaCharacter* Interactor,
    const FKalmalaDamageChannels& Damage)
{
    if (!HasAuthority() || !CanReceiveAxeAttack() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || Interactor->GetWorld() != GetWorld() || Interactor->GetHealth() <= 1.0f || PersistentSpawnId.IsEmpty()
        || !IsWithinAxeRange(Interactor->GetActorLocation())) return false;
    const auto* Inventory = Interactor->GetInventoryComponent();
    const FName ItemId = Inventory ? Inventory->GetActiveItem() : NAME_None;
    const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(ItemId);
    FKalmalaToolServerSelection Selection;
    if (!Definition || Definition->Action != EKalmalaToolAction::Woodcutting
        || !FKalmalaToolLifecycleContract::BuildServerSelection(GatheringSourceId, Selection)
        || !FKalmalaToolLifecycleContract::IsToolSuitableForSelection(
            *Definition, Interactor->GetCarriedToolLevel(ItemId), Selection)) return false;
    int32* Condition = Interactor->FindToolDurabilityFromServer(ItemId);
    const int32 MaximumCondition = FKalmalaToolLifecycleContract::GetMaxDurability(ItemId, Interactor->GetCarriedToolLevel(ItemId));
    if (!Condition || *Condition <= 0 || *Condition > MaximumCondition) return false;
    float ResolvedDamage = 0.0f;
    const auto* Resistances = FindComponentByClass<UKalmalaResistanceComponent>();
    uint8 NextStage;
    float NextHealth;
    if (!Resistances || !Resistances->ResolveFromServer(Damage, ResolvedDamage)
        || !AdvanceTreeHit(TreeStage, TreeHealth, ResolvedDamage, NextStage, NextHealth)) return false;
    auto* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    if (!GameMode) return false;
    --*Condition;
    if (!GameMode->PersistPlayerStateFromServer(Interactor)) { ++*Condition; return false; }
    TreeHealth = NextHealth;
    if (TreeStage != NextStage)
    {
        TreeStage = NextStage;
        if (IsTreeFalling())
        {
            // Begin only after the felling hit's condition write succeeds.
            const auto* State = GetWorld()->GetGameState();
            TreeFallStartedAt = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
            GetWorldTimerManager().SetTimer(TreeFallTimer, this,
                &AKalmalaHarvestNode::SettleTreeFromServer, TreeFallSeconds, false);
        }
        BuildSourcePresentation();
    }
    Interactor->ForceNetUpdate();
    ForceNetUpdate();
    return true;
}

bool AKalmalaHarvestNode::IsWithinAxeRange(const FVector& InteractorLocation, const float MaximumRange) const
{
    if (!CanReceiveAxeAttack() || InteractorLocation.ContainsNaN()
        || !FMath::IsFinite(MaximumRange) || MaximumRange <= 0.0f) return false;
    if (!IsFelledTrunk())
        return FVector::DistSquared(InteractorLocation, GetActorLocation()) <= FMath::Square(MaximumRange);

    // Use the same tapered timber and cut faces as the visibility collision.
    // This geometry is reconstructed from server-owned dimensions and phase;
    // neither a client target point nor a collision query supplies the range.
    const FProcMeshSection* Section = SourceMesh ? SourceMesh->GetProcMeshSection(0) : nullptr;
    if (!Section) return false;
    const FTransform Transform = SourceMesh->GetComponentTransform();
    for (int32 Index = 0; Index + 2 < Section->ProcIndexBuffer.Num(); Index += 3)
    {
        const FVector A = Transform.TransformPosition(Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index]].Position);
        const FVector B = Transform.TransformPosition(Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index + 1]].Position);
        const FVector C = Transform.TransformPosition(Section->ProcVertexBuffer[Section->ProcIndexBuffer[Index + 2]].Position);
        const FVector Closest = FMath::ClosestPointOnTriangleToPoint(InteractorLocation, A, B, C);
        if (FVector::DistSquared(InteractorLocation, Closest) <= FMath::Square(MaximumRange)) return true;
    }
    return false;
}

void AKalmalaHarvestNode::OnRep_TreeStage() { BuildSourcePresentation(); }

void AKalmalaHarvestNode::SettleTreeFromServer()
{
    if (!HasAuthority() || !IsTreeFalling() || bHarvested) return;
    TreeStage = 1;
    BuildSourcePresentation();
    ForceNetUpdate();
}

float AKalmalaHarvestNode::SampleTreeFallProgress(const double StartedAt, const double ServerTime)
{
    if (!FMath::IsFinite(StartedAt) || !FMath::IsFinite(ServerTime)) return 0.0f;
    return static_cast<float>(FMath::Clamp((ServerTime - StartedAt) / TreeFallSeconds, 0.0, 1.0));
}

float AKalmalaHarvestNode::GetTreeFallProgress() const
{
    if (IsFelledTrunk() || IsWoodDrop()) return 1.0f;
    if (!IsTreeFalling() || !GetWorld()) return 0.0f;
    const auto* State = GetWorld()->GetGameState();
    return SampleTreeFallProgress(TreeFallStartedAt,
        State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds());
}

FTransform AKalmalaHarvestNode::SampleTreeFallPose(const float Progress, const float TrunkRadius)
{
    const float Fraction = FMath::IsFinite(Progress) ? FMath::Clamp(Progress, 0.0f, 1.0f) : 0.0f;
    const float Radius = FMath::IsFinite(TrunkRadius) ? FMath::Max(0.0f, TrunkRadius) : 0.0f;
    // Accelerate around the original root towards actor-local +X. Lift the hinge
    // by the projected radius so the settled timber rests on the root's plane.
    const float Angle = Fraction * Fraction * HALF_PI;
    return FTransform(FQuat(FVector::YAxisVector, Angle), FVector(0, 0, Radius * FMath::Sin(Angle)));
}

FVector AKalmalaHarvestNode::GetHarvestInteractionLocation() const
{
    const float Height = ForestDimensions.Y > 0 ? ForestDimensions.Y : 280.0f;
    return IsTreeResource() && IsWoodDrop()
        ? GetActorTransform().TransformPosition(FVector(Height * 0.5f, 0, 0)) : GetActorLocation();
}

void AKalmalaHarvestNode::UpdateTreePresentationPose()
{
    if (!SourceMesh) return;
    const float Radius = ForestDimensions.X > 0 ? ForestDimensions.X : 20.0f;
    if (IsTreeResource() && IsWoodDrop())
    {
        const float Height = ForestDimensions.Y > 0 ? ForestDimensions.Y : 280.0f;
        SourceMesh->SetRelativeTransform(FTransform(FVector(Height * 0.5f, 0, 0)));
        return;
    }
    SourceMesh->SetRelativeTransform(IsTreeFalling() || IsFelledTrunk()
        ? SampleTreeFallPose(GetTreeFallProgress(), Radius) : FTransform::Identity);
}

void AKalmalaHarvestNode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Cosmetic sampling only: phase changes and settlement stay in the server timer.
    UpdateTreePresentationPose();
}

void AKalmalaHarvestNode::CommitHarvestedStateFromServer()
{
    if (!HasAuthority() || bHarvested || PersistentSpawnId.IsEmpty()) return;
    bHarvested = true;
    ApplyHarvestedState();
    const FString M9ResourceId = GetM9ResourceDepletionId();
    if (!M9ResourceId.IsEmpty())
    {
        OnM9ResourceDepleted.Broadcast(M9ResourceId);
    }
    else
    {
        OnHarvested.Broadcast(PersistentSpawnId);
    }
    ForceNetUpdate();
}

FName AKalmalaHarvestNode::GetHarvestItemId() const
{
    if (PersistentSpawnId.IsEmpty()) return NAME_None;
    if (IsGemstone()) return FKalmalaGemstoneContract::ItemForSource(GatheringSourceId);
    if (IsHandGatherablePatch())
    {
        if (GatheringSourceId == TEXT("meadows-wild-lingonberry-shrub")) return FName(TEXT("Lingonberry"));
        if (GatheringSourceId == TEXT("elderwood-wild-chanterelle-patch")) return FName(TEXT("ForestChanterelle"));
        if (GatheringSourceId == TEXT("meadows-wild-potato-patch")) return FName(TEXT("Potato"));
        if (GatheringSourceId == TEXT("lakes-wild-rutabaga-patch")) return FName(TEXT("Rutabaga"));
        if (GatheringSourceId == TEXT("elderwood-wild-onion-patch")) return FName(TEXT("Onion"));
        return FName(TEXT("Carrot"));
    }
    FKalmalaToolServerSelection Selection;
    if (FKalmalaToolLifecycleContract::BuildServerSelection(GatheringSourceId, Selection))
        return Selection.RewardItemId;
    static const FName Materials[] = { TEXT("Wood"), TEXT("Stone") };
    return Materials[FCrc::StrCrc32(*PersistentSpawnId) % UE_ARRAY_COUNT(Materials)];
}

bool AKalmalaHarvestNode::GetPickupInfo(FName& OutItemId, int32& OutQuantity) const
{
    OutItemId = NAME_None;
    OutQuantity = 0;
    if (bHarvested || PersistentSpawnId.IsEmpty()
        || (IsGemstone() ? GemOutcome != 2 : IsTreeResource() ? !IsWoodDrop() : !GatheringSourceId.IsNone())) return false;
    int32 Quantity = 1;
    FName ItemId = NAME_None;
    if (IsTreeResource())
    {
        FKalmalaToolServerSelection Selection;
        if (!FKalmalaToolLifecycleContract::BuildServerSelection(GatheringSourceId, Selection)) return false;
        ItemId = Selection.RewardItemId;
        Quantity = Selection.RewardQuantity;
        if (FKalmalaM9SourceLootContract::IsM9Source(GatheringSourceId))
        {
            const auto* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
            if (!WorldState || !FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
                WorldState->GetWorldGenerationConfig(), GatheringSourceId, PersistentSpawnId, Quantity)) return false;
        }
    }
    else ItemId = GetHarvestItemId();
    OutItemId = ItemId;
    OutQuantity = Quantity;
    return !OutItemId.IsNone();
}

bool AKalmalaHarvestNode::TryAutomaticPickupFromServer(AKalmalaCharacter* Interactor)
{
    FName ItemId;
    int32 Quantity;
    if (!HasAuthority() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || !Interactor->IsAutomaticPickupEnabled() || Interactor->GetHealth() <= 0.0f
        || Interactor->GetWorld() != GetWorld()
        || !IsHarvestAllowed(true, bHarvested, Interactor->GetActorLocation(),
            GetHarvestInteractionLocation(), AutomaticPickupRadius) || !GetPickupInfo(ItemId, Quantity)) return false;
    // Quietly leave a drop intact when it cannot fit; do not repeat full-pack notifications.
    auto* Inventory = Interactor->GetInventoryComponent();
    TArray<FKalmalaInventoryStack> After;
    FString Reason;
    if (!Inventory || !UKalmalaInventoryComponent::BuildGrant(Inventory->GetStacks(), ItemId, Quantity, After, Reason)
        || !Inventory->CanFitContents(After, Inventory->CountPackedTools(Interactor->GetCarriedToolInventory()))) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(AutomaticPickup), true, Interactor);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Interactor->GetActorLocation(),
        GetHarvestInteractionLocation(), ECC_Visibility, Query) && Hit.GetActor() != this) return false;
    Interact_Implementation(Interactor);
    return bHarvested;
}

void AKalmalaHarvestNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaHarvestNode, bHarvested);
    DOREPLIFETIME(AKalmalaHarvestNode, TreeStage);
    DOREPLIFETIME(AKalmalaHarvestNode, GemHits);
    DOREPLIFETIME(AKalmalaHarvestNode, GemOutcome);
    DOREPLIFETIME(AKalmalaHarvestNode, TreeHealth);
    DOREPLIFETIME(AKalmalaHarvestNode, TreeFallStartedAt);
    DOREPLIFETIME(AKalmalaHarvestNode, ForestDimensions);
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
    SetActorTickEnabled(IsTreeFalling() && !bHarvested);
    UpdateTreePresentationPose();
    SourceMesh->ClearAllMeshSections();
    // Timber and bundles use their presented mesh as the target. The fixed
    // identity-root sphere must not leave an invisible target behind them.
#if !UE_BUILD_SHIPPING
    const bool bIgnoreHarvestCollision = FParse::Param(FCommandLine::Get(), TEXT("KalmalaSwimmingTest"));
#else
    const bool bIgnoreHarvestCollision = false;
#endif
    Collision->SetCollisionEnabled(bIgnoreHarvestCollision || bHarvested || IsFelledTrunk() || (IsTreeResource() && IsWoodDrop())
        ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);

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
    if (IsGemstone())
    {
        const FName Gem = GetHarvestItemId();
        const FLinearColor Inclusion = Gem == TEXT("GreenJade") ? FLinearColor(.12f,.7f,.3f)
            : Gem == TEXT("Amethyst") ? FLinearColor(.65f,.2f,.9f)
            : Gem == TEXT("Tourmaline") ? FLinearColor(.9f,.25f,.45f)
            : Gem == TEXT("StarOpal") ? FLinearColor(.55f,.85f,1.f) : FLinearColor(.65f,.5f,.35f);
        if (GemOutcome != 2)
            AddTaperedBox(Vertices, Triangles, Colors, FVector::ZeroVector, FVector(28,24,0),
                GemOutcome == 1 ? 8.f : 45.f - GemHits * 6.f, .65f, 12.f, FLinearColor(.22f,.24f,.27f));
        if (GemOutcome != 1)
            AddTaperedBox(Vertices, Triangles, Colors, FVector(0,-12,GemOutcome == 2 ? 0 : 24), FVector(9,7,0),
                GemOutcome == 2 ? 14.f : 18.f, .25f, -15.f, Inclusion);
    }
    else if (IsTreeResource())
    {
        const bool bAspen = PresentationId == TEXT("aspen-trunk-harvest");
        const FLinearColor BarkColor = bAspen ? FLinearColor(0.66f, 0.62f, 0.54f) : FLinearColor(0.34f, 0.22f, 0.12f);
        const FLinearColor LeafColor = bAspen ? FLinearColor(0.37f, 0.50f, 0.20f) : FLinearColor(0.18f, 0.35f, 0.12f);
        const FLinearColor CutFaceColor = bAspen ? FLinearColor(0.82f, 0.75f, 0.59f) : FLinearColor(0.72f, 0.51f, 0.28f);
        if (TreeStage == 0 || IsTreeFalling() || IsFelledTrunk())
        {
            const bool bForest = ForestDimensions.X > 0 && ForestDimensions.Y > 0
                && ForestDimensions.Z > 0 && ForestDimensions.W > 0;
            const float Radius = bForest ? ForestDimensions.X : 20.0f;
            const float Height = bForest ? ForestDimensions.Y : 280.0f;
            const float CanopyRadius = bForest ? ForestDimensions.Z : 85.0f;
            const float CanopyBase = bForest ? Height * 0.68f : 175.0f;
            const float CanopyHeight = bForest ? Height + ForestDimensions.W - CanopyBase : 145.0f;
            AddTaperedBox(Vertices, Triangles, Colors, FVector::ZeroVector, FVector(Radius, Radius, 0), Height, bForest ? 0.62f : 0.7f, 0, BarkColor);
            if (IsFelledTrunk())
            {
                // Exposed original cut faces; retain the falling trunk's full
                // dimensions and final pose rather than replacing it with a prop.
                const float TopRadius = Radius * (bForest ? 0.62f : 0.7f);
                AddTaperedBox(Vertices, Triangles, Colors, FVector(0, 0, -0.5f), FVector(Radius, Radius, 0), 0.5f, 1, 0, CutFaceColor);
                AddTaperedBox(Vertices, Triangles, Colors, FVector(0, 0, Height), FVector(TopRadius, TopRadius, 0), 0.5f, 1, 0, CutFaceColor);
            }
            else
            {
                AddTaperedBox(Vertices, Triangles, Colors, FVector(0, 0, CanopyBase), FVector(CanopyRadius, CanopyRadius, 0), CanopyHeight, 0.15f, 25, LeafColor);
            }
        }
        else
        {
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-12, 0, 0), FVector(12, 35, 0), 22, 0.85f, 12, FLinearColor(0.6f, 0.4f, 0.2f));
            AddTaperedBox(Vertices, Triangles, Colors, FVector(12, 0, 0), FVector(12, 35, 0), 22, 0.85f, -12, FLinearColor(0.6f, 0.4f, 0.2f));
        }
        Collision->SetSphereRadius(TreeStage == 0 || IsTreeFalling() ? 80.0f : 65.0f);
    }
    else if (PresentationId == TEXT("reed-cluster") || PresentationId == TEXT("frostmoss-clump"))
    {
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-18.0f, 0.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 58.0f, 0.55f, -8.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(0.0f, 7.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), PresentationId == TEXT("frostmoss-clump") ? 42.0f : 78.0f, 0.65f, 4.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -2.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 66.0f, 0.5f, 11.0f, Accent);
    }
    else if (IsRockSource(PresentationId))
    {
        if (PresentationId == TEXT("copper-ore-vein"))
        {
            const FLinearColor RockBase(0.27f, 0.24f, 0.21f);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-14.0f, 0.0f, 0.0f), FVector(24.0f, 14.0f, 0.0f), 28.0f, 0.72f, -9.0f, RockBase);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(14.0f, 3.0f, 0.0f), FVector(18.0f, 12.0f, 0.0f), 38.0f, 0.68f, 12.0f, RockBase);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-18.0f, -10.0f, 14.0f), FVector(12.0f, 3.0f, 0.0f), 3.0f, 0.85f, -9.0f, Accent);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(10.0f, -7.0f, 20.0f), FVector(10.0f, 2.5f, 0.0f), 3.0f, 0.85f, 12.0f, Accent);
        }
        else if (PresentationId == TEXT("tin-shore-gravel"))
        {
            const FLinearColor Gravel(0.38f, 0.40f, 0.39f);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-19.0f, -2.0f, 0.0f), FVector(13.0f, 10.0f, 0.0f), 14.0f, 0.68f, -12.0f, Gravel);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(3.0f, 4.0f, 0.0f), FVector(14.0f, 11.0f, 0.0f), 18.0f, 0.64f, 8.0f, Gravel);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(23.0f, -5.0f, 0.0f), FVector(9.0f, 8.0f, 0.0f), 11.0f, 0.72f, 18.0f, Gravel);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-1.0f, -4.0f, 9.0f), FVector(7.0f, 2.5f, 0.0f), 2.0f, 0.85f, 8.0f, Accent);
        }
        else
        {
            AddTaperedBox(Vertices, Triangles, Colors, FVector(-14.0f, 0.0f, 0.0f), FVector(24.0f, 14.0f, 0.0f), 28.0f, 0.72f, -9.0f, Accent);
            AddTaperedBox(Vertices, Triangles, Colors, FVector(14.0f, 3.0f, 0.0f), FVector(18.0f, 12.0f, 0.0f), 38.0f, 0.68f, 12.0f, Accent);
        }
    }
    else if (PresentationId == TEXT("lingonberry-shrub-harvest"))
    {
        const FLinearColor LeafColor(0.18f, 0.40f, 0.17f);
        const FLinearColor BerryColor(0.72f, 0.08f, 0.13f);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-12.0f, 0.0f, 0.0f), FVector(12.0f, 12.0f, 0.0f), 30.0f, 0.55f, -12.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(11.0f, 3.0f, 0.0f), FVector(12.0f, 11.0f, 0.0f), 34.0f, 0.56f, 11.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(0.0f, -8.0f, 22.0f), FVector(13.0f, 9.0f, 0.0f), 18.0f, 0.58f, 4.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-18.0f, -10.0f, 23.0f), FVector(4.0f, 4.0f, 0.0f), 5.0f, 0.82f, 0.0f, BerryColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-3.0f, -13.0f, 31.0f), FVector(4.0f, 4.0f, 0.0f), 5.0f, 0.82f, 0.0f, BerryColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(15.0f, -8.0f, 26.0f), FVector(4.0f, 4.0f, 0.0f), 5.0f, 0.82f, 0.0f, BerryColor);
    }
    else if (PresentationId == TEXT("carrot-patch-harvest"))
    {
        const FLinearColor CarrotColor(0.94f, 0.34f, 0.08f);
        const FLinearColor LeafColor(0.25f, 0.58f, 0.16f);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 15.0f, 0.2f, -8.0f, CarrotColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(3.0f, 8.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 15.0f, 0.2f, 6.0f, CarrotColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(17.0f, -9.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 15.0f, 0.2f, 14.0f, CarrotColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 6.0f), FVector(2.5f, 7.0f, 0.0f), 35.0f, 0.12f, -20.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(3.0f, 8.0f, 6.0f), FVector(2.5f, 7.0f, 0.0f), 38.0f, 0.12f, 10.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(17.0f, -9.0f, 6.0f), FVector(2.5f, 7.0f, 0.0f), 34.0f, 0.12f, 24.0f, LeafColor);
    }
    else if (PresentationId == TEXT("potato-patch-harvest"))
    {
        const FLinearColor PotatoColor(0.67f, 0.52f, 0.32f);
        const FLinearColor LeafColor(0.29f, 0.50f, 0.18f);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 0.0f), FVector(7.0f, 5.0f, 0.0f), 10.0f, 0.75f, -8.0f, PotatoColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 0.0f), FVector(7.0f, 5.0f, 0.0f), 11.0f, 0.72f, 7.0f, PotatoColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 0.0f), FVector(6.0f, 5.0f, 0.0f), 9.0f, 0.8f, 15.0f, PotatoColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-13.0f, -2.0f, 8.0f), FVector(3.0f, 5.0f, 0.0f), 24.0f, 0.18f, -18.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 9.0f), FVector(3.0f, 5.0f, 0.0f), 27.0f, 0.18f, 13.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 7.0f), FVector(3.0f, 5.0f, 0.0f), 22.0f, 0.18f, 24.0f, LeafColor);
    }
    else if (PresentationId == TEXT("rutabaga-patch-harvest"))
    {
        const FLinearColor RutabagaColor(0.62f, 0.28f, 0.48f);
        const FLinearColor RootColor(0.88f, 0.78f, 0.65f);
        const FLinearColor LeafColor(0.26f, 0.48f, 0.18f);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 0.0f), FVector(8.0f, 6.0f, 0.0f), 13.0f, 0.78f, -8.0f, RutabagaColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 0.0f), FVector(8.0f, 6.0f, 0.0f), 14.0f, 0.76f, 7.0f, RutabagaColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 0.0f), FVector(7.0f, 6.0f, 0.0f), 12.0f, 0.8f, 15.0f, RutabagaColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 10.0f), FVector(4.0f, 3.0f, 0.0f), 6.0f, 0.85f, -8.0f, RootColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 11.0f), FVector(4.0f, 3.0f, 0.0f), 6.0f, 0.85f, 7.0f, RootColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 9.0f), FVector(3.5f, 3.0f, 0.0f), 6.0f, 0.85f, 15.0f, RootColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 12.0f), FVector(3.0f, 6.0f, 0.0f), 29.0f, 0.16f, -20.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 13.0f), FVector(3.0f, 6.0f, 0.0f), 31.0f, 0.16f, 10.0f, LeafColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 11.0f), FVector(3.0f, 6.0f, 0.0f), 27.0f, 0.16f, 24.0f, LeafColor);
    }
    else if (PresentationId == TEXT("chanterelle-patch-harvest"))
    {
        const FLinearColor StemColor(0.84f, 0.75f, 0.56f);
        const FLinearColor CapColor(0.83f, 0.43f, 0.10f);
        const FVector MushroomCenters[] = { FVector(-13.0f, -7.0f, 0.0f), FVector(0.0f, 5.0f, 0.0f), FVector(15.0f, -6.0f, 0.0f) };
        const float MushroomScales[] = { 0.72f, 1.0f, 0.82f };
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(MushroomCenters); ++Index)
        {
            const float Scale = MushroomScales[Index];
            const FVector Center = MushroomCenters[Index];
            AddTaperedBox(Vertices, Triangles, Colors, Center, FVector(3.5f * Scale, 3.5f * Scale, 0.0f),
                16.0f * Scale, 0.78f, 0.0f, StemColor);
            AddTaperedBox(Vertices, Triangles, Colors, Center + FVector(0.0f, 0.0f, 13.0f * Scale),
                FVector(9.0f * Scale, 8.0f * Scale, 0.0f), 6.0f * Scale, 0.5f, 0.0f, CapColor);
        }
    }
    else if (PresentationId == TEXT("onion-patch-harvest"))
    {
        const FLinearColor OnionColor(0.74f, 0.57f, 0.39f);
        const FLinearColor ShootColor(0.24f, 0.42f, 0.17f);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 0.0f), FVector(6.0f, 5.0f, 0.0f), 13.0f, 0.48f, -8.0f, OnionColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 0.0f), FVector(6.0f, 5.0f, 0.0f), 14.0f, 0.46f, 7.0f, OnionColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 0.0f), FVector(5.0f, 5.0f, 0.0f), 12.0f, 0.5f, 15.0f, OnionColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-15.0f, -3.0f, 9.0f), FVector(2.5f, 3.5f, 0.0f), 39.0f, 0.12f, -20.0f, ShootColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(5.0f, 8.0f, 10.0f), FVector(2.5f, 3.5f, 0.0f), 43.0f, 0.12f, 10.0f, ShootColor);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(18.0f, -9.0f, 8.0f), FVector(2.5f, 3.5f, 0.0f), 36.0f, 0.12f, 24.0f, ShootColor);
    }
    else
    {
        AddTaperedBox(Vertices, Triangles, Colors, FVector(-16.0f, 0.0f, 0.0f), FVector(8.0f, 15.0f, 0.0f), 72.0f, 0.55f, -12.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(6.0f, 2.0f, 0.0f), FVector(9.0f, 13.0f, 0.0f), 94.0f, 0.6f, 8.0f, Accent);
        AddTaperedBox(Vertices, Triangles, Colors, FVector(20.0f, -4.0f, 0.0f), FVector(6.0f, 11.0f, 0.0f), 60.0f, 0.5f, 18.0f, Accent);
    }

    SourceMesh->SetMaterial(0, (IsGemstone() || IsRockSource(PresentationId)) ? RockMaterial : BarkMaterial);
    SourceMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, {}, {}, Colors, {}, IsTreeResource());
    SourceMesh->SetCollisionProfileName(TEXT("BlockAll"));
    SourceMesh->SetCollisionEnabled(!bIgnoreHarvestCollision && IsTreeResource() && !bHarvested
        ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    SourceMesh->SetVisibility(!bHarvested);
}

void AKalmalaHarvestNode::ApplyHarvestedState()
{
    SetActorTickEnabled(IsTreeFalling() && !bHarvested);
    SetActorHiddenInGame(bHarvested);
#if !UE_BUILD_SHIPPING
    const bool bIgnoreHarvestCollision = FParse::Param(FCommandLine::Get(), TEXT("KalmalaSwimmingTest"));
#else
    const bool bIgnoreHarvestCollision = false;
#endif
    Collision->SetCollisionEnabled(bIgnoreHarvestCollision || bHarvested || IsFelledTrunk() || (IsTreeResource() && IsWoodDrop())
        ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
    if (SourceMesh)
    {
        SourceMesh->SetVisibility(!bHarvested && !GetGatheringPresentationId().IsNone());
        SourceMesh->SetCollisionEnabled(!bIgnoreHarvestCollision && IsTreeResource() && !bHarvested
            ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    }
}
