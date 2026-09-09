#include "KalmalaWorldMapWidget.h"

#include "Async/Async.h"
#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "KalmalaWorldMapExplorationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Rendering/DrawElements.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void UKalmalaWorldMapWidget::InitializeForLocalPlayer(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr || !InOwningPlayer->IsLocalController()) return;
    SetOwningPlayer(InOwningPlayer);
    ViewModel = NewObject<UKalmalaMinimapViewModel>(this);
    ViewModel->Initialize(InOwningPlayer);
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->SetMapSampleDimensions(FIntPoint(161, 91));
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaWorldMapWidget::ConfigureViewportPlacement()
{
    SetAlignmentInViewport(FVector2D::ZeroVector);
    SetPositionInViewport(FVector2D::ZeroVector, false);
    SetDesiredSizeInViewport(FVector2D(1920.0f, 1080.0f));
    SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
}

void UKalmalaWorldMapWidget::Open()
{
    if (GetOwningPlayer() == nullptr || ViewModel == nullptr) return;
    bMapOpen = true;
    Recenter();
    SetVisibility(ESlateVisibility::Visible);
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    Controller->SetInputMode(InputMode);
    Controller->SetIgnoreMoveInput(true);
    Controller->SetIgnoreLookInput(true);
}

void UKalmalaWorldMapWidget::Close()
{
    if (!bMapOpen) return;
    bMapOpen = false;
    bDragging = false;
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->SetShowMouseCursor(false);
        Controller->SetInputMode(FInputModeGameOnly());
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
    }
}

void UKalmalaWorldMapWidget::Recenter()
{
    if (ViewModel == nullptr) return;
    ViewModel->RecenterOnOwningPlayer();
    ViewModel->SetMapRadius(MapZoom);
    InvalidateOutstandingTileJobs();
}

void UKalmalaWorldMapWidget::RunDeveloperVerification()
{
    if (!bMapOpen || ViewModel == nullptr || bDeveloperVerificationLogged) return;
    const FVector2D InitialCentre = ViewModel->GetMapCentre();
    ZoomAtScreenPosition(1000.0f, FVector2D(400.0f, 300.0f), FVector2D(800.0f, 600.0f));
    const bool bMinimumZoom = FMath::IsNearlyEqual(MapZoom, MinZoom);
    ZoomAtScreenPosition(-1000.0f, FVector2D(400.0f, 300.0f), FVector2D(800.0f, 600.0f));
    const bool bMaximumZoom = FMath::IsNearlyEqual(MapZoom, MaxZoom);
    PanByScreenDelta(FVector2D(160.0f, -120.0f), FVector2D(800.0f, 600.0f));
    const bool bPanChangedCentre = !ViewModel->GetMapCentre().Equals(InitialCentre, 1.0f);
    Recenter();
    const bool bRecentered = ViewModel->GetMapCentre().Equals(InitialCentre, 1.0f);
    bDeveloperVerificationLogged = true;
    UE_LOG(LogTemp, Display, TEXT("World map verification: Open=%d Input=%d ZoomMin=%d ZoomMax=%d Pan=%d Recenter=%d."),
        bMapOpen ? 1 : 0, GetOwningPlayer() && GetOwningPlayer()->IsMoveInputIgnored() && GetOwningPlayer()->IsLookInputIgnored() ? 1 : 0,
        bMinimumZoom ? 1 : 0, bMaximumZoom ? 1 : 0, bPanChangedCentre ? 1 : 0, bRecentered ? 1 : 0);
}

float UKalmalaWorldMapWidget::ClampMapZoom(const float RequestedZoom, const float InMinZoom, const float InMaxZoom)
{
    return FMath::Clamp(RequestedZoom, FMath::Min(InMinZoom, InMaxZoom), FMath::Max(InMinZoom, InMaxZoom));
}

