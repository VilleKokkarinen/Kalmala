#include "KalmalaWorldMapWidget.h"

#include "Async/Async.h"
#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "KalmalaWorldMapExplorationSaveGame.h"
#include "KalmalaWorldMapPinsSaveGame.h"
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

FVector2D UKalmalaWorldMapWidget::WorldToMapNormalized(const FVector2D WorldPosition, const FVector2D MapCentre, const FVector2D MapExtent)
{
    if (!FMath::IsFinite(WorldPosition.X) || !FMath::IsFinite(WorldPosition.Y) || MapExtent.X <= 0.0f || MapExtent.Y <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }
    return (WorldPosition - (MapCentre - MapExtent)) / (MapExtent * 2.0f);
}

FVector2D UKalmalaWorldMapWidget::GetFacingDirection(const float FacingDegrees)
{
    const float FacingRadians = FMath::DegreesToRadians(FMath::IsFinite(FacingDegrees) ? FacingDegrees : 0.0f);
    return FVector2D(FMath::Cos(FacingRadians), FMath::Sin(FacingRadians));
}

float UKalmalaWorldMapWidget::ChooseGridSpacing(const float InMapRadius)
{
    const float SafeRadius = FMath::Max(0.0f, InMapRadius);
    return SafeRadius <= 7000.0f ? 2500.0f : SafeRadius <= 20000.0f ? 5000.0f : 10000.0f;
}

FString UKalmalaWorldMapWidget::SanitizePinLabel(FString Label)
{
    Label.TrimStartAndEndInline();
    FString Sanitized;
    Sanitized.Reserve(FMath::Min(Label.Len(), MaxPinLabelLength));
    for (const TCHAR Character : Label)
    {
        if (Sanitized.Len() >= MaxPinLabelLength) break;
        if (FChar::IsAlnum(Character) || Character == TEXT(' ') || Character == TEXT('-') || Character == TEXT('\'')) Sanitized.AppendChar(Character);
    }
    Sanitized.TrimStartAndEndInline();
    return Sanitized;
}

bool UKalmalaWorldMapWidget::IsValidPinLabel(const FString& Label)
{
    return !Label.IsEmpty() && Label.Len() <= MaxPinLabelLength && Label == SanitizePinLabel(Label);
}

FString UKalmalaWorldMapWidget::GetPinAccessibilityLabel(const FKalmalaWorldMapPersonalPin& Pin)
{
    const TCHAR* StyleName = Pin.Style == EKalmalaWorldMapPinStyle::Lantern ? TEXT("Lantern")
        : Pin.Style == EKalmalaWorldMapPinStyle::Thread ? TEXT("Thread") : TEXT("Cairn");
    return FString::Printf(TEXT("%s marker: %s; %s; %s"), StyleName, *Pin.Label,
        Pin.bComplete ? TEXT("complete") : TEXT("active"), Pin.bVisible ? TEXT("shown") : TEXT("hidden"));
}

