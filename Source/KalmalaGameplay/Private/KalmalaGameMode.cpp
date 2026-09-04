#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationMarker.h"
#include "KalmalaWorldPopulationSaveGame.h"

#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace KalmalaGameMode
{
    constexpr int32 PlayerTerrainPatchRadius = 1;
    constexpr int32 MaxActiveTerrainPatches = 25;
    constexpr int32 MaxActivePopulationSpatialKeys = 9;
    constexpr float TerrainPatchActivationIntervalSeconds = 1.0f;
    constexpr float TraversalTestSpeed = 1800.0f;
    constexpr float TraversalTestArrivalDistance = 180.0f;
    constexpr int32 TraversalTargetSearchExtent = 12000;
    constexpr int32 TraversalTargetSearchStep = 250;
}

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AKalmalaCharacter::StaticClass();
    GameStateClass = AKalmalaWorldGenerationGameState::StaticClass();
}

void AKalmalaGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create the generated player start because world identity is unavailable."));
        return;
    }

    WorldGenerationConfig = WorldGenerationState->GetWorldGenerationConfig();
    PopulationSaveGame = NewObject<UKalmalaWorldPopulationSaveGame>(this);
    PopulationSaveGame->InitializeForWorld(WorldGenerationConfig);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GeneratedPlayerStart = GetWorld()->SpawnActor<APlayerStart>(
        APlayerStart::StaticClass(),
        FKalmalaWorldPlayerStartResolver::ResolveStartTransform(WorldGenerationState->GetWorldGenerationConfig()),
        SpawnParameters);

    if (GeneratedPlayerStart != nullptr)
    {
        GeneratedPlayerStart->Tags.Add(TEXT("GeneratedWorldPlayerStart"));
        UE_LOG(LogTemp, Display, TEXT("Server created seed-derived player start at %s."), *GeneratedPlayerStart->GetActorLocation().ToCompactString());

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        TerrainPatchOrigin = FVector2D(StartLocation.X, StartLocation.Y);
        ActivateTerrainPatchNeighborhood(TerrainPatchOrigin);
        UE_LOG(LogTemp, Display, TEXT("Server activated %d seed-derived terrain patches around the generated start."), ActiveTerrainPatchCoordinates.Num());
        ConfigureTraversalTest();
    }
}

void AKalmalaGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr)
    {
        return;
    }

    DriveTraversalTest();

    if (GetWorld()->GetTimeSeconds() < NextTerrainPatchActivationTime)
    {
        return;
    }

    NextTerrainPatchActivationTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::TerrainPatchActivationIntervalSeconds;
    for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
    {
        const APlayerController* PlayerController = PlayerControllerIterator->Get();
        const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
        if (PlayerPawn != nullptr)
        {
            ActivateTerrainPatchNeighborhood(FVector2D(PlayerPawn->GetActorLocation()));
            ActivatePopulationKey(FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(PlayerPawn->GetActorLocation())));
        }
    }
}

void AKalmalaGameMode::ActivatePopulationKey(const FIntPoint& SpatialKey)
{
    if (!HasAuthority() || GetWorld() == nullptr || ActivePopulationSpatialKeys.Contains(SpatialKey) || ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
    {
        return;
    }

    int32 SpawnedMarkerCount = 0;
    for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard })
    {
        for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, Kind))
        {
            if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
            {
                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (PopulationSaveGame != nullptr && PopulationSaveGame->IsHarvested(PersistentSpawnId))
                {
                    continue;
                }

                AKalmalaHarvestNode* HarvestNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (HarvestNode != nullptr)
                {
                    HarvestNode->InitializeServer(Spawn);
                    HarvestNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                    ++SpawnedMarkerCount;
                }
            }
            else
            {
                AKalmalaWorldPopulationMarker* Marker = GetWorld()->SpawnActor<AKalmalaWorldPopulationMarker>(AKalmalaWorldPopulationMarker::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (Marker != nullptr)
                {
                    Marker->InitializeServer(Spawn);
                    ++SpawnedMarkerCount;
                }
            }
        }
    }

    ActivePopulationSpatialKeys.Add(SpatialKey);
    UE_LOG(LogTemp, Display, TEXT("Server activated %d deterministic population markers for spatial key (%d, %d); %d/%d active."), SpawnedMarkerCount, SpatialKey.X, SpatialKey.Y, ActivePopulationSpatialKeys.Num(), KalmalaGameMode::MaxActivePopulationSpatialKeys);
}

void AKalmalaGameMode::RecordHarvestedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkHarvested(PersistentSpawnId);
    }
}

void AKalmalaGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!bTraversalTestEnabled || NewPlayer == nullptr)
    {
        return;
    }

    // GameModeBase does not guarantee a pawn for a headless, command-line client.
    // The harness creates its normal default pawn only when the test switch is enabled.
    if (NewPlayer->GetPawn() == nullptr)
    {
        RestartPlayer(NewPlayer);
    }

    UE_LOG(LogTemp, Display, TEXT("Traversal-test server player joined with pawn: %s."), *GetNameSafe(NewPlayer->GetPawn()));
}