bool UKalmalaWorldMapWidget::IsWithinLocalRevealRadius(const FVector2D WorldPosition, const FVector2D OwningPawnLocation, const float RevealRadius)
{
    return RevealRadius > 0.0f && FMath::IsFinite(WorldPosition.X) && FMath::IsFinite(WorldPosition.Y)
        && FMath::IsFinite(OwningPawnLocation.X) && FMath::IsFinite(OwningPawnLocation.Y)
        && FVector2D::DistSquared(WorldPosition, OwningPawnLocation) <= FMath::Square(RevealRadius);
}

TArray<FColor> UKalmalaWorldMapWidget::BuildFogPixels(const FVector2D MapCentre, const FVector2D MapExtent,
    const FVector2D OwningPawnLocation, const FIntPoint Dimensions, const UKalmalaWorldMapExplorationSaveGame* Exploration)
{
    TArray<FColor> Pixels;
    if (Dimensions.X <= 0 || Dimensions.Y <= 0 || MapExtent.X <= 0.0f || MapExtent.Y <= 0.0f) return Pixels;
    Pixels.Reserve(Dimensions.X * Dimensions.Y);
    for (int32 Y = 0; Y < Dimensions.Y; ++Y) for (int32 X = 0; X < Dimensions.X; ++X)
    {
        const FVector2D Normalized(
            Dimensions.X > 1 ? static_cast<float>(X) / static_cast<float>(Dimensions.X - 1) * 2.0f - 1.0f : 0.0f,
            Dimensions.Y > 1 ? static_cast<float>(Y) / static_cast<float>(Dimensions.Y - 1) * 2.0f - 1.0f : 0.0f);
        const FVector2D WorldPosition = MapCentre + Normalized * MapExtent;
        // Fully opaque pixels disclose neither sampled terrain nor water treatment.
        Pixels.Add((IsWithinLocalRevealRadius(WorldPosition, OwningPawnLocation, LocalRevealRadius)
                || (Exploration != nullptr && Exploration->IsExplored(WorldPosition)))
            ? FColor(0, 0, 0, 0) : FColor(8, 18, 24, 255));
    }
    return Pixels;
}

TArray<FColor> UKalmalaWorldMapWidget::BuildTilePixels(const FKalmalaWorldGenerationConfig Config, const FIntPoint TileCoordinate)
{
    const FVector2D Centre = (FVector2D(TileCoordinate) + FVector2D(0.5f)) * TileWorldSize;
    return FKalmalaMinimapRaster::BuildPixels(UKalmalaMinimapViewModel::BuildTerrainSamples(
        Config, Centre, FVector2D(TileWorldSize * 0.5f), FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis)),
        FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis));
}

void UKalmalaWorldMapWidget::InvalidateOutstandingTileJobs()
{
    ++TileEpoch;
    if (TileEpoch == 0) ++TileEpoch;
    // The old view can have a full pending set. Drop those local handles so
    // a pan or zoom cannot accumulate multiple capped request sets; workers
    // retain their epoch and their output is never uploaded into the new view.
    Tiles.Reset();
}

void UKalmalaWorldMapWidget::StartTile(const FIntPoint& TileCoordinate, const FKalmalaWorldGenerationConfig& Config)
{
    FWorldMapTile& Tile = Tiles.FindOrAdd(TileCoordinate);
    if (Tile.PendingPixels.IsValid() || Tile.Texture != nullptr) return;
    Tile.RequestEpoch = TileEpoch;
    Tile.PendingPixels = Async(EAsyncExecution::ThreadPool, [Config, TileCoordinate]() { return BuildTilePixels(Config, TileCoordinate); });
}

