#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaBiomeClassifier.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

namespace
{
    constexpr float TimberStairsHalfRun = 100.0f;
    constexpr float TimberStairsHalfWidth = 50.0f;
    constexpr float TimberStairsHalfRise = 50.0f;
    constexpr float TimberBeamHalfLength = 100.0f;
    constexpr float TimberBeamHalfWidth = 7.5f;
    constexpr float StructuralEndpointTolerance = 22.0f;

    bool TraceTerrainSupport(const UWorld* World, const APawn* Pawn, const FVector& Point, FHitResult& OutHit)
    {
        if (!World || !Pawn || Point.ContainsNaN()) return false;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(TimberBeamGroundSupport), false, Pawn);
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
            Query.AddIgnoredActor(*It);
        return World->LineTraceSingleByChannel(OutHit, Point + FVector(0.0f, 0.0f, 500.0f),
                Point - FVector(0.0f, 0.0f, 1500.0f), ECC_Visibility, Query)
            && Cast<AKalmalaGeneratedTerrainPatch>(OutHit.GetActor()) != nullptr;
    }

    AKalmalaConstructionActor* FindStructuralEndpointSupport(const UWorld* World, const FVector& Endpoint)
    {
        if (!World || Endpoint.ContainsNaN()) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            AKalmalaConstructionActor* Construction = *It;
            if (!IsValid(Construction) || Construction->GetWorld() != World
                || !AKalmalaConstructionActor::IsStructuralEndpointKit(Construction->GetConstructionKit())) continue;
            TArray<FVector> Endpoints;
            AKalmalaConstructionActor::GetStructuralEndpoints(Construction, Endpoints);
            if (Endpoints.ContainsByPredicate([&Endpoint](const FVector& Candidate)
                { return FVector::DistSquared(Candidate, Endpoint) <= FMath::Square(StructuralEndpointTolerance); }))
                return Construction;
        }
        return nullptr;
    }

    bool IsSupportedBeamEndpoint(const UWorld* World, const APawn* Pawn, const FVector& Endpoint,
        AKalmalaConstructionActor*& OutConstructionSupport, AActor*& OutGroundSupport)
    {
        OutConstructionSupport = nullptr;
        OutGroundSupport = nullptr;
        FHitResult GroundHit;
        if (TraceTerrainSupport(World, Pawn, Endpoint, GroundHit)
            && FMath::Abs(GroundHit.ImpactPoint.Z - Endpoint.Z) <= StructuralEndpointTolerance)
        {
            OutGroundSupport = GroundHit.GetActor();
            return true;
        }
        OutConstructionSupport = FindStructuralEndpointSupport(World, Endpoint);
        return OutConstructionSupport != nullptr;
    }

    void AddUniqueSupportActor(TArray<AActor*>& Supports, AActor* Actor)
    {
        if (IsValid(Actor)) Supports.AddUnique(Actor);
    }

    FKalmalaPlacementPreview EvaluateStandingLight(const UWorld* World, const APawn* Pawn)
    {
        FKalmalaPlacementPreview Result;
        Result.Message = TEXT("Preview invalid: place the Standing Light on ground or a supported timber floor");
        if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN())
        {
            Result.Message = TEXT("Preview unavailable: waiting for local terrain");
            return Result;
        }

        Result.Rotation = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f);
        const FVector Probe = Pawn->GetActorLocation() + Result.Rotation.Vector() * 165.0f;
        FCollisionQueryParams SupportQuery(SCENE_QUERY_STAT(StandingLightSupport), false, Pawn);
        FHitResult SupportHit;
        if (Probe.ContainsNaN() || !World->LineTraceSingleByChannel(SupportHit,
            Probe + FVector(0.0f, 0.0f, 300.0f), Probe - FVector(0.0f, 0.0f, 500.0f),
            ECC_Visibility, SupportQuery)) return Result;

        AActor* SupportActor = SupportHit.GetActor();
        AKalmalaConstructionActor* Floor = Cast<AKalmalaConstructionActor>(SupportActor);
        const bool bTerrainSupport = Cast<AKalmalaGeneratedTerrainPatch>(SupportActor) != nullptr;
        FVector SupportPoint = SupportHit.ImpactPoint;
        if (Floor)
        {
            if (Floor->GetWorld() != World || Floor->GetConstructionId().IsEmpty()
                || Floor->GetConstructionKit() != TEXT("FloorKit") || SupportHit.ImpactNormal.Z < 0.85f)
                return Result;
            const FTransform FloorTransform = Floor->GetActorTransform();
            const FVector LocalHit = FloorTransform.InverseTransformPosition(SupportHit.ImpactPoint);
            const float FloorTop = AKalmalaConstructionActor::GetCollisionExtent(TEXT("FloorKit")).Z;
            const float FootprintRadius = AKalmalaConstructionActor::GetCollisionExtent(TEXT("StandingLightKit")).X;
            if (FMath::Abs(LocalHit.X) + FootprintRadius > 110.0f
                || FMath::Abs(LocalHit.Y) + FootprintRadius > 110.0f
                || FMath::Abs(LocalHit.Z - FloorTop) > 5.0f) return Result;
            SupportPoint = FloorTransform.TransformPosition(FVector(LocalHit.X, LocalHit.Y, FloorTop));
        }
        else if (!bTerrainSupport || SupportHit.ImpactNormal.Z < 0.85f)
        {
            return Result;
        }

        const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
        if (!State || !State->GetWorldGenerationConfig().IsValid())
        {
            Result.Message = TEXT("Preview unavailable: waiting for world data");
            return Result;
        }
        const auto& Config = State->GetWorldGenerationConfig();
        if (!FKalmalaWorldBounds::Contains(Config, FVector2D(SupportPoint), 300))
        {
            Result.Message = TEXT("Preview invalid: Standing Light is too close to the world edge");
            return Result;
        }
        if (bTerrainSupport && (FKalmalaOceanSampler::Sample(Config, FVector2D(SupportPoint)).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, FVector2D(SupportPoint))
            || FKalmalaRegionalGeneration::Sample(Config, FVector2D(SupportPoint)).bHasWater))
        {
            Result.Message = TEXT("Preview invalid: Standing Light needs dry ground");
            return Result;
        }

        const FVector Extent = AKalmalaConstructionActor::GetCollisionExtent(TEXT("StandingLightKit"));
        Result.Location = SupportPoint + FVector(0.0f, 0.0f, Extent.Z);
        FCollisionQueryParams OverlapQuery(SCENE_QUERY_STAT(StandingLightPlacement), false, Pawn);
        if (IsValid(SupportActor)) OverlapQuery.AddIgnoredActor(SupportActor);
        if (World->OverlapBlockingTestByChannel(Result.Location, Result.Rotation.Quaternion(), ECC_Pawn,
            FCollisionShape::MakeBox(Extent), OverlapQuery))
        {
            Result.Message = TEXT("Preview invalid: clear space for the Standing Light");
            return Result;
        }

        Result.bIsValid = true;
        Result.Message = Floor
            ? TEXT("Preview valid: Standing Light rests on a supported timber floor; server will recheck")
            : TEXT("Preview valid: Standing Light rests on level dry ground; server will recheck");
        return Result;
    }

    FKalmalaPlacementPreview EvaluateTimberStairs(const UWorld* World, const APawn* Pawn)
    {
        FKalmalaPlacementPreview Result;
        Result.Message = TEXT("Preview invalid: ground the lower stair edge or attach the upper edge to a supported floor");
        if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN())
        {
            Result.Message = TEXT("Preview unavailable: waiting for local terrain");
            return Result;
        }

        const FVector ViewLocation = Pawn->GetPawnViewLocation();
        const FVector ViewDirection = Pawn->GetViewRotation().Vector();
        if (ViewLocation.ContainsNaN() || ViewDirection.ContainsNaN()) return Result;

        FVector SlopeUp = FKalmalaPlacementPreview::GetTimberStairsPlacementRotation(Pawn->GetActorRotation()).Vector();
        FVector LowerEndpoint = FVector::ZeroVector;
        AActor* GroundSupport = nullptr;
        AKalmalaConstructionActor* FloorSupport = nullptr;

        FCollisionQueryParams TargetQuery(SCENE_QUERY_STAT(TimberStairsTarget), false, Pawn);
        FHitResult TargetHit;
        if (World->LineTraceSingleByChannel(TargetHit, ViewLocation,
                ViewLocation + ViewDirection * 250.0f, ECC_Visibility, TargetQuery))
        {
            AKalmalaConstructionActor* CandidateFloor = Cast<AKalmalaConstructionActor>(TargetHit.GetActor());
            if (IsValid(CandidateFloor) && CandidateFloor->GetWorld() == World
                && !CandidateFloor->GetConstructionId().IsEmpty()
                && CandidateFloor->GetConstructionKit() == TEXT("FloorKit"))
            {
                const FTransform FloorTransform = CandidateFloor->GetActorTransform();
                const FVector LocalPawn = FloorTransform.InverseTransformPosition(Pawn->GetActorLocation());
                FVector LocalOutward;
                FVector LocalEdge;
                if (FMath::Abs(LocalPawn.X) >= FMath::Abs(LocalPawn.Y))
                {
                    const float Side = LocalPawn.X < 0.0f ? -1.0f : 1.0f;
                    LocalOutward = FVector(Side, 0.0f, 0.0f);
                    LocalEdge = FVector(Side * 120.0f, 0.0f, AKalmalaConstructionActor::GetCollisionExtent(TEXT("FloorKit")).Z);
                }
                else
                {
                    const float Side = LocalPawn.Y < 0.0f ? -1.0f : 1.0f;
                    LocalOutward = FVector(0.0f, Side, 0.0f);
                    LocalEdge = FVector(0.0f, Side * 120.0f, AKalmalaConstructionActor::GetCollisionExtent(TEXT("FloorKit")).Z);
                }
                const FVector Outward = FloorTransform.TransformVectorNoScale(LocalOutward).GetSafeNormal();
                SlopeUp = -Outward;
                Result.Rotation = FRotator(0.0f, SlopeUp.Rotation().Yaw, 0.0f);
                SlopeUp = Result.Rotation.Vector();
                const FVector UpperEndpoint = FloorTransform.TransformPosition(LocalEdge);
                LowerEndpoint = UpperEndpoint - SlopeUp * (TimberStairsHalfRun * 2.0f)
                    - FVector(0.0f, 0.0f, TimberStairsHalfRise * 2.0f);
                FloorSupport = CandidateFloor;
            }
        }

        if (!FloorSupport)
        {
            Result.Rotation = FKalmalaPlacementPreview::GetTimberStairsPlacementRotation(Pawn->GetActorRotation());
            SlopeUp = Result.Rotation.Vector();
            const FVector LowerProbe = Pawn->GetActorLocation() + SlopeUp * 165.0f;
            FHitResult LowerGround;
            if (!TraceTerrainSupport(World, Pawn, LowerProbe, LowerGround))
            {
                Result.Message = TEXT("Preview invalid: the lower stair edge needs firm ground or a supported floor attachment");
                return Result;
            }
            LowerEndpoint = LowerGround.ImpactPoint;
            GroundSupport = LowerGround.GetActor();
            if (LowerGround.ImpactNormal.Z < 0.85f)
            {
                Result.Message = TEXT("Preview invalid: the lower stair edge needs level ground");
                return Result;
            }
        }
        else
        {
            FHitResult LowerGround;
            if (!TraceTerrainSupport(World, Pawn, LowerEndpoint, LowerGround)
                || FMath::Abs(LowerGround.ImpactPoint.Z - LowerEndpoint.Z) > StructuralEndpointTolerance
                || LowerGround.ImpactNormal.Z < 0.85f)
            {
                Result.Message = TEXT("Preview invalid: floor-attached stairs need firm ground at the lower edge");
                return Result;
            }
            GroundSupport = LowerGround.GetActor();
        }

        const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
        if (!State || !State->GetWorldGenerationConfig().IsValid())
        {
            Result.Message = TEXT("Preview unavailable: waiting for world data");
            return Result;
        }
        const auto& Config = State->GetWorldGenerationConfig();
        const FVector2D GroundSurface(LowerEndpoint);
        if (!FKalmalaWorldBounds::Contains(Config, GroundSurface, 300))
        {
            Result.Message = TEXT("Preview invalid: stairs are too close to the world edge");
            return Result;
        }
        if (FKalmalaOceanSampler::Sample(Config, GroundSurface).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, GroundSurface)
            || FKalmalaRegionalGeneration::Sample(Config, GroundSurface).bHasWater)
        {
            Result.Message = TEXT("Preview invalid: stairs need dry ground");
            return Result;
        }

        Result.Location = LowerEndpoint + SlopeUp * TimberStairsHalfRun
            + FVector(0.0f, 0.0f, TimberStairsHalfRise);
        FCollisionQueryParams OverlapQuery(SCENE_QUERY_STAT(TimberStairsPlacement), false, Pawn);
        if (IsValid(GroundSupport)) OverlapQuery.AddIgnoredActor(GroundSupport);
        if (IsValid(FloorSupport)) OverlapQuery.AddIgnoredActor(FloorSupport);
        const FCollisionShape StairsShape = FCollisionShape::MakeBox(
            FVector(TimberStairsHalfRun, TimberStairsHalfWidth, TimberStairsHalfRise));
        if (World->OverlapBlockingTestByChannel(Result.Location, Result.Rotation.Quaternion(), ECC_Pawn,
            StairsShape, OverlapQuery))
        {
            Result.Message = TEXT("Preview invalid: clear more space around the Timber Stairs");
            return Result;
        }

        Result.bIsValid = true;
        Result.Message = FloorSupport
            ? TEXT("Preview valid: upper stair edge attaches to the floor; server will recheck before payment")
            : TEXT("Preview valid: lower stair edge rests on dry ground; server will recheck before payment");
        return Result;
    }

    FKalmalaPlacementPreview EvaluateCampBed(const UWorld* World, const APawn* Pawn)
    {
        FKalmalaPlacementPreview Result;
        Result.Message = TEXT("Preview invalid: place the Camp Bed on dry, supported ground or a timber floor");
        if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN())
        {
            Result.Message = TEXT("Preview unavailable: waiting for local terrain");
            return Result;
        }

        Result.Rotation = FKalmalaPlacementPreview::GetTimberStairsPlacementRotation(Pawn->GetActorRotation());
        const FVector Forward = Result.Rotation.Vector();
        const FVector Right = FRotationMatrix(Result.Rotation).GetUnitAxis(EAxis::Y);
        const FVector Probe = Pawn->GetActorLocation() + Forward * 165.0f;
        FCollisionQueryParams SupportQuery(SCENE_QUERY_STAT(CampBedSupport), false, Pawn);
        FHitResult SupportHit;
        if (!World->LineTraceSingleByChannel(SupportHit, Probe + FVector(0, 0, 300),
                Probe - FVector(0, 0, 500), ECC_Visibility, SupportQuery)) return Result;

        AActor* SupportActor = SupportHit.GetActor();
        AKalmalaConstructionActor* FloorSupport = Cast<AKalmalaConstructionActor>(SupportActor);
        const bool bTerrainSupport = Cast<AKalmalaGeneratedTerrainPatch>(SupportActor) != nullptr;
        if (FloorSupport && (FloorSupport->GetWorld() != World || FloorSupport->GetConstructionId().IsEmpty()
            || FloorSupport->GetConstructionKit() != TEXT("FloorKit") || SupportHit.ImpactNormal.Z < 0.85f))
        {
            return Result;
        }
        if (!bTerrainSupport && !FloorSupport) return Result;
        if (bTerrainSupport && SupportHit.ImpactNormal.Z < 0.85f)
        {
            Result.Message = TEXT("Preview invalid: the Camp Bed needs level supported ground");
            return Result;
        }

        FVector SupportPoint = SupportHit.ImpactPoint;
        FTransform FloorTransform;
        if (FloorSupport)
        {
            FloorTransform = FloorSupport->GetActorTransform();
            const FVector LocalHit = FloorTransform.InverseTransformPosition(SupportHit.ImpactPoint);
            const float FloorTop = AKalmalaConstructionActor::GetCollisionExtent(TEXT("FloorKit")).Z;
            if (FMath::Abs(LocalHit.X) > 110.0f || FMath::Abs(LocalHit.Y) > 110.0f
                || FMath::Abs(LocalHit.Z - FloorTop) > 5.0f) return Result;
            SupportPoint = FloorTransform.TransformPosition(FVector(LocalHit.X, LocalHit.Y, FloorTop));
        }

        const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
        if (!State || !State->GetWorldGenerationConfig().IsValid())
        {
            Result.Message = TEXT("Preview unavailable: waiting for world data");
            return Result;
        }
        const auto& Config = State->GetWorldGenerationConfig();
        const FVector2D Surface(SupportPoint);
        if (!FKalmalaWorldBounds::Contains(Config, Surface, 300))
        {
            Result.Message = TEXT("Preview invalid: the Camp Bed is too close to the world edge");
            return Result;
        }
        const auto IsDry = [&Config](const FVector& Point)
        {
            const FVector2D SurfacePoint(Point);
            return !FKalmalaOceanSampler::Sample(Config, SurfacePoint).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(Config, SurfacePoint)
                && !FKalmalaRegionalGeneration::Sample(Config, SurfacePoint).bHasWater;
        };
        if (!IsDry(SupportPoint))
        {
            Result.Message = TEXT("Preview invalid: the Camp Bed needs a dry floor");
            return Result;
        }

        constexpr float FootprintHalfLength = 88.0f;
        constexpr float FootprintHalfWidth = 38.0f;
        for (const float Along : {-FootprintHalfLength, FootprintHalfLength})
        {
            for (const float Across : {-FootprintHalfWidth, FootprintHalfWidth})
            {
                const FVector Contact = SupportPoint + Forward * Along + Right * Across;
                if (!IsDry(Contact))
                {
                    Result.Message = TEXT("Preview invalid: the Camp Bed footprint must stay on dry ground");
                    return Result;
                }
                if (FloorSupport)
                {
                    const FVector LocalContact = FloorTransform.InverseTransformPosition(Contact);
                    if (FMath::Abs(LocalContact.X) > 110.0f || FMath::Abs(LocalContact.Y) > 110.0f)
                    {
                        Result.Message = TEXT("Preview invalid: the complete Camp Bed needs supported floor");
                        return Result;
                    }
                }
                else
                {
                    FHitResult ContactHit;
                    if (!TraceTerrainSupport(World, Pawn, Contact, ContactHit)
                        || ContactHit.ImpactNormal.Z < 0.85f
                        || FMath::Abs(ContactHit.ImpactPoint.Z - SupportPoint.Z) > 12.0f)
                    {
                        Result.Message = TEXT("Preview invalid: the complete Camp Bed needs level support");
                        return Result;
                    }
                }
            }
        }

        const FVector BedExtent = AKalmalaConstructionActor::GetCollisionExtent(TEXT("CampBedKit"));
        Result.Location = SupportPoint + FVector(0, 0, BedExtent.Z);
        FCollisionQueryParams OverlapQuery(SCENE_QUERY_STAT(CampBedPlacement), false, Pawn);
        if (IsValid(SupportActor)) OverlapQuery.AddIgnoredActor(SupportActor);
        if (World->OverlapBlockingTestByChannel(Result.Location, Result.Rotation.Quaternion(), ECC_Pawn,
            FCollisionShape::MakeBox(BedExtent), OverlapQuery))
        {
            Result.Message = TEXT("Preview invalid: clear space for the Camp Bed footprint");
            return Result;
        }

        const FVector ApproachCentre = Result.Location - Forward * 150.0f + FVector(0, 0, 28.0f - BedExtent.Z);
        if (World->OverlapBlockingTestByChannel(ApproachCentre, Result.Rotation.Quaternion(), ECC_Pawn,
            FCollisionShape::MakeBox(FVector(50.0f, 40.0f, 50.0f)), OverlapQuery))
        {
            Result.Message = TEXT("Preview invalid: keep a clear 1 m approach to the Camp Bed");
            return Result;
        }

        Result.bIsValid = true;
        Result.Message = FloorSupport
            ? TEXT("Preview valid: Camp Bed rests on a dry timber floor with a clear approach")
            : TEXT("Preview valid: Camp Bed rests on level dry ground with a clear approach");
        return Result;
    }

    FKalmalaPlacementPreview EvaluateTimberBeam(const UWorld* World, const APawn* Pawn)
    {
        FKalmalaPlacementPreview Result;
        Result.Message = TEXT("Preview invalid: place a Timber Beam on ground or at supported endpoints");
        if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN())
        {
            Result.Message = TEXT("Preview unavailable: waiting for local terrain");
            return Result;
        }

        const FRotator ViewRotation = Pawn->GetViewRotation();
        if (ViewRotation.ContainsNaN()) return Result;
        Result.Rotation = FKalmalaPlacementPreview::GetTimberBeamPlacementRotation(ViewRotation);
        const bool bVertical = Result.Rotation.Pitch != 0.0f;
        const FVector Axis = Result.Rotation.Vector().GetSafeNormal();
        const FVector GroundProbe = Pawn->GetActorLocation()
            + FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Vector() * 165.0f;
        const FVector ViewLocation = Pawn->GetPawnViewLocation();
        const FVector ViewDirection = ViewRotation.Vector();
        if (GroundProbe.ContainsNaN() || ViewLocation.ContainsNaN() || ViewDirection.ContainsNaN()) return Result;

        TArray<AActor*> SupportActors;
        FCollisionQueryParams TargetQuery(SCENE_QUERY_STAT(TimberBeamTarget), false, Pawn);
        FHitResult TargetHit;
        const bool bHitTarget = World->LineTraceSingleByChannel(TargetHit, ViewLocation,
            ViewLocation + ViewDirection * 250.0f, ECC_Visibility, TargetQuery);
        AKalmalaConstructionActor* Anchor = bHitTarget
            ? Cast<AKalmalaConstructionActor>(TargetHit.GetActor()) : nullptr;
        const bool bStructuralTarget = IsValid(Anchor) && Anchor->GetWorld() == World
            && !Anchor->GetConstructionId().IsEmpty()
            && AKalmalaConstructionActor::IsStructuralEndpointKit(Anchor->GetConstructionKit());

        if (bStructuralTarget)
        {
            TArray<FVector> Endpoints;
            AKalmalaConstructionActor::GetStructuralEndpoints(Anchor, Endpoints);
            int32 NearestIndex = INDEX_NONE;
            float NearestDistanceSquared = TNumericLimits<float>::Max();
            for (int32 Index = 0; Index < Endpoints.Num(); ++Index)
            {
                const float DistanceSquared = FVector::DistSquared(Endpoints[Index], TargetHit.ImpactPoint);
                if (DistanceSquared < NearestDistanceSquared)
                {
                    NearestDistanceSquared = DistanceSquared;
                    NearestIndex = Index;
                }
            }
            if (NearestIndex == INDEX_NONE || NearestDistanceSquared > FMath::Square(65.0f))
            {
                Result.Message = TEXT("Preview invalid: aim near a structural endpoint");
                return Result;
            }

            const FVector AnchorPoint = Endpoints[NearestIndex];
            AddUniqueSupportActor(SupportActors, Anchor);
            if (bVertical)
            {
                AKalmalaConstructionActor* ConstructionSupport = nullptr;
                AActor* GroundSupport = nullptr;
                if (!IsSupportedBeamEndpoint(World, Pawn, AnchorPoint, ConstructionSupport, GroundSupport))
                {
                    Result.Message = TEXT("Preview invalid: the vertical beam needs a supported lower endpoint");
                    return Result;
                }
                AddUniqueSupportActor(SupportActors, ConstructionSupport);
                AddUniqueSupportActor(SupportActors, GroundSupport);
                Result.Location = AnchorPoint + FVector(0.0f, 0.0f, TimberBeamHalfLength);
            }
            else
            {
                const FVector Start = AnchorPoint + FVector(0.0f, 0.0f, TimberBeamHalfWidth);
                bool bFoundSupportedSpan = false;
                for (const float Sign : {1.0f, -1.0f})
                {
                    const FVector End = Start + Axis * (TimberBeamHalfLength * 2.0f * Sign);
                    AKalmalaConstructionActor* StartConstruction = nullptr;
                    AKalmalaConstructionActor* EndConstruction = nullptr;
                    AActor* StartGround = nullptr;
                    AActor* EndGround = nullptr;
                    if (!IsSupportedBeamEndpoint(World, Pawn, Start, StartConstruction, StartGround)
                        || !IsSupportedBeamEndpoint(World, Pawn, End, EndConstruction, EndGround)) continue;
                    if (Sign < 0.0f) Result.Rotation.Yaw = FRotator::NormalizeAxis(Result.Rotation.Yaw + 180.0f);
                    Result.Location = (Start + End) * 0.5f;
                    AddUniqueSupportActor(SupportActors, StartConstruction);
                    AddUniqueSupportActor(SupportActors, EndConstruction);
                    AddUniqueSupportActor(SupportActors, StartGround);
                    AddUniqueSupportActor(SupportActors, EndGround);
                    bFoundSupportedSpan = true;
                    break;
                }
                if (!bFoundSupportedSpan)
                {
                    Result.Message = TEXT("Preview invalid: both horizontal beam endpoints need ground or structural support");
                    return Result;
                }
            }
        }
        else
        {
            FHitResult GroundHit;
            if (bVertical)
            {
                if (!TraceTerrainSupport(World, Pawn, GroundProbe, GroundHit)
                    || GroundHit.ImpactNormal.Z < 0.85f)
                {
                    Result.Message = TEXT("Preview invalid: vertical beams need firm, level ground");
                    return Result;
                }
                Result.Location = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, TimberBeamHalfLength);
                AddUniqueSupportActor(SupportActors, GroundHit.GetActor());
            }
            else
            {
                FHitResult StartGround;
                FHitResult EndGround;
                const FVector StartProbe = GroundProbe - Axis * TimberBeamHalfLength;
                const FVector EndProbe = GroundProbe + Axis * TimberBeamHalfLength;
                if (!TraceTerrainSupport(World, Pawn, StartProbe, StartGround)
                    || !TraceTerrainSupport(World, Pawn, EndProbe, EndGround)
                    || StartGround.ImpactNormal.Z < 0.85f || EndGround.ImpactNormal.Z < 0.85f
                    || FMath::Abs(StartGround.ImpactPoint.Z - EndGround.ImpactPoint.Z) > 30.0f)
                {
                    Result.Message = TEXT("Preview invalid: both horizontal beam ends need level ground or structural support");
                    return Result;
                }
                Result.Location = (StartGround.ImpactPoint + EndGround.ImpactPoint) * 0.5f
                    + FVector(0.0f, 0.0f, TimberBeamHalfWidth);
                AddUniqueSupportActor(SupportActors, StartGround.GetActor());
                AddUniqueSupportActor(SupportActors, EndGround.GetActor());
            }
        }

        const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
        if (!State || !State->GetWorldGenerationConfig().IsValid()) return Result;
        const auto& Config = State->GetWorldGenerationConfig();
        const FVector2D Surface(Result.Location);
        if (!FKalmalaWorldBounds::Contains(Config, Surface, 300))
        {
            Result.Message = TEXT("Too close to the world edge");
            return Result;
        }
        if (FKalmalaOceanSampler::Sample(Config, Surface).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, Surface)
            || FKalmalaRegionalGeneration::Sample(Config, Surface).bHasWater)
        {
            Result.Message = TEXT("Preview invalid: dry ground or a supported structure is required");
            return Result;
        }

        FCollisionQueryParams OverlapQuery(SCENE_QUERY_STAT(TimberBeamPlacement), false, Pawn);
        for (AActor* Support : SupportActors) OverlapQuery.AddIgnoredActor(Support);
        const FCollisionShape BeamShape = FCollisionShape::MakeBox(
            FVector(TimberBeamHalfLength, TimberBeamHalfWidth, TimberBeamHalfWidth));
        if (World->OverlapBlockingTestByChannel(Result.Location, Result.Rotation.Quaternion(), ECC_Pawn,
            BeamShape, OverlapQuery))
        {
            Result.Message = TEXT("Preview invalid: clear more space around the Timber Beam");
            return Result;
        }
        Result.bIsValid = true;
        Result.Message = bVertical
            ? TEXT("Preview valid: vertical beam snaps to a supported lower endpoint")
            : TEXT("Preview valid: horizontal beam snaps between supported endpoints");
        return Result;
    }
}