FColor UKalmalaWorldMapWidget::GetFogTreatmentColor(const EKalmalaWorldMapFogTreatment Treatment)
{
    switch (Treatment)
    {
    case EKalmalaWorldMapFogTreatment::CurrentPersonal:
        return FColor(0, 0, 0, 0);
    case EKalmalaWorldMapFogTreatment::RememberedPersonal:
        // Sea-glass teal keeps personal memory visible without making it read as current sight.
        return FColor(20, 78, 70, 112);
    case EKalmalaWorldMapFogTreatment::ReservedShared:
        // Warm lichen-ember is intentionally distinct; no shared data is rendered yet.
        return FColor(106, 68, 32, 112);
    default:
        return FColor(8, 18, 24, 255);
    }
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
        const bool bCurrentlyVisible = IsWithinLocalRevealRadius(WorldPosition, OwningPawnLocation, LocalRevealRadius);
        const bool bPersonallyExplored = Exploration != nullptr && Exploration->IsExplored(WorldPosition);
        // Fully opaque pixels disclose neither sampled terrain nor water treatment. Personal
        // memory is tinted separately from current sight; shared coverage has no source yet.
        Pixels.Add(GetFogTreatmentColor(bCurrentlyVisible ? EKalmalaWorldMapFogTreatment::CurrentPersonal
            : bPersonallyExplored ? EKalmalaWorldMapFogTreatment::RememberedPersonal : EKalmalaWorldMapFogTreatment::Unexplored));
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

FString UKalmalaWorldMapWidget::GetPinsSaveSlot(const FKalmalaWorldGenerationConfig& Config) const
{
    const int32 PlayerIndex = GetOwningPlayer() != nullptr ? GetOwningPlayer()->GetLocalPlayer()->GetControllerId() : 0;
    return FString::Printf(TEXT("KalmalaPersonalMapPins_v1_%llu_%d_%d"), Config.WorldSeed, Config.GeneratorRevision, PlayerIndex);
}

void UKalmalaWorldMapWidget::EnsureExplorationForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    if (ExplorationSave != nullptr && ExplorationSave->MatchesWorld(Config)) return;
    ExplorationSave = Cast<UKalmalaWorldMapExplorationSaveGame>(UGameplayStatics::LoadGameFromSlot(GetExplorationSaveSlot(Config), 0));
    bExplorationLoadedForWorld = ExplorationSave != nullptr && ExplorationSave->MatchesWorld(Config);
    if (!bExplorationLoadedForWorld)
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

void UKalmalaWorldMapWidget::EnsurePinsForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    if (PinsSave != nullptr && PinsSave->MatchesWorld(Config)) return;
    PinsSave = Cast<UKalmalaWorldMapPinsSaveGame>(UGameplayStatics::LoadGameFromSlot(GetPinsSaveSlot(Config), 0));
    if (PinsSave != nullptr && PinsSave->MatchesWorld(Config))
    {
        LocalPins = PinsSave->GetPins();
        if (!LocalPins.IsValidIndex(SelectedPinIndex)) SelectedPinIndex = INDEX_NONE;
        return;
    }
    PinsSave = NewObject<UKalmalaWorldMapPinsSaveGame>(this);
    PinsSave->InitializeForWorld(Config);
    LocalPins.Reset();
    SelectedPinIndex = INDEX_NONE;
}