void UKalmalaWorldMapWidget::UploadCompletedTiles()
{
    for (TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        FWorldMapTile& Tile = Pair.Value;
        if (!Tile.PendingPixels.IsValid() || !Tile.PendingPixels.IsReady()) continue;
        TArray<FColor> Pixels = Tile.PendingPixels.Get();
        Tile.PendingPixels = {};
        // A stale worker never reaches the render resource after pan, zoom,
        // identity, or player-centre changes.
        if (Tile.RequestEpoch != TileEpoch || Pixels.Num() != TileSamplesPerAxis * TileSamplesPerAxis) continue;
        Tile.PixelHash = FCrc::MemCrc32(Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Tile.Texture = TStrongObjectPtr<UTexture2D>(UTexture2D::CreateTransient(TileSamplesPerAxis, TileSamplesPerAxis, PF_B8G8R8A8));
        if (Tile.Texture == nullptr) continue;
        Tile.Texture->SRGB = true; Tile.Texture->Filter = TF_Bilinear; Tile.Texture->NeverStream = true; Tile.Texture->UpdateResource();
        const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
        uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount)); FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
        auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, TileSamplesPerAxis, TileSamplesPerAxis);
        Tile.Texture->UpdateTextureRegions(0, 1, Region, TileSamplesPerAxis * sizeof(FColor), sizeof(FColor), Upload,
            [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
    }
}

void UKalmalaWorldMapWidget::UpdateFogTexture(const FVector2D MapCentre, const FVector2D MapExtent,
    const FVector2D OwningPawnLocation, const FIntPoint Dimensions)
{
    const TArray<FColor> Pixels = BuildFogPixels(MapCentre, MapExtent, OwningPawnLocation, Dimensions, ExplorationSave);
    if (Pixels.Num() != Dimensions.X * Dimensions.Y) return;
    if (FogTexture != nullptr && (FogTexture->GetSizeX() != Dimensions.X || FogTexture->GetSizeY() != Dimensions.Y))
    {
        FogBrush.SetResourceObject(nullptr);
        FogTexture = nullptr;
    }
    if (FogTexture == nullptr)
    {
        FogTexture = TStrongObjectPtr<UTexture2D>(UTexture2D::CreateTransient(Dimensions.X, Dimensions.Y, PF_B8G8R8A8));
        if (FogTexture == nullptr) return;
        FogTexture->SRGB = true; FogTexture->Filter = TF_Bilinear; FogTexture->NeverStream = true; FogTexture->UpdateResource();
        FogBrush.SetResourceObject(FogTexture.Get()); FogBrush.ImageSize = FVector2D(Dimensions); FogBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    if (FogTexture->GetResource() == nullptr) return;
    const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
    uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount)); FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
    auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Dimensions.X, Dimensions.Y);
    FogTexture->UpdateTextureRegions(0, 1, Region, Dimensions.X * sizeof(FColor), sizeof(FColor), Upload,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
}

FString UKalmalaWorldMapWidget::GetExplorationSaveSlot(const FKalmalaWorldGenerationConfig& Config) const
{
    const int32 PlayerIndex = GetOwningPlayer() != nullptr ? GetOwningPlayer()->GetLocalPlayer()->GetControllerId() : 0;
    return FString::Printf(TEXT("KalmalaPersonalMapCoverage_v1_%llu_%d_%d"), Config.WorldSeed, Config.GeneratorRevision, PlayerIndex);
}

void UKalmalaWorldMapWidget::EnsureExplorationForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    if (ExplorationSave != nullptr && ExplorationSave->MatchesWorld(Config)) return;
    ExplorationSave = Cast<UKalmalaWorldMapExplorationSaveGame>(UGameplayStatics::LoadGameFromSlot(GetExplorationSaveSlot(Config), 0));
    if (ExplorationSave == nullptr || !ExplorationSave->MatchesWorld(Config))
    {
        ExplorationSave = NewObject<UKalmalaWorldMapExplorationSaveGame>(this);
        ExplorationSave->InitializeForWorld(Config);
    }
}

