#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"

#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

namespace KalmalaGameMode
{
    constexpr int32 InitialTerrainPatchRadius = 1;
}

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
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

        int32 SpawnedTerrainPatchCount = 0;
        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        const FVector2D StartPatchCenter(StartLocation.X, StartLocation.Y);
        for (int32 PatchY = -KalmalaGameMode::InitialTerrainPatchRadius; PatchY <= KalmalaGameMode::InitialTerrainPatchRadius; ++PatchY)
        {
            for (int32 PatchX = -KalmalaGameMode::InitialTerrainPatchRadius; PatchX <= KalmalaGameMode::InitialTerrainPatchRadius; ++PatchX)
            {
                AKalmalaGeneratedTerrainPatch* TerrainPatch = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainPatch>(AKalmalaGeneratedTerrainPatch::StaticClass());
                if (TerrainPatch != nullptr)
                {
                    TerrainPatch->Initialize(
                        WorldGenerationState->GetWorldGenerationConfig(),
                        FKalmalaTerrainPatchLayout::GetPatchCenter(StartPatchCenter, PatchX, PatchY));
                    ++SpawnedTerrainPatchCount;
                }
            }
        }
        UE_LOG(LogTemp, Display, TEXT("Server activated %d seed-derived terrain patches around the generated start."), SpawnedTerrainPatchCount);
    }
}

AActor* AKalmalaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    return GeneratedPlayerStart != nullptr ? GeneratedPlayerStart : Super::ChoosePlayerStart_Implementation(Player);
}