void UKalmalaWorldMapWidget::PersistPins()
{
    if (PinsSave != nullptr && PinsSave->MatchesWorld(TileConfig) && PinsSave->SetPins(LocalPins))
    {
        UGameplayStatics::SaveGameToSlot(PinsSave, GetPinsSaveSlot(TileConfig), 0);
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
    EnsurePinsForWorld(Config);
    RecordLocalExploration(PawnLocation);
    UpdateFogTexture(Centre, Extent, PawnLocation, ViewModel->GetMapSampleDimensions());
    if (!bDeveloperFogVerificationLogged && bDeveloperVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
    {
        const FVector2D RemoteProbe = PawnLocation + FVector2D(LocalRevealRadius * 2.0f + 1.0f, 0.0f);
        const bool bRemoteExplored = ExplorationSave != nullptr && ExplorationSave->IsExplored(RemoteProbe);
        bDeveloperFogVerificationLogged = true;
        UE_LOG(LogTemp, Display, TEXT("World map fog presentation: Seed=%d Revision=%d Cells=%d Loaded=%d Remote=%d."),
            Config.WorldSeed, Config.GeneratorRevision, ExplorationSave != nullptr ? ExplorationSave->GetExploredCellCount() : 0,
            bExplorationLoadedForWorld ? 1 : 0, bRemoteExplored ? 1 : 0);
    }
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
        const float GridSpacing = ChooseGridSpacing(MapZoom);
        const FVector2D WorldMinimum = Centre - Extent;
        const FVector2D WorldMaximum = Centre + Extent;
        const FLinearColor GridColour(0.42f, 0.72f, 0.67f, 0.16f);
        for (float X = FMath::FloorToFloat(WorldMinimum.X / GridSpacing) * GridSpacing; X <= WorldMaximum.X; X += GridSpacing)
        {
            const float NormalizedX = WorldToMapNormalized(FVector2D(X, Centre.Y), Centre, Extent).X;
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 1, MapGeometry,
                { FVector2D(NormalizedX * MapSize.X, 0.0f), FVector2D(NormalizedX * MapSize.X, MapSize.Y) }, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }
        for (float Y = FMath::FloorToFloat(WorldMinimum.Y / GridSpacing) * GridSpacing; Y <= WorldMaximum.Y; Y += GridSpacing)
        {
            const float NormalizedY = WorldToMapNormalized(FVector2D(Centre.X, Y), Centre, Extent).Y;
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 1, MapGeometry,
                { FVector2D(0.0f, NormalizedY * MapSize.Y), FVector2D(MapSize.X, NormalizedY * MapSize.Y) }, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }
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
        DrawPins(AllottedGeometry, MapSize, DrawLayer + 3, OutDrawElements);
        if (bDeveloperVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("World map painted: Size=%.0fx%.0f Map=%.0fx%.0f Tiles=%d."), Size.X, Size.Y, MapSize.X, MapSize.Y, Tiles.Num());
        }
    }
    FVector2D PawnLocation;
    FKalmalaWorldGenerationConfig PresentationConfig;
    if (ViewModel != nullptr && ViewModel->GetPresentationInputs(PresentationConfig, PawnLocation))
    {
        const FVector2D NormalizedPawn = WorldToMapNormalized(PawnLocation, ViewModel->GetMapCentre(), ViewModel->GetMapExtent());
        if (NormalizedPawn.X >= 0.0f && NormalizedPawn.X <= 1.0f && NormalizedPawn.Y >= 0.0f && NormalizedPawn.Y <= 1.0f)
        {
            const FVector2D MarkerCentre = FVector2D(Margin.Left, Margin.Top) + NormalizedPawn * MapSize;
            const FVector2D Forward = GetFacingDirection(ViewModel->GetPlayerFacingDegrees());
            const FVector2D Right(-Forward.Y, Forward.X);
            const TArray<FVector2D> MarkerLines = { MarkerCentre + Forward * 15.0f, MarkerCentre - Forward * 9.0f + Right * 8.0f,
                MarkerCentre - Forward * 9.0f - Right * 8.0f, MarkerCentre + Forward * 15.0f };
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 4, AllottedGeometry.ToPaintGeometry(), MarkerLines,
                ESlateDrawEffect::None, FLinearColor(0.02f, 0.04f, 0.05f, 1.0f), true, 5.0f);
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 5, AllottedGeometry.ToPaintGeometry(), MarkerLines,
                ESlateDrawEffect::None, FLinearColor(0.86f, 0.96f, 0.9f, 1.0f), true, 2.5f);
        }
    }
    const FString Hint = FString::Printf(TEXT("MAP  |  %.0fm  |  Grid %.0fm  |  Drag/Arrows pan · Wheel/PgUp zoom · R recenter · M / Esc close"),
        MapZoom / 100.0f, ChooseGridSpacing(MapZoom) / 100.0f);
    FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 5, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(20.0f, 18.0f))), Hint,
        FCoreStyle::GetDefaultFontStyle("Regular", 16), ESlateDrawEffect::None, FLinearColor(0.85f, 0.91f, 0.87f, 1.0f));
    if (LocalPins.IsValidIndex(SelectedPinIndex))
    {
        const FString SelectedHint = FString::Printf(TEXT("SELECTED: %s  |  Enter complete · H show/hide · Delete remove"),
            *GetPinAccessibilityLabel(LocalPins[SelectedPinIndex]));
        FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 6, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(20.0f, 42.0f))), SelectedHint,
            FCoreStyle::GetDefaultFontStyle("Regular", 14), ESlateDrawEffect::None, FLinearColor(0.92f, 0.8f, 0.58f, 1.0f));
    }
    if (bPinLabelEntry)
    {
        const FString Draft = PendingPinLabel.IsEmpty() ? TEXT("Name marker…") : PendingPinLabel;
        const FString Prompt = FString::Printf(TEXT("MARKER: %s  |  1 Cairn · 2 Lantern · 3 Thread · Enter save · Esc cancel"), *Draft);
        FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 7, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(20.0f, Size.Y - 38.0f))), Prompt,
            FCoreStyle::GetDefaultFontStyle("Regular", 14), ESlateDrawEffect::None, FLinearColor(0.9f, 0.82f, 0.62f, 1.0f));
    }
    return DrawLayer + 7;
}