void UKalmalaWorldMapWidget::RecordLocalExploration(const FVector2D OwningPawnLocation)
{
    if (ExplorationSave != nullptr && ExplorationSave->RecordReveal(OwningPawnLocation, LocalRevealRadius))
    {
        UGameplayStatics::SaveGameToSlot(ExplorationSave, GetExplorationSaveSlot(TileConfig), 0);
    }
}

void UKalmalaWorldMapWidget::EvictUnusedTiles()
{
    if (Tiles.Num() <= MaxCachedTiles) return;
    TArray<FIntPoint> Evictable;
    for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        if (Pair.Value.LastUsedEpoch != TileEpoch && !Pair.Value.PendingPixels.IsValid()) Evictable.Add(Pair.Key);
    }
    Evictable.Sort([](const FIntPoint& Left, const FIntPoint& Right) { return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X; });
    for (const FIntPoint& Key : Evictable)
    {
        if (Tiles.Num() <= MaxCachedTiles) break;
        Tiles.Remove(Key);
    }
}

TArray<FIntPoint> UKalmalaWorldMapWidget::BuildPrioritizedTileCoordinates(const FVector2D& Centre, const FVector2D& Extent)
{
    const FIntPoint Min(FMath::FloorToInt((Centre.X - Extent.X) / TileWorldSize), FMath::FloorToInt((Centre.Y - Extent.Y) / TileWorldSize));
    const FIntPoint Max(FMath::FloorToInt((Centre.X + Extent.X) / TileWorldSize), FMath::FloorToInt((Centre.Y + Extent.Y) / TileWorldSize));
    TArray<FIntPoint> Coordinates;
    for (int32 Y = Min.Y; Y <= Max.Y; ++Y) for (int32 X = Min.X; X <= Max.X; ++X) Coordinates.Add(FIntPoint(X, Y));
    Coordinates.Sort([Centre](const FIntPoint& Left, const FIntPoint& Right)
    {
        const FVector2D LeftCentre = (FVector2D(Left) + FVector2D(0.5f)) * TileWorldSize;
        const FVector2D RightCentre = (FVector2D(Right) + FVector2D(0.5f)) * TileWorldSize;
        const double LeftDistance = FVector2D::DistSquared(LeftCentre, Centre);
        const double RightDistance = FVector2D::DistSquared(RightCentre, Centre);
        if (!FMath::IsNearlyEqual(LeftDistance, RightDistance)) return LeftDistance < RightDistance;
        return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X;
    });
    Coordinates.SetNum(FMath::Min(Coordinates.Num(), MaxCachedTiles));
    return Coordinates;
}

void UKalmalaWorldMapWidget::LogDeveloperTileFingerprint()
{
    if (bDeveloperTileFingerprintLogged || !bDeveloperVerificationLogged || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification"))) return;
    TArray<FIntPoint> Coordinates;
    uint32 Digest = 0;
    for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        if (Pair.Value.LastUsedEpoch != TileEpoch) continue;
        if (Pair.Value.Texture == nullptr) return;
        Coordinates.Add(Pair.Key);
    }
    if (Coordinates.IsEmpty()) return;
    Coordinates.Sort([](const FIntPoint& Left, const FIntPoint& Right) { return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X; });
    for (const FIntPoint& Coordinate : Coordinates)
    {
        const FWorldMapTile& Tile = Tiles.FindChecked(Coordinate);
        Digest = FCrc::MemCrc32(&Coordinate, sizeof(Coordinate), Digest);
        Digest = FCrc::MemCrc32(&Tile.PixelHash, sizeof(Tile.PixelHash), Digest);
    }
    bDeveloperTileFingerprintLogged = true;
    UE_LOG(LogTemp, Display, TEXT("World map tile presentation: Seed=%d Revision=%d Tiles=%d Fingerprint=%u PollOnly=1."),
        TileConfig.WorldSeed, TileConfig.GeneratorRevision, Coordinates.Num(), Digest);
}