FRotator FKalmalaPlacementPreview::GetTimberBeamPlacementRotation(const FRotator ViewRotation)
{
    if (ViewRotation.ContainsNaN()) return FRotator::ZeroRotator;
    const bool bVertical = FMath::Abs(ViewRotation.Pitch) >= 60.0f;
    const float SnappedYaw = static_cast<float>(FMath::RoundToInt(ViewRotation.Yaw / 90.0f) * 90);
    return FRotator(bVertical ? 90.0f : 0.0f, SnappedYaw, 0.0f);
}

FRotator FKalmalaPlacementPreview::GetTimberStairsPlacementRotation(const FRotator ViewRotation)
{
    if (ViewRotation.ContainsNaN()) return FRotator::ZeroRotator;
    const float SnappedYaw = static_cast<float>(FMath::RoundToInt(ViewRotation.Yaw / 90.0f) * 90);
    return FRotator(0.0f, SnappedYaw, 0.0f);
}

bool FKalmalaPlacementPreview::IsSupportedKit(const FName ItemId)
{
    if (ItemId == TEXT("ForgeKit")) return true;
    return ItemId == TEXT("CampfireKit") || ItemId == TEXT("WorkbenchKit") || ItemId == TEXT("StorageKit")
        || ItemId == TEXT("CharcoalKilnKit") || ItemId == TEXT("SmelterKit")
        || ItemId == TEXT("CookingRackKit") || ItemId == TEXT("CauldronKit") || ItemId == TEXT("FryingPanKit")
        || ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit") || ItemId == TEXT("GrindingStoneKit")
        || ItemId == TEXT("FloorKit") || ItemId == TEXT("WallKit") || ItemId == TEXT("RoofKit")
        || ItemId == TEXT("GardenPlotKit") || ItemId == TEXT("TimberDoorKit") || ItemId == TEXT("TimberBeamKit")
        || ItemId == TEXT("TimberStairsKit") || ItemId == TEXT("CampBedKit")
        || ItemId == TEXT("StandingLightKit");
}