FVector2D UKalmalaWorldMapWidget::ScreenToWorld(const FVector2D& ScreenPosition, const FVector2D& MapSize) const
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return FVector2D::ZeroVector;
    const FVector2D Normalized = ScreenPosition / MapSize;
    return ViewModel->GetMapCentre() + (Normalized * 2.0f - FVector2D(1.0f, 1.0f)) * ViewModel->GetMapExtent();
}

int32 UKalmalaWorldMapWidget::FindVisiblePinAtScreenPosition(const FVector2D& ScreenPosition, const FVector2D& MapSize) const
{
    if (ViewModel == nullptr) return INDEX_NONE;
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    for (int32 Index = LocalPins.Num() - 1; Index >= 0; --Index)
    {
        const FKalmalaWorldMapPersonalPin& Pin = LocalPins[Index];
        if (!Pin.bVisible) continue;
        const FVector2D PinScreen = WorldToMapNormalized(Pin.WorldLocation, Centre, Extent) * MapSize;
        if (FVector2D::DistSquared(PinScreen, ScreenPosition) <= FMath::Square(16.0f)) return Index;
    }
    return INDEX_NONE;
}

void UKalmalaWorldMapWidget::BeginPinPlacement(const FVector2D& WorldLocation)
{
    bDragging = false;
    bPinLabelEntry = true;
    PendingPinLocation = WorldLocation;
    PendingPinLabel.Reset();
    PendingPinStyle = EKalmalaWorldMapPinStyle::Cairn;
}

void UKalmalaWorldMapWidget::CommitPinPlacement()
{
    const FString Label = SanitizePinLabel(PendingPinLabel);
    if (!IsValidPinLabel(Label)) return;
    while (LocalPins.Num() >= UKalmalaWorldMapPinsSaveGame::MaxPersonalPins) LocalPins.RemoveAt(0);
    FKalmalaWorldMapPersonalPin& Pin = LocalPins.AddDefaulted_GetRef();
    Pin.WorldLocation = PendingPinLocation;
    Pin.Label = Label;
    Pin.Style = PendingPinStyle;
    SelectedPinIndex = LocalPins.Num() - 1;
    bPinLabelEntry = false;
    PendingPinLabel.Reset();
    PersistPins();
}

void UKalmalaWorldMapWidget::SelectNextPin()
{
    if (LocalPins.IsEmpty()) { SelectedPinIndex = INDEX_NONE; return; }
    SelectedPinIndex = LocalPins.IsValidIndex(SelectedPinIndex) ? (SelectedPinIndex + 1) % LocalPins.Num() : 0;
}

bool UKalmalaWorldMapWidget::ToggleSelectedPinCompletion()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins[SelectedPinIndex].bComplete = !LocalPins[SelectedPinIndex].bComplete;
    PersistPins();
    return true;
}

bool UKalmalaWorldMapWidget::ToggleSelectedPinVisibility()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins[SelectedPinIndex].bVisible = !LocalPins[SelectedPinIndex].bVisible;
    PersistPins();
    return true;
}

bool UKalmalaWorldMapWidget::RemoveSelectedPin()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins.RemoveAt(SelectedPinIndex);
    SelectedPinIndex = LocalPins.IsEmpty() ? INDEX_NONE : FMath::Min(SelectedPinIndex, LocalPins.Num() - 1);
    PersistPins();
    return true;
}

void UKalmalaWorldMapWidget::PanByKeyboardDelta(const FVector2D& ScreenDelta)
{
    const FVector2D MapSize = GetCachedGeometry().GetLocalSize() - FVector2D(88.0f);
    PanByScreenDelta(ScreenDelta, MapSize);
}