void UKalmalaWorldMapWidget::RefreshTiles(const FVector2D& MapSize)
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    FKalmalaWorldGenerationConfig Config;
    FVector2D PawnLocation;
    if (!ViewModel->GetPresentationInputs(Config, PawnLocation))
    {
        if (!bDeveloperTileInputsUnavailableLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
        {
            bDeveloperTileInputsUnavailableLogged = true;
            UE_LOG(LogTemp, Display, TEXT("World map tile inputs are not ready."));
        }
        return;
    }
    if (!(Config == TileConfig)) { TileConfig = Config; InvalidateOutstandingTileJobs(); }
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    EnsureExplorationForWorld(Config);
    RecordLocalExploration(PawnLocation);
    UpdateFogTexture(Centre, Extent, PawnLocation, ViewModel->GetMapSampleDimensions());
    for (const FIntPoint& Key : BuildPrioritizedTileCoordinates(Centre, Extent))
    {
        FWorldMapTile* ExistingTile = Tiles.Find(Key);
        if (ExistingTile == nullptr && Tiles.Num() >= MaxCachedTiles) continue;
        FWorldMapTile& Tile = Tiles.FindOrAdd(Key); Tile.LastUsedEpoch = TileEpoch; StartTile(Key, Config);
    }
    UploadCompletedTiles();
    EvictUnusedTiles();
}

void UKalmalaWorldMapWidget::TickTilePresentation(const float DeltaTime)
{
    if (!bMapOpen || ViewModel == nullptr) return;
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    if (APlayerController* Controller = GetOwningPlayer()) Controller->GetViewportSize(ViewportWidth, ViewportHeight);
    const FVector2D MapSize = FVector2D(ViewportWidth, ViewportHeight) - FVector2D(88.0f);
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const float AspectRatio = MapSize.Y > 0.0f ? MapSize.X / MapSize.Y : 1.0f;
    ViewModel->SetMapAspectRatio(AspectRatio);
    ViewModel->SetMapSampleDimensions(FIntPoint(161, FMath::Clamp(FMath::RoundToInt(161.0f / AspectRatio), 61, 129)));
    RefreshAccumulator += DeltaTime;
    if (RefreshAccumulator >= 0.10f) { RefreshAccumulator = 0.0f; RefreshTiles(MapSize); LogDeveloperTileFingerprint(); }
}

void UKalmalaWorldMapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TickTilePresentation(InDeltaTime);
    if (bDeveloperVerificationLogged && !bRequestedVerificationScreenshot)
    {
        VerificationElapsed += InDeltaTime;
        FString ScreenshotPath;
        if (VerificationElapsed >= 3.0f && FParse::Value(FCommandLine::Get(), TEXT("KalmalaWorldMapScreenshot="), ScreenshotPath))
        {
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
            bRequestedVerificationScreenshot = true;
        }
    }
}

