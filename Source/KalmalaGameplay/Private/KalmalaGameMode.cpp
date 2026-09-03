#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

namespace KalmalaGameMode
{
    constexpr int32 PlayerTerrainPatchRadius = 1;
    constexpr int32 MaxActiveTerrainPatches = 25;
    constexpr float TerrainPatchActivationIntervalSeconds = 1.0f;
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
    }
}

void AKalmalaGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr || GetWorld()->GetTimeSeconds() < NextTerrainPatchActivationTime)
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
        }
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