void UKalmalaWorldMapWidget::ZoomAtMapCentre(const float WheelDelta)
{
    const FVector2D MapSize = GetCachedGeometry().GetLocalSize() - FVector2D(88.0f);
    ZoomAtScreenPosition(WheelDelta, MapSize * 0.5f, MapSize);
}

void UKalmalaWorldMapWidget::CancelPinPlacement()
{
    bPinLabelEntry = false;
    PendingPinLabel.Reset();
}

FLinearColor UKalmalaWorldMapWidget::GetPinColour(const EKalmalaWorldMapPinStyle Style)
{
    switch (Style)
    {
    case EKalmalaWorldMapPinStyle::Lantern: return FLinearColor(0.93f, 0.68f, 0.28f, 1.0f);
    case EKalmalaWorldMapPinStyle::Thread: return FLinearColor(0.7f, 0.5f, 0.82f, 1.0f);
    default: return FLinearColor(0.48f, 0.84f, 0.73f, 1.0f);
    }
}

void UKalmalaWorldMapWidget::DrawPins(const FGeometry& AllottedGeometry, const FVector2D& MapSize, const int32 LayerId,
    FSlateWindowElementList& OutDrawElements) const
{
    if (ViewModel == nullptr) return;
    const FVector2D Margin(44.0f, 44.0f);
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    for (int32 Index = 0; Index < LocalPins.Num(); ++Index)
    {
        const FKalmalaWorldMapPersonalPin& Pin = LocalPins[Index];
        if (!Pin.bVisible) continue;
        const FVector2D Normalized = WorldToMapNormalized(Pin.WorldLocation, Centre, Extent);
        if (Normalized.X < 0.0f || Normalized.X > 1.0f || Normalized.Y < 0.0f || Normalized.Y > 1.0f) continue;
        const FVector2D Position = Margin + Normalized * MapSize;
        const TArray<FVector2D> Diamond = { Position + FVector2D(0.0f, -8.0f), Position + FVector2D(7.0f, 0.0f),
            Position + FVector2D(0.0f, 8.0f), Position + FVector2D(-7.0f, 0.0f), Position + FVector2D(0.0f, -8.0f) };
        const FLinearColor Colour = GetPinColour(Pin.Style).CopyWithNewOpacity(Pin.bComplete ? 0.45f : 1.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Diamond, ESlateDrawEffect::None, FLinearColor::Black, true, 4.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Diamond, ESlateDrawEffect::None, Colour, true, 2.0f);
        if (Pin.bComplete) FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
            { Position + FVector2D(-4.0f, 0.0f), Position + FVector2D(-1.0f, 3.0f), Position + FVector2D(5.0f, -4.0f) }, ESlateDrawEffect::None, FLinearColor::White, true, 1.5f);
        if (Index == SelectedPinIndex) FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
            AllottedGeometry.ToPaintGeometry(FVector2D(22.0f, 22.0f), FSlateLayoutTransform(Position - FVector2D(11.0f))),
            FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.2f));
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(Position + FVector2D(10.0f, -8.0f))),
            GetPinAccessibilityLabel(Pin),
            FCoreStyle::GetDefaultFontStyle("Regular", 12), ESlateDrawEffect::None, FLinearColor::White);
    }
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
    if (!bMapOpen) return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    const FVector2D MapSize = InGeometry.GetLocalSize() - FVector2D(88.0f);
    const FVector2D MapPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f);
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (const int32 PinIndex = FindVisiblePinAtScreenPosition(MapPosition, MapSize); PinIndex != INDEX_NONE)
        {
            LocalPins.RemoveAt(PinIndex);
            SelectedPinIndex = LocalPins.IsEmpty() ? INDEX_NONE : FMath::Min(PinIndex, LocalPins.Num() - 1);
            PersistPins();
        }
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (const int32 PinIndex = FindVisiblePinAtScreenPosition(MapPosition, MapSize); PinIndex != INDEX_NONE)
        {
            SelectedPinIndex = PinIndex;
            if (InMouseEvent.IsControlDown()) LocalPins[PinIndex].bVisible = !LocalPins[PinIndex].bVisible;
            else LocalPins[PinIndex].bComplete = !LocalPins[PinIndex].bComplete;
            PersistPins();
            return FReply::Handled();
        }
        if (InMouseEvent.IsShiftDown()) { BeginPinPlacement(ScreenToWorld(MapPosition, MapSize)); return FReply::Handled(); }
        bDragging = true; LastDragPosition = MapPosition; return FReply::Handled().CaptureMouse(TakeWidget());
    }
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
    const FVector2D Position = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f);
    PanByScreenDelta(Position - LastDragPosition, InGeometry.GetLocalSize() - FVector2D(88.0f)); LastDragPosition = Position;
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (bPinLabelEntry)
    {
        if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom) { CommitPinPlacement(); return FReply::Handled(); }
        if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right) { CancelPinPlacement(); return FReply::Handled(); }
        if (Key == EKeys::BackSpace) { PendingPinLabel.LeftChopInline(1); return FReply::Handled(); }
        if (Key == EKeys::One) { PendingPinStyle = EKalmalaWorldMapPinStyle::Cairn; return FReply::Handled(); }
        if (Key == EKeys::Two) { PendingPinStyle = EKalmalaWorldMapPinStyle::Lantern; return FReply::Handled(); }
        if (Key == EKeys::Three) { PendingPinStyle = EKalmalaWorldMapPinStyle::Thread; return FReply::Handled(); }
        return FReply::Unhandled();
    }
    if (!bMapOpen) return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    if (Key == EKeys::Tab || Key == EKeys::Gamepad_FaceButton_Left) { SelectNextPin(); return FReply::Handled(); }
    if (Key == EKeys::P) { BeginPinPlacement(ScreenToWorld((InGeometry.GetLocalSize() - FVector2D(88.0f)) * 0.5f, InGeometry.GetLocalSize() - FVector2D(88.0f))); return FReply::Handled(); }
    if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        if (!ToggleSelectedPinCompletion()) BeginPinPlacement(ScreenToWorld((InGeometry.GetLocalSize() - FVector2D(88.0f)) * 0.5f, InGeometry.GetLocalSize() - FVector2D(88.0f)));
        return FReply::Handled();
    }
    if (Key == EKeys::H || Key == EKeys::Gamepad_FaceButton_Right) { ToggleSelectedPinVisibility(); return FReply::Handled(); }
    if (Key == EKeys::Delete || Key == EKeys::Gamepad_LeftTrigger) { RemoveSelectedPin(); return FReply::Handled(); }
    if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Top) { Recenter(); return FReply::Handled(); }
    if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left) { PanByKeyboardDelta(FVector2D(64.0f, 0.0f)); return FReply::Handled(); }
    if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right) { PanByKeyboardDelta(FVector2D(-64.0f, 0.0f)); return FReply::Handled(); }
    if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) { PanByKeyboardDelta(FVector2D(0.0f, -64.0f)); return FReply::Handled(); }
    if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) { PanByKeyboardDelta(FVector2D(0.0f, 64.0f)); return FReply::Handled(); }
    if (Key == EKeys::PageUp || Key == EKeys::Gamepad_RightShoulder) { ZoomAtMapCentre(1.0f); return FReply::Handled(); }
    if (Key == EKeys::PageDown || Key == EKeys::Gamepad_LeftShoulder) { ZoomAtMapCentre(-1.0f); return FReply::Handled(); }
    return FReply::Unhandled();
}

FReply UKalmalaWorldMapWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
    if (!bPinLabelEntry) return Super::NativeOnKeyChar(InGeometry, InCharacterEvent);
    const TCHAR Character = InCharacterEvent.GetCharacter();
    if (PendingPinLabel.Len() < MaxPinLabelLength && (FChar::IsAlnum(Character) || Character == TEXT(' ') || Character == TEXT('-') || Character == TEXT('\'')))
    {
        PendingPinLabel.AppendChar(Character);
    }
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bMapOpen) return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    ZoomAtScreenPosition(InMouseEvent.GetWheelDelta(), InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f), InGeometry.GetLocalSize() - FVector2D(88.0f));
    return FReply::Handled();
}