bool FKalmalaPlacementPreview::IsGardenPlotGroundAllowed(const EKalmalaBiome Biome, const float SurfaceNormalZ)
{
    constexpr float MinimumUpComponentFor15Degrees = 0.9659258263f;
    const bool bAllowedBiome = Biome == EKalmalaBiome::Meadows || Biome == EKalmalaBiome::Elderwood
        || Biome == EKalmalaBiome::ShimmeringLakes;
    return bAllowedBiome && FMath::IsFinite(SurfaceNormalZ)
        && SurfaceNormalZ >= MinimumUpComponentFor15Degrees && SurfaceNormalZ <= 1.0f;
}

bool FKalmalaPlacementPreview::IsSessionOnlyKit(const FName ItemId)
{
    return ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit");
}

FKalmalaPlacementPreview FKalmalaPlacementPreview::Evaluate(const UWorld* World, const APawn* Pawn, const FName ItemId)
{
    FKalmalaPlacementPreview Result;
    Result.Message = TEXT("Preview: select a camp or construction object");
    if (ItemId == TEXT("StandingLightKit")) return EvaluateStandingLight(World, Pawn);
    if (ItemId == TEXT("CampBedKit")) return EvaluateCampBed(World, Pawn);
    if (ItemId == TEXT("TimberBeamKit")) return EvaluateTimberBeam(World, Pawn);
    if (ItemId == TEXT("TimberStairsKit")) return EvaluateTimberStairs(World, Pawn);
    if (!IsSupportedKit(ItemId)) return Result;
    if (Pawn) Result.Rotation = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f);
    Result.Message = TEXT("Preview unavailable: waiting for local terrain");
    if (!World || !Pawn || Pawn->GetActorLocation().ContainsNaN()) return Result;
    if (ItemId == TEXT("TimberDoorKit"))
    {
        Result.Message = TEXT("Preview invalid: target an existing Windbreak Wall segment");
        const FVector ViewLocation = Pawn->GetPawnViewLocation();
        const FVector ViewDirection = Pawn->GetViewRotation().Vector();
        if (ViewLocation.ContainsNaN() || ViewDirection.ContainsNaN()) return Result;
        FCollisionQueryParams DoorQuery(SCENE_QUERY_STAT(TimberDoorPreview), false, Pawn);
        FHitResult WallHit;
        if (!World->LineTraceSingleByChannel(WallHit, ViewLocation,
                ViewLocation + ViewDirection * 250.0f, ECC_Visibility, DoorQuery)) return Result;
        AKalmalaConstructionActor* Wall = Cast<AKalmalaConstructionActor>(WallHit.GetActor());
        if (!IsValid(Wall) || Wall->GetWorld() != World || Wall->GetConstructionKit() != TEXT("WallKit")
            || Wall->GetConstructionId().IsEmpty()
            || FVector::DistSquared(Pawn->GetActorLocation(), Wall->GetActorLocation()) > FMath::Square(200.0f)) return Result;
        Result.Location = Wall->GetActorLocation();
        Result.ReplacementWall = Wall;
        Result.bIsValid = true;
        Result.Message = TEXT("Preview valid: replaces this Windbreak Wall segment; server will recheck before payment");
        return Result;
    }
    const bool bGardenPlot = ItemId == TEXT("GardenPlotKit");

    const FVector Forward = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Vector();
    const FVector Probe = Pawn->GetActorLocation() + Forward * 165.0f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(LocalConstructionPreview), false, Pawn);
    FHitResult Ground;
    Result.Message = TEXT("Preview invalid: face clear, dry, gently sloping ground");
    if (Probe.ContainsNaN() || !World->LineTraceSingleByChannel(Ground, Probe + FVector(0, 0, 100), Probe - FVector(0, 0, 350), ECC_Visibility, Query)
        || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor())
        || (!bGardenPlot && Ground.ImpactNormal.Z < 0.85f)) return Result;

    const auto* State = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return Result;
    const auto& Config = State->GetWorldGenerationConfig();
    const FVector2D Surface(Ground.ImpactPoint);
    if (!FKalmalaWorldBounds::Contains(Config, Surface, 300)) { Result.Message = TEXT("Too close to the world edge"); return Result; }
    if (FKalmalaOceanSampler::Sample(Config, Surface).IsWater() || FKalmalaShimmeringLakeSampler::IsWater(Config, Surface)
        || (FKalmalaRegionalGeneration::Sample(Config, Surface).bHasWater))
    {
        Result.Message = TEXT("Preview invalid: dry ground is required");
        return Result;
    }

    if (bGardenPlot)
    {
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Surface));
        if (!IsGardenPlotGroundAllowed(Biome, Ground.ImpactNormal.Z))
        {
            Result.Message = TEXT("Preview invalid: Garden Plot needs dry Meadows, Elderwood, or Shimmering Lakes ground within 15 degrees");
            return Result;
        }
        const FVector SkyStart = Ground.ImpactPoint + FVector(0, 0, 25.0f);
        FHitResult Overhead;
        if (World->LineTraceSingleByChannel(Overhead, SkyStart, SkyStart + FVector(0, 0, 100000.0f), ECC_Visibility, Query))
        {
            Result.Message = TEXT("Preview invalid: Garden Plot needs open sky");
            return Result;
        }
        Query.AddIgnoredActor(Ground.GetActor());
    }
    else if (ItemId == TEXT("WallKit"))
        Query.AddIgnoredActor(Ground.GetActor());

    const bool bIsStationAttachment = ItemId == TEXT("WorkbenchToolRackKit") || ItemId == TEXT("ForgeAnvilKit");
    const bool bIsKiln = ItemId == TEXT("CharcoalKilnKit");
    const bool bIsSmelter = ItemId == TEXT("SmelterKit");
    const bool bNeedsProcessingClearance = bIsKiln || bIsSmelter;
    const bool bIsWallSegment = ItemId == TEXT("WallKit");
    const bool bIsCompactConstruction = bGardenPlot || bIsStationAttachment || bNeedsProcessingClearance
        || ItemId == TEXT("GrindingStoneKit") || ItemId == TEXT("FryingPanKit") || bIsWallSegment;
    const FVector PlacementExtent = bIsCompactConstruction
        ? AKalmalaConstructionActor::GetCollisionExtent(ItemId) : FVector(54, 54, 56);
    const float PlacementHalfHeight = PlacementExtent.Z
        + (bIsCompactConstruction && !bGardenPlot && !bIsWallSegment ? 2.0f : 0.0f);
    Result.Location = Ground.ImpactPoint + FVector(0, 0, PlacementHalfHeight);
    const float PreviewRadius = bIsCompactConstruction
        ? FMath::Max3(PlacementExtent.X, PlacementExtent.Y, PlacementExtent.Z) : 54.0f;
    const FCollisionShape PlacementShape = (bNeedsProcessingClearance || bGardenPlot || bIsWallSegment)
        ? FCollisionShape::MakeBox(PlacementExtent) : FCollisionShape::MakeSphere(PreviewRadius);
    if (World->OverlapBlockingTestByChannel(Result.Location, FQuat::Identity, ECC_Pawn, PlacementShape, Query))
    {
        Result.Message = TEXT("Preview invalid: clear more space");
        return Result;
    }
    if (bNeedsProcessingClearance)
    {
        const FVector OverheadCentre = Result.Location + FVector(0, 0, PlacementExtent.Z + 100.0f);
        const FCollisionShape OverheadClearance = FCollisionShape::MakeBox(
            FVector(PlacementExtent.X, PlacementExtent.Y, 100.0f));
        if (World->OverlapBlockingTestByChannel(OverheadCentre, FQuat::Identity, ECC_Pawn, OverheadClearance, Query))
        {
            Result.Message = bIsKiln ? TEXT("Preview invalid: Charcoal Kiln needs 2 m of overhead clearance")
                : TEXT("Preview invalid: Smelter needs 2 m of overhead clearance");
            return Result;
        }
        const FVector ApproachCentre = Result.Location - Forward * (PlacementExtent.X + 50.0f)
            + FVector(0, 0, 50.0f - PlacementExtent.Z);
        const FCollisionShape ApproachClearance = FCollisionShape::MakeBox(FVector(50.0f, 40.0f, 50.0f));
        const FQuat ApproachRotation = FRotator(0.0f, Pawn->GetActorRotation().Yaw, 0.0f).Quaternion();
        if (World->OverlapBlockingTestByChannel(ApproachCentre, ApproachRotation, ECC_Pawn,
            ApproachClearance, Query))
        {
            Result.Message = bIsKiln ? TEXT("Preview invalid: keep a clear 1 m approach to the Charcoal Kiln")
                : TEXT("Preview invalid: keep a clear 1 m approach to the Smelter");
            return Result;
        }
    }
    Result.bIsValid = true;
    Result.Message = TEXT("Preview valid locally: server will recheck before placement");
    return Result;
}
