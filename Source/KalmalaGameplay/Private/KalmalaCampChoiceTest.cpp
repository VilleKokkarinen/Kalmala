#include "KalmalaGameMode.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaCampConditionSampler.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaTerrainHeightSampler.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Scenario fixtures only: no camp markers, routes, saved changes, or client mutation RPCs.
void AKalmalaGameMode::DriveCampChoiceTest()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest")) || CampChoiceStage == 3)
    {
        return;
    }
    if (CampChoiceStage == 0)
    {
        CampChoicePlayers.Reset();
        bool bHasRemotePlayer = false;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* Controller = It->Get();
            if (AKalmalaCharacter* Character = Controller ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr)
            {
                CampChoicePlayers.Add(Character);
                bHasRemotePlayer |= !Controller->IsLocalController();
            }
        }
        if (CampChoicePlayers.Num() != 2 || !bHasRemotePlayer) return;

        // Compare local dry-land choices inside the existing initial terrain neighborhood.
        FVector2D Sites[2];
        float LeastCover = TNumericLimits<float>::Max();
        float MostCover = -1.0f;
        for (int32 Y = -2800; Y <= 2800; Y += 400)
        {
            for (int32 X = -2800; X <= 2800; X += 400)
            {
                const FVector2D Position = TerrainPatchOrigin + FVector2D(X, Y);
                if (FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position)) continue;
                const float Cover = FKalmalaEnvironmentalExposureSampler::Sample(WorldGenerationConfig, Position).NaturalCover;
                if (Cover < LeastCover) { LeastCover = Cover; Sites[0] = Position; }
                if (Cover > MostCover) { MostCover = Cover; Sites[1] = Position; }
            }
        }
        if (MostCover - LeastCover < 0.1f || FVector2D::Distance(Sites[0], Sites[1]) < 1000.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Camp choice FAILED: seed lacks separated contrasting fixtures in the bounded search."));
            CampChoiceStage = 3;
            return;
        }
        for (int32 Index = 0; Index < 2; ++Index)
        {
            AKalmalaCharacter* Character = CampChoicePlayers[Index].Get();
            const FVector2D Position = Sites[Index];
            ActivateTerrainPatchNeighborhood(Position);
            Character->SetActorLocation(FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position) + Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.0f), false, nullptr, ETeleportType::TeleportPhysics);
            Character->GetCharacterMovement()->StopMovementImmediately();
            FKalmalaExposureState Initial;
            Initial.Wetness = 45.0f;
            Initial.Warmth = 30.0f;
            Initial.TravelSpeedMultiplier = FKalmalaExposureResponse::GetTravelSpeedMultiplier(Initial.Warmth);
            Character->SetExposureStateFromServer(Initial);
            const FKalmalaCampConditionSample Conditions = FKalmalaCampConditionSampler::Sample(WorldGenerationConfig, Position);
            UE_LOG(LogTemp, Display, TEXT("Camp choice site %d pawn=%s Pos=%s Cover=%.2f GroundWet=%.2f WaterDistance=%.0f NearbyHarvestNodes=%d."), Index, *FString::FromInt(Character->GetPlayerState()->GetPlayerId()), *Position.ToString(), Conditions.NaturalCover, Conditions.GroundWetness, Conditions.WaterDistance, Conditions.NearbyHarvestNodeCount);
        }
        CampChoiceStartTime = GetWorld()->GetTimeSeconds();
        CampChoiceStage = 1;
    }
    const float Elapsed = GetWorld()->GetTimeSeconds() - CampChoiceStartTime;
    if (CampChoiceStage == 1 && Elapsed >= 12.0f)
    {
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : CampChoicePlayers)
        {
            AKalmalaCharacter* Character = Player.Get();
            if (!Character) { CampChoiceStage = 3; UE_LOG(LogTemp, Error, TEXT("Camp choice FAILED: player disconnected.")); return; }
            CampChoiceBaselineWarmth.Add(Character->GetExposureState().Warmth);
            CampChoiceBaselineWetness.Add(Character->GetExposureState().Wetness);
            UE_LOG(LogTemp, Display, TEXT("Camp choice unprepared %s: Wetness=%.2f Warmth=%.2f Travel=%.2f."), *FString::FromInt(Character->GetPlayerState()->GetPlayerId()), Character->GetExposureState().Wetness, Character->GetExposureState().Warmth, Character->GetExposureState().TravelSpeedMultiplier);
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaCampfire* Fire = GetWorld()->SpawnActor<AKalmalaCampfire>(AKalmalaCampfire::StaticClass(), Character->GetActorLocation() + FVector(100, 0, 0), FRotator::ZeroRotator, Parameters);
            if (Fire) Fire->Interact_Implementation(Character);
            if (!Fire || !Fire->IsLit()) { CampChoiceStage = 3; UE_LOG(LogTemp, Error, TEXT("Camp choice FAILED: normal server fire interaction rejected.")); return; }
        }
        CampChoiceStage = 2;
    }
    if (CampChoiceStage == 2 && Elapsed >= 32.0f)
    {
        bool bRecovered = true;
        for (int32 Index = 0; Index < 2; ++Index)
        {
            const AKalmalaCharacter* Character = CampChoicePlayers[Index].Get();
            if (!Character) { bRecovered = false; continue; }
            const FKalmalaExposureState& State = Character->GetExposureState();
            bRecovered &= State.Warmth > CampChoiceBaselineWarmth[Index] + 5.0f && State.Wetness < CampChoiceBaselineWetness[Index] - 1.0f && State.TravelSpeedMultiplier > FKalmalaExposureResponse::GetTravelSpeedMultiplier(CampChoiceBaselineWarmth[Index]);
            UE_LOG(LogTemp, Display, TEXT("Camp choice prepared %s: Wetness=%.2f Warmth=%.2f Travel=%.2f."), *FString::FromInt(Character->GetPlayerState()->GetPlayerId()), State.Wetness, State.Warmth, State.TravelSpeedMultiplier);
        }
        if (bRecovered)
        {
            UE_LOG(LogTemp, Display, TEXT("Camp choice server PASSED: two distinct local choices recovered through normal server fire and exposure updates."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Camp choice FAILED: both camps must dry, warm, and recover travel speed."));
        }
        CampChoiceStage = 3;
    }
#endif
}