void AKalmalaGameMode::ConfigureTraversalTest()
{
    bTraversalTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaTraversalTest"));
    if (!bTraversalTestEnabled)
    {
        return;
    }

    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -KalmalaGameMode::TraversalTargetSearchExtent; Y <= KalmalaGameMode::TraversalTargetSearchExtent; Y += KalmalaGameMode::TraversalTargetSearchStep)
    {
        for (int32 X = -KalmalaGameMode::TraversalTargetSearchExtent; X <= KalmalaGameMode::TraversalTargetSearchExtent; X += KalmalaGameMode::TraversalTargetSearchStep)
        {
            const FVector2D Candidate = TerrainPatchOrigin + FVector2D(X, Y);
            if (!FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Candidate))
            {
                continue;
            }

            const float DistanceSquared = FVector2D::DistSquared(TerrainPatchOrigin, Candidate);
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                TraversalTestTarget = Candidate;
            }
        }
    }

    if (ClosestDistanceSquared == TNumericLimits<float>::Max())
    {
        UE_LOG(LogTemp, Error, TEXT("Traversal-test switch was requested but no Shimmering Lakes target was found within %d units."), KalmalaGameMode::TraversalTargetSearchExtent);
        bTraversalTestEnabled = false;
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("Traversal-test server target is %s, %.0f units from the generated start."), *TraversalTestTarget.ToString(), FMath::Sqrt(ClosestDistanceSquared));
}

void AKalmalaGameMode::DriveTraversalTest()
{
    if (!bTraversalTestEnabled)
    {
        return;
    }

    for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
    {
        const APlayerController* PlayerController = PlayerControllerIterator->Get();
        if (PlayerController == nullptr || !PlayerController->IsLocalController())
        {
            continue;
        }

        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
        if (Character == nullptr || TraversalTestCompletedPawns.Contains(Character))
        {
            continue;
        }

        if (!TraversalTestStartedPawns.Contains(Character))
        {
            TraversalTestStartedPawns.Add(Character);
            Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
            UE_LOG(LogTemp, Display, TEXT("Traversal-test server is driving pawn %s from %s toward the Shimmering Lakes target."), *Character->GetName(), *Character->GetActorLocation().ToCompactString());
        }

        const FVector CurrentLocation = Character->GetActorLocation();
        const FVector2D RemainingOffset = TraversalTestTarget - FVector2D(CurrentLocation);
        if (RemainingOffset.SizeSquared() <= FMath::Square(KalmalaGameMode::TraversalTestArrivalDistance))
        {
            TraversalTestCompletedPawns.Add(Character);
            UE_LOG(LogTemp, Display, TEXT("Traversal-test server pawn %s reached the Shimmering Lakes target after travelling %.0f units."), *Character->GetName(), FMath::Sqrt(FVector2D::DistSquared(TerrainPatchOrigin, FVector2D(CurrentLocation))));
            continue;
        }

        Character->GetCharacterMovement()->MaxWalkSpeed = KalmalaGameMode::TraversalTestSpeed;
        Character->AddMovementInput(FVector(RemainingOffset.GetSafeNormal(), 0.0f), 1.0f, true);
    }
}

void AKalmalaGameMode::ActivateTerrainPatch(const FIntPoint& PatchCoordinate)
{
    if (!HasAuthority() || ActiveTerrainPatchCoordinates.Contains(PatchCoordinate) || ActiveTerrainPatchCoordinates.Num() >= KalmalaGameMode::MaxActiveTerrainPatches || GetWorld() == nullptr)
    {
        return;
    }

    AKalmalaGeneratedTerrainPatch* TerrainPatch = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainPatch>(AKalmalaGeneratedTerrainPatch::StaticClass());
    if (TerrainPatch == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not activate terrain patch (%d, %d)."), PatchCoordinate.X, PatchCoordinate.Y);
        return;
    }

    TerrainPatch->Initialize(WorldGenerationConfig, FKalmalaTerrainPatchLayout::GetPatchCenter(TerrainPatchOrigin, PatchCoordinate.X, PatchCoordinate.Y));
    ActiveTerrainPatchCoordinates.Add(PatchCoordinate);
    UE_LOG(LogTemp, Display, TEXT("Server activated terrain patch (%d, %d); %d/%d active."), PatchCoordinate.X, PatchCoordinate.Y, ActiveTerrainPatchCoordinates.Num(), KalmalaGameMode::MaxActiveTerrainPatches);
}

void AKalmalaGameMode::ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition)
{
    const FIntPoint CenterPatchCoordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(TerrainPatchOrigin, WorldPosition);
    for (int32 PatchY = CenterPatchCoordinate.Y - KalmalaGameMode::PlayerTerrainPatchRadius; PatchY <= CenterPatchCoordinate.Y + KalmalaGameMode::PlayerTerrainPatchRadius; ++PatchY)
    {
        for (int32 PatchX = CenterPatchCoordinate.X - KalmalaGameMode::PlayerTerrainPatchRadius; PatchX <= CenterPatchCoordinate.X + KalmalaGameMode::PlayerTerrainPatchRadius; ++PatchX)
        {
            ActivateTerrainPatch(FIntPoint(PatchX, PatchY));
        }
    }
}

AActor* AKalmalaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    return GeneratedPlayerStart != nullptr ? GeneratedPlayerStart : Super::ChoosePlayerStart_Implementation(Player);
}
