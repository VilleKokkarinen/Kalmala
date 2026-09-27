#pragma once

#include "CoreMinimal.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"

class AKalmalaConstructionActor;

/** Buildable station family required by an authored tool recipe. */
enum class EKalmalaToolStationKind : uint8
{
    None,
    Workbench,
    Forge
};

/** One catalogue item and quantity in a server-authored tool recipe. */
struct KALMALAGAMEPLAY_API FKalmalaToolMaterialCost
{
    FName ItemId = NAME_None;
    int32 Quantity = 0;
};

/** Static progression data; it does not itself grant or upgrade a carried tool. */
struct KALMALAGAMEPLAY_API FKalmalaToolProgressionEntry
{
    FName ToolId = NAME_None;
    int32 TargetToolLevel = 0;
    FName PreviousToolId = NAME_None;
    int32 PreviousToolLevel = 0;
    EKalmalaToolStationKind RequiredStation = EKalmalaToolStationKind::None;
    int32 RequiredStationLevel = 0;
    TArray<FKalmalaToolMaterialCost> MaterialCosts;
};

/** A paid placed object that raises exactly one compatible station tier. */
struct KALMALAGAMEPLAY_API FKalmalaStationAttachmentDefinition
{
    FName KitId = NAME_None;
    FName StationKitId = NAME_None;
};

/** Minimal world observation used by the deterministic station-level rule. */
struct KALMALAGAMEPLAY_API FKalmalaStationAttachmentCandidate
{
    FName KitId = NAME_None;
    float DistanceToStationCm = TNumericLimits<float>::Max();
    bool bSameWorld = false;
    bool bInitialized = false;
};

/** Bounded authored progression catalogue for M9's two tiered axes. */
class KALMALAGAMEPLAY_API FKalmalaToolProgressionContract
{
public:
    static constexpr int32 MaxAxeProgressionEntries = 2;
    static constexpr int32 MaxStationAttachments = 32;
    static constexpr int32 MaxStationLevel = 2;
    static constexpr float MaxAttachmentDistanceCm = 125.0f;

    static const TArray<FKalmalaToolProgressionEntry>& GetEntries();
    static const TArray<FKalmalaStationAttachmentDefinition>& GetAttachmentDefinitions();
    static const FKalmalaToolProgressionEntry* FindEntry(FName ToolId);
    static const FKalmalaStationAttachmentDefinition* FindAttachment(FName KitId);
    static bool IsStationAttachmentKit(FName KitId);
    static FName GetAttachmentStationKit(FName AttachmentKitId);
    static FName GetStationKit(EKalmalaToolStationKind Station);
    static int32 GetBaseStationLevel(FName KitId);
    static int32 DeriveEffectiveStationLevel(
        FName StationKitId,
        const TArray<FKalmalaStationAttachmentCandidate>& Attachments);
    static int32 GetEffectiveStationLevel(const AKalmalaConstructionActor* Station);
    static bool CanPlaceAttachment(
        FName AttachmentKitId,
        FName StationKitId,
        float DistanceToStationCm,
        bool bStationUsable,
        bool bAlreadyUpgraded,
        FString& Reason);
    static bool IsCatalogueValid();
    static bool BuildServerUpgrade(
        bool bServerAuthority,
        FName SelectedStationKit,
        int32 EffectiveStationLevel,
        FName ToolId,
        const TArray<FKalmalaToolState>& ExistingTools,
        const TArray<FKalmalaInventoryStack>& ExistingInventory,
        TArray<FKalmalaToolState>& OutTools,
        TArray<FKalmalaInventoryStack>& OutInventory,
        FString& Reason);
};