int32 UKalmalaWorldMapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FMargin Margin(44.0f);
    const FVector2D MapSize(FMath::Max(1.0f, Size.X - Margin.Left - Margin.Right), FMath::Max(1.0f, Size.Y - Margin.Top - Margin.Bottom));
    const FPaintGeometry MapGeometry = AllottedGeometry.ToPaintGeometry(MapSize, FSlateLayoutTransform(FVector2D(Margin.Left, Margin.Top)));
    FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None, FLinearColor(0.008f, 0.018f, 0.025f, 0.96f));
    if (ViewModel != nullptr)
    {
        const FVector2D Centre = ViewModel->GetMapCentre();
        const FVector2D Extent = ViewModel->GetMapExtent();
        for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles) if (Pair.Value.Texture != nullptr)
        {
            const FVector2D WorldMin = FVector2D(Pair.Key) * TileWorldSize;
            const FVector2D Offset = (WorldMin - (Centre - Extent)) / (Extent * 2.0f);
            const FVector2D TileFraction(TileWorldSize / (Extent.X * 2.0f), TileWorldSize / (Extent.Y * 2.0f));
            const FPaintGeometry TileGeometry = AllottedGeometry.ToPaintGeometry(MapSize * TileFraction,
                FSlateLayoutTransform(FVector2D(Margin.Left, Margin.Top) + Offset * MapSize));
            FSlateBrush TileBrush;
            TileBrush.SetResourceObject(Pair.Value.Texture.Get()); TileBrush.ImageSize = FVector2D(TileSamplesPerAxis); TileBrush.DrawAs = ESlateBrushDrawType::Image;
            FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 1, TileGeometry, &TileBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        if (FogTexture != nullptr)
        {
            FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 2, MapGeometry, &FogBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        if (bDeveloperVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("World map painted: Size=%.0fx%.0f Map=%.0fx%.0f Tiles=%d."), Size.X, Size.Y, MapSize.X, MapSize.Y, Tiles.Num());
        }
    }
    FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 3, MapGeometry, FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None, FLinearColor(0.55f, 0.75f, 0.68f, 0.9f));
    const FVector2D Centre = FVector2D(Margin.Left, Margin.Top) + MapSize * 0.5f;
    TArray<FVector2D> Cross;
    Cross.Add(Centre - FVector2D(12.0f, 0.0f)); Cross.Add(Centre + FVector2D(12.0f, 0.0f));
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 4, AllottedGeometry.ToPaintGeometry(), Cross, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
    Cross = { Centre - FVector2D(0.0f, 12.0f), Centre + FVector2D(0.0f, 12.0f) };
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 4, AllottedGeometry.ToPaintGeometry(), Cross, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
    const FString Hint = FString::Printf(TEXT("MAP  |  %.0fm  |  Drag to pan · Wheel to zoom · R to recenter · M / Esc to close"), MapZoom / 100.0f);
    FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 5, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(20.0f, 18.0f))), Hint,
        FCoreStyle::GetDefaultFontStyle("Regular", 16), ESlateDrawEffect::None, FLinearColor(0.85f, 0.91f, 0.87f, 1.0f));
    return DrawLayer + 5;
}

void UKalmalaWorldMapWidget::PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D Extent = ViewModel->GetMapExtent();
    const FVector2D WorldDelta(-ScreenDelta.X / MapSize.X * 2.0f * Extent.X, -ScreenDelta.Y / MapSize.Y * 2.0f * Extent.Y);
    ViewModel->SetMapCentre(ViewModel->GetMapCentre() + WorldDelta);
    InvalidateOutstandingTileJobs();
}

void UKalmalaWorldMapWidget::ZoomAtScreenPosition(const float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || FMath::IsNearlyZero(WheelDelta) || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D Normalized((ScreenPosition.X / MapSize.X - 0.5f) * 2.0f, (ScreenPosition.Y / MapSize.Y - 0.5f) * 2.0f);
    const FVector2D PinnedWorld = ViewModel->GetMapCentre() + Normalized * ViewModel->GetMapExtent();
    MapZoom = ClampMapZoom(MapZoom * (1.0f - WheelDelta * ZoomStep), MinZoom, MaxZoom);
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->SetMapCentre(PinnedWorld - Normalized * ViewModel->GetMapExtent());
    InvalidateOutstandingTileJobs();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bMapOpen && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) { bDragging = true; LastDragPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()); return FReply::Handled().CaptureMouse(TakeWidget()); }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) { bDragging = false; return FReply::Handled().ReleaseMouseCapture(); }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bDragging) return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
    const FVector2D Position = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    PanByScreenDelta(Position - LastDragPosition, InGeometry.GetLocalSize() - FVector2D(88.0f)); LastDragPosition = Position;
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bMapOpen) return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    ZoomAtScreenPosition(InMouseEvent.GetWheelDelta(), InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f), InGeometry.GetLocalSize() - FVector2D(88.0f));
    return FReply::Handled();
}
